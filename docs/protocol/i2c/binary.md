# I2C — binary (`I2C1`) sub-protocol

Part of the I2C documentation set (`docs/protocol/i2c/`). This page owns the
binary/host-programming interface to I2C mode: entry, defaults, the command set,
configuration, bulk transfers, write-then-read and the binary sniffer framing.
The general binary layer — entering `BBIO1`, framing, the shared
`0x4x`/`0x5x`/`0x6x`/`0x7x` groups — is owned by
[docs/protocol/binary/](../binary/README.md), especially
[entering-and-framing.md](../binary/entering-and-framing.md),
[commands.md](../binary/commands.md) and
[sub-protocols.md](../binary/sub-protocols.md).

## Entry and exit

1. Send 20 consecutive `0x00` bytes from the terminal; the binary layer replies
   `BBIO1`.
2. Send `0x02` to enter I2C; the mode replies `I2C1` and starts its command
   loop.
3. Send `0x00` to leave I2C; the layer replies `BBIO1`.
4. From `BBIO1`, send `0x0F` to return to the terminal through a reset.

`0x0F` is **not** an I2C exit: inside `I2C1` it starts the sniffer. Exit with
`0x00` first. `0x00` does not send a stop, so finish an open transaction with
`0x03` before leaving. The entry bytes are also only meaningful in `BBIO1`:
inside `I2C1`, `0x02` is a start and `0x03` is a stop, not a mode change.

The binary path is always the **software** (bit-banged) driver, regardless of
which driver the interactive mode last used. There is no terminal `W` step in
the binary path.

## Entry defaults

| Setting | Binary entry value |
|---|---|
| driver | software (bit-banged) |
| speed | software profile 3, `Maximum speed` |
| byte width | 8 bits; a 16-bit width is forced off |
| bit order | MSB-first; the terminal `L` setting does not carry over |
| output type | open drain (high = high impedance); no `0x8x` group |
| SDA / SCL | inputs, released |
| CS | output, driven low (not used by I2C) |
| AUX target | `AUX` (the shared `0x7x` wait uses it) |
| AUX pin | released (input) until `0x09` or `0x4x` drives it |
| on-board VPU source | both off; send `0x5x` if you need one |
| pull-up switch | off |
| target supply | off |
| deferred ACK | not used; the binary path sends ACK/NACK explicitly |

Entry releases SDA, SCL and AUX, drives CS low, and turns the target supply, the
pull-up switch and the on-board VPU source off. Set them explicitly with `0x4x`
(or `0x5x` plus `0x4x` bit 2) when you need them.

The binary speed is the software profile number (0–3), not the terminal menu
number (1–4): profile 3 is `Maximum speed`, which is terminal menu 4. `0x6x`
changes it; see [speeds.md](speeds.md).

## Commands

The high nibble of each request byte is the command group; the low nibble is
the operand.

| Request | Group | Meaning | Reply |
|---|---|---|---|
| `0x00` | action | exit to `BBIO1` | `BBIO1` |
| `0x01` | action | report identifier | `I2C1` |
| `0x02` | action | send start | `0x01`; `0x00` on a bus fault |
| `0x03` | action | send stop | `0x01`; `0x00` on a bus fault |
| `0x04` | action | read one byte | the byte (`0xFF` on a bus fault) |
| `0x06` | action | send ACK | `0x01`; `0x00` on a bus fault |
| `0x07` | action | send NACK | `0x01`; `0x00` on a bus fault |
| `0x08` | action | write-then-read | `0x01`, then the read bytes; `0x00` on failure |
| `0x09` | action | extended AUX (one sub-command byte follows) | `0x01`, then the sub-result (always exactly two bytes) |
| `0x0A` | action | last bus status | `0x01` no fault; `0x00` timeout/fault |
| `0x0F` | action | enter the sniffer; the next host byte exits | `0x01` after the exit byte |
| `0x1x` | bulk | write `x + 1` bytes | `0x01`, then one ACK bit per byte |
| `0x4x` | configure | power, pull-ups, AUX, CS | `0x01` |
| `0x5x` | configure | set the pull-up feed (**v4 only**) | `0x01` applied / `0x00` refused |
| `0x6x` | configure | set the software profile (low 2 bits) | `0x01` |
| `0x7x` | wait | wait for an edge on the selected AUX target | result byte, then the level |
| other | — | unrecognised | `0x00` |

