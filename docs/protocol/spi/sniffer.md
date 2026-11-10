# SPI — sniffer

Part of the SPI documentation set (`docs/protocol/spi/`). The SPI sniffer
listens to a live bus — somebody else's master — and prints the traffic. It
is macro `(1)` (sniff while CS is low) and macro `(2)` (sniff all traffic).

## Usage

```
SPI>(1)
Sniffer
Any key to exit
[0x30(0x00)0xFF(0x12)0xFF(0x50)][0x40(0x00)]
SPI>
```

Entering the sniffer prints `Sniffer` then `Any key to exit`; it runs until a
key is pressed. The stopping key is consumed, so the next thing you type is a
normal command. Keystrokes entered **while** the sniffer runs are not executed
as commands — the loop is watching the bus, not the prompt.

## Output format

| Element | Shown as |
|---|---|
| CS low | `[` |
| CS high | `]` |
| byte on MOSI | `0xNN` in hex |
| byte on MISO | `(0xNN)` in hex, in parentheses |

The markers follow the **physical CS level**, not the mode's CS-polarity
setting: `[` is always CS low and `]` always CS high. With the default `/CS`
polarity that is asserted/released; with the `CS` (active-high) polarity `[`
marks a deasserted CS instead.

So `[0x30(0x00)0xFF(0x12)0xFF(0x50)][0x40(0x00)]` reads as: in the first
transaction, MOSI bytes `0x30 0xFF 0xFF` with the MISO byte after each
(`0x00 0x12 0x50`), bracketed by the CS transitions; then a second
single-byte transaction with MOSI `0x40` and MISO `0x00`. (This is the
firmware's format; the example line is a format illustration.)

The sniffer follows the clock **polarity** and **edge** settings of the mode
but **not** the input sample phase; it always samples in the middle.

## CS-low vs all traffic

| Macro | Trigger |
|---|---|
| `(1)` | capture only while CS is low (hardware CS filter) |
| `(2)` | capture all clocks, no CS filter |

`(1)` uses the SPI peripheral's hardware slave-select, so MISO/MOSI capture is
gated by CS. `(2)` leaves the interface free-running and captures every clocked
byte regardless of CS.

## CS capture path

| | v3 | v4 |
|---|---|---|
| CS markers | polled | change-notification (CN) capture |
| very short CS pulse | can be missed | captured, subject to the limits below |

On v4 the CS pin is a change-notification input: an interrupt records the
transitions in a 32-entry marker ring, so a CS pulse between back-to-back
transactions is normally captured even when it is far shorter than a polled
loop pass. CN is change notification, not an unlimited edge FIFO:

- interrupt latency can merge adjacent transitions, so two edges closer
  together than the interrupt response can produce one marker (or none);
- a full 32-entry marker ring aborts the capture (see below);
- the SPI input FIFOs and the output buffer also bound throughput, and their
  overflow aborts the capture.

An overflow-free capture is not a guarantee that an arbitrary-rate bus was
captured losslessly.

## Safety

Enter the sniffer **before** connecting it to the target. In sniffer mode all
four pins are inputs, but the act of entering/changing modes can momentarily
drive a line; connecting a live target first risks grounding the target's
clock or data.

Stop the sniffer only while the external master is quiet. Normal exit restores
the master pin configuration (CLK, MOSI and CS become outputs), so stopping on
a live bus can contend with a master that is still driving those lines.

If the display looks like garbage, no capture command repairs it: exit the
sniffer with any key and reset the Bus Pirate with `#` (or power-cycle it), then
re-enter the mode.

## Integrity and stopping

Sniffer output is queued in the terminal ring buffer and pushed to the host as
the link frees up, so a slow display does not by itself drop bytes:

| Build | Buffer |
|---|---|
| v4 | 4096 bytes |
| v3 | 2048 bytes |

A long enough burst still overtakes the capture path. The CS marker ring, the
SPI input FIFOs and the output buffer share one abort handling, and the two
interfaces react differently:

| Mode | On overflow |
|---|---|
| interactive (`(1)`/`(2)` at the prompt) | prints `Couldn't keep up` and restarts the sniffer; data around the cut is lost |
| binary (`SPI1` sniffer command) | sends one unescaped `0x00` byte, disables capture, turns the mode LED off and stays passive |

After that `0x00` the binary sniffer waits: **send any byte to leave the aborted
sniffer, and that byte becomes the next command.** Nothing is captured while it
waits, so the master that caused the overflow is not being driven by the Bus
Pirate. The same rule applies to a normal binary stop: the stopping byte is not
consumed and is treated as the next command. The byte formats are in
[binary.md](binary.md) §Sniffer.

## Binary-mode sniffing

The sniffer is also reachable from the binary `SPI1` sub-protocol. In that path
the output is not the `0xNN(0xNN)` text but an escaped byte pair — escape
byte, MOSI byte, MISO byte — with `[` / `]` CS markers; the format table and a
worked stream are in [binary.md](binary.md) §Sniffer.

## Limits

- No maximum bus rate is specified: the usable rate depends on the interrupt,
  FIFO and link load. Do not assume a 10 MHz bus can be captured; for a fast
  or continuous bus use a logic analyzer.
- The sniffer reports bytes and CS transitions; it does not decode SPI
  transactions beyond that framing (no register names, no device model).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| macros `(1)` / `(2)` | present | present |
| CS capture | polled (needs a wider pulse) | CN (edge merging, 32-entry ring) |
| ring buffer | 2048 bytes | 4096 bytes |
| text format | same | same |

v3 is **not verified**.
