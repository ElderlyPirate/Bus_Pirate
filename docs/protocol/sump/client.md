# SUMP — host programs and capture workflow

Part of the SUMP set (`docs/protocol/sump/`).  This page covers the host side:
which profile to use, how a capture is set up, which client settings reach the
device, and how long a capture takes.  The byte-level entry is in
[entry.md](entry.md); the commands are in [commands.md](commands.md).

## Scope

This page describes what a SUMP/OLS client must do to drive the Bus Pirate.  It
does not cover the client's own display or file formats, and it does not repeat
the command encodings.

## Identification and profile

The `0x02` that enters SUMP is answered with the four bytes `1ALS`; each later
`0x02` request returns it again.  The published SUMP protocol names the
signature `SLA0` or `SLA1`; `1ALS` is `SLA1` in reverse byte order.  The OLS
and sigrok clients do not accept the literal signature, so the firmware sends
the reversed form.

Use the client's SUMP / OLS-compatible profile (recent sigrok/PulseView
releases label it **Openbench Logic Sniffer & SUMP compatibles**; the exact
label is **unverified**).  The device presents itself as an OLS-class SUMP
device, so that profile is the match.

The metadata response identifies the board and its limits, and carries the
ancillary version string `enh`, the enhanced-profile marker:

| Metadata | v4 | v3 |
|---|---|---|
| device name | `BPv4` | `BPv3` |
| probe slots | 8 (7 wired) | 5 |
| sample memory | 4096 bytes | 4096 bytes |
| maximum rate | 16 000 000 Hz | 16 000 000 Hz |
| protocol version | 2 | 2 |
| ancillary version string | `enh` | `enh` |

The full key table is in [commands.md](commands.md#metadata-response).

## Serial settings

The analyzer uses the same serial link as the terminal:

| Board | Link | Settings |
|---|---|---|
| v4 | USB CDC | the port the terminal uses; no baud choice on the USB side |
| v3 | UART | the terminal's baud rate (default 115200), 8 data bits, no parity, one stop bit |

On v3 the default is 115200 bps.  If `b` was used to change the terminal rate,
set the client to the same rate; a mismatch gives no `1ALS` or a garbled one.
See [generic navigation.md](../../generic/navigation.md#terminal-baud--b).

Only one program can own the port.  Close the terminal before opening the
client, and close the client before opening a terminal.  The client sends the
entry sequence itself; do not pre-enter SUMP from a terminal.  A board that is
already in SUMP still accepts the client's five resets and ID request, because
those bytes are valid SUMP commands too, but the port must be free.

## Capture workflow

1. Close any terminal on the port.
2. Open the client and select its SUMP / OLS profile and the Bus Pirate port.
   The client sends five NUL bytes and `0x02`; the device answers `1ALS`.
3. Choose a sample rate.  At or below about 1 MHz the rate is timer-paced and
   quantised; above that one of four fixed engines runs.  A request above the
   advertised 16 MHz is not an error; it selects the 16 MHz engine.  The slow
   end is the exception: a request slower than about 6 Hz wraps the divider, so
   clamp it before computing the request.  The rate table is in
   [rates.md](rates.md).
4. Choose the number of samples.  This is the read count and it also selects
   the depth tier and therefore the recorded channels.  See
   [depth.md](depth.md).
5. Optionally set a trigger channel mask and a trigger position.  The mask is
   always used; the position is used only at timer rates.  See
   [trigger.md](trigger.md).
6. Send `0x01` (run).  The device samples, then streams exactly the requested
   number of bytes.
7. Read exactly that many bytes.  Samples arrive newest-first.
8. Leave SUMP with `0x0F`, `#` or `$`, or leave the client running and set up
   the next capture.

Step 8 matters because **the device resets its configuration after every
capture**.  The rate, read count, trigger mask and flags must be sent again
before the next run; a client that caches the setup on the host side still has
to transmit it.

## Sample order

Samples are sent **newest-first** ("last sample first").  The OLS/SUMP profiles
expect that order, so leave the client's sample-order or "reverse order"
setting at its profile default.  If it is flipped, the trace is displayed
backwards.

The order also applies to padding: when the requested count is larger than the
capture, the padding bytes are sent before the recorded samples.  See
[readout.md](readout.md#padding).

## What each client setting does

| Client setting | Effect on the Bus Pirate |
|---|---|
| sample rate | sets the divider; timer rates are quantised, fast rates are fixed engines |
| number of samples | sets the read count, the depth tier and the reply length |
| trigger channel mask | selects the pins watched for an edge |
| trigger position | number of samples after the trigger, at timer rates only |
| trigger level (high/low) | no effect: the trigger is always an edge |
| trigger values / stages | no effect |
| channel enable/disable | no effect: the depth tier decides which channels are recorded |
| RLE compression | no effect |
| external clock | no effect |
| demux / filter | no effect |
| XON/XOFF flow control | no effect; the device applies back-pressure by stalling |

Channel enable/disable is the most common mismatch: a client can believe it
disabled CS, while the capture still records it because the read count kept
every channel.  The reverse also happens at a capture tier of 8192 or deeper,
where **high** channel bits are dropped to gain depth: on v4 at 8192, bits 0–3
(AUX2, MOSI, CLK, MISO) are kept and CS (bit 4) is dropped.  The recorded set
is a property of the count, not of the client's channel checkboxes
([depth.md](depth.md#depth-tiers)).

## Capture time and host timeouts

The capture runs first; the readout starts when it ends.  The device does not
send a byte until sampling is complete, so a client timeout must cover the
capture, any trigger wait, and the transfer.

| Capture | Rate | Time before the first byte |
|---|---|---|
| 4096 samples | 1 kHz | 4.1 s |
| 4096 samples | 10 kHz | 0.41 s |
| 4096 samples | 1 MHz | 4.1 ms |
| 32768 samples | 10 kHz | 3.3 s |
| 32768 samples | 1 MHz | 32.8 ms |
| 2048 samples (fast) | 16 MHz | 128 µs |

A trigger that never arrives adds an unbounded wait before the capture even
starts.  Use a client timeout that is longer than the expected capture, and be
ready to abort.

After the capture, the transfer itself takes time on v3: 4096 bytes at
115200 baud occupy the UART for about 0.36 s (10 bits per byte), and 32768
bytes for about 2.8 s.  On v4 the transfer is over USB and is much shorter.

## Leaving and recovering

Close the client, then send `0x0F`, `#` or `$` from a terminal to return to
`HiZ`.  A client that closes the port without an exit leaves the board in SUMP;
the next terminal session must send an exit byte before a prompt appears.  The
detail is in [entry.md](entry.md#leaving).

## Options that do nothing

The SUMP flags byte, the trigger values, the staged trigger commands, RLE,
demux, filter, channel groups, the external clock and the inverted clock are
all accepted or ignored.  They are listed in
[commands.md](commands.md#ignored-commands) with their exact effect: none.
