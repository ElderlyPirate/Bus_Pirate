# UART — entry & setup

Part of the UART documentation set (`docs/protocol/uart/`).  This page owns
how the mode is entered, the six setup questions and their defaults, custom
baud, entry-time auto-baud, the settings header, and the one-line settings
form.  Wiring is in [wiring.md](wiring.md); the half-duplex behaviour is in
[hduart.md](hduart.md); bring-up with `W` is below.

## Entering UART

`UART` is mode number **3** (HiZ is 1).  The menu is built from the enabled
protocols, so the exact list depends on the build; `UART` is 3 in the default
v3 and v4 menus.  The shared menu grammar is in
[docs/generic/](../../generic/navigation.md).

| Option | Mode |
|---|---|
| `1` | HiZ |
| `2` | 1WIRE |
| `3` | UART |
| `4` | I2C |
| `5` | SPI |
| `6` | 2WIRE |
| `7` | 3WIRE |
| `8` | KEYB |
| `9` | PIC |
| `10` | DIO |
| `x` | `exit(without change)` |

That is the default v4 image; take the numbers from the `m` output of the
build in front of you.  The firmware spells the 1-Wire mode `1WIRE`; prose
may write `1-Wire`.

Three forms enter the mode:

| Form | What happens |
|---|---|
| `m` [Enter], then `3` at `(1)>` | menu form; the setup prompts run |
| `m3` or `m 3` | direct form; the setup prompts run |
| `m3 <values>` or `m 3 <values>` | direct form with settings on one line; the settings header prints |

The menu form and the direct form differ in what they print after the setup:

| Form | After the settings are stored |
|---|---|
| `m` then `3` | `Clutch disengaged!!!`, then the `W` reminder, then `Ready` |
| `m3` | `Ready` only |
| `m3 <values>` | the settings header, then `Ready` |

All three end at the `UART>` prompt with the terminal port **not yet
configured**.  The prompts and the one-line form only store the settings;
`W` applies them (see §Bring-up with `W`).

The six questions are asked in this order: **speed**, **data bits and
parity**, **stop bits**, **receive polarity**, **output type**, **duplex**.
Each prompt shows its default in `(1)>`, so pressing Enter through the chain
selects 300 bps, 8/NONE, one stop bit, idle 1, open drain, full duplex.  The
speed prompt defaults to `1` = 300 bps, not 9600 — pick the speed explicitly.

### Menu-form example

The default v4 menu is shown; the setup chain is complete.  The answers pick
9600 bps, 8/NONE, one stop, idle 1, open drain, and **half duplex**.  The
blank line before each `(1)>` is printed by the prompt reader:

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

(1)>3
Set serial port speed: (bps)
 1. 300
 2. 1200
 3. 2400
 4. 4800
 5. 9600
 6. 19200
 7. 38400
 8. 57600
 9. 115200
10. Input Custom BAUD
11. Auto-Baud Detection (Activity Required)

(1)>5
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

(1)>2
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
UART>
```

A full-duplex session differs only in the last answer (`1`).  The menu form
prints `Clutch disengaged!!!` and the `W` reminder **once**, at the mode
change; the direct `m3` form prints neither.

## The speed menu

On **v4** the prompt is `Set serial port speed: (bps)` and offers eleven
entries:

| Prompt | Option | Text | Rate |
|---|---|---|---|
| `Set serial port speed: (bps)` | `1` | `300` | 300 bps |
| | `2` | `1200` | 1200 bps |
| | `3` | `2400` | 2400 bps |
| | `4` | `4800` | 4800 bps |
| | `5` | `9600` | 9600 bps |
| | `6` | `19200` | 19200 bps |
| | `7` | `38400` | 38400 bps |
| | `8` | `57600` | 57600 bps |
| | `9` | `115200` | 115200 bps |
| | `10` | `Input Custom BAUD` | see §Custom baud |
| | `11` | `Auto-Baud Detection (Activity Required)` | see §Entry-time auto-baud |

On **v3** the menu has ten entries; option `10` is a raw divider:

| Prompt | Option | Text | Meaning |
|---|---|---|---|
| `Set serial port speed: (bps)` | `1`–`9` | `300` … `115200` | same rates as above |
| | `10` | `BRG raw value` | raw divider, see §Custom baud |

The presets are nominal.  The divider is the nearest whole number to
`4,000,000 / rate`, minus one, so the actual rate is quantised: the 9600
preset lands at about 9592 bps and the 115200 preset at about 114.3 kbaud.

## Custom baud (option 10)

| Build | Prompt | Range | Unit | Default |
|---|---|---|---|---|
| v4 | `Input a custom BAUD rate:` | 62–999999 | bps | 115200 |
| v3 | `Raw value for BRG (MIDI=127)` | 1–32767 | divider | 34 |

On **v4** the firmware chooses the nearest representable divider for the
requested rate; the achieved rate is quantised, not exact.  A value below 62
is rejected and the prompt re-asks.  Options above 999999 are rejected the
same way.

On **v3** the value is the divider itself.  The relationship is
`rate = 4,000,000 / (BRG + 1)`, so `127` is 31250 bps (MIDI), `34` is about
115200 bps, and `416` is about 9592 bps.  An empty line takes the default 34.
Zero, a value above 32767, or an over-long entry prints
`Invalid choice, try again` and re-asks at `(34)>`; non-digit characters are
ignored.

```
(1)>10
Input a custom BAUD rate:

