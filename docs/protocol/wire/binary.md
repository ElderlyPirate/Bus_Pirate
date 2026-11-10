# Raw-wire binary (`RAW1`)

Part of the [wire protocol set](README.md).  This page owns the **shared**
binary framing for 2-Wire and 3-Wire: `RAW1` is one sub-protocol that drives
either raw-wire count, and the same command groups serve both.  The per-wire
behaviour is in [2wire/binary.md](2wire/binary.md) and
[3wire/bus-operations.md](3wire/bus-operations.md).  1-Wire has its own
sub-protocol, `1W01` — see [1wire/binary.md](1wire/binary.md).

The general binary layer (entering BBIO, leaving the CLI, the shared actions)
is in [docs/protocol/binary/](../binary/README.md).

## Entry and exit

1. Enter the binary bitbang layer from the terminal by sending 20 × `0x00`; it
   prints `BBIO1`.
2. Send `0x05` (enter raw wire mode).  The mode prints `RAW1` and the raw-wire
   command loop starts.  On entry it is in the 2-wire configuration, open
   drain, 8-bit, MSB-first.
3. Send `0x00` to leave raw wire (back to `BBIO1`); from BBIO, `0x0F` returns
   to the interactive terminal.

`RAW1` is the shared mode for both wire counts.  The `0x8x` configuration byte
selects 2-wire or 3-wire; a host must choose deliberately.

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
| `0x7x` | wait for an edge (shared) | success/failure + level byte |
| `0x8x` | configuration (HiZ, wires, bit order) | `0x01` |
| `0xAx` | PIC programming | per sub-command |
| `0xFx` | SMPS | per sub-command |
| other | unrecognised | `0x00` |

### Generic actions (`0x0x`)

| Request | Action | Reply |
|---|---|---|
| `0x00` | exit raw wire mode | `BBIO1` |
| `0x01` | report the identifier | `RAW1` |
| `0x02` | send an I2C-style start | `0x01` |
| `0x03` | send an I2C-style stop | `0x01` |
| `0x04` | CS low | `0x01` |
| `0x05` | CS high | `0x01` |
| `0x06` | read one byte | one data byte |
| `0x07` | read one bit | one bit value (`0`/`1`) |
| `0x08` | peek the input bit (no clock) | the data-pin level |
| `0x09` | advance one clock tick | `0x01` |
| `0x0A` | CLK low | `0x01` |
| `0x0B` | CLK high | `0x01` |
| `0x0C` | DATA low | `0x01` |
| `0x0D` | DATA high | `0x01` |

These mirror the terminal bitwise controls: start/stop map to `[`/`]`, and the
CS/clock/data commands map to `/`, `\`, `-`, `_`, `^`, `.` and `!`.

### Shared groups (`0x4x`–`0x7x`)

The peripheral (`0x4x`), pull-up-feed (`0x5x`), speed (`0x6x`) and
wait-for-edge (`0x7x`) groups are shared by every nibble-group sub-protocol.
Their bit tables, operands and reply rules are owned once by
[protocol/binary/sub-protocols.md](../binary/sub-protocols.md) §Shared command
groups; this page does not repeat them.

`RAW1` speed (`0x6x`) uses the same four bit-bang profiles as the terminal:
`0` (about 6 kHz) through `3` (maximum).

### Configure (`0x8x`)

The low bits of the configuration byte set the mode flags:

| Bit | Mask | When set | When clear |
|---|---|---|---|
| 3 | `0x08` | high impedance / open drain | normal output |
| 2 | `0x04` | 3 wire | 2 wire |
| 1 | `0x02` | LSB-first | MSB-first |

`0x88` is open drain + 2 wires + MSB-first; `0x8C` is open drain + 3 wires +
MSB-first; `0x84` is normal output + 3 wires + MSB-first.

The PIC (`0xAx`) and SMPS (`0xFx`) groups share this mode with both wire
counts and belong to the PIC and SMPS topics; they are not raw-wire features.

## Worked exchange

A minimal 2-wire byte write and byte read, open drain with a pull-up.  The
device-specific bytes are illustrative.

```
host -> 0x00 x20     enter bitbang
host <- "BBIO1"
host -> 0x05         select raw wire
host <- "RAW1"
host -> 0x88         configure: open drain, 2 wires, MSB-first
host <- 0x01
host -> 0x60         set speed = profile 0
host <- 0x01
host -> 0x51         v4: enable the 3.3V pull-up feed
host <- 0x01
host -> 0x4C         peripherals: supply + pull-ups on, CS low
host <- 0x01
host -> 0x10         bulk-write 1 byte (length selector 0)
host <- 0x01
host -> 0xA5         data byte
host <- 0x01         Bus Pirate result byte
host -> 0x06         read one byte
host <- (one data byte)
host -> 0x00         exit -> BBIO1
host <- "BBIO1"
```

The 3-wire equivalent changes the configuration byte and drives CS explicitly;
see [3wire/bus-operations.md](3wire/bus-operations.md) §Scripting.

## Limits

- The `RAW1` binary path is 2-wire until reconfigured; the same mode also
  serves 3-wire, so a host must set the wire count deliberately.
- The bulk bit-transfer command takes its data level from bit 7 of the command
  byte; the host byte it reads is consumed but not used.
- The `0x5x` pull-up feed group is v4 only; on v3 it returns `0x00`.
- The full PIC and SMPS sub-protocols are not covered here.
- The binary path is code-derived; the terminal path in the topic files is the
  described reference.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| `RAW1` raw-wire mode | present | present |
| generic / bulk / config / speed groups | present | present |
| pull-up feed (`0x5x`) | — (request returns `0x00`) | present |

The v3 column is **not verified** (v3 is build-only here).

## Related docs

- [2-Wire binary specifics](2wire/binary.md)
- [3-Wire scripting](3wire/bus-operations.md)
- [1-Wire `1W01`](1wire/binary.md)
- [docs/protocol/binary/](../binary/README.md) — the BBIO byte layer
