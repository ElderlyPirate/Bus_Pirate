# PIC — entry & setup

Part of the PIC documentation set (`docs/pic/`).  How the mode is entered,
what it asks, and what the one-line settings form accepts.  Wiring and the
electrical requirements are in [wiring.md](wiring.md).

## Entering PIC

`PIC` is mode number **9** in the v4 mode menu and mode number **8** in the
v3 menu.  The shift is because the keyboard mode (`KEYB`) sits between 3WIRE
and PIC on v4 but is **not compiled into the v3 build**.  The menu form is `m`
then `9`/`8`; the direct form is `m9`/`m8`.  The shared mode-menu grammar is
owned by `docs/generic/` — see
[navigation.md](../../generic/navigation.md) §Mode menu.

There are two settings, asked in this order:

| Prompt | Option | Meaning | Default |
|---|---|---|---|
| `Commandmode?` | `1` | **6b/14b** — 6-bit command opcodes, 14-bit data payloads | `(1)` |
| | `2` | **4b/16b** — 4-bit command opcodes, 16-bit data payloads | |
| `Delay?` | `0`–`3` | delay in milliseconds inserted after each command write | `(1)` |

Pressing Enter through both prompts selects **6b/14b with a 1 ms delay**.

The exact prompt text is:

```
HiZ>m9
Commandmode?
1. 6b/14b
2. 4b/16b
(1)>1
Delay?
(1)>1
Ready
PIC>
```

Entering through the `m` menu (`m` then `9`) also prints the parking lines
`Clutch disengaged!!!` / `To finish setup…` before `Ready`; the direct form
`m9` prints only `Ready`.  The bus stays parked until `W` (see below).

## The one-line settings form

Like the other modes, PIC accepts its settings on the mode line: after the
mode number it reads two integers (width, then delay).

| Typed | Field | Accepted | Meaning |
|---|---|---|---|
| 1st number | width | `1`–`2` | `1` = 6b/14b, `2` = 4b/16b |
| 2nd number | delay | `0`–`3` | delay in milliseconds |

When a valid pair is given, the prompts are skipped and the firmware prints a
one-line settings header instead:

```
HiZ>m 9 1 0
PIC(mod dly)=(6 0)
Ready
PIC>
```

The header fields are:

| Field | Printed value | Meaning |
|---|---|---|
| `mod` | `6` / `4` | opcode width: `6` = 6b/14b, `4` = 4b/16b |
| `dly` | `0`–`3` | the post-command delay in milliseconds |

**Quirk — the one-line form does not apply its numbers.**  In this build the
two integers are validated to decide whether to show the prompts, but they
are not stored; the header prints the mode's internal defaults (`6 0`) and
the mode keeps them.  One-line entry therefore runs at **6b/14b with a 0 ms
delay** — the `0` in the header is the delay actually used, not the `(1)` the
interactive prompt shows.  To choose something other than the defaults, use
the interactive prompts (`m9`/`m8` with no numbers on the line).  The 4b/16b
selection via the one-line form is **unverified**.

## Power and pull-ups

Entering the mode parks the bus and sets up the pins: the mode drives PGD
(MOSI) and PGC (CLK) **low** immediately at entry so the target sees a quiet
bus before power is applied.  To bring the link up:

| Step | Command | Effect |
|---|---|---|
| engage | `W` | `POWER SUPPLIES ON` + `Clutch engaged!!!`; target VCC on the supply pin |
| source the pull-up voltage (v4) | `e2` / `e3` | `3.3V` / `5.0V` on-board pull-up voltage enabled |
| enable the pull-ups | `P` | `Pull-up resistors ON` |
| park again | `w` (`p` releases pull-ups) | `Clutch disengaged!!!` |

**Feed VPU before `P`.**  `P` ties the I/O lines to the voltage already on
the physical Vpullup pin; if nothing is on VPU the firmware prints
`Warning: no voltage on Vpullup pin` and the open-drain lines have no
idle-high.  On v4 `e2`/`e3` provides the source; on v3 the Vpullup pin must
be fed externally.  These are base-layer elements — see
[commands.md](../../generic/commands.md).

Unlike SPI, I2C and 2WIRE, PIC has **no output-type prompt**: it is always
open-drain, so a pull-up is required (see [wiring.md](wiring.md)).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| mode number | 8 | 9 |
| setup prompts and defaults | present | present |
| one-line form | present | present |
| `e` VPU source | — (feed externally) | present |

The two builds ask the same two questions with the same wording.  The v3
build and its mode number 8 are derived from the v3 build configuration and
are **not verified** on hardware.