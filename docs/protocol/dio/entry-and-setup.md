# DIO — entry & setup

Part of the DIO documentation set (`docs/protocol/dio/`).  How the mode is
entered and engaged.  Wiring is in [wiring.md](wiring.md); the write values
are in [pin-control.md](pin-control.md).

## Scope

Covers entering DIO, the mode number, the absence of a settings menu, and how
the bus is engaged.  It does not cover the pin values themselves.

## Entering DIO

`DIO` is the last entry in the mode menu.  Its position depends on the build:

| Board | Menu number | Menu line |
|---|---|---|
| v4 | **10** | `10. DIO` |
| v3 | **9** (unverified) | `9. DIO` |

The v3 image has no `KEYB` entry, so every later mode sits one slot lower:
DIO is entry 10 on v4 and entry 9 on v3.  (The v3 number is unverified.)

As with every mode the menu form is `m`, Enter, then the number; the direct
form is `m10` (v4) or `m9` (v3).  The shared mode-menu grammar is owned
by `docs/generic/` — see [navigation.md](../../generic/navigation.md) §Mode
menu.

There is **no settings menu** and no prompt during entry: DIO has nothing to
configure, so entering it goes straight to the parking lines.  The mode has no
defaults beyond the shared 8-bit read/write width.

### Menu form

Choosing DIO from the menu prints the parking lines first.  The capture below
shows the full v4 menu (the v3 list is one entry shorter; the shared menu
grammar is in [navigation.md](../../generic/navigation.md) §Mode menu):

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

### Direct form

Choosing the mode directly (here on v4) prints only `Ready`, then the prompt:

```
HiZ>m10
Ready
DIO>
```

The menu form prints the two `Clutch disengaged!!!` / `To finish setup …`
lines; the direct form does not.  Both leave the bus parked.

## Engaging the bus

| Step | Command | Output |
|---|---|---|
| supply on | `W` | `POWER SUPPLIES ON` + `Clutch engaged!!!` |
| supply off | `w` | `Clutch disengaged!!!` + `POWER SUPPLIES OFF` |

`W` turns on the on-board supply rails (`+5.0V` / `+3.3V`).  If the rails do
not come up it prints `VREG too low, is there a short?` followed by
`POWER SUPPLIES OFF` and leaves the supply off.

`w` turns the supply off and restores the default 8-bit read/write width.  It
does **not** change pin directions, levels or pull-ups: pins that were
outputs stay outputs.  To park every pin as an input and return to `HiZ`, use
`#` instead (see [edge-cases.md](edge-cases.md)).

Unlike the bus modes, DIO has no setup step inside `W`; the supply is the only
engagement.  Pin writes take effect whether or not `W` was used: the `POWER`
bit inside a state value also controls the supply (see
[pin-control.md](pin-control.md)), so a state write can turn the supply on or
off on its own.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| mode number | 9 (unverified) | 10 |
| settings menu | none | none |
| `W` engagement | same | same |
| extra parking lines on the menu form | same | same |

The v3 column is **not verified**.