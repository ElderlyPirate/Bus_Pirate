# SPI — entry & setup

Part of the SPI documentation set (`docs/protocol/spi/`). This page owns how the
mode is entered, the speed menu, the setup prompt chain, and what the one-line
settings form accepts. Wiring is in [wiring.md](wiring.md); the sniffer is in
[sniffer.md](sniffer.md).

## Entering SPI

`SPI` is mode number **5** in the mode menu (HiZ is 1). As with every mode the
menu form is `m` then `5`; the direct form is `m5`. The shared mode-menu
grammar is owned by `docs/generic/` — see
[navigation.md](../../generic/navigation.md) §Mode menu.

After the mode number the firmware runs the setup chain: **speed, clock
polarity, clock edge, input sample phase, CS polarity, output type**, in that
order.

## The speed menu

`Set speed:` offers twelve rates. The order is a fixed list, not sorted:

| Option | Text | Nominal rate |
|---:|---|---|
| 1 | `30kHz` | 30 kHz |
| 2 | `125kHz` | 125 kHz |
| 3 | `250kHz` | 250 kHz |
| 4 | `1MHz` | 1 MHz |
| 5 | `2MHz` | 2 MHz |
| 6 | `2.6MHz` | 2.6 MHz |
| 7 | `4MHz` | 4 MHz |
| 8 | `8MHz` | 8 MHz |
| 9 | `50kHz` | 50 kHz |
| 10 | `1.3MHz` | 1.3 MHz |
| 11 | `3.2MHz` | 3.2 MHz |
| 12 | `5.3MHz` | 5.3 MHz |

The rates are nominal and the menu prints a fixed, unsorted order; the full set
is `30, 50, 125, 250 kHz; 1, 1.3, 2, 2.6, 3.2, 4, 5.3, 8 MHz`. The default
answer is `1` (`30kHz`); measure the actual clock if a target is marginal.

## The setup prompt chain

Each prompt shows its default in the `(n)>` marker; pressing Enter accepts it.

| Order | Prompt | Options | Default |
|---:|---|---|---|
| 1 | `Set speed:` | 1–12 (see above) | `1` `30kHz` |
| 2 | `Clock polarity:` | `1. Idle low` / `2. Idle high` | `1` Idle low |
| 3 | `Output clock edge:` | `1. Idle to active` / `2. Active to idle` | `2` Active to idle |
| 4 | `Input sample phase:` | `1. Middle` / `2. End` | `1` Middle |
| 5 | `CS:` | `1. CS` / `2. /CS` | `2` `/CS` |
| 6 | `Select output type:` | `1. Open drain (H=Hi-Z, L=GND)` / `2. Normal (H=3.3V, L=GND)` | `1` Open drain |

The **output type** decides the electrical behaviour of the output pins and
therefore the pull-up requirement — see [wiring.md](wiring.md). The **CS**
choice sets the CS polarity: `CS` is active-high, `/CS` is active-low.

After the last answer the menu form prints the parking lines. The bus is inert
until `W` succeeds; `W` is what runs the pin and module setup.

```
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
SPI>
```

## Example entry

The menu form with the defaults accepted for everything except the output type
(`2` = normal, which needs no pull-up). The mode menu is abridged — the full
menu is in `docs/generic/`:

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

(1)>5
Set speed:
 1.  30kHz
 2. 125kHz
 3. 250kHz
 4.   1MHz
 5.   2MHz
 6. 2.6MHz
 7.   4MHz
 8.   8MHz
 9.  50kHz
10. 1.3MHz
11. 3.2MHz
12. 5.3MHz

(1)>
Clock polarity:
 1. Idle low *default
 2. Idle high

(1)>
Output clock edge:
 1. Idle to active
 2. Active to idle *default

(2)>
Input sample phase:
 1. Middle *default
 2. End

(1)>
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
SPI>
```

## Bring-up with `W`

Run `W` before the first bus operation:

```
SPI>W
POWER SUPPLIES ON
Clutch engaged!!!
SPI>
```

On success `W` enables the Bus Pirate's programmable supply on the VCC pin and
then runs the mode's pin and module setup. The SPI pins and the SPI module are
therefore configured **only** at that point, so `W` is required even when the
target is powered from its own supply — leave VCC disconnected from a
self-powered target. If the rails do not come up, `W` prints
`VREG too low, is there a short?` and `POWER SUPPLIES OFF`, and the setup does
not run — see [edge-cases.md](edge-cases.md).

## The one-line settings form

SPI can be entered with its settings on one line. After the mode number it
reads six integers in the order shown by the settings header. For example
`m5 4 1 2 1 2 2` selects speed `4` = `1MHz`, polarity `1` idle low, edge `2`
active→idle, sample `1` middle, CS `2` `/CS`, output type `2` normal. Any
speed from 1 to 12 is accepted, so `m5 12 1 2 1 2 2` selects `12` = `5.3MHz`.

| Field | Meaning | Header display | Input value (menu number) |
|---|---|---|---|
| `spd` | speed | menu number 1–12 | menu number 1–12 |
| `ckp` | clock polarity | `0` idle low, `1` idle high | `1` idle low, `2` idle high |
| `ske` | clock edge | `0` idle→active, `1` active→idle | `1` idle→active, `2` active→idle |
| `smp` | input sample phase | `0` middle, `1` end | `1` middle, `2` end |
| `csl` | CS polarity | `0` `CS`, `1` `/CS` | `1` `CS`, `2` `/CS` |
| `hiz` | output type | `1` open drain / Hi-Z, `0` normal | `1` open drain / Hi-Z, `2` normal |

The **Header display** column is what the printed settings header shows; it is
not what you type. The header prints `spd` one-based (matching the menu) and
the other five fields in their raw `0`/`1` form. To enter the form by hand you
type the **Input value** column — the menu numbers for the two-option prompts.
When all six values are in range the firmware echoes the settings header instead
of running the prompts:

```
SPI (spd ckp ske smp csl hiz)=( 4 0 1 0 1 0 )
```

If a value is missing or out of range, the interactive prompts run instead.

## Buffer limit

The terminal ring buffer bounds a long capture (v4 4096 bytes, v3 2048 bytes).
Overflow handling and the other capture limits are in [sniffer.md](sniffer.md)
§Integrity and stopping; the binary path has its own framing rules — see
[binary.md](binary.md).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| mode number 5 | present | present |
| setup chain and defaults | same | same |
| speed menu | same | same |
| terminal buffer | 2048 bytes | 4096 bytes |

The v3 column is **not verified**.