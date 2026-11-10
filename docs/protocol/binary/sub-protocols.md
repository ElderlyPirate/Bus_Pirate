# Binary sub-protocols and shared groups

Part of the binary-layer documentation set (`docs/protocol/binary/`).  This
page lists the sub-protocol **entry bytes** and identifiers, describes the
**command groups the sub-protocol modes share**, and links each protocol to the
page that owns its command set.  Entry, replies and framing are in
[entering-and-framing.md](entering-and-framing.md); the BBIO1 core map is in
[commands.md](commands.md).

## Entry bytes

Send the entry byte to the bitbang layer (after `BBIO1`).  The mode prints its
identifier and starts its own command loop; `0x00` returns to `BBIO1`.

| Entry byte | Sub-protocol | Identifier | Command set owned by |
|---|---|---|---|
| `0x01` | SPI | `SPI1` | [SPI binary command set](../spi/binary.md) |
| `0x02` | I2C | `I2C1` | [I2C binary command set](../i2c/binary.md) |
| `0x03` | UART | `ART1` | [UART binary command set](../uart/binary.md) |
| `0x04` | 1-Wire | `1W01` | [1-Wire binary command set](../wire/1wire/binary.md) |
| `0x05` | raw wire (2-Wire and 3-Wire) | `RAW1` | [shared `RAW1` framing](../wire/binary.md) |
| `0x06` | OpenOCD JTAG | `OCD1` | [OpenOCD adapter](../jtag/openocd.md) |
| `0x07` | PIC | `PIC1` | [PIC binary command set](../../programmers/pic/binary.md) |
| `0x08` | AVRISP-MKII USB personality (v4 only) | none | [AVRISP-MKII protocol](../../programmers/avrisp/protocol.md) |
| `0x09` | AVRISP-MKII serial STK500v2 route (v4 only) | none | [AVRISP-MKII serial route](../../programmers/avrisp/serial-route.md) |
| `0x18` | XSVF player (v4 only) | `XSV1` | [XSVF player](../jtag/xsvf.md) |

Notes:

- The AVRISP entries re-enumerate or re-purpose the link rather than printing
  an identifier, and they have no command loop: they end on their own after
  about **five seconds** without host traffic, or on a reset.  On a v3 build
  both bytes are answered with `0x00` and the layer then re-announces itself
  with `BBIO1`.
- `RAW1` is the **shared** raw wire mode.  It serves 2-Wire and 3-Wire; the
  entry sequence, shared framing and worked exchange are owned by
  [the shared `RAW1` page](../wire/binary.md).  The per-wire behaviour is in
  [2-Wire binary](../wire/2wire/binary.md) and
  [3-Wire bus operations](../wire/3wire/bus-operations.md).
- OpenOCD (`0x06`) and XSVF (`0x18`) have no terminal mode — these bytes are
  the only way to reach the JTAG hardware.
- SMPS has no entry byte of its own; it is a command group inside `RAW1` — see
  [the SMPS binary page](../../capabilities/smps/binary.md).

## Shared command groups

Inside a sub-protocol the request byte's **high nibble** is the command group
and the **low nibble** is the operand.  Several groups mean the same thing
across modes.

These shared groups are owned by this page: it defines the cross-mode group
semantics, and a mode's `binary.md` may only add per-mode operands or reply
detail — it must not change what the group means.  `RAW1` entry, framing and
the worked exchange are owned by
[the shared `RAW1` page](../wire/binary.md).

**These groups exist only inside a sub-protocol.**  In BBIO1 the whole
`0x40`–`0x5F` range is the pin **direction** byte, so `0x4x` and `0x5x` are
not command groups there; sending one changes pin directions and returns the
pin levels.  See [commands.md](commands.md) §Same byte, different layer or
mode.

### `0x4x` — configure peripherals

Present in SPI, I2C, UART, 1-Wire and `RAW1`.  The low four bits set the board
state; the reply is `0x01`.

| Bit | Mask | When set | When clear |
|---|---|---|---|
| 3 | `0x08` | target supply (VREG) on | supply off |
| 2 | `0x04` | pull-up resistors on | pull-ups off |
| 1 | `0x02` | AUX high | AUX low |
| 0 | `0x01` | CS high (released) | CS low (asserted) |

For example `0x4C` = bits 3 and 2 set = supply on + pull-ups on, CS low.

This group is the sub-protocol counterpart of the BBIO1 **state byte**; note
the different bit positions (in the state byte, power is bit 6 and pull-up is
bit 5).  The AUX bit here also **forces `A0` to an output**, unlike the
state-byte AUX bit, which only sets the level and leaves the direction alone.

