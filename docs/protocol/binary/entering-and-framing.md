# Entering the binary layer and framing

Part of the binary-layer documentation set (`docs/protocol/binary/`).  This
page owns the entry sequence, the reply conventions and the byte framing shared
by every binary mode.  The command bytes themselves are in
[commands.md](commands.md) (BBIO1 core) and [sub-protocols.md](sub-protocols.md)
(the per-protocol entries).

## Scope

- **Covered:** how a host enters and leaves the binary layer, what the shared
  result bytes mean, how request bytes are framed, and the hardware state the
  layer starts in.
- **Not covered:** the per-protocol command sets.  Each mode owns its own
  `binary.md`; the entry bytes and links are in
  [sub-protocols.md](sub-protocols.md).

## Entering from the terminal

The binary layer is entered from the interactive terminal; the terminal
cannot be used once the layer is active.

1. Make sure the Bus Pirate is at the command line.  If it may be sitting in a
   configuration menu, press Enter a few times and send `#` to reset first.
2. Send **20 × `0x00`** (NUL) bytes.  The Bus Pirate replies `BBIO1`.

Notes:

- A NUL byte cannot be typed in a normal terminal; a host program or script
  sends the entry sequence.
- `BBIO1` means **binary bitbang, protocol version 1**.  Every later `0x00`
  sent to the layer re-prints `BBIO1`; it does not reset anything.
- On entry the layer resets the hardware: it switches the on-board pull-up
  feed off, releases the pull-ups, switches the supply off, leaves all five
  lines as high-impedance inputs, and clears the output latches low.

## Wiring

The layer uses the same five header lines as the mode it selects, plus the
ADC probe and pull-up reference, and ground:

| Pin | Binary-layer role |
|---|---|
| AUX | pin I/O; the `0x70` wait-for-edge and the state-byte AUX bit |
| MOSI | data out (and, in raw wire, the bidirectional data line) |
| CLK | clock out |
| MISO | data in |
| CS | chip select |
| ADC | probe input for `0x14` / `0x15`; 0–6.6 V via the on-board divider |
| Vpullup (Vpu) | pull-up reference for the pull-up resistors; on v4 the `0x5x` group selects an on-board +3.3 V or +5.0 V source; on v3 feed it externally (0–5 V) — the self-test uses a `Vpu`→`+5 V` jumper |

The AUX row is the plain `AUX` (`A0`) line; how the terminal layer's
`c` / `C` / `k` / `K` reassignment applies to the binary layer is summarised in
[commands.md](commands.md) §The AUX pin.

Whether a **pull-up or supply is required is inherited from the selected
sub-protocol and its output type**: open-drain output needs the pull-up
resistors enabled with a Vpu voltage (v4 has the resistors and the source on
board; on v3 feed the reference externally, 0–5 V), push-pull output
needs none.  Each mode's `wiring.md` is the authority for its bus.  The link to
the host is the USB serial connection; its settings are in
[generic/README.md](../../generic/README.md).

## Reply conventions

| Reply | Meaning |
|---|---|
| `0x01` | success / acknowledgement |
| `0x00` | failure / refusal |
| one or more data bytes | the value a read command requested |

- A command that is not recognised is answered with a single `0x00` failure
  byte.
- Commands that read a value (a bus byte, a bit, an ADC sample, a frequency
  count) return the value instead of a result byte; the reference for each is
  in [commands.md](commands.md) and the mode pages.
- Some commands acknowledge with a `0x01` before their data payload; the reply
  column of each command table states the exact shape.
- A pin **state** or **direction** byte is answered with the five pin levels as
  a single byte, `0x00`–`0x1F`: the state byte's class bit (bit 7) is not
  echoed.

## Framing

- **BBIO1 command byte.**  Bit 7 selects the class:
  - `1xxxxxxx` (`0x80`–`0xFF`) — a **state byte**: drive the pins and
    peripherals and read the current state back;
  - `010xxxxx` (`0x40`–`0x5F`) — a **direction byte**: set each pin to input
    or output;
  - otherwise — a **command byte** from the core command table.
- **Sub-protocol request byte.**  The **high nibble** selects a command group
  and the **low nibble** is the operand.  Variable-length commands (write
  counts, payloads) follow the request byte.
- **Multi-byte values are big-endian** (most significant byte first) unless a
  mode's page states otherwise: write/read counts, ADC samples, frequency
  counts and the wait timeout.
- **Settling.**  After a pin-state or pin-direction update the firmware waits
  roughly 5 µs before returning the state byte, so a host sees the settled
  level rather than a transient.

## Leaving

- **`0x0F` — return to the terminal through a reset.**  The Bus Pirate replies
  `0x01`, then:
  - **v3:** performs a real hardware reset;
  - **v4:** runs the **terminal reset** — pins become inputs, pull-ups are
    released, supplies switch off, the AUX output source is detached and the
    AUX assignment returns to `A0`, the display format returns to HEX, and
    BASIC is left — then prints the banner and the `HiZ>` prompt.
  Send 20 NUL bytes again to re-enter the binary layer.
- **Garbage before the banner.**  On v3 the hardware reset may emit stray bytes
  as the serial link comes back up; discard bytes until the banner.  On v4 the
  terminal reset prints the banner directly on the same link.
- **Leaving a sub-protocol.**  Send `0x00` to the active sub-protocol; it
  returns to the bitbang layer and prints `BBIO1`.  Then send `0x0F` to leave
  for the terminal.  (The XSVF player is the exception: it ignores `0x00`;
  leave it with `0x0F`, which returns to the bitbang loop without re-printing
  `BBIO1` — see [../jtag/xsvf.md](../jtag/xsvf.md).)  The AVRISP entries
  (`0x08` / `0x09`) have no command loop at all: they hand the link to the
  personality, which ends after about five seconds without host traffic or on
  a reset.

## Availability

| Item | v3 | v4 |
|---|---|---|
| entry `20 × 0x00` → `BBIO1` | present (build-only) | present |
| exit `0x0F` | real hardware reset | terminal reset, back to `HiZ>` with the banner |
| entry NUL counter reset on return | reset by the hardware | reset in firmware |

The v3 column is **not verified** (v3 is build-only here).

## Related docs

- [commands.md](commands.md) — the BBIO1 command byte map.
- [sub-protocols.md](sub-protocols.md) — the sub-protocol entries.
- [edge-cases.md](edge-cases.md) — entry and stuck-state failure modes.
- [generic/commands.md](../../generic/commands.md) §Leaving the CLI — the
  terminal-side view of the entry sequence.
