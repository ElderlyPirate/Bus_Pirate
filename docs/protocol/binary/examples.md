# Binary layer — worked exchanges

Part of the binary-layer documentation set (`docs/protocol/binary/`).  These
are format illustrations: the command bytes and replies are exact, the read
data values are examples.  Each sub-protocol's authoritative command set is on
its own page (linked from [sub-protocols.md](sub-protocols.md)); entry, replies
and framing are in [entering-and-framing.md](entering-and-framing.md).

## Reading the exchanges

- `host ->` is a byte sent to the Bus Pirate; `host <-` is a byte it returns.
- `"..."` is ASCII text; `0xNN` is one byte; `x20` is this page's shorthand
  for twenty copies, not bytes to send.
- Comments start in the same column in every block.
- A read value is shown as an example; the reply **shape** is the contract.

## Enter, identify, leave

```
host -> 0x00 x20                        enter the binary layer
host <- "BBIO1"                         identifier: 5 bytes, no line ending
host -> 0x00                            re-print, no state change
host <- "BBIO1"
host -> 0x0F                            return to the terminal
host <- 0x01                            acknowledged
host <- "HiZ>"                          terminal prompt
```

The version banner is printed before `HiZ>` and is omitted here.  On v3 the
hardware reset may emit stray bytes first; discard until the banner.  The
`0x00` in BBIO1 only re-prints the identifier — it does not reset pins, power
or the pull-ups.

A host program runs the same sequence in five steps:

1. Open the serial port at the terminal speed.
2. Write twenty `0x00` bytes.
3. Read exactly five bytes and compare them with `BBIO1`.
4. Send requests, reading exactly the reply length each command defines.
5. Send `0x0F`, read the `0x01`, then read text until the banner and the
   `HiZ>` prompt appear.

Step 3 reads the reply; it does not scan the input for the string.  In this
sequence, step 5 is the one reply whose length is not fixed.

## Pin control in BBIO1

Make CLK the only output, drive it high, then drive all latches low.  Every
state or direction byte is answered with the five pin levels.

```
host -> 0x00 x20                        enter the binary layer
host <- "BBIO1"
host -> 0x5B                            direction: CLK output, all others input
host <- 0x00                            example levels: all low
host -> 0x84                            state: CLK high (bit 2)
host <- 0x04                            example levels: CLK high
host -> 0x80                            state: all pin latches low
host <- 0x00                            example levels
```

`0x5B` is `0101 1011`: bit 4 (AUX), bit 3 (MOSI), bit 1 (MISO) and bit 0 (CS)
are set, which means input; only the CLK bit is clear, which means output.  Set
means input throughout the direction byte, so `0x40` would make **all** lines
outputs.  The reply reports the actual levels, not the latches: a floating
input may read high, and a target may hold a line, so the `0x00`/`0x04` values
above are examples only.

## Supply and pull-ups in BBIO1

```
host -> 0xC0                            state: supply on, all pins low
host <- 0x00                            example levels
host -> 0xE0                            state: supply + pull-ups on
host <- 0x00                            example levels
host -> 0x00                            re-print, state is unchanged
host <- "BBIO1"
```

`0xC0` is `1|1|0|00000`: state byte, supply on, pull-up bit clear.
`0xE0` adds the pull-up bit.  The pull-up bit switches the resistor network
only; in BBIO1 the on-board Vpullup source is off and cannot be selected, so a
voltage must be present on the Vpullup pin from outside.  The state-byte reply
shows the pin levels only — not the supply or pull-up state.

## Wait for an edge

Configure the watched pin as an input first; the wait does not change its
direction.  This frame waits for a rising edge with a 100 ms timeout.

```
host -> 0x5F                            direction: all five pins input
host <- 0x00                            example levels
host -> 0x70 0x01 0x00 0x64             rising, timeout 100 ms
host <- 0x01                            result: the edge arrived
host <- 0x01                            level at the end: high
```

A timeout of `0` checks the pin once and returns immediately, which doubles as
a level sample:

```
host -> 0x70 0x03 0x00 0x00             either edge, no wait
host <- 0x00                            result: no edge
host <- 0x01                            level: high
```

An invalid edge selector still consumes its timeout.  Send the complete frame:

```
host -> 0x70 0x00 0x00 0x64             selector 0x00 is invalid
host <- 0x00                            failure, after the timeout field
```

A host that stops after `0x70 0x00` leaves the layer waiting for the two
timeout bytes.  An invalid sub-command is different: `0x71` is refused
immediately and reads no payload.  See [edge-cases.md](edge-cases.md).

## ADC one-shot

```
host -> 0x14                            one probe conversion
host <- 0x02 0x00                       example: 512 counts, about 3.3 V
```

The reply is two bytes, high byte first, with no acknowledgement and no error
byte.  The value is a raw 10-bit count; the ADC set owns the scale.

## ADC continuous stream

