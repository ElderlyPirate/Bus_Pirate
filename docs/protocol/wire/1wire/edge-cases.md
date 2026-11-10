# 1-Wire — edge cases & failure modes

Part of the 1-Wire documentation set (`docs/1wire/`).  Each entry is a
concrete trigger, the observable symptom, and the recovery.

## Bus and reset

Rows below quote only the distinguishing text: the reset warnings share the
prefix `BUS RESET Warning: `; the `P`-command warning has no prefix.

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| No pull-up, or VPU unpowered | `*No device detected` | fit the pull-up; put a voltage on VPU; retry |
| VPU unpowered while `P` runs | `Warning: no voltage on Vpullup pin` | supply VPU (`e2`/`e3` on v4; external on v3) |
| Data line shorted low, or pull-up weak/missing | `*Short or no pull-up` | clear the short or fit the pull-up; retry |
| Device unpowered or not on common ground | `*No device detected` | power the target; share ground |
| `r` read with the line never high | `READ: 0x00` | the all-zero read is a floating/low line, not a device value; check the pull-up |

## Speed

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `O` typed outside 1-WIRE (or in HiZ) | `Command not used in this mode` | `O` is accepted only in 1-WIRE |
| Host standard, device in overdrive | reads/writes fail or are garbage | device overdrive lasts until its power cycle; match or power-cycle |
| Overdrive device, mode re-entered | device overdrive, host standard (or vice versa) | use `O` to match the device instead of leaving |

## Bit and byte operations

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `-^^^` as four high bits (pre-5.2) | only three bits sent / off by one | since v5.2 use `-^^^^`; `-`/`_` sets the next `^` |
| `/` or `\` used to "clock" the bus | prints `CLOCK, 1` / `CLOCK, 0`; no bus change | no clock line; use `^` (`-`/`_` sets the bit) |
| `.` expected to touch the bus | prints `DATA STATE: 0` / `1` and returns | `.` reads the stored value only; no bus drive |

## Macros and the roster

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `(n)` on an empty roster slot (`n` = 1…49) | `No device, try (ALARM) SEARCH macro first` | run `(240)`/`(236)` first, or use a found address |
| `(n)` not a ROM macro or roster slot | `Unknown macro, try ? or (0) for help` | use `(0)` to list the valid macros |
| `(51)` READ ROM with multiple devices | ROM bytes are garbled (devices drive the line together) | READ ROM is single-device; use `(240)` and the roster/matching instead |
| More than ten devices found | all printed, only first ten recallable | re-search for the rest, or address by hand |
| Address number ≠ `(0)` roster number | first device is `(1)`, line reads `ADDRESS MACRO 2:` | Requires verification; trust the roster order/ROM bytes, not the printed number |
| Roster expected to survive a mode change | no device from address macros after re-entry | roster clears on re-entry; re-run the search |

## Setup

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `e` with VPU already powered | `Warning: already a voltage on Vpullup pin` + syntax error | the external supply fixes the source; drop the `e` attempt |

## Unverified paths

- The 1-Wire **failure** outputs (`*No device detected`, `*Short or no
  pull-up`) are the firmware's messages, and are unverified.  The no-pull-up
  `READ: 0x00` symptom is likewise unverified.
- All search/roster macros and the `1W01` binary sub-protocol are documented
  from the firmware and are unverified.
- v3 behaviour throughout this set is build-only and **not verified**.