AVRISP-MKII programmer
======================

The Bus Pirate v4 firmware contains an AVRISP-MKII compatible programmer
personality for programming AVR microcontrollers over ISP, PDI (XMEGA)
and TPI (TINY) interfaces.  The protocol engine is a port of
[LUFA's AVRISP-MKII project](https://www.lufa-lib.org) (Copyright (C)
Dean Camera, MIT-style licence, full text in
[avrisp-mkii-license.txt](avrisp-mkii-license.txt)).

The programmer is implemented as two binary bitbang mode commands; both
are entered from the binary bitbang mode (send `0x00` to the terminal at
least 20 times until `BBIO1` appears, then send the command byte).

- **`0x08` - AVRISP-MKII over USB** (BPv4 only): re-enumerates the Bus
  Pirate as a vendor-class device with Atmel's AVRISP mkII VID/PID
  (03EB:2104), speaking the raw STK500v2/XPROG command set over bulk
  endpoint 2.  Use `avrdude -c avrispmkII -P usb`.
- **`0x09` - AVRISP-MKII over CDC serial** (BPv4 only): serves the
  framed STK500v2 (AVR068) protocol over the normal USB serial port. Use
  `avrdude -c stk500v2 -P COMx`.  Because of a host-side limitation in
  avrdude (it only enables the XPROG PDI/TPI protocol for mkII-class
  programmers), this route provides **ISP programming only**; PDI and
  TPI require the USB route on BPv4.

Both modes end automatically after a few seconds without host traffic;
the Bus Pirate then returns to the binary bitbang prompt (USB users see
the serial port re-appear).  A hardware reset always leaves the mode.

## Use with avrdude (USB route, BPv4)

1. Enter binary bitbang mode and send `0x08`.
2. Wait for the driver/model string change (the device re-enumerates as
   "AVRISP mkII").
3. Program, for example:

```
avrdude -c avrispmkII -P usb -p <part> -U flash:w:<file>.hex
```

