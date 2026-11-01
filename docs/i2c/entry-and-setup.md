# I2C — entry & setup

Part of the I2C documentation set (`docs/i2c/`).  How the mode is entered, how
the driver and its speed are chosen, and what the one-line settings form
accepts.  Wiring is in [wiring.md](wiring.md); the optional output-type prompt
does not apply to I2C (see below).

## Entering I2C

`I2C` is mode number **4** in the mode menu (HiZ is 1).  As with every mode the
menu form is `m` then `4`; the direct form is `m4`.  The shared mode-menu
grammar is owned by `docs/generic/` (see
[navigation.md](../generic/navigation.md) §Mode menu).

On **v4** (and any v3 build that compiles in the hardware engine — see
[v3 / v4](#v3--v4)) entering the mode asks a second question first, the driver
sub-menu:

| Prompt | Option | Text | Driver |
|---|---|---|---|
| `I2C mode:` | `1` | `Software` | bit-banged SDA/SCL |
| | `2` | `Hardware` | the on-chip I2C engine |

**Rule of thumb:** choose **Software** unless you need the on-board EEPROM
macros (v4 only) or want the hardware engine's rates; Software is the default,
works on both boards, and is what the binary `I2C1` path always uses.

A default **v3** build has no hardware engine compiled in, so it **skips the
sub-menu** and goes straight to the software speed prompt.  The sub-menu and
its strings exist in the v3 source but are removed from that build.

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

_not verified_

The two menus are **different**: the software driver offers the four bit-bang
profiles, the hardware driver the three engine rates.  The default answer to
both is `(1)`, so pressing Enter selects `~6kHz` (software) or `100kHz`
(hardware).

After the speed answer the mode prints the standard parking lines; the bus is
inert until `W`:

```
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
I2C>
```

## Example entry — software driver

The software branch of the entry sequence.  The mode menu is abridged (the full
menu is in `docs/generic/`):

```
HiZ>m
1. HiZ
2. 1-WIRE
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

Like the other modes, I2C can be entered with its settings on one line.  After
the mode number it reads an optional driver and an optional speed, in the
format shown by the settings header:

- driver (`mod`): **1** = Software, **2** = Hardware;
- speed (`spd`): the menu number (software 1–4, hardware 1–3).

The form accepts `mod` 1–2 and `spd` 1–4.  The software driver has four
profiles (`spd` 1–4); the hardware driver has only three engine rates, so use
`spd` 1–3 with `mod` 2.  The form does not reject `spd=4` with the hardware
driver, but there is no fourth engine rate, so treat that value as invalid.  A
settings line with the driver present is only directly usable on v4, because a
default v3 build has no hardware driver.

The header itself is printed as `I2C (mod spd)=( ` followed by the **current**
values, and those printed values are **zero-based**:

| Quirk | Detail |
|---|---|
| The header prints the driver zero-based | `0` means Software, `1` means Hardware |
| The header prints the speed zero-based | `0` is the first menu entry (`~6kHz` / `100kHz`), not `1` |

So a fresh v4 software session prints `I2C (mod spd)=( 0 0 )`.  The header is
a display of the current settings, not the text to type back; type the menu
numbers (`1`–`2` and `1`–`4`).

## Output type — not asked

Unlike SPI, UART and the raw modes, **I2C does not prompt for an output type**.
Its two lines are always driven open-drain, and the mode forces the high-Z
(open-drain) state itself.  There is therefore no `Open drain` / `Normal`
question when entering I2C.  The bus requirement this creates is the pull-up —
see [wiring.md](wiring.md).

## Power and pull-ups

Entering the mode only parks the bus.  To bring it up:

| Step | Command | Effect |
|---|---|---|
| engage | `W` | `POWER SUPPLIES ON` + `Clutch engaged!!!` |
| enable the pull-ups | `P` | `Pull-up resistors ON` (warns if no VPU voltage) |
| source the pull-up voltage (v4) | `e2` / `e3` | `3.3V` / `5.0V` on-board pull-up voltage enabled |
| park again | `w` (`p` releases pull-ups) | `Clutch disengaged!!!` |

`P` only works if a voltage is already on the VPU pin; on v4 `e2`/`e3`
provides it, on v3 it must be fed externally.  Selecting the source **before**
`P` avoids the `Warning: no voltage on Vpullup pin` line.  These are
base-layer elements — see [commands.md](../generic/commands.md) and the
I2C-specific notes in [wiring.md](wiring.md).

Bring-up example (pull-ups enabled before feeding VPU):

```
I2C>W
POWER SUPPLIES ON
Clutch engaged!!!
I2C>P
Pull-up resistors ON
Warning: no voltage on Vpullup pin
I2C>e2
3.3V on-board pullup voltage enabled
I2C>
```

The pull-ups were enabled but had no reference until `e2` supplied one.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| mode number 4 | present | present |
| driver sub-menu | not in a default build (skips to software speed) | always asked |
| software speed menu | present | present |
| hardware speed menu | not in a default build | present |
| output-type prompt | not asked | not asked |

The v3 column is **not verified**.  Before this
rewrite the published I2C page stated a single speed menu of
`~50kHz / ~100kHz / ~400kHz`; that wording matches neither driver and has been
replaced.