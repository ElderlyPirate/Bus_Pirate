# AVRISP-MKII — design notes and deviations (maintainer summary)

**Status:** maintainer record for the AVRISP-MKII user set
(`docs/avrisp/`); the user pages carry no code references.

This page is a maintainer-oriented summary of *why* the implementation
behaves as it does and where it deliberately differs from LUFA's
AVRISP-MKII project.  It is written at the behaviour level.

## Overall shape

- The personality is one **inlined** programmer image: there is no
  separate firmware to flash and no alternate STK500v2 build.  The
  protocol engine is a port of LUFA's AVRISP-MKII project; only the
  AVRISP mkII command surface is implemented.
- The V2 layer executes host-supplied AVR programming opcodes and poll
  values.  It does not carry a device database or per-part knowledge.
- Two transports share one engine: the **USB mkII personality** (bulk
  endpoint 2) and the **framed STK500v2 serial route** (USB CDC).  They
  differ only in framing and identity (see
  [protocol.md](../avrisp/protocol.md)).

## USB transport design

- Vendor-class device, VID/PID `03EB:2104`, product string
  `AVRISP mkII`; bulk endpoint 2 OUT/IN at 64 bytes per packet; bus
  powered.
- A complete request is buffered through its terminating short packet or
  ZLP before dispatch, so a host that splits a command across packets is
  handled correctly.  Replies that are an exact packet multiple are
  terminated with a ZLP.
- **Unknown, malformed and oversized requests are answered** with a
  failure/unknown status rather than the LUFA stall/no-reply paths.  This
  keeps `avrdude` synchronised instead of timing out.
- The personality **waits for the host to configure the USB interface**
  before exposing it, and resumes the binary mode only after the CDC port
  is configured again on the way out.  This avoids reading unarmed
  endpoints.
- **Idle exit** is about five seconds without host activity.  Empty OUT
  packets and an idle handle do **not** renew the session; a real non-empty
  request during legitimate programming does.
- **Recovery is bounded:** a host-held USB reset is bounded by a timer and
  does not block forever; if CDC configuration fails, the firmware detaches
  and re-attaches and retries rather than waiting permanently.  Target
  pins are released before CDC recovery, even if the host is slow.
- A host that physically disables the port or refuses enumeration cannot
  be forced to expose a COM port, but recovery does not need a Bus Pirate
  power cycle once normal enumeration returns.

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
  servicing and satisfy PDI's inactivity limit.  Interrupt nesting is
  enabled for the mode and restored on exit.
- The idle-clock interrupt deliberately avoids an inline repeated
  instruction that could clobber an interrupted count register.

## Transport and protocol deviations from LUFA

| Area | Behaviour here | LUFA |
|---|---|---|
| Parameters | reset to defaults on every mode entry | persisted to EEPROM |
| Unknown/oversize requests | answered with failure/unknown status | stall / no reply |
| Connection-status parameter | fixed *ISP ready* (`0x00`), queried separately by the host | same value; no fault bits |
| XPROG CRC reply order | high byte, low byte, middle byte | same (preserved deliberately) |
| PDI/TPI physical layer | software synchronous 12-bit 8E2 (~125 kHz); idle clock between commands | AVR USART/USRT-based |
| XMEGA section/page erase | LUFA/Table sequence retained | same; manual contradicts itself |

- **ISP enable reply** is exactly command + status (two bytes), on success
  and on failure.  `avrdude` then queries the connection-status parameter
  separately; the absence of a third reply byte is not a fault.  A
  misleading `Unknown status 0x00` from the host's secondary diagnostic is
  not a firmware defect.
- **XPROG ERASE takes a 7-byte request**, not eight.  An over-long request
  is rejected before the target is touched.
- **EEPROM erase tags** the selected page before erasing so only the
  intended page is cleared.

## PDI / TPI physical layer

- The clock and data lines are bit-banged as a synchronous 12-bit 8E2
  frame (LSB first), because the PIC24 UARTs have no synchronous mode.
- Reception aligns on the target's start bit; parity and both stop bits
  are checked, and a bad frame aborts the command rather than returning
  corrupt data.
- The NVM enable key and the ready/busy poll loop are shared between PDI
  and TPI; the opcodes, pointer setup and completion conditions stay
  explicit per protocol.
- The target data line is half duplex: transmit drives through the series
  resistors, receive releases the driver and senses the same node.

## Known limits (see the topic pages)

- v4 only; TPI not verified.
- 256-byte request/reply cap; one-second target budget at the slowest ISP
  rates; software ISP rates approximate.
- No power generation, no high-voltage programming, no JTAG.
- A damaged PDI/TPI link may require a target power cycle if its interface
  cannot be reset by the bounded cleanup.