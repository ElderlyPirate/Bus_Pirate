# 1-Wire — wiring and electrical requirements

Part of the [1-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  This page owns the full wiring table, the
pull-up statement, the power options and the strong-pull-up limit; the README
carries the headline only.

## Wiring

| Bus Pirate pin | Direction | 1-Wire signal | Notes |
|---|---|---|---|
| MOSI | ↔ | data (shown as `OWD` in the pin table) | open drain: the Bus Pirate pulls it low, the pull-up makes the high |
| GND | ⏚ | ground | common ground with the target |
| VPU (`Vpu`) | in | pull-up voltage reference | the voltage the pull-up ties the data line to; **not a wire to the data line** — see below |

The 1-Wire data pin is the **MOSI** line; the mode's own pin table (`v`) labels
it `OWD` and leaves the other pins unnamed.  The label row differs by build
because the `v` table's last four columns are ordered differently:

| Build | Mode label row | `OWD` lands on |
|---|---|---|
| v4 | `-  -  -  OWD` | MOSI |
| v3 | `-  OWD  -  -` | MOSI |

VPU is not wired to the data line.  It is the reference voltage the Bus
Pirate's pull-up ties the data line to, fed by `e2`/`e3` on v4 and by an
external supply on v3.  If you fit your own pull-up from the data line to a
supply, you do not need `P`/VPU at all.

## The pull-up is required — stated explicitly

**A pull-up resistor on the data line is mandatory.**  1-Wire is an open-drain
bus: the Bus Pirate and every device can only pull the line low; the high level
comes from the pull-up.  With no pull-up the reset fails — usually with
`*Short or no pull-up` if the line stays low, or `*No device detected` if it
floats high (see [bus-operations.md](bus-operations.md)).

Value guidance:

| Situation | Pull-up |
|---|---|
| General devices | **2 kΩ – 10 kΩ**, from the data line to the pull-up voltage |
| Parasitically powered parts (bus-powered, no separate VCC) | **2 kΩ or smaller** |

Parasitically powered parts draw their operating current through the data line
while it is high, so the pull-up must be strong enough to supply them.  They
may appear to work with a larger resistor but fail operations that need more
current (for example EEPROM writes).

The Bus Pirate's own on-board pull-up resistors can serve as the pull-up when a
voltage is present on VPU (`P`).  The v3a and later boards are documented with
an on-board 2 kΩ pull-up on the data line; older boards (for example v2go) need
an external 2 kΩ resistor.  **Treat the 2 kΩ figure as unverified for v4.**

## Power and strong pull-up

There are two ways to power a 1-Wire part:

| Part type | Power |
|---|---|
| has a VCC pin | wire VCC to the Bus Pirate's 3.3 V or 5.0 V header rail (enabled by `W`) or to an external supply; share ground |
| parasitic (no VCC pin) | draws from the data line through the pull-up; the pull-up must be 2 kΩ or less |

Some parts need a **strong pull-up** — a low-value resistor or a transistor to
VCC — during a conversion or an EEPROM write.  This mode has **no
strong-pull-up command** and never drives the data line high: it only pulls
low or releases the line.  To provide one, add an external transistor/MOSFET
pull-up switched by AUX (`A`/`a`) or by another GPIO, or power the part from a
VCC pin.  The on-board pull-up remains the normal idle-high source.

## Feeding the pull-up voltage (VPU)

This mode ties the **data line** to VPU through the on-board pull-up.  On v4
select the source with `e2` (on-board 3.3 V) or `e3` (on-board 5 V); on v3
feed Vpullup from an external 0–5 V supply.  If nothing is on VPU, `P` still
enables the pull-ups but prints `Warning: no voltage on Vpullup pin`.

A common order is `W`, then `e2`/`e3`, then `P`.  `P` can be enabled before
`W`; it needs a non-HiZ mode and a voltage on VPU to produce an idle high.  The
`e` source selection needs a non-HiZ open-drain mode and no voltage already on
VPU.  The shared `P`/`e` sequence, the v4 source choices and the ordering rule
are on the cluster page:
[wire protocols](../README.md#power-pull-ups-and-vpu).

## Bus engagement

Until `W` is sent the bus stays parked (`Clutch disengaged!!!`): the mode's
I/O is not driven.  At `W` the data line is set to an **input** (released) with
its latch low, so it idles at the pull-up level rather than being driven high.
`W` prints `POWER SUPPLIES ON` then `Clutch engaged!!!`; `w` parks the bus and
prints `Clutch disengaged!!!` then `POWER SUPPLIES OFF`.  This engagement
sequence is base-layer behaviour — see
[docs/generic/commands.md](../../../generic/commands.md).

## Voltage limit

The 1-Wire interface is 5 V safe; do not exceed 5.5 V on the data line.  The
Bus Pirate's own pull-up source is at most the on-board 5.0 V rail.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| data pin | MOSI | MOSI |
| pull-up required | yes | yes |
| VPU source selection | none — feed externally | `e1` / `e2` / `e3` |
| on-board pull-up | documented 2 kΩ (v3a+) | unverified |
| `v` mode label row | `-  OWD  -  -` | `-  -  -  OWD` |
