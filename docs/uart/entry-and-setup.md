# UART — entry & setup

Part of the UART documentation set (`docs/uart/`).  How the mode is entered,
what each setup prompt asks, and what the one-line settings form accepts.
Wiring is in [wiring.md](wiring.md); the half-duplex choice is covered in
full in [hduart.md](hduart.md).

## Entering UART

`UART` is mode number **3** in the mode menu (HiZ is 1).  The menu form is
`m` then `3`; the direct form is `m3`.  The shared mode-menu grammar is owned
by `docs/generic/` (see [navigation.md](../generic/navigation.md) §Mode
menu).  The full menu, for reference:

| Option | Mode |
|---|---|
| `1` | HiZ |
| `2` | 1-WIRE |
| `3` | UART |
| `4` | I2C |
| `5` | SPI |
| `6` | 2WIRE |
| `7` | 3WIRE |
| `8` | KEYB |
| `9` | PIC |
| `10` | DIO |
| `x` | exit (without change) |

Entering UART asks six questions in order: **speed**, **data bits and
parity**, **stop bits**, **receive polarity**, **output type**, and
**duplex**.  The prompt shows `(1)>` for each, so pressing Enter through the
chain takes the first entry every time: **300 bps**, 8/NONE, one stop bit,
idle-1, open-drain, full duplex.  Note that the speed prompt defaults to
`1` = 300 bps, not to 9600 — pick the speed you want explicitly.

### Speed menu

On **v4** the speed prompt offers eleven entries:

| Prompt | Option | Text |
|---|---|---|
| `Set serial port speed: (bps)` | `1` | `300` |
| | `2` | `1200` |
| | `3` | `2400` |
| | `4` | `4800` |
| | `5` | `9600` |
| | `6` | `19200` |
| | `7` | `38400` |
| | `8` | `57600` |
| | `9` | `115200` |
| | `10` | `Input Custom BAUD` |
| | `11` | `Auto-Baud Detection (Activity Required)` |

On **v3** the menu has ten entries and no entry-time auto-baud; option `10`
is a raw divider instead of a rate:

| Prompt | Option | Text |
|---|---|---|
| `Set serial port speed: (bps)` | `1`–`9` | `300` … `115200` (same as above) |
| | `10` | `BRG raw value` |

The v3 column is **not verified**.

### Custom baud (option 10)

- **v4** prompts `Input a custom BAUD rate:` and accepts a rate from `1` to
  `999999`.  The firmware derives the internal divider; the achieved rate is
  quantised, so it will not be exact (the built-in 9600 preset lands at
  9592 bps).
- **v3** prompts `Raw value for BRG (MIDI=127)` and accepts the divider
  directly (`1`–`32767`, the default offered is `34`).  The relationship is
  `BRG = 4,000,000 / rate − 1`, so `127` gives 31250 bps (MIDI) and `416`
  gives ≈ 9592 bps.

**MIDI (31250 bps)** is not a separate menu entry in either build.  Reach it
with a v3 raw BRG of `127`, a v4 custom baud of `31250`, or the binary
speed operand (see [binary.md](binary.md)).

### Entry-time auto-baud (v4, option 11)

Choosing `11` prints `Baud detection selected..` and starts listening on the
MISO receive input (the hardware RX pin).  Once activity arrives it measures
the bit period and prints an estimated and a nearest-common rate
(`Waiting activity...`, then
`Estimated:  <n> bps` / `Calculated: <n> bps`), and configures the port to
that rate.  Pressing a key during the wait aborts with `** Early Exit!` and
leaves the mode at the default rate.  This entry-time option is **v4 only**;
the `(4)` macro is the portable way to measure a rate (see
[macros.md](macros.md)).

### Data bits, stop bits, polarity, output type, duplex

