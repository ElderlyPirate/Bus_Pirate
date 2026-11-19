# DIO — parking, leaving and reset

Part of the DIO documentation set (`docs/protocol/dio/`).  This page owns the
safe-park contract: what "parked" means, when the mode releases the pins, and
how `w`, `#` and a mode change differ.  Entry is in
[entry-and-setup.md](entry-and-setup.md).

## What "parked" means

Direct I/O is safe to leave only when every pin it drives is released.  The
mode's cleanup parks seven pins on v4 and five on v3: the five signal pins on
both boards, plus `AUX1` and `AUX2` on v4.  It does five things, in this
order:

1. set the parked pins to **input** (the five signal pins, plus `AUX1` and
   `AUX2` on v4);
2. switch the pull-up resistors **off**;
3. deselect the on-board VPU source (v4);
4. clear the matching output **latches** to low;
5. restore the width to **8 bits**.

The order is deliberate: the pins stop driving before the bias is removed, and
the latches are cleared after, so a pin that is made an output later cannot
drive a level left over from before the park.  The cleanup is idempotent;
running it again changes nothing.

`w` performs this park and then switches the supply off.  On v4 the park
includes both `AUX1` and `AUX2`, regardless of which AUX target is selected.
The board reset and a mode change add the rest of the board state: the AUX
target returns to `AUX` (`A0`), the AUX output routing and the `AUX1`
open-drain setting are cleared, the bit order returns to MSB-first, and the
mode configuration is cleared.

## When it runs

| Action | What runs | Supply | Pins after | Prompt after |
|---|---|---|---|---|
| `w` | direct I/O cleanup | off | five signal pins input (v3); seven pins input (v4), pull-ups off, latches low | `DIO>` |
| `#` (v4) | cleanup + board reset | off | all I/O input, including `AUX1` / `AUX2` | `HiZ>` |
| `#` (v3) | hardware reset | off | all input after the reboot | `HiZ>` |
| `m1`, or `m` then `1` | cleanup + board reset | off | all I/O input | `HiZ>` |
| `m<N>` to another mode | cleanup + board reset | off | all I/O input | new mode |
| `m10` again (v4) | cleanup + board reset + setup | off | all I/O input | `DIO>` |
| `m` then `x` | nothing | unchanged | unchanged | `DIO>` |

`m` with `x` is the one cancel path: it prints `no mode change` and leaves
every pin, the supply and the mode settings exactly as they were.  An invalid
menu number only re-prompts.

Re-entering direct I/O, even from `DIO>` with `m10`, runs the cleanup and the
board reset first.  It therefore releases every I/O pin and turns the supply
off; it is not a no-op.

## `w` versus `#`

The difference is how much state is reset.  `w` releases every pin the park
covers (five on v3, seven on v4); `#` releases every I/O pin and resets the
mode settings too:

| Setting | `w` | Mode change | `#` (v4) |
|---|---|---|---|
| supply | off | off | off |
| five signal pins | input | input | input |
| `AUX1` / `AUX2` (v4) | input | input | input |
| pull-ups | off | off | off |
| on-board VPU source (v4) | off | off | off |
| output latches | low | low | low |
| width | 8 bits | 8 bits | 8 bits |
| bit order | **unchanged** | MSB-first | MSB-first |
| AUX selector | **unchanged** | `AUX` (`A0`) | `AUX` (`A0`) |
| AUX output routing / `AUX1` open-drain | **unchanged** | cleared | cleared |
| display format | **unchanged** | **unchanged** | HEX |
| prompt | `DIO>` | new mode | `HiZ>` |
| user macros | kept | kept | kept |

`w` resets the parked pins, the pull-ups, the VPU source, the latches, the
supply and the width.  It leaves the mode settings alone: the bit order, the
AUX selector, the AUX output routing, the `AUX1` open-drain setting and the
display format.  A mode change resets the parked pins, the pull-ups, the VPU
source, the latches, the width and those mode settings; it keeps the display
format and the macros.  `#` also restores HEX and returns to `HiZ>`.

On v3 `#` prints `RESET` and performs a hardware reset; the startup banner
appears as the board restarts in `HiZ`, and the restart clears the macro
slots.  The shared reset description is in
[usage.md](../../generic/usage.md#reset-and-the-idle-state).

## What `W` does not do

`W` switches the rails on and prints `POWER SUPPLIES ON` and
`Clutch engaged!!!`.  It runs the mode's setup hook, which is empty in direct
I/O, so it does **not** restore pin directions or levels.  After `w`, the
parked pins are inputs and the latches are low; `W` leaves them that way.
Send a state value and a direction value to drive them again:

```
DIO>w
Clutch disengaged!!!
POWER SUPPLIES OFF
DIO>W
POWER SUPPLIES ON
Clutch engaged!!!
DIO>0xC4
WRITE: 0xC4
DIO>0x1B
WRITE: 0x1B
DIO>
```

The state value is written while the pins are still inputs (it stores the
latches), then the direction value makes `CLK` an output.  That order avoids
driving a stale level; see
[pin-control.md](pin-control.md#direction-and-level-are-separate).
`v` immediately after `W` shows the signal pins as `I` (and `AUX1` / `AUX2` as
`I` on v4) and the rails at the supply voltage; after the two writes `CLK` is
`O` and `H`.  The `H` / `L` entries on an input are floating.

## The v4 extra pins

`w` includes `AUX1` and `AUX2` in the park on v4.  Both are set to input and
their latches are cleared, regardless of which AUX target is selected, so a
pin driven with `A` after `k` / `K` is released by `w` as well.

| To release | Do |
|---|---|
| a driven `AUX` or `CS` | `w`, `@` on that target, or `#` |
| a driven `AUX1` / `AUX2` | `w`, `@` with that target selected, or `#` / a mode change |

Use `#` (or leave the mode) when you also need the selector, bit order or
display reset; `w` already releases the pin.

## Verifying the parked state

`v` is the check, but it proves different things after the two commands:

- After `#` (or a mode change): every I/O pin must show `I` in the direction
  row, including `AUX1` / `AUX2`.
- After `w`: the five signal pins (v3) or all seven pins (v4) show `I`; on v4
  that includes both `AUX1` and `AUX2`.
- The `5.0V` and `3.3V` values must read `0.00V` after either command; `VPU`
  reads `0.00V` unless the target feeds it.

The `H` / `L` row is not a park indicator: an input reads the level on the
pin, which floats when nothing is connected.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| `w` parks the five signal pins | yes | yes |
| `w` also parks `AUX1` / `AUX2` | — | yes (both, regardless of the selected target) |
| `w` leaves bit order / AUX selector / display | yes | yes |
| `#` | hardware reset | terminal reset to `HiZ` |
| `#` resets bit order, AUX selector, display | yes | yes |
| `#` keeps user macros | no (reset) | yes |
| `AUX1` / `AUX2` exist | — | yes |
