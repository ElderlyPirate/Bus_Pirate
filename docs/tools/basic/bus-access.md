# BASIC — bus access

Part of the BASIC documentation set (`docs/basic/`).  This page owns the
`MODE` statement, the mode numbers, and every statement that touches the bus.

## Scope

How a program selects a mode and drives or reads the bus.  The rest of the
language is in [language.md](language.md).  Wiring, pull-ups and supplies are
the active mode's — see [the generic layer](../../generic/README.md) and
`docs/<mode>/`.

## The `MODE` statement

`MODE` selects the bus mode from inside a program, or reports the active one.

| Statement | Action |
|---|---|
| `MODE` | print the active mode's name (no change) |
| `MODE expression` | switch to the mode that the mode menu lists as `expression`, apply that mode's **default** settings, and print the new mode's name |

- The argument is an expression, so `MODE M` works after `LET M=4`.
- The number is the **one-based** mode-menu number — the same list the `m`
  command shows.
- The switch runs the new mode's setup with every prompt taking its default
  (the settings you get by pressing Enter through the interactive setup), so
  the mode's pins are configured and the statements that follow can use the
  bus right away.
- A number outside the menu's range stops the program with a syntax error
  (error 3) and leaves the current mode unchanged.
- Used inside an expression, `MODE` yields the active mode's menu number.

Example:

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

## Mode numbers

The mode menu is build-dependent because it only lists the modes compiled
into the board.  The names are the firmware's own mode names.

| Number | v4 default build | v3 default build |
|---|---|---|
| 1 | `HiZ` | `HiZ` |
| 2 | `1-WIRE` | `1-WIRE` |
| 3 | `UART` | `UART` |
| 4 | `I2C` | `I2C` |
| 5 | `SPI` | `SPI` |
| 6 | `2WIRE` | `2WIRE` |
| 7 | `3WIRE` | `3WIRE` |
| 8 | `KEYB` | `PIC` |
| 9 | `PIC` | `DIO` |
| 10 | `DIO` | — |

Two points to keep straight:

- `KEYB` is present only on v4, which is why the last two numbers differ
  between the boards.  `MODE 9` is `PIC` on v4 but `DIO` on v3.
- `JTAG`, `SUMP` and `AVRISP` are **not** modes in this menu, so `MODE`
  cannot select them.

The `LCD` mode is not enabled in the default builds of either board.

## Bus statements

These act on the currently active mode.  In `HiZ` there is no bus protocol
behind them, so they have no visible effect.

| Statement | Action |
|---|---|
| `START` | protocol start |
| `STARTR` | protocol start with read |
| `STOP` | protocol stop |
| `STOPR` | protocol stop from read |
| `SEND value` | write a value to the bus |
| `RECEIVE` | a value read from the bus — **expression only** |
| `CLK value` | `0` clock low, `1` clock high, `2` clock pulse |
| `DAT value` | `1` data high, `0` data low; as an expression, reads the data-line state |
| `BITREAD` | one bit read from the bus — **expression only** |
| `AUX value` | `1` drive AUX high, `0` drive AUX low; as an expression, reads AUX |
| `AUXPIN value` | `1` alternate AUX function on, `0` off |
| `PSU value` | `1` power supplies on, `0` off; as an expression, reads the state |
| `PULLUP value` | `1` pull-up resistors on, `0` off; as an expression, reads the state |
| `DELAY ms` | pause for `ms` milliseconds |
| `FREQ hz duty` | set the PWM output: `hz` 0..4000 (0 = off), `duty` 1..100 % |
| `DUTY duty` | change the PWM duty cycle (1..100 %) |
| `MACRO n` | run the active mode's macro `n` |
| `ADC` | the voltage-probe ADC reading — **expression only** |
| `TIME` | the microsecond timer in 64 µs units — **expression only** |

Notes:

- `SEND` and `RECEIVE` honour the mode's LSb/MSb bit-order setting, exactly as
  the interactive `8` and `r` commands do.
- `FREQ`/`DUTY` clamp out-of-range values into 0..4000 Hz and 1..100 %.
- `TIME` yields a 16-bit value that wraps about every 4.19 seconds, so
  measure an interval by subtracting two readings.
- Values sent with `SEND` and used with `CLK`/`DAT` are 16-bit; the meaning of
  the bits is the active mode's.
- `RECEIVE`, `BITREAD`, `ADC` and `TIME` cannot begin a line — using them as a
  statement is a syntax error.  They are read tokens for expressions.

## v3 / v4

`MODE`, the argument rules and every bus statement are the same in both
builds.  Only the mode-number table differs (above), because v4 adds `KEYB`.
v3 behaviour is **not verified**.