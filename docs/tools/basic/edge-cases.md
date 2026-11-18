# BASIC — edge cases and failure modes

Part of the BASIC documentation set (`docs/tools/basic/`).  Each entry is a
concrete trigger, the observable symptom, and the behaviour or recovery.
Ranges and build differences are in
[limits-and-variants.md](limits-and-variants.md).

## Entering and editing

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| a statement typed with no line number | `Syntax error` | add a line number to store it |
| a bare Enter at the BASIC prompt | `Syntax error` | type a numbered line or a command |
| a normal command inside BASIC (`#`, `$`, `s`, `m`, `w`, `?`) | `Syntax error` | `EXIT` first, then use the normal command |
| `RUN 10`, `LIST 10`, `EXIT now` | `Syntax error` | `RUN`/`LIST`/`NEW`/`EXIT` take no arguments |
| a leading space before a line number | `Syntax error` | the number must be the first character |
| the line number `65535` | `Syntax error` | it is reserved for the end marker; use `0`..`65534` |
| more than 31 statement bytes | `Too long!`; not stored | shorten the line; if it replaced a line, the old line is kept |
| no room left in the program area | `Too long!`; not stored | the stored program is unchanged; delete lines or `NEW` |
| `NEWTON`, `RUNAWAY`, `LISTING`, `EXITNOW` | `Syntax error` | command names must be complete words |
| `SAVE`, `LOAD`, `FORMAT`, `DEBUG` in the default image | `Syntax error` | these are build options that are off (the EEPROM filesystem also does not build); see [limits-and-variants.md](limits-and-variants.md) |
| lower-case text inside quotes | printed upper case | input is folded to upper case before parsing |
| a `"` inside a `REM` | `LIST` can differ from what was typed; the byte count can change | execution still skips the line; retype without the quote |
| a line number on its own | the line is deleted, silently | if the line did not exist, nothing happens |

## Running and stopping

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| a program stuck in an endless loop | no prompt; the terminal is ignored | press **Ctrl-C** to stop it between statements |
| Ctrl-C during a run | `Error(10) @line:L @pgmspace:P` | the run stops between statements; the program and variables are kept |
| Ctrl-C at an `INPUT` prompt | a bell; nothing else | `INPUT` ignores Ctrl-C; enter `0`..`32767` or press Enter |
| Ctrl-C during `DELAY` or a mode macro | no immediate effect | that step finishes first; a macro can wait for input |
| ordinary keys typed during a run | discarded | only Ctrl-C is read |
| 20 NULs, or 5–19 NULs plus Left arrow, during a run | discarded | the sequences work at the prompt, not during a run |
| assignment without `LET`, e.g. `10 A=1` | `Error(3) @line:10 @pgmspace:6` | use `LET A=1` |
| `GOTO`/`GOSUB` to a missing line | `Error(6)` | check the target; line numbers must match exactly |
| `NEXT` with no active `FOR` | `Error(5)` | match every `NEXT` to an active `FOR` with the same variable |
| a fifth nested `FOR` | `Error(4)` | at most 4 loops may be active |
| an eleventh nested `GOSUB` | `Error(7)` | at most 10 frames |
| `RETURN` with no `GOSUB` | `Error(8)` | remove the stray `RETURN` or add the call |
| `READ` past the last `DATA`, or no `DATA` at all | `Error(9)` | add `DATA` or `RESTORE`; the values do not wrap |
| a stored program damaged | `Error(2)` or `Error(3)` | `NEW`, then retype the program |
| an empty program followed by `RUN` | a blank line, then `Ready` | nothing to execute; not an error |

## Expressions and PRINT

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `PRINT 1,2` | prints `102`, then `Error(3)` | a comma is parsed as a value and sets a syntax error; use `;` |
| an unterminated quote in `PRINT` | the stored text, then `Error(3)` | add the closing quote |
| `PRINT` with no items | a blank line | this is the normal empty print |
| division by zero, e.g. `1/0` | `0`, no error | guard the divisor if zero is possible |
| `-32768 / -1` | `0`, no error | the one signed-overflow case is defined as zero |
| `PRINT` of a negative value | its unsigned form (`-1` prints `65535`) | interpret the value as signed when it matters |
| 16-bit overflow | the value wraps | arithmetic is 16-bit |
| `8/4/2` | `1`, not `4` | level-1 operators are evaluated left to right |
| `1=1+1` | `2`, not `0` | comparisons bind at level 2, left to right |
| `IF PULLUP THEN` | true when the pull-ups are **off** | the expression is the complement: `0` on, `1` off |
| `PRINT AUX;` after driving `AUX` | the pin is released and sampled | drive it again if it must stay an output |
| `ADC` while the SMPS owns the converter | `Error(3)` | stop the SMPS first; the terminal probe is refused too |
| `SEND "A"` | `Error(3)` | `SEND` takes a number, not a string |
| `SEND 0x7B` | sends `0`, then `Error(2)` | decimal only; the `x` is trailing text |
| `FREQ`/`DUTY` while the SMPS owns the PWM channel | no output, no error | stop the SMPS first |

