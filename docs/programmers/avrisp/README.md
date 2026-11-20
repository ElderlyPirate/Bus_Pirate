# AVRISP-MKII

**Applies to:** Bus Pirate **v4** only.  The personality is not part of the v3
feature set, and a v3 build that tries to enable it stops with a compile-time
error.

AVRISP-MKII is a programmer personality in the v4 firmware.  On command it
re-enumerates the Bus Pirate as an Atmel **AVRISP mkII** compatible USB device
(VID/PID `03EB:2104`) and runs the STK500v2 / XPROG command set that `avrdude`
uses, so the Bus Pirate can program AVR targets over **ISP** (ATtiny/ATmega),
**PDI** (XMEGA) and **TPI** (tinyAVR).  A second entry runs the same engine as
framed STK500v2 over the normal USB CDC serial port.  The engine is a port of
LUFA's AVRISP-MKII project — see [license.md](license.md).

Three names belong to the same feature: the personality is **AVRISP-MKII**,
the USB product string is **AVRISP mkII**, and the protocol `SIGN_ON`
identifier is **AVRISP_MK2** on the USB route and **STK500_2** on the serial
route.  The mapping is stated again in [protocol.md](protocol.md).

**Who it is for:** an operator programming AVR targets from a host with
`avrdude` (or another mkII-capable host tool) over the Bus Pirate's USB link.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry.md](entry.md) | build availability, entry bytes, host snippet, startup and idle deadlines, leaving |
| [wiring.md](wiring.md) | v4 header, ISP/PDI/TPI wiring, pull-ups, supply, VTARGET |
| [isp.md](isp.md) | ISP flow: `avrdude`, fuses, lock bits, erase, verify, speed |
| [pdi.md](pdi.md) | PDI flow (XMEGA): `avrdude`, fuses, limits |
| [tpi.md](tpi.md) | TPI flow (tinyAVR): 5 V target supply, word rules, limits |
| [serial-route.md](serial-route.md) | framed STK500v2 over CDC: AVR068 framing and the serial host path |
| [host-tools.md](host-tools.md) | prerequisites, device identity, `avrdude`, host drivers, Atmel Studio note |
| [protocol.md](protocol.md) | USB transport, request/reply shapes, commands, status, parameters |
| [command-model.md](command-model.md) | the session and state model at user level |
| [sessions.md](sessions.md) | worked identity, parameter, error and ISP sessions |
| [rescue-clock-osccal.md](rescue-clock-osccal.md) | 4 MHz rescue clock and AVR053 calibration |
| [edge-cases.md](edge-cases.md) | failure modes: trigger → symptom → recovery |
| [license.md](license.md) | LUFA copyright and licence text |

## The five-minute version

- **Build:** included in the default v4 image.  The all-features custom feature
  set does **not** include it, and v3 cannot enable it — see
  [entry.md](entry.md) §Build availability.
- **Enter** from binary bitbang mode: send `0x00` until `BBIO1` appears
  (20 ×), then byte **`0x08`** (USB) or byte **`0x09`** (serial).  A
  terminal emulator normally cannot send NULs — use the host snippet in
  [entry.md](entry.md), then start `avrdude` once `03EB:2104` appears (USB)
  or once the entry script has released the port (serial).
- **Deadline:** USB — about **two minutes (120 s)** from attach to the first
  request (covers enumeration and driver binding); serial — about **five
  seconds** from entry.  After the first request both routes end after about
  **five seconds** without host traffic; only a completed request renews it
  on USB, any byte on the serial route.
- **USB route:** the device re-enumerates as `AVRISP mkII`; program with
  `avrdude -c avrispmkII -P usb -p m328p -U flash:w:app.hex:i`.  Host
  drivers and permissions are in [host-tools.md](host-tools.md).  A
  first-time Windows libusb/WinUSB binding is retried by re-entering if it
  resets the device or takes longer than the two-minute startup budget; see
  [host-tools.md](host-tools.md) §Windows USB first use.  For ISP first
  use, prefer the serial route.
