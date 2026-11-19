# Entering the binary layer and framing

Part of the binary-layer documentation set (`docs/protocol/binary/`).  This
page owns the entry sequence, the wiring the layer uses, the shared reply and
ACK rules, the byte framing, and the way back to the terminal.  The command
bytes themselves are in [commands.md](commands.md) (BBIO1) and
[sub-protocols.md](sub-protocols.md) (the entries and shared groups).

## Scope

- **Covered:** how a host enters and leaves the layer, what the shared result
  bytes mean, how request bytes are framed, and the hardware state the layer
  starts in.
- **Not covered:** the per-protocol command sets.  Each mode owns its own
  `binary.md`; the entry bytes and links are in
  [sub-protocols.md](sub-protocols.md).

## Entering from the terminal

The layer is entered from the terminal prompt.  Once it is active the terminal
text parser is no longer running.

1. Make sure the Bus Pirate is at a mode prompt (for example `HiZ>`).  If a
   settings prompt is waiting for an answer, press Enter until the prompt
   appears, then send `#` to reset to the idle state.
2. Send **20 consecutive `0x00`** bytes.  The Bus Pirate replies `BBIO1`.
3. Wait for the five identifier bytes before sending the first request.

Notes:

- A NUL cannot be typed in a normal terminal; a host program or script sends
  the sequence.  The terminal-side rule is in
  [generic/commands.md](../../generic/commands.md#leaving-the-cli--binary-and-sump-entry)
  §Leaving the CLI — binary and SUMP entry.
- The count is **consecutive**: any byte other than NUL or the SUMP STX resets
  it, including Enter.  A run of fewer than twenty NULs produces no reply and
  no error.
- The counter is shared with the SUMP entry: 5–19 NULs followed by a raw
  `0x02` (STX) selects SUMP instead.  Twenty NULs always win, so a `0x02`
  after them arrives in BBIO1 and selects binary I2C.
- Extra NULs are harmless: every `0x00` sent to BBIO1 re-prints `BBIO1`.  A
  robust host can therefore send NULs until the identifier appears.
- `BBIO1` means **binary bitbang, protocol version 1**.  It is five bytes with
  no line ending.

What entry changes:

| Element | State after entry |
|---|---|
| header lines AUX, MOSI, CLK, MISO, CS | inputs (Hi-Z); output latches low |
| target supply (VREG) | off |
| pull-up resistors | off |
| on-board Vpullup source (v4) | off |
| AUX target selection | unchanged (the terminal's selection persists) |
| a PWM/servo generator left running | not driven any more; timer setup not cleared |
| an SMPS feedback loop | not stopped (it starts only in raw wire) |

The entry reset runs again before each sub-protocol entry.  The XSVF entry is
the exception: it skips that reset, configures only TDI, TDO, TCK and TMS, and
switches the on-board regulator on; the other header pins and the pull-ups are
left alone.

## Wiring

The layer uses the same five header lines as the mode it selects, plus the ADC
probe pin, the Vpullup reference and ground:

| Pin | Binary-layer role |
|---|---|
| AUX | pin I/O; the `0x70` wait-for-edge and the state-byte AUX bit |
| MOSI | data out (and, in raw wire, the bidirectional data line) |
| CLK | clock out |
| MISO | data in |
| CS | chip select |
| ADC | probe input for `0x14` / `0x15`; 0–6.6 V, 2:1 divider (v3 `unverified`) |
| Vpullup (`Vpu`) | pull-up reference: external 0–5 V, or `0x5x` in a sub-protocol (v4) |
| GND | common ground with the target |

Whether a **pull-up or supply is required** depends on what the host is doing,
not on the layer alone:

| Use | Pull-up | Target supply |
|---|---|---|
| BBIO1 pin I/O with the pull-ups on | resistors on + external Vpullup | only if powered from the Bus Pirate |
| open-drain bus in a sub-protocol | resistors on + Vpullup (v4: `0x5x`; v3 external) | as above |
| push-pull bus | none | as above |
| ADC probe | none | none |
| edge wait | none (the source drives the pin) | as above |

Important consequences:

- **Entry switches the on-board Vpullup source off.**  In BBIO1 the `0x5x`
  group is not available (the whole `0x40`–`0x5F` range is a pin direction
  byte), so a BBIO1-only session can only feed the pull-up resistors from an
  external source on Vpullup.  Inside a sub-protocol, send `0x51` or `0x52`
  after entry to use the on-board source.
- The **pull-up resistors and the target supply are different circuits.**  The
  resistors tie the I/O lines to Vpullup; the supply switches the on-board
  regulator, the same element as terminal `W`, which puts the 3.3 V and 5.0 V
  rails on their header pins.  The state byte has one bit for each (bit 5 and
  bit 6); the sub-protocol `0x4x` group has one bit for each (bit 2 and bit 3).
- If the target is self-powered, keep the Bus Pirate supply off.  Unlike `W`,
  the binary supply bit runs no rail check and no bus setup; it is applied as
  sent.
- Vpullup accepts 0–5 V.  On v4 the `0x5x` request first measures the pin and
  refuses while a voltage is already present.
- The ADC probe pin is an input behind a divider; it needs no pull-up and must
  stay within 0–6.6 V.  The ADC set owns the scale and accuracy.

## Reply and ACK rules

| Reply | Meaning |
|---|---|
| `0x01` | success / accepted / acknowledgement |
| `0x00` | failure / refused |
| one or more data bytes | the value a read command returns |

The table is the usual case, not a universal rule: a reply can also be a count
(`0x10` / `0x11`), a pin level (`0x00`–`0x1F`) or a bus ACK bit, where `0x00`
is a normal value.  The full set of shapes is in
[commands.md](commands.md#reply-shapes).

- There is **no frame marker, length byte or checksum**.  Each request has a
  fixed reply shape, and the host must know it to keep the stream aligned.
- A command that reads a value returns the **data instead of** a result byte.
  Do not wait for `0x01` after `0x14`, `0x16` or a mode read command.
- Some commands acknowledge with `0x01` **before** their data.  Examples are
  I2C write-then-read (`0x01`, then the read bytes) and the shared `0x1x` bulk
  groups (`0x01`, then a per-byte reply).  The command table for the mode
  states the exact shape.
- A `0x00` failure is the same byte value as a legitimate data `0x00`.  Only
  its position and the command's known reply length distinguish them.
- A pin **state** or **direction** byte is answered with one level byte,
  `0x00`–`0x1F`: the five pin levels, class bit not echoed.
- In BBIO1, `PIC1` and the nibble-group modes, an unrecognised command is
  answered with a single `0x00`; the layer stays in its current mode and keeps
  reading.  `OCD1` also answers a single `0x00`.  Only the `XSV1` loop ignores
  unknown bytes.
- Outside the deliberate streams (ADC `0x15`, the I2C sniffer, the UART
  bridge), the layer emits nothing it was not asked for.

## Framing

In BBIO1 the request byte's class is selected by its top bits:

| Pattern | Class | Reply |
|---|---|---|
| `1xxxxxxx` (`0x80`–`0xFF`) | pin **state** byte | one level byte |
| `010xxxxx` (`0x40`–`0x5F`) | pin **direction** byte | one level byte |
| all other low-bit bytes | core **command** | per command |

In the five nibble-group sub-protocols (SPI, I2C, UART, 1-Wire, raw wire) the
request byte splits into a **high-nibble command group** and a **low-nibble
operand**.  Variable-length payloads follow where the command defines them.
The dedicated loops (`PIC1`, `OCD1`, `XSV1`) do not use this split; their own
pages define their byte layouts.

Other framing rules:

- Multi-byte values are **big-endian** (most significant byte first) unless a
  mode page states otherwise: ADC samples, frequency counts, wait timeouts,
  bulk counts and the SMPS sample.
- After a BBIO1 state or direction byte the firmware waits about **5 µs**
  before replying, so the returned levels are settled.  The sub-protocol
  configure groups reply immediately.
- The state-byte reply reports the **actual pin levels**, not the latched
  values.  A floating input can read high; a driven line can be held by the
  target.
- There is **no read-back command**: nothing reports the current pin
  directions, speed, output type or wire count.  The settings must be tracked
  on the host.  The levels come back only as the reply to a state or direction
  byte, which is a write — there is no read-only five-pin read
  ([commands.md](commands.md#reading-pins)).

## Leaving

| From | Send | Result |
|---|---|---|
| BBIO1 | `0x0F` | `0x01`, then a reset to the terminal |
| SPI, I2C, UART, 1-Wire, raw wire, `PIC1`, `OCD1` | `0x00` | back to BBIO1; `BBIO1` printed |
| `XSV1` | `0x0F` | back to BBIO1 **silently**; `0x00` ignored |
| AVRISP-MKII (`0x08` / `0x09`) | none | idle timeout (about 5 s) or reset; `BBIO1` printed |

`0x0F` always replies `0x01` before anything else happens.  Then:

- **v4** runs the **terminal reset**: pins to inputs, pull-ups released, the
  on-board Vpullup source and the target supply off, the AUX output source
  detached and the AUX assignment back to `AUX`, the display format back to
  HEX, and BASIC left.  On an SMPS-enabled build it also stops the SMPS
  feedback loop.  It is not a substitute for `0x13`: the PWM timer is left
  running, so send `0x13` first if the module must be idle.  The version
  banner and the `HiZ>` prompt follow on the same link.
- **v3** performs a hardware reset, which stops a PWM timer and any SMPS loop.
  The serial link may emit stray bytes as it comes back up; discard bytes until
  the banner, then wait for `HiZ>`.

`0x00` from a sub-protocol does **not** stop a running SMPS feedback loop; stop
it with `0xF1` while still in raw wire (see
[sub-protocols.md](sub-protocols.md#0xfx--smps-raw-wire-v4-only)).

The XSVF player's `0x0F` is not a terminal return: it resets the JTAG chain and
returns to BBIO1 without re-printing `BBIO1`; it does not reset the pins or
switch the regulator off.  Send `0x0F` a second time from BBIO1 to reach the
terminal.  See [../jtag/xsvf.md](../jtag/xsvf.md).

To use the layer again after a terminal return, send the twenty NULs again.
On v4 the terminal reset also clears the entry counter; on v3 the reset does.

## Availability

| Item | v3 | v4 |
|---|---|---|
| entry 20 × `0x00` → `BBIO1` | present, unverified | present |
| exit `0x0F` | hardware reset | terminal reset, banner and `HiZ>` |
| `0x00` re-print | present | present |
| ADC pin and Vpullup pin | present | present |
| on-board Vpullup source | — | present (and switched off by entry) |
| XSVF silent return from `0x18` | — | present |

The v3 column is **not verified** on hardware.

## Related docs

- [commands.md](commands.md) — the BBIO1 command byte map.
- [sub-protocols.md](sub-protocols.md) — the sub-protocol entries and shared
  groups.
- [edge-cases.md](edge-cases.md) — entry and stuck-state failure modes.
- [generic/commands.md](../../generic/commands.md#leaving-the-cli--binary-and-sump-entry)
  §Leaving the CLI — binary and SUMP entry — the terminal-side view of the
  entry sequence.
- [generic/README.md](../../generic/README.md) — Vpullup, `W`/`P`/`e` and the
  idle state.
