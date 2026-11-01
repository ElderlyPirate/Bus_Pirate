# 1-Wire — edge cases & failure modes

Part of the 1-Wire documentation set (`docs/1wire/`).  Each entry is a
concrete trigger, the observable symptom, and the recovery.

## Bus and reset

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| No pull-up, or VPU unpowered | `BUS RESET Warning: *No device detected`; `P` also prints `Warning: no voltage on Vpullup pin` | fit the pull-up and put a voltage on VPU (`e2`/`e3` on v4, external on v3), then retry the reset |
| Data line shorted low, or pull-up missing/too weak | `BUS RESET Warning: *Short or no pull-up` | clear the short or fit the pull-up; retry the reset |
| Device unpowered or not on the same ground | `BUS RESET Warning: *No device detected` | power the target and share ground |
| Reading with `r` when the line never goes high | `READ: 0x00` | the all-zero read is the floating/low line, not a device value — check the pull-up |

## Speed

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| `O` typed outside 1-WIRE (or in HiZ) | `Command not used in this mode` | `O` is accepted only in 1-WIRE |
| Host toggled back to standard while the device is still in overdrive | reads/writes return garbage or fail | the device stays in overdrive until **its** power is cycled; either keep the host in overdrive or power-cycle the target |
| Device put into overdrive, then the mode left and re-entered | device answers at overdrive, host at standard (or vice versa) | use `O` to match the device rather than leaving the mode |

## Bit and byte operations

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| `-^^^` expected to send four high bits (pre-v5.2 style) | only three bits are sent, or the sequence is off by one | since v5.2 `-`/`_` only set the value for the **next** `^`; use `-^^^^` |
| `/` or `\` used to "clock" the bus | prints `CLOCK, 1` / `CLOCK, 0` but nothing changes on the bus | 1-Wire has no discrete clock line; use `^` (with `-`/`_` to set the bit) |
| `.` expected to touch the bus | prints `DATA STATE: 0` / `1` and returns | `.` only reads the stored value; it does not drive the bus |

## Macros and the roster

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| `(n)` for an empty roster slot (`n` = 1…49) | `No device, try (ALARM) SEARCH macro first` | run `(240)` or `(236)` first, or use an address that was found |
| `(n)` for a number that is not a ROM macro or roster slot | `Unknown macro, try ? or (0) for help` | use `(0)` to list the valid macros |
| `(51)` READ ROM on a bus with more than one device | the ROM bytes are garbled (devices drive the line together) | READ ROM is single-device only; use `(240)` and the roster/matching instead |
| More than ten devices found by a search | all are printed, but only the first ten are recallable | re-search to reach the rest, or address devices by hand |
| Address-macro line number does not match the `(0)` roster number | the first listed device is `(1)` but the line reads `ADDRESS MACRO 2:` | Requires verification (firmware-derived, not captured); trust the roster order / ROM bytes, not the printed number |
| Roster expected to survive a mode change | address macros report no device after re-entering | the roster is cleared when 1-WIRE is entered; re-run the search |

## Setup

| Trigger | Observable symptom | Behaviour / recovery |
|---|---|---|
| `e` used while VPU is already powered | `Warning: already a voltage on Vpullup pin` + syntax error | the source is fixed by the external supply; remove the `e` attempt |
| No 1-Wire target on the bench | reset reports no device | the current bench rig is wired for other modes; a 1-Wire capture needs a target (or an emulated slave) plus the pull-up |

## Unverified paths

- The 1-Wire **failure** outputs (`*No device detected`, `*Short or no
  pull-up`) are the firmware's messages; there is no bench capture of the
  1-Wire failure path yet.  The no-pull-up `READ: 0x00` symptom is likewise
  derived, not captured.
- All search/roster macros and the `1W01` binary sub-protocol are documented
  from the firmware, not from a bench run.
- v3 behaviour throughout this set is build-only — **not verified on this
  bench**.