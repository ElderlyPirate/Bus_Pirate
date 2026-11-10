# SUMP logic analyzer — capture

Part of the SUMP documentation set (`docs/protocol/sump/`). This page covers
the whole capture protocol: how the analyzer is entered and left, what it
records, the sample rates and their precision, capture depth and the
channel trade-off, triggers, the command reference, a worked exchange, and
the enhanced engines that unlock faster rates and larger captures. There is
no separate "extended mode": the enhancement is part of the same protocol
and command set.

The shared prompt grammar and the terminal reset/bootloader commands live in
[the generic layer](../../generic/README.md) and are not repeated here.

## Scope

This page covers the SUMP protocol as the Bus Pirate implements it, driven
from an external host. It does not cover the host program's own user
interface.

## Entry and exit

SUMP is **not on the mode menu**. It is entered when the serial port receives
a hidden byte sequence. The firmware counts the NUL bytes as they arrive;
once five or more have been seen, the next raw STX byte enters SUMP. The
sequence is:

1. **At least five `0x00` (NUL) bytes**, but **fewer than twenty**.
2. The **raw `0x02` (STX) byte**.

Only the raw `0x02` enters. A keyboard's left-arrow sends the three-byte
sequence `ESC [ D` (`0x1B 0x5B 0x44`); that does **not** enter SUMP. The
firmware checks for a non-NUL/non-STX byte before it decodes escape
sequences, so the `ESC` resets the NUL count first and the sequence just
moves the cursor.

The NUL count survives further NUL bytes, but **any other byte resets it** —
Enter, a typed command, `ESC`, or anything else. Send the NULs immediately
before the raw `0x02`. With fewer than five NULs, the `0x02` also just moves
the cursor (or beeps at the start of the line).

At twenty NULs the firmware enters the binary bitbang layer (`BBIO1`)
instead of SUMP, so the entry window is five to nineteen NULs.

Typing the sequence at the terminal is **not** the entry path; it is always
sent by a program — a SUMP client as it opens the port, or a script driving
the port by hand (the raw form is five `0x00` then `0x02`).

On entry the firmware:

- switches every analyzer pin to an input and turns the mode LED off;
- resets the analyzer state;
- immediately sends its four-byte identification, `1ALS`.

There is no visible prompt while SUMP is running.

### Leaving

SUMP itself has no exit command in its protocol, so the firmware also accepts
three terminal/binary reset bytes at the command level:

| Byte | Meaning |
|---|---|
| `0x0F` | the binary "return to terminal" byte |
| `#` | the terminal safe-reset command |
| `$` | the terminal bootloader command |

Any of the three stops the analyzer and returns the Bus Pirate to the
terminal in HiZ mode, then prints the version banner and prompt. Without one
of them the board stays in SUMP until it is power-cycled. `$` is treated as a
plain reset here: it returns to the terminal and does **not** additionally
enter the bootloader.

## Channels and pin mapping

The analyzer records the Bus Pirate signal pins. Each sample byte sent to the
host maps them onto bits of the result.

v4 (advertises eight probe slots; seven are wired):

| Bit | Signal | Notes |
|---|---|---|
| 0 | AUX2 | |
| 1 | MOSI | |
| 2 | CLK | |
| 3 | MISO | |
| 4 | CS | |
| 5 | AUX | |
| 6 | — | no pin behind it; always the alternating pattern |
| 7 | AUX1 | soft-wired in firmware |

v3 (five channels):

| Bit | Signal |
|---|---|
| 0 | CS |
| 1 | MISO |
| 2 | CLK |
| 3 | MOSI |
| 4 | AUX |
| 5–7 | —; always the alternating pattern |

