# 3WIRE — wiring and electrical requirements

Part of the [3-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  This page owns the full wiring table, the
output-type behaviour, and the pull-up statement; the README carries the
headline only.

## Wiring

The mode is a four-signal bit bus plus ground:

| Bus Pirate pin | Direction | 3WIRE signal | Notes |
|---|---|---|---|
| MOSI | → | data out | clocked out by writes and by `-` / `_` |
| MISO | ← | data in | sampled by reads and by `.` / `!` |
| CLK | → | clock | driven by `/`, `\`, `^` and by every value transfer |
| CS | → | chip select | driven by `[` / `{` / `]` / `}` |
| GND | ⏚ | ground | common ground with the target |
| VPU (`Vpu`) | in | pull-up voltage reference | the voltage the pull-ups tie the lines to for open drain; not a wire to the bus |

The mode splits data into a dedicated output line and a dedicated input line —
unlike 2WIRE, where one bidirectional line carries both.  The mode's own pin
table (`v`) prints the four signal names in build-specific order (see
[§Pin table](#pin-table)); the mapping to the Bus Pirate labels above is the
one to use.

## Output types — stated explicitly

The mode asks for an output type at entry (see
[entry-and-setup.md](entry-and-setup.md)).  The two choices differ in what a
"high" means:

| Output type | High level | Low level | Pull-up |
|---|---|---|---|
| **Open drain** (`1`) | released — a pull-up makes it high | pulled to ground | **required** |
| **Normal** (`2`) | driven to 3.3 V | pulled to ground | **not needed** |

This is why the pull-up rule is **conditional**: it follows the output type, not
the mode.

## The pull-up — open drain only

**Open-drain 3WIRE needs a pull-up on the open-drain lines.**  In open drain the
Bus Pirate can only pull a line low; the high level comes solely from the
pull-up resistor.  Value guidance: **2 kΩ – 10 kΩ** to the pull-up voltage.

The Bus Pirate's own on-board pull-ups can serve when a voltage is present on
VPU (`P`).  The v3a and later boards are documented with an on-board 2 kΩ
pull-up; older boards need an external resistor.  **The on-board value is
unverified for v4.**

Unlike I2C, 3WIRE's CS control does **not** check the released lines and prints
no short/pull-up warning.  With no pull-up in open drain, a floating line reads
as a level and a read returns meaningless data without any error.  Use `.` to
check the data-in pin (MISO), or `v` for the pin states.  See
[edge-cases.md](edge-cases.md).

## Feeding the pull-up voltage (VPU)

This mode ties all four signal lines to VPU through the on-board pull-ups.  On
v4 select the source with `e2` (3.3 V) or `e3` (5.0 V); on v3 feed Vpullup
from an external 0–5 V supply.  If nothing is on VPU, `P` still enables the
pull-ups but prints `Warning: no voltage on Vpullup pin`, and the open-drain
bus has no idle-high.

The shared `P`/`e` sequence, the v4 source choices and the ordering rule are
on the cluster page: [wire protocols](../README.md#power-pull-ups-and-vpu).

## Voltage limit

The 3WIRE interface is 5 V safe; do not exceed **5.5 V** on any signal line.
The Bus Pirate's own pull-up source is at most the on-board 5.0 V rail.

## Pin table

The `v` command prints a label row for the mode's four signals.  The order
differs between builds:

| Build | Printed label row |
|---|---|
| v4 | `CS` `MISO` `CLK` `MOSI` |
| v3 | `CLK` `MOSI` `CS` `MISO` |

The names are the mode's own; the mapping to the Bus Pirate labels is the one
in [§Wiring](#wiring).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| data out / data in pin | MOSI / MISO | MOSI / MISO |
| output-type choice | open drain or normal | open drain or normal |
| pull-up required (open drain) | yes | yes |
| on-board pull-up | documented 2 kΩ (v3a+) | unverified |
| VPU source selection | none — feed externally | `e1` / `e2` / `e3` |
| `v` label order | `CLK MOSI CS MISO` | `CS MISO CLK MOSI` |

v3 wiring and the v3 VPU route are **not verified**.