(115200)>31250
```

On v3 the same choice reads:

```
(1)>10
Raw value for BRG (MIDI=127)

(34)>127
```

### One-line `brg` is not the same on both builds

The one-line form has a `brg` field only when `spd` is `10`.  Its meaning
follows the build, not the prompt:

| Build | One-line `brg` | Range |
|---|---|---|
| v4 | a **baud rate in bps** | 62–65535 |
| v3 | a **raw BRG divider** | 1–32767 |

So `m3 10 9600 …` means 9600 bps on **v4**, but on **v3** it loads divider
9600 (about 416 bps).  On v3 use the raw divider, for example `m3 10 416 …`
for about 9600 bps.  The one-line field is 16-bit on v4, so the highest rate
it can express is 65535 bps; use the prompt path for higher rates.

### MIDI (31250 bps)

MIDI is not a speed-menu entry in either build.  It is reachable by several
routes:

| Build | Route | Value |
|---|---|---|
| v3 | custom prompt | `127` |
| v4 | custom prompt | `31250` |
| either | one-line form | `m3 10 127 …` (v3) or `m3 10 31250 …` (v4) |
| either | binary preset | `0x69` |
| either | binary divider | `0x007F` |

The binary preset index `9` is 31250; see [binary.md](binary.md).

## Entry-time auto-baud (v4, option 11)

Choosing `11` at the speed prompt only **records** the choice and prints
`Baud detection selected..`.  The measurement runs when `W` starts the
supplies, after the remaining five prompts.  This option is **v4 only**; the
`(4)` macro is the portable way to measure a rate (see [macros.md](macros.md)).

At `W` the mode listens on the **active receive input** — MISO in full
duplex, the shared MOSI line in half duplex — measures the bit period, and
prints an estimated and a nearest-common rate:

```
UART>W
POWER SUPPLIES ON
Waiting activity...

