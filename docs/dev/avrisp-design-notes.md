# AVRISP-MKII — design notes and deviations (maintainer summary)

**Status:** maintainer record for the AVRISP-MKII user set
(`docs/programmers/avrisp/`); the user pages carry no code references.

This page is a maintainer-oriented summary of *why* the implementation behaves
as it does and where it deliberately differs from LUFA's AVRISP-MKII project.
It is written at the behaviour level.  The user-facing state model is
[command-model.md](../programmers/avrisp/command-model.md), the worked
exchanges are [sessions.md](../programmers/avrisp/sessions.md), and the host
identity and driver material are
[host-tools.md](../programmers/avrisp/host-tools.md).

## Overall shape

- The personality is one **inlined** programmer image: there is no separate
  firmware to flash and no alternate STK500v2 build.  The protocol engine is a
  port of LUFA's AVRISP-MKII project; only the AVRISP mkII command surface is
  implemented.
- The V2 layer executes host-supplied AVR programming opcodes and poll values.
  It does not carry a device database or per-part knowledge.
- Two transports share one engine: the **USB mkII personality** (bulk
  endpoint 2) and the **framed STK500v2 serial route** (USB CDC).  They differ
  only in framing and identity.  See
  [protocol.md](../programmers/avrisp/protocol.md) and
  [serial-route.md](../programmers/avrisp/serial-route.md).

## USB transport design

- Vendor-class device, VID/PID `03EB:2104`, product string `AVRISP mkII`;
  bulk endpoint 2 OUT/IN at 64 bytes per packet; bus powered at 100 mA.
- A complete request is buffered through its terminating short packet or ZLP
  before dispatch, so a host that splits a command across packets is handled
  correctly.  Replies that are an exact packet multiple are terminated with a
  ZLP.
- **Unknown, malformed and oversized requests are answered** with a
  failure/unknown status rather than the LUFA stall/no-reply paths.  This keeps
  `avrdude` synchronised instead of timing out.
- The personality **waits for the host to configure the USB interface** before
  exposing it, and resumes the binary mode only after the CDC port is
  configured again on the way out.  This avoids reading unarmed endpoints.
- **Idle exit** is about five seconds without host activity.  Empty OUT
  packets and an idle handle do **not** renew the session; a real non-empty
  request during legitimate programming does.
- **Recovery is bounded:** a host-held USB reset is bounded by a timer and
  does not block forever; if CDC configuration fails, the firmware detaches
  and re-attaches and retries rather than waiting permanently.  Target pins
  are released before CDC recovery, even if the host is slow.
- A host that physically disables the port or refuses enumeration cannot be
  forced to expose a COM port, but recovery does not need a Bus Pirate power
  cycle once normal enumeration returns.

## Resource usage

| Resource | Role |
|---|---|
| Timer 3 | 10 ms tick for the command and idle watchdogs |
| Timer 4 | 25 kHz idle clock on the PDI/TPI clock line between commands |
| Timer 2 + output compare 5 | 4 MHz rescue clock (`CS`) / 32.787 kHz OSCCAL clock (`MOSI`) |
| Timer 1 | ordinary delay timebase, unchanged |
| Hardware SPI module | the seven fastest ISP ladder steps |
| Software clocking | the remaining ISP ladder steps and the PDI/TPI bit stream |

- The idle clock runs at a raised interrupt priority so it can preempt USB
  servicing and satisfy PDI's inactivity limit.  Interrupt nesting is enabled
  for the mode and restored on exit.
- The idle-clock interrupt deliberately avoids an inline repeated instruction
  that could clobber an interrupted count register.

## Transport and protocol deviations from LUFA

| Area | Behaviour here | LUFA |
|---|---|---|
| Parameters | reset to defaults on every mode entry | persisted to EEPROM |
| Unknown/oversize requests | answered with failure/unknown status | stall / no reply |
| Connection-status parameter | fixed *ISP ready* (`0x00`), queried separately by the host | same value; no fault bits |
| XPROG CRC reply order | high byte, low byte, middle byte; not a normalised order | same; no normalised order |
| PDI/TPI physical layer | software synchronous 12-bit 8E2 (~125 kHz); idle clock between commands | AVR USART/USRT-based |
| XMEGA section/page erase | the same command/trigger sequence; the manual is ambiguous | same |

- **ISP enable reply** is exactly command + status (two bytes), on success and
  on failure.  `avrdude` then queries the connection-status parameter
  separately; the absence of a third reply byte is not a fault.
- **XPROG ERASE takes a 7-byte request**, not eight.  An over-long request is
  rejected before the target is touched.
- **EEPROM erase tags** the selected page before erasing so only the intended
  page is cleared.

## PDI / TPI physical layer

