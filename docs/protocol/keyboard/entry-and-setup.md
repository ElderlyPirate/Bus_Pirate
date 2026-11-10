# PC keyboard (KEYB) — entry & setup

Part of the KEYB documentation set (`docs/protocol/keyboard/`).

## Scope

How KEYB is entered, that it has no setup prompts, and the mode's fixed
defaults.  Wiring is in [wiring.md](wiring.md); using the mode is in
[usage.md](usage.md).

## Mode number and the mode menu

KEYB is **mode 8** in the default v4 build: the eighth entry of `i`'s
`[ … ]` list, between 3WIRE and PIC.

| Menu slot | Mode |
|---|---|
| 1 | HiZ |
| 2 | 1WIRE |
| 3 | UART |
| 4 | I2C |
| 5 | SPI |
| 6 | 2WIRE |
| 7 | 3WIRE |
| **8** | **KEYB** |
| 9 | PIC |
| 10 | DIO |

The menu is dynamic — it follows the build's enabled modes — but in the
default v4 image KEYB is 8 and the whole `i` list is
`[HiZ 1WIRE UART I2C SPI 2WIRE 3WIRE KEYB PIC DIO]`.  `i` prints the enabled
modes in menu order; it does not number them.  The shared `m` menu and entry
grammar are owned by `docs/generic/`
([navigation.md](../../generic/navigation.md)).

## Entering KEYB

| Input | Behaviour | Prints |
|---|---|---|
| `m`, then `8` | menu form, then the mode | the menu, then the lines in the menu-form capture below |
| `m8` (or `m 8`) | direct form | `Ready`, then `KEYB>` |

KEYB has **no setup prompts** — there is no `(1)>` question to answer, unlike
UART, I2C or SPI.  Both forms land at the `KEYB>` prompt.

Menu form:

```
HiZ>m
1. HiZ
2. 1WIRE
...
8. KEYB
...
(1)>8
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
KEYB>
```

Direct form:

```
HiZ>m8
Ready
KEYB>
```

## Engaging the bus — `W`

Until `W` is sent the keyboard has no supply, so it does not answer.  `W`
switches on the 5 V rail (which powers the keyboard) and runs the mode's pin
setup:

```
KEYB>W
POWER SUPPLIES ON
Clutch engaged!!!
KEYB>
```

Send `W` before any read, write, or the monitor.  `w` powers the keyboard down
again and parks the bus.  If the rail does not come up the firmware refuses:

```
KEYB>W
VREG too low, is there a short?
POWER SUPPLIES OFF
KEYB>
```

The supplies and the `W`/`w` behaviour are base-layer features
([commands.md](../../generic/commands.md)); this page records only that **KEYB
needs `W` for the keyboard's supply**.

## Defaults

KEYB has no setup questions, so its settings are fixed:

| Setting | Value | Where it shows |
|---|---|---|
| bits per value | 8 | `i` status: `Number of bits read/write: 8` |
| output type | open drain (`H=Hi-Z, L=GND`) | `i` status |

The mode sets its output type to open drain automatically; there is no output
type prompt to answer.  The status block is printed by `i` when the mode is not
HiZ ([navigation.md](../../generic/navigation.md)).

## Pin state display — `v`

`v` prints the rail/pin table.  The table is assembled from three
contributions:

| Rows | Supplied by | Content |
|---|---|---|
| rail and colour | the base layer | the rail names and the connector colour names |
| signal names | **the mode** | `CS MISO CLK MOSI` |
| direction and state | the base layer | `I` / `O` per pin, then `H` / `L` from the live levels |

The mode contributes the signal-name row only, so the active pins are
labelled:

```
CS      MISO    CLK     MOSI
```

Only **MOSI** (keyboard data) and **CLK** (keyboard clock) carry the keyboard
interface; `CS` and `MISO` are unused here.  The direction and state rows are
filled in by the base layer from the pin setup, not by the mode.  The full `v`
table layout is described in
[the generic usage notes](../../generic/usage.md).

## v3 / v4

KEYB is **v4 only**.  On v3 the mode is not compiled in: it is absent from the
mode menu and the `i` banner, and there is no equivalent.
