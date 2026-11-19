# Binary sub-protocols and shared groups

Part of the binary-layer documentation set (`docs/protocol/binary/`).  This page
lists the sub-protocol **entry bytes** and identifiers, maps them to the
terminal modes, and owns the **cross-mode semantics of the shared command
groups** (`0x0x`–`0x8x`, plus the raw-wire PIC and SMPS groups).  Entry,
replies and framing are in [entering-and-framing.md](entering-and-framing.md);
the BBIO1 core map is in [commands.md](commands.md).

## Entry bytes

Send the entry byte to the bitbang layer, after `BBIO1`.  The mode prints its
identifier and starts its own command loop; `0x00` returns to `BBIO1`.  The
XSVF player is the exception: it ignores `0x00` and leaves with `0x0F`.

| Entry byte | Sub-protocol | Identifier | Command set owned by |
|---|---|---|---|
| `0x01` | SPI | `SPI1` | [SPI binary command set](../spi/binary.md) |
| `0x02` | I2C | `I2C1` | [I2C binary command set](../i2c/binary.md) |
| `0x03` | UART | `ART1` | [UART binary command set](../uart/binary.md) |
| `0x04` | 1-Wire | `1W01` | [1-Wire binary command set](../wire/1wire/binary.md) |
| `0x05` | raw wire (2-Wire and 3-Wire) | `RAW1` | [shared `RAW1` framing](../wire/binary.md) |
| `0x06` | OpenOCD JTAG | `OCD1` | [OpenOCD path](../jtag/openocd.md) (host: [adapter](../../tools/openocd/README.md)) |
| `0x07` | PIC | `PIC1` | [PIC binary command set](../../programmers/pic/binary.md) |
| `0x08` | AVRISP-MKII USB personality (v4) | none | [AVRISP-MKII](../../programmers/avrisp/entry.md) |
| `0x09` | AVRISP-MKII serial route (v4) | none | [serial route](../../programmers/avrisp/serial-route.md) |
| `0x18` | XSVF player (v4 only) | `XSV1` | [XSVF player](../jtag/xsvf.md) |

Notes:

- Every sub-protocol entry except XSVF first runs the same hardware reset as
  BBIO1 entry, then applies the mode's own defaults.  The XSVF entry skips that
  reset: it configures only its four JTAG pins and switches the regulator on,
  leaving the other pins and the pull-ups alone.
- The AVRISP-MKII entries have **no command loop and no identifier**.  They
  hand the link to the personality, which ends after about **five seconds**
  without host traffic or on a reset; on the way out it resets the hardware and
  prints `BBIO1` again (the USB route re-enumerates first).  They are accepted
  only from BBIO1, and only on v4.  On a v3 build both bytes are answered with
  `0x00` and the layer then re-announces itself with `BBIO1`.
- `RAW1` is the **shared** raw-wire mode.  It serves 2-Wire and 3-Wire; the
  entry sequence, shared framing and worked exchange are owned by
  [the shared `RAW1` page](../wire/binary.md).  Entry always selects 2 wires,
  open drain and MSB-first, even if the terminal was left in `3WIRE`; send
  `0x8x` to change the wire count.
- OpenOCD (`0x06`) and XSVF (`0x18`) have no terminal mode — these bytes are
  the only way to reach those JTAG paths.  PIC has both a terminal mode and the
  binary `PIC1` loop; the raw-wire PIC group is a third path.
- SMPS has no entry byte of its own; it is a command group inside `RAW1` — see
  [the SMPS binary page](../../capabilities/smps/binary.md).

## Terminal modes and binary modes

The terminal modes are entered from the mode menu (`m`) and are named in the
prompt.  Their binary counterparts are separate command loops on the same
hardware; the two are not the same interface.

| Terminal mode | Binary entry | Identifier | Relationship |
|---|---|---|---|
| `HiZ` | — | — | the idle mode; the binary layer is entered from a prompt |
| `SPI` | `0x01` | `SPI1` | same bus, different command set |
| `I2C` | `0x02` | `I2C1` | same bus; entry selects `AUX` |
| `UART` | `0x03` | `ART1` | same bridge, different command set |
| `1WIRE` | `0x04` | `1W01` | same bus; the binary entry sets standard speed |
| `2WIRE` | `0x05` | `RAW1` | one shared raw-wire mode; entry is 2 wires |
| `3WIRE` | `0x05` | `RAW1` | same entry; send `0x8x` to select 3 wires |
| `PIC` | `0x07` | `PIC1` | separate engines and prompts |
| `KEYB` | — | — | no binary path |
| `LCD` (optional) | — | — | no binary path |
| `DIO` | — | — | no binary path |
| — | `0x06` | `OCD1` | no terminal mode |
| — | `0x18` | `XSV1` | no terminal mode (v4 only) |
| — | `0x08` / `0x09` | none | AVRISP-MKII personality (v4 only) |

