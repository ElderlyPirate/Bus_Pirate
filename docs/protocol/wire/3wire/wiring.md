# 3-Wire — wiring and electrical requirements

Part of the [3-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  This page owns the full wiring table, the
output-type behaviour, the pull-up statement and the pin-table mapping; the
README carries the headline only.

## Wiring

The mode is a four-signal bit bus plus ground:

| Bus Pirate pin | Direction | 3-Wire signal | Notes |
|---|---|---|---|
| MOSI | → | data out | clocked out by writes and by `-` / `_` |
| MISO | ← | data in | sampled by reads and by `.` / `!` |
| CLK | → | clock | driven by `/`, `\`, `^` and by every value transfer |
| CS | → | chip select | driven by `[` / `{` / `]` / `}` |
| GND | ⏚ | ground | common ground with the target |
| VPU (`Vpu`) | in | pull-up voltage reference | the voltage the pull-ups tie the lines to for open drain; not a wire to the bus |

The mode splits data into a dedicated output line and a dedicated input line —
unlike 2WIRE, where one bidirectional line carries both.  The mode's own pin
table (`v`) prints the four signal names in build-specific order:

| Build | Mode label row |
|---|---|
| v4 | `CS  MISO  CLK  MOSI` |
| v3 | `CLK  MOSI  CS  MISO` |

The names are the mode's own; the mapping to the Bus Pirate labels is the one
in the table above.

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

**Open-drain 3-Wire needs a pull-up on the open-drain lines.**  In open drain
the Bus Pirate can only pull a line low; the high level comes solely from the
pull-up resistor.  Value guidance: **2 kΩ – 10 kΩ** to the pull-up voltage.

The Bus Pirate's on-board pull-ups can serve when a voltage is present on
VPU (`P`).  They are switched as one group — there is no per-line control.
The v3a and later boards are documented with an on-board 2 kΩ pull-up; older
boards need an external resistor.  **The on-board value is unverified for
v4.**

In open drain, MISO (an input) needs the pull-up to have a defined idle high;
the driven lines (CS, CLK, MOSI) also need it for their high levels, because a
high is a release.  Unlike I2C, 3-Wire's CS control does **not** check the
released lines and prints no short/pull-up warning.  With no pull-up a floating
line reads as a level and a read returns meaningless data without any error.
Use `.` to check the data-in pin (MISO), or `v` for the pin states.  See
[edge-cases.md](edge-cases.md).

## Feeding the pull-up voltage (VPU)

This mode ties all four signal lines to VPU through the on-board pull-ups.  On
v4 select the source with `e2` (3.3 V) or `e3` (5 V); on v3 feed Vpullup from
an external 0–5 V supply.  If nothing is on VPU, `P` still enables the
pull-ups but prints `Warning: no voltage on Vpullup pin`, and the open-drain
bus has no idle-high.

The shared `P`/`e` sequence, the v4 source choices and the ordering rule are
on the cluster page: [wire protocols](../README.md#power-pull-ups-and-vpu).

## Target power

The target's VCC is separate from VPU.  `W` puts the on-board 3.3 V and 5.0 V
rails on their header pins; wire the target's VCC to the rail it needs, or
power it externally and share ground.  VPU only sets the reference the
pull-ups tie to.

## Voltage limit

The 3-Wire interface is 5 V safe; do not exceed **5.5 V** on any signal line.
The Bus Pirate's own pull-up source is at most the on-board 5.0 V rail.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| data out / data in pin | MOSI / MISO | MOSI / MISO |
| output-type choice | open drain or normal | open drain or normal |
| pull-up required (open drain) | yes | yes |
| on-board pull-up | documented 2 kΩ (v3a+) | unverified |
| VPU source selection | none — feed externally | `e1` / `e2` / `e3` |
| `v` label order | `CLK MOSI CS MISO` | `CS MISO CLK MOSI` |
