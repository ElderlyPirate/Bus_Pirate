# 1-Wire — macros and the device roster

Part of the [1-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  This page owns the mode macros: the `(0)`
list, the ROM commands, and the discovered-device roster.

## Scope

The 1-Wire mode macros, invoked as `(n)`.  Shared user macros `<n>` are a
base-layer feature (see [macros.md](../../../generic/macros.md)) and are not
repeated here.

## The `(0)` list

`(0)` lists the mode's macros and, if a search has been run, the current
device roster.  With an empty roster:

```
1WIRE>(0)
 0.Macro menu
Macro     1WIRE address
No device, try (ALARM) SEARCH macro first
1WIRE ROM COMMAND MACROs:
 51.READ ROM (0x33) *for single device bus
 85.MATCH ROM (0x55) *followed by 64bit address
 204.SKIP ROM (0xCC) *followed by command
 236.ALARM SEARCH (0xEC)
 240.SEARCH ROM (0xF0)
```

The `(0)` list prints, in order: the ` 0.Macro menu` header, the
`Macro     1WIRE address` table header (five spaces), the roster entries or the
`No device, …` line, then the ROM command list.  It does **not** print the
search trailer.

## ROM command macros

| Macro | Name | ID byte | Behaviour |
|---|---|---|---|
| `(51)` | READ ROM | `0x33` | reset, send `0x33`, read 8 ROM bytes; single-device bus only |
| `(85)` | MATCH ROM | `0x55` | reset, send `0x55`; you follow it with the 64-bit address |
| `(204)` | SKIP ROM | `0xCC` | reset, send `0xCC`; you follow it with the device command |
| `(236)` | ALARM SEARCH | `0xEC` | enumerate devices in alarm state; fills the roster |
| `(240)` | SEARCH ROM | `0xF0` | enumerate every device; fills the roster |

The list the firmware prints verbatim:

```
1WIRE ROM COMMAND MACROs:
 51.READ ROM (0x33) *for single device bus
 85.MATCH ROM (0x55) *followed by 64bit address
 204.SKIP ROM (0xCC) *followed by command
 236.ALARM SEARCH (0xEC)
 240.SEARCH ROM (0xF0)
```

`(51)` resets, sends `0x33`, and reads the 8 ROM bytes back.  The first byte is
the family code, looked up against this table:

| First ROM byte | Printed family |
|---|---|
| `0x10` | DS18S20 High Prec Dig Therm |
| `0x28` | DS18B20 Prog Res Dig Therm |
| `0x22` | DS1822 Econo Dig Therm |
| `0x04` | DS2404 EconoRAM time Chip |
| `0x2D` | DS2431 1K EEPROM |
| any other | Unknown device |

Each printed byte uses the current display format (`o`), hex by default.
`(51)` does **not** check the ROM's CRC byte and does **not** fill the roster.

```
1WIRE>(51)
BUS RESET  OK
READ ROM (0x33): 0x28 0x23 0x45 0x67 0x89 0xAB 0x0C 0x7D
DS18B20 Prog Res Dig Therm
```

The ROM values are examples.  `(51)` is for a **single-device bus**; with more
than one device answering, the ROM bytes are garbled.

## The device roster

The roster is filled by a **successful search**: `(240)` (SEARCH ROM) and
`(236)` (ALARM SEARCH) walk the bus, print each device found, and save it so it
can be recalled later without searching again.  Running either search macro
again re-enumerates the bus and refills the roster.

The search prints its name, then the table header, then one **two-line** entry
per device, then the trailer:

```
1WIRE>(240)
SEARCH (0xF0)
Macro     1WIRE address
 1.0x28 0x23 0x45 0x67 0x89 0xAB 0x0C 0x7D
   *DS18B20 Prog Res Dig Therm
 2.0x10 0x11 0x22 0x33 0x44 0x55 0x66 0x77
   *DS18S20 High Prec Dig Therm
Device IDs are available by MACRO, see (0).
```

Shapes to note:

- Every entry line begins with a space, then the 1-based index, a `.`, and the
  eight ROM bytes, each printed in the current display format.
- Each ROM line ends with a space after the last byte.
- The family lookup is on its own line, indented three spaces and prefixed
  `*`; it is the same table as `(51)`.
- The trailer is the literal line `Device IDs are available by MACRO, see (0).`
- The values above are examples.

The roster holds up to **10** devices.  If a search finds more than ten, they
are all printed, but only the first ten are stored and recallable as address
macros; the rest can be addressed by their printed bytes.

A search that fails at the reset (no device or a short) prints the name, the
header and the trailer with no entries, and **clears any previous roster**.  A
ROM that fails the search CRC ends the enumeration silently at that device;
the trailer still prints.

### Recalling a device — the address macros

After a search, the roster entries are invoked as macros `(1)` … `(49)`:

| Input | Behaviour |
|---|---|
| `(1)` … `(10)` (a filled slot) | prints `ADDRESS MACRO`, the slot number, then `:` and the 8 ROM bytes, and writes those 8 bytes on the bus |
| any empty address slot (for example `(11)` … `(49)`) | prints `No device, try (ALARM) SEARCH macro first` |
| any other number (for example `(50)`) | prints `Unknown macro, try ? or (0) for help` |

Three different number ranges share the `(n)` input:

- **`1`–`49`** are the address-macro slots.  Only the first ten can ever
  hold a device (the roster size), so `1`–`10` fill up after a search and
  `11`–`49` stay empty.
- **`51`, `85`, `204`, `236`, `240`** are the ROM command macros listed above.
- **`0`** lists the macros and the current roster.

The number printed after `ADDRESS MACRO` is the **same** number used to
recall the device; roster entry 1 is `(1)` and prints `ADDRESS MACRO 1:`.

```
1WIRE>(1)
ADDRESS MACRO 1: 0x28 0x23 0x45 0x67 0x89 0xAB 0x0C 0x7D
```

An address macro writes only the stored ROM bytes; it does **not** send a
leading `0x55` (MATCH ROM).  To address a specific device, send `(85)` (or a
plain `0x55`) first, then the address macro — or send the bytes by hand:

```
1WIRE>(85)
BUS RESET  OK
MATCH ROM (0x55)
1WIRE>(1)
ADDRESS MACRO 1: 0x28 0x23 0x45 0x67 0x89 0xAB 0x0C 0x7D
```

Roster entries live only as long as the mode entry: leaving and re-entering
1-Wire clears the roster, and a fresh search is needed.  `O`, `W` and `w` do
not clear it.

## Unverified paths

The `(0)` list, `(85)`, `(204)`, `(236)`, `(240)`, the roster table, and the
address macros are **unverified**.
