# Worked exchanges — bytes on the link

Part of the OpenOCD adapter set.  This page shows the request/reply sequences
in byte form: what a stock OpenOCD session sends, and what a hand-driven
session sends for each command.  The rules are in
[commands.md](commands.md); the TAP walk that the shift bytes belong to is
in [protocol/jtag/tap.md](../../protocol/jtag/tap.md).

## Scope

- Entry, configuration, shift, ADC, speed and delay as bytes.
- The framing of a clean exit.
- **Not covered:** the host program's output and the failure tables; see
  [workflow.md](workflow.md) and [limits-and-errors.md](limits-and-errors.md).

Quoted TDO values are illustrative unless stated otherwise; they depend on
the chain and the TAP state.  The request bytes, the echo and the byte
counts are fixed by the protocol.

## A stock session's opening bytes

With `buspirate mode normal`, `buspirate vreg 0` and `buspirate pullup 0`,
OpenOCD sends this after it opens the port:

```
host -> 0x00 x20               enter the binary layer
host <- "BBIO1"
host -> 0x06                   select the OpenOCD path
host <- "OCD1"

  with buspirate speed fast only:
host -> 0x07 0x01              select fast
host -> 0xAA 0x55              confirmation; on v3, at 1 Mbaud
host <- 0x07 0x01              echo: fast

host -> 0x01 0x01              pin mode: JTAG push-pull
host -> 0x02 0x02 0x00         VREG off
host -> 0x02 0x10 0x00         on-board pull-ups off
host -> 0x02 0x04 0x01         TRST high: released
host -> 0x02 0x08 0x01         SRST high: released
```

The pin mode and the four feature requests have **no reply**.  The driver
then starts the target script's chain probe, which uses `0x05` shift
blocks.

## Configuring a hand-driven session

The pin mode asserts both resets.  Release them explicitly:

```
host -> 0x01 0x01              pin mode: JTAG push-pull
                               (SRST and TRST are now driven low)
host -> 0x02 0x04 0x01         TRST high: released
host -> 0x02 0x08 0x01         SRST high: released
host -> 0x02 0x02 0x00         VREG off: target on its own supply
host -> 0x02 0x10 0x00         on-board pull-ups off
                               no reply to any of these
```

For an open-drain session, use `0x01 0x02` and fit the pull-ups.  For a
board-powered target, use `0x02 0x02 0x01` and wire the target's VCC to one
rail only.

## An 8-bit shift

One group: two payload bytes, one TDO byte.

```
host -> 0x05 0x00 0x08         TAP shift, 8 bits
host <- 0x05 0x00 0x08         3-byte echo, requested count repeated
host -> 0x00 0x00              TDI low byte, TMS low byte
host <- 0x5A                   one TDO byte (illustrative)
```

The payload may be written before the echo is read; on the wire the order is
header, payload, and the echo is the first thing in the reply.

## A 16-bit shift

One 16-bit group uses four payload bytes and returns two TDO bytes.

```
host -> 0x05 0x00 0x10         TAP shift, 16 bits
host <- 0x05 0x00 0x10         3-byte echo
host -> 0xFF 0xFF 0x00 0x00    TDI low, TMS low, TDI high, TMS high
host <- 0x00 0x00              two TDO bytes, low byte first (illustrative)
```

The TMS bytes above hold TMS high for the low eight clocks and low for the
next eight; that is a valid fragment of a reset walk, not a complete scan.
For a full 43-bit single-device IDCODE walk, including the TMS table and the
group split, see [protocol/jtag/tap.md](../../protocol/jtag/tap.md).

## A 17-bit shift

The count need not be a multiple of 16.  The final group is short:

| Group | Count | Payload bytes | TDO bytes |
|---|---|---|---|
| 1 | 16 | 4 | 2 |
| 2 | 1 | 2 | 1 |

