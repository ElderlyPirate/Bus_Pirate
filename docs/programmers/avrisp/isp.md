# AVRISP-MKII — ISP path (ATtiny / ATmega)

Part of the AVRISP-MKII documentation set.  This page covers the **ISP**
target interface; wiring is in [wiring.md](wiring.md), the command set in
[protocol.md](protocol.md), and the entry/exit lifecycle in
[entry.md](entry.md).

## Scope

- Covers: the `avrdude` ISP flow, fuses/lock bits, erase and read-back, the
  clock-speed ladder, and ISP-specific limits.
- Does not cover: PDI ([pdi.md](pdi.md)), TPI ([tpi.md](tpi.md)), the CDC
  serial route ([serial-route.md](serial-route.md)), or the rescue clock
  ([rescue-clock-osccal.md](rescue-clock-osccal.md)).

## Entry

Send the USB entry byte `0x08` from `BBIO1`; the device re-enumerates as
`AVRISP mkII`.  The host-side snippet, the startup and idle deadlines (USB:
about two minutes to the first request; five seconds of idle afterwards) and
the way back out are owned by [entry.md](entry.md).

The USB route is the normal route for PDI and TPI.  The serial route
([serial-route.md](serial-route.md)) is the ISP route in `avrdude`; the same
engine also accepts XPROG for a custom host.

## Use with avrdude

```
avrdude -c avrispmkII -P usb -p m328p -U flash:w:app.hex:i
```

`-p m328p` is the part (an ATmega328P); use the host tool's own name for the
target.  Real part and file names are used throughout this set; in
PowerShell, avoid angle brackets in a command line because the shell treats
them as redirection.

A read-only signature check:

```
avrdude -c avrispmkII -P usb -p m328p -n -U signature:r:-:h
```

Host drivers, permissions and the USB identity are in
[host-tools.md](host-tools.md).  The programmer reports the target
**VTARGET** and a firmware version to the host as part of the normal mkII
handshake.  VTARGET is measured on the `ADC` pin and is informational — it
does not set the I/O voltage.

## Fuses, lock bits, erase and verify

All of these use the same `avrdude` mechanisms as flash; the personality
executes the ISP commands the host sends and has no fuse table of its own.

| Task | Command | Notes |
|---|---|---|
| write a fuse | `avrdude -c avrispmkII -P usb -p m328p -U lfuse:w:0x62:m` | `lfuse`, `hfuse`, `efuse` name the low/high/extended fuse byte; the value is the part's, not a universal one |
| read a fuse | `avrdude -c avrispmkII -P usb -p m328p -U hfuse:r:-:h` | reads and prints, writes nothing |
| write the lock bits | `avrdude -c avrispmkII -P usb -p m328p -U lock:w:0xFF:m` | `0xFF` is unlocked; locking is the last step of a production flow |
| chip erase | `avrdude -c avrispmkII -P usb -p m328p -e` | erases flash and EEPROM and clears the lock bits on parts that allow it |
| read back | `avrdude -c avrispmkII -P usb -p m328p -U flash:r:readback.hex:i` | writes the target's flash into a file |
| write and compare | `avrdude -c avrispmkII -P usb -p m328p -U flash:w:app.hex:i` | `avrdude` reads back and compares after writing; add `-V` to skip the comparison |

- **A wrong fuse can lock you out.**  Changing the clock source or disabling
  the reset pin can make the part stop answering ISP.  Read the fuse bytes
  first, change only the bit you mean, and keep a note of the original
  value.  A part whose reset pin is disabled needs a high-voltage programmer,
  which this personality is not.
- **`-e` is not an undo for a wrong fuse.**  Chip erase clears the memory and
  (on parts that allow it) the lock bits; it does not restore a fuse that
  disables ISP.
- **`-V` only skips the compare**, so a `:w:` write no longer proves the
  written image.  Use it only when you compare separately.
- **EEPROM** uses the same form with the `eeprom` memory name instead of
  `flash`.
- The personality does not compare anything itself; the host compares the
  data it reads back.

## Speed

The host selects the ISP clock through the STK500v2 `SCK_DURATION`
parameter (`0x98`); the value is a speed-ladder index, not a frequency.  The
engine uses the AVR069 index numbering (0–163): seven hardware steps are
tabulated below, and the remaining software steps are an approximate
ladder.  You do not need AVR069 for normal use; `-B` selects a period.

| Index | SCK frequency | Clock engine |
|---|---|---|
| 0 | 8 MHz | hardware SPI |
| 1 | 4 MHz | hardware SPI |
| 2 | 2 MHz | hardware SPI |
| 3 | 1 MHz | hardware SPI |
| 4 | 500 kHz | hardware SPI |
| 5 | 250 kHz | hardware SPI |
| 6 (default) | 125 kHz | hardware SPI |
| 7 … 163 | about 96 kHz down to about 51 Hz (approximate) | software |

- The seven hardware steps (indices 0–6, 8 MHz down to 125 kHz) use the
  PIC24 hardware SPI module.  The remaining 157 steps use blocking software
  clocking and are **approximate**: integer-microsecond delays plus firmware
  overhead vary with the selected rate.  The exact rate at a given software
  index is not tabulated here.
- `avrdude` picks the index from its `-B` option, which takes a period in
  microseconds.  The number is a **period**, not a frequency: **`-B 8`
  means 8 µs = 125 kHz** and **`-B 125` means 125 µs = 8 kHz**.  Use
  `-B 8` for 125 kHz.
- The parameter is a ladder index, not the period.  The default is 6
  (125 kHz); setting `02 98 06` writes the index directly.
- The speed is latched when `ENTER_PROGMODE_ISP` runs.  Changing the
  parameter later in the same session has no effect until the next enable.
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
- **No reset output drive on release.**  `RESET` is asserted at the
  host-selected polarity and tristated on release; a target that needs a
  driven reset release must have its own pull-up.
- **A failed enable leaves `RESET` asserted** until `LEAVE_PROGMODE_ISP`
  or the end of the mode.  Send `LEAVE` before retrying or rewiring; the
  full interface-change sequence is in [command-model.md](command-model.md)
  §Changing target interface.
- **No target power.**  See [wiring.md](wiring.md).
- **No stated minimum `avrdude` version.**  The personality implements the
  mkII command surface; whether the host knows a particular part is the
  host's own device database, not this personality.  A host that refuses a
  part or an XPROG target needs a newer host, not different wiring.

## ISP vs. the ordinary SPI mode

This is an ISP *programmer*: the host supplies the AVR programming opcodes
and the poll values, and the personality executes them.  It is unrelated to
the Bus Pirate's interactive SPI mode and to the host-side `buspirate` and
`buspirate_bb` interfaces, which remain available and are not changed by
this personality.  See [the SPI set](../../protocol/spi/README.md) for the
interactive mode.

## Related docs

- [entry.md](entry.md) — entry, deadline and the return to `BBIO1`.
- [wiring.md](wiring.md) — the ISP table and the electrical contract.
- [protocol.md](protocol.md) — commands, status codes, request shapes.
- [command-model.md](command-model.md) — the ISP session order.
- [sessions.md](sessions.md) — a worked ISP sequence.
- [rescue-clock-osccal.md](rescue-clock-osccal.md) — a target with no
  running clock, and AVR053 calibration.
- [edge-cases.md](edge-cases.md) — failure modes.
