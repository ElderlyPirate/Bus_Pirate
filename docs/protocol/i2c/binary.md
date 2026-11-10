# I2C — binary (`I2C1`) sub-protocol

Part of the I2C documentation set (`docs/protocol/i2c/`).  This page owns the
binary/host-programming interface to I2C mode.  The general binary layer
(entering BBIO, framing, shared commands) is in
[docs/generic/](../../generic/commands.md) §Leaving the CLI and
[docs/protocol/binary/](../binary/README.md) — see
[entering-and-framing.md](../binary/entering-and-framing.md),
[commands.md](../binary/commands.md) and
[sub-protocols.md](../binary/sub-protocols.md).

## Entry and exit

1. Enter the binary bitbang layer from the terminal by sending 20 × `0x00`;
   it prints `BBIO1`.
2. Send `0x02` to select I2C; the mode prints `I2C1` and the I2C command loop
   starts.
3. Send `0x00` to leave I2C (back to `BBIO1`); from BBIO, `0x0F` returns to
   the interactive terminal.

The binary I2C path is always the **software** (bit-banged) driver, regardless
of which driver the interactive mode last used.

## Commands

The high nibble of each request byte is the command group; the low nibble is
the operand.

| Request | Group | Meaning | Reply |
|---|---|---|---|
| `0x00` | action | exit to bitbang | `BBIO1` |
| `0x01` | action | report identifier | `I2C1` |
| `0x02` | action | send start | `0x01`; `0x00` on a bus fault |
| `0x03` | action | send stop | `0x01`; `0x00` on a bus fault |
| `0x04` | action | read one byte | one data byte (`0xFF` on a bus fault) |
| `0x06` | action | send ACK | `0x01`; `0x00` on a bus fault |
| `0x07` | action | send NACK | `0x01`; `0x00` on a bus fault |
| `0x08` | action | write-then-read (see below) | `0x01`, then the read bytes; `0x00` on failure |
| `0x09` | action | extended AUX (one sub-command byte follows) | `0x01`, then the sub-result |
| `0x0A` | action | last bus status | `0x01` no fault; `0x00` timeout/fault |
| `0x0F` | action | enter the sniffer; the next host byte exits | `0x01` after the exit byte |
| `0x1x` | bulk | write `x + 1` bytes (`x` = 0…15) | `0x01`, then one ACK/NACK per byte |
| `0x4x` | configure | peripherals: power, pull-ups, AUX, CS | `0x01` |
| `0x5x` | configure | set the pull-up feed (**v4 only**) | `0x01` applied / `0x00` refused |
| `0x6x` | configure | set speed; low 2 bits select profile 0–3 | `0x01` |
| `0x7x` | wait | wait for an edge on the AUX pin (shared) | shared 2-byte reply |
| other | — | unrecognised | `0x00` |

- `0x01` is success/ack; `0x00` is failure.  For a written byte the returned
  bit is the **slave's** ACK (`0x00` = ACK, `0x01` = NACK).
- The bulk-write group sends exactly `x + 1` bytes: `x = 0` sends one byte,
  `x = 15` sixteen.  A fresh `0x1x` is sent for each group of up to sixteen.
- `0x6x` selects the four software profiles from the terminal menu
  (`0` ≈ 6 kHz, `1` ≈ 60 kHz, `2` ≈ 150 kHz, `3` maximum).

### Errors and `0x0A`

`0x02`, `0x03`, `0x06` and `0x07` reply `0x00` when the operation could not
complete (a clock-stretch timeout or bus fault).  `0x04` keeps its
single-byte reply; on a fault that byte is `0xFF`, so by itself it does not
distinguish a fault from a legitimate `0xFF` read.  Use `0x0A` for that:

- `0x01` — the last bus operation had no fault.
- `0x00` — the last bus operation timed out or faulted.

Reading `0x0A` does not clear the status; the next bus operation does.  A bulk
write latches a fault for the whole command: the remaining payload is consumed
and answered NACK without further bus activity.  A normal slave NACK is not a
bus fault.  See [clock-stretching.md](clock-stretching.md).

### Configure peripherals (`0x4x`)

The low four bits set the board state: bit 3 = power, bit 2 = pull-ups, bit 1
= AUX line, bit 0 = CS line.  For example `0x4C` = power on + pull-ups on.

The AUX bit and the extended AUX command (`0x09`) drive the AUX pin.  If AUX
is jumpered to a bus pin, release AUX (`0x09 0x02`, high-Z) before bus traffic
when it is jumpered to SDA or SCL.

### Pull-up feed (`0x5x`, v4 only)

The low nibble selects the on-board pull-up source and applies it:

| Operand | Source |
|---|---|
| `0x51` | on-board +3.3 V |
| `0x52` | on-board +5.0 V |
| any other | both off |

The reply is `0x01` when the request was applied and `0x00` when it was
refused, because the pins are not open-drain HiZ or a voltage is already on
VPU.  This group is shared across modes; see
[sub-protocols.md](../binary/sub-protocols.md) §`0x5x`.

### Write-then-read (`0x08`)

After the command byte:

1. a **big-endian 16-bit** write count,
2. a **big-endian 16-bit** read count,
3. the write payload (the first byte is the device address).

