# I2C — entry & setup

Part of the I2C documentation set (`docs/protocol/i2c/`).  How the mode is
entered, how the driver and its speed are chosen, and what the one-line
settings form accepts.  Wiring is in [wiring.md](wiring.md).

## Entering I2C

`I2C` is mode number **4** in the mode menu (HiZ is 1).  As with every mode the
menu form is `m` then `4`; the direct form is `m4`.  The shared mode-menu
grammar is owned by [docs/generic/](../../generic/README.md) — see
[navigation.md](../../generic/navigation.md) §Mode menu.

On **v4** — and on a custom v3 build that compiles in the hardware engine, not
the default v3 build — entering the mode first asks the driver sub-menu:

| Prompt | Option | Text | Driver |
|---|---|---|---|
| `I2C mode:` | `1` | `Software` | bit-banged SDA/SCL |
| | `2` | `Hardware` | the on-chip I2C engine |

**Rule of thumb:** choose **Software** unless you need the on-board EEPROM
macros (v4 only) or want the hardware engine's rates.  Software works on both
boards and is what the binary `I2C1` path always uses.

A default **v3** build has no hardware engine compiled in, so it **skips the
sub-menu** and goes straight to the software speed prompt.  The sub-menu
strings still exist in the v3 message table, but that build never shows them.

### Software driver speed menu

| Prompt | Option | Text |
|---|---|---|
| `Set speed:` | `1` | `~6kHz` |
| | `2` | `~60kHz` |
| | `3` | `~150kHz` |
| | `4` | `Maximum speed` |

### Hardware driver speed menu

| Prompt | Option | Text |
|---|---|---|
| `Set speed:` | `1` | `100kHz` |
| | `2` | `400kHz` |
| | `3` | `1MHz` |

The rates are nominal: the actual SCL rate is lower and depends on bus rise
time and load (see [wiring.md](wiring.md)).  _not verified_

The two menus are **different**: the software driver offers four bit-bang
profiles, the hardware driver three engine rates.  The default answer to both
is `(1)`, so pressing Enter selects `~6kHz` (software) or `100kHz` (hardware).

After the speed answer the mode prints the standard parking lines; the bus is
inert until `W`:

```
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
I2C>
```

## Example entry — software driver

The mode menu is abridged (the full menu is in
[docs/generic/](../../generic/README.md)):

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

(1)>4
I2C mode:
 1. Software
 2. Hardware

(1)>1
Set speed:
 1. ~6kHz
 2. ~60kHz
 3. ~150kHz
 4. Maximum speed

(1)>1
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
I2C>
```

The hardware branch is **not verified**.

## The one-line settings form

Like the other modes, I2C can be entered with its settings on one line.  On
v4, after the mode number it reads an optional driver and an optional speed:

| Line | Enters |
|---|---|
| `m4 1 1` | Software, `~6kHz` |
| `m4 2 1` | Hardware, `100kHz` |

- driver (`mod`): **1** = Software, **2** = Hardware;
- speed (`spd`): the menu number (software 1–4, hardware 1–3).

`mod 2` with `spd 4` is **rejected**: the firmware sets the speed to zero and
re-runs the driver and speed prompts.  There is no fourth hardware rate; use
`spd 1`–`3` with Hardware.

A settings line with the driver present is only directly usable on v4, or on
a custom v3 build with the hardware engine.  On a default v3 build the form
takes the **speed only** (`m4 3` for `~150kHz`); there is no driver digit.

The header is printed as `I2C (mod spd)=( ` followed by the **current**
values, and those printed values are **zero-based**:

| Quirk | Detail |
|---|---|
| driver prints zero-based | `0` = Software, `1` = Hardware |
| speed prints zero-based | `0` = the first menu entry, not `1` |

A fresh v4 software session prints `I2C (mod spd)=( 0 0 )`.  The header
displays the current settings; type the menu numbers (`1`–`2`, `1`–`4`).

## Output type — not asked

Unlike SPI, UART and the raw modes, **I2C does not prompt for an output type**.
Its two lines are always driven open-drain, and the mode forces the high-Z
(open-drain) state itself.  The bus requirement this creates is the pull-up —
see [wiring.md](wiring.md).

## Power and pull-ups

Entering the mode only parks the bus.  Recommended bring-up order:

| Step | Command | Effect |
|---|---|---|
| engage | `W` | `POWER SUPPLIES ON` + `Clutch engaged!!!` |
| source the pull-up voltage (v4) | `e2` / `e3` | `3.3V` / `5V` on-board pullup voltage enabled |
| enable the pull-ups | `P` | `Pull-up resistors ON` |
| release the pull-ups | `p` | `Pull-up resistors OFF` |
| park again | `w` | `Clutch disengaged!!!` |

```
I2C>W
POWER SUPPLIES ON
Clutch engaged!!!
I2C>e2
3.3V on-board pullup voltage enabled
I2C>P
Pull-up resistors ON
I2C>
```

`P` enables the pull-up resistors, but they have no idle-high until a voltage
is on the VPU pin: on v4 `e2`/`e3` supplies it, on v3 it must be fed
externally.  Doing `P` first is allowed but warns:

```
I2C>P
Pull-up resistors ON
Warning: no voltage on Vpullup pin
I2C>e2
3.3V on-board pullup voltage enabled
I2C>
```

These are base-layer elements — see [commands.md](../../generic/commands.md)
and [wiring.md](wiring.md).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| mode number 4 | present | present |
| driver sub-menu | not in the default build | always asked |
| software speed menu | present | present |
| hardware speed menu | optional build | present |
| output-type prompt | not asked | not asked |

The v3 column is **not verified**.
