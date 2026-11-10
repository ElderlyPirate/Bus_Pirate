# PIC — entry & setup

Part of the PIC documentation set (`docs/programmers/pic/`).  How the mode is
entered, what it asks, and what the one-line settings form accepts.  Wiring
and the electrical requirements are in [wiring.md](wiring.md); current
limitations are owned by [edge-cases.md](edge-cases.md).

## Entering PIC

`PIC` is mode number **9** in the **default v4 image** and mode number **8**
in the **default v3 image**.  The shift is because the keyboard mode (`KEYB`)
sits between 3WIRE and PIC on v4 but is not compiled into the v3 build.  A
custom build with a different enabled-mode set shifts the number; the menu is
dynamic.  See the mode-menu note in
[the generic navigation page](../../generic/navigation.md).

The menu form is `m` then `9`/`8`; the direct form is `m9`/`m8`.

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

## Width ↔ family

The first choice is the **programming-protocol width**, not the instruction
word size.  Match it to the part's programming specification:

| Width | Opcode / payload | Typical parts | `(1)` devID |
|---|---|---|---|
| **6b/14b** | 6-bit command / 14-bit data | PIC10/12/16 families | works |
| **4b/16b** | 4-bit command / 16-bit data | PIC18 family | `Not implemented (yet)` |

PIC24/dsPIC (24-bit) parts are not served by either terminal width; the host
tool drives the raw-wire 24-bit engine.  If a part is not listed, take the
width from its programming specification or the host tool's part entry — a
wrong width makes the entry or ID read fail.

The built-in `(1)` macro works only in **6b/14b**; in **4b/16b** use the host
tool ([macros.md](macros.md)).

## The one-line settings form

Like the other modes, PIC accepts its settings on the mode line: after the
mode number it reads two integers (width, then delay).

| Typed | Field | Accepted | Meaning |
|---|---|---|---|
| 1st number | width | `1`–`2` | `1` = 6b/14b, `2` = 4b/16b |
| 2nd number | delay | `0`–`3` | delay in milliseconds |

A valid pair is **applied and echoed**; the prompts are skipped and the
firmware prints a one-line settings header:

```
HiZ>m 9 2 1
PIC(mod dly)=(4 1)
Ready
PIC>
```

The header fields are:

| Field | Printed value | Meaning |
|---|---|---|
| `mod` | `6` / `4` | opcode width: `6` = 6b/14b, `4` = 4b/16b |
| `dly` | `0`–`3` | the post-command delay in milliseconds |

The two numbers are the settings the mode runs with.  If the width is not
`1`–`2` or the delay is not `0`–`3`, the mode runs the two interactive
prompts instead.

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
[generic commands](../../generic/commands.md).

Unlike SPI, I2C and 2WIRE, PIC has **no output-type prompt**: it is always
open-drain, so a pull-up is required (see [wiring.md](wiring.md)).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| mode number in the default image | 8 | 9 |
| setup prompts and defaults | present | present |
| one-line form | present | present |
| `e` VPU source | — (feed externally) | present |

The two builds ask the same two questions with the same wording.  The v3
build and its default mode number 8 are derived from the v3 build
configuration and are **not verified** on hardware.
