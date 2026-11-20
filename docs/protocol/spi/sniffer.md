# SPI — sniffer

Part of the SPI documentation set (`docs/protocol/spi/`). The SPI sniffer
listens to a live bus — somebody else's master — and prints the traffic. It is
macro `(1)` (`Sniff CS low`) and macro `(2)` (`Sniff all traffic`). This page
owns the display format, the CS capture path, integrity, stopping and the
limits. The macros themselves are listed in [macros.md](macros.md); the binary
sniffer framing is in [binary.md](binary.md).

## Usage

1. Disconnect the target from the Bus Pirate.
2. At `SPI>`, run `(1)` to capture only while CS is low, or `(2)` to capture
   all traffic. The macro configures all four SPI pins as inputs before the
   capture starts.
3. Connect the target and let its own master run.
4. Read the stream: each clocked byte prints as `0xNN(0xNN)`, with `[` and
   `]` marking the physical CS level. §Output format explains the fields.
5. Press any key while the external master is quiet. The key stops the
   capture, a newline is printed, and the `SPI>` prompt returns.

```
SPI>(1)
Sniffer
Any key to exit
[0x30(0x00)0xFF(0x12)0xFF(0x50)][0x40(0x00)]

SPI>
```

Entering the sniffer prints `Sniffer` then `Any key to exit`; it runs until a
key is pressed. The stopping key is not echoed; it ends the capture line, and
the blank line and `SPI>` prompt follow. Keystrokes entered **while** the
sniffer runs are not executed as commands — the loop is watching the bus, not
the prompt. The byte values in the example are a format illustration, not a
target's data.

On exit the sniffer runs the master pin/module setup, so CLK, MOSI and CS
become outputs; it does not restore the previous input state. It does not
switch the target supply. Run `W` if the Bus Pirate rails are needed.

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
single-byte transaction with MOSI `0x40` and MISO `0x00`.

The display is hex and uses uppercase digits. It is not affected by the
terminal display format (`o`).

## CS-low vs all traffic

| Macro | Trigger |
|---|---|
| `(1)` | capture only while CS is low (hardware CS filter) |
| `(2)` | capture all clocks, no CS filter |

`(1)` uses the SPI peripheral's hardware slave select, so MISO/MOSI capture is
gated by the physical CS level. `(2)` leaves the interface free-running and
captures every clocked byte regardless of CS.

Because the filter is the **physical low level**, `(1)` does not mean
"CS asserted" on an active-high bus. With the `CS` polarity the low level is
the idle state, so `(1)` gates on the idle phase and does not capture the
transaction; use `(2)` for an active-high bus.

## CS capture path

| | v3 | v4 |
|---|---|---|
| CS markers | polled | change-notification (CN) capture |
| Very short CS pulse | can be missed | captured, subject to the limits below |
| CS pulse with no clocked byte | no marker pair | the edges are reported |

On v4 the CS pin is a change-notification input: an interrupt records the
transitions in a 32-entry marker ring, so a CS pulse between back-to-back
transactions is normally captured even when it is far shorter than a polled
loop pass. CN is change notification, not an unlimited edge FIFO:

- interrupt latency can merge adjacent transitions, so two edges closer
  together than the interrupt response can produce one marker (or none);
- a full 32-entry marker ring aborts the capture (see below);
- the SPI input FIFOs and the terminal output buffer also bound throughput,
  and their overflow aborts the capture;
- the handler records only actual level changes, so an unrelated
  change-notification event does not inject a marker.

On v3 the CS level is polled. The `[` marker is emitted with the first captured
byte after CS goes low, and `]` when the loop next sees CS high; a CS pulse
with no clocked byte produces no marker.

Markers are reported in order, and a byte pair never prints before its opening
marker. At a boundary the closing `]` can be delayed until the next `[` is
pending, so the two markers can print together before the next byte pair; the
byte pairs themselves stay in bus order. An overflow-free capture is not a
guarantee that an arbitrary-rate bus was captured losslessly.

## Sample timing and rate

The sniffer follows the clock **polarity** and **edge** settings of the mode
but **not** the input sample phase; it always samples in the middle. The speed
setting does not set the sniffed rate — the external master does. There is no
maximum bus rate specification; see §Limits.

## Safety and wiring

The sniffer is input-only and does not use the Bus Pirate's target supply:

- Connect **GND** common and all four signals: CS, CLK, MOSI and MISO.
- Power the target from its own system. Do not connect the Bus Pirate's
  3.3 V/5.0 V rails to a live target. The sniffer does not switch the target
  supply on or off; if you ran `W`, the rails stay on.
- The sniffed bus needs whatever pull-ups its normal operation requires. If it
  is open-drain and has none, enable the Bus Pirate pull-ups with `P` and feed
  Vpullup, or fit external 2 kΩ – 10 kΩ resistors. The sniffer itself only
  listens.
- All four pins are inputs in sniffer mode: the external master drives CLK,
  MOSI and CS; the target drives MISO.

Enter the sniffer **before** connecting the target. Until the macro runs, the
mode's master configuration can drive CS, CLK and MOSI; connecting a live
target first risks contention on those lines.

