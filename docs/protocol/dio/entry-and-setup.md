# DIO — availability, entry and setup

Part of the DIO documentation set (`docs/protocol/dio/`).  This page owns the
availability table, both entry forms, and what entering the mode does to the
board.  Wiring is in [wiring.md](wiring.md); the full park contract is in
[park-and-reset.md](park-and-reset.md).

## Scope

Covers which builds have direct I/O, the mode number, entering it, the absence
of a settings menu, and the supply and pull-up commands used around entry.  It
does not cover the pin values themselves; those are in
[pin-control.md](pin-control.md).

## Availability

| Item | v3 | v4 |
|---|---|---|
| build | included in the default image | included in the default image |
| build option | on by default; a build can leave it out | same |
| menu number | **9** | **10** |
| prompt | `DIO>` | `DIO>` |
| settings menu | none | none |
| numeric direction / state writes | yes | yes |
| `r` level read | yes | yes |
| `v` pin table | 10 columns | 12 columns |
| `a` / `A` / `@` on `AUX` / `CS` | yes | yes |
| `a` / `A` / `@` on `AUX1` / `AUX2` | — | yes |
| `P` / `p` pull-ups | yes | yes |
| `e` VPU source | absent | refused |
| `#` | hardware reset | terminal reset to `HiZ` |
| mode macros `(n)` | none | none |
| user macros `<n>` | yes | yes |

Hardware needed: a target or probe on the signal pins.  No external component
is required for basic pin control, and no pull-up is required.

Direct I/O is compiled into both default images.  A build that leaves it out
has no `DIO` entry in the mode menu; the build options are listed in
[building.md](../../dev/building.md).  The v3 column is not verified on
v3 hardware.

## Entering DIO

The mode menu is dynamic: it lists only the modes compiled into the build, in
a fixed order.  The default v4 list ends with `PIC` at 9 and `DIO` at 10.  The
default v3 build omits `KEYB`, so `PIC` is 8 and `DIO` is 9.  A custom build
that adds optional modes (for example the LCD adapter, or `KEYB` on v3)
renumbers the menu; use the number the menu prints.

| Form | Input | Reply |
|---|---|---|
| menu | `m`, Enter, then `10` at `(1)>` (v4) or `9` (v3) | `Clutch disengaged!!!`, the finish-setup line, `Ready`, `DIO>` |
| direct | `m10` (v4) or `m9` (v3) | `Ready`, `DIO>` |

The menu form on v4:

```
HiZ>m
1. HiZ
2. 1WIRE
3. UART
4. I2C
5. SPI
6. 2WIRE
7. 3WIRE
8. KEYB
9. PIC
10. DIO
x. exit(without change)

(1)>10
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
DIO>
```

The blank line before `(1)>` is printed by the prompt reader.  At `(1)>`,
`x` cancels and prints `no mode change`; an out-of-range number prints
`Invalid choice, try again` and re-prompts.  A direct form with a number that
is not a mode, such as `m11` on v4, prints `Nonexistent protocol!` and leaves
the current mode active.  The direct form on v4:

```
HiZ>m10
Ready
DIO>
```

The two engagement lines in the menu form are the shared announcement for a
non-`HiZ` mode.  They are not a state change: in both forms the supply is off
and all I/O pins are inputs.  Direct I/O has no settings to finish, so the
finish-setup line has nothing to do; `W` only turns the rails on and runs an
empty setup hook.

## No settings menu

There is no prompt chain and no `W`-time configuration.  Entering the mode
applies a fixed base state.  `W` starts the rails and runs the mode's setup
hook, which is empty in direct I/O: it does not change directions, levels,
latches or pull-ups.

| Setting | Value after entry |
|---|---|
| width | 8 bits |
| bit order | MSB-first |
| read-on-write | off |
| output type | push-pull (fixed) |
| AUX target | `AUX` (`A0`) |
| display format | unchanged from the previous mode |
| user macros | unchanged |

The output type is fixed: direct I/O never selects open drain, so there is no
`Select output type:` prompt.  `o` is the **display** format (HEX, DEC, BIN,
RAW), not an output type; the `?` help row `o  Set output type` is a misnomer
that applies to every mode ([values.md](../../generic/values.md)).

## Supply and pull-ups

| Input | Effect | Notes |
|---|---|---|
| `W` | on-board `+5.0V` and `+3.3V` rails on | rail check; `POWER SUPPLIES ON` then `Clutch engaged!!!` |
| `w` | rails off **and five pins (v3) / seven pins (v4) parked** | `Clutch disengaged!!!` then `POWER SUPPLIES OFF`; stays in `DIO>` |
| `P` | pull-up resistors on | prints the open-drain warning first; samples VPU and warns if it is low |
| `p` | pull-up resistors off | — |
| state bit 6 | supply on / off | silent; no rail check and no `POWER SUPPLIES ON` line |
| state bit 5 | pull-ups on / off | silent; no VPU sample |
| `e` | refused on v4; absent on v3 | direct I/O is push-pull, so no on-board VPU source |

`W` and `w` are the shared engagement commands.  In direct I/O, `W` adds the
rails and runs the mode's setup hook, which is empty: it does not change pin
directions, levels or pull-ups.  `w` runs the mode's safe park, which releases
the five signal pins (and, on v4, `AUX1` / `AUX2`) before the rails go off.
The full contract, including the `w` versus `#` difference, is in
[park-and-reset.md](park-and-reset.md).  Wiring requirements for the rails and
for VPU are in [wiring.md](wiring.md).

Pin writes do not need `W`: a state value with bit 6 set can switch the supply
on by itself, and a direction or level value works with the supply off.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| menu number | 9 | 10 |
| menu list | no `KEYB`; `PIC` is 8 | `KEYB` is 8; `PIC` is 9 |
| settings menu | none | none |
| `W` / `w` | `W` same; `w` parks five pins | `W` same; `w` parks seven pins |
| `#` | prints `RESET`, then reboots | prints `RESET`, then the banner, back at `HiZ>` |