Do not rely on terminal settings carrying into a binary mode: each mode applies
the defaults it needs on entry (speed, output type, width), so configure the
mode after entering it.  The AUX target selection is the exception — it
persists into BBIO1 and into every sub-protocol except binary I2C, which forces
it back to `AUX`; this matters for the wait
(see [commands.md](commands.md#the-aux-pin) §The AUX pin).  BASIC rides the
terminal prompt and has no binary interface; the self-test is reachable both as
the terminal `~` command and as BBIO1 `0x10` / `0x11`.

## Shared command groups

Inside the five nibble-group sub-protocols the request byte's **high nibble** is
the command group and the **low nibble** is the operand.  Several groups mean
the same thing across SPI, I2C, UART, 1-Wire and raw wire.

These groups exist **only inside a sub-protocol**.  In BBIO1 the whole
`0x40`–`0x5F` range is the pin direction byte, and `0x80`–`0xFF` is the pin
state byte, so none of these bytes is a command there.  The dedicated `PIC1`,
`OCD1` and `XSV1` loops do not use the groups either; their pages define their
own layouts.  [commands.md](commands.md#same-byte-different-layer-or-mode) §Same
byte, different layer or mode lists the collisions.

This page defines the cross-mode meaning of a group.  A mode's `binary.md` may
add per-mode operands or reply detail, but it must not change what the group
means.

### `0x0x` — actions

The action group holds the per-mode control bytes.  Two are common:

| Request | Action | Reply |
|---|---|---|
| `0x00` | exit the mode, back to `BBIO1` | `BBIO1` |
| `0x01` | re-print the mode identifier | the identifier |

The rest are mode-specific (start/stop, read, CS, sniffer, bridge, AUX
sub-commands, …).  Each mode's page owns them.

### `0x1x` — bulk byte transfer

The low nibble is `n`; the host sends `n + 1` data bytes (`n = 0` sends one,
`n = 15` sends sixteen).  The command starts with one `0x01`; the per-byte
reply then depends on the mode:

| Mode | Per-byte reply after the leading `0x01` |
|---|---|
| SPI | the byte clocked in on MISO during that write |
| I2C | the slave ACK bit: `0x00` = ACK, `0x01` = NACK |
| UART | `0x01` (the byte was accepted for transmission) |
| 1-Wire | `0x01` (the byte was written) |
| raw wire, 2 wires | `0x01` — the Bus Pirate's result, not a slave ACK |
| raw wire, 3 wires | the MISO byte sampled during that write |

The mode consumes exactly `n + 1` bytes as data.  A command byte sent early is
written to the bus as data, not executed.  The mode's page owns the exact
exchange and the error behaviour.

### `0x2x` — clocks or speed

This group differs by mode and is the clearest example of why the mode matters:

| Mode | Meaning of `0x2n` |
|---|---|
| raw wire | advance `n + 1` clock ticks; reply `0x01` |
| 1-Wire | select the speed: low bits `0` standard, `1` overdrive; `2`/`3` refused |
| SPI, I2C, UART | not present; answered `0x00` |

### `0x3x` — bulk bit transfer

Raw wire only.  The low nibble is `n`; the command writes `n + 1` bits.  The
reply is two bytes: `0x01` before the host's data byte is read and `0x01` after
the bits are written.  The firmware takes the level for every bit from **bit 7
of the command byte**, and in `0x30`–`0x3F` that bit is always 0, so the group
writes `n + 1` **low** bits.  The host still sends one byte; the firmware
consumes it and ignores its bits.  The other modes answer `0x00`.

### `0x4x` — configure peripherals

Present in SPI, I2C, UART, 1-Wire and raw wire.  The low four bits set the board
state; the reply is `0x01`.  There is no rail check — the bits are applied as
sent.

| Bit | Mask | When set | When clear |
|---|---|---|---|
| 3 | `0x08` | target supply (VREG) on | supply off |
| 2 | `0x04` | pull-up resistors on | pull-ups off |
| 1 | `0x02` | AUX high | AUX low |
| 0 | `0x01` | CS released | CS driven low (asserted) |

Details:

- This group is the sub-protocol counterpart of the BBIO1 **state byte**, but
  the bit positions differ (in the state byte, power is bit 6 and pull-up is
  bit 5).
- The AUX bit **forces `AUX` to an output**, unlike the state-byte AUX bit,
  which only sets the level and leaves the direction alone.
- Bit 0 is a level, not a pulse.  Clear drives CS low; set releases it —
  driven high in normal output, an input (Hi-Z) in open drain.  Check the
  mode page for how a later `0x6x`/`0x8x` affects the CS direction.
- The pull-up bit switches the resistors on; it does not choose their voltage.
  Select the source with `0x5x` first, exactly as `e2`/`e3` precedes `P` in the
  terminal.

For example `0x4C` = supply on + pull-ups on + CS asserted; `0x4D` is the same
with CS released.

### `0x5x` — set the pull-up feed (v4 only)

Present in SPI, I2C, UART, 1-Wire and raw wire.  This group is a **configure
action**, not a read: the low nibble selects the on-board source that feeds
Vpullup and switches it on or off.

| Operand | Source |
|---|---|
| `0x51` | on-board **+3.3 V** |
| `0x52` | on-board **+5.0 V** |
| any other | both off |

The reply is always a single byte: `0x01` when the request was applied and
`0x00` when it was refused.  The group never returns a state byte.

A request is refused when:

- the outputs are not open drain (for example after `0x8x` selected normal
  output in SPI, or after raw wire's `0x8x` cleared bit 3);
- the ADC is owned by the SMPS feedback loop (SMPS-enabled build);
- a voltage is already present on Vpullup — the firmware measures the pin and
  refuses above roughly 1.6 V.

The two "not applicable" refusals return before anything changes.  The
external-voltage case is different: both on-board sources are switched off
**before** the measurement, so a refusal caused by an external voltage has
already turned them off.

Entry leaves the source off, so send `0x5x` after entering the mode when you
need the on-board source.  The feed does not enable the pull-up resistors;
`0x4x` bit 2 does.  On a v3 build the group is not present and is answered
`0x00`.

### `0x6x` — set speed

Present in SPI, I2C, UART and raw wire; 1-Wire uses its `0x2x` group instead.
The meaning is per mode:

| Mode | Operand | Selects |
|---|---|---|
| SPI | low nibble | raw speed-table index 0–11 (the mode page lists the rates) |
| I2C | low 2 bits | software (bit-banged) profile 0–3 |
| UART | low nibble | preset 0–9 (300–115200 bps, and 31250 MIDI) |
| raw wire | low 2 bits | bit-bang profile 0–3 |

The reply is `0x01`.  An out-of-range operand is refused with `0x00` in SPI
(index 12–15) and UART (preset 10–15); I2C and raw wire mask to the low two
bits, so every operand selects a profile.  In raw wire the command also returns
CS to its released level; in SPI it re-runs the module setup, which sets CS an
output at its idle level.

### `0x7x` — wait for an edge

Present in every nibble-group mode (SPI, I2C, UART, 1-Wire, raw wire); in BBIO1
it is the single byte `0x70`.  Only sub-command `0` is defined; any other low
nibble is answered with `0x00` **without reading a payload**.

Payload and reply are owned by
[commands.md](commands.md#wait-for-an-edge-0x70) §Wait for an edge: an edge
selector, a 16-bit big-endian timeout in milliseconds, then a result byte and
the selected AUX pin level.  The selector and timeout are always consumed
before an invalid selector is refused.  The wait does not change the selected
pin's direction — configure it as an input first.

The dedicated `PIC1`, `OCD1` and `XSV1` loops do not implement this group.

### `0x8x` — mode configuration

Present in SPI, UART and raw wire (UART also answers `0x9x` with the same
configuration).  The bits are per mode; the reply is always `0x01`.

| Mode | Bits |
|---|---|
| SPI | clock polarity, edge and sample phase; output type |
| UART | data/parity, stop bits, receive polarity; output type |
| raw wire | output type, wire count, bit order |

- **SPI:** bit 0 sample at end, bit 1 edge active→idle, bit 2 clock idle high,
  bit 3 **normal** output (clear = open drain).
- **UART:** bit 4 push-pull, bits 3–2 data/parity, bit 1 two stop bits,
  bit 0 receive polarity.
- **raw wire:** bit 3 open drain, bit 2 three wires, bit 1 LSB-first.

`0x8x` is the one group where the same byte has opposite meanings: `0x88` is
normal output in SPI and open drain in `RAW1`.  It also switches CS to its idle
level in raw wire.  There is no read-back; track the configuration on the host.
The modes' pages own the full bit tables.

### `0xAx` — PIC programming (raw wire)

Raw wire only.  Four operands are defined:

| Request | Action |
|---|---|
| `0xA0` | select the PIC engine; one operand byte follows |
| `0xA4` | write command words |
| `0xA5` | write and read bits |
| `0xA7` | write and read command words |

The PIC set owns the payloads and the engine values; see
[the PIC binary page](../../programmers/pic/binary.md).  The dedicated `PIC1`
loop (`0x07`) is the other binary path.

### `0xFx` — SMPS (raw wire, v4 only)

Raw wire only.  Every `0xF0`–`0xFF` byte is an SMPS request:

| Request | Meaning | Reply |
|---|---|---|
| `0xF0` | last stored feedback sample; a start clears it | 2 bytes, high byte first; no result byte |
| `0xF1` | stop the board | `0x01` |
| `0xF2`–`0xFF` | start; one operand byte follows | `0x01` |

The `0xF0` sample is an ADC-node count, not a calibrated voltage.  A start
clears the stored sample, so a read after the new start is `0x00 0x00` until
the first conversion completes; a stop leaves the last sample unchanged.

For a start, the requested value is
`(low nibble of the command << 8) | operand`, so the reachable values are
`0x200`–`0xFFF`.  The regulation loop, the range where the firmware cutoff can
act and the electrical caveats are in
[the SMPS binary page](../../capabilities/smps/binary.md); whether the add-on
actually reaches a requested target is unknown.  The unit of the requested
value is not defined by the firmware (`unverified`).

On a build without SMPS support every request is refused with `0x00`, and a
start still consumes its operand byte so the following request stays aligned.
The group is absent from the default image.  Leaving raw wire with `0x00` does
**not** stop the SMPS: stop it with `0xF1` while still in raw wire, or leave
the binary layer with `0x0F`; on v4 the terminal reset stops the loop.
In BBIO1, `0xF0`–`0xFF` are pin state bytes, so a stop byte sent after leaving
raw wire switches the target supply and pull-ups on and presets the **AUX and
CS latches high** (MOSI, CLK and MISO latch low); the pins themselves stay
inputs until a direction byte drives them.

## Per-mode group matrix

`x` = present · `—` = not present.

| Group | SPI | I2C | UART | 1-Wire | raw wire |
|---|---|---|---|---|---|
| `0x0x` actions (exit, identifier, mode actions) | x | x | x | x | x |
| `0x1x` bulk byte transfer | x | x | x | x | x |
| `0x2x` second group | — | — | — | x (speed) | x (clock ticks) |
| `0x3x` bulk bit transfer | — | — | — | — | x |
| `0x4x` peripherals | x | x | x | x | x |
| `0x5x` pull-up feed (v4) | x | x | x | x | x |
| `0x6x` speed | x | x | x | — | x |
| `0x7x` wait for an edge | x | x | x | x | x |
| `0x8x` mode configuration | x | — | x | — | x |
| `0x9x` UART frame (same as `0x8x`) | — | — | x | — | — |
| `0xAx` PIC programming | — | — | — | — | x |
| `0xFx` SMPS (optional build) | — | — | — | — | x |

`0x8x` is per mode; use the mode's page.  The SMPS group is off in the default
image, where it answers `0x00`.  `PIC1`, `OCD1` and `XSV1` are outside this
matrix.

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| SPI / I2C / UART / 1-Wire / raw wire entries | present, unverified | present |
| OpenOCD (`0x06`) and `PIC1` (`0x07`) | present, unverified | present |
| AVRISP-MKII entries (`0x08`, `0x09`) | — (`0x00`, then `BBIO1`) | present |
| XSVF entry (`0x18`) | — (`0x00`, stays in BBIO1) | present |
| pull-up feed (`0x5x`) | — (`0x00`) | present |
| SMPS group (`0xFx`) | — (`0x00`) | routed; `0x00` unless the optional build is enabled |

The v3 column is **not verified** on hardware.

## Related docs

- [commands.md](commands.md) — the BBIO1 core command map.
- [entering-and-framing.md](entering-and-framing.md) — entry, replies and
  framing.
- [examples.md](examples.md) — worked exchanges, including the mode skeleton.
- [the shared `RAW1` page](../wire/binary.md) — raw-wire framing and operands.
- [SPEC.md](../../SPEC.md) — the function and capability index.
