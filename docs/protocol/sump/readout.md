# SUMP — readout framing

Part of the SUMP set (`docs/protocol/sump/`).  This page owns what comes back
after a run: the reply length, the byte layout, the sample order, padding, and
what happens when the host stops reading or aborts.  Depth is in
[depth.md](depth.md); the trigger is in [trigger.md](trigger.md).

## Reply shape

A run sends **exactly the read count in bytes**, one byte per sample.  There is
no header, no length prefix, no checksum and no terminator.  The host already
knows the count because it set it with `0x81`.

```
host -> 81 ff 03 ff 00          read 4096, delay 1024
host -> 01                      run
host <- <4096 bytes>            one byte per sample, newest first
```

The 4096 bytes are the whole reply.  Nothing follows them until the host sends
another command.  If the capture supplied fewer than 4096 samples, the reply is
still 4096 bytes; the difference is padding.

The byte carries the channel bits of the board; the layout is in
[depth.md](depth.md#channel-bits).  The packing used inside the buffer never
reaches the host: the device unpacks 4, 8 or 16 samples per word back into one
byte per sample.

## Sample order

Samples are sent **newest-first**.  The published OLS/SUMP profiles expect that
order ("last sample first"), so a client that keeps its profile default plots
the trace correctly.  Flipping the client's sample-order setting displays the
trace backwards.

The order is the same on both boards and at every depth.

## Recorded, unrecorded and overwritten bits

| Bit in the byte | Value |
|---|---|
| recorded channel, real sample | the pin level at the sample instant |
| unrecorded channel | the alternating `0x55` / `0xAA` pattern, one flip per sample |
| recorded channel, overwritten sample | 0 |

The alternating pattern marks a bit that was not recorded, so a client can tell
it from a signal.  It is a fast square wave, not a quiet channel; do not read it
as data.

In a packed tier the recorded bits are the low bits of the byte.  At 8192 the
low four bits carry data, so a real sample reads `0x5?` or `0xA?` while the
unrecorded high nibble alternates; a padding byte is exactly `0x55` or `0xAA`.
A sample whose four data bits match the pattern has the same byte value, so
treat the pattern as a strong hint, not an absolute marker.

An overwritten sample is indistinguishable from a genuine all-low sample on the
recorded bits: both read 0.  Only packed captures can lose their oldest samples
this way ([depth.md](depth.md#ring-arithmetic)).

## Padding

When the read count is larger than the capture, the reply is padded to the
requested length.  The padding bytes alternate `0x55` / `0xAA` in **every**
bit, including the recorded-channel positions, and they are sent **before** the
recorded samples.

```
reply:  [ padding ][ newest sample ][ ... ][ oldest sample ]
```

So the padding is the first part of the reply, and the recorded samples follow
it, newest first.  A client that plots newest-first shows the padding at the
newest end of the trace.  Request a count the chosen engine can actually supply
to avoid it: 2048 samples at a fast rate on v4, 4096 at 2/4 MHz on v3, or the
tier size at a timer rate.

The tier cliffs are tabulated in [depth.md](depth.md#the-tier-cliff); a count of
8188, for example, captures 4096 samples and pads the other 4092 bytes.

## Capture first, readout second

The device finishes sampling before it sends the first byte.  Readout speed
therefore does not affect the sample timing: the window is already closed when
the transfer starts.

| Board | Readout path | Effect |
|---|---|---|
| v4 | USB CDC bulk transfer | short transfer; blocks when the host stops reading |
| v3 | the terminal UART at its configured baud rate | 4096 bytes take about 0.36 s at 115200 baud (derived from the rate, not verified on v3) |

The MODE LED is on from the run command until the readout finishes, then goes
off.

## Back-pressure

The device does not drop samples when the host is slow.  Its transmit path
blocks when the buffer or the USB endpoint is full, so a host that stops reading
stalls the device instead of losing data.  XON/XOFF is accepted but not
implemented; there is nothing to enable.

The flip side is that a host which never reads holds the board in the readout
until it reads or resets.  A capture that is much larger than expected is
usually a mis-set read count, not a stuck device.

## Aborts and bytes during a reply

| Byte arrives | Behaviour |
|---|---|
| while a timer-paced capture runs | the capture stops at the next burst boundary (up to 2.7 s at the slowest rate), no samples are sent, and the byte is handled as a command |
| while a fast capture waits for its trigger | the wait stops, no samples are sent, and the byte is handled |
| during the short fast post-trigger window | not polled; the window and the readout finish, then the byte is handled |
| during the readout | handled after the last sample; bytes wait in the receiver, and a long burst can overrun the v3 UART |

An aborted capture has **no partial readout**.  The host gets no bytes for that
run; the next bytes it sees belong to the next reply.  A host that aborts with
`#` both stops the capture and leaves SUMP.

The fast post-trigger window is about 1 ms on v4 (2.05 ms for 4096 samples at
2 MHz on v3); the readout that follows takes about 0.36 s for 4096 bytes at
115200 baud on v3.

A single byte sent during the readout is handled after the last sample.  A long
burst can overrun the v3 UART: bytes beyond the receiver are lost, and later
commands can be ignored until the board is power-cycled.  After the readout,
send five `0x00` resets or wait one second before the next command to realign
the parser ([commands.md](commands.md#parser-rules)).  If the board is still
silent, power-cycle it.

## Nothing unsolicited

While SUMP is idle the device sends nothing.  The only bytes it produces are:

| Event | Bytes |
|---|---|
| entry | `1ALS` |
| a later `0x02` | `1ALS` |
| `0x04` | the metadata stream |
| a run | the sample stream |
| an exit | the version banner and `HiZ>` |

There are no status messages, no prompts and no keep-alives.  A host waiting for
a reply that never comes has sent an unimplemented command, lost command-stream
sync, or is still waiting for a trigger.