- **Serial route:** after `0x09`, use
  `avrdude -c stk500v2 -P COM5 -p m328p -U flash:w:app.hex:i`.  `COM5` is
  a Windows example; use `/dev/ttyACM0` on Linux or the
  `/dev/cu.usbmodem…` name on macOS.  This is the ISP route in `avrdude`
  and needs no libusb driver; see [serial-route.md](serial-route.md).
- **Wiring (ISP):** `CLK`→`SCK`, `MOSI`→target `MOSI`,
  `MISO`→target `MISO`, `AUX`→`RESET`, `GND`→`GND`.
  **No pull-up or target supply is required from the Bus Pirate**; it drives
  push-pull 3.3 V and does not power the target.  `CS` outputs the 4 MHz
  rescue clock for the session — leave it open unless you are injecting that
  clock into `XTAL1`.
  **PDI/TPI:** `CLK`→clock, data = `MOSI` and `MISO` joined through
  resistors; PDI leaves `AUX` open, TPI uses `AUX`→`RESET`.  Details in
  [wiring.md](wiring.md).

## Availability (summary)

| Feature | v3 | v4 |
|---|---|---|
| build | not available (compile-time refusal) | included (default image) |
| USB mkII personality (entry `0x08`) | — | yes |
| serial STK500v2 route (entry `0x09`) | — | yes |
| ISP programming | — | yes |
| PDI programming (XMEGA) | — | yes (USB route with `avrdude`); **not verified for every XMEGA part** |
| TPI programming (tinyAVR) | — | yes (USB route with `avrdude`); **not verified** |
| rescue clock / AVR053 OSCCAL | — | yes (calibration **not verified**) |
| target power supply from the Bus Pirate | — | no — the target is powered externally |

A v4 custom build that uses the all-features feature set omits the
personality: the list is fixed and leaves AVRISP-MKII out.  Add
`BP_ENABLE_AVRISP_MKII_SUPPORT` to the custom feature list, or use the
default v4 feature set, to keep it.  On v3 the build stops if the switch is
forced, and a prebuilt v3 image answers the entry bytes with `0x00`, then
`BBIO1`.  The build route is [dev/building.md](../../dev/building.md).

## Limits & out of scope

- **v4 only.**  The v3 firmware does not contain the personality; its existing
  programming interfaces are unchanged.
- **No high-voltage programming.**  HVPP/HVSP (the STK500 high-voltage
  heritage) and 12 V-reset targets are out of scope — the Bus Pirate does not
  generate VPP, and 12 V must never reach a Bus Pirate pin.
- **No JTAG over this personality.**  AVR-JTAG would need a different
  personality family; the Bus Pirate's JTAG support is its own OpenOCD and
  XSVF modes.
- **Not a general STK500 clone.**  Only the AVRISP mkII command surface is
  implemented; the V2 engine executes the host-supplied programming opcodes
  and does not add a device database.
- **No target power.**  The programmer drives the signal pins only; the
  target must be externally powered, with a common ground.
- **PDI is not verified for every XMEGA part, and TPI is not verified at
  all.**  Atmel Studio is **not a supported host path** for this port; use
  `avrdude` (see [host-tools.md](host-tools.md)).

## Related docs

- Lifecycle and host entry: [entry.md](entry.md).
- Shared terminal / binary grammar: [docs/generic/](../../generic/README.md)
  and [docs/generic/commands.md](../../generic/commands.md).
- The binary layer under this personality:
  [docs/protocol/binary/](../../protocol/binary/README.md).
- The sibling programmer: [PIC programmer](../pic/README.md).
- Interactive SPI (a different function): [SPI](../../protocol/spi/README.md).
- Capability index: [docs/SPEC.md](../../SPEC.md).
- Implementation design notes (maintainers):
  [avrisp-design-notes.md](../../dev/avrisp-design-notes.md).
