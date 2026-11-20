# SPI — binary (`SPI1`) sub-protocol

Part of the SPI documentation set (`docs/protocol/spi/`). This page documents
the binary/host-programming interface to SPI mode: entry, defaults, the
command set, configuration, bulk transfers, the AVR helper and the binary
sniffer framing. The general binary layer — entering `BBIO1`, framing, the
shared `0x4x`/`0x5x`/`0x6x`/`0x7x` groups — is owned by
[docs/protocol/binary/](../binary/README.md), especially
[entering-and-framing.md](../binary/entering-and-framing.md),
[commands.md](../binary/commands.md) and
[sub-protocols.md](../binary/sub-protocols.md).

## Entry and exit

1. Send 20 consecutive `0x00` bytes from the terminal; the binary layer replies
   `BBIO1`.
2. Send `0x01` to enter SPI; the mode replies `SPI1` and starts its command
   loop.
3. Send `0x00` to leave SPI; the layer replies `BBIO1`. From `BBIO1`, `0x0F`
   returns to the terminal through a reset.

`0x0F` is **not** an SPI command. Inside `SPI1` it is an unrecognised byte and
is answered `0x00`; exit to `BBIO1` with `0x00` first. The entry bytes are
also only meaningful in `BBIO1`: inside `SPI1`, `0x08` and `0x09` are unknown
actions, `0x10` is a one-byte bulk transfer and `0x18` is a nine-byte bulk
transfer — not the XSVF entry. `0x06` is the `BBIO1` OpenOCD/JTAG entry but the
extended AVR helper inside `SPI1`. The configure-SPI group is `0x80`–`0x8F`
(`0x8x`), not `0x08`.

Entering `SPI1` configures the pins and the SPI module itself. There is no
terminal `W` step in the binary path.

## Entry defaults

The binary path starts in its own defaults, independent of the last terminal
session:

| Setting | Binary entry value |
|---|---|
| speed | table index 1 = `125kHz` |
| clock polarity | idle low |
| output clock edge | active to idle |
| input sample phase | middle |
| output type | open drain (high = Hi-Z) |
| CS polarity | `/CS` (active-low) |
| CS idle state | released (high; Hi-Z in open drain) |
| target supply | off |
| pull-up resistors | off |
| on-board VPU sources (v4) | both off |

The speed index is the raw table index, not the terminal menu number: index 1
is `125kHz`, while terminal menu option 1 is `30kHz`. The mapping is in
§Set speed.

## Electrical setup in binary mode

The entry defaults leave the bus in **open drain with no pull-ups and the
supply off**. The high level of CS, CLK and MOSI is therefore high impedance,
and the target needs pull-ups to see a valid high:

- enable the Bus Pirate pull-ups with `0x4x` bit 2 **and** feed Vpullup — on
  v4 with `0x5x` (`0x51` = 3.3 V, `0x52` = 5.0 V), on v3 externally; or
- fit external **2 kΩ – 10 kΩ** pull-ups to the target rail; or
- select **Normal** output with `0x8x` bit 3, which drives 3.3 V high and needs
  no pull-up (but cannot produce a 5 V high).

