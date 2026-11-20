# 3-Wire — entry & setup

Part of the [3-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  How the mode is entered, what it asks, and
what the one-line settings form accepts.  Wiring and the electrical
requirements are in [wiring.md](wiring.md).

## Entering 3-Wire

`3WIRE` is mode number **7** in the mode menu (HiZ is 1).  The menu form is
`m` then `7`; the direct form is `m7`.  Every mode before it is present in both
builds, so it is 7 on both.  The shared mode-menu grammar is owned by
[docs/generic/](../../../generic/README.md) — see
[navigation.md](../../../generic/navigation.md) §Mode menu.

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
all three selects **~6kHz, `/CS`, open drain**.

The **CS** answer sets the chip-select polarity: the `CS` option (menu `1`,
active-high) and `/CS` (menu `2`, active-low, the default).  The physical level
each one drives is described in
[bus-operations.md](bus-operations.md) §Which level is driven.

The **output type** answer sets the high-impedance state:

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

Like the other modes, 3-Wire can take its settings on the mode line.  After the
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
interactive prompt path prints the prompt menus instead.  The exact line is
`R3W (spd csl hiz)=( `, then the three values, then ` )`.  Unlike the 2-Wire
header, it has **one** space before `)`.

**The printed values are not the menu numbers you type**, and the CS field is
printed as `csl`:

| Header field | Printed value | Means |
|---|---|---|
| `spd` | `0`–`3` | menu entry 1–4 (so `0` is `~6kHz`) |
| `csl` | `0` / `1` | `0` = `CS`, `1` = `/CS` |
| `hiz` | `1` / `0` | `1` = open drain, `0` = normal |

| Typed form | Printed header |
|---|---|
| `m7 1 2 1` | `R3W (spd csl hiz)=( 0 1 1 )` |
| `m7 2 2 1` | `R3W (spd csl hiz)=( 1 1 1 )` |
| `m7 4 1 2` | `R3W (spd csl hiz)=( 3 0 0 )` |

So `R3W (spd csl hiz)=( 1 1 1 )` is the second speed + `/CS` + open drain, and
the form is a *display* of the current settings, not the text to type back.
Type the menu numbers (`1`–`4`, `1`–`2`, `1`–`2`).  The same header is
repeated at the end of the `i` status block.

**`csl` is not the driven level.**  `csl` = 1 (`/CS`, the default) means the
wrappers assert CS **low**; `csl` = 0 (`CS`) means they assert it **high**.

The acceptance check for the one-line form is narrow: the speed must be
`1`–`4`, the CS number `1`–`2` and the output number `1`–`2`.  Anything
else falls back to the interactive prompts, and the typed values are
discarded.

**`m7 2` is not a one-field form.**  It supplies the speed only; the mode then
runs all three interactive prompts and discards the typed `2`.  Pressing Enter
at the prompts therefore selects the defaults (~6kHz, `/CS`, open drain), not
~60kHz.  Answer all three prompts, or type the complete form `m7 2 2 1`.

## Clock speeds

The four menu entries are the same software bit-bang profiles used by software
I2C and 2WIRE:

| Menu | Profile | Settle | Clock | Derived bit rate |
|---|---|---|---|---|
| `1` `~6kHz` | 0 | 19 µs | 80 µs | ≈ 5.6 kHz |
| `2` `~60kHz` | 1 | 1 µs | 9 µs | ≈ 53 kHz |
| `3` `~150kHz` | 2 | 1 µs | 4 µs | ≈ 111 kHz |
| `4` `Maximum speed` | 3 | 0 | 0 | as fast as the loop runs |

A bit transfer is one settle phase plus two clock phases.  The rate column is
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

At `W` the firmware drives CLK and MOSI low and puts CS at its inactive level
(`/CS` high, `CS` low).  MISO is an input.  The first `^` therefore starts with
a rising edge.  The shared `P`/`e` commands and the v4 source choices are in
[commands.md](../../../generic/commands.md); the 3-Wire wiring notes are in
[wiring.md](wiring.md).

`P` switches the pull-ups on but they only produce an idle high if a voltage
is already on the VPU pin.  **Feed VPU before `P`** — running `P` first on v4
only prints `Warning: no voltage on Vpullup pin` after
`Pull-up resistors ON`.
`P` can be enabled before `W`; it needs a non-HiZ mode.  The `e` source
selection needs a non-HiZ open-drain mode and no voltage already on VPU.

If the mode was entered with **normal** output, `P` prints
`WARNING: pins not open drain (HiZ)` because driven pins make the pull-up
pointless; the pull-ups are still switched on.  `e` is also refused in normal
output (`WARNING: pins not open drain (HiZ)` + syntax error), because the VPU
source requires open-drain outputs.
