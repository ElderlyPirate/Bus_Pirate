# 2-Wire — entry & setup

Part of the [2-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  How the mode is entered, what it asks, and
what the one-line settings form accepts.  Wiring and the electrical
requirements are in [wiring.md](wiring.md).

## Entering 2-Wire

`2WIRE` is mode number **6** in the mode menu (HiZ is 1).  The menu form is
`m` then `6`; the direct form is `m6`.  The shared mode-menu grammar is owned
by [docs/generic/](../../../generic/README.md) — see
[navigation.md](../../../generic/navigation.md) §Mode menu.

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
**~6kHz open-drain**.

The output-type answer sets the high-impedance state:

| Option | Meaning | `hiz` value |
|---|---|---|
| `1` Open drain | the Bus Pirate only pulls a line low; a pull-up makes the high | `1` |
| `2` Normal | the Bus Pirate drives high (3.3 V) and low | `0` |

A pull-up is required for **open drain** and not for **normal** — see
[wiring.md](wiring.md).  The output-type answer is shown again later as the
`hiz` field of the settings header.

## Example entry — `m` menu

The full menu is in [docs/generic/](../../../generic/README.md); it is
abridged here (other modes elided).  Entering through the menu prints the
parking lines:

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
prompts but prints only `Ready` — the `Clutch disengaged!!!` / `To finish
setup…` pair is printed by the menu path.  The bus is inert until `W`.

## The one-line settings form

Like the other modes, 2-Wire can take its settings on the mode line.  After the
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

The header is printed only when the whole form is supplied and valid; the
interactive prompt path prints the prompt menus instead.  The exact line is
`R2W (spd hiz)=( `, then the two values, then ` )`.  **The double space before
`)` is correct**: each value is followed by a space, and the closing token
itself begins with a space.

**The printed values are zero-based**, and the output field is printed as
`hiz`:

| Header field | Printed value | Means |
|---|---|---|
| `spd` | `0`–`3` | menu entry 1–4 (so `0` is `~6kHz`) |
| `hiz` | `1` / `0` | `1` = open drain, `0` = normal |

| Typed form | Printed header |
|---|---|
| `m6 1 1` | `R2W (spd hiz)=( 0 1  )` |
| `m6 2 1` | `R2W (spd hiz)=( 1 1  )` |
| `m6 4 2` | `R2W (spd hiz)=( 3 0  )` |

So `R2W (spd hiz)=( 1 1  )` is the second speed + open drain, and the form is a
*display* of the current settings, not the text to type back.  Type the menu
numbers (`1`–`4` and `1`–`2`).  The same header is repeated at the end of
the `i` status block; it is the only settings read-back the mode has.

**`m6 2` is not a one-field form.**  It supplies the speed only; the mode then
runs both interactive prompts and discards the typed `2`.  Pressing Enter at
the prompts therefore selects the default ~6kHz open-drain setting, not
~60kHz.  Answer both prompts, or type the complete form `m6 2 1`.

## Clock speeds

The four menu entries are the same software bit-bang profiles used by software
I2C and 3WIRE.  The firmware's per-phase delays are:

| Menu | Profile | Settle | Clock | Derived bit rate |
|---|---|---|---|---|
| `1` `~6kHz` | 0 | 19 µs | 80 µs | ≈ 5.6 kHz |
| `2` `~60kHz` | 1 | 1 µs | 9 µs | ≈ 53 kHz |
| `3` `~150kHz` | 2 | 1 µs | 4 µs | ≈ 111 kHz |
| `4` `Maximum speed` | 3 | 0 | 0 | as fast as the loop runs |

A bit write is one settle phase plus two clock phases.  The rate column is
derived from those delays and excludes loop and transport overhead; the menu
labels are the firmware's nominal names, not measured rates.  The same profile
sets the delays used by `^`, `/`, `\`, `-`, `_` and every byte transfer.

## Power and pull-ups

Entering the mode only parks the bus.  A common bring-up order:

| Step | Command | Effect |
|---|---|---|
| engage | `W` | `POWER SUPPLIES ON` + `Clutch engaged!!!` |
| source the pull-up voltage (v4) | `e2` / `e3` | `3.3V` / `5V` on-board pullup voltage enabled |
| enable the pull-ups (open drain) | `P` | `Pull-up resistors ON` |
| park again | `w` (`p` releases pull-ups) | `Clutch disengaged!!!` + `POWER SUPPLIES OFF` |

`P` switches the pull-ups on but they only produce an idle high if a voltage is
already on the VPU pin; on v4 `e2`/`e3` provides it, on v3 it must be fed
externally.  **Feed VPU before `P`** — running `P` first on v4 only prints
`Warning: no voltage on Vpullup pin` after `Pull-up resistors ON`.  `P` can be
enabled before `W`; it needs a non-HiZ mode.  The `e` source selection needs a
non-HiZ open-drain mode and no voltage already on VPU.  The shared `P`/`e`
commands are in
[commands.md](../../../generic/commands.md); the 2-Wire wiring notes are in
[wiring.md](wiring.md).

If the mode was entered with **normal** output, `P` prints
`WARNING: pins not open drain (HiZ)` because driven pins make the pull-up
pointless; the pull-ups are still switched on.  `e` is also refused in normal
output (`WARNING: pins not open drain (HiZ)` + syntax error), because the VPU
source requires open-drain outputs.
