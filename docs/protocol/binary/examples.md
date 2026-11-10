# Binary layer — worked exchanges

Part of the binary-layer documentation set (`docs/binary/`).  These are format
illustrations: the byte values for read data are examples, and each
sub-protocol's authoritative command set is on its own page (linked from
[sub-protocols.md](sub-protocols.md)).  Entry, replies and framing are in
[entering-and-framing.md](entering-and-framing.md).

In the transcripts, `host ->` is a byte sent to the Bus Pirate and `host <-`
is a byte it returns.

## Enter, then leave

```
host -> 0x00 x20       enter the binary layer
host <- "BBIO1"        binary bitbang, version 1
host -> 0x00           re-print the identifier
host <- "BBIO1"
host -> 0x0F           reset and return to the terminal
host <- 0x01           acknowledgement
host <- "HiZ>"         terminal banner after the reset
```

## Pin control in BBIO1

Drive CS low and CLK high as outputs, then read the pin state.  The reply to
each state or direction byte is the five pin levels (`0x00`–`0x1F`).

```
host -> 0x00 x20       enter bitbang
host <- "BBIO1"
host -> 0x41           direction: CLK output (bit 2 clear); all others input
host <- 0x00           state: all lines low
host -> 0x84           state: CLK high (bit 2 set)
host <- 0x04           state: CLK reads high
host -> 0x80           state: all lines low
host <- 0x00
host -> 0x0F           leave
```

The state byte's class bit (bit 7) is `1`; the direction byte's top bits are
`010`.  See [commands.md](commands.md) for the bit maps.

## Wait for an edge (`0x70`)

Wait for a rising edge with a 100 ms timeout.  The edge selector is `0x01`;
the timeout is big-endian (`0x0064` = 100).

```
host -> 0x70           wait for an edge
host -> 0x01           rising (low -> high)
host -> 0x00 0x64      timeout = 100 ms (big-endian)
host <- 0x01           result: the edge arrived
host <- 0x01           AUX level at the end: high
```

A timeout of `0` checks the pin once and returns immediately, which doubles as
a level sample:

```
host -> 0x70 0x03 0x00 0x00    either edge, zero timeout
host <- 0x00                   result: no (timed out immediately)
host <- 0x01                   AUX level: high
```

An invalid edge selector returns a single failure byte:

```
host -> 0x70 0x00 ...   0x00 is not a valid edge selector
host <- 0x00            failure
```

## Full entry → configure → transfer → exit (SPI)

This frame selects SPI, configures it for normal output and a 250 kHz clock,
writes one byte and reads one byte.  The `0x04` command manages CS for the
transfer, so no separate CS command is sent.  The read byte is an example.

```
host -> 0x00 x20        enter bitbang
host <- "BBIO1"
host -> 0x01            select SPI
host <- "SPI1"
host -> 0x88            configure: normal output, idle low, idle->active
host <- 0x01            success
host -> 0x62            set speed index 2 (250 kHz)
host <- 0x01            success
host -> 0x04            write-then-read (asserts CS for the transfer)
host -> 0x00 0x01       write count = 1 (big-endian)
host -> 0x00 0x01       read count = 1
host -> 0x9F            write payload
host <- 0x01            success
host <- 0xEF            read byte (example)
host -> 0x00            return to the bitbang layer
host <- "BBIO1"
host -> 0x0F            leave for the terminal
host <- 0x01            acknowledgement
host <- "HiZ>"          terminal banner after the reset
```

The SPI command set, speed table and sniffer are in
[../spi/binary.md](../spi/binary.md).  The same skeleton applies to the other
sub-protocols: enter (`0x01`–`0x07`, `0x18`), configure with the shared groups
([sub-protocols.md](sub-protocols.md)), transfer, then `0x00` to `BBIO1` and
`0x0F` to the terminal.

## Related docs

- [commands.md](commands.md) — the BBIO1 command map.
- [sub-protocols.md](sub-protocols.md) — the sub-protocol entries and shared
  groups.
- [edge-cases.md](edge-cases.md) — what to do when a step misbehaves.