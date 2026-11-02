# I2C — binary (`I2C1`) sub-protocol

Part of the I2C documentation set (`docs/i2c/`).  This page documents the
binary/host-programming interface to I2C mode.  The general binary layer
(entering BBIO, framing, the shared commands) is documented in `docs/generic/`
(see [commands.md](../generic/commands.md) §Leaving the CLI) and `docs/binary/`
— see [entering-and-framing.md](../binary/entering-and-framing.md),
[commands.md](../binary/commands.md) and
[sub-protocols.md](../binary/sub-protocols.md).

## Entry and exit

1. Enter the binary bitbang layer from the terminal by sending 20 × `0x00`;
   it prints `BBIO1`.
2. Send `0x02` to select the I2C protocol; the mode prints `I2C1` and the I2C
   command loop starts.
3. Send `0x00` to leave I2C (back to `BBIO1`); from BBIO, `0x0F` returns to the
   interactive terminal.

The binary I2C path is always the **software** (bit-banged) driver, regardless
of which driver the interactive mode last used.

## Commands

The high nibble of each request byte is the command group; the low nibble is
the operand.

| Request | Group | Meaning | Reply |
|---|---|---|---|
| `0x00` | action | exit to bitbang | `BBIO1` |
| `0x01` | action | report identifier | `I2C1` |
| `0x02` | action | send start | `0x01` |
| `0x03` | action | send stop | `0x01` |
| `0x04` | action | read one byte | one data byte |
| `0x06` | action | send ACK | `0x01` |
| `0x07` | action | send NACK | `0x01` |
| `0x08` | action | write-then-read (see below) | `0x01`, then the read bytes; `0x00` on failure |
| `0x09` | action | extended AUX (one sub-command byte follows) | `0x01`, then the sub-result |
| `0x0F` | action | enter the sniffer; the next host byte exits | `0x01` |
| `0x1x` | bulk | write `x + 1` bytes (`x` = 0…15) | `0x01`, then one ACK/NACK byte per written byte |
| `0x4x` | configure | peripherals: power, pull-ups, AUX, CS | `0x01` |
| `0x5x` | configure | set the pull-up feed (**v4 only**) | `0x01` / `0x00` |
| `0x6x` | configure | set speed; low 2 bits select software profile 0–3 | `0x01` |
| `0x7x` | wait | wait for an edge on the AUX pin (shared) | shared 2-byte reply |
| other | — | unrecognised | `0x00` |

- `0x01` is success/ack; `0x00` is failure.  For a written byte the returned
  bit is the **slave's** ACK (`0x00` = ACK, `0x01` = NACK).
- The bulk-write group sends exactly `x + 1` bytes: `x = 0` sends one byte,
  `x = 15` sends sixteen.  A fresh `0x1x` command is sent for each group of up
  to sixteen bytes.
- `0x6x` selects the same four software speed profiles as the terminal menu
  (`0` ≈ 6 kHz, `1` ≈ 60 kHz, `2` ≈ 150 kHz, `3` maximum).  An old source
  comment that listed `5/50/100/400 kHz` is stale and does not describe the
  current behaviour.

### Configure peripherals (`0x4x`)

The low four bits set the board state: bit 3 = power, bit 2 = pull-ups,
bit 1 = AUX line, bit 0 = CS line.  For example `0x4C` = power on + pull-ups
on.

The AUX bit and the extended AUX command (`0x09`) drive the AUX pin.  If AUX is
jumpered to a bus pin, driving it will clamp that line; release AUX
(`0x09 0x02`, high-Z) before any bus traffic when it is jumpered to SDA or SCL.
The AUX sub-commands also print the AUX state line into the stream (for
example `AUX INPUT/HI-Z`) in addition to the result byte.

### Write-then-read (`0x08`)

After the command byte:

1. a **big-endian 16-bit** write count,
2. a **big-endian 16-bit** read count,
3. the write payload (the first byte is the device address).

The firmware sends a start, writes the payload, and checks the slave ACK after
each byte.  If the read count is non-zero and more than one byte was written,
it sends a repeated start and the read address (device address with the R/W bit
set).  It then reads the requested bytes, ACKing all but the last (NACK), sends
a stop, replies `0x01`, and finally sends the read bytes to the host.

Bounds: both counts must fit the terminal buffer — **4096 bytes on v4, 2048
on v3** — and the all-zero case is refused.  A request with either count over
the limit is answered `0x00` with **no bytes consumed and no bus traffic**.  A
NACK during the write phase aborts with `0x00`.

## Worked exchange

Host → Bus Pirate and Bus Pirate → host.  `0x51`/`0x4C`/`0x09`/`0x60` are
the bring-up; the frames then write `0xAA 0xBB` at register `0x20` and read
them back.  The addressed device in this example is an EEPROM-style slave at
address `0x50`.

```
host -> 0x00 x20        enter bitbang
host <- "BBIO1"
host -> 0x02            select I2C
host <- "I2C1"
host -> 0x51            v4: enable on-board 3.3V pull-up feed
host <- 0x01            (0x00 = a voltage was already present)
host -> 0x4C            power + pull-ups on
host <- 0x01
host -> 0x09 0x02       extended AUX: set AUX high-Z
host <- 0x01
host -> 0x60            speed = software profile 0

; frame [ 0xA0 0x20 0xAA 0xBB ]
host -> 0x02            start
host <- 0x01
host -> 0x10            bulk-write 1 byte (length selector 0)
host <- 0x01
host -> 0xA0            slave write address
host <- 0x00            slave ACK
host -> 0x10
host <- 0x01
host -> 0x20            register pointer
host <- 0x00
host -> 0x10
host <- 0x01
host -> 0xAA
host <- 0x00
host -> 0x10
host <- 0x01
host -> 0xBB
host <- 0x00
host -> 0x03            stop
host <- 0x01

; frame [ 0xA1 r r ]
host -> 0x02            start
host <- 0x01
host -> 0x10            bulk-write 1 byte
host <- 0x01
host -> 0xA1            slave read address
host <- 0x00            slave ACK
host -> 0x04            read one byte
host <- 0xAA
host -> 0x06            ACK (another byte follows)
host <- 0x01
host -> 0x04            read one byte
host <- 0xBB
host -> 0x07            NACK (last byte)
host <- 0x01
host -> 0x03            stop
host <- 0x01
```

## v3 / v4

| | v3 | v4 |
|---|---|---|
| action / bulk / speed / `0x08` groups | present | present |
| set the pull-up feed (`0x5x`) | — (answered `0x00`) | present |
| terminal buffer bound for `0x08` | 2048 bytes | 4096 bytes |

The v3 column is **not verified**.