### `0x5x` — set the pull-up feed (v4 only)

Present in SPI, I2C, UART, 1-Wire and `RAW1`.  This group is a **configure
action**, not a read: the low nibble selects the on-board source that feeds the
pull-up voltage (Vpu) and switches it on or off.

| Operand | Source |
|---|---|
| `0x51` | on-board **+3.3 V** |
| `0x52` | on-board **+5.0 V** |
| any other | both off |

The reply is always a single byte: `0x01` when the request was applied and
`0x00` when it was refused.  A request is refused when the pins are not
open-drain HiZ, or when a voltage is already present on Vpu.  A refusal caused
by an external Vpu voltage has already switched **both on-board sources off**
(the voltage check runs after they are disabled); the open-drain refusal
returns before that.  The group never returns a state byte.  On a v3 build the
group is not present and is answered `0x00`.

### `0x6x` — set speed

Present in SPI, I2C, UART and `RAW1` (and as the `0x2x` group in 1-Wire).  The
low bits select a speed profile.  The meaning is **per mode**:

| Mode | Operand | Selects |
|---|---|---|
| SPI | low nibble | raw SPI speed-table index (30 kHz … 8 MHz) |
| I2C | low 2 bits | software (bit-banged) profile 0–3 |
| UART | low nibble | preset 0–9 (300 bps … 31250 MIDI) |
| 1-Wire | low bit (`0x2x`) | 0 standard, 1 overdrive |
| `RAW1` | low 2 bits | bit-bang profile 0–3 |

The reply is `0x01` (out-of-range indexes in some modes are answered `0x00`).
Exact tables: each mode's `binary.md`.

### `0x7x` — wait for an edge

Present in every **nibble-group** mode (BBIO1, SPI, I2C, UART, 1-Wire and
`RAW1`).  The dedicated `PIC1`, `OCD1` and `XSV1` loops do not implement it.
Only sub-command `0` is defined; any other low nibble is answered with failure.

Payload and reply are the same as the `0x70` command in
[commands.md](commands.md) §Wait for an edge: an edge selector, a 16-bit
big-endian timeout in milliseconds, then a result byte and the AUX pin level.
The selector and timeout are always consumed before the selector is validated,
and the wait does not change the selected pin's direction — configure it as an
input first.

## Per-mode group matrix

`x` = present · `—` = not present.

| Group | SPI | I2C | UART | 1-Wire | RAW1 |
|---|---|---|---|---|---|
| `0x0x` actions (exit / identifier / mode actions) | x | x | x | x | x |
| `0x1x` bulk write / transfer | x | x | x | x | x |
| `0x2x` second group | — | — | — | x (speed) | x (clock ticks) |
| `0x3x` bulk bit transfer | — | — | — | — | x |
| `0x4x` peripherals | x | x | x | x | x |
| `0x5x` pull-up feed (v4) | x | x | x | x | x |
| `0x6x` speed | x | x | x | — | x |
| `0x7x` wait-for-edge | x | x | x | x | x |
| `0x8x` mode configuration | x (SPI) | — | x (frame) | — | x (HiZ / wires / bit order) |
| `0x9x` frame configuration | — | — | x | — | — |
| `0xAx` PIC programming | — | — | — | — | x |
| `0xFx` SMPS | — | — | — | — | x (optional build; off in the default image) |

The `0x0x` group holds the common actions (exit, report identifier) plus each
mode's own read/write/start/stop actions — see each mode's page.

`0x8x` is **per mode**: bit 3 is the output type, but its sense differs — for
example `0x88` is normal output in SPI and open drain in `RAW1`.  Use the
mode's page.

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| SPI / I2C / UART / 1-Wire / RAW1 entries | present (build-only) | present |
| AVRISP entries (`0x08`, `0x09`) | — (answered `0x00`, then `BBIO1`) | present |
| XSVF entry (`0x18`) | — (answered `0x00`) | present |
| pull-up feed (`0x5x`) | — (answered `0x00` in the sub-protocol modes) | present |

The v3 column is **not verified** (v3 is build-only here).

## Related docs

- [commands.md](commands.md) — the BBIO1 core command map.
- [entering-and-framing.md](entering-and-framing.md) — entry, replies,
  framing.
- [the shared `RAW1` page](../wire/binary.md) — the `RAW1` entry, framing and
  worked exchange.
- [SPEC.md](../../SPEC.md) — the function and capability index.
