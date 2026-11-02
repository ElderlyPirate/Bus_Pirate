# HiZ — what it is and the pin-state model

Part of the HiZ documentation set (`docs/hiz/`). This page owns the idle mode
itself: what "high-impedance" means on this hardware, what the `v` table shows
in HiZ, the safety properties, and how you enter and leave the mode. The
shared prompt grammar and the general command tables are in `docs/generic/`.

## What HiZ is

HiZ is the firmware's idle/default protocol. Its name is the mode string the
firmware prints — `HiZ` — and it is the prompt you land on:

```
HiZ>
```

In HiZ the firmware does not clock, send or receive anything on the bus. The
protocol table for HiZ has no bus operations: every start, stop, read, write,
clock or bit operation is a no-op that reports `ERROR: command has no effect
here` (see [commands.md](commands.md)). Only the general commands and the
self-test act.

## Pin-state model

"High-impedance" (Hi-Z) means a pin is configured as an **input**: its output
driver is switched off, so the Bus Pirate neither drives the pin high nor low.
The pin's voltage is then set entirely by whatever external circuit is
connected to it; with nothing connected, the pin floats and its measured state
is indeterminate.

In HiZ every header I/O pin is put into this state. The four bus lines
(MOSI, CLK, MISO, CS) and the AUX pins are all inputs, and the switch that
would connect an on-board supply or the pull-up resistors is off.

The `v` command prints the pin table; the HiZ table is:

```
v
Pinstates:
12.(Bk)  11.(Wt)  10.(Gy)  9.(Pu)  8.(Bu)  7.(Gn)  6.(Yw)  5.(Or)  4.(Rd)  3.(Br)  2.(Bk)  1.(Wt)*
GND      5.0V     3.3V     VPU     ADC     AUX2    AUX1    AUX     CS      MISO    CLK     MOSI
P        P        P        I       I       I       I       I       I       I       I       I
GND      0.00V    0.00V    0.00V   0.00V   L       L       H       L       H       H       H
HiZ>
```

\* the `1.` legend label of pin 1 is garbled by a known firmware string
defect; the values are unaffected.

Read the table by row:

| Row | Meaning |
|---|---|
| `12.(Bk) … 1.(Wt)` | pin number and wire colour legend |
| `GND 5.0V 3.3V VPU ADC AUX2 AUX1 AUX CS MISO CLK MOSI` | signal name for each pin in this mode |
| `P P P I …` | pin **direction**: `P` = fixed power/analogue, `I` = input, `O` = output |
| `GND 0.00V …` | measured rail voltages, then each I/O pin **state** (`H`/`L`) |

- The first columns (`GND`, `5.0V`, `3.3V`, `VPU`, `ADC`) are fixed
  power/analogue pins and always read `P`; the rest are the I/O pins.
- In HiZ every I/O pin shows direction `I` — none is driven.
- The state row lists the measured rail voltages, then one **state** (`H`/`L`)
  per I/O pin, read back from the pin. With nothing connected they float, so
  they read whatever the pin happens to settle at; a `H`/`L` reading here is
  not a driven level.
- The signal names in HiZ are the default/SPI names (`CS`, `MISO`, `CLK`,
  `MOSI`). A mode that renames its pins prints its own names in this row — see
  `docs/generic/usage.md` for an example.

The general table format (row order, `P`/`I`/`O`, `H`/`L`, the pin-1 legend
defect) is documented once in `docs/generic/`; this page only states what HiZ
puts in it.

### AUX targets and pin names

`c`/`C` (and `k`/`K` on v4) choose which pin the `a`/`A`/`@` commands act
on. The firmware names the same target two ways: the `?` help lists the
choices by their `A0`/`CS`/`A1`/`A2` token, while the confirmation message
and the `v` table use the pin name. Both name the same pin:

| `c` target (`?`-help token) | Select with | Confirmation message | Pin name in the `v` table |
|---|---|---|---|
| `A0` (`aux` on v3) | `c` | `a/A/@ controls AUX pin` | `AUX` |
| `CS` | `C` | `a/A/@ controls CS pin` | `CS` |
| `A1` (v4) | `k` | `a/A/@ controls AUX1 pin` | `AUX1` |
| `A2` (v4) | `K` | `a/A/@ controls AUX2 pin` | `AUX2` |

