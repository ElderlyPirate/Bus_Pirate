# BASIC — language reference

Part of the BASIC documentation set (`docs/tools/basic/`).  This page owns the
statement set, expressions, operators, variables, `INPUT`, `DATA`/`READ`, the
jump and loop statements, and error reporting.  Bus access and `MODE` have
their own pages: [bus-access.md](bus-access.md) and
[board-pins.md](board-pins.md).

## Scope

What you can write inside a program.  Entry, storage and editing are in
[entry-and-prompt.md](entry-and-prompt.md); limits are in
[limits-and-variants.md](limits-and-variants.md).

## Statements

A stored line holds one statement.  The core statement set:

| Statement | Form | Action |
|---|---|---|
| `LET` | `LET A=expression` | assign to a variable; the keyword is required |
| `IF` | `IF expression THEN statement [ELSE statement]` | run the `THEN` statement when the expression is non-zero, otherwise the `ELSE` statement if present |
| `GOTO` | `GOTO expression` | jump to a line number |
| `GOSUB` | `GOSUB expression` | call a subroutine at a line number |
| `RETURN` | `RETURN` | return from the innermost `GOSUB` |
| `FOR` | `FOR A=expression TO expression` | start a counted loop |
| `NEXT` | `NEXT A` | close the matching `FOR` loop |
| `PRINT` | `PRINT item[;item]...` | print text and values |
| `INPUT` | `INPUT A` or `INPUT "text", A` | read a number from the terminal |
| `DATA` | `DATA value[,value]...` | a list of values, read back by `READ` |
| `READ` | `READ A` | take the next value from the `DATA` list |
| `RESTORE` | `RESTORE` | reset the `DATA` read position to the first `DATA` line |
| `REM` | `REM text` | a comment; execution skips the line |
| `END` | `END` | stop the program |

Bus statements (`MODE`, `START`/`STARTR`, `STOP`/`STOPR`, `SEND`, `RECEIVE`,
`BITREAD`, `CLK`, `DAT`, `MACRO`) are in [bus-access.md](bus-access.md).  The
board-level set (`PSU`, `PULLUP`, `AUX`, `AUXPIN`, `FREQ`, `DUTY`, `DELAY`) and
the board-level tokens (`ADC`, `TIME`) are in
[board-pins.md](board-pins.md).

There are no string variables and no string expressions; string literals are
allowed in `PRINT` and `INPUT`.  There are no arrays, no user-defined
functions, no `STEP` clause, no `AND`/`OR` keywords, and no multi-statement
lines.  A `THEN`/`ELSE` branch is exactly one statement.

## Variables

- Names are the single letters `A` through `Z` — 26 variables.
- Each holds a **signed 16-bit** value.  Arithmetic wraps at 16 bits.
- Variables are cleared to zero at the start of every `RUN`.
- Assignment uses the keyword `LET`: `LET A=1`.  A bare `A=1` stores, but
  stops at run time with `Error(3)`.

## Expressions

A value in an expression is one of:

- a decimal number from `0` to `65535` (hexadecimal and binary literals are
  not parsed inside a stored program),
- a variable `A`..`Z`,
- a read token (below),
- a parenthesised expression,
- a unary `+` or `-` before a value.

The hardware tokens access the board from inside an expression:

