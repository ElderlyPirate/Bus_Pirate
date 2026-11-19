# PIC — entry and setup

Part of the PIC documentation set (`docs/programmers/pic/`).  This page owns
the mode number, the two settings, the one-line form and the entry state.
Wiring and the electrical requirements are in [wiring.md](wiring.md); the
current limitations are owned by [edge-cases.md](edge-cases.md).

## Mode number

`PIC` is mode **9** in the default v4 image and mode **8** in the default v3
image.  The v3 build leaves `KEYB` out; the v4 build includes it before PIC.
A custom build shifts the number; the menu is dynamic.  See the mode-menu
note in [the generic navigation page](../../generic/navigation.md).

The menu form is `m` then the number; the direct form is `m9`/`m8`.

## The two settings

The mode asks two questions, in this order:

| Prompt | Answer | Meaning | Default |
|---|---|---|---|
| `Commandmode?` | `1` | **6b/14b** — 6-bit opcodes, 14-bit data | `(1)` |
| | `2` | **4b/16b** — 4-bit opcodes, 16-bit data | |
| `Delay?` | `0`–`3` | milliseconds added after each command write | `(1)` |

Pressing Enter through both prompts selects 6b/14b with a 1 ms delay.

The exact prompt text and answers:

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

The menu form (`m`, then `9`) also prints `Clutch disengaged!!!` and
`To finish setup, start up the power supplies with command 'W'` before
`Ready`.  The direct form `m9` prints only `Ready`.

## Width and family

The first answer is the **programming-protocol width**, not the instruction
word size.  Match it to the part's programming specification:

| Width | Opcode / payload | Typical families | `(1)` devID |
|---|---|---|---|
| **6b/14b** | 6-bit command / 14-bit data | PIC10/12/16 | works |
| **4b/16b** | 4-bit command / 16-bit data | PIC18 | `Not implemented (yet)` |

PIC24/dsPIC (24-bit) parts are not served by either terminal width; the
raw-wire 24-bit engine is the binary path (see [binary.md](binary.md)).  A
wrong width makes the entry sequence or the ID read fail.  The family split
is a convention, not a firmware table; take the width from the part's
programming specification.

## The one-line settings form

The mode line accepts the two settings after the mode number: width first,
then delay.

| Typed | Field | Accepted | Meaning |
|---|---|---|---|
| 1st number | width | `1`–`2` | `1` = 6b/14b, `2` = 4b/16b |
| 2nd number | delay | `0`–`3` | delay in milliseconds |

A valid pair is applied and echoed; the prompts are skipped and the firmware
prints a one-line header:

```
HiZ>m 9 2 1
PIC(mod dly)=(4 1)
Ready
PIC>
```

The header fields:

| Field | Printed | Meaning |
|---|---|---|
| `mod` | `6` / `4` | opcode width: `6` = 6b/14b, `4` = 4b/16b |
| `dly` | `0`–`3` | post-command delay in milliseconds |

A complete pair is the only form that skips the prompts.  An incomplete pair
(`m 9 2`, missing delay) or any value outside the ranges is a parse error:
both prompts run from the top, nothing from the partial line is applied,
there is no settings header and no `Syntax error` line.  `m 9 2` therefore
asks `Commandmode?` and `Delay?` and uses your answers.  A complete pair
(`m 9 2 1`) is unchanged: it is applied and echoed.  See
[edge-cases.md](edge-cases.md).

## State at entry

The Command/Data state is part of the mode, not the prompt.  Entry and
cleanup both reset it to **Data**, so a fresh session starts ready for
payloads.  `[` changes to Command, `]` changes to Data; the banner tells you
which state is active.  See [bus-operations.md](bus-operations.md).

The entry sequence also sets the mode to **open drain** (there is no
output-type prompt), sets up the display width and drives PGC (CLK) and PGD
(MOSI) low, so the target sees a quiet bus before power is applied.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| mode number in the default image | 8 | 9 |
| prompts and defaults | same | same |
| one-line form | same | same |
| VPU source (`e`) | external feed only | `e1` external/none, `e2` 3.3 V, `e3` 5.0 V |
