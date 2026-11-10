# 2WIRE — binary `RAW1` sub-protocol

Part of the 2WIRE documentation set (`docs/2wire/`).  This page documents the
scriptable binary interface that drives the same two-wire primitives without the
terminal.  The general binary layer (entering BBIO, framing, the shared
commands) is documented in `docs/generic/`
([commands.md](../../../generic/commands.md) §Leaving the CLI) and `docs/binary/`.

## Entry and exit

1. Enter the binary bitbang layer from the terminal by sending 20 × `0x00`;
   it prints `BBIO1`.
2. Send `0x05` (enter raw wire mode).  The mode prints `RAW1` and the raw-wire
   command loop starts.
3. Send `0x00` to leave raw wire (back to `BBIO1`); from BBIO, `0x0F` returns to
   the interactive terminal.

`RAW1` is the **shared** raw-wire binary mode: it serves both 2-wire and
3-wire.  It starts in the 2-wire configuration; the `0x8x` configuration byte
(below) is how a host switches it to 3-wire.  For 2WIRE, leave it in 2-wire.

The framing, command groups and generic actions on this page are common to both
wire counts, so **this page owns the shared `RAW1` framing**.  Configuration
bit 2 (below) selects the wire count; the 3-wire behaviour is documented in
the [3-wire set](../3wire/README.md).

## Framing

Each request byte is split into two nibbles: the **high nibble** selects a
command group and the **low nibble** is the operand.  Replies are `0x01` for
success and `0x00` for failure; commands that read a value return the data byte
instead of a result byte.

| Request | Group | Reply |
|---|---|---|
| `0x0x` | generic actions (see below) | per command |
| `0x1x` | bulk byte transfer | `0x01`, then `0x01` per byte |
| `0x2x` | bulk clock ticks | `0x01` |
| `0x3x` | bulk bit transfer | `0x01` twice |
| `0x4x` | configure peripherals (power, pull-ups, AUX, CS) | `0x01` |
| `0x5x` | configure: pull-up feed (**v4 only**) | `0x01` applied / `0x00` not applied |
| `0x6x` | set speed (low 2 bits = profile 0–3) | `0x01` |
| `0x7x` | wait for an edge (shared) | success/flags + level byte |
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

These mirror the terminal's bitwise controls: start/stop map to `[`/`]`, the
CS/clock/data commands map to `/`, `\`, `-`, `_`, `^`, `.` and `!`.

### Peripherals (`0x4x`)

The low nibble of `0x4x` selects what to switch; the reply is `0x01`.

| Bit | Mask | When set | When clear |
|---|---|---|---|
| 3 | `0x08` | target supply on | supply off |
| 2 | `0x04` | pull-up resistors on | pull-ups off |
| 1 | `0x02` | AUX high | AUX low |
| 0 | `0x01` | CS high (released) | CS low (asserted) |

So `0x4C` = bits 3 and 2 = supply on + pull-ups on, bit 0 clear = CS low.

### 2-wire specifics

- **Read byte (`0x06`)** clocks a full value in from SDA (MOSI), MSB-first
  unless LSB-first is configured.
- **Bulk byte (`0x1x`)** — the low nibble is `n`; the host sends `n + 1` data
  bytes.  The exact per-byte sequence in 2-wire is:

  1. host → `0x1n` (the request),
  2. host ← `0x01` (the command is accepted),
  3. for each of the `n + 1` bytes: host → data byte, host ← `0x01`.

  Each byte is clocked out on SDA (MOSI); the trailing `0x01` is the device's
  own result byte, not a slave ACK.  There is no slave-ACK byte in 2-wire
  (compare I2C's binary mode, which returns the slave ACK per byte).
- **Configuration (`0x8x`)** sets the mode flags from the low bits: bit 3 =
  high-impedance/open drain, bit 2 = 3-wire, bit 1 = LSB-first.  On entry the
  handler starts open-drain, 8-bit, MSB-first and in 2-wire.
- **Set speed (`0x6x`)** selects the same four bit-bang profiles as the
  terminal (`0` ≈ 6 kHz … `3` maximum).
- **Pull-up (`0x5x`)** reproduces the v4 `e` choices: `0x51` = 3.3 V, `0x52` =
  5.0 V, any other value disables both.  The reply is `0x01` when the source is
  applied and `0x00` when it is not, i.e. in non-open-drain mode or when VPU
  already has a voltage.

The PIC (`0xAx`) and SMPS (`0xFx`) groups share the mode with 3-wire and belong
to the PIC and SMPS topics; they are not 2-wire protocol features.

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
host -> 0x51         v4: enable the 3.3V pull-up feed (0x00 if already powered)
host <- 0x01
host -> 0x4C         peripherals: supply + pull-ups on (bit 0 clear = CS low)
host <- 0x01
host -> 0x10         bulk-write 1 byte (length selector 0)
host <- 0x01
host -> 0xA5         data byte
host <- 0x01
host -> 0x06         read one byte
host <- (one data byte)
host -> 0x00         exit -> BBIO1
host <- "BBIO1"
```

## Limits

- The `RAW1` binary path is 2-wire until reconfigured; the same mode also serves
  3-wire, so a host must set the wire count deliberately.
- The bulk bit-transfer command takes its data level from bit 7 of the command
  byte; the host byte it reads is consumed but not used.
- The `0x5x` pull-up feed group is v4 only.
- The full PIC and SMPS sub-protocols are not covered here.
- This binary path is **not verified** on hardware; the terminal path in the
  other topic files is the described reference.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| `RAW1` raw-wire mode | present | present |
| generic / bulk / config / speed groups | present | present |
| pull-up feed (`0x5x`) | — | present |

The v3 column is **not verified**.