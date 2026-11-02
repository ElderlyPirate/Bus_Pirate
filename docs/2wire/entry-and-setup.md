# 2WIRE — entry & setup

Part of the 2WIRE documentation set (`docs/2wire/`).  How the mode is entered,
what it asks, and what the one-line settings form accepts.  Wiring and the
electrical requirements are in [wiring.md](wiring.md).

## Entering 2WIRE

`2WIRE` is mode number **6** in the mode menu (HiZ is 1).  The menu form is `m`
then `6`; the direct form is `m6`.  The shared mode-menu grammar is owned by
`docs/generic/` — see [navigation.md](../generic/navigation.md) §Mode menu.

Entering asks two questions, in this order:

| Prompt | Option | Text |
|---|---|---|
| `Set speed:` | `1` | `~6kHz` |
| | `2` | `~60kHz` |
| | `3` | `~150kHz` |
| | `4` | `Maximum speed` |
| `Select output type:` | `1` | `Open drain (H=Hi-Z, L=GND)` |
| | `2` | `Normal (H=3.3V, L=GND)` |

Both prompts default to `(1)`, so pressing Enter through both selects
**~6kHz open-drain**.  The speed labels are approximate; the four profiles are
the same software bit-bang profiles used by software I2C, and the actual clock
rate is set by the profile's delays.

The output-type answer sets the high-impedance state:

| Option | Meaning | `hiz` value |
|---|---|---|
| `1` Open drain | the Bus Pirate only pulls a line low; a pull-up makes the high | `1` |
| `2` Normal | the Bus Pirate drives high (3.3 V) and low | `0` |

A pull-up is required for **open drain** and not for **normal** — see
[wiring.md](wiring.md).  The output-type answer is shown again later as the
`hiz` field of the settings header.

## Example entry — `m` menu

The full menu is in `docs/generic/`; it is abridged here.  Entering through the
menu prints the parking lines:

```
HiZ>m
1. HiZ
...
6. 2WIRE
...
x. exit(without change)

(1)>6
Set speed:
 1. ~6kHz
 2. ~60kHz
 3. ~150kHz
 4. Maximum speed

(1)>1
Select output type:
 1. Open drain (H=Hi-Z, L=GND)
 2. Normal (H=3.3V, L=GND)

(1)>1
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
2WIRE>
```

Entering with the direct form `m6` (no settings on the line) asks the same two
prompts but prints only `Ready` — the `Clutch disengaged!!!` /
`To finish setup…` pair is printed by the menu path.  The bus is inert until
`W`.

## The one-line settings form

Like the other modes, 2WIRE can take its settings on the mode line.  After the
mode number it reads two numbers:

| Typed | Field | Accepted | Meaning |
|---|---|---|---|
| 1st number | speed | `1`–`4` | the speed-menu number above |
| 2nd number | output | `1`–`2` | `1` open drain, `2` normal |

Example — `m6 2 1` selects the second speed (`~60kHz`) and open drain:

```
HiZ>m6 2 1
R2W (spd hiz)=( 1 1  )
Ready
2WIRE>
```

The header is printed only when the whole form is supplied; the interactive
prompt path prints the prompt menus instead.  The header text is
`R2W (spd hiz)=( `, then the two values, then ` )`.  **The printed values are
zero-based**, and the output field is printed as `hiz`:

| Header field | Printed value | Means |
|---|---|---|
| `spd` | `0`–`3` | menu entry 1–4 (so `0` is `~6kHz`) |
| `hiz` | `1` / `0` | `1` = open drain, `0` = normal |

So `R2W (spd hiz)=( 1 1  )` is the second speed + open drain, and the form is a
*display* of the current settings, not the text to type back.  Type the menu
numbers (`1`–`4` and `1`–`2`).

## Power and pull-ups

Entering the mode only parks the bus.  To bring it up:

| Step | Command | Effect |
|---|---|---|
| engage | `W` | `POWER SUPPLIES ON` + `Clutch engaged!!!` |
| source the pull-up voltage (v4) | `e2` / `e3` | `3.3V` / `5.0V` on-board pull-up voltage enabled |
| enable the pull-ups (open drain) | `P` | `Pull-up resistors ON` |
| park again | `w` (`p` releases pull-ups) | `Clutch disengaged!!!` |

`P` only works if a voltage is on the VPU pin; on v4 `e2`/`e3` provides it, on
v3 it must be fed externally.  **Feed VPU before `P`** — running `P` first on
v4 only prints `Warning: no voltage on Vpullup pin`.  These are base-layer
elements — see [commands.md](../generic/commands.md) and the 2WIRE notes in
[wiring.md](wiring.md).

If the mode was entered with **normal** output, `P` prints
`WARNING: pins not open drain (HiZ)` because driven pins make the pull-up
pointless; the pull-ups are still switched on.  See
[edge-cases.md](edge-cases.md).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| mode number 6 | present | present |
| speed menu | present | present |
| output-type menu | present | present |
| `e` VPU source | — (feed externally) | present |

The two builds ask the same two questions with the same wording.  The v3 column
is **not verified** (v3 is build-only here).