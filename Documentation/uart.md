UART Mode
===================
UART is also known as the common PC serial port. A PC serial port operates at full RS232 voltage levels (-13volts to +13volts), which are not compatible with the Bus Pirate.

  - **Bus:** [UART](https://en.wikipedia.org/wiki/Universal_asynchronous_receiver/transmitter), [MIDI](http://en.wikipedia.org/wiki/Musical_Instrument_Digital_Interface), half-duplex single-wire UART
  - **Connections:** two pins (RX/TX) and ground; one data pin (TX) and ground in half-duplex single-wire mode
  - **Output types:** 
    - 3.3volt normal output (high=3.3volts, low=ground)
    - [open drain/open collector](http://en.wikipedia.org/wiki/High_impedence) (high=Hi-Z, low=ground), [pull-up resistors](http://dangerousprototypes.com/docs/Practical_guide_to_Bus_Pirate_pull-up_resistors) required. Use open drain/open collector output types with pull-up resistors for multi-voltage interfacing.
  - **Pull-up resistors:** required for open collector output mode (2K – 10K)
  - **Maximum voltage:** 5.5volts (5volt safe)
  - **Speed:** (BPS) 300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 31250 (v2.7+), BRG (v5.1+)
  - **Data bits and parity:** 8/none, 8/even, 8/odd, 9/none
  - **Stop bits:** 1,2
  - **Receive polarity:** idle 1, idle 0
  - **Duplex:** full (separate RX/TX pins) or half (single wire, open drain, pull-up required)


Bus Requirements
---------------------

### Pull-up resistors

In open-collector mode, it requires pull-up resistors to hold the RX and TX lines high.

In half-duplex single-wire mode the two data pins share one wire and the bus is
open drain; a **pull-up resistor (1–10 kΩ) to 3.3 V is required**, and both ends
must be single-wire/open-drain capable.


Starting UART Mode
---------------------

    HiZ>m    <<<open the mode menu
    1 . HiZ
    2 . …
     3 . UART
    …
    (1)>3    <<<choose UART mode
    Set serial port speed: (bps)
     1. 300
     2. 1200
     3. 2400
     4. 4800
     5. 9600
     6. 19200
     7. 38400
     8. 57600
     9. 115200
    10. Custom BAUD (BPv4) / raw BRG (BPv3)
    11. Auto-Baud Detection (BPv4 only, activity required)
    
    (1)>5    <<<choose UART speed
    Data bits and parity:
     1. 8, NONE *default
     2. 8, EVEN
     3. 8, ODD
     4. 9, NONE
    (1)>     <<<choose data bits per frame (default accepted)
    Stop bits:
     1. 1 *default
     2. 2
    (1)>     <<<choose frame stop bits (default accepted)
    Receive polarity:
     1. Idle 1 *default
     2. Idle 0
    (1)>     <<<choose polarity (default accepted)
    Select output type:
     1. Open drain (H=Hi-Z, L=GND)
     2. Normal (H=3.3V, L=GND)

    (1)>2    <<<choose output type
    Duplex:
     1. Full (TX+RX) *default
     2. Half (single wire)

    (1)>     <<<choose duplex (default accepted = full duplex)
    Ready
    UART>


### Half-duplex single-wire mode (HDUART)

Select `2. Half (single wire)` at the duplex prompt (setup option 6; optional
in inline parameter lists — omitting it selects full duplex).

  - **Wiring:** one data wire (the TX pin, MOSI on BPv4) plus ground. The bus is
    open drain and **requires a pull-up resistor (1–10 kΩ) to 3.3 V**; both ends
    must be open-drain/single-wire capable (another Bus Pirate, or a target
    running its USART with wired-AND on its TX pin).
  - **No self-echo:** bytes the Bus Pirate transmits are not displayed back;
    received bytes are read normally (`r`, `{`/`[`, macros).
  - Selecting half duplex forces the stored output type to open drain, even if
    Normal was selected earlier. The settings header and VPU controls use that
    effective output type. Half duplex requires idle-high polarity.
  - On BPv4 enable the supplies (`W`) and the pull-ups (`P`) after mode entry,
    as with the regular UART mode.
  - All commands, macros and the transparent bridge work unchanged. The binary
    bitbang UART protocol stays full duplex.

    UART>2      <<<half duplex selected; peer sends 0x55 0xAA
    READ: 0x55
    READ: 0xAA
    UART>0x12   <<<transmit 0x12 on the shared wire
    WRITE: 0x12


Command Syntax
-------------------

  * A/a/@
	  * Toggle auxiliary pin.
	  * Capital “A” sets AUX high.
	  * Small “a” sets to ground.
	  * @ sets aux to input (high impedance mode) and reads the pin value.
  * D/d (v1+ hardware only)
	  * Measure voltage on the ADC pin.
  * W/w (v1+ hardware only)
	  * Capital ‘W' enables the on-board power supplies.
	  * Small ‘w' disables them.
  * [
      * Start asynchronous formatted UART display (same as `{`).
  * {
	  * Start asynchronous formatted UART display.
  * ] or }
	  * Stop live display; RX remains buffered while UART is engaged.
  * r
	  * Check UART for byte, or fail if empty.
      * Displays framing (-f) and parity (-p) errors ([more](#r-read-a-byte)). (r:1…255 for bulk reads)
  * 0b
	  * Write this binary value.
	  * Format is 0b00000000 for a byte, but partial bytes are also fine: 0b1001.
  * 0x
	  * Write this HEX value.
	  * Format is 0x01. Partial bytes are fine: 0xA. A-F can be lower-case or capital letters.
  * 0-255
	  * Write this decimal value, check ACK.
	  * Any number not preceded by 0x or 0b is interpreted as a decimal value.
  * , space
	  * Value delimiter.
	  * Use a comma or space to separate numbers.
	  * Any combination is fine, no delimiter is required between non-number values: {0xa6,0, 0 16 5 0b111 0xaF}.
  * &
	  * Delay 1uS. (&:1…255 for multiple delays)
  * (#) 
	  * Run macro, (0) for macro list 


### r read a byte

UART mode requires special handling compared to the other Bus Pirate modes because data can arrive at any time. UART mode displays framing and parity errors, and automatically clears buffer overruns.

###  Parity and framing errors

    READ: -p -f 0×40 <<<-p -f flag set

The Bus Pirate reports framing errors (-f) and parity errors (-p) when reading a byte from the UART. It's unlikely you'll see these errors unless the UART speed is mismatched with the sender.

### Buffer overrun errors

The RX interrupt drains the four-word hardware FIFO into a software ring with
127 usable words. Each word retains its parity/framing flags and ninth data bit.
When the ring fills, newly received words are dropped; hardware overruns are
cleared after draining the FIFO. `uart2_rx_overrun_count()` counts software drops
and hardware-overrun events since the last UART enable, not necessarily the
number of lost characters. Ordinary `r` output does not print a dropped-byte
warning. Transparent bridge macro (1) extinguishes the MODE LED on overrun;
transparent streams do not inject error bytes.

Binary UART services a snapshot of queued RX between commands, including
between successive bulk writes. It retains the command ACK followed by one ACK
per transmitted byte without inserting RX bytes into that ACK group. RX echo
may appear between command responses. With echo disabled, received words are
consumed without forwarding, as requested by that mode.

Raw bridge/monitor output and ART1 are byte-oriented: they forward only the low
eight data bits and do not encode parity/framing flags. Use formatted terminal
reads or live display when ninth-bit values and per-word error flags matter.

Buffering is finite, not a lossless-stream guarantee: the host must drain the
response stream and the sender must use pacing or suitable flow control when
the downstream link cannot keep up. At 1 Mbps 8N1, 127 words cover about 1.27 ms
of stalled foreground service. Raw bridge/monitor output avoids formatted-text
expansion but cannot absorb an indefinitely stalled host. Binary clients that
cannot drain concurrently should use acknowledged chunks and collect their RX
responses before sending more.

On BPv4, raw bridge/monitor RX forwarding checks CDC IN readiness before
removing a word from the ring. A busy host therefore does not trap those loops
inside a blocking CDC write: NORMAL can leave the bridge/monitor, and any host
key can leave the raw monitor. The exit newline/prompt may itself wait until
the host resumes reading. Binary bridge `0x0F` returns to ART1 on NORMAL; its
ordinary RX echo forwarding is also readiness-gated. The configured echo-off
mode still consumes RX without forwarding it.

This is not flow control. A sender that continues through a host stall can fill
the 127-word ring and lose new words. Bridge/monitor overruns extinguish the
MODE LED; no error markers are inserted into the transparent byte stream.
Command ACKs, formatted terminal output and incomplete binary commands retain
their normal blocking I/O contracts: the host must drain responses and supply
the promised command operands. The readiness change is not a general escape
from every terminal or binary parser wait, nor a USB-disconnection guarantee.

### Custom baud rate

BPv4's custom menu accepts a nominal baud rate from 62 to 999999 bps. Custom
setup and menu autobaud choose the representable frequency closest to that
rate, using `baud = FCY / (4 * (BRG + 1))` with `FCY = 16 MHz`. Thus custom
9600 selects BRG 416 (about 9592.33 bps) and custom 115200 selects BRG 34
(about 114285.71 bps), matching the presets. Exact-error ties prefer the slower
frequency. Inline custom baud operands are 16-bit; use the prompted menu for
larger nominal rates.

BPv3's custom option still accepts a raw BRG value, not nominal baud. Use a
[PIC UART calculator](http://www.micromagicsystems.com/#/pic-baud/4523812801)
to find the correct value. Binary UART `0x07` also takes a raw 16-bit BRG on
both boards; it does not apply nominal-baud rounding.

![Baud Rate Calculator](images/BP-uart-custom-baud.png)

Set the calculator with the Bus Pirate values: PIC24, 32MHz clock. Enter the desired baud rate and hit calculate. Use the value from the BRGH=1 section. For 9700bps enter 411 at the Bus Pirate BRG prompt.

Macros
------------------

| (#) | Description |
|:---:| ----------- |
| 0 | Macro menu |
| 1 | Transparent UART bridge. BPv4 NORMAL button exits; BPv3 requires reset. |
| 2 | Live raw UART monitor. Any host key exits; BPv4 NORMAL also exits. |
| 3 | BPv3 bridge with CTS/RTS forwarding; ordinary bridge on BPv4. |
| 4 | Estimate/report baud without changing the selected rate or engagement state. |
  
### Transparent UART bridge

    UART>(1)    <<<macro 1, transparent UART bridge
    UART bridge
    Normal to exit
    Are you sure?  <<<y or Y continues; other keys cancel.

The transparent UART mode **macro (1)** forwards raw bytes in both directions.
On BPv4, press NORMAL to return to the terminal. BPv3 requires a hardware reset.
The BPv4 example's "Normal to exit" banner means the physical NORMAL button.
Host input is UART payload in this bridge, not an escape key.

Buffer overrun errors that occur during bridge mode are automatically cleared so that data continues as normal (firmware v3.0+). The MODE LED will turn off to alert you of the buffer overrun error.

**Macro (3)** forwards CTS/RTS only on BPv3. The CS input is mirrored to FTDI
CTS; CLOCK mirrors FTDI RTS to the target. BPv4 has no equivalent FTDI
handshake path: macro (3) behaves like macro (1), without hardware flow control.

On BPv3, UART1 facing the FTDI USB converter is not adjusted to match UART2's
target rate. It remains at its configured terminal baud (115200 by default).
Set the host and the BPv3 `b` menu consistently before entering a bridge.

BPv4 uses native USB CDC: virtual-port line-coding metadata does not program
UART2. Select the target baud in UART setup; changing only the host's virtual
baud does not change the wire rate.
    
### Live UART monitor

    UART>(2)     <<<macro 2, UART monitor
    Raw UART input
    Any key to exit

The UART monitor **macro (2)** shows a live display of UART input as raw byte values without any type of formatting. Press any key to exit the live monitor. This mode works best with a terminal that can display raw byte values in a variety of formats.

This macro is like the transparent UART **macro (1)** but without transmission abilities, and it can be exited with a key press. It's useful for monitoring high-speed UART input that causes buffer overrun errors in other modes.

### Baud detection

Macro (4) reports an estimate and the closest listed common rate, including MIDI
31250. It temporarily disables UART2, samples activity on the selected RX pin
(MOSI in half duplex, MISO in full duplex), and preserves whether UART2 was
engaged. Any host key cancels; absent activity times out after about five
seconds. It does not apply the reported rate. BPv4's menu autobaud instead runs
once at the next `W` and applies the selected common rate, with a 115200 fallback
when no rate is recognized.

Use a suitable repetitive training pattern such as `0x55`, then stop and allow
an idle interval before sending application data. Detection is not lossless RX:
training sent while UART2 is disabled is unavailable, and reopening within a
continuous character stream can produce a shifted trailing word. Account for
that training separately and verify the first fresh, idle-separated packet.

The software estimator samples 25 idle-level pulses, finds the shortest group
of at least three intervals within one GPIO polling quantum (12 instruction
clock ticks; 13 for BPv3 single-wire), and averages that group. This accounts for
polling quantization
and rejects isolated short timestamps without masking USB interrupts. Training
must contain at least three one-bit idle-level pulses; a repetitive `0x55`
burst supplies them. A single unusually short measurement is not sufficient
evidence for a baud rate.

MIDI
------------------

[MIDI](http://en.wikipedia.org/wiki/Musical_Instrument_Digital_Interface) is a command set used by electronic (music) instruments. It travels over a standard serial UART configured for 31250bps/8/n/1. Since firmware v2.7 MIDI is a speed option in the UART library.

On BPv4 enter nominal custom baud `31250`; BPv3 raw BRG is `127`. Binary UART's
MIDI preset is selector `0x69`, or use raw-BRG command `0x07 0x00 0x7F`.

DIN MIDI uses current-loop signaling, not a direct TTL UART connection. Use a
suitable MIDI electrical interface, including input isolation; do not connect
the Bus Pirate pins directly to a MIDI DIN port. Adapter references:
[example 1](http://www.compuphase.com/electronics/midi_rs232.htm),
[example 2](http://www.midi.org/techspecs/electrispec.php).

Connections
------------------

![UART Connection Diagram](images/connections/UART.png)

Connect the Bus Pirate transmit pin (TX/MOSI) to the UART device receive pin (RX). Connect the Bus Pirate receive pin (RX/MISO) to the UART device transmit pin (TX).

In half-duplex single-wire mode, connect the Bus Pirate TX pin (MOSI) to the peer's single data line, share ground, and fit the pull-up resistor to 3.3 V.

For BPv3 macro (3), CTS is on CS and RTS is on CLOCK. BPv4 does not forward
these hardware handshake signals.

Diagram by Uwe Bannow, released under the [GNU free documentation license](http://en.wikipedia.org/wiki/Wikipedia:Text_of_the_GNU_Free_Documentation_License).

External Links
------------------

- [asciinema.org > connecting to a Raspberry Pi UART via GPIO using a Bus Pirate](https://asciinema.org/a/212988)
