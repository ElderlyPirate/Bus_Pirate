# DIO — wiring

Part of the DIO documentation set (`docs/protocol/dio/`).  This page owns the
pin header, the pins DIO controls, and the electrical limits; the README
carries the headline only.

## Wiring

DIO has no bus and needs no external component for basic pin control.  Each
of the five controlled pins is an independent general-purpose I/O — connect
each one to whatever you are driving or reading.  Ground is always common
with the target.

| Bus Pirate pin | DIO controls | Value bit |
|---|---|---|
| `CS` | direction + level | bit 0 |
| `MISO` | direction + level | bit 1 |
| `CLK` | direction + level | bit 2 |
| `MOSI` | direction + level | bit 3 |
| `AUX` | direction + level | bit 4 |
| `GND` | — | common ground with the target |
| `+3.3V` / `+5.0V` | supply rails | the on-board supply enabled by `W` |
| `Vpu` / `VPU` | pull-up reference | used only if the pull-ups are enabled |

`MISO` is normally a mode *input*; DIO can drive it too, like any other pin.

**Pull-up: not required.**  DIO is direct pin control, not an open-drain bus.
The on-board pull-up resistors are optional and are enabled either with
`P`/`p` or with bit 5 of a state value.

### Pin naming

The firmware's own names for the five pins, as shown by `v`, are `AUX`,
`MOSI`, `CLK`, `MISO`, `CS`.  `AUX` is the pin the AUX-assignment commands
call target `A0`; on v4 the header also carries `AUX1` and `AUX2`, which a DIO
value **cannot** address.  The mapping used throughout this set: a direction
or state bit drives the pin named in the table above, and `AUX` means the
`AUX0` pin.

The pull-up reference pin is labelled `Vpu` / `VPU` on the board and header;
the firmware calls it **Vpullup** (the name it uses in `Warning: no voltage on
Vpullup pin`).  This set writes the pin `VPU` and the firmware name `Vpullup`
only in quoted output.

### Header per board

| v3 label | v4 label | Role |
|---|---|---|
| `GND` | `GND` | ground |
| `+3.3V` | `+5.0V` | rail |
| `+5.0V` | `+3.3V` | rail |
| `ADC` | `ADC` | ADC / probe pin (not a DIO pin) |
| `VPU` | `VPU` | pull-up reference |
| `AUX` | `AUX` | DIO bit 4 |
| `CS` | `CS` | DIO bit 0 |
| `MISO` | `MISO` | DIO bit 1 |
| `CLK` | `CLK` | DIO bit 2 |
| `MOSI` | `MOSI` | DIO bit 3 |
| — | `AUX1` | not a DIO pin |
| — | `AUX2` | not a DIO pin |

The v3 label row is ordered `GND 3.3V 5.0V ADC VPU AUX CS MISO CLK MOSI`; the
v4 label row is `GND 5.0V 3.3V VPU ADC AUX2 AUX1 AUX CS MISO CLK MOSI`.

## Voltage limits

The I/O pins are **5 V class**: the on-board supply tops out at 5.0 V and the
Vpullup pin is specified as 0–5 V.  Keep applied signals within 0–5 V and
ground common with the target.  The exact absolute-maximum rating of the pins
is not documented here (unverified).

## Safety

- Do not short the rails (`+3.3V`, `+5.0V`) to ground or to each other; `W`
  reports `VREG too low, is there a short?` if the rails do not come up.
- Never drive a pin against an external source.  Set a pin to input (its
  direction bit = 1) before attaching a driver to the same net — see
  [edge-cases.md](edge-cases.md).

## Power and pull-ups

DIO drives its five pins **push-pull only**; it never selects an open-drain
output type.  As a result the on-board VPU source cannot be selected from DIO:
`e` (v4) is refused with `WARNING: pins not open drain (HiZ)`, and entering
DIO also clears any source selected earlier.  Feed the **VPU** pin from an
external supply on both boards when the pull-ups are used.

| Step | Command | Effect |
|---|---|---|
| supply on | `W` | on-board rails on, `POWER SUPPLIES ON` + `Clutch engaged!!!` |
| supply off | `w` | rails off, `Clutch disengaged!!!` + `POWER SUPPLIES OFF`; pin directions, levels and pull-ups stay as set |
| pull-ups on | `P` | ties the I/O lines to VPU; prints `WARNING: pins not open drain (HiZ)` first, then `Pull-up resistors ON` |
| pull-ups off | `p` | releases the pull-ups |

`P` also prints `Warning: no voltage on Vpullup pin` when its single VPU
sample is low.  A state value with bit 5 set enables the same resistors
without that warning (and without the open-drain warning).  The pull-ups are
optional for DIO; without a VPU feed they have no high reference.  See
[commands.md](../../generic/commands.md) §Feeding the VPU pin.

The same two functions are reachable from a state value: bit 6 = power,
bit 5 = pull-up (see [pin-control.md](pin-control.md)).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| DIO pins | `AUX` `MOSI` `CLK` `MISO` `CS` | `AUX` `MOSI` `CLK` `MISO` `CS` |
| extra header pins | — | `AUX1`, `AUX2` (not DIO pins) |
| supply | on-board VREG via `W` | on-board VREG via `W` |
| output type | push-pull only | push-pull only |
| `e` command | not present | refused in DIO |
| `VPU` source | external only | external only |
| pull-up required | no | no |

The v3 column is **not verified**.
