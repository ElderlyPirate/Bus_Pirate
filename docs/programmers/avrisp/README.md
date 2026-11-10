# AVRISP-MKII

**Applies to:** Bus Pirate **v4** — the personality is compiled out on v3.

AVRISP-MKII is a USB programmer personality built into the Bus Pirate v4
firmware.  It re-enumerates the Bus Pirate as an Atmel AVRISP mkII
compatible device (VID/PID `03EB:2104`, product string `AVRISP mkII`) and
speaks the STK500v2 / XPROG command set that `avrdude` uses, so the Bus
Pirate can program AVR targets over **ISP** (ATtiny/ATmega), **PDI**
(XMEGA) and **TPI** (tinyAVR).  A second entry serves the framed
STK500v2 protocol over the normal USB CDC serial port.  The protocol
engine is a port of LUFA's AVRISP-MKII project — see
[license.md](license.md).

**Who it is for:** an operator programming AVR targets from a host using
`avrdude` (or an equivalent mkII-capable host tool) over the Bus Pirate's
USB link.

This README is a map, not the manual — the details live in the files
below.

| File | Covers |
|---|---|
| [entry.md](entry.md) | getting in and out: entry bytes, host snippet, idle deadline, `BBIO1` return |
| [wiring.md](wiring.md) | target wiring for ISP, PDI and TPI; voltages; the rescue-clock wire |
| [isp.md](isp.md) | ISP path: `avrdude` flow, speeds, fuses, lock bits, erase, verify |
| [pdi.md](pdi.md) | PDI path (XMEGA): `avrdude` flow, fuses, limits |
| [tpi.md](tpi.md) | TPI path (tinyAVR): wiring, the 5 V supply, word rules, limits |
| [serial-route.md](serial-route.md) | the framed STK500v2 route over CDC (`-c stk500v2`) |
| [rescue-clock-osccal.md](rescue-clock-osccal.md) | the 4 MHz rescue clock and AVR053 OSCCAL calibration |
| [protocol.md](protocol.md) | both transports, and the STK500v2 / XPROG catalogue |
| [edge-cases.md](edge-cases.md) | failure modes: trigger → symptom → recovery |
| [license.md](license.md) | LUFA copyright and licence text |

## The five-minute version

- **Enter** from binary bitbang mode: send `0x00` until `BBIO1` appears
  (20× on v4), then byte **`0x08`** (USB) or byte **`0x09`** (serial).  A
  terminal cannot send NULs — the host snippet is in [entry.md](entry.md).
- **Start `avrdude` immediately.**  About **five seconds** of silence ends
  the mode, counted from the moment of entry (USB enumeration is inside
  that window).
- **USB route:** after `0x08` the device re-enumerates as `AVRISP mkII`;
  program with `avrdude -c avrispmkII -P usb -p m328p …`.  Windows needs
  a WinUSB/libusb driver bound once; Linux needs a udev rule or the
  permissions; macOS needs neither (see [isp.md](isp.md)).
- **Serial route:** after `0x09` use `avrdude -c stk500v2 -P COM5 …`; this
  route is **ISP only**, and needs no extra driver.
- **Wiring headline (ISP):** CLK→SCK, MOSI→target MOSI, MISO→target MISO,
  AUX→/RESET, GND→GND.  `CS` is **not** a chip select here: it
  continuously outputs the 4 MHz rescue clock, so leave it open unless you
  are injecting that clock into XTAL1 — see
  [rescue-clock-osccal.md](rescue-clock-osccal.md).  **No pull-up or supply
  is required from the Bus Pirate** — it drives push-pull 3.3 V and does
  not power the target.
- **Wiring headline (PDI/TPI):** CLK→clock, DATA = MOSI **and** MISO joined
  through resistors; PDI leaves AUX open, TPI uses AUX→/RESET.
- **Leaving:** the mode ends by itself and the binary layer prints `BBIO1`
  again; byte `0x0F` then returns to the terminal.  A hardware reset always
  leaves.

Shared prompt grammar (`r`, `W`, `P`, wrappers, `$`, `#`) is documented
once in `docs/generic/` and not repeated here.

## Availability (summary)

| Feature | v3 | v4 |
|---|---|---|
| build | not included (compiled out) | included (default image) |
| USB mkII personality (entry `0x08`) | — | yes |
| serial STK500v2 route (entry `0x09`) | — | yes |
| ISP programming | — | yes |
| PDI programming (XMEGA) | — | yes (USB route); **not verified for every XMEGA part** |
| TPI programming (tinyAVR) | — | yes (USB route); **not verified** |
| rescue clock / AVR053 OSCCAL | — | yes |
| target power supply from the Bus Pirate | — | no — the target is powered externally |

Both entry bytes are accepted only on v4: a v3 build answers `0x00` and
stays in the binary layer.  Full per-item detail is in the topic files.

## Limits & out of scope

- **v4 only.**  The v3 firmware does not contain the personality; its
  existing programming interfaces are unchanged.
- **No high-voltage programming.**  HVPP/HVSP (the STK500 high-voltage
  heritage) and 12 V-reset targets are out of scope — the Bus Pirate does
  not generate VPP, and 12 V must never reach a Bus Pirate pin.
- **No JTAG over this personality.**  AVR-JTAG would need a different
  personality family; the Bus Pirate's JTAG support is its own OpenOCD and
  XSVF modes.
- **Not a general STK500 clone.**  Only the AVRISP mkII command surface is
  implemented; the V2 protocol engine executes the host-supplied
  programming opcodes and does not add a device database.
- **No target power.**  The programmer drives the signal pins only; the
  target must be externally powered, with a common ground.

## Related docs

- Lifecycle and host entry: [entry.md](entry.md).
- Shared terminal / binary grammar: `docs/generic/` (see
  `docs/generic/commands.md`).
- The binary layer under this personality:
  `docs/protocol/binary/entering-and-framing.md`.
- Implementation design notes (maintainers):
  [avrisp-design-notes.md](../../dev/avrisp-design-notes.md).
