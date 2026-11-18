# SPI — entry & setup

Part of the SPI documentation set (`docs/protocol/spi/`). This page owns how
the mode is entered, the speed menu, the clock/output settings, the one-line
settings form, and bring-up with `W`. Wiring is in [wiring.md](wiring.md); the
sniffer is in [sniffer.md](sniffer.md); the binary layer is in
[binary.md](binary.md).

## Entering SPI

`SPI` is mode number **5** in the default v3 and v4 menus (HiZ is 1). The menu
is build-dependent — the default v4 build lists ten modes, the default v3 build
omits `KEYB` and shifts `PIC`/`DIO` — but `SPI` stays 5. Take the number from
the `m` menu of the build in front of you. The shared menu grammar is in
[docs/generic/](../../generic/navigation.md).

Three forms enter the mode:

| Form | What happens |
|---|---|
| `m` [Enter], then the number at `(1)>` | menu form; the setup prompts run |
| `m5` or `m 5` | direct form; the setup prompts run |
| `m5 <six values>` or `m 5 <six values>` | direct form with settings on one line; the firmware echoes the settings header |

Spaces are allowed between `m`, the mode number and the six values
(`m 5 4 1 2 1 2 2`). Only `m` **alone**, with no number on the same line,
opens the menu.

The menu form and the direct form differ in what they print after the setup:

| Form | After the settings are applied |
|---|---|
| `m` then `5` | `Clutch disengaged!!!`, `To finish setup, start up the power supplies with command 'W'`, `Ready` |
| `m5` | `Ready` only |
| `m5 <six values>` | the settings header, then `Ready` |

All three end at the `SPI>` prompt with the bus **not yet engaged**. `W` runs
the pin and module setup; see §Bring-up with `W`.

### Menu-form example

The default v4 menu is shown; the default v3 build omits `KEYB`, so `PIC` and
`DIO` move up, but `SPI` stays 5. The setup chain is complete. Defaults are
accepted except for the output type (`2` = normal, which needs no pull-up):

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

The text and spacing of the option lines are the firmware's; the blank line
before each `(1)>`/`(2)>` marker is printed by the prompt reader.

## The speed menu

`Set speed:` offers twelve rates in a fixed, unsorted order. The menu number is
the value you type; the binary `0x6x` command uses the table index (menu number
minus one).

| Menu | Label | Nominal rate | Binary index |
|---:|---|---|---:|
| 1 | `30kHz` | 30 kHz | 0 |
| 2 | `125kHz` | 125 kHz | 1 |
| 3 | `250kHz` | 250 kHz | 2 |
| 4 | `1MHz` | 1 MHz | 3 |
| 5 | `2MHz` | 2 MHz | 4 |
| 6 | `2.6MHz` | 2.6 MHz | 5 |
| 7 | `4MHz` | 4 MHz | 6 |
| 8 | `8MHz` | 8 MHz | 7 |
| 9 | `50kHz` | 50 kHz | 8 |
| 10 | `1.3MHz` | 1.3 MHz | 9 |
| 11 | `3.2MHz` | 3.2 MHz | 10 |
| 12 | `5.3MHz` | 5.3 MHz | 11 |

The rates are nominal; the clock is derived from the Bus Pirate's oscillator.
The default answer is `1` (`30kHz`). The higher entries depend on wiring and
the target, so check the clock against the target's limit rather than assuming
a rate.

## Clock polarity, edge and sample phase

The three clock settings are separate:

| Setting | Meaning |
|---|---|
| `Clock polarity` | the idle level of CLK: `1. Idle low` or `2. Idle high` |
| `Output clock edge` | the edge on which the Bus Pirate changes MOSI: `1. Idle to active` or `2. Active to idle` |
| `Input sample phase` | where MISO is sampled: `1. Middle` or `2. End` |

Targets usually describe their timing as CPOL/CPHA. The polarity and edge pair
selects the same timing; the sample phase is a separate sample-point control,
not the CPHA bit.

