# BASIC — worked sessions

Part of the BASIC documentation set (`docs/basic/`).  Each session is an
example that shows the entry sequence, the program, and the interpreter's
output.  They illustrate the engine's format and behaviour.

## Hello, and leaving

```
HiZ> s
HiZ(BASIC)> 10 PRINT "HI"
Ready
HiZ(BASIC)> 20 END
Ready
HiZ(BASIC)> RUN
HI

Ready
HiZ(BASIC)> EXIT
Ready
HiZ>
```

Storing a line answers `Ready`; `RUN` prints `HI`, then a blank line (the
`PRINT` newline plus the interpreter's own line break), then `Ready`.

## Arithmetic and the `LET` keyword

```
10 LET A=1+2
20 PRINT A;
30 END
```

`RUN` prints `3`.  The assignment keyword `LET` is required: `10 A=1` stores
but stops with a syntax error when run.

## Conditionals

```
10 IF 1 THEN PRINT "Y";
20 END
```

`RUN` prints `Y`.  With a false condition the `ELSE` branch runs:

```
10 IF 0 THEN PRINT "N"; ELSE PRINT "Z";
20 END
```

`RUN` prints `Z`.

## A counting loop

```
10 FOR B=1 TO 3
20 PRINT B;
30 NEXT B
40 END
```

`RUN` prints `123`.  The loop variable runs from the start value up to and
including the end value.

## DATA and READ

```
10 DATA 1,2
20 READ A
30 READ B
40 PRINT A;B;
50 END
```

`RUN` prints `12`.

`RESTORE` sends the read position back to the first `DATA` line, so the same
values can be read again:

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

## A subroutine

```
10 GOSUB 100
20 PRINT "DONE";
30 END
100 PRINT "SUB";
110 RETURN
```

`RUN` prints `SUBDONE`.

## Division by zero

```
10 LET A=1/0
20 PRINT A;
30 END
```

`RUN` prints `0`.  It is not an error and the Bus Pirate is not disturbed.

## Switching mode from inside the program

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

`MODE 5` switches to the mode the menu numbers `5` — `SPI` on a v4 —
applying that mode's default settings, and prints the new name.  `PRINT
MODE;` then prints the active mode's number.  The prompt now shows
`SPI(BASIC)>`.  See [bus-access.md](bus-access.md) for the mode-number table.

## Driving an I2C bus (illustration)

A worked bus transaction: switch to I2C, address a device, read one byte and
release the bus.  It is an illustration — the byte read back depends on the
device you are talking to.

```
HiZ(BASIC)> 10 MODE 4
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

- `MODE 4` selects I2C and prints `I2C`, so the prompt becomes `I2C(BASIC)>`.
- `START` issues the I2C start condition and `STOP` the stop condition.
- `SEND 161` writes one byte — here `161`, the read address `0xA1`.
- `RECEIVE` is an expression, so it must be printed (or assigned) to be seen.
  `PRINT RECEIVE;` clocks in one byte and prints it; the example prints `255`,
  the all-ones value an idle bus reads.  An attached device returns its own
  data.

## Driving an AUX pin

```
10 AUX 1
20 PRINT AUX;
30 AUX 0
40 PRINT AUX;
50 END
```

`RUN` prints `10`: the first `AUX 1` drives the pin high, `PRINT AUX;` reads
it back, and the second pair drives it low and reads it back.

## A timed pause

```
10 LET T=TIME
20 DELAY 100
30 PRINT TIME-T;
40 END
```

`RUN` prints the elapsed time in 64 µs units — about `1562` for the 100 ms
delay.  Subtract two `TIME` readings because the counter wraps.

## v3 / v4

The examples use only the shared language and the v4 mode numbers shown in
[bus-access.md](bus-access.md).  On v3 the interpreter behaves the same; v3
is **not verified**.