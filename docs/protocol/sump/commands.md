# SUMP — command reference

Part of the SUMP set (`docs/protocol/sump/`).  This page owns the byte-level
command set, the two command lengths, the parameter encodings, the metadata
reply, the parser recovery rules and the lifetime of the configuration.  Rates
are in [rates.md](rates.md), depth in [depth.md](depth.md), triggers in
[trigger.md](trigger.md).

## Framing

A short command is **one byte**.  A long command is the opcode plus **four
parameter bytes**, five bytes in total.  The opcode's top bit selects the form:

| Opcode | Form |
|---|---|
| `0xxxxxxx` | one byte |
| `1xxxxxxx` | five bytes: opcode, then four parameters with no gap |

Most commands have no reply.  The protocol replies are the identification (4
bytes), the metadata stream (variable length) and the capture itself (the read
count in bytes); an exit additionally prints the terminal banner and the
`HiZ>` prompt, which are terminal output rather than a SUMP reply.  There are no
line endings, length prefixes or checksums in the protocol replies.

## Command set

| Byte | Length | Name | Action |
|---|---|---|---|
| `0x00` | 1 | Reset | reset the analyzer and stay in SUMP |
| `0x01` | 1 | Run | arm and run one capture |
| `0x02` | 1 | ID | reply `1ALS` |
| `0x04` | 1 | Metadata | reply with the key stream |
| `0x0F` | 1 | Exit | return to the terminal in `HiZ` |
| `0x11` | 1 | XON | accepted; no effect |
| `0x13` | 1 | XOFF | accepted; no effect |
| `0x23` (`#`) | 1 | Exit | return to the terminal in `HiZ` |
| `0x24` (`$`) | 1 | Exit | return to the terminal in `HiZ`; no bootloader |
| `0x80` | 5 | Set divider | set the sample rate |
| `0x81` | 5 | Set read/delay count | set the depth and the trigger position |
| `0x82` | 5 | Set flags | accepted; no effect |
| `0xC0` | 5 | Set trigger mask | select the pins watched for an edge |
| `0xC1` | 5 | Set trigger values | accepted; no effect |
| any other `0x00`–`0x7F` | 1 | — | ignored; framing kept |
| any other `0x80`–`0xFF` | 5 | — | four parameters consumed, then ignored |

`0x01` turns the MODE LED on and runs exactly one capture.  When the capture
and its readout finish, the analyzer resets and the LED turns off.  There is no
continuous mode.

`0x00` resets the analyzer but does not leave SUMP.  It also discards a trigger
mask, a read count and a divider set earlier, so it is the "clean slate"
command, not an exit.

## Parameter encodings

Parameter bytes are numbered 1–4 after the opcode.

| Command | Parameter 1 | Parameter 2 | Parameter 3 | Parameter 4 |
|---|---|---|---|---|
| `0x80` divider | divider bit 0–7 | divider bit 8–15 | divider bit 16–23 | unused |
| `0x81` read/delay | read count low | read count high | delay count low | delay count high |
| `0x82` flags | flags | unused | unused | unused |
| `0xC0` trigger mask | mask | unused | unused | unused |
| `0xC1` trigger values | ignored | ignored | ignored | ignored |

The divider is a 24-bit little-endian value, so the largest divider is
16 777 215.  It maps to the actual rate as described in [rates.md](rates.md).

The read and delay counts are each a 16-bit little-endian value encoding
`(raw + 1) × 4` samples.  Both are therefore always multiples of four, with a
minimum of 4.  There is no way to express 8191, 4095 or any other count that is
not a multiple of four.  The largest expressible read count is 262 144 samples;
the largest capture is 32 768 samples (see [depth.md](depth.md)).

The delay count is the number of samples **after** the trigger.  It is clamped
to the capture size, so a delay larger than the window means "trigger at the
start".  See [trigger.md](trigger.md#trigger-position).

