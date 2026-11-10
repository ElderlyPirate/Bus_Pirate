# BASIC — edge cases & failure modes

Part of the BASIC documentation set (`docs/basic/`).  Each entry is a concrete
trigger, the observable symptom, and the behaviour or recovery.

## Entering lines

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| A statement typed with no line number, e.g. `PRINT "HI"` | `Syntax error` | only `RUN`/`LIST`/`NEW`/`EXIT` are commands at the prompt; put a line number in front to store a statement |
| A bare Enter (empty line) at the `(BASIC)>` prompt | `Syntax error` | unverified; type a numbered line or a command instead |
| A normal-mode command typed inside BASIC, e.g. `#`, `w`, `m`, `s` | `Syntax error` | `EXIT` first, then use normal commands |
| Lower-case text inside quotes | printed upper case | input is folded to upper case before parsing; type the intended case only if you can accept it |
| More than 31 statement bytes on one line | `Too long!`; the line is not stored | split the statement across lines |
| No room left in the program area for another line | `Too long!`; the line is not stored | `NEW` or delete unneeded lines to free space |
| The line number `65535` | end-of-program marker is overwritten | do not use `65535`; it is the interpreter's own marker (unverified) |

## Running

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Program stuck in an endless loop | no prompt; the interpreter ignores the terminal | press **Ctrl-C** to stop it |
| Ctrl-C during a run | `Error(10) @line:L @pgmspace:P` | the run stops between statements; the program is kept, fix it and `RUN` again |
| Assignment written without `LET`, e.g. `10 A=1` | `Error(3) @line:10 @pgmspace:P` | use `LET A=1` |
| `GOTO`/`GOSUB` to a line that does not exist | error 6 | check the target line number; lines are sorted on entry so the target must be stored |
| `NEXT` without a matching active `FOR` | error 5 | match every `NEXT` to a `FOR` with the same variable |
| More than 4 nested `FOR` loops | error 4 | reduce the nesting to at most 4 |
| More than 10 nested `GOSUB` calls | error 7 | flatten the calls or reduce the depth to at most 10 |
| `RETURN` with no `GOSUB` to return to | error 8 | remove the stray `RETURN` or add the matching `GOSUB` |
| `READ` with no `DATA` statement anywhere | error 9 | add a `DATA` line |
| Program memory damaged | error 2 or 3 | re-enter the program (`NEW`, then retype lines) |
| Empty program and `RUN` | finishes quietly, then `Ready` | nothing to execute; that is not an error |

The exact error text is `Error(N) @line:L @pgmspace:P` (see
[language.md](language.md) §Errors).

## Bus access

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Bus statement (`START`, `SEND`, …) in `HiZ` | no bus activity | `HiZ` has no protocol behind the statements; switch with `MODE` first |
| `MODE` given a number outside the menu | error 3 | the current mode is unchanged; use a number from the table in [bus-access.md](bus-access.md) |
| Read/write value looks bit-reversed | wrong byte | `SEND`/`RECEIVE` follow the mode's LSb/MSb setting; set the bit order for the mode before running |
| `FREQ`/`DUTY` given an out-of-range value | value silently changed | values are clamped to 0..4000 Hz and 1..100 % |

## DATA and values

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `READ` runs past the last `DATA` value | reading restarts at the first `DATA` line | `READ` cycles; use `RESTORE` to reset the position deliberately (wrapping behaviour is unverified) |
| Division by zero, e.g. `1/0` | result is `0`, no error | not a fault; guard the divisor in the program if a zero is possible |
| `INPUT` given a value above `32767` | value is limited to the accepted range | enter `0`..`32767` |

## Program lifetime

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Board reset or power cycle | program gone; empty program on boot | the default build is RAM-only; there is no save unless the v4 EEPROM file system is built in |
| `EXIT` then `s` again | program still present | `EXIT` keeps the program; `NEW` is what erases it |

## Unverified behaviours

The following are stated from the firmware and are **not verified**:

- v3 behaviour throughout this set (build-only, no v3 hardware here).
- `READ` wrapping back to the first `DATA` value after the last one.
- Using `65535` as a line number.
- A bare Enter at the `(BASIC)>` prompt producing `Syntax error`.
- The default-off `DEBUG` dump and the default-off v4 EEPROM file system
  (`SAVE`/`LOAD`/`FORMAT`).