| Token | Value or effect |
|---|---|
| `RECEIVE` | one value from the active mode's read path |
| `SEND` | sends its argument and yields the mode's write reply (see [bus-access.md](bus-access.md#send-and-receive)) |
| `AUX` | the selected AUX target's pin level |
| `DAT` | the data-line level (in `1WIRE`, the bit stored for the next clock) |
| `BITREAD` | one bit from the active mode's read-bit path |
| `PSU` | the voltage-regulator state: `1` on, `0` off |
| `PULLUP` | the pull-up state: `0` on, `1` off (the complement; see [board-pins.md](board-pins.md#pullup)) |
| `ADC` | the probe ADC reading, raw `0`..`1023`; volts = `ADC / 1024 × 6.6` |
| `MODE` | the number of the active mode (one-based, as the mode menu lists it) |
| `TIME` | the microsecond timer divided by 64 (one unit = 64 µs) |

`RECEIVE`, `BITREAD`, `ADC` and `TIME` are read-only tokens; they cannot
begin a statement.  `SEND`, `PULLUP`, `AUX`, `DAT`, `PSU` and `MODE` also exist
as statements.

### Operators

Two levels, each evaluated left to right:

| Level | Operators | Effect |
|---|---|---|
| 1 (higher) | `*` `/` `&` `\|` | multiply, divide, bitwise AND, bitwise OR |
| 2 | `+` `-` `=` `<>` `<` `<=` `>` `>=` | add, subtract, compare |

The engine's binding is flatter than in many BASICs: `&` and `|` bind like `*`
and `/`, and the comparisons bind like `+` and `-`.  Within a level everything
is left to right, so `1+2=3` is `(1+2)=3` = `1`, while `1=1+1` is
`(1=1)+1` = `2`.  `1|2*3` is `(1|2)*3` = `9`, not `1|(2*3)`.

There are no `AND`/`OR` keywords.  Parenthesise each comparison and combine
with `*` (both) or `+` (either): `IF (A>1)*(B<5) THEN ...` is true only when
both are true, and `IF (A=0)+(B=0) THEN ...` is true when either is.

A comparison yields `1` (true) or `0` (false).  A condition is true when its
value is **non-zero**.  Division by zero yields `0` and is not an error; the
single overflow case `-32768 / -1` likewise yields `0`.

`PRINT` shows the 16-bit value unsigned: a negative result prints as its
two's-complement form (for example `-1` prints `65535`).  Comparisons and
division use signed values.

## PRINT

`PRINT` writes its items one after another:

- Text in double quotes is printed literally, without the quotes.
- A numeric expression is printed in decimal, unsigned, with no padding.
- Items are concatenated.  A `;` between items is skipped; a **trailing** `;`
  suppresses the newline.
- `;` is the separator to use.  A `,` is not a separator: it is parsed as a
  value and prints `0`, and it leaves the run in a syntax-error state.  So
  `PRINT 1,2` prints `102` and then stops with `Error(3)` — use `;`.
- A `PRINT` with no items prints a blank line.

```basic
10 PRINT "A=";
20 PRINT 1+2
30 END
```

This prints `A=3`: line 10 writes `A=` with no newline, line 20 writes `3` and
a newline, and line 30 ends the program.

An unterminated quote prints the text that was stored and then stops with
`Error(3)`.

## INPUT

`INPUT` asks for a number and stores it in a variable.  Two forms are
accepted:

| Form | What you see |
|---|---|
| `INPUT A` | the value prompt `(0)>` only |
| `INPUT "text", A` | the text, then the value prompt `(0)>` |

The comma is **required after a quoted prompt**, and there is no comma-only
form: `INPUT , A` stops the run with `Error(3)`; write `INPUT A` instead.

- The `0` in `(0)>` is the default.  A bare Enter stores `0`.
- The value must be `0`..`32767`.  A larger, negative or over-long value
  prints `Invalid choice, try again` and prompts again — it is not clamped.
- Only one variable is read per `INPUT`.
- Ctrl-C is ignored at the value prompt (it rings the bell).  You must enter a
  valid value or press Enter; a reset is the only other way out.

```basic
HiZ(BASIC)> 10 INPUT "VALUE", A
Ready
HiZ(BASIC)> 20 PRINT A;
Ready
HiZ(BASIC)> 30 END
Ready
HiZ(BASIC)> RUN
VALUE
(0)> 12
12
Ready
```

Bare Enter selects the default:

```basic
HiZ(BASIC)> RUN
VALUE
(0)>
0
Ready
```

An out-of-range value comes back to the prompt:

```basic
HiZ(BASIC)> RUN
VALUE
(0)> 40000

Invalid choice, try again

(0)> 12
12
Ready
```

## DATA / READ / RESTORE

- `DATA` holds comma-separated **signed decimal literals**:
  `DATA 1,2,-3`.  Variables, operators, expressions, quoted text and a
  trailing comma are not accepted.  A value outside the 16-bit range wraps.
- `DATA` values are read in stored order, wherever the program is executing:
  `READ` scans forward from its current position to the next `DATA` line.
- `READ var` stores the next value and advances.  `var` is a single letter.
- `READ` past the last value stops the program with `Error(9)`; the values do
  **not** wrap around.
- `RESTORE` sets the read position back to the first `DATA` line, so the same
  sequence can be read again.
- A `READ` with no `DATA` line anywhere also stops with `Error(9)`.
- The position is reset at the start of every `RUN`.

```basic
10 DATA 1,2
20 READ A
30 READ B
40 RESTORE
50 READ C
60 PRINT A;B;C;
70 END
```

`RUN` prints `121`.

## GOTO, GOSUB and RETURN

- `GOTO expression` jumps to the line whose number the expression yields.
  The number must match a stored line exactly; a missing line stops the
  program with `Error(6)`.
- `GOSUB expression` pushes a return frame and jumps.  `RETURN` pops the
  innermost frame and continues at the line after the `GOSUB` line (or after
  the line that contained a conditional `GOSUB`).
- The call stack holds 10 frames.  An 11th nested `GOSUB` stops with
  `Error(7)`.
- `RETURN` with no matching `GOSUB` stops with `Error(8)`.
- A `GOTO`/`GOSUB` target may be computed, for example `GOSUB A*100`.

## FOR and NEXT

- `FOR A=start TO end` assigns `start`, remembers the line after the `FOR`,
  and runs the body.  `NEXT A` increments `A` and jumps back while `A` is less
  than `end`; the end value is included.
- There is no `STEP`; the increment is always `+1`.
- `NEXT` must name its loop variable.  `NEXT` without a variable, or with a
  variable that has no active loop, stops with `Error(5)`.
- At most 4 `FOR` loops may be active at once.  A fifth nested `FOR` stops
  with `Error(4)`.
- `start` greater than `end` runs the body once.
- A completed loop is popped at its `NEXT`.  Jumping out of a loop with
  `GOTO` leaves its frame active; jumping back in adds another frame.

```basic
10 FOR B=1 TO 3
20 PRINT B;
30 NEXT B
40 END
```

`RUN` prints `123`.

## IF, THEN and ELSE

- The condition is any expression; non-zero is true.
- The `THEN` branch is one statement.  An optional `ELSE` and one more
  statement follow on the same line.
- A false condition with no `ELSE` skips the rest of the line.
- A true condition skips the `ELSE` branch.
- `THEN` is required.  `IF 1 PRINT "Y"` is a syntax error.

```basic
10 IF 1 THEN PRINT "Y";
20 END
```

`RUN` prints `Y`.  With a false condition the `ELSE` branch runs:

```basic
10 IF 0 THEN PRINT "N"; ELSE PRINT "Z";
20 END
```

`RUN` prints `Z`.

## REM

`REM` makes the rest of the line a comment; execution skips it.  Spaces in the
comment are preserved.  A `"` inside a `REM` toggles the tokenizer's string
mode, so the rest of the comment can be tokenized and its spaces removed:
`LIST` may not show what you typed and the line's byte count can change.
Execution still skips the line.  Avoid `"` inside a `REM`.

## Errors

Errors appear in two places.  A line rejected as you type prints one of the
entry messages; a stored line that fails while it runs prints the `Error(N)`
line.

| When | Message | Meaning |
|---|---|---|
| typing a line | `Syntax error` | an unnumbered line whose first word is not `RUN`/`LIST`/`NEW`/`EXIT`, or a byte that cannot be stored as text |
| typing a line | `Too long!` | over 31 statement bytes, or no room left in the program area |
| running | `Error(N) @line:L @pgmspace:P` | the stored line failed; `N` is the table below |

`L` is the line that was executing and `P` is the byte offset inside the
stored program.  `N` is:

| N | Meaning |
|---|---|
| 2 | partial instruction — trailing text after a complete statement (`START 1`, `INPUT A, B`), or a corrupt stored line |
| 3 | syntax error — a bad statement or expression; an unusable `MODE` number; an out-of-range `AUXPIN`; malformed `FREQ`/`DUTY` operands; an `ADC` refused because the SMPS owns the converter |
| 4 | the `FOR` table is full (more than 4 active loops) |
| 5 | `NEXT` does not match an active `FOR` |
| 6 | a `GOTO`/`GOSUB` target line was not found |
| 7 | the `GOSUB` stack is full (more than 10 frames) |
| 8 | `RETURN` without a matching `GOSUB` |
| 9 | `READ` found no further `DATA` value |
| 10 | the program was interrupted with Ctrl-C |

A program that reaches `END`, or the end-of-program marker, finishes without
an error message.  Code 1 is not an error: it is the interpreter's internal
"program finished" state.  Codes 2 and 3 can also come from a corrupt stored
program; `NEW` and retype it.  The concrete triggers are collected in
[edge-cases.md](edge-cases.md).

## v3 / v4

The statement set, operators, variables and error codes are the same in both
builds; the build differences are in
[limits-and-variants.md](limits-and-variants.md).