`0x01` is success for a command. For a written byte in the `0x1x` group the
per-byte reply is the **slave's** ACK bit: `0x00` = ACK, `0x01` = NACK — the
opposite sense to the layer's own result byte.

### Errors and `0x0A`

`0x02`, `0x03`, `0x06` and `0x07` reply `0x00` when the operation could not
complete (a clock-stretch timeout or a bus fault). `0x04` keeps its single-byte
reply; on a fault that byte is `0xFF`, so by itself it does not distinguish a
fault from a legitimate `0xFF` read. Use `0x0A` for that:

- `0x01` — the last bus operation had no fault.
- `0x00` — the last bus operation timed out or faulted.

Reading `0x0A` does not clear the status; the next real bus operation does. A
request that is refused before touching the bus (a zero or over-limit `0x08`
count) does not change it either. A bulk write latches a fault for the whole
command: the remaining payload is consumed and answered NACK without further bus
activity. A normal slave NACK is **not** a bus fault. See
[clock-stretching.md](clock-stretching.md).

### Groups not implemented

I2C has no `0x2x`, `0x3x`, `0x8x`, `0x9x`, `0xAx` or `0xFx` group; those request
bytes answer `0x00`. `0x0A` belongs to the action group and is the documented
exception. In particular, there is no `0x8x` output-type group: binary I2C is
always open drain.

## Shared configuration groups

The bit layouts and the shared refusal rules are in
[sub-protocols.md](../binary/sub-protocols.md); only the I2C effect is repeated
here.

### `0x4x` — peripherals

| Bit | Effect in I2C |
|---|---|
| 3 | target supply (VREG) on/off; no rail check |
| 2 | pull-up resistors on/off; the voltage comes from `0x5x` or Vpullup |
| 1 | AUX low/high; the bit **forces AUX to an output** |
| 0 | CS released (set) / driven low (clear) |

`0x4C` = supply on, pull-ups on, AUX low, CS driven low. If AUX is jumpered to
SDA or SCL, the bit-1 level clamps the bus; release AUX with `0x09 0x02` before
any bus traffic.

### `0x5x` — pull-up feed (v4 only)

| Operand | Source |
|---|---|
| `0x51` | on-board +3.3 V |
| `0x52` | on-board +5.0 V |
| any other | both off |

The reply is `0x01` when applied and `0x00` when refused. The shared refusal
reasons are: the outputs are not open drain, the ADC is owned by an SMPS
feedback loop, or a voltage is already present on Vpullup. In I2C the first
never applies, because the mode is always open drain. Entry leaves both sources
off, so send `0x5x` before `0x4x` bit 2 when the on-board pull-ups are the ones
you want. On v3 the group is not present and is answered `0x00`.

### `0x6x` — speed

The low two bits select the software profile: `0x60` = `~6kHz`, `0x61` =
`~60kHz`, `0x62` = `~150kHz`, `0x63` = `Maximum speed`. Higher operands are
masked to the low two bits, so `0x64` selects profile 0 and `0x6F` profile 3.
The reply is `0x01`. The hardware engine is not reachable from this group.

### `0x7x` — wait for an edge

