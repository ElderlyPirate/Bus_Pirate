# BASIC (script engine)

**Status:** included in the default v3 and v4 images.  The v3 behaviour is
not verified on v3 hardware.

BASIC is the on-board interpreter entered with `s`.  A program is typed at the
prompt, stored in RAM, and run by the Bus Pirate itself, so a bus sequence can
repeat without a host driving every step.  The interpreter can drive the
active bus mode, control the board-level pins, read values into the program,
and print results to the terminal.

The firmware uses two names for the same feature: `?` help lists `s` as
`Script engine`, and the prompt marks the interpreter with `(BASIC)`.  This set
calls it the **BASIC interpreter**.  The prompt is the active mode name with
the marker inserted: `HiZ(BASIC)>`, `SPI(BASIC)>`, `I2C(BASIC)>`.

This README is a map, not the manual; the details live in the files below.

**Start here:** first program →
[examples.md §Hello, LIST and EXIT](examples.md#hello-list-and-exit); line
model → [entry-and-prompt.md](entry-and-prompt.md); language →
[language.md](language.md); bus session →
[bus-access.md §Protocol operations](bus-access.md#protocol-operations).

| File | Covers |
|---|---|
| [entry-and-prompt.md](entry-and-prompt.md) | `s`, `EXIT`, the prompt, line entry and editing, `RUN`/`LIST`/`NEW`, the byte count, program lifetime |
| [language.md](language.md) | statements, expressions, operators, variables, `DATA`/`READ`/`RESTORE`, `GOTO`/`GOSUB`, `FOR`/`NEXT`, errors |
| [bus-access.md](bus-access.md) | `MODE` and its reset contract, mode numbers, protocol and bit-bang statements, `MACRO`, mode wiring |
| [board-pins.md](board-pins.md) | `PSU`, `PULLUP`, `AUX`/`AUXPIN`, `FREQ`/`DUTY`, `ADC`, `TIME`, `DELAY`, HiZ safety |
| [examples.md](examples.md) | worked programs and sessions |
| [limits-and-variants.md](limits-and-variants.md) | size and loop limits, unavailable features, v3/v4 differences |
| [edge-cases.md](edge-cases.md) | concrete traps and failure modes |

## The five-minute version

- Enter with `s` at any mode prompt.  The prompt gains `(BASIC)`; `EXIT`
  returns to the normal prompt.  The mode does not change.
- A line that starts with a number is stored; retyping the number replaces it,
  and a number alone deletes it.  `RUN` executes, `LIST` prints, `NEW` erases.
- Language: `LET`, `IF`/`THEN`/`ELSE`, `GOTO`, `GOSUB`/`RETURN`, `FOR`/`NEXT`,
  `PRINT`, `INPUT`, `DATA`/`READ`/`RESTORE`, `REM`, `END`.  One statement per
  line; variables are `A`..`Z`, signed 16-bit; there are no arrays, no string
  variables and no user functions.
- Bus: `MODE n` switches mode; `START`/`STARTR`, `STOP`/`STOPR`, `SEND`,
  `RECEIVE`, `BITREAD`, `CLK`, `DAT` and `MACRO` use the active mode.  The
  board-level set is `PSU`, `PULLUP`, `AUX`, `AUXPIN`, PWM `FREQ`/`DUTY`,
  `DELAY`, `ADC` and `TIME`.
- Ctrl-C stops a running program between statements.  It is ignored inside
  `INPUT`, a `DELAY` and an interactive mode macro.
- Limits (default build): 1 KB program area, 31 statement bytes per line,
  4 nested `FOR` loops, 10 `GOSUB` frames, PWM off or 1–4000 Hz.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| interpreter (`s`) | included in the default image | included |
| mode numbers used by `MODE` | `PIC` 8, `DIO` 9 | `KEYB` 8, `PIC` 9, `DIO` 10 |
| `AUXPIN` targets | `A0`, `CS` | `A0`, `CS`, `A1`, `A2` |
| on-board Vpullup source (`e`) | — | terminal only; `MODE` clears it |
| on-board program storage | — | build option, off and not buildable |
| raw `DEBUG` dump | build option, off | build option, off |

`SAVE`/`LOAD`/`FORMAT` are not available: the optional on-board EEPROM
filesystem is disabled and does not compile in the current tree.  There is no
program persistence.  `DEBUG` is off in the default images; a custom build can
enable the raw dump.

## Wiring (headline)

BASIC adds no wiring of its own.  The bus statements use the active mode's
pins, pull-ups and supply; the board-level statements use the AUX, Vpullup and
ADC pins directly.

The default setup that `MODE` applies is open drain for every bus mode except
`DIO`, so those modes need a pull-up and a voltage on **Vpullup** unless the
target provides its own.  `PULLUP 1` enables the pull-ups but does not create
the Vpullup voltage.  Summary tables:
[bus-access.md §Wiring per mode](bus-access.md#wiring-per-mode) and
[board-pins.md §Pins and rails](board-pins.md#pins-and-rails).  The per-mode
sets are linked from the mode table in
[generic/README.md](../../generic/README.md).

## Limits & out of scope

- No arrays, no string variables or string expressions (string literals are
  allowed in `PRINT` and `INPUT`), no user-defined or built-in numeric
  functions (`ABS`, `INT`, `RND`), no `AND`/`OR` keywords, no `STEP`, no
  multi-statement lines.
- No hexadecimal or binary literals inside a stored program; decimal only.
- The program area holds one program; there is no file system, no slot and no
  persistence.  A power cycle clears it.
- No binary sub-protocol or host API of its own.  The 20-NUL binary entry and
  the SUMP entry sequence are handled outside the interpreter and leave it.
- Terminal commands with no BASIC token include `f` (frequency counter), `v`
  (status), `~` (self-test), `O` (1-Wire overdrive) and `o` (display mode).
  Use them before `s` or after `EXIT`.
- PWM `FREQ`/`DUTY` reach 4000 Hz; the terminal `g` works in kHz and reaches
  higher.  See [board-pins.md](board-pins.md#pwm-freq-and-duty).

## Related docs

| Page | Why |
|---|---|
| [../../SPEC.md](../../SPEC.md) | function and capability index |
| [../../generic/README.md](../../generic/README.md) | prompt, mode menu, supply and pull-up basics |
| [../../generic/commands.md](../../generic/commands.md) | shared command grammar and AUX target names |
| [bus-access.md](bus-access.md) | `MODE`, mode numbers and the bus statements |
| [board-pins.md](board-pins.md) | board-level pins and rails |
| [../../protocol/binary/README.md](../../protocol/binary/README.md) | the 20-NUL binary entry |
| [../../protocol/sump/README.md](../../protocol/sump/README.md) | the SUMP entry and exit |
| [../../capabilities/auxiliary/README.md](../../capabilities/auxiliary/README.md) | AUX targets, PWM and frequency |
| [../../capabilities/adc/README.md](../../capabilities/adc/README.md) | the ADC probe and its scale |
