# I2C — macros

Part of the I2C documentation set (`docs/i2c/`).  This page owns the mode
macro list and the on-board EEPROM macros.  The address scanner is described
in full in [bus-operations.md](bus-operations.md); the sniffer in
[sniffer.md](sniffer.md); the EEPROM story in [eeprom.md](eeprom.md).

## The `(0)` list

`(0)` prints the macro menu (the v4 wording):

```
 0.Macro menu
 1.7bit address search
 2.I2C sniffer
 3.Connect to on-board EEPROM
 4.Enable Writing the on-board EEPROM
```

On a v3 build the same menu lists only `0`–`2`.

| Macro | Description | Where |
|---|---|---|
| `(0)` | macro menu | both |
| `(1)` | 7-bit address search — find every responding device | both |
| `(2)` | I2C sniffer — listen to all traffic | both |
| `(3)` | connect to the on-board EEPROM | **v4 only** |
| `(4)` | enable writing the on-board EEPROM (clear its write protect) | **v4 only** |
| any other | `Unknown macro, try ? or (0) for help` | both |

Macros 3 and 4 are compiled into the v4 build only.  An older published I2C
page listed only macros 1 and 2 and called macro 2 "I2C snooper"; the firmware
name is **I2C sniffer** and there are four macros on v4.

## Macro 1 — 7-bit address search

See [bus-operations.md](bus-operations.md) §Address scanner for the full
behaviour and output format.

## Macro 2 — I2C sniffer

See [sniffer.md](sniffer.md).

## Macro 3 — connect to the on-board EEPROM (v4)

The Bus Pirate v4 carries a small (about **8 kB**) EEPROM-style I2C flash chip
on the board, wired to its own internal I2C bus — not to the external SDA/SCL
pins.  Macro 3 switches the active interface to that internal bus:

```
I2C>(3)
Now using on-board EEPROM I2C interface
I2C>
```

After this, start/stop/read/write go to the on-board EEPROM (7-bit address
`0x50`) instead of the external bus, until the Bus Pirate is reset.

The internal bus is driven by the on-chip hardware I2C engine, so this macro
belongs with the **Hardware** driver; in Software driver mode the bit-bang
path stays pointed at the external MOSI/CLK pins.

## Macro 4 — enable writing the on-board EEPROM (v4)

The on-board EEPROM is write-protected by default.  Macro 4 clears that
protect:

```
I2C>(4)
On-board EEPROM write protect disabled
I2C>
```

- Macro 4 **does nothing** (no message, no state change) unless macro 3 was run
  first.
- The protect is re-asserted by a reset; run macro 3, then macro 4, before any
  EEPROM write in a new session.

## Evidence

Macros 3 and 4 were **not exercised** on this bench; their strings, routing and
the write-protect line are read from the firmware — verify on a v4 with a
populated on-board EEPROM.  The macro menu wording, macro 1 and macro 2 are
present in the source; the menu itself was **not captured** on this bench.  A
capture would need the mode entered and `(0)` typed, with the on-board-EEPROM
checks run on v4 hardware.

## v3 / v4

| Macro | v3 | v4 |
|---|---|---|
| `(0)`, `(1)`, `(2)` | present | present |
| `(3)`, `(4)` | absent | present (documented, not exercised here) |

Running `(3)` or `(4)` on a v3 build falls to the unknown-macro message, since
those cases are compiled out.  The v3 column is read from the source — **not
verified on this bench**.