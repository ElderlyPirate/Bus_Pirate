# BASIC — limits and v3 / v4 variants

Part of the BASIC documentation set (`docs/tools/basic/`).  This page owns the
memory and language limits, the features that are not available, and the
differences between the two boards.  Failure behaviour when a limit is hit is
in [edge-cases.md](edge-cases.md).

## Scope

How much a program can hold and how the two builds differ.  The statement
semantics are in [language.md](language.md); the bus ranges are in
[bus-access.md](bus-access.md) and [board-pins.md](board-pins.md).

## Limits

The values below are those of the default build.  The program area, loop table
and stack are build-time settings, so a custom build can differ.

| Item | Default |
|---|---|
| program area | 1024 bytes |
| statement bytes per line | up to 31; a longer line is rejected |
| variables | `A`..`Z`, 26 signed 16-bit values |
| active `FOR` loops | 4 |
| `GOSUB` call depth | 10 frames |
| line numbers | decimal `0`..`65534`; `65535` is the end-of-program marker |
| `INPUT` value range | `0`..`32767` |
| `DATA` values | signed 16-bit decimals; a value outside the range wraps |
| `DELAY` | `0`..`65535` ms |
| PWM (`FREQ`/`DUTY`) | off (`0`) or `1`..`4000` Hz; duty `1`..`100` % |
| ADC reading | raw `0`..`1023` (about 6.45 mV per count) |
| `TIME` resolution | 64 µs per unit; wraps about every 4.19 s |
| `SEND`/`RECEIVE` value | 16-bit in the language; the mode's width reaches the bus |

The program area holds one program.  A program plus its line headers and the
four-byte end marker must fit; when no room is left, storing another line is
rejected with `Too long!`.  A rejected line leaves the stored program
unchanged.

A **statement byte** is one byte of the stored line.  A keyword is stored as a
single byte however it is spelled (`PRINT` is one byte, not five); every other
character — digit, punctuation or quoted text — counts as one byte.  Spaces
outside quotes and outside a `REM` are dropped before storage and do not
count; a `REM`'s own text counts, spaces included.

The count that `LIST` prints is one less than the stored size; the worked
breakdown is in [entry-and-prompt.md §What `LIST`
prints](entry-and-prompt.md#what-list-prints).  An empty program reports
`3 bytes.`

## Not available in this build

| Feature | Status |
|---|---|
| on-board EEPROM `SAVE`/`LOAD`/`FORMAT` | disabled; the optional filesystem does not compile in the current tree |
| `DEBUG` raw program dump | off in the default images; a custom build can enable it |
| arrays, string variables, user-defined functions | not in the language |
| `STEP` clause, multi-statement lines, `AND`/`OR` keywords | not in the language |
| hexadecimal or binary literals in a stored program | not parsed; decimal only |
| binary sub-protocol or host API | none of its own |

`DEBUG` is a build-time option that dumps the raw program area as hex bytes.
It is off by default and is not part of the normal command set.

The on-board EEPROM storage commands are a v4 build option that is off by
default.  Enabling it does not build in the current tree; treat the commands
as unavailable, and treat every program as RAM-only with no persistence.

## Language limits

- There are no arrays.  Use the 26 variables and `DATA`/`READ`.
- There are no string variables and no string expressions.  String literals
  are accepted only in `PRINT` and `INPUT`.
- There are no user-defined functions and no built-in numeric functions such
  as `ABS`, `INT` or `RND`.
- `FOR` has no `STEP`: the loop always counts up by one, and the end value is
  included.
- `NEXT` always names its loop variable.
- A `THEN`/`ELSE` branch is a single statement; there is no multi-statement
  line and no `ELSEIF`.
- A `PRINT` comma is not a separator: it prints `0` and leaves a syntax error.
  Use `;`.
- A `DATA` value must be a signed decimal literal; variables, operators,
  expressions, quoted text and a trailing comma are not accepted.
- `INPUT` accepts one number and one variable per statement; it is not a
  string input.

## v3 / v4 differences

| Feature | v3 | v4 |
|---|---|---|
| interpreter (`s`) | included in the default image | included |
| language, statements, limits, errors | same | same |
| mode numbers used by `MODE` | no `KEYB`; `PIC`=8, `DIO`=9 | adds `KEYB`=8; `PIC`=9, `DIO`=10 |
| `AUXPIN` targets | `A0`, `CS` | `A0`, `CS`, `A1`, `A2` |
| on-board Vpullup source `e` | — | terminal only; `MODE` clears it |
| on-board program storage | — | build option, off and not buildable |
| `DEBUG` raw dump | build option, off | build option, off |
| `#` | hardware reset; the stored program is cleared | terminal reset; the stored program is kept in RAM |
| ADC divider | unverified | 6.6 V scale |

The interpreters in the two builds share the same behaviour; the v3 column is
stated from the build only and is **not verified** on v3 hardware.

## Custom builds

- The program area, `FOR` table and `GOSUB` stack are build-time constants; a
  custom build can change them.  The defaults above are what the examples
  assume.
- A build can leave the interpreter out entirely; then `s` is not a command.
- The mode menu lists only the modes compiled in.  A custom build with `LCD`
  inserts it after `KEYB`, so `PIC` and `DIO` move to 10 and 11; a custom v3
  build can also gain `KEYB`.  Use the `m` menu of the build in front of you as
  the authority for `MODE` numbers.
- `SMPS` is a binary capability, not a mode; it is not selectable with `MODE`.
  It shares the ADC and PWM output-compare resources with the `ADC` and
  `FREQ`/`DUTY` statements.
