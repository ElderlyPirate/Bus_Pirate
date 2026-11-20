# SUMP — worked captures

Part of the SUMP set (`docs/protocol/sump/`).  These are script-level
transcripts: the command bytes and reply lengths are exact, the sample values
are not shown.  A SUMP client sends the same bytes.  Entry and exit detail is in
[entry.md](entry.md); the command encodings are in [commands.md](commands.md).

## Reading the exchanges

- `host ->` is a byte sent to the Bus Pirate; `host <-` is a byte it returns.
- `"..."` is ASCII text; `0xNN` is one byte; `x20` is the shorthand for twenty
  copies, not bytes to send.
- `<...>` is a variable-length or build-dependent reply, named rather than
  transcribed; the version banner is the common case.
- Comments start in the same column in every block.
- A reply shown as `<N bytes>` is one byte per sample, newest-first.  The
  values depend on the probed signals.

## 1 MHz timer capture with a CS trigger

A 4096-sample capture at 1 MHz with the trigger on CS and 1024 samples after
the edge.  All seven v4 channels are recorded because the count is 4096.

The metadata reply is one continuous stream; the lines group one key each.

```
host -> 00 00 00 00 00 02      five NULs + raw STX: enter SUMP
host <- "1ALS"                 identification, sent on entry
host -> 02                     request ID
host <- "1ALS"
host -> 04                     request metadata
host <- 01 42 50 76 34 00      key 0x01, "BPv4", string end
host <- 21 00 00 10 00         key 0x21, sample memory = 4096
host <- 23 00 f4 24 00         key 0x23, maximum rate = 16000000
host <- 03 65 6e 68 00         key 0x03, "enh", string end
host <- 40 08                  key 0x40, 8 probe slots
host <- 41 02                  key 0x41, protocol version 2
host <- 00                     end of metadata
host -> 80 63 00 00 00         divider 99: 1 MHz timer
host -> 81 ff 03 ff 00         read 4096, delay 1024 (post-trigger)
host -> c0 10 00 00 00         trigger mask: CS (bit 4)
host -> 01                     run
host <- <4096 sample bytes>    newest-first; CS is bit 4 of each byte
host -> 00                     reset, stay in SUMP
host -> 0f                     leave SUMP
host <- <version banner>
host <- "HiZ>"
```

The window is the 1024 samples after the edge plus the 3072 before it.  The
capture takes 4.1 ms at 1 MHz and the readout follows.  The `0x00` reset is
optional before an exit, but it is the clean way to clear the trigger.  This
block is for v4: `c0 10` selects CS (bit 4).  On v3 the same byte selects AUX
(bit 4); use `c0 01` for CS there.

## 16 MHz fast capture, 512 samples

A short capture on CLK, in a new session: the port was released at the end of
the previous example, so the entry sequence is sent again.  The divider and
counts follow because entry reset them.

```
host -> 00 00 00 00 00 02      five NULs + raw STX: enter SUMP
host <- "1ALS"
host -> 80 05 00 00 00         divider 5: 16 MHz engine
host -> 81 7f 00 7f 00         read 512, delay 512
host -> c0 04 00 00 00         trigger mask: CLK (bit 2)
host -> 01                     run
host <- <512 sample bytes>     one byte per sample, all channels recorded
```

The delay of 512 is ignored at this rate: the trigger marks the start of the
window, and the window is 512 samples at 16 MHz, or 32 µs.  Every wired channel
is recorded because fast captures ignore the depth tier.

## 8192-sample packed capture at 100 kHz

A deeper timer capture.  The 8192 tier keeps the low four channel bits; on v4
those are AUX2, MOSI, CLK and MISO, and CS (bit 4) is dropped.

```
host -> 80 e7 03 00 00         divider 999: 100 kHz timer
host -> 81 ff 07 ff 07         read 8192, delay 8192
host -> c0 01 00 00 00         trigger mask: AUX2 (bit 0), a kept channel
host -> 01                     run
host <- <8192 sample bytes>    one byte per sample; four channels carry data
```

The delay of 8192 puts the trigger at the start.  Send a smaller delay to keep
pre-trigger samples: at 100 kHz, 8192 samples take 82 ms.  The trigger arms
after the first 4 ms burst, so an edge inside that burst is lost
([trigger.md](trigger.md#how-the-window-is-filled)).  This block is for v4:
`c0 01` selects AUX2.  On v3 the same byte selects CS (bit 0), which the 8192
tier keeps; change the mask deliberately there.

## A fast request that comes back padded

Asking for more samples than the fast buffer holds is legal, but the reply is
mostly padding:

```
host -> 80 05 00 00 00         divider 5: 16 MHz engine
host -> 81 ff 03 ff 00         read 4096, delay 1024
host -> 01                     run
host <- <2048 padding bytes>   0x55, 0xAA, 0x55, ... (the first bytes)
host <- <2048 sample bytes>    the actual 16 MHz capture
```

On v4 the fast buffer holds 2048 samples at every fast rate, so half of this
reply is padding.  The padding comes first, which a newest-first client shows at
the newest end of the trace.  Ask for 2048 samples or fewer at 16 MHz to avoid
it.  On v3 the 8/16 MHz paths are capped at 2048 as well; only its 2/4 MHz fast
paths hold 4096 samples.

## Leaving from a terminal

The entry cannot be typed reliably, but the exit can.  Close the client first.

```
(no prompt)                    SUMP is running
#                              typed by hand: the byte leaves SUMP
<version banner>
HiZ>                           the terminal is back
                               the Enter after # gives a blank line and a
                               second prompt
HiZ>
```

The second prompt is normal.  The `#` is consumed by SUMP, so the carriage
return that follows reaches the terminal and runs an empty command.  A script
that sends exactly one exit byte and no line ending sees only one prompt.
