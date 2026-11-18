# Raw-wire binary (`RAW1`)

Part of the [wire protocol set](README.md).  This page owns the **shared**
binary framing for 2-Wire and 3-Wire: `RAW1` is one sub-protocol that drives
either raw-wire count, and the same command groups serve both.  The per-wire
behaviour is in [2wire/binary.md](2wire/binary.md) and
[3wire/binary.md](3wire/binary.md).  1-Wire has its own sub-protocol, `1W01` —
see [1wire/binary.md](1wire/binary.md).

The general binary layer (entering BBIO, leaving the CLI, the shared actions)
is in [docs/protocol/binary/](../binary/README.md).  The shared `0x4x`–`0x7x`
groups' cross-mode meaning is owned by
[sub-protocols.md](../binary/sub-protocols.md); this page gives the `RAW1`
operands and the per-wire replies.  This path is not verified on hardware.

## Entry and exit

1. Enter the binary bitbang layer from the terminal by sending 20 × `0x00`; it
   prints `BBIO1`.
2. Send `0x05` (enter raw wire mode).  The mode prints `RAW1` and the raw-wire
   command loop starts.
3. Send `0x00` to leave raw wire (back to `BBIO1`); from BBIO, `0x0F` returns
   to the interactive terminal.

Entry always sets:

| Setting | Value |
|---|---|
| wire count | 2 wires |
| output type | open drain (high impedance high) |
| width | 8 bits; there is no width command |
| bit order | MSB-first |
| speed setting | profile 1 (the maximum profile is applied on entry) |

The entry defaults are applied even if the terminal was left in `3WIRE`: a host
that wants 3 wires must send a configuration byte.  The applied timing profile
on entry is the maximum rate, while the recorded speed value is 1; send a
`0x6x` command before relying on a particular rate.

## Framing

Each request byte is split into two nibbles: the **high nibble** selects a
command group and the **low nibble** is the operand.  Replies are `0x01` for
success and `0x00` for failure; commands that read a value return the data byte
instead of a result byte.

| Request | Group | Reply |
|---|---|---|
| `0x0x` | generic actions (see below) | per command |
| `0x1x` | bulk byte transfer | `0x01`, then a per-byte reply (wire-specific) |
| `0x2x` | bulk clock ticks | `0x01` |
| `0x3x` | bulk bit transfer | `0x01` twice |
| `0x4x` | configure peripherals (power, pull-ups, AUX, CS) | `0x01` |
| `0x5x` | configure: pull-up feed (v4 only) | `0x01` applied / `0x00` not applied |
| `0x6x` | set speed (low 2 bits = profile 0–3) | `0x01` |
| `0x7x` | wait for an edge on AUX (`0x70` only) | result byte + level byte |
| `0x8x` | configuration (HiZ, wires, bit order) | `0x01` |
| `0xAx` | PIC programming | per sub-command |
| `0xFx` | SMPS | per sub-command |
| other | unrecognised | `0x00` |

### Generic actions (`0x0x`)

| Request | Action | Reply |
|---|---|---|
| `0x00` | exit raw wire mode | `BBIO1` |
| `0x01` | report the identifier | `RAW1` |
| `0x02` | send an I2C-style start condition | `0x01` |
| `0x03` | send an I2C-style stop condition | `0x01` |
| `0x04` | CS low | `0x01` |
| `0x05` | CS high | `0x01` |
| `0x06` | read one byte | one data byte |
| `0x07` | read one bit (with a clock) | one bit value (`0`/`1`) |
| `0x08` | peek the input bit (no clock) | the data-pin level |
| `0x09` | advance one clock tick | `0x01` |
| `0x0A` | CLK low | `0x01` |
| `0x0B` | CLK high | `0x01` |
| `0x0C` | DATA low | `0x01` |
| `0x0D` | DATA high | `0x01` |

`0x02`/`0x03` are **I2C-style start/stop conditions** on the clock/data lines,
not chip-select actions.  In 2-Wire they correspond to the terminal `[` and
`]`.  In 3-Wire the terminal `[`/`]` are CS actions, and the binary equivalents
are `0x04` (CS low) and `0x05` (CS high); `0x02`/`0x03` still generate the
I2C-style waveform and are not the 3-Wire wrappers.

## Peripherals (`0x4x`)

The low four bits set the board state; the reply is `0x01`.

| Bit | Mask | When set | When clear |
|---|---|---|---|
| 3 | `0x08` | target supply (VREG) on | supply off |
| 2 | `0x04` | pull-up resistors on | pull-ups off |
| 1 | `0x02` | AUX high (AUX is forced to an output) | AUX low |
| 0 | `0x01` | CS high (released in open drain) | CS low (asserted) |

