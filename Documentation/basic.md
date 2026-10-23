# BASIC interpreter

The Bus Pirate firmware contains a small BASIC interpreter so that a script
can drive a bus without a PC at every step. Programs live in the Bus
Pirate's own RAM and are executed from there; nothing survives a power
cycle.

Enter the interpreter with `s` from the normal prompt. The prompt changes to
`... (BASIC)>` (for example `HiZ(BASIC)>`) and terminal input goes to the
interpreter. The current mode (HiZ, I2C, SPI, ...) is the one that was
active when `s` was typed, so select the mode first and then enter BASIC; a
program can also switch modes on its own with `MODE`.

Type `EXIT` to return to the normal prompt, or reset the board.

## Quick start

```
HiZ> s
HiZ(BASIC)> 10 PRINT "HI"
Ready
HiZ(BASIC)> 20 END
Ready
HiZ(BASIC)> LIST

10  PRINT "HI"
20  END
65535  END
15 bytes.
Ready
HiZ(BASIC)> RUN
HI

Ready
HiZ(BASIC)> EXIT
HiZ>
```

`LIST` prints the stored program. Tokens are printed with a space on either
side, and the interpreter's internal end-of-program marker appears as the
`65535  END` line. The trailing number is how many bytes the program takes
in the program area. `RUN` executes it.

## Entering and editing programs

* A line that starts with a decimal number is stored as program text, for
  example `10 PRINT "HI"`.
* Entering a line number that already exists replaces the old line, so a
  program is edited by retyping lines.
* Lines are kept sorted by their number no matter in which order they are
  typed.
* A line number on its own (no statement after it) deletes the line.
* All input is converted to upper case before it is parsed, *including text
  inside quotes*: `10 PRINT "hi"` prints `HI`.
* One line can hold at most 31 statement bytes; a longer line is rejected
  without being stored.
* Statements cannot be combined on one line. `IF ... THEN` takes a single
  statement, optionally followed by `ELSE` and another single statement.

Commands at the BASIC prompt (not stored as program lines):

| Command | Function |
|---|---|
| `RUN` | execute the stored program |
| `LIST` | print the stored program |
| `NEW` | erase the stored program |
| `EXIT` | leave the BASIC interpreter |
| `DEBUG` | hex-dump the program area (only when the firmware is built with `BP_BASIC_DEBUG_COMMAND`) |

## Statements

