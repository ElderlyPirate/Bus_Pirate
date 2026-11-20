# DIO — pin control (values)

Part of the DIO documentation set (`docs/protocol/dio/`).  This page owns the
numeric write values: the direction byte, the state byte, the bit maps and the
value traps.  The per-pin `a` / `A` / `@` path is in
[aux-pin.md](aux-pin.md); read-back is in [read-back.md](read-back.md).

## A value is a write

The terminal's normal value entry is the direct I/O write.  Type a decimal,
`0x…` hex or `0b…` binary value and press Enter.  The shared value grammar
is in [values.md](../../generic/values.md); this page uses the 8-bit default.

The firmware looks at **bit 7 of the low byte** and chooses one of two
actions:

| Bit 7 | Value range | Action |
|---|---|---|
| 0 | `0x00`–`0x7F` (decimal 0–127) | set the five pin **directions** |
| 1 | `0x80`–`0xFF` (decimal 128–255) | set the five pin **levels**, the supply and the pull-ups |

Only the low byte reaches the pin map.  Higher bits are ignored, and a value
above `0xFF` behaves as its low byte: `0x100` is `0x00`, `0x180` is `0x80`.
The reply echoes the value at the current display width:

```
DIO>0x1F
WRITE: 0x1F
DIO>
```

The echo is not the resulting pin state.  With the default 8-bit width and
MSB-first order it is the applied byte; a wider `;N` can show bits above the
low byte that the pins ignore, and a narrower `;N` can hide bits they still
use.  `L` (LSB-first) reverses the value after the echo.  Read the pins with
`r` or `v` ([read-back.md](read-back.md)).

## Direction values (`0x00`–`0x7F`)

Bits 4–0 select each pin's direction; 1 makes the pin an input (high-Z), 0 an
output.  A direction value sets all five directions, not one.  Bits 6–5 are
ignored, even though they are the supply and pull-up bits of a **state**
value: `0x40` or `0x20` is a direction write, not "supply on" or "pull-ups
on".  Both make all five pins outputs and leave the supply and pull-ups at
their previous settings.

| Bit | Value | Pin | 1 | 0 |
|---|---|---|---|---|
| 4 | `0x10` | `AUX` | input | output |
| 3 | `0x08` | `MOSI` | input | output |
| 2 | `0x04` | `CLK` | input | output |
| 1 | `0x02` | `MISO` | input | output |
| 0 | `0x01` | `CS` | input | output |

`0x00` makes all five outputs; `0x1F` makes all five inputs.  Only use
`0x00` when all five nets are safe to drive.  A bare `1` is the direction byte
`0x01` (CS input, the other four outputs), not "pin 1 high".  To change one
direction, send the complete set you want.

## State values (`0x80`–`0xFF`)

Bit 7 selects the state action.  The other bits set the supply, the pull-ups
and every pin level:

| Bit | Value | Controls | 1 | 0 |
|---|---|---|---|---|
| 7 | `0x80` | selects a state write | — | — |
| 6 | `0x40` | on-board supply | on | off |
| 5 | `0x20` | pull-up resistors | on | off |
| 4 | `0x10` | `AUX` | high | low |
| 3 | `0x08` | `MOSI` | high | low |
| 2 | `0x04` | `CLK` | high | low |
| 1 | `0x02` | `MISO` | high | low |
| 0 | `0x01` | `CS` | high | low |

A state value is **absolute, not a mask**.  It sets the supply and the
pull-ups from bits 6–5 and sets all five output latches from bits 4–0.  A
pin whose bit is 0 goes low even if it was high; a state value does not keep
the other pins or the previous supply/pull-up state.  Bits 6 and 5 are the
binary bitbang layer's `POWER` and `PULLUP` bits in the same byte layout.

Examples:

| Value | Effect |
|---|---|
| `0x80` | supply off, pull-ups off, all five latches low |
| `0x84` | supply off, pull-ups off, `CLK` high, the rest low |
| `0xC0` | supply on, pull-ups off, all five low |
| `0xC4` | supply on, pull-ups off, `CLK` high |
| `0xE0` | supply on, pull-ups on, all five low |
| `0xE4` | supply on, pull-ups on, `CLK` high |
| `0xFF` | supply on, pull-ups on, all five high |

To keep the supply on while changing a pin, include bit 6: `0xC0 | pins`
leaves the pull-ups off, `0xE0 | pins` also enables them.  To keep another
pin high, include its bit as well: `0xCD` is supply on, `CS`, `CLK` and `MOSI`
high, pull-ups off.