After a board reset the default target is `A0` — the pin shown as `AUX` (see
"What `#` does" above for the paths that reset board state). v3 offers only
the first two targets (`A0`/`AUX` and `CS`), and its help prints the pair as
`aux/CS`. This set uses the pin-name form (`AUX`, `CS`, `AUX1`, `AUX2`)
throughout.

## Safety

- **Nothing is driven.** All header I/O pins are inputs, so HiZ cannot damage a
  target by driving a line against it.
- **No supply.** The on-board voltage regulator is off, and `W` (supplies on)
  is refused in HiZ, so no target VCC is sourced.
- **No pull-up.** The pull-up resistors are off and `P` is refused in HiZ; a
  mode change into HiZ also releases them.
- **The AUX pin can still be driven.** `A` drives the selected AUX target pin
  high and `a` drives it low — those commands are not restricted to a bus
  mode.  A high-impedance pin becomes an active output only when you ask,
  with `A` or `a`.
- **Before the self-test, disconnect the target.** `~` drives the Bus Pirate's
  own pins and supply; see `docs/self-test/`.

## Entering and leaving

HiZ is the default. It is where the firmware starts, and it is where resets
land.

| Action | Result |
|---|---|
| power-up / boot | prompt is `HiZ>`.  On v3 the version banner is printed at boot; on v4 it is not — only the prompt appears until you type `i`. |
| `m` then `1` (menu form) | switches to HiZ: prints `Clutch disengaged!!!` then `Ready`; prompt `HiZ>` |
| `m1` (direct form) | switches to HiZ: prints `Ready`; prompt `HiZ>`.  HiZ has no setup prompts to answer. |
| `#` | reset: see below.  Also reached by the binary layer's reset byte and by leaving SUMP. |

Worked transcripts are in [usage.md](usage.md).

### What `#` does

- **v4 — simulated reset.** `#` prints `RESET`, changes the mode back to HiZ
  (so the prompt reads `HiZ>`), and prints the version banner. It resets the
  **mode** and prints the **banner**; it does **not** run the board-state
  reset, so it does not re-impede the I/O pins, release the pull-ups, or
  switch the supplies off. Those keep whatever state the previous mode left.
  The binary layer's reset byte (`0x0F`) and the exit from the SUMP logic
  analyzer land on the same HiZ mode + banner; they, too, do not run the full
  board-state reset. This mode + banner result is what host tools wait for
  (`HiZ>`) after they reset the bus.
- **v3 — real reset.** `#` prints `RESET` and performs a hardware
  reset/reboot of the board; the reboot re-initialises the board, so its
  pins, pull-ups and supplies are reset too.

The version banner printed by a reset is the `i`-style block; the git hash
line is baked in at build time, so your build's hash differs.

To return to HiZ **and** release the board state, leave a mode with `m`/`m1`
instead: entering HiZ that way runs the board-state reset — the I/O pins go
back to inputs, the pull-ups are released, and the supplies are switched off.
The `$` bootloader path runs the same board-state reset before it reboots,
and on v3 the `#` reboot covers it. A user who wants the safe idle state
therefore uses `m1`, not `#`.

## v3 / v4 differences

| Aspect | v3 | v4 |
|---|---|---|
| `v` table columns | 10 pins; `GND 3.3V 5.0V ADC VPU AUX CLK MOSI CS MISO` | 12 pins; `GND 5.0V 3.3V VPU ADC AUX2 AUX1 AUX CS MISO CLK MOSI` |
| VPU pull-up source `e` | — (no such command) | present; refused in HiZ |
| AUX assignment | `AUX` / `CS` | `AUX` / `CS` / `AUX1` / `AUX2` |
| boot banner | printed at boot | not printed; only the prompt appears |
| `#` reset | real reboot | simulated reset to HiZ + banner |

The v3 column is **not verified** (v3 is build-only here).

## Related docs

- [README.md](README.md) — the set map.
- [commands.md](commands.md) — what works, what is refused, what is HiZ-only.
- [usage.md](usage.md) — worked sessions.
- [edge-cases.md](edge-cases.md) — failure modes and recovery.
- [../generic/README.md](../generic/README.md) — the shared prompt layer.
- [../self-test/README.md](../self-test/README.md) — the `~` self-test.