# AVRISP-MKII — ISP path (ATtiny / ATmega)

Part of the AVRISP-MKII documentation set (`docs/avrisp/`).  This page
covers the **ISP** target interface; wiring is in [wiring.md](wiring.md),
the command set in [protocol.md](protocol.md).

## Scope

- Covers: entering the programmer, the `avrdude` ISP flow, the clock-speed
  ladder, and ISP-specific limits.
- Does not cover: PDI ([pdi.md](pdi.md)), TPI ([tpi.md](tpi.md)), the CDC
  serial route ([serial-route.md](serial-route.md)), or the rescue clock
  ([rescue-clock-osccal.md](rescue-clock-osccal.md)).

## Entry

The personality has no terminal menu entry.  Enter it from binary bitbang
mode (BBIO1) — see `docs/generic/` for reaching BBIO1 — then send the
**USB** entry byte:

1. Send `0x00` repeatedly until the reply `BBIO1` appears.
2. Send byte **`0x08`** — mkII USB personality.
3. The CDC serial port disappears; the device re-enumerates as a vendor
   device with product string `AVRISP mkII` (VID/PID `03EB:2104`).
4. Run `avrdude` against that device.

The USB route is the only route that can reach PDI and TPI.  The serial
route ([serial-route.md](serial-route.md)) is ISP-only.

## Use with avrdude

```
avrdude -c avrispmkII -P usb -p <part> -U flash:w:<file>.hex:i
```

For example, an ATmega328P is `-p m328p`.

A read-only signature check:

```
avrdude -c avrispmkII -P usb -p <part> -n -U signature:r:-:h
```

- **On Windows** the `03EB:2104` device needs a libusb/WinUSB style driver
  bound to it once (for example with the Zadig tool, choosing the
  `AVRISP mkII` interface).  The normal Bus Pirate serial driver is not
  touched.  The serial route needs no extra driver.
- The programmer reports the target **VTARGET** and a firmware version to
  the host as part of the normal mkII handshake.

## Speed

The host selects the ISP SCK period through the STK500v2 `SCK_DURATION`
parameter (`0x98`).  The programmer implements the full AVR069 speed
ladder (indices 0–163):

| Index | SCK frequency | Clock engine |
|---|---|---|
| 0 | 8 MHz | hardware SPI |
| 1 | 4 MHz | hardware SPI |
| 2 | 2 MHz | hardware SPI |
| 3 | 1 MHz | hardware SPI |
| 4 | 500 kHz | hardware SPI |
| 5 | 250 kHz | hardware SPI |
| 6 (default) | 125 kHz | hardware SPI |
| 7 … 163 | below 125 kHz, down to about 51 Hz (computed; approximate) | software |

- The fastest seven ladder steps (indices 0–6, 8 MHz down to 125 kHz) use
  the PIC24 hardware SPI module.  The remaining steps use blocking
  software clocking and are **approximate** (integer-microsecond delays
  plus firmware overhead vary with the selected rate).
- `avrdude` picks the index with `-B <period-in-microseconds>`.  The
  number is a **period**, not a frequency: **`-B 8` means 8 µs =
  125 kHz** and **`-B 125` means 125 µs = 8 kHz**.  Use `-B 8` for
  125 kHz.
- The speed parameter is set by the host each session and is **not
  persisted** across sessions.

## Limits

- **1-second target budget.**  A one-second watchdog bounds each command.
  At the slowest software rates a full-page operation can exceed it; the
  programmer fails fast and reports a timeout rather than hanging.  Use
  small blocks with a custom host there.
- **256 data bytes per request.**  Flash/EEPROM transfers are limited to
  256 bytes each; hosts split larger pages.
- **Software SPI rates are approximate and not every full page at every
  rate is promised.**
- **No reset output drive on release.**  `/RESET` is asserted at the
  host-selected polarity and tristated on release; a target that needs a
  driven reset release must have its own pull-up.
- **No target power.**  See [wiring.md](wiring.md).

## ISP vs. the ordinary SPI mode

This is an ISP *programmer*: the host supplies the AVR programming opcodes
and the poll values, and the personality executes them.  It is unrelated to
the Bus Pirate's interactive SPI mode and to the host-side `buspirate` and
`buspirate_bb` interfaces, which remain available and are not changed by
this personality.