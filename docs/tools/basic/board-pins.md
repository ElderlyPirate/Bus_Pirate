# BASIC — board-level pins and signals

Part of the BASIC documentation set (`docs/tools/basic/`).  This page owns the
statements that act on the board itself rather than on the active mode's bus:
`PSU`, `PULLUP`, `AUX`, `AUXPIN`, `FREQ`, `DUTY`, `DELAY`, and the read tokens
`ADC` and `TIME`.  The bus statements and `MODE` are in
[bus-access.md](bus-access.md).

## Scope

The board-level calls, their pins and rails, their ranges, and their behaviour
in HiZ.  The mode wiring summary is in
[bus-access.md §Wiring per mode](bus-access.md#wiring-per-mode).  Shared
terminal commands (`W`, `P`, `g`, `e`, `d`, `t`) are owned by
[docs/generic/](../../generic/README.md) and the
[AUX](../../capabilities/auxiliary/README.md) and
[ADC](../../capabilities/adc/README.md) sets.

## Board-level versus mode-level

These statements are **direct hardware calls**, not protocol callbacks.  They
work in every mode, including `HiZ`, and they are not subject to the terminal's
HiZ guards on `W`, `P` and `g`.  `MODE` resets most of them; see the reset
table in [bus-access.md](bus-access.md#what-a-mode-switch-resets).

## PSU

| Form | Action |
|---|---|
| `PSU 0` / `PSU 1` | switch the on-board regulator off / on |
| `PSU` (expression) | `1` when on, `0` when off |

`PSU 1` is the same regulator switch that the terminal `W` turns on, without
the `W` voltage checks, messages or bus-engagement step.  It does not depend on
the active mode and works in HiZ.  `MODE` turns it off.

**Do not use `PSU 1` into a self-powered target.**  It bypasses `W`'s voltage
checks, so it can back-drive the target's supply.  A target with its own power
needs no `PSU` statement.

Wiring: the regulator feeds the board's `5.0V` / `3.3V` supply pins; the
target's ground must be common.  See the
[generic wiring headline](../../generic/README.md#wiring-headline) for the
rails and limits.

## PULLUP

| Form | Action |
|---|---|
| `PULLUP 0` / `PULLUP 1` | release / enable the on-board pull-up resistors |
| `PULLUP` (expression) | `0` while the pull-ups are **on**, `1` while they are **off** |

The pull-up resistors tie the I/O lines to the **Vpullup** pin.  They do not
create a voltage: something must feed Vpullup.  On v4 the on-board Vpullup
source is selected with the terminal `e` menu (`3.3 V` or `5.0 V`; the `v`
table prints the pin as `VPU`); BASIC has no `e` statement, and `MODE` turns
the on-board source off.  On v3 feed Vpullup externally.

The expression is the **logical complement** of the pull-up state — the
opposite sense from the `i` status line in a non-HiZ mode.  `IF PULLUP THEN`
is true when the pull-ups are **off**.  To test for "on", write
`IF PULLUP=0 THEN` or track the state in a variable.

`PULLUP` is a direct hardware call: it is not refused in HiZ or by a mode's
terminal guard.  `MODE` releases the pull-ups.

## AUX and AUXPIN

`AUXPIN` selects the pin that `AUX` drives or reads.  It is a **target
selector**, not an on/off flag:

| `AUXPIN` | Target token | Pin name | Terminal select |
|---|---|---|---|
| `0` | `A0` | `AUX` | `c` |
| `1` | `CS` | `CS` | `C` |
| `2` (v4) | `A1` | `AUX1` | `k` |
| `3` (v4) | `A2` | `AUX2` | `K` |

The v3 `?` help prints the `A0` token as `aux`; the pin is still `AUX`.

- `AUX 0` and `AUX 1` drive the selected target low and high.
- `AUX` as an expression reads the selected target.  The read sets the pin to
  an input first, so it **releases** the pin; drive it again to restore an
  output level.  The value depends on what holds the line.
- `AUXPIN` out of range stops the program with `Error(3)` and leaves the
  current target unchanged.  v3 accepts 0 and 1; v4 accepts 0..3.
- `MODE` resets the target to `A0` (the `AUX` pin).
- The statements are silent during a run; the terminal's confirmation lines
  are suppressed.
- If the active mode uses the same pin (SPI and DIO use `CS`, for example),
  driving it from the program overrides the mode's signal.  Restore the state
  or switch mode before returning to the terminal.

## PWM: FREQ and DUTY

| Form | Action |
|---|---|
| `FREQ hz duty` | set up PWM: frequency in **Hz**, duty in percent |
| `DUTY duty` | change the duty of the current BASIC frequency |
| `FREQ 0 duty` | stop PWM and detach the output |

| Item | Behaviour |
|---|---|
| frequency | `0` (off) or `1`..`4000` **Hz** |
| duty | `1`..`100` percent; `DUTY 100` is accepted, the terminal `g` prompt stops at 99 % |
| out of range | clamped, not rejected: `FREQ 5000 0` is 4000 Hz at 1 %, `FREQ 1000 150` is 100 % |
| separators | two expressions; a space, comma or operator boundary works: `FREQ 1000 50`, `FREQ 1000,50`, `FREQ 500+500 25*2`, `FREQ A B` |
| operands | exactly two; a single number (`FREQ 100050`), a missing operand or a third stops with `Error(3)` |
| stored form | two decimal values separated by a space are stored with a comma: `10 FREQ 1000 50` lists as `10  FREQ 1000,50` |
| output | the `AUX` pin, whatever `AUXPIN` selects; 3.3 V logic, no supply or pull-up, and the terminal's `PWM active` / `PWM disabled` messages do not appear |
| accuracy | whole timer ticks, at or above the requested frequency and at or below the duty; 1 Hz is 62,500 counts of 16 µs, 100 Hz 20,000 of 0.5 µs, 4 kHz 4,000 of 62.5 ns |
| SMPS | if the optional [SMPS](../../capabilities/smps/README.md) owns the output-compare channel, `FREQ`/`DUTY` do nothing and report no error; the terminal `g` is refused instead |
| HiZ | works in HiZ, unlike the terminal `g`; `MODE` detaches the output |

`FREQ 1000 50` is 1 kHz at 50 %; `DUTY 25` then changes the duty on `AUX`,
and `FREQ 0 50` stops it.  `DUTY` uses the frequency of the last BASIC `FREQ`,
even after a `MODE` switch; it does not read a generator started at the
terminal.  With no BASIC `FREQ` since power-on, `DUTY` detaches the output.

The terminal `g` takes **kHz**; BASIC takes **Hz**.  `g10` is 10 kHz, while
`FREQ 10` is 10 Hz.  BASIC's 4000 Hz ceiling is far below the terminal's
4 MHz range.  The timer and prescaler description is written for the
terminal's kHz range: [pwm-servo.md](../../capabilities/auxiliary/pwm-servo.md).

## ADC

`ADC` is an expression-only token: it takes one sample of the voltage probe.

- Raw value `0`..`1023`; volts = `ADC / 1024 × 6.6` (about 6.45 mV per count,
  with a full scale of 6.59 V at raw 1023).
- The probe pin is the ADC input, not the AUX pin.  The input range is in the
  [ADC set](../../capabilities/adc/README.md); the v3 divider is unverified.
- It works in every mode, including HiZ.
- When the optional SMPS owns the ADC, `ADC` stops the program with `Error(3)`
  instead of taking a sample.  The terminal probe commands use the same check
  and are refused the same way; a sample needs the SMPS stopped (see
  [SMPS](../../capabilities/smps/README.md)).

```basic
10 LET A=ADC
20 PRINT A;
30 END
```

## TIME

`TIME` is an expression-only token: the shared microsecond timer divided by
64, so one unit is **64 µs**.

- The terminal `t` prints the same timer in microseconds: 1 µs units, 31-bit,
  wrapping after about 35.8 minutes; `T` resets it.  BASIC `TIME` is `t / 64`:
  16-bit, wrapping about every 4.19 s.
- Measure an interval shorter than the wrap by subtracting two readings; the
  16-bit subtraction gives the elapsed count modulo 65536.
- `TIME` does not reset or reconfigure the timer.
- It works in every mode, including HiZ.

```basic
10 LET T=TIME
20 DELAY 100
30 PRINT TIME-T;
40 END
```

`RUN` prints the elapsed time in 64 µs units — about 1562 counts for the
100 ms delay.

## DELAY

`DELAY ms` is a busy wait in milliseconds, `0`..`65535`.  It works in every
mode, including HiZ, and is silent.

- Ctrl-C is not serviced during the delay; a long delay finishes first.
- The argument is an expression and is used as an unsigned 16-bit count, so a
  negative value becomes a very long delay (`DELAY 0-1` is 65535 ms).

## HiZ safety

In HiZ the protocol and bit-bang statements are silent no-ops, but every
statement on this page still acts:

| Statement | In HiZ |
|---|---|
| `PSU` | switches the regulator |
| `PULLUP` | enables the pull-ups (Vpullup must be fed) |
| `AUX` / `AUXPIN` | drives or reads the selected AUX target |
| `FREQ` / `DUTY` | starts PWM on `AUX` |
| `DELAY` | waits |
| `ADC` | samples the probe |
| `TIME` | reads the timer |

Unlike the terminal, which refuses `W`, `P` and `g` in HiZ, a BASIC program can
power or drive a connected circuit.  Disconnect the target, or set the states
deliberately, before running a HiZ program.  A run does not reset the board
when it ends.  `PSU 0`, `PULLUP 0` and `FREQ 0 50` switch off the regulator,
the pull-ups and the PWM output; `AUX 0` is a **drive-low**, not a release.  To
release the AUX pin, read it (`PRINT AUX;`) or reset the board.

## Pins and rails

| Operation | Pin or rail | Requirement |
|---|---|---|
| `PSU` | on-board regulator to the `5.0V` / `3.3V` supply pins | common ground; target connected to the supply; no `W` check |
| `PULLUP` | `Vpullup` pin, then the I/O lines | a voltage on `Vpullup` (external, or the v4 on-board Vpullup source selected at the terminal); the resistors do not create it |
| `AUX` / `AUXPIN` | `AUX`, `CS`, `AUX1`, `AUX2` | 3.3 V logic; common ground; no pull-up needed to drive, but a read needs a defined level |
| `FREQ` / `DUTY` | `AUX` pin | 3.3 V logic output into a load or driver input; no supply or pull-up needed; always `AUX`, not the selected target |
| `ADC` | ADC probe pin | 0–6.6 V; common ground; not the AUX pin |
| `DELAY`, `TIME` | none | internal |

## v3 / v4

The statements and ranges are the same in both builds, with these differences:

| Item | v3 | v4 |
|---|---|---|
| `AUXPIN` targets | `A0`, `CS` | `A0`, `CS`, `A1`, `A2` |
| on-board Vpullup source (`e`) | — | terminal only; `MODE` clears it |
| ADC divider | unverified | 6.6 V scale |
| PWM output | `AUX` pin | `AUX` pin |