The firmware sends a start, writes the payload, and checks the slave ACK after
each byte.  If the read count is non-zero and more than one byte was written,
it sends a repeated start and the read address (device address with the R/W
bit set).  It then reads the requested bytes, ACKing all but the last (NACK),
sends a stop, replies `0x01`, and finally sends the read bytes to the host.

Bounds: the binary payload buffer is **4096 bytes on v4, 2048 on v3** (not the
small command line).  The write count must be at least 1 — the first byte is
the address — while the read count may be 0.  A zero write count, or either
count over the limit, is answered `0x00` with **no payload consumed and no bus
traffic**.

A slave NACK during the write phase sends a stop and answers `0x00`; start a
new transaction normally.  A bus timeout releases both pins, sends no stop,
and answers `0x00`; `0x0A` then reads `0x00`.  Binary I2C uses the software
driver only — the nine-clock bus-clear belongs to the interactive hardware
driver, so there is no automatic bus-clear here.  Start fresh; if SCL is still
held, the start times out again.

### Groups not implemented

I2C has no `0x2x`, `0x3x`, `0x8x`, `0x9x`, `0xAx` or `0xFx` group; those
request bytes answer `0x00`.  (`0x0A` belongs to the action group and is
documented above.)

### AUX (`0x09`)

After `0x09` the host sends one sub-command byte.  The reply is always **two
bytes**: `0x01` (success) and then the sub-result.

| Sub-command | Action | Sub-result |
|---|---|---|
| `0x00` | drive the selected pin low | `0x01` |
| `0x01` | drive the selected pin high | `0x01` |
| `0x02` | release the selected pin to high-Z | `0x01` |
| `0x03` | read the selected pin | the level (`0x00` / `0x01`) |
| `0x10` | select `AUX` as the target | `0x01` |
| `0x20` | select `CS` as the target | `0x01` |
| other | ignored; the result stays `0x01` | `0x01` |

The selected pin is the current AUX target (`AUX` by default).  `0x10`
switches the extended-AUX control and the shared `0x7x` wait-for-edge command
to `AUX`; `0x20` switches both to `CS`.  These two sub-commands only change the
target — they do not drive the pin.  No text line is printed.

## Sniffer in binary mode (`0x0F`)

`0x0F` enters the same sniffer as macro `(2)`, but frames the output for a
host parser.  The sniffer runs until the host sends one exit byte.  The reply
`0x01` is sent **after** that exit byte, not at entry.

| Token | Meaning |
|---|---|
| `[` | start condition |
| `]` | stop condition |
| `\` + one raw byte | one data byte |
| `+` | ACK (SDA low) after that byte |
| `-` | NACK (SDA high) after that byte |

A data byte therefore appears as three bytes: `\`, the value, then `+` or
`-`.  Start and stop markers are single bytes.  Bytes are sampled on SCL
rising edges; the ninth bit is reported as `+`/`-`.  The output ring buffer is
4096 bytes on v4 and 2048 on v3; overflow drops silently.  See
[sniffer.md](sniffer.md).

## Worked exchange

Host → Bus Pirate and Bus Pirate → host.  The frames write `0xAA 0xBB` at
address `0x0020` of a 24xx64-style EEPROM at 7-bit `0x50`, wait for the write
cycle, then read back through `0x08`.

```
host -> 0x00 x20        enter bitbang
host <- "BBIO1"
host -> 0x02            select I2C
host <- "I2C1"
host -> 0x51            enable on-board 3.3 V pull-up feed (v4)
host <- 0x01            (0x00 = refused / already present)
host -> 0x4C            power + pull-ups on
host <- 0x01
host -> 0x09 0x02       extended AUX: set AUX high-Z
host <- 0x01            success
host <- 0x01            sub-result
host -> 0x60            speed = software profile 0

; write frame [ 0xA0 0x00 0x20 0xAA 0xBB ]
host -> 0x02            start
host <- 0x01
host -> 0x14            bulk-write 5 bytes
host <- 0x01
host -> 0xA0            slave write address
host <- 0x00            slave ACK
host -> 0x00            address high
host <- 0x00
host -> 0x20            address low
host <- 0x00
host -> 0xAA            data
host <- 0x00
host -> 0xBB            data
host <- 0x00
host -> 0x03            stop (starts the write cycle)
host <- 0x01

; write-cycle poll: repeat until the address ACKs
host -> 0x02            start
host <- 0x01
host -> 0x10            bulk-write 1 byte
host <- 0x01
host -> 0xA0            slave write address
host <- 0x00            ACK = ready (0x01 = busy, repeat)
host -> 0x03            stop
host <- 0x01

; write-then-read [ 0xA0 0x00 0x20 ] then 2 bytes
host -> 0x08
host -> 0x00 0x03       write count = 3
host -> 0x00 0x02       read count = 2
host -> 0xA0 0x00 0x20  write payload (address + pointer)
host <- 0x01            success
host <- 0xAA 0xBB       read bytes
```

The first write frame uses the 24xx64's two-byte memory address.  The pointer
must be set again before reading; `0x08` does that with one repeated start.
A one-byte-address part (`24LC08`-style) would use one address byte instead.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| action / bulk / speed / `0x08` groups | present | present |
| set the pull-up feed (`0x5x`) | — (answered `0x00`) | present |
| binary payload buffer | 2048 bytes | 4096 bytes |

The v3 column is **not verified**.