```
host -> 0x05 0x00 0x11         TAP shift, 17 bits
host <- 0x05 0x00 0x11         3-byte echo
host -> 0x00 0x00 0x00 0x00    16-bit group: TDI low, TMS low,
                               TDI high, TMS high
host -> 0x00 0x00              1-bit group: TDI low, TMS low
host <- 0x00 0x00 0x00         three TDO bytes, in group order
                               (illustrative)
```

## An ADC read

```
host -> 0x03
host <- 0x03 0x08 0x00 0x00 0x00 0x00 0x02 0x01 0x03 0x08
```

| Bytes | Reading | Counts | Volts |
|---|---|---|---|
| `0x03 0x08` | command and data length | — | — |
| `0x00 0x00` | ADC/probe pin | 0 | 0.00 |
| `0x00 0x00` | Vpullup | 0 | 0.00 |
| `0x02 0x01` | `3.3V` rail | 513 | 3.30 |
| `0x03 0x08` | `5.0V` rail | 776 | 5.00 |

The example shows VREG on and the board powering a 3.3 V target from the
`3.3V` rail; the `5.0V` rail is up but not connected to the target, and
Vpullup has no source.  The values depend on the wiring.

## A speed change

```
host -> 0x07 0x01              select fast
                               switch the host port to the new rate
host -> 0xAA 0x55              confirmation in a separate write
host <- 0x07 0x01              echo: fast
```

The reply is an echo, not a status.  A wrong confirmation:

```
host -> 0x07 0x01              select fast
                               switch the host port to the new rate
host -> 0x00 0x00              wrong confirmation, sent at the new rate
host <- 0x07 0x00              firmware reverted to normal
```

On v3 the firmware changes its UART before reading the confirmation, and the
failed reply is sent at normal.  A host that switched to fast must switch
back before it can read the reply.  Do not send the selector and the
confirmation in one write; the confirmation must follow the host's rate
switch.  On v4 the rate does not change either way, so the split has no
effect.

## Setting the TCK delay

```
host -> 0x08 0x00 0x00         delay 0: fastest loop
host -> 0x08 0x00 0x14         delay 20: slower TCK
                               no reply to either request
```

The delay is a loop count, not microseconds.  A host that waits for a reply
consumes the next command's reply instead.

## Wrong bytes

`0x04` is not the TAP shift in this path, and `0x0F` is not an exit:

```
host -> 0x04                   unrecognised; still in OCD1
host <- 0x00

host -> 0x0F                   unrecognised; still in OCD1
host <- 0x00
```

The TAP-shift command is `0x05`, and the exit is `0x00` to `BBIO1`, then
`0x0F` from there.

## A clamped shift

```
host -> 0x05 0x20 0x01         request 8193 bits
host <- 0x05 0x20 0x01         echo repeats 8193
                               the firmware shifts 8192 bits and
                               returns TDO for 8192 bits
                               the surplus payload is read as commands
```

The surplus is not discarded: it becomes the next request bytes, so the
session desynchronises.  The TDO reply is also sized for the clamped count.
Split the scan before sending it.

## Leaving the path

```
host -> 0x00                   leave OCD1
host <- "BBIO1"
host -> 0x0F                   leave the binary layer
host <- 0x01                   return-to-terminal acknowledgement
host <- "HiZ>"                 version banner omitted
```

`0x01` is a raw byte, not the text "1".  On v4 the terminal reset prints the
version banner before `HiZ>`; it is omitted above so the exchange stays
aligned.  On v3 the final `0x0F` performs a hardware reset.

## Related

| Doc | Why |
|---|---|
| [commands.md](commands.md) | the rules behind these exchanges |
| [entry-and-exit.md](entry-and-exit.md) | the entry state and the exit effects |
| [workflow.md](workflow.md) | the same session at the OpenOCD level |
| [../../protocol/jtag/tap.md](../../protocol/jtag/tap.md) | the full IDCODE walk |
| [limits-and-errors.md](limits-and-errors.md) | the clamp, zero counts and recovery |