On Windows the 03EB:2104 device needs a libusb/WinUSB style driver
bound to it once (for example with the [Zadig](https://zadig.akeo.ie)
tool, choose the "AVRISP mkII" interface and install the WinUSB or
libusb driver).  The normal Bus Pirate serial port driver is untouched.
The serial route needs no additional driver at all.

### USB entry troubleshooting

With PowerShell `System.IO.Ports.SerialPort`, binary writes must use all
three arguments: `$sp.Write([byte[]]@(0x08), 0, 1)`. The single-argument
call `$sp.Write([byte[]]@(0x08))` selects `Write(string)` and sends ASCII
`8` (0x38), which BBIO correctly rejects with 0x00. The same trap turns
a supposed binary-zero probe into ASCII `0` (0x30).

After idle exit, close the old handle and reopen the returned CDC port.
CDC configuration completes before BBIO resumes, and one `BBIO1` return
identifier may be waiting. Read through quiescence before sending a new
binary 0x00 probe; its reply must be exactly one `BBIO1`. Do not discard
receive buffers while investigating unsolicited output: some serial
libraries purge them automatically on open and can hide a regression.

An idle WinUSB handle or empty OUT packets do not keep the programmer
session alive. USB reset/deconfiguration aborts an active transfer; a
host-held reset is bounded, and failed CDC configuration is retried by
detaching/attaching CDC again. Target pins are released before CDC
recovery, and BBIO does not use endpoints until they are configured.
If the host disables the USB port or refuses enumeration, firmware cannot
force Windows to expose a COM port, but recovery does not require a
Bus Pirate power cycle once normal host enumeration resumes.

## Use with avrdude (CDC serial route, BPv4)

1. Enter binary bitbang mode over the BPv4 USB serial port and send `0x09`.
2. Close the terminal and run, for example:

```
avrdude -c stk500v2 -P COMx -p <part> -U flash:w:<file>.hex
```

## Wiring

ISP (ATtiny/ATmega, 6-pin programming header convention):

Check the exact target's programming pin map. ATmega64A uses PE0/PDI as
MOSI input and PE1/PDO as MISO output, not its ordinary PB2/PB3 SPI pins.

| programmer pin | BP pin | target pin |
|---|---|---|
| SCK | CLK | SCK |
| MOSI | MOSI | MOSI |
| MISO | MISO | MISO |
| /RESET | AUX | /RESET |
| VTARGET sense | ADC | target VCC (optional) |
| rescue clock (4 MHz) | CS | XTAL1, rescue only, see below |
| GND | GND | GND |

PDI (XMEGA) / TPI (TINY):

| programmer pin | BP pin | target pin |
|---|---|---|
| CLOCK | CLK | PDI_CLK (the XMEGA RESET pin) / TPI_CLK |
| DATA | MOSI **and** MISO, each through a 220 ohm resistor | PDI_DATA / TPIDATA |
| /RESET (TPI only) | AUX | /RESET (leave AUX unconnected for PDI) |
| VTARGET sense | ADC | target VCC (optional) |
| GND | GND | GND |

The two 220 ohm resistors join the Bus Pirate's MOSI and MISO pins into
the single bidirectional DATA line, the same arrangement LUFA's AVRISP
project uses.

For ATtiny4/5/9/10, NVM programming requires a **5 V** target supply
(datasheet 8127H, sections 15.3.1 and 16). Check the exact TPI target's
requirements and use suitable level translation. RSTDISBL targets that
require 12 V reset are unsupported; never connect 12 V to a BP pin.

The **VTARGET** parameter reported to avrdude is measured on the BP's
voltage probe pin (ADC), which has a 2:1 divider and accepts 0 to ~6.6 V.
If the target supply is not wired to the ADC pin, the floating input is
not a reliable voltage reading. VTARGET is informational; it does not
set the I/O voltage or interlock programming.

The programmer drives push-pull **3.3 V** signals, including the rescue
clock, and does not supply target power. Power the target externally and
connect grounds. Use appropriate bidirectional level translation for
incompatible target voltages/logic thresholds; a 5 V-tolerant input is
not a 5 V output. The 220 ohm resistors are not level shifters. Entry
from BBIO resets/disables the Bus Pirate's power and pull-ups.

The **rescue clock** runs continuously on the CS pin at 4 MHz while the
personality is active.  For targets whose fuses select an external
clock, wire CS to the target's XTAL1 pin and use 125 kHz ISP speed
(`-B 8`, a period of 8 microseconds). `-B 125` means 125 microseconds
(8 kHz), not 125 kHz. The rescue clock pauses during AVR053 OSCCAL,
when the same PWM generator supplies a 32.787 kHz calibration clock
on MOSI. OSCCAL requires AVR053 calibration firmware on the target;
it does not install that firmware or directly program an OSCCAL fuse.

## Notes and limitations

- BPv3: this personality is not included or supported. Its existing
  Bus Pirate programming interfaces are unchanged. Both programmer entry
  commands described here require BPv4.
- The serial route is AVR068 framing over USB CDC, not a hardware-UART
  personality or a baud-switching mode. Host baud settings do not set an
  electrical target clock.
- TPI: the protocol engine is complete, but no TPI-capable target was
  available for bench verification yet.
- **Hardware verification:** the programmer was exercised with
  ATxmega128A3U **PDI** (signature, read,
  erase, write + verify, EEPROM, fuses, restore), **ATmega64A ISP**
  (signature, fuses, erase, write + full readback, EEPROM, fuse write),
  mkII USB entry/re-enumeration/idle-exit (incl. Windows driver bind and
  PnP restarts) and the BPv4 CDC serial STK500v2 route. **TPI remains
  source/build/native-tested only**; no TPI target was hardware-tested.
- Timer 3 supplies the 10 ms watchdog; Timer 4 supplies a 25 kHz idle
  PDI/TPI clock between commands. Byte transfers use software clocking
  at approximately 125 kHz. Timer 1 remains the normal delay timebase;
  Timer 2 and OC5 supply the rescue/calibration clock.
- Requests and reads support up to 256 memory data bytes. XMEGA pages
  larger than this are split by avrdude. The one-second target watchdog
  limits how much can be transferred at the slowest ISP rates; use small
  blocks with a custom host there. Software SPI rates are approximate
  (integer-microsecond delays plus firmware overhead).
- Parameters reset on mode entry; SCK duration and reset polarity are
  not persisted to EEPROM as they are in LUFA.
- Malformed, oversized, and unknown requests receive failure/unknown
  status rather than LUFA's stall/no-reply paths. XPROG CRC preserves
  LUFA's unusual high-byte, low-byte, middle-byte reply order.
- This main-firmware personality supersedes the separate STK500v2
  alternate on BPv4. It is independent of the existing avrdude
  `buspirate` interface, which remains available.

## Hardware-free regression checks

With Python 3, a host GCC, and XC16 installed, run from `Firmware`:

```text
python tests/test_avrisp.py
python tests/test_avrisp_cdc_gate.py
python tests/test_avrisp_board_gate.py
```

The native suite checks protocol/PHY models and CDC serial framing,
including 266-byte payloads. The XC16 checks verify actual CDC dispatch
and enforce BPv4 inclusion/BPv3 exclusion without injected feature flags.
These tests do not emulate USB enumeration, MCU-cycle timing or target
electrical behavior. Generated files remain under the ignored build tree.
