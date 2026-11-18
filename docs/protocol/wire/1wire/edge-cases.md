# 1-Wire — edge cases & failure modes

Part of the [1-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  Each entry is a concrete trigger, the
observable symptom, and the recovery.

## Bus and reset

Reset warnings share the prefix `BUS RESET Warning: `; the distinguishing text
is quoted below.

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| No pull-up, VPU unpowered, or the line held low | `*Short or no pull-up` if the line stays low; `*No device detected` if it floats high | fit the pull-up and supply VPU (`e2`/`e3` on v4; external on v3); retry |
| Device unpowered or not on common ground | `*No device detected` | power the target; share ground |
| `r` read with the line never high | `READ: 0x00` | the all-zero read is a floating/low line, not a device value; check the pull-up |
| `]` or `}` used to close a transaction | `ERROR: command has no effect here` then `Syntax error at char 1` | 1-Wire has no stop handler; use `[` or `{` for the reset |
| A reset with no device on the bus | the warning is printed, but the mode stays usable | the reset is the detection step, not a fatal error; check wiring and retry |

## Power and pull-up

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `P` with nothing on VPU | `Pull-up resistors ON` then `Warning: no voltage on Vpullup pin` | supply VPU first (`e2`/`e3` on v4, external on v3), then re-run `P` |
| `e` on v3 | `Syntax error at char 1` | v3 has no `e` command; feed Vpullup externally |
| `e` while VPU already has a voltage | `Warning: already a voltage on Vpullup pin` + syntax error | the external source is already feeding VPU; drop the `e` attempt |
| Parasitic part with a pull-up above 2 kΩ | reads work, but a conversion or EEPROM write fails or returns bad data | use 2 kΩ or less, or power the part from a VCC pin |
| A part that needs a strong pull-up | no command can provide it; the line never drives high | add an external transistor/MOSFET pull-up switched by AUX, or power the part from VCC |

## Speed

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `O` typed outside `1WIRE` (or in HiZ) | `Command not used in this mode` | `O` is accepted only in `1WIRE` |
| Host standard, device in overdrive | reads/writes fail or are garbage | toggle `O` to overdrive to match; or toggle `O` to standard and issue a reset (`[`/`{`, at least 480 µs low); a power cycle also returns the device to standard |
| Overdrive device, mode re-entered | device overdrive, host standard (or vice versa) | use `O` to match the device instead of leaving; a standard reset returns the device to standard |
| `0x3C` sent, then a standard reset before `O` | the parts return to standard at that reset; the sequence simply restarts | send `O` before the next reset when you intend to stay in overdrive |

## Bit and byte operations

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `-^^^` expected to send four high bits | only three bits sent / off by one | `-`/`_` set the next `^` only; four high bits is `-^^^^` |
| `/` or `\` used to "clock" the bus | prints `CLOCK, 1` / `CLOCK, 0`, then `ERROR: command has no effect here` and a syntax error | no clock line; use `^` (`-`/`_` sets the bit) |
| `.` expected to read the bus | prints `DATA STATE: 0` or `1` from the stored value, and the next prompt runs on the same line | `.` reports the stored value, not the line; use `!` to read the bus |
| `.` after `-` still reports `1` on a low bus | the stored value, not the level | same as above; `!` is the bus read |
| `;16` or another width used | the echo shows the wider form, but the bus still transfers eight time slots | the width only changes the printed/echoed number; 1-Wire is always byte-wide |
| `r;16` | one byte is displayed as four hex digits (for example `READ: 0x0000`) | one byte is read; reset the width to 8 if the display is confusing |

## Macros and the roster

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `(n)` on an empty roster slot (`n` = 1…49) | `No device, try (ALARM) SEARCH macro first` | run `(240)`/`(236)` first, or use a found address |
| `(n)` not a ROM macro or roster slot (for example `(50)`) | `Unknown macro, try ? or (0) for help` | use `(0)` to list the valid macros |
| `(51)` READ ROM with multiple devices | ROM bytes are garbled (devices drive the line together) | READ ROM is single-device; use `(240)` and the roster/matching instead |
| More than ten devices found | all printed, only the first ten recallable | address the rest by their printed bytes; a re-run stores the same first ten |
| Search with no device on the bus | the name, table header and trailer print with no entries | the failed search also clears a previous roster; check wiring and re-run |
| A device's ROM fails the search CRC | the search stops silently at that device; the trailer still prints | the roster keeps what was found before the failure; check the bus and re-run |
| Address macro expected to send MATCH ROM too | only the eight ROM bytes are written | send `(85)` (or `0x55`) first, then the address macro |
| `(51)` expected to fill the roster | `(0)` still says no device | READ ROM does not save a device; use `(240)`/`(236)` |
| Roster expected to survive a mode change | no device from address macros after re-entry | the roster clears on entry; re-run the search |

## Binary path

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `0x02` treated as "device found" | always `0x01` | the reply only acknowledges the reset; use the terminal reset messages or read the bus |
| `0x22`/`0x23` sent to set a speed | `0x00` and the speed is unchanged | use `0x20` (standard) or `0x21` (overdrive); the low two bits select the speed and 2/3 are out of contract |
| Binary search expected to fill the roster | `(0)` is unaffected | the stream is read-only; the roster is a terminal feature |
| Fewer than `n + 1` data bytes sent | the next request byte is consumed as bus data | send exactly `n + 1` bytes |
| `0x5x` sent on v3 | `0x00` | the pull-up feed is v4 only; feed VPU externally |
| Bit operations expected over the binary link | no commands exist | drive the byte stream from the terminal or build the slots from bulk writes |

## Unverified paths

- The 1-Wire **failure** outputs (`*No device detected`, `*Short or no
  pull-up`) are the firmware's messages and are unverified.
- All search/roster macros and the `1W01` binary sub-protocol are
  **unverified**.