**Match the Vpullup source to the target's logic supply:** use `0x51` for a
3.3 V target and `0x52` only for a 5 V target. A 5 V pull-up can damage a
3.3 V target. See [wiring.md](wiring.md#the-pull-up-reference-vpullup).

CS is released high (Hi-Z) between transactions in open drain. If the target
has no CS pull-up, it can see CS as asserted; enable the pull-ups or use Normal
output.

The target supply is controlled by `0x4x` bit 3. Unlike the terminal `W`, the
binary group has no rail check: `0x4x` replies `0x01` when the state was
written, not when the rails came up.

MISO is driven by the target. The interface limit is 5 V safe / 5.5 V maximum.

## Commands

The high nibble of each request byte is the command group; the low nibble is
the operand.

| Request | Group | Meaning | Reply |
|---|---|---|---|
| `0x00` | action | exit to bitbang | `BBIO1` |
| `0x01` | action | report identifier | `SPI1` |
| `0x02` | action | CS low (assert; enables the output) | `0x01` |
| `0x03` | action | CS high (release: input/Hi-Z in open drain, driven high in Normal) | `0x01` |
| `0x04` | action | write-then-read, CS asserted for the transfer | `0x01`, then the read bytes |
| `0x05` | action | write-then-read, CS left untouched | `0x01`, then the read bytes |
| `0x06` | action | extended AVR command | `0x01`, then the sub-reply |
| `0x0D` | action | sniffer, all traffic | `0x01`, then the stream |
| `0x0E` | action | sniffer, only while CS is low | `0x01`, then the stream |
| `0x1x` | bulk | write `x + 1` bytes, return the bus byte for each | `0x01`, then one byte per input |
| `0x4x` | configure | power, pull-ups, AUX, CS | `0x01` |
| `0x5x` | configure | set the pull-up feed (**v4 only**) | `0x01` / `0x00` |
| `0x6x` | configure | set speed; the low nibble indexes the table | `0x01` / `0x00` |
| `0x7x` | wait | wait for an edge on the selected AUX pin (only `0x70`) | result byte, then the level |
| `0x8x` | configure | set the clock and output options | `0x01` |
| other | — | unrecognised | `0x00` |

`0x01` is success/ack; `0x00` is failure. `0x02` writes the CS latch low and
makes CS an output, so it asserts. `0x03` writes the latch high and releases
CS: an input (Hi-Z) in open drain, driven high in Normal. Both set the
direction; the `0x4x` bit-0 interaction is in §Configure peripherals (`0x4x`).

### Write-then-read (`0x04`, `0x05`)

After the command byte:

1. a **big-endian 16-bit** write count,
2. a **big-endian 16-bit** read count,
3. the write payload (the bytes to clock out on MOSI).

The firmware writes the payload, then clocks in the requested number of bytes
and returns them. `0x04` uses the same CS helper: it drives CS low and makes
it an output at the start, then releases it (input/Hi-Z in open drain, driven
high in Normal) at the end. `0x05` does not touch CS. The reply
is one `0x01` success byte, followed by the read bytes. For a write-only
request (read count 0) the `0x01` comes after the write; for a read-only
request it comes before the read data.

A write count and a read count of **both zero** is answered with one `0x01`
and no bus traffic; CS is not changed. A client uses this during
initialisation.

The counts are 16-bit and are not checked against the terminal output buffer;
the data streams. CS control is by physical level, not by the terminal
CS-polarity setting, which is fixed to `/CS` in binary mode.

### Bulk transfer (`0x1x`)

`0x1x` writes `x + 1` bytes from the host to the bus and returns the bus byte
read for each one. `x = 0` writes one byte; `x = 15` writes sixteen. The reply
is one `0x01`, then one bus byte for each host byte. A fresh `0x1x` command is
sent for each group of up to sixteen bytes. This group does not manage CS: if
the target needs CS asserted, use `0x02` (it asserts and enables the output;
see §Configure peripherals for the `0x4x` bit-0 level).

The host must supply exactly `x + 1` bytes; the loop waits for them. There are
no streaming read/write commands in this build; use `0x04`/`0x05` or `0x1x`.

### Transfer faults

Peripheral polling is bounded, so a stalled SPI module cannot spin forever.
The request is completed with filler instead:

- unread read bytes are returned as `0xFF`;
- the reply length is unchanged — the host still receives exactly the number
  of bytes it asked for;
- the write payload is still consumed from the host, so it cannot be read
  later as commands.

`0xFF` means "no data", not an error status: there is no separate error byte in
the reply. The same fill applies to `0x04`, `0x05`, `0x1x`, the `0x06` bulk
read and the terminal `r`/`r:N`; a real all-ones target byte looks the same.
The bound applies to the SPI module, not to a slow host: waiting for the next
payload byte with no transfer pending does not consume the fault budget.

### Configure peripherals (`0x4x`)

The low four bits set the board state:

| Bit | Mask | When set | When clear |
|---:|---|---|---|
| 3 | `0x08` | target supply on | supply off |
| 2 | `0x04` | pull-up resistors on | pull-ups off |
| 1 | `0x02` | AUX high | AUX low |
| 0 | `0x01` | CS released | CS low / asserted |

The reply is `0x01`; there is no rail check.

Bit 0 is a level, not a pulse:

| Bit 0 | CS in Normal output | CS in open drain |
|---|---|---|
| set | driven high (released) | input / Hi-Z (released) |
| clear | driven low (asserted) | driven low (asserted) |

Every `0x4x` request also drives AUX as an output; bit 1 sets its level. For
example `0x4C` = power on + pull-ups on + AUX low + CS low; `0x4D` does the
same with CS released. If the target is self-powered, clear bit 3: `0x44` is
pull-ups on + AUX low + CS low.

**CS direction interaction.** `0x4x` bit 0 still sets CS directly: set releases
it (an input/Hi-Z in open drain, driven high in Normal); clear drives it
low/asserted. `0x02`, `0x03` and a non-empty `0x04` set the direction
themselves: `0x02` drives CS low and makes it an output, so it recovers a CS
left as an input by `0x4x`; `0x03` releases CS (input/Hi-Z in open drain,
driven high in Normal); `0x04` asserts through the same helper at the start and
releases at the end. `0x6x`/`0x8x` re-run the module setup, which also leaves CS
an output at its released level. A second `0x4x` is not needed for recovery.

The AUX bit here also forces `A0` to an output; the shared group semantics are
owned by [sub-protocols.md](../binary/sub-protocols.md).

### Pull-up feed (`0x5x`, v4 only)

Sets the on-board source feeding the pull-up voltage (Vpullup):

| Request | Source |
|---|---|
| `0x51` | on-board **+3.3 V** |
| `0x52` | on-board **+5.0 V** |
| any other | both off |

**Use `0x51` for a 3.3 V target and `0x52` only for a 5 V target; a 5 V
pull-up can damage a 3.3 V target.** The Vpullup reference rules are in
[wiring.md](wiring.md#the-pull-up-reference-vpullup).

The reply is one byte: `0x01` when the request was applied and `0x00` when it
was refused. A request is refused when the pins are not open-drain high
impedance, or when a voltage is already present on Vpullup. Selecting Normal
output with `0x8x` bit 3 therefore makes `0x5x` refuse.

On a v3 build this group is not present and is answered `0x00`. The on-board
sources feed Vpullup, not the target supply; the target supply is `0x4x`
bit 3.

### Set speed (`0x6x`)

The low nibble indexes the speed table directly:

| Index | Rate | Menu | Index | Rate | Menu |
|---:|---|---:|---:|---|---:|
| 0 | `30kHz` | 1 | 6 | `4MHz` | 7 |
| 1 | `125kHz` | 2 | 7 | `8MHz` | 8 |
| 2 | `250kHz` | 3 | 8 | `50kHz` | 9 |
| 3 | `1MHz` | 4 | 9 | `1.3MHz` | 10 |
| 4 | `2MHz` | 5 | 10 | `3.2MHz` | 11 |
| 5 | `2.6MHz` | 6 | 11 | `5.3MHz` | 12 |

An index of 12 or above is out of range and is answered `0x00`. The terminal
menu number is this index + 1; for example menu option 9 (`50kHz`) is index 8,
request `0x68`.

Changing the speed re-runs the module setup, which drives CS to its idle
(released) level. Do not change speed in the middle of a transfer.

### Configure SPI (`0x8x`)

The low bits set the clock and output options:

| Bit | Setting |
|---:|---|
| 0 | sample at **end** (clear = middle) |
| 1 | clock edge **active→idle** (clear = idle→active) |
| 2 | clock idle **high** (clear = idle low) |
| 3 | **Normal** output when set; clear = open drain |

Unset option bits take their own defaults — idle low, idle→active edge,
middle sample, open drain — not the entry defaults. `0x8x` does not change the
speed or the CS polarity. The reply is `0x01`.

Changing the options re-runs the module setup, which drives CS to its idle
(released) level. Do not change options in the middle of a transfer.

The entry default edge is **active→idle**, but an unset `0x8x` bit 1 selects
**idle→active**. To keep the entry timing with normal output, send `0x8A`
(bit 3 = Normal, bit 1 = active→idle); `0x88` selects Normal with the
idle→active edge. The polarity/edge pair has the same CPOL/CPHA meaning as the
terminal setup — see [entry-and-setup.md](entry-and-setup.md).

### Extended AVR command (`0x06`)

`0x06` is acknowledged with `0x01` immediately; one sub-command byte follows:

| Sub-command | Action | Reply after the `0x06` ack |
|---|---|---|
| `0x00` | no operation | `0x01` |
| `0x01` | report protocol version | `0x01`, then `0x00 0x01` |
| `0x02` | bulk read | `0x01`, then the data bytes; `0x00` if the bounds fail |
| other | unrecognised | `0x00` |

A successful `0x06 0x02` therefore returns **two** `0x01` bytes before the data
— the `0x06` acknowledgement and the bulk-read acknowledgement. A refused bulk
read returns `0x01` then `0x00`. A bulk read with length `0` is valid: it
returns the two acknowledgements and no data. A no-op returns `0x01 0x01`; the
version sub-command returns `0x01 0x01 0x00 0x01`, where the last two bytes
are the big-endian version `0x0001` (version 1).

The bulk-read sub-command reads a big-endian 32-bit address and a big-endian
32-bit length, then reads the flash a word at a time: it clocks the AVR
fetch-low-byte command (`0x20`) followed by the 16-bit address and captures a
byte, then the fetch-high-byte command (`0x28`) followed by the address and
captures the next byte. Each 16-bit word is returned **low byte first, then
high byte**; an odd length ends after the low byte.

The two fields use different units:

| Field | Unit |
|---|---|
| address | **word** address (`0x0000`–`0xFFFF`) |
| length | **byte** count (up to `0xFFFF`) |

The request is refused (`0x00`) when `address > 0xFFFF`, `length > 0xFFFF`, or
`address + ceil(length / 2) > 0x10000`. A read that ends on the last word is
valid: address `0xFFFF` with length `2` returns the final 16-bit word. The
final word is readable, but the word address must not wrap past `0xFFFF`, so
address `0xFFFF` with length `3` is refused. If the SPI module stalls during
the read, the remaining bytes are sent as `0xFF` (see Transfer faults).

`0x06` does **not** drive CS. Assert CS with `0x02` before the read (it drives
low as an output) and release it with `0x03` afterwards (input/Hi-Z in open
drain, driven high in Normal). It also does not drive the AVR's reset;
the target must already be in its ISP flash-read state. The helper assumes an
AVR that understands the ISP flash-read commands. Set a speed the AVR's ISP
interface accepts with `0x6x` before the read if the entry default is too fast.

This command is present in the default v3 and v4 builds; a custom build
without the extended AVR option answers `0x00` to `0x06`.

## Sniffer

`0x0D` (all traffic) and `0x0E` (CS low) start the same sniffer as macros
`(2)` / `(1)`, but in a binary byte framing. The mode replies `0x01` and then
streams:

| Element | Bytes |
|---|---|
| CS low | `[` |
| CS high | `]` |
| one clocked byte pair | `\` (escape), then the MOSI byte, then the MISO byte |
| capture aborted (overflow) | one bare `0x00`, outside a `\` pair |

Each clocked byte pair is escaped by a leading `\` byte, followed by the MOSI
byte and then the MISO byte; CS transitions are sent as bare `[` / `]` bytes
and follow the physical CS level, not the mode's CS-polarity setting. The
terminal form writes the same data as text instead: `0xNN(0xNN)` — see
[sniffer.md](sniffer.md).

Format illustration of the transaction used in [sniffer.md](sniffer.md)
(`[0x30(0x00)0xFF(0x12)0xFF(0x50)][0x40(0x00)]`), as the binary stream would
look (byte values in hex):

```
host <- '['
host <- '\' 0x30 0x00     MOSI 0x30, MISO 0x00
host <- '\' 0xFF 0x12
host <- '\' 0xFF 0x50
host <- ']'
host <- '['
host <- '\' 0x40 0x00
host <- ']'
```

The sniffer exits when the host sends any byte; that stopping byte is **not**
consumed, so it is treated as the next command. On overflow the firmware sends
one unescaped `0x00` byte, disables capture, turns the mode LED off and stays
passive until the host sends a byte; that byte is then the next command. Stop
the sniffer only while the external master is quiet — exit runs the master
pin/module setup, so CLK, MOSI and CS become outputs; it does not restore the
previous input state or switch the target supply. See
[sniffer.md](sniffer.md) §Integrity and stopping.

Because the stopping byte becomes the next command, sending `0x00` to stop the
sniffer exits to `BBIO1`. `0x0F` is not a reset inside `SPI1`; leave to
`BBIO1` first.

## Worked frames

The frames below show the byte protocol. They assume the target is powered and
the bus has a valid high level (pull-ups in open drain, or Normal output); they
do not switch the target supply on. The read/data bytes are examples.

### One write and one read

This frame selects SPI, configures normal output with the entry timing and a
250 kHz clock, then writes one byte and reads one byte. `0x04` manages CS
itself, so no separate CS command is sent. The read byte is an example value.

```
host -> 0x00 x20        enter bitbang
host <- "BBIO1"
host -> 0x01            select SPI
host <- "SPI1"
host -> 0x8A            Normal output, idle low, active->idle, middle
host <- 0x01
host -> 0x62            speed index 2 = 250kHz
host <- 0x01
host -> 0x04            write-then-read, CS asserted for the transfer
host -> 0x00 0x01       write count = 1 (big-endian)
host -> 0x00 0x01       read count = 1
host -> 0x9F            write payload
host <- 0x01            success
host <- 0xEF            read byte
host -> 0x00            exit to bitbang
host <- "BBIO1"
```

### Bulk write with CS held low

`0x44` enables the pull-ups, drives AUX low and drives CS low without
switching the target supply on; add bit 3 (`0x4C`) if the Bus Pirate should
power the target. `0x13` writes four bytes and returns the bus byte for each.
The returned bytes are examples.

```
host -> 0x44            pull-ups on, AUX low, CS low (no target supply)
host <- 0x01
host -> 0x13            bulk, four bytes
host -> 0x9F 0x00 0x00 0x00
host <- 0x01
host <- 0xEF 0x00 0x00 0x00
host -> 0x03            CS released
host <- 0x01
```

### AVR bulk read

The two `0x01` bytes are the extended-command ack and the bulk-read ack. The
data bytes are examples.

```
host -> 0x02            CS low
host <- 0x01
host -> 0x06            extended command
host <- 0x01            extended ack
host -> 0x02            bulk read
host -> 0x00 0x00 0x00 0x00   word address 0
host -> 0x00 0x00 0x00 0x04   byte length 4
host <- 0x01            bulk-read ack
host <- 0xAA 0xBB 0xCC 0xDD   low, high, low, high
host -> 0x03            CS released
host <- 0x01
```

## Edge cases

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `0x0F` is sent inside `SPI1` | `0x00`, no reset | send `0x00` to reach `BBIO1`, then `0x0F` |
| `0x18` is sent inside `SPI1` | a nine-byte bulk transfer, not the XSVF entry | entry bytes are only meaningful in `BBIO1` |
| `0x5x` is sent on v3 | `0x00` | the group is v4 only |
| `0x6x` uses index 12–15 | `0x00` | use 0–11 |
| `0x04`/`0x05` both counts are zero | `0x01`, no bus traffic, CS unchanged | intended for initialisation |
| `0x06` without CS | the AVR sees no chip select; the read is meaningless | first assert CS with `0x02` (it enables the output) |
| `0x06` bulk read succeeds | `0x01 0x01` before the data | account for both acks |
| `0x06` bulk read is refused | `0x01 0x00` | address ≤ `0xFFFF`, length ≤ `0xFFFF`, no word wrap |
| `0x06` length is odd | the last word returns only its low byte | expected |
| `0x4x` released CS as input | CS high/Hi-Z, not asserted | `0x02` or a non-empty `0x04` asserts; `0x6x`/`0x8x` restore the released output |
| `0x4x` bit 0 set (open drain) | CS released as an input | expected; `0x02` asserts, `0x03` releases |
| `0x6x` or `0x8x` is sent mid-transfer | CS returns to its idle level | change speed/options between transfers |
| `0x8x` clears an option bit | that bit takes its own default, not the entry default | `0x82` keeps the entry edge; `0x80` selects idle→active |
| `0x5x` is sent after Normal output was selected | `0x00` | select open drain with `0x8x` first |
| `0x4x` turns the supply on | `0x01` even if the rail is shorted | there is no rail check; use the ADC or the terminal to verify |
| A stalled SPI module | read bytes are `0xFF`; the reply length is unchanged | framing, not an error status |
| A read count is large | the data streams; there is no buffer bound | the host must read the promised bytes |

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| action / bulk / speed / `0x04` / `0x05` / `0x8x` groups | present | present |
| pull-up feed (`0x5x`) | replies `0x00` | present |
| extended AVR command (`0x06`) | present | present |
| sniffer (`0x0D` / `0x0E`) | present, polled CS | present, CN CS capture |
| terminal output buffer | 4096 bytes | 4096 bytes |

The v3 column is **not verified**.

## Related docs

- [../binary/entering-and-framing.md](../binary/entering-and-framing.md) —
  BBIO1 entry and framing.
- [../binary/sub-protocols.md](../binary/sub-protocols.md) — the shared
  `0x4x`/`0x5x`/`0x6x`/`0x7x` groups.
- [entry-and-setup.md](entry-and-setup.md) — the terminal settings and CPOL/CPHA.
- [sniffer.md](sniffer.md) — the terminal sniffer and its limits.
- [bus-operations.md](bus-operations.md) — the terminal byte operations.
- [wiring.md](wiring.md#the-pull-up-reference-vpullup) — Vpullup reference
  and electrical limits.
- [edge-cases.md](edge-cases.md) — binary framing and CS-direction failures.
