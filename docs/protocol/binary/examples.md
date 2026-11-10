# Binary layer — worked exchanges

Part of the binary-layer documentation set (`docs/protocol/binary/`).  These
are layer-level format illustrations: the byte values for read data are
examples, and each sub-protocol's authoritative command set is on its own page
(linked from [sub-protocols.md](sub-protocols.md)).  Entry, replies and framing
are in [entering-and-framing.md](entering-and-framing.md).

In the transcripts, `host ->` is a byte sent to the Bus Pirate and `host <-`
is a byte it returns.

## Enter, then leave

```
host -> 0x00 x20       enter the binary layer
host <- "BBIO1"        binary bitbang, version 1
host -> 0x00           re-print the identifier (no state change)
host <- "BBIO1"
host -> 0x0F           return to the terminal through a reset
host <- 0x01           acknowledgement
host <- "HiZ>"         terminal prompt
```

The version banner is printed before the `HiZ>` prompt; it is omitted here.

## Pin control in BBIO1

Set CLK as the only output and leave the other four lines as inputs, then drive
CLK.  The reply to each state or direction byte is the five pin levels
(`0x00`–`0x1F`).

```
host -> 0x00 x20       enter bitbang
host <- "BBIO1"
host -> 0x5B           direction: CLK output, all others input
host <- 0x00           example: CLK low, inputs read low
host -> 0x84           state: CLK high (bit 2 set)
host <- 0x04           example: CLK high, other inputs low
host -> 0x80           state: all pin latches low
host <- 0x00           example: CLK low, inputs unchanged
host -> 0x0F           return to the terminal
```

The reply shows the **actual** pin levels, not the latched values: a floating
input can read high, so the `0x00`/`0x04` values above are examples only.  The
state byte's class bit (bit 7) is `1`; the direction byte's top bits are `010`.
See [commands.md](commands.md) for the bit maps.

## Wait for an edge (`0x70`)

Make the selected AUX pin an input first — the wait does not change its
direction.  This frame waits for a rising edge with a 100 ms timeout.  The edge
selector is `0x01`; the timeout is big-endian (`0x0064` = 100).

```
host -> 0x5F           direction: all pins inputs
host <- 0x00           example pin levels
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

An invalid edge selector still consumes its timeout.  Send the complete frame:

```
host -> 0x70 0x00 0x00 0x64    selector 0x00 is invalid, timeout 100
host <- 0x00                   failure
```

A host that stops after `0x70 0x00` leaves the layer waiting for the two
timeout bytes.  See [edge-cases.md](edge-cases.md).

## Entry → configure → transfer → exit (mode skeleton)

The complete SPI frame is canonical in
[the SPI binary page](../spi/binary.md) §Worked exchange.  At the layer level
the sequence is the same for the nibble-group modes (`0x01`–`0x07`):

```
enter 20 x 0x00              -> BBIO1
entry byte (0x01-0x07)       -> mode identifier
shared configure groups      -> 0x01 per command
mode transfer commands       -> per mode
0x00                         -> BBIO1
0x0F                         -> 0x01, then the terminal reset
```

XSVF (`0x18`) prints `XSV1` but leaves with `0x0F` (it ignores `0x00`); the
AVRISP entries (`0x08`/`0x09`) print no identifier and have no command loop.
Use the mode's page for its configure bytes, transfer counts and replies.

`0x8x` is per mode: for example `0x88` is normal output in SPI but open drain
in `RAW1`.  Use the selected mode's page.

## Related docs

- [commands.md](commands.md) — the BBIO1 command map.
- [sub-protocols.md](sub-protocols.md) — the sub-protocol entries and shared
  groups.
- [edge-cases.md](edge-cases.md) — what to do when a step misbehaves.
- [the SPI binary page](../spi/binary.md) §Worked exchange — the canonical
  SPI frame.