| Conventional mode | Clock polarity | Output clock edge | CPHA |
|---:|---|---|---:|
| 0 | Idle low | Active to idle | 0 |
| 1 | Idle low | Idle to active | 1 |
| 2 | Idle high | Active to idle | 0 |
| 3 | Idle high | Idle to active | 1 |

The target datasheet is the authority; a mode number is only a shorthand. Keep
`Input sample phase` at `Middle` for these conventional modes and select `End`
only when the target or the bus needs a later sample. The sniffer follows the
polarity and edge but always samples in the middle (see
[sniffer.md](sniffer.md)).

## The setup prompt chain

Each prompt shows its default in the `(n)>` marker; pressing Enter accepts it.
The chain is **speed, clock polarity, clock edge, input sample phase, CS
polarity, output type**, in that order.

| Order | Prompt | Options | Default |
|---:|---|---|---|
| 1 | `Set speed:` | 1–12 (see the speed table) | `1` `30kHz` |
| 2 | `Clock polarity:` | `1. Idle low` / `2. Idle high` | `1` idle low |
| 3 | `Output clock edge:` | `1. Idle to active` / `2. Active to idle` | `2` active to idle |
| 4 | `Input sample phase:` | `1. Middle` / `2. End` | `1` middle |
| 5 | `CS:` | `1. CS` / `2. /CS` | `2` `/CS` |
| 6 | `Select output type:` | `1. Open drain (H=Hi-Z, L=GND)` / `2. Normal (H=3.3V, L=GND)` | `1` open drain |

**CS polarity.** `CS` is active-high; `/CS` is active-low. The status line adds
a `/` prefix only for the `/CS` polarity: `/CS ENABLED` / `/CS DISABLED` versus
`CS ENABLED` / `CS DISABLED`.

**Output type.** `Open drain (H=Hi-Z, L=GND)` is the default. Its high level is
high impedance and must come from a pull-up. `Normal (H=3.3V, L=GND)` drives
3.3 V high and needs no pull-up, but it cannot produce a 5 V high. The
electrical rules are in [wiring.md](wiring.md).

## The settings header and one-line form

The firmware prints the current settings as:

```
SPI (spd ckp ske smp csl hiz)=( 4 0 1 0 1 0 )
```

The header is **what the firmware prints**, not what you type. To enter the
mode with settings on one line, follow `m5` with six integers in the same
order. The `spd` field is one-based (the menu number); the other five fields
are typed as the menu numbers of their two-option prompts, but the header
prints their raw `0`/`1` value.

| Field | Meaning | Header display | Input value |
|---|---|---|---|
| `spd` | speed | menu number 1–12 | menu number 1–12 |
| `ckp` | clock polarity | `0` idle low, `1` idle high | `1` idle low, `2` idle high |
| `ske` | output clock edge | `0` idle→active, `1` active→idle | `1` idle→active, `2` active→idle |
| `smp` | input sample phase | `0` middle, `1` end | `1` middle, `2` end |
| `csl` | CS polarity | `0` `CS`, `1` `/CS` | `1` `CS`, `2` `/CS` |
| `hiz` | output type | `1` open drain, `0` normal | `1` open drain, `2` normal |

The six fields are positional: a value in the wrong position is accepted if it
is in range and produces the wrong settings, and the echoed header is the only
record of what was applied.

### Direct-form example

```
HiZ>m5 4 1 2 1 2 2
SPI (spd ckp ske smp csl hiz)=( 4 0 1 0 1 0 )
Ready
SPI>
```

That line selects speed `4` (`1MHz`), idle-low clock, active→idle edge, middle
sample, `/CS`, and normal output. Any speed from 1 to 12 is accepted, so
`m5 12 1 2 1 2 2` selects `12` = `5.3MHz` and prints:

```
SPI (spd ckp ske smp csl hiz)=( 12 0 1 0 1 0 )
```

