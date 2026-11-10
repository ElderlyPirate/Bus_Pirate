# BASIC — language reference

Part of the BASIC documentation set (`docs/basic/`).  This page owns the
statement set, expressions, operators, variables and error reporting.  Bus
statements and `MODE` have their own page: [bus-access.md](bus-access.md).

## Scope

What you can write inside a program.  Entry, storage and editing are in
[entry-and-prompt.md](entry-and-prompt.md); limits are in
[limits-and-variants.md](limits-and-variants.md).

## Statements

A stored line holds one statement.  The interpreter's statement set:

| Statement | Action |
|---|---|
| `LET var = expression` | assign to a variable; `var` is `A`..`Z` |
| `IF expression THEN statement [ELSE statement]` | run the `THEN` statement when the expression is non-zero, otherwise the `ELSE` statement if present |
| `GOTO line` | jump to a line number |
| `GOSUB line` | call a subroutine at a line number |
| `RETURN` | return from the innermost `GOSUB` |
| `FOR var = start TO end` | start a counted loop |
| `NEXT var` | close the matching `FOR` loop |
| `PRINT item[; item]...` | print text and values |
| `INPUT ["prompt",] var` | read a number from the terminal into `var` |
| `DATA value[, value]...` | a comma-separated list of values, read back by `READ` |
| `READ var` | take the next value from the `DATA` statements |
| `RESTORE` | reset the `DATA` read position to the first `DATA` statement |
| `REM text` | a comment; the rest of the line is ignored |
| `END` | stop the program |

Bus statements (`START`, `STARTR`, `STOP`, `STOPR`, `SEND`, `CLK`, `DAT`,
`AUX`, `AUXPIN`, `PSU`, `PULLUP`, `DELAY`, `FREQ`, `DUTY`, `MACRO`) and the
`MODE` statement are in [bus-access.md](bus-access.md).

There are no strings, no string variables, no arrays, no user-defined
functions and no multi-statement lines.  A `THEN`/`ELSE` branch is exactly one
statement.

The engine also has no `STEP` clause on `FOR`, no built-in numeric functions
(`ABS`, `INT`, `RND`), and no bare `NEXT` — `NEXT` always names its loop
variable.  `PRINT` has one separator, `;`; a `,` is not accepted as a `PRINT`
separator.

## Variables

- Names are the single letters `A` through `Z` — 26 variables.
- Each holds a **signed 16-bit** value.  Values wrap at 16 bits; arithmetic
  is performed at 16 bits.
- Variables are cleared to zero at the start of every `RUN`.
- Assignment uses the keyword `LET`: `LET A=1`.  A bare `A=1` is not accepted
  (it stores, but stops at run time with a syntax error).

## Expressions

A value in an expression is one of:

- a decimal number (`0`..`65535`; hexadecimal and binary literals are **not**
  parsed inside a stored program),
- a variable `A`..`Z`,
- a hardware read token (below),
- a parenthesised expression.

The hardware read tokens read the board from inside an expression:

| Token | Reads |
|---|---|
| `RECEIVE` | one value from the active mode's read path |
| `AUX` | the AUX-pin state |
| `DAT` | the data-line state |
| `BITREAD` | one bit from the active mode's read-bit path |
| `PSU` | the voltage-regulator (power-supply) state |
| `PULLUP` | the pull-up-resistor state |
| `ADC` | the voltage-probe ADC reading |
| `MODE` | the number of the active mode (one-based, as the mode menu lists it) |
| `TIME` | the microsecond timer divided by 64 (one unit = 64 µs) |

`RECEIVE` and `BITREAD` are read-only tokens; they are not statements.
`AUX`, `DAT`, `PSU`, `PULLUP` and `MODE` also exist as statements
([bus-access.md](bus-access.md)).

### Operators

Two levels, each evaluated left to right:

| Level | Operators | Effect |
|---|---|---|
| 1 (higher) | `*` `/` `&` `\|` | multiply, divide, bitwise AND, bitwise OR |
| 2 | `+` `-` `=` `<>` `<` `<=` `>` `>=` | add, subtract, compare |

Note that this is the engine's actual binding, which is flatter than in many
BASICs: `&`/`|` bind like `*`/`/`, and the comparisons bind like `+`/`-`.
Within a level everything is left to right, so `1+2=3` evaluates as
`(1+2)=3`.

- A comparison yields `1` (true) or `0` (false).  A condition is true when its
  value is **non-zero**.
- Division by zero yields `0` and is not an error (the interpreter does not
  trap it).  The single overflow case `-32768 / -1` likewise yields `0`.
- `PRINT` shows the 16-bit value; a negative result prints as its unsigned
  form (for example `-1` prints `65535`).

## PRINT

`PRINT` writes its items one after another:

- Text in double quotes is printed literally, without the quotes.
- A numeric expression is printed in decimal.
- Multiple items are written one after another; a trailing `;` suppresses
  the newline.
- `;` is the only item separator; a `,` is not a `PRINT` separator.

```
10 PRINT "A=";
20 PRINT 1+2
30 END
```

prints `A=3`: line 10 writes `A=` with no newline, line 20 writes `3` and a
newline, and line 30 ends the program.

## INPUT

`INPUT ["prompt",] var` asks for a number and stores it in `var`:

- A leading `"prompt"` in double quotes is printed first; then a comma.
- The value entered must be `0`..`32767`; a bare Enter selects `0`.

## DATA / READ / RESTORE

- `DATA` lines hold a comma-separated list of values.
- `READ var` evaluates the **next** value in program order with the same value
  reader the expressions use, so a value may be a decimal number or a variable
  `A`..`Z`.  A variable in `DATA` is evaluated when the `READ` runs (its value
  at that moment), not when the `DATA` line was stored.
- The read position runs through the `DATA` values in the order the `DATA`
  lines appear.  When it runs past the last value the position resets to the
  first `DATA` statement, so `READ` cycles through the values.
- `RESTORE` resets the position to the first `DATA` statement on demand, so a
  program can read the same sequence twice.
- A `READ` with no `DATA` statement anywhere stops the program with error 9.

## Errors

When a program stops on an error the interpreter prints:

```
Error(N) @line:L @pgmspace:P
```

`L` is the line that was executing and `P` is the position inside the
program area.  `N` is:

| N | Meaning |
|---|---|
| 2 | partial instruction — the stored program is corrupt |
| 3 | syntax error |
| 4 | the `FOR`-loop table is full (too many nested `FOR`s) |
| 5 | `NEXT` does not match an active `FOR` |
| 6 | a `GOTO`/`GOSUB` target line was not found |
| 7 | `GOSUB` nested too deeply |
| 8 | `RETURN` without a matching `GOSUB` |
| 9 | `READ` with no `DATA` statement to read from |
| 10 | the program was interrupted with Ctrl-C |

A program that reaches `END` (or runs past its last line) finishes without an
error message.

Codes 2 and 3 can also be produced by a corrupt stored program, for example
after memory is damaged; see [edge-cases.md](edge-cases.md).

## v3 / v4

The statement set, operators, variables and error codes are the same in both
builds.  v3 behaviour is **not verified**.