Estimated:  9601 bps
Calculated: 9600 bps
Clutch engaged!!!
UART>
```

| Condition | Result |
|---|---|
| key pressed during the wait | `** Early Exit!`; the measurement is abandoned |
| no usable activity within five seconds | `** Early Exit!` |
| measurement timer already in use | `** Early Exit!` |
| early exit, or no rate matched | the port stays at **115200 bps**, not the 300 bps menu default |
| measured rate above about 150000 bps | `** Baud>150k: The BP cannot measure above 150000, Done.`; a reading up to the 256000 table end still maps to 128000 or 256000, above it falls back to 115200 |
| later `W` | no second measurement; the first result is stored |

Until `W` runs, the stored speed is the temporary 115200 (`spd 8` in the `i`
header); the measured result replaces it at `W`.  `Calculated:` is picked from
a wider table than the menu and can report 600, 14400, 28800, 56000, 128000,
and 31250 bps.  The port is configured to that rate (shown as `spd 9` with the
divider in `brg`).  A measured rate above 150000 prints the overflow message
instead of the `Estimated:`/`Calculated:` pair; the entry-time path still
selects 128000 or 256000 from the table, and a reading above the 256000 end
falls back to 115200.

## Frame and line settings

| Prompt | Option | Text | Meaning |
|---|---|---|---|
| `Data bits and parity:` | `1` | `8, NONE *default` | 8 data bits, no parity |
| | `2` | `8, EVEN` | 8 data bits, even parity |
| | `3` | `8, ODD` | 8 data bits, odd parity |
| | `4` | `9, NONE` | 9 data bits, no parity |
| `Stop bits:` | `1` | `1 *default` | one stop bit |
| | `2` | `2` | two stop bits |
| `Receive polarity:` | `1` | `Idle 1 *default` | standard idle-high (mark) |
| | `2` | `Idle 0` | inverted idle |
| `Select output type:` | `1` | `Open drain (H=Hi-Z, L=GND)` | the line is only pulled low; a pull-up makes the high |
| | `2` | `Normal (H=3.3V, L=GND)` | push-pull 3.3 V output |
| `Duplex:` | `1` | `Full (TX+RX) *default` | separate TX and RX pins |
| | `2` | `Half (single wire)` | one shared wire — see [hduart.md](hduart.md) |

UART **does** ask for an output type, unlike I2C.  The default is **open
drain**, so the TX line needs a pull-up unless you choose Normal.  Selecting
`1` makes the pull-up a wiring requirement; selecting `2` drives the line
push-pull at 3.3 V.  See [wiring.md](wiring.md).

**Half duplex overrides the output answer.**  Selecting `2` at `Duplex:`
stores the output type as **open drain** whatever was chosen; the settings
header's `hiz` field and the `e`/`P` pull-up controls use that effective
type.  Half duplex also requires **idle-high** receive polarity.  With
`Idle 0` selected, the `Duplex:` prompt accepts only `1. Full`; typing `2`
prints `Invalid choice, try again` and re-asks.  A one-line entry that combines
idle 0 with half duplex falls back to the full prompt chain.

The 9-bit choice sets the frame to nine data bits.  Values are then written
and read as 0–511; the generic `;N` width changes the displayed width, not
the frame.

## The settings header

The header appears only on a complete one-line entry and in the `i` status
block.  The prompt chain and `v` do not print it:

| Trigger | Does the header print? |
|---|---|
| a complete one-line entry (`m3 …`) | yes, before `Ready` |
| the info command `i` | yes, as the last content line of the status block, after the `Pinstates:` pin table |
| the interactive prompt chain (`m` then `3`, or `m3` with no values) | no |
| `v` | no — it prints the pin table only |

A complete one-line entry:

```
HiZ>m3 5 1 1 1 1 1
UART (spd brg dbp sb rxp hiz dup)=( 4 416 0 0 0 1 0 )
Ready
UART>
```

The header is a settings echo, not a live status: it prints before `W`, and
after `W` it still reports the stored values.  The fields are:

| Field | Meaning | Values |
|---|---|---|
| `spd` | speed index | **zero-based** — `0` is the first menu entry (300 bps), `4` is 9600, `8` is 115200, `9` is custom baud or an auto-baud result |
| `brg` | the divider loaded into the generator | preset divider, custom v4 divider, or v3 raw value |
| `dbp` | data bits / parity | `0`=8/NONE, `1`=8/EVEN, `2`=8/ODD, `3`=9/NONE |
| `sb` | stop bits | `0`=1, `1`=2 |
| `rxp` | receive polarity | `0`=idle 1, `1`=idle 0 |
| `hiz` | effective output type | `0`=normal, `1`=open drain; half duplex shows `1` |
| `dup` | duplex | `0`=full, `1`=half (single wire) |

The header numbers are **not** the one-line input numbers for `spd`, `brg`,
`hiz` and `dup`:

| Menu number (prompt) | One-line `spd` | Header `spd` |
|---|---|---|
| `1` (300 bps) | `1` | `0` |
| `5` (9600 bps) | `5` | `4` |
| `9` (115200 bps) | `9` | `8` |
| `10` (custom) | `10` + `brg` | `9` (custom or auto-baud) |

In particular, header `spd 4` is 9600 bps, but one-line `4` is menu entry 4
= 4800 bps, and the header `brg` is always a divider.  Do not paste a header
line back as a settings line; the `brg` field shifts the rest and the entry
falls back to the prompts.

## The one-line settings form

```
m3 spd [brg] dbp sb rxp output duplex
```

Spaces are allowed between `m`, the mode number and the values; `m 3` works
too.  The fields are one integer each, in this order:

| Field | Accepted | Notes |
|---|---|---|
| `spd` | `1`–`10` | menu number; `11` (auto-baud) is prompt-only |
| `brg` | present only with `spd` `10` | v4 bps 62–65535; v3 raw 1–32767 |
| `dbp` | `1`–`4` | `1` 8/NONE, `2` 8/EVEN, `3` 8/ODD, `4` 9/NONE |
| `sb` | `1`–`2` | `1` one stop, `2` two |
| `rxp` | `1`–`2` | `1` idle 1, `2` idle 0 |
| `output` | `1`–`2` | `1` open drain, `2` normal |
| `duplex` | `1`–`2`, or omitted | `1` full, `2` half; omitted means full |

The fields through `output` are required; `duplex` may be omitted (full
duplex).  An explicit `duplex` of `0` is out of range, not a synonym for
full.  If any **required** field (through `output`) is missing or out of
range, the firmware falls back to the full prompt chain and asks everything
again; the typed values are discarded.

`m3 5 1 1 1 1 1` selects 9600 bps, 8/NONE, one stop, idle 1, open drain,
full duplex.  A custom-baud example on v4:

```
HiZ>m3 10 9600 1 1 1 2 1
UART (spd brg dbp sb rxp hiz dup)=( 9 416 0 0 0 0 0 )
Ready
UART>
```

The header shows `spd 9` (the zero-based custom entry), the divider 416, and
`hiz 0` because `output 2` chose normal.

## Bring-up with `W`

Mode entry only stores the settings.  Run `W` before the first bus operation
— it starts the on-board rails **and** runs the UART pin and module setup:

```
UART>W
POWER SUPPLIES ON
Clutch engaged!!!
UART>
```

- `W` is required even when the target has its own supply; leave the 3.3 V
  and 5.0 V pins disconnected from a self-powered target.  The success check
  reads the Bus Pirate's own rails, not the target's VCC.
- After mode entry, the UART port is not configured until a `W` succeeds:
  writes are not transmitted and no new words are received.  The disable that
  ended the previous session cleared the queued receive words and last-word
  flags, so `r` before `W` reports `READ: FAILED, NO DATA0x00`; the overrun
  counter survives until the next `W`.  Before `W`, `v` shows `TxD` as an
  input; after `W`, `TxD` is an output.
- `W` re-runs the setup and enables the port.  Enabling resets the receive
  ring and the overrun counter.
- If the rails do not come up, `W` prints the failure pair and does **not**
  run the setup:

```
UART>W
VREG too low, is there a short?
POWER SUPPLIES OFF
UART>
```

  Clear the fault and run `W` again.  The setup did not run, so this `W` did
  not (re)configure the port.
- `w` does the reverse: it runs the mode cleanup, switches the rails off and
  disables the port.  The cleanup also clears the queued receive words and
  last-word flags; the overrun counter survives until the next `W`.  The
  stored settings remain; `W` re-engages with them.

```
UART>w
Clutch disengaged!!!
POWER SUPPLIES OFF
UART>
```

BASIC's `MODE 3` applies the UART defaults and runs the setup without the
terminal `W`; see [BASIC bus access](../../tools/basic/bus-access.md).

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| mode number 3 | yes | yes |
| speed menu | 10 entries (option `10` = raw BRG) | 11 entries (option `10` = custom baud, `11` = auto-baud) |
| custom prompt | raw BRG, 1–32767 | bps, 62–999999 |
| one-line `brg` with `spd 10` | raw BRG, 1–32767 | bps, 62–65535 |
| entry-time auto-baud | — | option `11` |
| data bits / parity / stop / polarity | same | same |
| output-type prompt | asked | asked |
| duplex prompt | asked; idle 0 allows full only | same |
| half duplex | idle 1 required; output forced open drain | same |
| terminal output buffer | 4096 bytes | 4096 bytes |
| `W` required to configure | yes | yes |

## Related docs

- [wiring.md](wiring.md) — full-duplex and HDUART wiring, pull-ups, VPU.
- [terminal-usage.md](terminal-usage.md) — sending, reading and the live
  display.
- [hduart.md](hduart.md) — the half-duplex single-wire mode.
- [macros.md](macros.md) — the `(4)` auto-baud macro.
- [binary.md](binary.md) — the binary layer's own setup defaults.
