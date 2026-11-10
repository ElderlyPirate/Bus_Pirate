# ADC probe — binary layer

Part of the ADC capability set (`docs/capabilities/adc/`).  This page covers the
two BBIO1 commands that read the probe: `0x14` (one sample) and `0x15`
(continuous).  Entry, reply conventions and byte framing are owned by
[docs/protocol/binary/](../../protocol/binary/README.md); the scale is in
[probe.md](probe.md).

## Scope

Covered: the request/reply shape, byte order, the no-acknowledgement rule, the
continuous stream and its stop contract, and a worked exchange.  Not covered:
how to enter the binary layer, the shared result bytes, or any other command.

## Commands

Both are **BBIO1** commands.  They take no operand bytes and return no result
or error byte.

| Request | Reply | Notes |
|---|---|---|
| `0x14` | 2 bytes, high byte first | one conversion on the probe channel |
| `0x15` | a stream of 2-byte samples | runs until the host sends any byte |

The sample is the raw 10-bit count, so the high byte is `0x00`..`0x03`.

## One-shot — `0x14`

Enter the bitbang layer first: send 20 × `0x00` and wait for `BBIO1` (see
[entering and framing](../../protocol/binary/entering-and-framing.md)), then
send `0x14`:

```
host -> 0x14
host <- 0x02 0x00         sample 512, about 3.29 V
```

- There is **no leading `0x01` acknowledgement** and no failure byte.  A host
  that waits for an ack reads the sample bytes as that ack and loses frame
  alignment.
- Read exactly two bytes.  `0x0200` is 512.
- The conversion uses the probe channel; the reply has no units and no
  conversion to volts.  Convert on the host with `raw / 1024 × 6.6`.
- The layer stays at `BBIO1`, ready for the next command.

## Continuous — `0x15`

`0x15` starts streaming immediately; the first two bytes on the wire are
already a sample.  There is no acknowledgement, no count, no length and no
terminator.

```
host -> 0x15
host <- 0x02 0x00 0x01 0xFF 0x02 0x00 ...
host -> 0x00             stop (consumed)
```

### Stopping the stream

The stop is a drain, not a clean cut:

1. Send any byte.  It is the stop and is **consumed**: it is not executed as a
   command.
2. Keep reading 2-byte samples until none arrive within your read timeout.
3. Only then send the next command.

What that means:

- Samples already generated and buffered are still delivered after the stop.
  On v4 the output is buffered, so several can be waiting; the exact backlog
  depends on the host's USB stack and how promptly it reads.
- The board checks for the stop at the loop boundary, after a sample has been
  generated and handed to the transmit path.  While that path is blocked
  because its output buffer is full, the board cannot see the stop at all.
- There is no end marker.  The only signal that the stream has ended is that
  no sample arrives within your timeout.
- There is no sample history: buffered output is delivered, but a host that
  stops reading cannot recover the samples it missed.

### After the stop

- Because the stop is a normal byte, `0x00` stops the stream without
  re-printing `BBIO1`; if you want the identifier, send another `0x00`.
- A command sent instead of a stop byte is swallowed.  Send the real command
  again after the stop.
- Once the drain is complete the layer is still at `BBIO1`; no identifier is
  re-printed.

## Rate and timing

- No rate is quoted: it depends on the host, the USB or serial link, and how
  fast the host drains the samples.  The loop itself is unthrottled — one
  sample per loop: sample, transmit; the stream pauses if the host falls
  behind.
- Read 2-byte samples in a loop with a timeout rather than expecting a fixed
  count.
- Samples carry no timestamp.  The byte order is preserved; only the arrival
  timing is uneven.  On v4 the USB link buffers output, so bytes can arrive in
  bursts that do not reflect the conversion instants.  Do not use `0x15` for
  timing or for evenly spaced data.
- A byte already waiting in the host-to-board direction stops the loop after
  the first sample.
- The sample is 10 bits: `0x0000`..`0x03FF`.

## Inside a sub-protocol

`0x14` and `0x15` are BBIO1 commands.  Inside SPI, I2C, UART, 1-Wire or `RAW1`
the `0x1x` range is the bulk transfer group, so the same byte is a different
command.  Return to `BBIO1` before reading the probe.  See
[sub-protocols.md](../../protocol/binary/sub-protocols.md).

## Related

- [probe.md](probe.md) — the scale and the limits of the reading.
- [terminal.md](terminal.md) — the same readings from the prompt.
- [docs/protocol/binary/commands.md](../../protocol/binary/commands.md) — the
  BBIO1 map and the shared error byte.
- [docs/protocol/binary/edge-cases.md](../../protocol/binary/edge-cases.md) —
  the layer-wide failure modes.