- The clock and data lines are bit-banged as a synchronous 12-bit 8E2 frame
  (LSB first), because the PIC24 UARTs have no synchronous mode.
- Reception aligns on the target's start bit; parity and both stop bits are
  checked, and a bad frame aborts the command rather than returning corrupt
  data.
- The NVM enable key and the ready/busy poll loop are shared between PDI and
  TPI; the opcodes, pointer setup and completion conditions stay explicit per
  protocol.
- The target data line is half duplex: transmit drives through the series
  resistors, receive releases the driver and senses the same node.
- PDI keeps a ~25 kHz keep-alive clock on the line between commands; data
  transfers run at ~125 kHz.  TPI holds the target's `RESET` low for the whole
  session.

## AVR053 OSCCAL calibration

The host sends `OSCCAL` (`0x05`); the personality performs the clock/sense part
of the AVR053 procedure and the target runs the AVR053 calibration firmware.

| Item | Behaviour |
|---|---|
| clock | 32.787 kHz on **`MOSI`**, from the Timer 2 / OC5 generator (16 MHz / 488 = 32786.9 Hz, +0.058% off 32768 Hz) |
| not `XTAL1` | the calibration clock does not go to the target's `XTAL1`; that pin is only for the 4 MHz rescue clock on `CS` |
| sense | **`MISO`**, with a pull-up; the previous pull-up state is restored |
| `RESET` | released before counting; the target's reset network must hold it high |
| `SCK` / `CLK` | not driven during the command; `CLK` is an input.  The AVR053 reference wiring leaves `SCK` off |
| window | 1024 calibration-clock cycles plus 25 cycles, about 32 ms nominal |
| reply | `05 00` success, `05 80` timeout, `05 C0` rejected extra bytes before any pin is touched |
| value | the reply never carries an OSCCAL value and the personality never writes the OSCCAL fuse or register |
| rescue clock | paused during the command, then restored to 4 MHz on `CS` |

- The target must already run the AVR053 firmware; it is the operator's own and
  is not installed by this personality.  `avrdude -O` / `--osccal` sends the
  same command.
- The full procedure and wiring are in
  [rescue-clock-osccal.md](../programmers/avrisp/rescue-clock-osccal.md).

## Build selection

| Build | AVRISP-MKII |
|---|---|
| v4 default (`BUSPIRATEV4`) | `BP_ENABLE_AVRISP_MKII_SUPPORT` defined |
| v3 default | not available; the switch `#error`s unless the board is v4 |
| `BP_CUSTOM_FEATURE_SET` | **not defined** in the current custom list |

- The custom all-features list omits `BP_ENABLE_AVRISP_MKII_SUPPORT`.  To
  include the programmer in a custom v4 build, add that switch to the
  `BP_CUSTOM_FEATURE_SET` block; the v4 all-features link result in
  [building.md](building.md) therefore excludes AVRISP.
- On v3 the guard is
  `#if defined(BP_ENABLE_AVRISP_MKII_SUPPORT) && !defined(BUSPIRATEV4)`.

## Known limits (see the topic pages)

- v4 only; TPI is not verified on a physical target.
- The transport buffer is 288 bytes; XPROG target payloads stay at or below
  256 bytes.  The command watchdog gives a one-second target budget at the
  slowest ISP rates; software ISP rates are approximate.
- No power generation, no high-voltage programming, no JTAG.
- A damaged PDI/TPI link may require a target power cycle if its interface
  cannot be reset by the bounded cleanup.
- Atmel Studio is not a supported host path for this port; use `avrdude`.

## Related docs

| Page | Why |
|---|---|
| [entry.md](../programmers/avrisp/entry.md) | entry bytes, idle deadline and exit |
| [command-model.md](../programmers/avrisp/command-model.md) | personality lifecycle, session state, command order |
| [sessions.md](../programmers/avrisp/sessions.md) | worked USB and serial exchanges |
| [host-tools.md](../programmers/avrisp/host-tools.md) | `avrdude`, drivers, Windows USB first use |
| [protocol.md](../programmers/avrisp/protocol.md) | command, status and parameter catalogue |
| [isp.md](../programmers/avrisp/isp.md) | ISP flow and the speed ladder |
| [pdi.md](../programmers/avrisp/pdi.md) / [tpi.md](../programmers/avrisp/tpi.md) | XPROG target flows |
| [rescue-clock-osccal.md](../programmers/avrisp/rescue-clock-osccal.md) | 4 MHz rescue clock and AVR053 |
| [wiring.md](../programmers/avrisp/wiring.md) | header pinout and target wiring |
| [serial-route.md](../programmers/avrisp/serial-route.md) | AVR068 framing over CDC |
| [edge-cases.md](../programmers/avrisp/edge-cases.md) | failure and recovery entries |
| [license.md](../programmers/avrisp/license.md) | licensing |
