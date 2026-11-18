# ADC probe — binary layer

Part of the ADC capability set (`docs/capabilities/adc/`).  This page covers
the two BBIO1 commands that read the probe: `0x14` (one sample) and `0x15`
(continuous).  Entry, reply conventions and byte framing are owned by
[docs/protocol/binary/](../../protocol/binary/README.md); the scale is in
[probe.md](probe.md).

## Scope

Covered: the request/reply shape, byte order, the no-acknowledgement rule, the
continuous stream and its stop contract, the SMPS-owned refusal, and a worked
exchange.  Not covered: how to enter the binary layer, the shared result
bytes, or any other command.

## Commands

Both are **BBIO1** commands.  They take no operand bytes and return no result
or error byte; the SMPS-owned refusal below is the one sample-shaped
exception.

| Request | Reply | Notes |
|---|---|---|
| `0x14` | 2 bytes, high byte first | one conversion on the probe channel |
| `0x15` | a stream of 2-byte samples | runs until the host sends any byte |

The sample is the raw 10-bit count, high byte first, so a valid high byte is
`0x00`..`0x03` and the value decodes as:

```
raw = (first_byte << 8) | second_byte
```

The scale is `raw / 1024 × 6.6`; the firmware sends no units.

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
- The conversion uses the probe channel; convert to volts on the host.
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
- A byte already waiting in the host-to-board direction when `0x15` arrives
  stops the loop after the first sample.

### After the stop

- Because the stop is a normal byte, `0x00` stops the stream without
  re-printing `BBIO1`; if you want the identifier, send another `0x00`.
- A command sent instead of a stop byte is swallowed.  Send the real command
  again after the stop.
- Only one byte is consumed as the stop; a second byte sent with it is read as
  the next command.
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
- The sample is 10 bits: `0x0000`..`0x03FF`.

## SMPS owns the ADC

On an SMPS-enabled v4 build, a running SMPS feedback loop owns the probe
channel ([probe.md](probe.md#who-owns-the-converter)).  The two commands do
not produce ASCII text in the binary stream; the refusal is a sample-shaped
pair:

```
host -> 0x14
host <- 0xFF 0xFF         refusal, not a sample
```

```
host -> 0x15
host <- 0xFF 0xFF         refusal; no stream follows
```

- `0xFF 0xFF` is sent once.  The ADC is 10-bit, so this pair is impossible as
  a real sample (a valid high byte is `0x00`..`0x03`); a host can flag it
  instead of treating it as a valid sample.
- `0x15` does **not** stream after the pair; the layer is ready for the next
  command.  Because no stream starts, a stop byte sent afterwards is **not**
  consumed: it is read as the next command.  Send the next command directly.
- Stop the SMPS with `0xF1` **in raw wire**.  After `0xFF 0xFF` the host is
  in `BBIO1`; send `0x05` first to enter `RAW1`, then `0xF1` (stop; reply
  `0x01`), then `0x00` to return to `BBIO1`.  Do **not** send `0xF1` in
  `BBIO1`: there it is a pin-state byte that switches the target supply and
  pull-ups on, not a stop.
- Alternatively send `0x0F` from `BBIO1`: it replies `0x01` and runs the v4
  terminal reset — pins to inputs, target supply and pull-ups off, board state
  reset, SMPS loop stopped — then prints the banner and `HiZ>`.  Returning
  from raw wire to `BBIO1` with `0x00` does **not** stop it, so the refusal
  continues.
- This only applies to an SMPS-enabled build; the default v4 image and every
  v3 build cannot enter this state.

See [the SMPS set](../smps/README.md) and
[binary edge cases][busy] for the other requests refused by the same guard.

## Inside a sub-protocol

`0x14` and `0x15` are BBIO1 commands.  Inside SPI, I2C, UART, 1-Wire or
`RAW1` the `0x1x` range is the bulk transfer group, so the same byte is a
different command: `0x14` starts a five-byte bulk transfer, not an ADC read.
Return to `BBIO1` before reading the probe.  See
[sub-protocols.md](../../protocol/binary/sub-protocols.md).

## Worked exchange

A full binary session with example sample values:

```
host -> 0x00 x20                 enter the binary layer
host <- "BBIO1"
host -> 0x14                     one-shot
host <- 0x02 0x00                example: 512 counts, about 3.29 V
host -> 0x15                     start the stream
host <- 0x02 0x00                example sample
host <- 0x01 0xFF                example sample (511 counts)
host -> 0x00                     stop byte; consumed
host <- 0x02 0x00                example: a buffered sample still arrives
host <- (no sample for timeout)  drain complete
host -> 0x00                     reprint the layer identifier
host <- "BBIO1"
```

The samples after `0x15` are examples only; the number still delivered after
the stop depends on how much the host had left buffered.  The timeout is
chosen by the host; the firmware has no fixed drain time.  The stop itself is
always consumed.

## Related

- [probe.md](probe.md) — the scale, channels and ownership.
- [terminal.md](terminal.md) — the same readings from the prompt.
- [docs/protocol/binary/commands.md](../../protocol/binary/commands.md) — the
  BBIO1 map and the shared error byte.
- [docs/protocol/binary/entering-and-framing.md][framing] — `BBIO1` entry,
  the result bytes and leaving.
- [docs/protocol/binary/edge-cases.md](../../protocol/binary/edge-cases.md) —
  the layer-wide failure modes.
- [../smps/README.md](../smps/README.md) — the SMPS feedback loop.

[busy]: ../../protocol/binary/edge-cases.md#adc-is-busy--the-smps-feedback-owns-the-converter
[framing]: ../../protocol/binary/entering-and-framing.md