A state value with bit 6 set switches the supply on **silently**: there is no
`POWER SUPPLIES ON` line and no rail check.  Check the rails with `v`.

## Worked session

On v4, with nothing connected, engage the supply, load `CLK` high while the
pins are still inputs, then make only `CLK` an output:

```
HiZ>m10
Ready
DIO>W
POWER SUPPLIES ON
Clutch engaged!!!
DIO>0xC4
WRITE: 0xC4
DIO>0x1B
WRITE: 0x1B
DIO>
```

`0xC4` is `0x80` (state write) + `0x40` (supply on) + `0x04` (`CLK` high); it
stores the levels without driving them.  `0x1B` is a direction value: bits 4,
3, 1 and 0 set (`AUX`, `MOSI`, `MISO` and `CS` inputs) and bit 2 clear (`CLK`
output).  Only `CLK` is driven; the other four stay inputs.

Run `v` to confirm: `CLK` is `O` and `H`; the other four are `I` and their
`H` / `L` entries are floating ([read-back.md](read-back.md)).
`r` need not read `0x04` in this state, because the four input bits float.
`0xE4` does the same with the pull-ups on; they need a voltage on `VPU`
([wiring.md](wiring.md)).

## Direction and level are separate

A direction change does not set a level: making a pin an output drives
whatever is in its output latch.  A state write stores the latch even while
the pin is an input; the level appears only after the direction bit changes.

For a new output, send the **state value first** (while the pin is still an
input) and the **direction value second**.  That order avoids driving the old
latch level for a moment when the pin turns on.  The reverse order also works
when you immediately follow it with the state value, but the pin can glitch to
the stored level first.  After `w` the latches are low, so a direction write
alone drives the pin low until a state write sets it high.

Within one value the firmware updates the five pins in the order
`AUX` → `MOSI` → `CLK` → `MISO` → `CS`, then waits a short settle time
(about 5 µs).  The pins in one value do not change at the same instant, and
the settle time is not a timing guarantee.

## Other value forms

| Input | Effect in direct I/O |
|---|---|
| `1`, `16`, `31` | decimal direction bytes (`0x01`, `0x10`, `0x1F`) |
| `0x18`, `0b00011000` | the same value in other bases |
| `0x1F,0xC4` | two writes; a comma or space separates commands |
| `0xC4:3` | three identical state writes; `WRITE: 0xC4 0xC4 0xC4` |
| `"AB"` | one string write: sends `0x41`, then `0x42`; echoes `WRITE: "AB"` |
| `0x123` | low byte `0x23`; the echo is `WRITE: 0x23` at 8 bits |

`;N` sets the display and bit-order window, not the pin count.  A width below
8 can hide the action bit in the echo: `0x84;4` prints `WRITE: 0x04;4` but is
still a state write.  Set the width back to 8 before using direct I/O values.
The repeat count and width grammar are in
[values.md](../../generic/values.md).

## Value reference

| Input | Action | Notes |
|---|---|---|
| `0x00`–`0x7F` | set pin directions | bits 4–0: 1 = input, 0 = output; bits 6–5 ignored |
| `0x80`–`0xFF` | set pin states | bit 6 supply, bit 5 pull-ups, bits 4–0 levels; absolute |
| `r` | read the five levels | bits 4–0, 1 = high; see [read-back.md](read-back.md) |
| `v` | directions, levels and rails | shared base command |
| `W` / `w` | supply on / park and off | `w` parks the five signal pins (v3) or seven pins including `AUX1` / `AUX2` (v4); see [park-and-reset.md](park-and-reset.md) |
| `P` / `p` | pull-ups on / off | optional; needs `VPU` |
| `a` / `A` / `@` | selected AUX target | separate path; see [aux-pin.md](aux-pin.md) |
| `c` / `C` (`k` / `K`) | select the AUX target | v4 adds `AUX1` / `AUX2` |
| `(n)` | no mode macros | reports an error |
| `<n>` | user macros | shared; see [macros.md](macros.md) |

The binary bitbang command codes (`0x01` SPI, `0x0F` terminal, …) do not apply
here.  In direct I/O every value is a direction or state write; `0x01` simply
makes `CS` an input.  The state-byte layout is the same one the binary
bitbang layer uses; the direction value uses the same five bit positions but
not the binary command framing ([binary/commands.md](../binary/commands.md)).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| direction / state bit maps | same | same |
| `AUX` bit 4 drives | `AUX` (`A0`) pin | `AUX` (`A0`) pin |
| `AUX1` / `AUX2` in a value | — | not addressable |
| `v` columns | 10 | 12 |
