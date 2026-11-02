# SPI — binary (`SPI1`) sub-protocol

Part of the SPI documentation set (`docs/spi/`). This page documents the
binary/host-programming interface to SPI mode. The general binary layer
(entering BBIO, framing, the shared commands) is documented in `docs/generic/`
(see [commands.md](../generic/commands.md) §Leaving the CLI) and `docs/binary/`
— see [entering-and-framing.md](../binary/entering-and-framing.md),
[commands.md](../binary/commands.md) and
[sub-protocols.md](../binary/sub-protocols.md).

## Entry and exit

1. Enter the binary bitbang layer from the terminal by sending 20 × `0x00`; it
   prints `BBIO1`.
2. Send `0x01` to select the SPI protocol; the mode prints `SPI1` and the SPI
   command loop starts.
3. Send `0x00` to leave SPI (back to `BBIO1`); from BBIO, `0x0F` returns to the
   interactive terminal.

The binary SPI path starts in its own defaults, independent of the last
interactive session: speed index `1` (125 kHz — the raw speed-table index, not
the interactive menu option `1`, which is 30 kHz), clock idle low, clock edge
active→idle, sample in the middle, open-drain output.

## Commands

The high nibble of each request byte is the command group; the low nibble is
the operand.

| Request | Group | Meaning | Reply |
|---|---|---|---|
| `0x00` | action | exit to bitbang | `BBIO1` |
| `0x01` | action | report identifier | `SPI1` |
| `0x02` | action | CS low | `0x01` |
| `0x03` | action | CS high | `0x01` |
| `0x04` | action | write-then-read, CS asserted for the transfer | `0x01`, then the read bytes |
| `0x05` | action | write-then-read, CS left untouched | `0x01`, then the read bytes |
| `0x06` | action | extended AVR command (see below) | `0x01`, then sub-reply |
| `0x0D` | action | sniffer, all traffic | `0x01`, then sniff stream |
| `0x0E` | action | sniffer, only while CS is low | `0x01`, then sniff stream |
| `0x1x` | bulk | write `x + 1` bytes (`x` = 0…15), return the bus byte for each | `0x01`, then one bus byte per written byte |
| `0x4x` | configure | peripherals: power, pull-ups, AUX, CS | `0x01` |
| `0x5x` | configure | set the pull-up feed (**v4 only**) | `0x01` / `0x00` |
| `0x6x` | configure | set speed; the low nibble indexes the speed table | `0x01` |
| `0x7x` | wait | wait for an edge on the AUX pin (shared) | shared 2-byte reply |
| `0x8x` | configure | set SPI clock/output options (see below) | `0x01` |
| other | — | unrecognised | `0x00` |

`0x01` is success/ack; `0x00` is failure. The CS commands (`0x02`/`0x03`) drive
the CS pin directly, outside any transaction.

### Write-then-read (`0x04`, `0x05`)

After the command byte:

1. a **big-endian 16-bit** write count,
2. a **big-endian 16-bit** read count,
3. the write payload (the bytes to clock out on MOSI).

The firmware writes the payload, then clocks in the requested number of bytes
and returns them to the host. `0x04` asserts CS low for the whole transfer and
releases it afterwards; `0x05` does not touch CS. A write count and read count
of **both zero** is answered with success (a client uses this during
initialisation). The counts are 16-bit; this path does not check them against
the terminal buffer.

### Bulk transfer (`0x1x`)

`0x1x` writes `x + 1` bytes from the host to the bus and returns the bus byte
read for each one. `x = 0` writes one byte; `x = 15` writes sixteen. A fresh
`0x1x` command is sent for each group of up to sixteen bytes. This group does
not manage CS.

### Configure peripherals (`0x4x`)

The low four bits set the board state: bit 3 = power, bit 2 = pull-ups,
bit 1 = AUX line, bit 0 = CS line. For example `0x4C` = power on + pull-ups on.

### Pull-up feed (`0x5x`, v4 only)

Sets the on-board source feeding the pull-up voltage (VPU):

| Request | Source |
|---|---|
| `0x51` | on-board **+3.3 V** |
| `0x52` | on-board **+5.0 V** |
| other | both off |

The reply byte is `0x01` when the request was applied and `0x00` when it was
refused (the pins are not open-drain HiZ, or a voltage is already present on
VPU). On a v3 build this group answers `0x00`.

### Set speed (`0x6x`)

The low nibble indexes the speed table directly:

| Nibble | Rate | Nibble | Rate |
|---:|---|---:|---|
| 0 | 30 kHz | 6 | 4 MHz |
| 1 | 125 kHz | 7 | 8 MHz |
| 2 | 250 kHz | 8 | 50 kHz |
| 3 | 1 MHz | 9 | 1.3 MHz |
| 4 | 2 MHz | 10 | 3.2 MHz |
| 5 | 2.6 MHz | 11 | 5.3 MHz |

An index of 12 or above is out of range and is answered `0x00`. Unlike the
interactive menu (whose option numbers happen to match this order for the first
eight entries only), the binary nibble is the raw table index.

### Configure SPI (`0x8x`)

The low bits set the clock and output options:

| Bit | Setting |
|---|---|
| 0 | sample at **end** (clear = middle) |
| 1 | clock edge **active→idle** (clear = idle→active) |
| 2 | clock idle **high** (clear = idle low) |
| 3 | **normal** output when set; clear = open-drain (Hi-Z) |

### Extended AVR command (`0x06`)

After `0x06`, one sub-command byte follows:

| Sub-command | Action | Reply |
|---|---|---|
| `0x00` | no operation | `0x01` |
| `0x01` | report protocol version | `0x01`, then `0x00 0x01` |
| `0x02` | bulk read | `0x01`, then the data bytes; `0x00` if the bounds fail |
| other | unrecognised | `0x00` |

The bulk-read sub-command reads a big-endian 32-bit address and a big-endian
32-bit length, then reads the flash a word at a time: it clocks the AVR
fetch-low-byte command (`0x20`) followed by the 16-bit address and captures a
byte, then the fetch-high-byte command (`0x28`) followed by the address and
captures the next byte. Address, length and their sum must all be within
16 bits, or the request is refused. This command exists because the build
enables the extended AVR option; without it the request falls through to the
unrecognised-command reply.

## Sniffer

`0x0D` (all traffic) and `0x0E` (CS low) start the same sniffer as macros
`(2)` / `(1)`, but in a binary byte framing. It replies `0x01` and then
streams:

| Element | Bytes |
|---|---|
| CS asserted | `[` |
| CS released | `]` |
| one clocked byte pair | `\` (escape), then the MOSI byte, then the MISO byte |

Each clocked byte pair is escaped by a leading `\` byte, followed by the MOSI
byte and then the MISO byte; CS transitions are sent as bare `[` / `]` bytes.
The interactive form writes the same data as text instead: `0xNN(0xNN)` — see
[sniffer.md](sniffer.md).

Format illustration of the transaction used in [sniffer.md](sniffer.md)
(`[0x30(0x00)0xff(0x12)0xff(0x50)][0x40(0x00)]`), as the binary stream would
look:

```
host <- '['
host <- '\' 0x30 0x00     MOSI 0x30, MISO 0x00
host <- '\' 0xff 0x12
host <- '\' 0xff 0x50
host <- ']'
host <- '['
host <- '\' 0x40 0x00
host <- ']'
```

The sniffer exits when the host sends any byte; that stopping byte is **not**
consumed, so it is treated as the next command. On overflow the sniffer stops
and the mode LED turns off. See [sniffer.md](sniffer.md).

## Worked exchange

Host → Bus Pirate and Bus Pirate → host. This frame selects SPI, configures
normal output and a 250 kHz clock, then writes one byte and reads one byte.
The `0x04` command manages CS itself — it asserts CS low for the transfer
and releases it afterwards — so no separate CS command is sent. The bytes
shown for the read are example values.

```
host -> 0x00 x20        enter bitbang
host <- "BBIO1"
host -> 0x01            select SPI
host <- "SPI1"
host -> 0x88            configure: normal output, idle low, idle->active
host <- 0x01
host -> 0x62            set speed index 2 (250 kHz)
host <- 0x01
host -> 0x04            write-then-read (asserts CS for the transfer)
host -> 0x00 0x01       write count = 1 (big-endian)
host -> 0x00 0x01       read count = 1
host -> 0x9F            write payload
host <- 0x01            success
host <- 0xEF            read byte
host -> 0x00            exit to bitbang
host <- "BBIO1"
```

## v3 / v4

| | v3 | v4 |
|---|---|---|
| action / bulk / speed / `0x04` / `0x05` / `0x8x` groups | present | present |
| pull-up feed (`0x5x`) | replies `0x00` | present |
| extended AVR command (`0x06`) | present | present |
| sniffer (`0x0D` / `0x0E`) | present (polled CS) | present (hardware-latched CS) |

The v3 column is **not verified**.