| Statement | Function |
|---|---|
| `LET var = expression` | assign to a variable (`var` is `A`..`Z`) |
| `IF expression THEN statement [ELSE statement]` | run a statement when the expression is non-zero, otherwise the `ELSE` statement if present |
| `GOTO line` | jump to a line number |
| `GOSUB line` / `RETURN` | call a subroutine / return from it, up to 10 levels deep |
| `FOR var = start TO end` ... `NEXT var` | counted loop; `var` runs from `start` to `end` inclusive, 4 levels of nesting |
| `PRINT item[; item]...` | print text and values; items are printed one after another, a trailing `;` suppresses the newline |
| `INPUT ["prompt",] var` | ask for a number and store it in `var` |
| `DATA n[, n]...` | storage for literal values |
| `READ var` | take the next value from the `DATA` statements |
| `REM text` | comment, the rest of the line is ignored |
| `END` | stop the program |
| `START` / `STARTR` | protocol start / start with read |
| `STOP` / `STOPR` | protocol stop / stop from read |
| `SEND value` | write a value to the bus |
| `CLK 0|1|2` | clock low / high / produce a pulse |
| `DAT 0|1` | set the data line low / high |
| `AUX 0|1` | drive the auxiliary pin low / high |
| `AUXPIN 0|1` | alternate auxiliary-pin function off / on |
| `PSU 0|1` | power supplies off / on |
| `PULLUP 0|1` | pull-up resistors off / on |
| `DELAY ms` | pause for the given number of milliseconds |
| `FREQ hz duty` | configure the PWM output |
| `DUTY duty` | change the PWM duty cycle |
| `MACRO n` | run a mode macro |
| `MODE [n]` | switch to the mode the menu numbers `n` (with that mode's default settings), or report the active mode's name |

## Expressions

Values in an expression are:

* decimal numbers (hexadecimal and binary literals are not parsed inside
  stored programs),
* variables `A`..`Z` (signed 16-bit),
* the special tokens `RECEIVE`, `AUX`, `DAT`, `BITREAD`, `PSU`, `PULLUP` and
  `ADC`, which read from the hardware as described above, and `MODE`, which
  yields the active mode's number,
* parenthesised expressions.

Operators, in binding order:

| Operators | Effect |
|---|---|
| `*` `/` `&` `\|` | multiply, divide, bitwise AND, bitwise OR |
| `+` `-` `=` `<>` `<` `<=` `>` `>=` | add, subtract, compare |

Comparisons and addition are evaluated left to right at the same level.
Comparison results are `1` (true) or `0` (false), and a condition is true
when its value is non-zero. Arithmetic wraps around at 16 bits. A division
by zero yields `0`.

## Limits

| Item | Value |
|---|---|
| program area | 1024 bytes (`BP_BASIC_PROGRAM_SPACE`) |
| statement bytes per line | up to 31 |
| variables | `A`..`Z`, signed 16-bit |
| nested `FOR` loops | 4 (`BP_BASIC_NESTED_FOR_LOOP_COUNT`) |
| nested `GOSUB` calls | 10 (`BP_BASIC_STACK_FRAMES_DEPTH`) |

The program area can be adjusted at build time through
`BP_BASIC_PROGRAM_SPACE`; the other limits likewise have build-time
settings in `Firmware/configuration.h`.

## Error reporting

When a program stops with an error, the interpreter prints:

```
Error(N) @line:L @pgmspace:P
```

`L` is the line that was executing and `P` is the position inside the
program area. `N` is one of:

| Code | Meaning |
|---|---|
| 2 | partial instruction (the program is corrupted) |
| 3 | syntax error |
| 4 | the `FOR` loop table is full |
| 5 | `NEXT` does not match an active `FOR` |
| 6 | the `GOTO`/`GOSUB` target line was not found |
| 7 | `GOSUB` nested too deeply |
| 8 | `RETURN` without a matching `GOSUB` |
| 9 | no `DATA` statement to read from |

A program that reaches `END` (or simply runs past its last line) finishes
without a message.

## Examples

All of the following were run on hardware while the interpreter was
verified, and show exactly what the board prints.

Hello:

```
10 PRINT "HI"
20 END
```

Arithmetic (note that the assignment keyword `LET` is required, `10 A=1` is
a syntax error):

```
10 LET A=1+2
20 PRINT A;
30 END
```
prints `3`.

Conditional:

```
10 IF 1 THEN PRINT "Y";
20 END
```
prints `Y`. `10 IF 0 THEN PRINT "N"; ELSE PRINT "Z";` prints `Z`.

Counting loop:

```
10 FOR B=1 TO 3
20 PRINT B;
30 NEXT B
40 END
```
prints `123`.

Data values:

```
10 DATA 1,2
20 READ A
30 READ B
40 PRINT A;B;
50 END
```
prints `12`.

Subroutine:

```
10 GOSUB 100
20 PRINT "DONE";
30 END
100 PRINT "SUB";
110 RETURN
```
prints `SUBDONE`.

Division by zero:

```
10 LET A=1/0
20 PRINT A;
30 END
```
prints `0`; it is not an error, and the Bus Pirate is not disturbed by it.

Switching modes from within the program:

```
HiZ(BASIC)> 10 MODE 5
Ready
HiZ(BASIC)> 20 PRINT MODE;
Ready
HiZ(BASIC)> RUN
SPI
5
Ready
SPI(BASIC)>
```

`MODE 5` switches to the mode the mode menu numbers `5` (SPI on a v4),
applying the defaults that pressing ENTER through the setup prompts would
choose, and prints the new mode's name. `PRINT MODE;` prints the mode number
from inside an expression, so a script can check where it is. A bare `MODE`
statement prints the name alone. The argument is an expression, so
`MODE M` after `LET M=4` works as well; a number outside the menu's range
stops the program with error 3 and leaves the current mode alone. The switch
also runs the new mode's setup callback (the step the interactive flow
performs on power-on), so the statements that follow can use the bus right
away.

## Notes for maintainers: the stored program format

The interpreter stores each program line as a small block of bytes:

```
[length] [line high] [line low] [statement bytes...]
```

`length` is `0xE0` plus the number of statement bytes. Statement keywords
are stored as one-byte tokens from `0x80` upwards; everything else (digits,
operators, quotes, text) is stored as its ASCII byte. The stored program is
terminated by a marker line with line number 65535 whose statement is
`END` - that is the `65535  END` line `LIST` shows.

For example, the program

```
10 PRINT "HI"
20 END
```

is stored (as shown by the `DEBUG` command) as:

```
0xE5 0x00 0x0A 0x88 0x22 0x48 0x49 0x22   <- 10 PRINT "HI"
0xE1 0x00 0x14 0xA1                        <- 20 END
0xE1 0xFF 0xFF 0xA1                        <- end-of-program marker
0x00 ...                                   <- unused area, zeroed
```

`0xE5` is `0xE0 + 5`, the five statement bytes `0x88` (`PRINT`) `"HI"`.

Two details are easy to get wrong when changing the storage code:

* The terminal appends a NUL byte to every entered line and the tokenizer
  counts it as one item, so the number of items is one more than the number
  of statement bytes: the length is `items - 4`, not `items - 3`.
* Inserting a line shifts the rest of the area with `memmove()` and copies
  the new line with `memcpy()`. Both were once loops with off-by-one errors;
  a line written at the wrong offset corrupts the area, and the walks below
  used to follow the corruption out of the array, where a read of
  unimplemented memory raises an address error trap - a board reset.

Every walk in the interpreter is therefore bounded by
`BP_BASIC_PROGRAM_SPACE`, and variable positions in `LET`, `FOR`, `READ` and
`INPUT` are validated before they are used as indexes. A corrupted byte can
produce error 2 or 3, but cannot make the interpreter access memory
outside the program area.