# BASIC — bus access

Part of the BASIC documentation set (`docs/tools/basic/`).  This page owns the
`MODE` statement, the mode numbers, the mapping to the terminal commands, and
every statement that touches the board.

## Scope

How a program selects a mode and drives or reads the bus.  The rest of the
language is in [language.md](language.md).  Protocol wiring, pull-ups and
supplies belong to the active mode; the board-level statements have their own
pins and rails ([below](#board-level-operations)).

## The `MODE` statement

| Statement | Action |
|---|---|
| `MODE` | print the active mode's name (no change) |
| `MODE expression` | switch to mode `expression`, apply its defaults, print its name |

The number is the **one-based** mode-menu number — the same list the `m`
command shows.  The argument is an expression, so `MODE M` works after
`LET M=4`.

### What a `MODE` switch resets

`MODE` runs the same board-state reset as an interactive mode change before it
applies the new mode's setup.  After `MODE n`:

| Setting | State |
|---|---|
| power supplies | off |
| pull-up resistors | released |
| on-board VPU source (v4) | off |
| AUX target | `A0` (the pin shown as `AUX`) |
| bit order | MSB-first |
| transfer width | 8 bits (the mode setup can change it) |
| mode-specific settings | the new mode's defaults (the interactive setup's Enter answers) |

BASIC has **no equivalent** of the terminal `e` (VPU source), `L`/`l` (bit
order) or `;N` (width) controls.  If a program needs one of those, set it at
the terminal **before** `s` and do not use a `MODE` statement — a `MODE`
switch clears it again.  Otherwise accept the defaults above.

A number outside the menu's range stops the program with a syntax error
(error 3) and leaves the current mode unchanged.  Used inside an expression,
`MODE` yields the active mode's menu number.

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

`s` on its own does not change the mode and does not finish an interactive
mode setup.  Either complete the setup at the terminal (including `W`, which
engages the bus) before `s`, or let the first `MODE` statement apply the mode
setup from inside the program (it does not switch the supplies on).

## Mode numbers

The mode menu is build-dependent because it only lists the modes compiled into
the board.  The names are the firmware's own mode names.

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

- `KEYB` is present only on v4, which is why the last numbers differ.
  `MODE 9` is `PIC` on v4 but `DIO` on v3.
- `JTAG`, `SUMP` and `AVRISP` are not mode-menu entries, so `MODE` cannot
  select them.
- `LCD` is off in the default builds.  A custom build that enables it lists
  `LCD` after `KEYB` and before `PIC`, so `PIC` and `DIO` become 10 and 11; a
  custom v3 build can also gain `KEYB`.  Take the numbers from the `m` menu of
  the build in front of you.

The firmware's mode strings are `1WIRE`, `2WIRE` and `3WIRE`; prose may write
1-Wire, 2-Wire and 3-Wire for the same modes.

## Statements and the terminal commands

Every bus statement has a terminal equivalent.  The terminal commands are the
authority for their own wiring; this table maps the two grammars.

| BASIC form | Terminal command | Action |
|---|---|---|
| `MODE` / `MODE n` | `m` | report / change the mode |
| `START` / `STARTR` | `[` / `{` | start / start with read |
| `STOP` / `STOPR` | `]` / `}` | stop / stop from read |
| `SEND value` | `123`, `0x7B`, `0b1111011`, `"text"` | send a value |
| `RECEIVE` (expression) | `r` | read a value |
| `CLK 0` / `CLK 1` / `CLK 2` | `\` / `/` / `^` | clock low / high / tick |
| `DAT 0` / `DAT 1` | `_` / `-` | data low / high |
| `DAT` (expression) | `.` | read the data line |
| `BITREAD` (expression) | `!` | read one bit |
| `AUX 0` / `AUX 1` / `AUX` | `a` / `A` / `@` | AUX low / high / read |
| `AUXPIN 0` / `AUXPIN 1` | `c` / `C` | AUX target `A0` / `CS` |
| `PSU 0` / `PSU 1` / `PSU` | `w` / `W` / `i` (non-HiZ) | supply off / on / state |
| `PULLUP 0` / `PULLUP 1` / `PULLUP` | `p` / `P` / `i` (non-HiZ) | pull-ups off / on / state |
| `DELAY ms` | `%` | delay `ms` milliseconds |
| `ADC` (expression) | `d` | one ADC sample |
| `TIME` (expression) | `t` | elapsed timer, 64 µs units |
| `MACRO n` | `(n)` | run mode macro `n` |
| `FREQ` / `DUTY` | `g` | PWM — **not usable from BASIC** (below) |

`RECEIVE`, `BITREAD`, `ADC` and `TIME` are **read tokens**: use them inside an
expression (`PRINT RECEIVE;`, `LET T=TIME`), not as a statement.  A line that
begins with one stops with a syntax error.

## Protocol operations

These go through the active mode's protocol callbacks.  A `–` means the mode
installs no handler for that operation.  During a program the firmware's
`ERROR: command has no effect here` message is suppressed, so the operation is
a **silent no-op**; a read token returns `0`.

| Mode | `START`/`STARTR` | `STOP`/`STOPR` | `SEND` | `RECEIVE` | `MACRO` |
|---|---|---|---|---|---|
| HiZ | – | – | – | – | – |
| 1WIRE | reset | – | yes | yes | yes |
| UART | yes | yes | yes | yes | yes |
| I2C | yes | yes | yes | yes | yes |
| SPI | yes | yes | yes | yes | yes |
| 2WIRE | yes | yes | yes | yes | yes |
| 3WIRE | yes | yes | yes | yes | – |
| KEYB | – | – | yes | yes | yes |
| PIC | yes | yes | yes | yes | yes |
| DIO | – | – | yes | yes | – |

- In `1WIRE`, `START` and `STARTR` both issue the reset pulse.
- `RECEIVE` and `BITREAD` are read tokens, not statements: use them inside an
  expression.
- `MACRO n` runs the same macro as the terminal's `(n)`.  There are no mode
  macros in HiZ, 3WIRE or DIO.  Some mode macros are interactive (a menu, a
  bus sniffer) and do not return until you finish them, so a program stops
  there.
- `SEND` and `RECEIVE` use the active bit order and the mode's transfer width,
  exactly as the terminal value entry and `r` do.  The value is 16-bit in the
  language, but the number of bits that reach the bus is the mode's width —
  8 bits after a `MODE` switch (the UART setup can select 9-bit).  BASIC has
  no `;N` width setting; set the width at the terminal before `s`, and avoid
  `MODE` if you need it.

## Bit-bang operations

`CLK`, `DAT` and `BITREAD` work only in the raw bit-bang modes.  In every other
mode they are silent no-ops.

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
| PIC | – | – | – | – | – |
| DIO | – | – | – | – | – |

`CLK 0`/`CLK 1` set the clock line; `CLK 2` issues one clock pulse.  `DAT 1`
and `DAT 0` drive the data line.  `DAT` read and `BITREAD` are read tokens and
return a single bit; use them in an expression, not as a statement.

## Board-level operations

These are direct hardware calls, not protocol callbacks.  They work in **every**
mode, including HiZ:

| Operation | Action | Notes |
|---|---|---|
| `PSU 0` / `PSU 1` | switch the on-board regulator off / on | raw switch; no `W` voltage check or bus-engagement step |
| `PULLUP 0` / `PULLUP 1` | release / enable the pull-up resistors | needs a voltage on **Vpullup**; the resistors do not create one |
| `AUX 0` / `AUX 1` | drive the selected AUX target low / high | target selected with `AUXPIN` |
| `AUXPIN 0` / `AUXPIN 1` | select the AUX target: `0` = `A0`, `1` = `CS` | `A1`/`A2` unreachable from BASIC |
| `DELAY ms` | busy delay in milliseconds | Ctrl-C is not serviced during it |
| `ADC` | the probe ADC reading — **expression only** | raw `0`..`1023`; volts = `ADC / 1024 × 6.6` |
| `TIME` | the elapsed timer — **expression only** | 64 µs units; 16-bit, wraps about every 4.19 s |

`PSU`, `PULLUP` and `AUX` work both as statements and as read tokens; `ADC` and
`TIME` are read tokens only.  A line that begins with `ADC` or `TIME` stops
with a syntax error.

### AUX target

`AUXPIN` is a **target selector**, not an on/off flag.  The interpreter stores
its argument as `1` or `0`, so only the first two terminal assignments are
reachable:

| `AUXPIN` | Target | Terminal command | Pin name |
|---|---|---|---|
| `0` | `A0` | `c` | `AUX` |
| `1` | `CS` | `C` | `CS` |

`AUX` drives or reads whichever target is selected.  The v4 targets `A1` and
`A2` (terminal `k`/`K`) cannot be selected from a program.

### The `PULLUP` expression

As an expression, `PULLUP` returns the **raw** status value, which is the
complement of the pull-up pin: `65534` when the pull-ups are on and `65535`
when they are off.  It is never `1` or `0` and is always non-zero, so
`IF PULLUP THEN` is always true.  Read the state with `i` in a non-HiZ mode
instead, or track it in a variable.

### Safety in HiZ

In HiZ the protocol statements are no-ops, but the board-level statements still
act.  Unlike the terminal, which refuses `W`, `P` and `g` in HiZ, a BASIC
program can switch the regulator, enable the pull-ups, drive the selected AUX
pin, run a delay, take ADC readings and read the timer.  A HiZ program can
therefore power or drive a connected circuit.  Disconnect the target, or set
the states deliberately, before running.

## PWM is not usable from BASIC

`FREQ hz duty` and `DUTY duty` are in the statement set, but the current
interpreter cannot parse them: it reads the arguments from the wrong place and
also removes the space between the two numbers, so a two-argument form is never
stored correctly.  A program that uses either statement stops with an error
instead of setting PWM.  Generate PWM from the terminal with `g` instead.

## v3 / v4

`MODE`, the argument rules and the protocol statements are the same in both
builds; only the mode-number table differs above.  The other build differences
are in [limits-and-variants.md](limits-and-variants.md).