If a value is missing or out of range, the interactive prompts run instead and
the typed values are discarded. Typing the header back verbatim also fails:
its labels, parentheses and `=` are not numbers. If `m5` is entered with no
numbers at all, the six prompts run.

## Bring-up with `W`

Mode entry only parses the settings. Run `W` before the first bus operation —
it puts the on-board rails on their header pins **and** runs the mode's pin and
module setup:

```
SPI>W
POWER SUPPLIES ON
Clutch engaged!!!
SPI>
```

`W` is required even when the target has its own supply; leave the 3.3V and
5.0V pins disconnected from a self-powered target. The success check reads the
Bus Pirate's own 3.3 V and 5.0 V rails, not the target's VCC, so a successful
`W` does not prove the target is powered.

On v4 with on-board pull-ups, select the Vpullup source with `e2`/`e3` before
`W` and enable `P` after it; the numbered order and the voltage warning are in
[wiring.md](wiring.md#bring-up-order-with-the-on-board-pull-ups-v4).

If the rails do not come up, `W` prints the failure pair and does **not** run
the setup:

```
SPI>W
VREG too low, is there a short?
POWER SUPPLIES OFF
SPI>
```

Clear the fault and run `W` again. The bus stays unconfigured until a `W`
succeeds.

`w` does the reverse: it runs the mode cleanup and switches the rails off. The
SPI pins are released to inputs at that point, so the bus is no longer driven:

```
SPI>w
Clutch disengaged!!!
POWER SUPPLIES OFF
SPI>
```

The SPI settings — speed, polarity, edge, sample phase, CS polarity and output
type — are retained across `w`/`W`. Run `W` again to re-engage with the stored
settings.

Two paths do not need the terminal `W`:

- **The sniffer** configures its own pins as inputs and, on exit, runs the
  master pin/module setup: CLK, MOSI and CS become outputs. It does not restore
  the input state and does not switch the supplies; run `W` only if you need
  the rails (see [sniffer.md](sniffer.md)).
- **BASIC's `MODE 5`** applies the mode's defaults and runs its setup, but
  leaves the supplies off. A self-powered target can be used that way; see
  [BASIC bus access](../../tools/basic/bus-access.md).

## Edge cases

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| A value is missing or out of range in the one-line form | the interactive prompts run | type six integers: speed 1–12, then five values 1–2 |
| The settings header is typed back | the prompts run | type the menu numbers, not `spd ckp …` |
| Six values in the wrong positions | accepted if each is in range; the header shows the result | check the echoed header; re-enter the mode to correct |
| A direct `m<N>` number outside the menu | `Nonexistent protocol!`; the mode does not change | use the number from `m` |
| `W` was skipped | writes echo `WRITE: 0xNN` but nothing is clocked; reads return `0xFF`; `v` shows CLK/MOSI/CS as inputs | run `W` |
| `W` prints `VREG too low, is there a short?` | `POWER SUPPLIES OFF` follows; the setup did not run | clear the short, then `W` again |
| `w` was used | the pins are inputs and the supplies are off | `W` re-engages with the stored settings |
| Output type left at the default | the open-drain bus needs a pull-up | select `2` Normal, or fit a pull-up |

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| mode number 5 | yes | yes |
| setup prompts and defaults | same | same |
| speed table | same | same |
| direct and one-line forms | same | same |
| terminal output buffer | 4096 bytes | 4096 bytes |
| `MODE` from BASIC | same | same |

The v3 column is **not verified**.

## Related docs

- [wiring.md](wiring.md) — pins, pull-ups, supplies, VPU.
- [bus-operations.md](bus-operations.md) — CS and byte transfers.
- [macros.md](macros.md) — the macro menu and the in-place tweaks.
- [sniffer.md](sniffer.md) — the sniff macros and the CN path.
- [binary.md](binary.md) — the `SPI1` sub-protocol and its own defaults.
- [edge-cases.md](edge-cases.md) — setup and bring-up failures.