The pull-up bit switches the resistors on; it does not choose their voltage
source.  Select the source with `0x5x` first, exactly as `e2`/`e3` precedes `P`
in the terminal.

`0x4D` = supply on, pull-ups on, CS high.  `0x4C` = supply on, pull-ups on,
CS low.  `0x40` = supply off, pull-ups off, CS low.

## Pull-up feed (`0x5x`, v4 only)

The low nibble selects the on-board source that feeds VPU:

| Operand | Source |
|---|---|
| `0x51` | on-board **+3.3 V** |
| `0x52` | on-board **+5.0 V** |
| any other | both off |

The reply is `0x01` when the request was applied and `0x00` when it was refused
(the pins are not open-drain HiZ, or a voltage is already present on VPU).  On
v3 the group is not built, so the request is answered with `0x00`.

## Speed (`0x6x`)

The low two bits select one of the four bit-bang profiles used by 2-Wire and
3-Wire:

| Operand | Profile | Menu label |
|---|---|---|
| `0x60` | 0 | ~6 kHz |
| `0x61` | 1 | ~60 kHz |
| `0x62` | 2 | ~150 kHz |
| `0x63` | 3 | Maximum speed |

The reply is `0x01`, and the command also drives CS high.  The labels are
nominal; see the [2-Wire clock table](2wire/entry-and-setup.md#clock-speeds)
for the phase delays behind them.

## Configuration (`0x8x`)

The low bits set the mode flags:

| Bit | Mask | When set | When clear |
|---|---|---|---|
| 3 | `0x08` | high impedance / open drain | normal output |
| 2 | `0x04` | 3 wires | 2 wires |
| 1 | `0x02` | LSB-first | MSB-first |

Examples:

| Byte | Meaning |
|---|---|
| `0x88` | open drain + 2 wires + MSB-first |
| `0x8C` | open drain + 3 wires + MSB-first |
| `0x84` | normal output + 3 wires + MSB-first |
| `0x80` | normal output + 2 wires + MSB-first |

The reply is `0x01`, and the command also drives CS high.  A 3-Wire host must
re-assert CS with `0x04` after configuring.

## Bulk byte transfer (`0x1x`)

The low nibble is `n`; the host sends `n + 1` data bytes (`n = 0` sends one,
`n = 15` sends sixteen).  The sequence starts with one `0x01`; the per-byte
reply then depends on the wire count.

| Wire count | Per-byte reply |
|---|---|
| 2-Wire | `0x01` — the Bus Pirate's own result byte, not a slave ACK |
| 3-Wire | one sampled MISO byte, full duplex |

2-Wire per byte:

```
host -> 0x1n      request
host <- 0x01      accepted
host -> data      one byte, n + 1 times
host <- 0x01      after each byte
```

3-Wire per byte:

```
host -> 0x1n      request
host <- 0x01      accepted
host -> data      one byte, n + 1 times
host <- sample    the MISO byte read during that write
```

The mode consumes exactly `n + 1` bytes as data.  A command byte sent early is
written to the bus as data.

## Bulk clock ticks (`0x2x`)

The low nibble is `n`; the command advances `n + 1` clock ticks and replies
`0x01`.  One tick is one full clock cycle.

## Bulk bit transfer (`0x3x`)

The low nibble is `n`; the command writes `n + 1` bits and replies `0x01`
before and after.  The firmware takes the **data level for every bit from bit
7 of the command byte**, but in this group bit 7 is always 0 (`0x30`–`0x3F`),
so the observable result is `n + 1` **low** bits.  The host must still send
one byte; it is consumed and its bits are not used.  `0x37` writes eight low
bits; `0x30` writes one low bit.

## Wait for an edge (`0x70` only)

Only sub-command 0 is defined.  `0x70` takes the shared payload (edge selector
and a 16-bit big-endian timeout in milliseconds) and replies with a result byte
plus the AUX level.  Any other low nibble is answered `0x00` without consuming
the payload.  The payload, selector values and reply meaning are owned by
[docs/protocol/binary/commands.md](../binary/commands.md) §Wait for an edge.

## Terminal wrappers vs binary commands

| Terminal | Binary | Notes |
|---|---|---|
| `W` / `w` | `0x4x` bit 3 | supply on / off |
| `P` / `p` | `0x4x` bit 2 | pull-ups on / off; `0x5x` selects the VPU source |
| `e2` / `e3` / `e1` | `0x5x` (`0x51` / `0x52` / other) | v4 only |
| speed menu | `0x6x` (0–3) | low two bits |
| output type | `0x8x` bit 3 | 1 = open drain |
| wire count | `0x8x` bit 2 | 1 = 3 wires |
| `l` / `L` | `0x8x` bit 1 | 1 = LSB-first |
| `[` / `{` (2WIRE) | `0x02` | I2C-style start |
| `]` / `}` (2WIRE) | `0x03` | I2C-style stop |
| `[` / `]` (3WIRE) | `0x04` / `0x05` | CS low / high |
| a value | `0x1n` | n + 1 bytes; see the per-wire reply |
| `r` | `0x06` | one byte |
| `^:N` | `0x2n` | n + 1 ticks |
| `/` / `\` | `0x0A` / `0x0B` | clock level, no pulse |
| `-` / `_` | `0x0C` / `0x0D` | data level |
| `.` | `0x08` | peek the input bit, no clock |
| `!` | `0x07` | one bit with a clock |
| `;N` | — | the binary path is fixed at 8 bits |
| `(n)` macros | — | no mode macros; the 1-Wire `1W01` path has `0x08`/`0x09` searches |

The binary layer has **no settings read-back**: a host must track its own
configuration.  There is also no binary command for the 2-Wire ATR macros or
the terminal `v`/`i` tables.

## Worked exchange

A minimal 2-Wire byte write and byte read framed by a start and a stop, open
drain with a pull-up.  The device-specific bytes are illustrative.

```
host -> 0x00 x20     enter bitbang
host <- "BBIO1"
host -> 0x05         select raw wire
host <- "RAW1"
host -> 0x88         configure: open drain, 2 wires, MSB-first
host <- 0x01
host -> 0x60         set speed = profile 0
host <- 0x01
host -> 0x51         v4: VPU = on-board 3.3 V
host <- 0x01
host -> 0x4D         peripherals: supply + pull-ups on, CS high
host <- 0x01
host -> 0x02         I2C-style start
host <- 0x01
host -> 0x10         bulk-write 1 byte
host <- 0x01         accepted
host -> 0xA5         data byte
host <- 0x01         Bus Pirate result byte
host -> 0x06         read one byte
host <- (one data byte)
host -> 0x03         I2C-style stop
host <- 0x01
host -> 0x00         exit -> BBIO1
host <- "BBIO1"
```

START and STOP are optional for raw clocking; add `0x02` and `0x03` when the
target expects a framed transaction.

The 3-Wire equivalent changes the configuration byte, drives CS explicitly and
reads a sampled byte in place of each write ack; a worked exchange is in
[3wire/binary.md](3wire/binary.md#worked-exchange).

## Edge cases and limits

- **Entry applies the maximum profile but records profile 1.**  A later `0x8x`
  re-applies the recorded profile (1 until a `0x6x` changes it); send `0x6x`
  to choose a profile explicitly.
- **`0x8x` and `0x6x` drive CS high.**  A 3-Wire host must re-assert CS after
  either command.
- **The 3-Wire bulk reply is not an ack.**  It is the MISO byte sampled during
  the write; a 2-Wire host's per-byte `0x01` pattern does not apply.
- **`0x02`/`0x03` are not CS actions in 3-Wire.**  Use `0x04`/`0x05`.
- **`0x3x` writes only low bits and ignores the host byte.**  Bit 7 of a
  `0x3x` command is always 0, so the group cannot produce a high bit.
- **`0x1n` consumes exactly `n + 1` bytes.**  A premature command byte becomes
  bus data.
- **The `0x5x` group is v4 only.**  On v3 the request returns `0x00`; the
  `0x4x` pull-up bit still switches the resistors, but there is no on-board
  source to select.
- **`0x71`–`0x7F` are refused without reading a payload.**
- **Unknown groups and unknown actions return `0x00`.**
- **`0x00` exits to `BBIO1`, not to the terminal.**  Send `0x0F` from BBIO to
  return to the prompt.
- **The width is fixed at 8 bits** and there is no width command.  A wider
  value's low byte is used.
- The PIC (`0xAx`) and SMPS (`0xFx`) groups belong to those topics.  SMPS is
  an optional build and is off in the default image, where it answers `0x00`.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| `RAW1` raw-wire mode | present | present |
| generic / bulk / config / speed groups | present | present |
| pull-up feed (`0x5x`) | — (request returns `0x00`) | present |
| PIC group (`0xAx`) | present | present |
| SMPS group (`0xFx`) | routed; answers `0x00` (off) | routed; answers `0x00` (off in the default image) |

## Related docs

- [2-Wire binary specifics](2wire/binary.md)
- [3-Wire binary specifics](3wire/binary.md)
- [1-Wire `1W01`](1wire/binary.md)
- [docs/protocol/binary/](../binary/README.md) — the BBIO byte layer