## INPUT

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `INPUT , A` | `Error(3)` | there is no comma-only form; write `INPUT A` |
| `INPUT "TEXT" A` | `Error(3)` | the comma after the quoted prompt is required |
| `INPUT 1`, or `INPUT` with no variable | `Error(3)` | the target must be a single letter `A`..`Z` |
| a bare Enter at `(0)>` | stores `0` | `0` is the default |
| a value above `32767`, a negative value or six digits | `Invalid choice, try again`, then `(0)>` again | enter `0`..`32767`; the value is not clamped |
| Ctrl-C at `(0)>` | a bell | ignored; enter a value or press Enter |
| a second variable, e.g. `INPUT A, B` | `Error(2)` later on the line | one variable per `INPUT` |

## DATA and values

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `DATA -1` | reads `-1` | signed decimal literals are accepted |
| `DATA 1,` (trailing comma) | the first `READ` gets `Error(9)` | remove the trailing comma |
| `DATA A`, `DATA 1+2`, `DATA "A"` | `Error(9)` | `DATA` takes signed decimal literals only |
| `DATA 70000` | reads `4464` | the value wraps at 16 bits |
| `READ` past the last value | `Error(9)` | `RESTORE` before re-reading; there is no wrap |
| a `RESTORE` in a conditional branch | the next `READ` starts at the first `DATA` line | the pointer is program-wide, not per line |
| `DATA` lines after a jump | still found by `READ` | the scan follows stored order, not execution order |

## Loops and subroutines

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `FOR A=1 TO 3` | runs with `A` = 1, 2, 3 | the end value is included |
| `FOR A=3 TO 1` | the body runs once | a start above the end does not loop |
| `FOR A=1 TO 3 STEP 2` | `Error(3)` | there is no `STEP` |
| `NEXT` without a variable | `Error(5)` | name the loop variable |
| a second `NEXT` for a completed loop | `Error(5)` | the frame is popped when the loop finishes |
| `GOTO` out of a `FOR` | the loop frame stays active | a later `NEXT` for that variable re-enters the loop |
| `GOSUB` to a missing line | `Error(6)` | the frame is pushed first; the run ends on the error |
| `RETURN` in a conditional branch | returns to the line after the conditional `GOSUB` | the return address is the end of the calling line |

## MODE and the bus

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `MODE` with a number outside the menu | `Error(3)`; mode unchanged | use the `m` menu's number |
| `MODE` after setting `e`, `L`/`l`, `;N` or the output type | those settings are lost | set them at the terminal before `s` and avoid `MODE` |
| `MODE` after `PSU`, `PULLUP`, `AUXPIN` or `FREQ` | those settings are cleared | put `MODE` first, then re-apply them |
| a protocol operation with no handler | silent no-op; a read token returns `0` | use a mode that supports it |
| `START 1` (a no-argument statement with trailing text) | `Error(2)` | end the line after `START` |
| `CLK 3` | nothing happens | `CLK` accepts 0, 1 (level) and 2 (pulse) |
| `AUXPIN 2` on v3 | `Error(3)` at run time | v3 has only `A0` and `CS`; the line is stored without complaint |
| `AUXPIN 4`, or a negative value | `Error(3)`; target unchanged | 0..3 on v4, 0..1 on v3 |
| `MACRO` in HiZ, 3WIRE or DIO | silent no-op | those modes have no macros |
| an interactive mode macro | the program stops in the macro; its prompts are not visible | press the board's reset button or power-cycle; `#` and `EXIT` cannot be typed meanwhile |
| `FREQ 100050` | `Error(3)` | separate the frequency and duty: `FREQ 1000 50` |
| `FREQ` with a missing or third operand | `Error(3)` | exactly two values |
| `FREQ 5000 0` | 4000 Hz at 1 % | values are clamped, not rejected |
| the terminal `g10` and BASIC `FREQ 10` | 10 kHz and 10 Hz | the terminal is kHz; BASIC is Hz |
| `DUTY` with no PWM set up | no output | start with `FREQ` |
| PWM in HiZ | the pin is driven | board-level calls bypass the terminal HiZ guard |
| `DELAY -1` | a 65535 ms delay | arguments are used as unsigned 16-bit counts |
| driving `CS` while SPI uses it | the mode's chip-select level is lost | select `AUX` again or switch mode before returning to the terminal |

## Program lifetime

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `EXIT` then `s` again | the program is still there | `EXIT` keeps it; `NEW` erases it |
| `#` on v4 | the terminal resets; BASIC exits | the stored program stays in RAM; type `s` again |
| `#` on v3 | the board restarts | the stored program is gone |
| the SUMP exit, or the binary return on v4 | a terminal reset; BASIC exits | the stored program stays in RAM |
| the binary return on v3 | a hardware reset | the stored program is gone |
| `$` or a power cycle | the board restarts | the stored program is gone |
| `NEW` | the program is erased | the interpreter keeps running |
