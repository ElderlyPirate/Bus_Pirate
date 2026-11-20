# BASIC — bus access and MODE

Part of the BASIC documentation set (`docs/tools/basic/`).  This page owns the
`MODE` statement, the mode numbers, the statement-to-terminal mapping, the
protocol and bit-bang operations, `SEND`/`RECEIVE`/`BITREAD` and `MACRO`.
The board-level statements (`PSU`, `PULLUP`, `AUX`, `AUXPIN`, `FREQ`, `DUTY`,
`ADC`, `TIME`, `DELAY`) are in [board-pins.md](board-pins.md).

## Scope

How a program selects a mode and drives or reads the bus.  The rest of the
language is in [language.md](language.md).  Protocol wiring, pull-ups and
supplies belong to the active mode; the per-mode summary is
[below](#wiring-per-mode).

## The MODE statement

| Form | Action |
|---|---|
| `MODE` | print the active mode's name (no change) |
| `MODE expression` | switch to mode `expression`, apply its defaults, print its name |

The number is the **one-based** mode-menu number — the same list the `m`
command shows.  The argument is an expression, so `MODE M` works after
`LET M=4`.  Used inside an expression, `MODE` yields the active mode's number.

A successful switch prints the new mode's name, for example:

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

A number outside the menu stops the program with `Error(3)` and leaves the
current mode unchanged.  The menu lists only the modes the build includes, so
it is also the set of valid numbers.

### What a MODE switch resets

`MODE` runs the previous mode's cleanup, the shared board-state reset, and then
the new mode's setup, including the engage step, with every prompt taking its
default answer.  After `MODE n`:

| Setting | State |
|---|---|
| I/O pins | released to inputs; the new mode's setup then configures the pins it uses |
| power supplies | off |
| pull-up resistors | released |
| on-board Vpullup source (`e`, v4) | off |
| AUX target | `A0` — the pin shown as `AUX` |
| PWM output | detached; a later `DUTY` reattaches it at the last BASIC `FREQ` |
| bit order | MSB-first |
| transfer width | 8 bits; the new mode's setup can change it (UART 9-bit, `PIC` 14/16) |
| output type | push-pull after the reset; the mode's default setup can select open drain |
| write-with-read | off (`STARTR` turns it on) |
| mode settings | the new mode's defaults, the same answers as pressing Enter through the interactive setup |
| bus engagement | the new mode's setup/engage step runs (the step `W` runs), without `W`'s voltage checks or `Clutch engaged!!!` message |
| supplies | **not** turned on; `MODE` does not run `W`'s voltage checks or messages |

BASIC has **no equivalent** of the terminal `e` (Vpullup source), `L`/`l`
(bit order), `;N` (width) or the output-type prompt.  If a program needs one of
those, set it at the terminal **before** `s`, and do not use a `MODE`
statement — a `MODE` switch clears it again.  Otherwise accept the defaults
above.

Because the reset also resets the supplies, pull-ups, AUX target and PWM, order
a program as `MODE`, then the states you want
(`PSU`/`PULLUP`/`AUXPIN`/`FREQ`), then the bus statements.

`MODE` leaves the bus engaged but unpowered.  A target with its own supply
needs no `PSU` statement; `PSU 1` is only for a target that draws power from
the board's `5.0V` / `3.3V` pins, and must not be used into a self-powered
target.

`s` on its own does not change the mode and does not finish an interactive
mode setup.  Either complete the setup at the terminal (including `W` where
the mode uses it) before `s`, or let the first `MODE` statement apply and
engage the mode setup from inside the program.

## Mode numbers

The menu is build-dependent because it lists only the modes compiled into the
board.  The names are the firmware's own mode names.

| Number | v4 default build | v3 default build |
|---|---|---|
| 1 | `HiZ` | `HiZ` |
| 2 | `1WIRE` | `1WIRE` |
| 3 | `UART` | `UART` |
| 4 | `I2C` | `I2C` |
| 5 | `SPI` | `SPI` |
| 6 | `2WIRE` | `2WIRE` |
| 7 | `3WIRE` | `3WIRE` |
| 8 | `KEYB` | `PIC` |
| 9 | `PIC` | `DIO` |
| 10 | `DIO` | – |

- `KEYB` is present only on v4 in the default image, which is why the last
  numbers differ.  `MODE 9` is `PIC` on v4 but `DIO` on v3.
- `JTAG`, `SUMP` and `AVRISP` are not mode-menu entries, so `MODE` cannot
  select them.
- `LCD` is off in the default builds.  A custom build that enables it lists
  `LCD` after `KEYB` and before `PIC`, so `PIC` and `DIO` become 10 and 11; a
  custom v3 build can also gain `KEYB`.  Take the numbers from the `m` menu of
  the build in front of you.
- The firmware's mode strings are `1WIRE`, `2WIRE` and `3WIRE`; prose may
  write 1-Wire, 2-Wire and 3-Wire for the same modes.

## Statements and the terminal commands

Every bus statement has a terminal equivalent.  The terminal commands are the
authority for their own wiring; this table maps the two grammars.

| BASIC form | Terminal command | Action |
|---|---|---|
| `MODE` / `MODE n` | `m` / `m<N>` | report / change the mode |
| `START` / `STARTR` | `[` / `{` | start; `STARTR` also sets write-with-read where the mode has it |
| `STOP` / `STOPR` | `]` / `}` | stop |
| `SEND value` | value entry: `123` (the terminal prompt also takes `0x7B`, `0b1111011` and `"text"`) | send one decimal value |
| `RECEIVE` (expression) | `r` | read a value |
| `CLK 0` / `CLK 1` / `CLK 2` | `\` / `/` / `^` | clock low / high / pulse |
| `DAT 0` / `DAT 1` | `_` / `-` | data low / high |
| `DAT` (expression) | `.` | read the data line |
| `BITREAD` (expression) | `!` | read one bit |
| `AUX 0` / `AUX 1` / `AUX` | `a` / `A` / `@` | AUX target low / high / read |
| `AUXPIN n` | `c` / `C` / `k` / `K` | select the AUX target |
| `PSU 0` / `PSU 1` / `PSU` | `w` / `W` / `i` (non-HiZ) | supply off / on / state |
| `PULLUP 0` / `PULLUP 1` / `PULLUP` | `p` / `P` / `i` (non-HiZ) | pull-ups off / on / state |
| `DELAY ms` | `%:N` | delay `ms` milliseconds |
| `ADC` (expression) | `d` | one ADC sample |
| `TIME` (expression) | `t` | elapsed timer; BASIC counts 64 µs units (16-bit), terminal `t` is 1 µs (31-bit) |
| `MACRO n` | `(n)` | run mode macro `n` |
| `FREQ hz duty` / `DUTY duty` | `g` | PWM — BASIC uses Hz and a different range (see [board-pins.md](board-pins.md#pwm-freq-and-duty)) |

`RECEIVE`, `BITREAD`, `ADC` and `TIME` are **read tokens**: use them inside an
expression (`PRINT RECEIVE;`, `LET T=TIME`), not as a statement.  A line that
begins with one stops with `Error(3)`.  The bus read token is `RECEIVE`;
`READ` is the `DATA` statement in [language.md](language.md) and does not read
the bus.

## Protocol operations

These go through the active mode's protocol callbacks.  A `–` means the mode
installs no handler for that operation.  During a program the firmware's
`ERROR: command has no effect here` message is suppressed, so the operation is
a **silent no-op**; a read token returns `0`.

The interpreter runs quiet: the terminal's `WRITE:`/`READ:`, start/stop and
error lines do not appear during a run.  Use `PRINT` to show values; a `MODE`
statement prints the mode name and `INPUT` prints its prompt.

| Mode | `START`/`STARTR` | `STOP`/`STOPR` | `SEND` | `RECEIVE` | `MACRO` |
|---|---|---|---|---|---|
| HiZ | – | – | – | – | – |
| 1WIRE | reset | – | yes | yes | yes |
| UART | display only | display only | yes | yes | yes |
| I2C | yes | yes | yes | yes | yes |
| SPI | yes | yes | yes | yes | yes |
| 2WIRE | yes | yes | yes | yes | yes |
| 3WIRE | yes | yes | yes | yes | – |
| KEYB | – | – | yes | yes | yes |
| LCD (custom build) | yes | yes | yes | – | yes |
| PIC | yes | yes | yes | yes | yes |
| DIO | – | – | yes | yes | – |

- In `1WIRE`, `START` and `STARTR` both issue the reset pulse; `STARTR` does
  not read.
- `STARTR` turns on the mode's write-with-read flag where it exists (`SPI` and
  `3WIRE`): a following `SEND` also clocks a byte in, and `SEND` used in an
  expression yields that byte.
- `STOPR` behaves like `STOP` in every mode.
- In `UART`, `START` and `STOP` only switch the terminal's live display and
  echo on and off; neither puts a byte on the wire.  `SEND` and `RECEIVE`
  need no wrapper.
- `MACRO n` runs the same macro as the terminal's `(n)`.  There are no mode
  macros in HiZ, 3WIRE or DIO.  Some mode macros are interactive (a menu, a
  bus sniffer) and run their own input loop, so a program stops there; the
  interpreter keeps the board quiet, so the macro's prompts are not visible.
  Press the board's reset button or power-cycle to recover from a macro that
  waits for input; `#` cannot be typed while the macro holds the interpreter.
- `SEND` and `RECEIVE` use the active bit order and the mode's transfer width,
  exactly as the terminal value entry and `r` do.  The value is 16-bit in the
  language, but the number of bits that reach the bus is the mode's width — 8
  bits in most modes after a `MODE` switch, 9 bits when the UART setup selects
  9-bit, and 14 or 16 for `PIC` data words.  BASIC has no `;N` width setting;
  set it at the terminal before `s`, and avoid `MODE` if you need it.

## SEND and RECEIVE

`SEND value` writes one value through the mode's send callback.  `RECEIVE` is
an expression that reads one value through the read callback.  Both honour the
current bit order: the terminal `L` setting reverses the value over the
configured width, and `MODE` resets that to MSB-first.

`SEND` is also accepted inside an expression.  It sends its argument and
yields the mode's write reply:

| Context | Reply |
|---|---|
| `SPI` or `3WIRE` with `STARTR` (write-with-read) | the byte clocked in |
| `UART` | `0` |
| a mode with no write handler (`HiZ`) | `256` |

Other modes return their own write status or `0`; only the SPI/3WIRE
write-with-read case is generally useful.  Most programs use `RECEIVE` for data
and `SEND` as a statement.

`SEND` takes a decimal number only.  `SEND 0x7B` sends `0` and then stops with
`Error(2)` (the `x` is trailing text); `SEND "A"` stops with `Error(3)`.

## Bit-bang operations

`CLK`, `DAT` and `BITREAD` work in the raw bit-bang modes (`2WIRE`, `3WIRE`)
and, for the data and pulse forms, in `1WIRE`.  In every other mode they are
silent no-ops.

| Mode | `CLK 0`/`CLK 1` | `CLK 2` (pulse) | `DAT 0`/`DAT 1` | `DAT` (read) | `BITREAD` |
|---|---|---|---|---|---|
| HiZ | – | – | – | – | – |
| 1WIRE | – | yes | yes | yes | yes |
| UART | – | – | – | – | – |
| I2C | – | – | – | – | – |
| SPI | – | – | – | – | – |
| 2WIRE | yes | yes | yes | yes | yes |
| 3WIRE | yes | yes | yes | yes | yes |
| KEYB | – | – | – | – | – |
| LCD (custom build) | – | – | – | – | – |
| PIC | – | – | – | – | – |
| DIO | – | – | – | – | – |

`CLK 0` and `CLK 1` set the clock line; `CLK 2` issues one clock pulse.  In
`2WIRE` and `3WIRE`, `DAT 1` and `DAT 0` drive the data output (`MOSI`) and
`DAT` reads the data input (`MISO`).  `BITREAD` returns a single bit; use it in
an expression, not as a statement.

`1WIRE` is special: `DAT 0`/`DAT 1` only record the bit that the next `CLK 2`
shifts out, and `DAT` read returns that recorded bit, not the bus level.  Use
`BITREAD` to sample a device bit.

## Wiring per mode

BASIC adds no wiring of its own: each bus statement uses the active mode's
pins.  The pull-up entry is about the **on-board** pull-up resistors, which
`PULLUP 1` enables and which need a voltage on **Vpullup**; a target with its
own pull-ups does not need them.

| Mode | Pins | Pull-up and supply |
|---|---|---|
| 1WIRE | data on `MOSI`, common `GND` | **pull-up 2–10 kΩ to Vpullup required**; the target may be powered separately or parasitically |
| UART | `TX`→`MOSI`, `RX`→`MISO`, common `GND` | default setup is open drain, so a pull-up is required unless the target provides one; full-duplex Normal needs none; half-duplex single wire uses `MOSI` only plus 1–10 kΩ |
| I2C | `SDA`→`MOSI`, `SCL`→`CLK`, common `GND` | **pull-up 2–10 kΩ on each line required** |
| SPI | `CS`, `MOSI`, `MISO`, `CLK`, common `GND` | default setup is open drain, so a pull-up is required unless the target provides one; select Normal at the terminal if you have none |
| 2WIRE | `SDA`/`SCL` | open drain needs 2–10 kΩ; Normal needs none |
| 3WIRE | `MOSI`, `MISO`, `CLK`, `CS` | open drain needs 2–10 kΩ; Normal needs none |
| KEYB | `DATA`, `CLOCK`, common `GND` | a pull-up on each line (the keyboard's own or 1–10 kΩ); the keyboard needs its own supply |
| PIC | `PGC`, `PGD`, `VPP`/`MCLR` | **pull-up on `PGC`/`PGD` required**; the target needs its programming supply |
| DIO | the five signal pins, common `GND` | **no pull-up required**; the target supply is optional |

The default setup that `MODE` applies is open drain for every bus mode except
`DIO`.  A program that uses `MODE` and wants push-pull outputs must select the
mode at the terminal, answer Normal at the output-type prompt, and avoid
`MODE`.  On v4 the on-board Vpullup source is also cleared by `MODE`; the
terminal `e` menu is the only way to select it.  The per-mode sets, which own
the full pin tables, are linked from the mode table in
[generic/README.md](../../generic/README.md).

## v3 / v4

`MODE`, the argument rules, the mapping and the protocol statements are the
same in both builds; the mode-number table and the `AUXPIN` range differ.
`AUXPIN` accepts 0 and 1 on v3 and 0..3 on v4; a larger value stops the
program with `Error(3)` and leaves the target unchanged.