| Prompt | Option | Text | Meaning |
|---|---|---|---|
| `Data bits and parity:` | `1` | `8, NONE *default` | 8 data bits, no parity |
| | `2` | `8, EVEN` | |
| | `3` | `8, ODD` | |
| | `4` | `9, NONE` | nine data bits |
| `Stop bits:` | `1` | `1 *default` | one stop bit |
| | `2` | `2` | two stop bits |
| `Receive polarity:` | `1` | `Idle 1 *default` | standard RS-232 idle (mark) high |
| | `2` | `Idle 0` | inverted idle |
| `Select output type:` | `1` | `Open drain (H=Hi-Z, L=GND)` | the line is only pulled low; a pull-up makes the high |
| | `2` | `Normal (H=3.3V, L=GND)` | push-pull 3.3 V output |
| `Duplex:` | `1` | `Full (TX+RX) *default` | separate TX and RX pins |
| | `2` | `Half (single wire)` | one shared wire — see [hduart.md](hduart.md) |

**UART does prompt for an output type.**  This is unlike I2C, which is fixed
open-drain and asks nothing.  Choosing `1` (open drain) makes the pull-up a
bus requirement; choosing `2` (Normal) drives the line push-pull.  In
half-duplex the output is forced open-drain regardless of this answer.

## Settings header and the one-line form

After setup the mode prints a header followed by the current values, then
`Ready` and the prompt:

```
UART (spd brg dbp sb rxp hiz dup)=( 4 416 0 0 0 1 0 )
Ready
UART>
```

This sample is a **full-duplex** session — the last field, `dup`, is `0`;
a half-duplex session shows `dup` `1`.

The header shows the fields in this order:

| Field | Meaning | Values |
|---|---|---|
| `spd` | speed index | **zero-based** — `0` is the first menu entry (300 bps), `4` is 9600 |
| `brg` | the divider in use | when a custom/raw baud is selected, otherwise the preset divider |
| `dbp` | data bits / parity | `0`=8/NONE, `1`=8/EVEN, `2`=8/ODD, `3`=9/NONE |
| `sb` | stop bits | `0`=1, `1`=2 |
| `rxp` | receive polarity | `0`=idle 1, `1`=idle 0 |
| `hiz` | output type | `0`=normal, `1`=open drain |
| `dup` | duplex | `0`=full, `1`=half (single wire) |

Like the other modes, UART can be entered with its settings on one line
after the mode number, in the same order as the header:

```
m3 spd [brg] dbp sb rxp output duplex
```

`brg` is only read when `spd` is `10` (the custom-baud/raw-BRG entry).  The
values are the **menu** numbers, not the zero-based header values: `spd`
1–11 on v4 (1–10 on v3), `output` 1 = open drain / 2 = normal, `duplex`
1 = full / 2 = half.  A `duplex` of `0` (omitted) means full duplex, so
older parameter lists stay valid.  **The list must be complete**: if any
field is missing or out of range the firmware falls back to the full prompt
chain and asks everything.

Example: `m3 5 1 1 1 1 1` = 9600 bps, 8/NONE, one stop bit, idle 1, open
drain, full duplex.

## Example: the half-duplex setup chain

The setup prompts are shown in full; `<<<` marks what was typed:

```
HiZ>m
1. HiZ
2. 1-WIRE
3. UART
4. I2C
…
(1)>3
Set serial port speed: (bps)
 1. 300
 …
 9. 115200
10. Input Custom BAUD
11. Auto-Baud Detection (Activity Required)
(1)>5                      <<< 9600
Data bits and parity:
 1. 8, NONE *default
 2. 8, EVEN
 3. 8, ODD
 4. 9, NONE
(1)>1
Stop bits:
 1. 1 *default
 2. 2
(1)>1
Receive polarity:
 1. Idle 1 *default
 2. Idle 0
(1)>1
Select output type:
 1. Open drain (H=Hi-Z, L=GND)
 2. Normal (H=3.3V, L=GND)
(1)>1
Duplex:
 1. Full (TX+RX) *default
 2. Half (single wire)
(1)>2                      <<< half duplex
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
UART>
```

The `Clutch disengaged!!!` block is printed until `W` starts the supplies
(v4).  A full-duplex session differs only in the last answer (`1`) and skips
the clutch warning once `W` has been sent.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| mode number 3 | present | present |
| speed options 1–9 | present | present |
| option 10 | raw BRG value | custom baud in bps |
| option 11 auto-baud | — | present |
| output-type prompt | asked | asked |
| duplex prompt | asked | asked |

The v3 column is **not verified**.