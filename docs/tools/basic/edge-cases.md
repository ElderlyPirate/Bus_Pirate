# BASIC — edge cases & failure modes

Part of the BASIC documentation set (`docs/tools/basic/`).  Each entry is a
concrete trigger, the observable symptom, and the behaviour or recovery.

## Entering lines

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| A statement typed with no line number | `Syntax error` | add a line number to store a statement |
| A bare Enter at the `(BASIC)>` prompt | `Syntax error` | type a numbered line or a command |
| A normal-mode command inside BASIC (`#`, `$`, `w`, `m`, `s`) | `Syntax error` | `EXIT` first, then use the normal command |
| Lower-case text inside quotes | printed upper case | input is folded to upper case before parsing |
| More than 31 statement bytes on one line | `Too long!`; not stored | split it; a replaced old line is removed too — shorten and retype |
| No room left in the program area | `Too long!`; not stored | `NEW` or delete lines; a replacement removes the old line |
| The line number `65535` | end-of-program marker overwritten | do not use `65535`; it is the interpreter's marker (unverified) |

## Running

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Program stuck in an endless loop | no prompt; terminal ignored | press **Ctrl-C** to stop it between statements |
| Ctrl-C during a run | `Error(10) @line:L @pgmspace:P` | run stops between statements; program kept |
| Ctrl-C in `INPUT`, `DELAY` or a mode macro | no immediate effect | only polled between statements; finish that step first |
| Assignment without `LET`, e.g. `10 A=1` | `Error(3) @line:10 @pgmspace:P` | use `LET A=1` |
| `GOTO`/`GOSUB` to a missing line | error 6 | check the target; lines are sorted on entry |
| `NEXT` without a matching active `FOR` | error 5 | match every `NEXT` to a `FOR` with the same variable |
| More than 4 nested `FOR` loops | error 4 | reduce the nesting to at most 4 |
| More than 10 nested `GOSUB` calls | error 7 | flatten the calls or reduce the depth to at most 10 |
| `RETURN` with no `GOSUB` | error 8 | remove the stray `RETURN` or add the matching `GOSUB` |
| `READ` with no `DATA` statement | error 9 | add a `DATA` line |
| Program memory damaged | error 2 or 3 | `NEW`, then retype the program |
| Empty program and `RUN` | finishes quietly, then `Ready` | nothing to execute; not an error |

The exact error text is `Error(N) @line:L @pgmspace:P` (see
[language.md](language.md) §Errors).

## Bus access

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Protocol operation with no handler, or in `HiZ` | no bus activity | silent no-op; use a mode that supports it ([bus-access.md](bus-access.md)) |
| `MODE` given a number outside the menu | error 3 | mode unchanged; use a table number |
| `MODE` after setting `e`, `L`/`l` or `;N` | those settings are lost | `MODE` resets them; set before `s` and avoid `MODE` |
| Read/write value looks bit-reversed | wrong byte | set the bit order at the terminal before `s`; `MODE` resets it |
| `PULLUP` used in an expression | `65534` or `65535`, never `1`/`0` | always non-zero; read the state with `i` in a non-HiZ mode |
| Board-level operation in `HiZ` | it still acts | direct hardware calls; not refused like `W`/`P`/`g` |
| `AUXPIN` given a value above `1` | `A1`/`A2` not selectable | coerced to on/off; only `A0` and `CS` are reachable |
| `FREQ`/`DUTY` used | the program stops with an error | not usable; use the terminal `g` |

## DATA and values

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `READ` runs past the last `DATA` value | reading restarts at the first `DATA` line | `READ` cycles; `RESTORE` resets deliberately |
| Division by zero, e.g. `1/0` | result is `0`, no error | guard the divisor if zero is possible |
| `INPUT` above `32767` or negative | `Invalid choice, try again` | prompts again; enter `0`..`32767` (not clamped) |
| `DATA -1` or an expression in `DATA` | `READ` stores `0`, keeps returning `0` | use non-negative decimals or simple variables |

## Program lifetime

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Board reset or power cycle | program gone; empty on boot | RAM-only; the EEPROM option is disabled |
| `EXIT` then `s` again | program still present | `EXIT` keeps it; `NEW` erases it |

## Unverified behaviour

- Using `65535` as a line number.