```
host -> 0x15                            start streaming
host <- 0x02 0x00                       sample (example)
host <- 0x01 0xFF                       sample (example)
host -> 0x00                            stop byte; it is consumed
host <- 0x02 0x00                       buffered samples may still arrive
```

There is no acknowledgement, count, length or end marker.  Keep reading until
no sample arrives within your timeout, then send the next command.

## PWM set and clear

Prescaler `0x02` = 1:64 (250 kHz timer tick), duty `0x0063` = 99, period
`0x00C7` = 199: about 1250 Hz at about 50 % on the `AUX` pin.

```
host -> 0x12 0x02 0x00 0x63 0x00 0xC7   set up PWM
host <- 0x01                            accepted
host -> 0x13                            clear PWM
host <- 0x01                            accepted
```

The output always uses the plain `AUX` pin, not the selected AUX target.  Send
`0x13` before leaving if you want the timer idle.

## Self-test, short

```
host -> 0x10                            short self-test
host <- 0x00                            error count: 0 = no errors
host -> 0x41                            any byte while the test idles
host <- 0x41                            0x41 + 0 errors, echoed back
host -> 0xFF                            leave the self-test
host <- 0x01                            confirmation
host -> 0x0F                            return to the terminal
host <- 0x01                            acknowledged
host <- "HiZ>"                          terminal prompt
```

Leaving the self-test does **not** re-print `BBIO1`; the host must remember
that it is back in the bitbang layer.  The full test (`0x11`) has the same
frame and needs the jumpers described in
[commands.md](commands.md#self-test-0x10-0x11) §Self-test.

## I2C byte write in `I2C1`

The shared `0x1x` group is easiest to see in I2C, where each per-byte reply is
the slave's ACK bit — the opposite sense to the layer's own result byte.  Write
two bytes to a device that acknowledges both:

```
host -> 0x00 x20                        enter the binary layer
host <- "BBIO1"                         identifier: 5 bytes, no line ending
host -> 0x02                            enter I2C
host <- "I2C1"                          mode identifier
host -> 0x02                            start condition
host <- 0x01                            start accepted
host -> 0x11 0xAA 0xBB                  bulk write: n = 1, so two bytes
host <- 0x01                            command accepted
host <- 0x00                            per-byte ACK, not a failure
host <- 0x00                            per-byte ACK
host -> 0x03                            stop condition
host <- 0x01                            stop accepted
host -> 0x00                            leave I2C
host <- "BBIO1"                         back in bitbang
host -> 0x0F                            return to the terminal
host <- 0x01                            acknowledged
host <- "HiZ>"                          terminal prompt
```

The three replies to `0x11` are in order: the command's `0x01`, then one ACK
bit per byte.  `0x00` is ACK and `0x01` is NACK, so both bytes were
acknowledged.  The address and payload bytes are examples; the reply shape is
the contract.  A NACK is a normal reply, not a bus fault; I2C `0x0A` reports
the bus status.  Other modes use the same leading `0x01` with their own
per-byte reply — see
[sub-protocols.md](sub-protocols.md#0x1x--bulk-byte-transfer).

## Mode skeleton

This is a template, not a literal exchange.  The five nibble-group modes
(`0x01`–`0x05`) follow it; `0x06` and `0x07` also reset, print an identifier
and leave with `0x00`, but their command layouts are their own.  The canonical
byte-by-byte SPI frame is in
[the SPI binary page](../spi/binary.md) §Worked frames.

```
host -> 0x00 x20                        enter the binary layer
host <- "BBIO1"
host -> 0x01                            entry byte: 0x01-0x07
host <- "SPI1"                          mode identifier
host -> (shared configure bytes)        0x4x, 0x5x, 0x6x, 0x8x as needed
host <- 0x01                            one reply per configure command
host -> (mode transfer bytes)           see the mode's binary page
host <- (per-mode replies)
host -> 0x00                            leave the mode
host <- "BBIO1"
host -> 0x0F                            return to the terminal
host <- 0x01                            acknowledged
host <- "HiZ>"                          terminal prompt
```

Exceptions and variations:

- `0x18` (XSVF) prints `XSV1` but ignores `0x00`; leave it with `0x0F`, which
  returns to BBIO1 silently, then send `0x0F` again for the terminal.
- `0x08` / `0x09` (AVRISP-MKII) print no identifier and have no command loop;
  they end by idle timeout and then print `BBIO1`.
- `0x8x` is per mode: `0x88` is normal output in SPI but open drain in `RAW1`.
  Use the selected mode's page.

## Related docs

- [commands.md](commands.md) — the BBIO1 command map.
- [sub-protocols.md](sub-protocols.md) — the entries and shared groups.
- [edge-cases.md](edge-cases.md) — what to do when a step misbehaves.
- [the SPI binary page](../spi/binary.md) §Worked frames — the
  canonical SPI frame.
- [the shared `RAW1` page](../wire/binary.md) §Worked exchange — the
  canonical raw-wire frame.
