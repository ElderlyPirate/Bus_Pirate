# SPI — sniffer

Part of the SPI documentation set (`docs/spi/`). The SPI sniffer listens to a
live bus — somebody else's master — and prints the traffic. It is macro
`(1)` (sniff while CS is low) and macro `(2)` (sniff all traffic).

## Usage

```
SPI>(1)
Sniffer
Any key to exit
[0x30(0x00)0xff(0x12)0xff(0x50)][0x40(0x00)]
SPI>
```

Entering the sniffer prints `Sniffer` then `Any key to exit`; it runs until a
key is pressed. The stopping key is consumed, so the next thing you type is a
normal command. Keystrokes entered **while** the sniffer runs are not executed
as commands — the loop is watching the bus, not the prompt.

## Output format

| Element | Shown as |
|---|---|
| CS asserted (bus transaction starts) | `[` |
| CS released (bus transaction ends) | `]` |
| byte on MOSI | `0xNN` in hex |
| byte on MISO | `(0xNN)` in hex, in parentheses |

So `[0x30(0x00)0xff(0x12)0xff(0x50)][0x40(0x00)]` reads as: in the first
transaction, MOSI bytes `0x30 0xff 0xff` with the MISO byte after each
(`0x00 0x12 0x50`), bracketed by the CS transitions; then a second
single-byte transaction with MOSI `0x40` and MISO `0x00`. (This is the
firmware's format; the example line is a format illustration.)

The sniffer follows the clock **polarity** and **edge** settings of the mode
but **not** the input sample phase.

## CS-low vs all traffic

| Macro | Trigger |
|---|---|
| `(1)` | capture only while CS is low (hardware CS filter) |
| `(2)` | capture all clocks, no CS filter |

`(1)` uses the SPI peripheral's hardware slave-select (`SSEN`), so MISO/MOSI
capture is gated by CS. `(2)` leaves the interface free-running and captures
every clocked byte regardless of CS.

## Hardware CS capture (v4)

The CS transition markers are captured differently on the two boards:

- **v4** — a change-notification interrupt latches every CS edge in hardware,
  so even a very short CS pulse between back-to-back transactions produces its
  `[` / `]` markers. The limiting factor is the SPI peripheral, not polling.
- **v3** — the CS edge is **polled**, so a CS pulse must be wide enough to be
  seen between loop passes. Very short pulses can be missed.

## Safety

Enter the sniffer **before** connecting it to the target. In sniffer mode all
four pins are inputs, but the act of entering/changing modes can momentarily
drive a line; connecting a live target first risks grounding the target's
clock or data. If the display looks like garbage, reset with the CS pin.

## Buffer and overflow

Sniffer output is queued in the terminal ring buffer and pushed to the host as
the link frees up, so a slow display does not by itself drop bytes:

| Build | Buffer |
|---|---|
| v4 | 4096 bytes |
| v3 | 2048 bytes |

A long enough burst still overtakes the buffer. The two interfaces react
differently on overflow:

- **interactive** (`(1)`/`(2)` at the prompt) — prints `Couldn't keep up` and
  restarts the sniffer.
- **binary** (the `SPI1` sniffer command) — stops sniffing and turns the mode
  LED off; the firmware returns to the binary command loop.

(The older published text quoted `Can't keep up`; the current firmware string
is `Couldn't keep up`.)

## Binary-mode sniffing

The sniffer is also reachable from the binary `SPI1` sub-protocol. In that path
the output is not the `0xNN(0xNN)` text but an escaped byte pair — escape
byte, MOSI byte, MISO byte — with `[` / `]` CS markers; the format table and a
worked stream are in [binary.md](binary.md) §Sniffer.

## Limits

- The sniffer's usable bus rate depends on the interrupt and link load, not on
  a fixed number. The older published "up to 10 MHz" claim is **unverified** —
  treat the maximum rate as unknown. It is not a substitute for a logic
  analyzer.
- The sniffer reports bytes and CS transitions; it does not decode SPI
  transactions beyond that framing (no register names, no device model).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| macros `(1)` / `(2)` | present | present |
| CS capture | polled (needs a wider pulse) | hardware-latched |
| ring buffer | 2048 bytes | 4096 bytes |
| text format | same | same |

v3 is **not verified**.