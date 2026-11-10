# AVRISP-MKII — ISP path (ATtiny / ATmega)

Part of the AVRISP-MKII documentation set (`docs/programmers/avrisp/`).  This
page covers the **ISP** target interface; wiring is in [wiring.md](wiring.md),
the command set in [protocol.md](protocol.md), and the entry/exit lifecycle in
[entry.md](entry.md).

## Scope

- Covers: the `avrdude` ISP flow, host driver setup, fuses/lock bits,
  erase and verification, the clock-speed ladder, and ISP-specific limits.
- Does not cover: PDI ([pdi.md](pdi.md)), TPI ([tpi.md](tpi.md)), the CDC
  serial route ([serial-route.md](serial-route.md)), or the rescue clock
  ([rescue-clock-osccal.md](rescue-clock-osccal.md)).

## Entry

Send the USB entry byte `0x08` from `BBIO1`; the device re-enumerates as
`AVRISP mkII`.  The host-side snippet, the five-second deadline and the way
back out are owned by [entry.md](entry.md) — do not enter the mode and then
look up a command.

The USB route is the only route that can reach PDI and TPI.  The serial
route ([serial-route.md](serial-route.md)) is ISP-only.

## Use with avrdude

```
avrdude -c avrispmkII -P usb -p m328p -U flash:w:app.hex:i
```

`-p m328p` is the part (an ATmega328P); use the host tool's own name for the
target.  A command line that shows `<part>` or `<file>` is using
placeholders: replace them with real names, and in PowerShell drop the angle
brackets or the shell treats them as redirection.

A read-only signature check:

```
avrdude -c avrispmkII -P usb -p m328p -n -U signature:r:-:h
```

### Host driver and permissions

| Host | What the USB route needs |
|---|---|
| Windows | a libusb/WinUSB style driver bound to `03EB:2104` once (for example with the Zadig tool, choosing the `AVRISP mkII` interface).  The normal Bus Pirate serial driver is not touched. |
| Linux | a udev rule for `03EB:2104`, or permission to open the device (for example running as a user in the group the rule grants).  The repository's Bus Pirate rules cover the terminal device (`04d8:fb00`, plus the v3 FTDI `0403:6001`), so they do not apply here. |
| macOS | nothing extra — the device is opened through the system's USB stack. |

A udev rule that grants access to the mkII identity:

```
# /etc/udev/rules.d/99-avrisp-mkii.rules
SUBSYSTEM=="usb", ATTR{idVendor}=="03eb", ATTR{idProduct}=="2104", MODE="0666"
```

Reload the rules (`sudo udevadm control -R`) and re-plug the board.  The
serial route needs no extra driver on any host.

The programmer reports the target **VTARGET** and a firmware version to the
host as part of the normal mkII handshake.  VTARGET is measured on the ADC
pin and is informational — it does not set the I/O voltage.

## Fuses, lock bits, erase and verify

All of these use the same `avrdude` mechanisms as flash; the personality
executes the ISP commands the host sends and has no fuse table of its own.

| Task | Command | Notes |
|---|---|---|
| write a fuse | `avrdude -c avrispmkII -P usb -p m328p -U lfuse:w:0x62:m` | `lfuse`, `hfuse`, `efuse` name the low/mid/high fuse byte; the value is the part's, not a universal one |
| read a fuse | `… -U hfuse:r:-:h` | reads and prints, writes nothing |
| write the lock bits | `… -U lock:w:0xFF:m` | `0xFF` is unlocked; locking is the last step of a production flow |
| chip erase | `avrdude -c avrispmkII -P usb -p m328p -e` | erases flash and EEPROM and clears the lock bits on parts that allow it |
| read back | `… -U flash:r:readback.hex:i` | writes the target's flash into a file |
| verify | `… -U flash:w:app.hex:i` | a `:w:` write is **verified** after writing; add `-V` to skip the check |

- **A wrong fuse can lock you out.**  Changing the clock source or disabling
  the reset pin can make the part stop answering ISP.  Read the fuse bytes
  first, change only the bit you mean, and keep a note of the original
  value.  A part whose reset pin is disabled needs a high-voltage programmer,
  which this personality is not.
- **`-e` is not an undo for a wrong fuse.**  Chip erase clears the memory and
  (on parts that allow it) the lock bits; it does not restore a fuse that
  disables ISP.
- **`-V` disables the automatic verify**, so `:w:` no longer proves the
  written image.  Use it only when you verify separately.
- **EEPROM** uses the same form with the `eeprom` memory name instead of
  `flash`.

## Speed

The host selects the ISP SCK period through the STK500v2 `SCK_DURATION`
parameter (`0x98`).  The programmer implements the full AVR069 speed
ladder (indices 0–163); the table below is the complete mapping, so the
AVR069 document is provenance for the numbering rather than a required
lookup.

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
- **No stated minimum `avrdude` version.**  The personality implements the
  mkII command surface; whether the host knows a particular part is the
  host's own device database, not this personality.  A host that refuses a
  part or an XPROG target needs a newer host, not different wiring.

## ISP vs. the ordinary SPI mode

This is an ISP *programmer*: the host supplies the AVR programming opcodes
and the poll values, and the personality executes them.  It is unrelated to
the Bus Pirate's interactive SPI mode and to the host-side `buspirate` and
`buspirate_bb` interfaces, which remain available and are not changed by
this personality.

## Related docs

- [entry.md](entry.md) — entry, deadline and the return to `BBIO1`.
- [wiring.md](wiring.md) — the ISP table and the electrical contract.
- [protocol.md](protocol.md) — commands, status codes, transports.
- [rescue-clock-osccal.md](rescue-clock-osccal.md) — a target with no
  running clock, and AVR053 calibration.
- [edge-cases.md](edge-cases.md) — failure modes.
