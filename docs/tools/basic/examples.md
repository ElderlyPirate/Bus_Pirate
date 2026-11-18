# BASIC — worked examples

Part of the BASIC documentation set (`docs/tools/basic/`).  Each example shows
the program and the replies it produces.  The examples use only the default
image's modes and the shared language; the details behind each statement are
in the topic pages.

| Task | Section |
|---|---|
| store, list and run a program | [Hello, LIST and EXIT](#hello-list-and-exit) |
| edit or delete a line | [Replacing, deleting and a rejected line](#replacing-deleting-and-a-rejected-line) |
| arithmetic and `LET` | [Arithmetic, LET and division](#arithmetic-let-and-division) |
| conditionals and a `FOR` loop | [Conditionals and a loop](#conditionals-and-a-loop) |
| read a number | [Reading a number with INPUT](#reading-a-number-with-input) |
| `DATA` and `READ` | [DATA and READ](#data-and-read) |
| call a subroutine | [A subroutine](#a-subroutine) |
| jump with `GOTO` | [A GOTO loop](#a-goto-loop) |
| I2C transaction | [Switching mode and an I2C transaction](#switching-mode-and-an-i2c-transaction) |
| bit-bang clock and data | [Bit-banging clock and data](#bit-banging-clock-and-data) |
| run a mode macro | [A mode macro](#a-mode-macro) |
| PWM output | [PWM](#pwm) |
| supply, pull-ups, AUX | [Supply, pull-ups and an AUX pin](#supply-pull-ups-and-an-aux-pin) |
| measure an interval | [A timed pause](#a-timed-pause) |

## Hello, LIST and EXIT

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
Ready
HiZ>
```

Storing a line answers `Ready`; `RUN` prints `HI`, then a blank line (the
`PRINT` newline plus the interpreter's own line break), then `Ready`.  The
`65535  END` line is the end-of-program marker, and `15 bytes.` is the count
`LIST` prints for the two stored lines plus the marker.

## Replacing, deleting and a rejected line

```
HiZ(BASIC)> 10 PRINT "HI"
Ready
HiZ(BASIC)> 10 REM 123456789012345678901234567890123456
Too long!
Ready
HiZ(BASIC)> RUN
HI

Ready
```

The replacement is too long, so it is rejected; the old line 10 is still
there.  Deleting a line is a bare line number:

```
HiZ(BASIC)> 10
Ready
HiZ(BASIC)> LIST

65535  END
3 bytes.
Ready
```

## Arithmetic, LET and division

```
10 LET A=1+2
20 PRINT A;
30 END
```

`RUN` prints `3`.  The assignment keyword `LET` is required:

```
HiZ(BASIC)> 10 A=1
Ready
HiZ(BASIC)> RUN
Error(3) @line:10 @pgmspace:6

Ready
```

Division by zero is defined as zero and is not an error:

```
10 LET A=1/0
20 PRINT A;
30 END
```

`RUN` prints `0`.  Each operator level is evaluated left to right, so
`8/4/2` is `(8/4)/2`:

```
10 LET A=8/4/2
20 PRINT A;
30 END
```

`RUN` prints `1`, not `4`.  See
[language.md §Operators](language.md#operators).

## Conditionals and a loop

```
10 IF 1 THEN PRINT "Y";
20 END
```

`RUN` prints `Y`.  With a false condition the `ELSE` branch runs:

```
10 IF 0 THEN PRINT "N"; ELSE PRINT "Z";
20 END
```

`RUN` prints `Z`.  A counted loop:

```
10 FOR B=1 TO 3
20 PRINT B;
30 NEXT B
40 END
```

`RUN` prints `123`.

## Reading a number with INPUT

`INPUT` always prints the value prompt `(0)>`, where `0` is the default.  With
a prompt text, the comma is required:

```
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

Without prompt text, write `INPUT A`:

```
HiZ(BASIC)> 10 INPUT A
Ready
HiZ(BASIC)> 20 PRINT A;
Ready
HiZ(BASIC)> 30 END
Ready
HiZ(BASIC)> RUN

(0)> 7
7
Ready
```

A bare Enter selects `0`:

```
HiZ(BASIC)> RUN
VALUE
(0)>
0
Ready
```

A value outside `0`..`32767` is not clamped; the prompt comes back:

```
HiZ(BASIC)> RUN
VALUE
(0)> 40000

Invalid choice, try again

(0)> 12
12
Ready
```

`INPUT , A` is not a valid no-prompt form; it stops with `Error(3)`.  Use
`INPUT A`.

## DATA and READ

```
10 DATA 1,2
20 READ A
30 READ B
40 PRINT A;B;
50 END
```

`RUN` prints `12`.  `RESTORE` sends the read position back to the first `DATA`
line, so the same values can be read again:

```
10 DATA 1,2
20 READ A
30 READ B
40 RESTORE
50 READ C
60 PRINT A;B;C;
70 END
```

`RUN` prints `121`.

Reading past the last value is an error, not a wrap:

```
HiZ(BASIC)> 10 DATA 1,2
Ready
HiZ(BASIC)> 20 READ A
Ready
HiZ(BASIC)> 30 READ B
Ready
HiZ(BASIC)> 40 READ C
Ready
HiZ(BASIC)> RUN
Error(9) @line:40 @pgmspace:21

Ready
```

## A subroutine

```
10 GOSUB 100
20 PRINT "DONE";
30 END
100 PRINT "SUB";
110 RETURN
```

`RUN` prints `SUBDONE`.

## A GOTO loop

`GOTO` jumps to a line number; a condition makes the loop finite:

```
HiZ(BASIC)> 10 LET A=0
Ready
HiZ(BASIC)> 20 LET A=A+1
Ready
HiZ(BASIC)> 30 PRINT A;
Ready
HiZ(BASIC)> 40 IF A<3 THEN GOTO 20
Ready
HiZ(BASIC)> 50 END
Ready
HiZ(BASIC)> RUN
123
Ready
```

The `IF` line is the loop test: while `A` is below 3 it jumps back to line 20.
On the last pass `A` is 3, the `THEN` branch is skipped, and the run falls into
line 50.

## Switching mode and an I2C transaction

The program below keeps `MODE 4`; `RUN` prints the mode banner and then the
reply:

```
HiZ(BASIC)> 10 MODE 4
Ready
HiZ(BASIC)> 15 PULLUP 1
Ready
HiZ(BASIC)> 20 START
Ready
HiZ(BASIC)> 30 SEND 161
Ready
HiZ(BASIC)> 40 PRINT RECEIVE;
Ready
HiZ(BASIC)> 50 STOP
Ready
HiZ(BASIC)> RUN
I2C
255
Ready
I2C(BASIC)>
```

- `MODE 4` selects I2C, applies its default setup and engages the bus, prints
  `I2C`, and the prompt becomes `I2C(BASIC)>` after the run.
- `PULLUP 1` ties the I/O lines to **Vpullup**.  That pin must be fed:
  `MODE 4` runs the board-state reset, and on v4 that turns the on-board
  Vpullup source off.  The program above therefore needs an external Vpullup
  supply on v4, and on v3 in every case.
- To use the v4 on-board source instead, delete line 10 and do the I2C setup
  at the terminal: select the source with `e`, enable the pull-ups with `P`,
  engage the bus with `W`, then enter `s`.  Any `MODE` line clears those
  settings again.  `W` also switches the target supply on, so use it only when
  the target should be powered from the board.
- `START` issues the I2C start condition and `STOP` the stop condition.
- `SEND 161` writes one byte — here `161`, the read address `0xA1`.
- `RECEIVE` is an expression, so it must be printed (or assigned) to be seen.
  `PRINT RECEIVE;` clocks in one byte and prints it; the example prints `255`,
  the all-ones value a pulled-up idle bus reads when no device answers.  An
  attached device returns its own data.

A `2WIRE` program has the same pull-up and `MODE`-reset caveat: `MODE 6`
clears the Vpullup source and bit order just as `MODE 4` does.

## Bit-banging clock and data

`2WIRE` and `3WIRE` expose the clock and data lines directly.  This 3WIRE
session clocks two data bits out by hand, with no device attached:

```
HiZ(BASIC)> 10 MODE 7
Ready
HiZ(BASIC)> 20 START
Ready
HiZ(BASIC)> 30 DAT 1
Ready
HiZ(BASIC)> 40 CLK 2
Ready
HiZ(BASIC)> 50 DAT 0
Ready
HiZ(BASIC)> 60 CLK 2
Ready
HiZ(BASIC)> 70 STOP
Ready
HiZ(BASIC)> RUN
3WIRE

Ready
3WIRE(BASIC)>
```

`DAT 1` and `DAT 0` drive `MOSI`; each `CLK 2` produces one clock pulse.
`START` asserts `CS` (low in the default setup), `STOP` returns it high, and
the clock and data outputs keep their last levels.  A real device needs its
own framing and timing on top of these primitives.

## A mode macro

`MACRO n` runs macro `n` of the active mode and prints that mode's own reply.
The mode page owns the reply text; a mode with no macros reports an error.

```
10 MODE 2
20 MACRO 1
30 END
```

Line 10 selects 1-Wire; line 20 runs its first macro (the bus reset). Run this
only with the bus wired as the 1-Wire page requires.

## PWM

`FREQ` takes Hz and a duty percentage; `DUTY` changes the duty of the current
frequency.  The statements are silent.

```
HiZ(BASIC)> 10 FREQ 1000 50
Ready
HiZ(BASIC)> LIST

10  FREQ 1000,50
65535  END
14 bytes.
Ready
HiZ(BASIC)> 20 DUTY 25
Ready
HiZ(BASIC)> RUN

Ready
```

The space between the two values is stored as a comma, so `LIST` shows
`FREQ 1000,50`.  The output is 1 kHz at 50 %, then 25 %, on the `AUX` pin.
`FREQ 0 50` stops it.  The terminal `g` command uses kHz; `g10` is 10 kHz,
while `FREQ 10` is 10 Hz.

## Supply, pull-ups and an AUX pin

```
HiZ(BASIC)> 10 PSU 1
Ready
HiZ(BASIC)> 20 PRINT PSU;
Ready
HiZ(BASIC)> 30 PSU 0
Ready
HiZ(BASIC)> 40 PRINT PSU;
Ready
HiZ(BASIC)> 50 END
Ready
HiZ(BASIC)> RUN
10
Ready
```

`PSU 1` switches the on-board regulator on and `PSU 0` off; the expression
reads `1` and `0`.  These are direct hardware calls and work in HiZ.

`PULLUP` uses the opposite sense in an expression — `0` while the pull-ups are
on, `1` while they are off:

```
HiZ(BASIC)> 10 PULLUP 1
Ready
HiZ(BASIC)> 20 PRINT PULLUP;
Ready
HiZ(BASIC)> 30 PULLUP 0
Ready
HiZ(BASIC)> 40 PRINT PULLUP;
Ready
HiZ(BASIC)> 50 END
Ready
HiZ(BASIC)> RUN
01
Ready
```

The pull-ups need a voltage on Vpullup; they do not create one.  Driving an
AUX target is silent:

```
HiZ(BASIC)> 10 AUXPIN 1
Ready
HiZ(BASIC)> 20 AUX 1
Ready
HiZ(BASIC)> 30 AUX 0
Ready
HiZ(BASIC)> 40 END
Ready
HiZ(BASIC)> RUN

Ready
```

This drives `CS` high then low (`AUXPIN 1` selects `CS`).  `PRINT AUX;` would
release the pin and sample it, so use it only when something holds the level.

## A timed pause

```
10 LET T=TIME
20 DELAY 100
30 PRINT TIME-T;
40 END
```

`RUN` prints the elapsed time in 64 µs units — about `1562` for the 100 ms
delay.  Subtract two `TIME` readings because the counter wraps; keep the
interval below the 4.19 s wrap.

## v3 / v4

The examples use the shared language and the default mode numbers in
[bus-access.md](bus-access.md).  The build differences are in
[limits-and-variants.md](limits-and-variants.md).
