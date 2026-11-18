# PC-AT keyboard (KEYB) — entry and setup

Part of the KEYB documentation set (`docs/protocol/keyboard/`).  This page owns
mode entry, the fixed defaults, the `W` engage, the `i` / `v` status output and
the v3 availability.  Wiring is in [wiring.md](wiring.md); the wire format is
in [protocol.md](protocol.md).

## Scope

How the KEYB mode is entered, what it prints, what it sets, and what `W` does.
It does not cover the keyboard protocol itself ([protocol.md](protocol.md)) or
typing the commands ([usage.md](usage.md)).

## Mode number and menu

KEYB is **mode 8** in the default v4 image: the eighth entry of the `m` menu
and of `i`'s `[ … ]` list, between 3WIRE and PIC.

| Menu slot | Mode |
|---|---|
| 1 | `HiZ` |
| 2 | `1WIRE` |
| 3 | `UART` |
| 4 | `I2C` |
| 5 | `SPI` |
| 6 | `2WIRE` |
| 7 | `3WIRE` |
| **8** | **`KEYB`** |
| 9 | `PIC` |
| 10 | `DIO` |
| x | `exit(without change)` |

The menu follows the modes compiled into the build.  `i` prints the enabled
modes in menu order but does not number them; the default v4 list is
`[HiZ 1WIRE UART I2C SPI 2WIRE 3WIRE KEYB PIC DIO]`.  The shared menu and entry
grammar are owned by [generic/navigation.md](../../generic/navigation.md).

## Entering the mode

| Input | Behaviour |
|---|---|
| `m`, then `8` | the menu, then the parked-state announcement |
| `m8`, `m 8` | direct entry |

KEYB has **no setup prompts**.  After the mode number there are no questions
to answer, unlike UART, I2C or SPI; the `(1)>` in the menu capture below is the
shared mode menu's own prompt for the mode number.  Both forms land at the
`KEYB>` prompt.

Menu form:

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

The direct form prints no `Clutch disengaged!!!` line and no finish-setup
prompt, but the mode starts in the same parked state.

## Parked on entry

On entry the mode is parked: the Bus Pirate pins are inputs and its supply is
off.  A keyboard fed from the `5.0V` pin is unpowered.  The Bus Pirate drives
nothing until `W`.  The mode's only entry-time setting is the **open-drain**
output type; there is no prompt for it.

## Engage — `W`

`W` switches on the 3.3 V and 5 V rails, then runs the mode's engage step: a
keyboard fed from `5.0V` gets its supply, CLOCK is driven low (the host
inhibit) and DATA is released as an input.

```
KEYB>W
POWER SUPPLIES ON
Clutch engaged!!!
KEYB>
```

Send `W` before any read, write or the monitor on a keyboard fed from `5.0V`;
it also engages the idle inhibit.  An externally powered keyboard can clock
without `W`, but then the mode does not hold CLOCK low.  If the regulator
output is too low, the firmware refuses and switches the supplies off:

```
KEYB>W
VREG too low, is there a short?
POWER SUPPLIES OFF
KEYB>
```

The supplies and the `W`/`w` behaviour are base-layer features
([generic/commands.md](../../generic/commands.md)); this page records only
that a keyboard fed from `5.0V` needs `W` for its supply, and that **`W`
engages the idle clock-low state**.

`w` switches the supply off and prints:

```
KEYB>w
Clutch disengaged!!!
POWER SUPPLIES OFF
KEYB>
```

`w` does not release the CLOCK line or turn off the pull-up switch (or an
on-board VPU source, if selected); see [wiring.md](wiring.md).

## Defaults

| Setting | Value | Where it shows |
|---|---|---|
| bits per value | 8 | `i`: `Number of bits read/write: 8` |
| output type | open drain (`H=Hi-Z, L=GND`) | `i` status line |
| bit order | prompt MSB (`l`); wire is LSB-first | `i`: `MSB set: MOST sig bit first` |
| display format | HEX | `i` does not show it; `o` changes it |
| mode settings | none | `i` prints no mode settings line |

The mode fixes open drain; there is no output-type prompt.  The width, bit
order and display format are **shared prompt settings**, not KEYB setup
answers.  They can be changed after entry with `;N`, `l`/`L` and `o`; the
keyboard frame is always one 8-bit byte, but the printed value, and with `L`
the byte sent, can change.  The `i` label `MSB set` describes the prompt's
value order, not the wire order: the frame is always LSB-first
([protocol.md §Two bit orders](protocol.md#two-bit-orders)).
See [scancodes.md](scancodes.md#what-changes-the-printed-value) and the traps
in [edge-cases.md](edge-cases.md#shared-settings).

The two mode-relevant `i` status lines after entry are:

```
POWER SUPPLIES OFF, Pull-up resistors OFF, Open drain outputs (H=Hi-Z, L=GND)
MSB set: MOST sig bit first, Number of bits read/write: 8
```

The full banner and status-block layout belong to
[generic/navigation.md](../../generic/navigation.md#version-and-status--i).
KEYB contributes no mode settings summary, so the status block has an empty
settings line before its closing `*----------*` separator.

## Pin state display — `v`

`v` prints the rail/pin table.  The mode supplies the last four signal-name
columns; the base layer supplies the first eight names, the directions and the
live levels.  The signal-name row, with the firmware's tabs shown as spaces:

```
GND  5.0V  3.3V  VPU  ADC  AUX2  AUX1  AUX  CS  MISO  CLK  MOSI
```

Only **MOSI** (keyboard data) and **CLK** (keyboard clock) carry the keyboard
interface; `CS` and `MISO` are unused here.  After `W`, the direction row shows
`CLK` as an output and `MOSI` as an input; the level row shows `CLK` low.  The
table is tab-separated, so the on-screen columns depend on the terminal's tab
stops.  The full `v` layout is described in
[generic/usage.md](../../generic/usage.md#v-in-hiz).

## Leaving the mode

| Command | Effect |
|---|---|
| a terminal key during `(1)` | ends the monitor, back to `KEYB>` |
| `w` | supply off, mode idle |
| `m` / `m<N>` | switches mode; the board reset releases the pins and the supply |
| `#` | terminal reset to HiZ; restores HEX and releases the pins |

The shared reset and mode-change rules are in
[generic/navigation.md](../../generic/navigation.md).

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| KEYB in the default build | no | yes, slot 8 |
| module source | present, optional | present, enabled |
| menu list | `KEYB` absent; PIC is 8, DIO is 9 | `KEYB` at 8 |
| hardware use | unverified | available in the default image |
| host writes | unverified | available; keyboard responses vary |

The default v3 image **does not include** KEYB.  A custom v3 build can enable
the module; treat the wiring and timing statements here as v4 statements.