The trigger mask uses one bit per channel.  Bits with no pin behind them are
ignored, and a mask with no wired bit disables the trigger.  See
[trigger.md](trigger.md#mask-bits).

## Metadata response

`0x04` returns a series of one-byte keys, each followed by that key's data.
The series ends with key `0x00`.  Strings are NUL-terminated, 32-bit values are
four big-endian bytes, and short values are one byte.

The keys are sent in this order:

| Key | Type | Value |
|---|---|---|
| `0x01` | string | `BPv4` or `BPv3` |
| `0x21` | 32-bit | sample memory in bytes, 4096 |
| `0x23` | 32-bit | maximum sample rate, 16 000 000 |
| `0x03` | string | ancillary version string `enh` (the enhanced-profile marker) |
| `0x40` | 8-bit | probe slots, 8 on v4 (7 wired) or 5 on v3 |
| `0x41` | 8-bit | protocol version, 2 |
| `0x00` | — | end of metadata |

The complete v4 reply is 26 bytes:

```
01 42 50 76 34 00     0x01 "BPv4" 0x00
21 00 00 10 00        0x21 = 4096
23 00 f4 24 00        0x23 = 16000000
03 65 6e 68 00        0x03 "enh" 0x00
40 08                 0x40 = 8 probe slots
41 02                 0x41 = protocol 2
00                    end
```

On v3 the device name is `BPv3` and the probe count is 5; the rest is the
same.

The published metadata defines keys this device does not send: `0x02` (FPGA
version), `0x20` (32-bit probe count), `0x22` (dynamic memory) and `0x24`
(32-bit protocol version).  A client that needs the probe count uses `0x40`;
the short protocol version is `0x41`.

`enh` is the ancillary version string the enhanced profiles match against.  It
does not select a different mode: the enhanced rates, depths and trigger
position described in this set are always available.  There is **no separate
extended entry or mode**.

## Parser rules

| Situation | Behaviour |
|---|---|
| receiver state unknown | send `0x00` up to five times; four flush a pending long command, the fifth is a plain reset |
| a long command stalls part-way | after a one-second gap the partial command is abandoned and the next byte is read as a new command |
| an unknown short command | ignored; the next byte is still a command |
| an unknown long command | four parameter bytes are consumed, then it is ignored |
| an exit byte mid-command | consumed as a parameter; the exit does not happen, and the command completes and takes effect (unless its opcode is unknown) |
| a byte after a long opcode | always a parameter, whatever its value |
| v3 UART overrun | the loop checks and clears the overrun before polling for a command, including on the first pass; a partial command and the capture configuration are discarded, and the next complete command is read normally.  The host should resend the command; no power cycle is needed |

The one-second gap is what heals a lost sync.  Real clients send the five-byte
commands back to back and never hit it; a script that dies mid-command does, and
the next connection can recover without a power cycle.

There is no length, checksum or marker in the command stream.  A lost or extra
byte while idle shifts the next command, and a long command consumes four
following bytes; the reset run or the one-second gap is the way back.

## Configuration lifetime

The analyzer resets after entry, after `0x00`, and after **every capture**.
Nothing about the rate, depth, trigger or flags survives a capture.

| Event | Configuration |
|---|---|
| entry | defaults |
| `0x00` | defaults, stays in SUMP |
| end of a capture | defaults |
| exit | not applicable; the analyzer is left |

| Setting | Default after a reset |
|---|---|
| divider | not set; the default timer period is 1601 ticks, about 9.994 kHz |
| read count | 4096 samples |
| delay count | 4096 samples (trigger at the start) |
| trigger mask | 0 (no trigger) |
| storage format | one sample per byte, every channel |

A host that sends `0x01` without configuring anything gets a 4096-sample
capture at about 9.994 kHz, with no trigger and every channel recorded.

There is **no read-back** of the current configuration.  `0x04` reports static
limits, not the divider, counts or mask; a host must track what it sent.

## Ignored commands

| Byte | Feature | Effect |
|---|---|---|
| `0x11` / `0x13` | XON / XOFF | none; transfer uses back-pressure instead |
| `0x82` | flags: demux, filter, channel groups, external clock, inverted clock | none |
| `0xC1` | trigger values (levels) | none; the trigger is an edge on the mask |
| `0xC2` `0xC6` `0xCA` `0xCE` | staged trigger configuration | consumed as unknown long commands, ignored |
| `0x9E` `0x9F` | extended SUMP commands | consumed as unknown long commands, ignored |
| — | RLE compression | not implemented |
| — | continuous sampling | not implemented |

Channel groups are part of the ignored flags.  A client that disables a group
does not change the capture; the read count selects the recorded channels
instead ([depth.md](depth.md#depth-tiers)).
