# 1-Wire — macros and the device roster

Part of the 1-Wire documentation set (`docs/1wire/`).  This page owns the
mode macros: the `(0)` list, the ROM commands, and the discovered-device
roster.

## Scope

The 1-WIRE mode macros, invoked as `(n)`.  Shared user macros `<n>` are a
base-layer feature (see [commands.md](../generic/commands.md)) and are not
repeated here.

## The `(0)` list

`(0)` lists the mode's macros and, if a search has been run, the current
device roster.  The firmware prints, in order:

1. the macro-menu header ` 0.Macro menu`;
2. the table header `Macro     1WIRE address`;
3. the roster entries — one line each, `N.` followed by the 8 ROM bytes and
   the family-name lookup — or `No device, try (ALARM) SEARCH macro first` if
   the roster is empty;
4. the ROM command list (below).

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

These ROM command macros are documented from the firmware and are unverified;
`(51)` is shown worked in [bus-operations.md](bus-operations.md).

`(51)` on a single-device bus resets, sends `0x33`, and reads the 8 ROM bytes
back; the first byte is the family code, looked up against the table below:

| First ROM byte | Printed family |
|---|---|
| `0x10` | DS18S20 High Prec Dig Therm |
| `0x28` | DS18B20 Prog Res Dig Therm |
| `0x22` | DS1822 Econo Dig Therm |
| `0x04` | DS2404 EconoRAM time Chip |
| `0x2D` | DS2431 1K EEPROM |
| any other | Unknown device |

## The device roster

The roster is filled by a **successful search**: `(240)` (SEARCH ROM) and
`(236)` (ALARM SEARCH) walk the bus, print each device found, and save it so
it can be recalled later without searching again.  Running either search macro
again re-enumerates the bus and refills the roster.  The single-device `(51)`
(READ ROM) does **not** fill the roster.

The header the firmware prints is the string `Macro     1WIRE address`; the
trailer is:

```
Device IDs are available by MACRO, see (0).
```

Each row between them is `N.`, then the 8 ROM bytes, then the family-name
lookup.

The roster holds up to **10** devices.  If a search finds more than ten, they
are all printed, but only the first ten can be recalled; re-run the search to
re-enumerate the rest.

### Recalling a device — the address macros

After a search, the roster entries are invoked as macros `(1)` … `(10)`:

| Input | Behaviour |
|---|---|
| `(1)` … `(10)` (a filled slot) | prints `ADDRESS MACRO <n>: ` and the 8 ROM bytes, and writes those 8 bytes on the bus |
| any empty address slot (e.g. `(11)` … `(49)`) | prints `No device, try (ALARM) SEARCH macro first` |
| any other number | prints `Unknown macro, try ? or (0) for help` |

Three different number ranges share the `(n)` input:

- **`1`–`49`** are the address-macro slots that recall roster entries.  Only
  the first ten can ever hold a device (the roster size), so `1`–`10` fill up
  after a search and `11`–`49` stay empty.
- **`51`, `85`, `204`, `236`, `240`** are the ROM command macros listed above.
- **`0`** lists the macros and the current roster.

An address macro writes only the stored ROM bytes; it does **not** send a
leading `0x55` (MATCH ROM).  To address a specific device, send `(85)` (or a
plain `0x55`) first, then the address macro — or send the bytes by hand.

**Requires verification:** the number printed in the `ADDRESS MACRO <n>:`
line is one greater than the roster number shown for the same device by
`(0)`.  The first listed device is recalled with `(1)` but its line reads
`ADDRESS MACRO 2:`.  Use the roster order (or the ROM bytes on the line) to
identify the device, not the printed number.

Roster entries live only as long as the mode is active; leaving and re-entering
1-WIRE clears the roster (a fresh search is needed).

## v3 / v4

- The macro identifiers, the 10-slot roster and the printed strings are the
  same in both v3 and v4; v3 is **not verified**.
- The `(0)` list, `(85)`, `(204)`, `(236)`, `(240)`, the roster table, and the
  address macros are documented from the firmware and are unverified.