Stop the sniffer only while the external master is quiet. Exit runs the master
pin/module setup, so CLK, MOSI and CS become outputs; stopping on a live bus
can contend with a master that is still driving those lines.

If the terminal is left in a bad state (for example after an interrupted
capture), exit the sniffer with any key, then reset the Bus Pirate with `#`
(or power-cycle it) and re-enter the mode. No capture command repairs a
garbled display.

## Integrity and stopping

Sniffer output is queued in the terminal output buffer and pushed to the host
as the link frees up. The buffer is **4096 bytes on both builds**. A long
enough burst still overtakes the capture path: the CS marker ring, the SPI
input FIFOs and the terminal output buffer share one abort handling.

| Mode | On overflow |
|---|---|
| interactive (`(1)`/`(2)` at the prompt) | prints `Couldn't keep up` and restarts the sniffer without reprinting the two header lines; data around the cut is lost |
| binary (`SPI1` sniffer command) | sends one unescaped `0x00` byte, disables capture, turns the mode LED off and stays passive |

After that `0x00` the binary sniffer waits: **send any byte to leave the
aborted sniffer, and that byte becomes the next command.** Nothing is captured
while it waits, so the master that caused the overflow is not being driven by
the Bus Pirate. The same rule applies to a normal binary stop: the stopping
byte is not consumed and is treated as the next command. For example, send
`0x01` to stop and immediately get `SPI1` back; send `0x00` to stop and exit
to `BBIO1`.

In binary mode, sending `0x00` to stop the sniffer therefore exits to `BBIO1`
(the stopping byte becomes the `0x00` exit command). `0x0F` inside `SPI1` is
**not** a reset — it is an unrecognised byte and is answered `0x00`; leave to
`BBIO1` with `0x00` first if you want the terminal reset. The byte formats are
in [binary.md](binary.md) §Sniffer.

## Binary-mode sniffing

The sniffer is also reachable from the binary `SPI1` sub-protocol with `0x0D`
(all traffic) and `0x0E` (CS low). In that path the output is not the
`0xNN(0xNN)` text but an escaped byte pair — escape byte, MOSI byte, MISO byte
— with bare `[` / `]` CS markers. The format table and a worked stream are in
[binary.md](binary.md) §Sniffer.

## Limits

- No maximum bus rate is specified: the usable rate depends on the interrupt,
  FIFO and link load. Do not assume a 10 MHz bus can be captured; for a fast
  or continuous bus use a logic analyzer.
- The sniffer reports bytes and CS transitions; it does not decode SPI
  transactions beyond that framing (no register names, no device model).
- The CS markers follow the physical level, so on an active-high bus `[` is
  the idle state.
- The input sample phase is ignored; the sniffer always samples in the middle.

## Edge cases

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| The target was connected, then the sniffer was entered | risk of driving/contending on CS, CLK or MOSI before the macro runs | enter the sniffer first, then connect |
| The sniffer was stopped while the external master was clocking | exit configures CLK/MOSI/CS as outputs; contention | stop only while the master is quiet |
| `(1)` was used on an active-high `CS` bus | the transaction is not captured; the gate follows the idle level | use `(2)` |
| The clock polarity or edge is wrong | bytes look shifted or inverted | match the master's polarity/edge; the sample phase is fixed at middle |
| CS markers are missing on short pulses | `[`/`]` can be absent | v3 polls and needs a wider pulse; v4 CN can merge edges |
| A long burst overflows (interactive) | `Couldn't keep up`; the sniffer restarts | lower the bus rate, shorten the burst, or use a logic analyzer |
| A long burst overflows (binary) | one `0x00`, no more capture, the mode LED is off | send a byte to leave; that byte is the next command |
| Commands are typed while sniffing | they are not executed | press a key to exit, then type the command |
| The stopping byte in binary mode is expected to be dropped | it becomes the next command | account for it; `0x01` stays in `SPI1`, `0x00` exits to `BBIO1` |
| `0x01` stops the binary sniffer | `SPI1` is reported | the stopping byte is the next command and stays in `SPI1` |
| `0x0F` is sent to stop the binary sniffer | it is answered `0x00`, not a reset | use `0x00` to exit to `BBIO1`, then `0x0F` |
| The display is garbled | the capture text cannot be trusted | exit with any key, reset with `#` or power-cycle, re-enter |

## v3 / v4

| | v3 | v4 |
|---|---|---|
| macros `(1)` / `(2)` | present | present |
| CS capture | polled; markers around bytes | CN; actual transitions |
| short CS pulse | can be missed | captured, subject to CN limits |
| CS marker ring | none | 32 entries |
| terminal output buffer | 4096 bytes | 4096 bytes |
| text format | same | same |

The v3 column is **not verified**.

## Related docs

- [macros.md](macros.md) — the macro list and the clock tweaks.
- [wiring.md](wiring.md) — sniffer wiring and pull-ups.
- [binary.md](binary.md) — the `0x0D`/`0x0E` byte framing.
- [entry-and-setup.md](entry-and-setup.md) — polarity, edge and sample phase.
- [edge-cases.md](edge-cases.md) — sniffer capture and stop failures.
