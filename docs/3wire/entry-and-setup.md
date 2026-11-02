# 3WIRE — entry & setup

Part of the 3WIRE documentation set (`docs/3wire/`).  How the mode is entered,
what it asks, and what the one-line settings form accepts.  Wiring and the
electrical requirements are in [wiring.md](wiring.md).

## Entering 3WIRE

`3WIRE` is mode number **7** in the mode menu (HiZ is 1).  The menu form is `m`
then `7`; the direct form is `m7`.  The number is fixed by the position of the
mode in the menu (after 2WIRE, before KEYB), so it is 7 on both builds even
though v3 omits the v4-only modes.  The shared mode-menu grammar is owned by
`docs/generic/` — see [navigation.md](../generic/navigation.md) §Mode menu.

Entering asks three questions, in this order:

| Order | Prompt | Option | Text |
|---:|---|---|---|
| 1 | `Set speed:` | `1` | `~6kHz` |
| | | `2` | `~60kHz` |
| | | `3` | `~150kHz` |
| | | `4` | `Maximum speed` |
| 2 | `CS:` | `1` | `CS` |
| | | `2` | `/CS *default` |
| 3 | `Select output type:` | `1` | `Open drain (H=Hi-Z, L=GND)` |
| | | `2` | `Normal (H=3.3V, L=GND)` |

Each prompt shows its default in the `(n)>` marker, so pressing Enter through
all three selects **~6kHz, `/CS`, open drain**.  The speed labels are
approximate; the four profiles are the same software bit-bang delay profiles
used by software I2C and 2WIRE.

The **CS** answer sets the chip-select polarity: the `CS` option (menu `1`,
as opposed to the CS wire) and `/CS` (menu `2`, the default).  Which physical
level that ends up driving is described in
[bus-operations.md](bus-operations.md) — the current firmware does not match
the label.

The **output type** answer sets the high-impedance state:

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
7. 3WIRE
...
x. exit(without change)

(1)>7
Set speed:
 1. ~6kHz
 2. ~60kHz
 3. ~150kHz
 4. Maximum speed

(1)>2
CS:
 1. CS
 2. /CS *default

(2)>
Select output type:
 1. Open drain (H=Hi-Z, L=GND)
 2. Normal (H=3.3V, L=GND)

(1)>2
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
3WIRE>
```

Entering with the direct form `m7` (no settings on the line) asks the same
three prompts but prints only `Ready` — the `Clutch disengaged!!!` /
`To finish setup…` pair is printed by the menu path.  The bus is inert until
`W`.

## The one-line settings form

Like the other modes, 3WIRE can take its settings on the mode line.  After the
mode number it reads three numbers:

| Typed | Field | Accepted | Meaning |
|---|---|---|---|
| 1st number | speed | `1`–`4` | the speed-menu number above |
| 2nd number | CS polarity | `1`–`2` | `1` `CS`, `2` `/CS` |
| 3rd number | output type | `1`–`2` | `1` open drain, `2` normal |

Example — `m7 2 2 1` selects the second speed (`~60kHz`), the `/CS` polarity
and open drain:

```
HiZ>m7 2 2 1
R3W (spd csl hiz)=( 1 1 1 )
Ready
3WIRE>
```

The header is printed only when the whole form is supplied and valid; the
interactive prompt path prints the prompt menus instead.  The header text is
`R3W (spd csl hiz)=( `, then the three values, then ` )`.  **The printed values
are not the menu numbers you type**, and the CS field is printed as `csl`:

| Header field | Printed value | Means |
|---|---|---|
| `spd` | `0`–`3` | menu entry 1–4 (so `0` is `~6kHz`) |
| `csl` | `0` / `1` | `0` = `CS`, `1` = `/CS` |
| `hiz` | `1` / `0` | `1` = open drain, `0` = normal |

So `R3W (spd csl hiz)=( 1 1 1 )` is the second speed + `/CS` + open drain, and
the form is a *display* of the current settings, not the text to type back.
Type the menu numbers (`1`–`4`, `1`–`2`, `1`–`2`).

The acceptance check for the one-line form is narrow: the speed must be
`1`–`4`, the CS number `1`–`2` and the output number `1`–`2`.  Anything
else (including an out-of-range speed) falls back to the interactive prompts.

## Power and pull-ups

Entering the mode only parks the bus.  To bring it up:

| Step | Command | Effect |
|---|---|---|
| engage | `W` | `POWER SUPPLIES ON` + `Clutch engaged!!!` |
| enable the pull-ups (open drain) | `P` | `Pull-up resistors ON` |
| source the pull-up voltage (v4) | `e2` / `e3` | `3.3V` / `5.0V` on-board pull-up voltage enabled |
| park again | `w` (`p` releases pull-ups) | `Clutch disengaged!!!` |

`P` only works if a voltage is on the VPU pin; on v4 `e2`/`e3` provides it, on
v3 it must be fed externally.  These are base-layer elements — see
[commands.md](../generic/commands.md) and the 3WIRE notes in
[wiring.md](wiring.md).

If the mode was entered with **normal** output, `P` prints
`WARNING: pins not open drain (HiZ)` because driven pins make the pull-up
pointless; the pull-ups are still switched on.  See
[edge-cases.md](edge-cases.md).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| mode number 7 | present | present |
| speed menu | present | present |
| CS-polarity menu | present | present |
| output-type menu | present | present |
| `e` VPU source | — (feed externally) | present |

The two builds ask the same three questions with the same wording and the same
defaults.  The v3 column is **not verified** (v3 is build-only here).