The generic layer names the AUX target `A0` and prints the pin as `AUX`;
v4 adds `AUX1` and `AUX2`. This set uses the pin-name form (`AUX`, `AUX1`,
`AUX2`) throughout (see
[generic commands.md](../../generic/commands.md#aux-targets-and-pin-names)).
The v4 firmware advertises eight probes while only seven signals are wired —
bit 6 carries no pin and is always filled with the alternating `0x55` /
`0xAA` pattern, so clients show a regular alternating pattern that marks the
bit as unrecorded, not a signal.

Wiring: connect the probe pins to the signals to observe and share a common
ground with the target. The v4 header labels are the signal names —
`GND 5.0V 3.3V VPU ADC AUX2 AUX1 AUX CS MISO CLK MOSI` (pin 1 is `MOSI`,
pin 12 `GND`; see [generic usage.md](../../generic/usage.md#v-in-hiz)).
**No pull-up and no supply are required** — the analyzer pins are inputs and
the on-board pull-ups are switched off on entry. Keep every input within the
Bus Pirate's input range (0–5.5 V).

## Client programs

The Bus Pirate identifies itself as a SUMP / Openbench Logic Sniffer class
device so an existing SUMP client can drive it:

- **sigrok / PulseView** — select the client's SUMP / OLS-compatible driver
  (recent releases label it *Openbench Logic Sniffer & SUMP compatibles*;
  the exact label is unverified), then the Bus Pirate's serial port.
- Other SUMP clients may also work; the connection settings (port, speed)
  are the same as the terminal's.

The device presents the published SUMP signature in reversed byte order: the
protocol's `SLA0` / `SLA1` signature is emitted as `1ALS` (and the `enh`
ancillary marker) because the Openbench and sigrok clients reject the device
under the literal signature. Use the client's SUMP / Openbench Logic Sniffer
profile.

Samples stream back **newest-first**, which is what the Openbench / SUMP
client profiles expect ("last sample first"). Leave the client's
sample-order/reverse setting at its SUMP-profile default; if it is changed,
the captured trace is displayed backwards.

The client owns the serial port and sends the hidden entry sequence itself
(the firmware side of that contract is fixed: ≥5× `0x00` bytes + raw `0x02`).
Only the client-side driver name/profile label varies between releases and
is **unverified**.

## Sample rates

The host sets the rate with the divider command. The Bus Pirate's core runs
at 16 MIPs, so it maps the host's requested nominal rate onto one of five
engines:

| Nominal rate | Engine | Notes |
|---|---|---|
| ≤ 1 MHz | timer-paced | exact tick pacing |
| 2 MHz | cycle-counted polling loop | 8 cycles per sample; exact |
| 4 MHz | cycle-counted polling loop | 4 cycles per sample; one cycle of per-pass jitter, exact long-term |
| 8 MHz | cycle-counted polling loop | 2 cycles per sample; one cycle of per-pass jitter, exact long-term |
| 16 MHz | cycle-counted polling loop | 1 cycle per sample; exact; the top rate |

The maximum advertised rate is **16 000 000 Hz**. The published SUMP divider
formula (`100 MHz ÷ (x + 1)`) is the nominal label the host asks for; the
actual pace follows from the 16 MHz core: 1 MHz and 16 MHz are exact, 2 MHz
is exact per sample, and 4 MHz and 8 MHz have one cycle of per-pass jitter
(exact long-term). The 16 MHz rate leaves no headroom on the 16 MIPs core, so
the timer-paced rates are the reliable ones.

The 4 MHz and 8 MHz loops unroll four samples per pass; 2 MHz and 16 MHz use
their own loop shapes, so do not assume the same grouping there.  All fast
loops end with a decrement-and-branch (the 16 MHz loop uses a repeat).

At the timer-paced rates the sampler works in short bursts of a few
milliseconds' worth of samples (at least 16); the per-burst bookkeeping is
amortised, so the window stays effectively tick-exact.

## Capture depth and channels

The host asks for a sample count with the read/delay command. The Bus Pirate
then picks a storage format to fit the request:

| Requested samples | Captured | Channels kept | Packing |
|---|---|---|---|
| below 4096 | as requested | all | 1 sample per byte |
| 4096 to 8191 | 4096 | all | 1 sample per byte |
| 8192 to 16383 | 8192 | lowest 4 | 4 samples per 16-bit word |
| 16384 to 32767 | 16384 | lowest 2 | 8 samples per 16-bit word |
| 32768 or more | 32768 | lowest 1 | 16 samples per 16-bit word |

"Lowest" means the low channel bits in the tables above. The effect per
version:

| Captured | v4 channels kept | v3 channels kept |
|---|---|---|
| 4096 | AUX2, MOSI, CLK, MISO, CS, AUX, AUX1 | CS, MISO, CLK, MOSI, AUX |
| 8192 | AUX2, MOSI, CLK, MISO | CS, MISO, CLK, MOSI |
| 16384 | AUX2, MOSI | CS, MISO |
| 32768 | AUX2 | CS |

Channel bits that are not recorded are filled with an alternating `0x55` /
`0xAA` pattern so the host can tell them from real data. The host is always
sent the number of sample bytes it asked for; short captures are padded the
same way.

At the fast rates the capture always records every channel and is capped by
the fast buffer (2048 samples on v4, one 16-bit storage word each); the depth
tiers above apply to the timer-paced rates.

The v4 sample buffer is 4096 bytes; the v3 buffer is 2048 bytes (half). On
v3, any capture above 2048 samples — the 4096-sample tier, the 8k/16k/32k
tiers and the fast paths — exceeds the v3 buffer and is unverified. The v3
fast-capture capacity is therefore **2048 bytes**, matching the buffer;
treat larger v3 fast requests as unavailable.

## Trigger

- The trigger-mask command selects **which channels** to watch. A trigger is
  an **edge** on any selected channel, not a level match. The mask is
  independent of the recorded-channel set, so a trigger on a channel the
  depth drops still fires; bit 6 has no pin and is ignored.
- At the **timer-paced rates** (≤ 1 MHz) the trigger can sit anywhere in the
  window: the sampler keeps a rolling pre-trigger ring and stops once the
  requested number of post-trigger samples is captured. The delay count in
  the read/delay command sets how many samples fall **after** the trigger;
  the rest of the window is before it.
- At the **fast rates (2 MHz and above)** the trigger always marks the
  **start** of the captured window and the delay count is ignored.
- Trigger **values** and per-stage trigger configuration are **not
  implemented** — only the channel mask is acted on. A client's high/low
  level trigger becomes "any edge on the selected channels".

## Command reference

A short command is one byte. A long command is **five bytes**: the opcode
plus four parameter bytes. The parameter semantics:

- divider: a 24-bit value in parameter bytes 1–3; parameter byte 4 is unused.
- read/delay: read count in bytes 1–2, delay count in bytes 3–4; each is
  little-endian and encodes `(raw + 1) × 4` samples, so counts are multiples
  of four.
- trigger mask: only parameter byte 1 is used.
- flags / trigger values: parsed but ignored.

| Byte | Command | Length | Behaviour |
|---|---|---|---|
| `0x00` | Reset | 1 | resets the analyzer; stays in SUMP |
| `0x01` | Run | 1 | arms the sampler (mode LED on) |
| `0x02` | ID | 1 | replies with `1ALS` |
| `0x04` | Metadata | 1 | replies with the key/value stream below |
| `0x11` | XON | 1 | accepted; flow control not implemented |
| `0x13` | XOFF | 1 | accepted; flow control not implemented |
| `0x80` | Set divider | 5 | sets the sample rate |
| `0x81` | Set read/delay count | 5 | sets capture depth and trigger position |
| `0x82` | Set flags | 5 | parsed but not applied |
| `0xC0` | Set trigger mask | 5 | selects the watched channels |
| `0xC1` | Set trigger values | 5 | not implemented |

When the receiver's state is unknown, send the reset byte up to five times to
flush any partially received long command. A long command that stalls is
abandoned after a one-second gap, so a lost sync heals.

The SUMP flags (demux, filter, channel groups, external clock, inverted
clock) are all received and ignored; RLE compression is not implemented.

### Metadata response

`0x04` returns a series of one-byte keys, each followed by that key's data;
the series ends with key `0x00`. Strings are NUL-terminated, 32-bit integers
are four big-endian bytes, and short integers are one byte.

| Key | Type | Value |
|---|---|---|
| `0x01` | string | device name, `BPv4` or `BPv3` |
| `0x03` | string | ancillary version, `enh` |
| `0x21` | 32-bit | sample memory in bytes: 4096 (v4) or 2048 (v3) |
| `0x23` | 32-bit | maximum sample rate, 16 000 000 |
| `0x40` | 8-bit | probe slots advertised: 8 (v4, seven wired) or 5 (v3) |
| `0x41` | 8-bit | protocol version, 2 |
| `0x00` | — | end of metadata |

Keys not sent: FPGA version (`0x02`), the 32-bit probe-count key (`0x20`),
dynamic memory (`0x22`) and the 32-bit protocol version (`0x24`). The `enh`
marker is what lets enhanced client profiles match the device.

## Worked exchange

```
host -> 00 00 00 00 00 02      hidden entry: five NULs + raw STX
host <- "1ALS"                 identification, sent on entry
host -> 02                     request ID
host <- "1ALS"
host -> 04                     request metadata
host <- <keys/values, ending 00>
host -> 80 63 00 00 00         set divider  (1 MHz)
host -> 81 ff 03 ff 00         read/delay   (4096 samples, post-trigger)
host -> c0 10 00 00 00         trigger mask (watch CS)
host -> 01                     run
host <- <samples, newest-first>
host -> 00                     reset, stay in SUMP
host -> 0f                     leave for the terminal
host <- <version banner and prompt>
```

The read count `ff 03` encodes 4096 samples and `ff 00` encodes 1024
post-trigger samples. A 4096-sample capture keeps every channel, so the CS
trigger channel is recorded. At 8192 samples and above CS is dropped from the
capture; a trigger mask on it would then fire on a channel that is not in
the result.

The host is sent exactly the number of sample bytes it asked for in the
read/delay command.

## Readout

Readout is a plain byte stream in the same newest-first order on both
boards; the bulk transfer path is internal to the device and adds no
host-side framing or flow control. A host that stops reading applies
back-pressure (the device waits for buffer space) rather than losing
samples.

## Using it

A capture is driven by the SUMP client; the terminal is only needed to leave
SUMP or to recover the board.

1. Open sigrok / PulseView, choose its SUMP / OLS-compatible driver (recent
   releases label it **Openbench Logic Sniffer & SUMP compatibles**; see
   §Client programs), and select the Bus Pirate's serial port. The client
   owns the port for the session: it opens the port and sends the hidden
   entry sequence itself, so the firmware leaves the terminal, enters SUMP
   and replies `1ALS`. You do **not** pre-enter SUMP from a terminal — a COM
   port cannot be open in two programs at once, and the hidden sequence is
   not meant to be typed at the prompt (see §Entry and exit).
2. Choose a sample rate and a sample count (the count drives the depth and
   channel negotiation above), set any channel trigger mask, and capture.
3. Leave SUMP with `0x0F`, `#` or `$`; the board returns to the terminal in
   HiZ.

The firmware holds SUMP until it receives one of those exit bytes or is
power-cycled; it does not time out. Whether a host resets the board when it
opens the port is **unverified**.

## v3 / v4

| Aspect | v3 | v4 |
|---|---|---|
| build | present (build-only) | present (default image) |
| probes | 5 | 8 advertised / 7 wired |
| sample memory | 2048 bytes | 4096 bytes |
| channel order (low bits) | CS, MISO, CLK, MOSI, AUX | AUX2, MOSI, CLK, MISO, CS, AUX, —, AUX1 |
| capture tiers above 2048 samples | unverified | present |
| fast captures | 2048-byte buffer, unverified | up to 2048 samples |
| entry, exit, command set | same | same |

The v3 build is not verified; the entry/exit bytes and command set are the
same in both builds, but the v3 depth/speed tiers and fast engines are
unverified.