Only sub-command 0 is defined. The payload and reply are owned by
[commands.md](../binary/commands.md#wait-for-an-edge-0x70): an edge selector, a
16-bit big-endian timeout in milliseconds, then a result byte and the selected
pin level. The wait uses the selected AUX target (`AUX` after entry; `0x20`
selects `CS`). It does not change the pin's direction, so configure it as an
input first.

## Write-then-read (`0x08`)

After the command byte:

1. a **big-endian 16-bit** write count,
2. a **big-endian 16-bit** read count,
3. the write payload (the first byte is the device address).

The firmware sends a start, writes the payload, and checks the slave ACK after
each byte. If the read count is non-zero **and** more than one byte was written,
it sends a repeated start and the read address (the first payload byte with the
R/W bit set). It then reads the requested bytes, ACKing all but the last
(NACK), sends a stop, replies `0x01`, and finally sends the read bytes to the
host.

### Bounds and refusals

- Both counts are limited by the **4096-byte terminal output buffer**, which is
  the same on v3 and v4. This is not the command line buffer (128 bytes on v3,
  256 on v4).
- The write count must be at least 1 — the first byte is the address —
  while the read count may be 0.
- A zero write count, or either count over the limit, is answered `0x00` with
  **no payload consumed and no bus traffic**. The two count words have already
  been consumed when the check runs.
- The refusal runs before the sticky bus status is cleared, so a refused `0x08`
  does not change what `0x0A` reports.
- Send the whole payload. The firmware reads exactly `write count` bytes before
  it touches the bus; a short payload stalls the command until the missing
  bytes arrive.

### Transaction shapes

| Write count | Read count | On the bus |
|---:|---:|---|
| 1 | 0 | start, one byte, stop — a write-only transaction |
| 2 or more | 0 | start, payload, stop — a write-only transaction |
| 1 | 1 or more | start, one byte, then the read bytes directly; **no repeated start and no read address is sent** |
| 2 or more | 1 or more | start, payload, repeated start, read address, read bytes |

The write-count-1 read form is the way to read when the device pointer is
already set: send the **read** address (for example `0xA1`) as the single byte,
and the following reads stay in the same transaction. With two or more write
bytes, the first byte is treated as the write address and the repeated start
turns it into the read address.

### Failures

- A slave NACK in the write phase, or on the repeated-start read address, sends
  a stop and answers `0x00`. Start a new transaction normally.
- A bus timeout releases both pins, sends no stop, and answers `0x00`; `0x0A`
  then reads `0x00`. Start fresh; if SCL is still held, the start times out
  again.
- Binary I2C uses the software driver only. The nine-clock bus-clear belongs to
  the interactive Hardware driver, so there is no automatic bus-clear here.

## Bulk write (`0x1x`)

`0x1n` writes `n + 1` bytes (`0x10` writes one, `0x1F` writes sixteen). The
reply is one `0x01`, then one ACK bit per byte as it is written. The command
does not send a stop; send `0x03` when the frame is complete.

- The command consumes exactly `n + 1` payload bytes. A command byte sent early
  is written to the bus as data.
- A NACK does not abort the bulk command: the remaining bytes are still sent and
  each ACK/NACK is reported. The host can stop and restart if a slave is
  ignoring the frame.
- A bus fault **does** latch: the remaining payload is consumed and answered
  NACK with no further bus activity. `0x0A` then reads `0x00`.

## AUX (`0x09`)

After `0x09` the host sends one sub-command byte. The reply is always exactly
two bytes: `0x01` (the command was accepted), then the sub-result. The first
byte is not the sub-command's success flag; it is `0x01` even when the
sub-command is unknown.

| Sub-command | Action | Sub-result |
|---|---|---|
| `0x00` | drive the selected pin low | `0x01` |
| `0x01` | drive the selected pin high | `0x01` |
| `0x02` | release the selected pin to high impedance | `0x01` |
| `0x03` | read the selected pin (the pin is made an input first) | the level (`0x00` / `0x01`) |
| `0x10` | select `AUX` as the target | `0x01` |
| `0x20` | select `CS` as the target | `0x01` |
| other | none; the result is zero | `0x00` |

The selected pin is the current AUX target (`AUX` by default). `0x10` switches
the extended-AUX control and the shared `0x7x` wait to `AUX`; `0x20` switches
both to `CS`. These two sub-commands only change the target; they do not drive
the pin. For the drive, release and select sub-commands the sub-result is
`0x01`; an unknown sub-command returns `0x01 0x00`, where the `0x00` means it
was not recognised. A `0x03` read returns the pin level, so `0x00` there is a
valid low.

If AUX is jumpered to SDA or SCL, release it (`0x09 0x02`, high-Z) before bus
traffic.

## Sniffer in binary mode (`0x0F`)

`0x0F` enters the same sniffer as macro `(2)`, but frames the output for a host
parser. The sniffer runs until the host sends one exit byte; that byte is
consumed. The reply `0x01` is sent **after** that exit byte, not at entry.

| Token | Meaning |
|---|---|
| `[` | start or repeated start condition |
| `]` | stop condition |
| `\` + one raw byte | one data byte |
| `+` | ACK (SDA low) after that byte |
| `-` | NACK (SDA high) after that byte |

A data byte therefore appears as three bytes: `\`, the value, then `+` or `-`.
Start and stop markers are single bytes. Bytes are sampled on SCL rising edges;
the ninth bit is reported as `+`/`-`. The output path and its overflow behaviour
are in [sniffer.md](sniffer.md).

## Worked exchange

Host → Bus Pirate and Bus Pirate → host. The frames write `0xAA 0xBB` at
address `0x0020` of a 24xx64-style EEPROM at 7-bit `0x50`, wait for the write
cycle, then read back through `0x08`.

```
host -> 0x00 x20        enter bitbang
host <- "BBIO1"
host -> 0x02            select I2C
host <- "I2C1"
host -> 0x51            on-board 3.3 V pull-up feed (v4)
host <- 0x01            (0x00 = refused / already present)
host -> 0x4C            supply + pull-ups on
host <- 0x01
host -> 0x09 0x02       extended AUX: release AUX to high-Z
host <- 0x01            command accepted
host <- 0x01            sub-result
host -> 0x60            speed = software profile 0
host <- 0x01

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
host <- 0x01            NACK = busy (0x00 = ready)
host -> 0x03            stop
host <- 0x01

; write-then-read [ 0xA0 0x00 0x20 ] then 2 bytes
host -> 0x08
host -> 0x00 0x03       write count = 3
host -> 0x00 0x02       read count = 2
host -> 0xA0 0x00 0x20  write payload (address + pointer)
host <- 0x01            success
host <- 0xAA 0xBB       read bytes

host -> 0x00            leave I2C
host <- "BBIO1"
host -> 0x0F            return to the terminal (v4: back to HiZ)
host <- 0x01
```

The first write frame uses the 24xx64's two-byte memory address. The pointer is
set again inside `0x08` by its repeated start. A one-byte-address part
(`24LC08`-style) would use one address byte instead.

On v3 the `0x51` step is answered `0x00`; it is harmless. The final `0x0F`
returns to the terminal through a reset, so the board comes back in HiZ.

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| action, bulk, speed, `0x08` groups | present | present |
| pull-up feed (`0x5x`) | absent — answered `0x00` | present |
| write-then-read buffer | 4096 bytes | 4096 bytes |
| hardware engine from binary | no | no |
| `0x0F` return to the terminal | full reset | reset to HiZ |

The v3 column is **not verified**.

## Related docs

- [speeds.md](speeds.md) — the profile table and `0x6x`.
- [clock-stretching.md](clock-stretching.md) — `0x0A` and failure recovery.
- [sniffer.md](sniffer.md) — the interactive sniffer's output.
- [eeprom.md](eeprom.md) — the `I2CEEPROMWIN` client that uses this layer.
- [docs/protocol/binary/sub-protocols.md](../binary/sub-protocols.md) — the
  shared group layouts.
