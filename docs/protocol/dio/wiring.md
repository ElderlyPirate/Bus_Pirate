# DIO — wiring

Part of the DIO documentation set (`docs/protocol/dio/`).  This page owns the
header pins, the supply and pull-up requirements, and the electrical limits.
The bit values are in [pin-control.md](pin-control.md); the AUX-target command
path is in [aux-pin.md](aux-pin.md).

## Scope

Covers what to connect to a board running direct I/O, which pins a value can
address, and the optional pull-up reference.  It does not describe a target
circuit or a protocol.

## The five signal pins

Direct I/O treats five header pins as independent general-purpose I/O.  Each
pin has a direction bit and a level bit in a write value:

| Header pin | Direction | Value bit | Also an AUX target |
|---|---|---|---|
| `AUX` | input or output | 4 (`0x10`) | yes, `A0` (v4) / `aux` (v3) |
| `MOSI` | input or output | 3 (`0x08`) | no |
| `CLK` | input or output | 2 (`0x04`) | no |
| `MISO` | input or output | 1 (`0x02`) | no |
| `CS` | input or output | 0 (`0x01`) | yes, token `CS` |

`MISO` is normally a bus input and `CS` normally an output; in direct I/O both
are ordinary pins.  A value above `0x1F` does not add pins: only bits 4–0
exist, and bits 6–5 are ignored in a direction value.

The header order and the `v`-table order agree on both boards; the value-bit
order is different from both.  This table maps them:

| Header pin | Value bit | `v` column, v3 | `v` column, v4 |
|---|---|---|---|
| `AUX` | 4 (`0x10`) | 6 | 8 |
| `MOSI` | 3 (`0x08`) | 8 | 12 |
| `CLK` | 2 (`0x04`) | 7 | 11 |
| `MISO` | 1 (`0x02`) | 10 | 10 |
| `CS` | 0 (`0x01`) | 9 | 9 |

Column numbers are 1-based.

The v4 header also carries `AUX1` and `AUX2`.  They are **not** in a value;
use the AUX-target path ([aux-pin.md](aux-pin.md)).  Their `v` columns are 7
(`AUX1`) and 6 (`AUX2`).

## Header order per board

The physical order differs by board.  The `GND` end comes first:

| Board | Header order, `GND` end first |
|---|---|
| v3 | `GND` `3.3V` `5.0V` `ADC` `VPU` `AUX` `CLK` `MOSI` `CS` `MISO` |
| v4 | `GND` `5.0V` `3.3V` `VPU` `ADC` `AUX2` `AUX1` `AUX` `CS` `MISO` `CLK` `MOSI` |

The `ADC` pin is the separate ADC probe input, not one of the five.  The v3
`v` table lists the pins in a different order from the header; the mapping
table above gives both column numbers, and the shared description is in
[usage.md](../../generic/usage.md#v-in-hiz).

## Ground and target supply

**Ground is always common with the target.**  Connect `GND` even when the
target is self-powered.

`W` switches the on-board `+5.0V` and `+3.3V` rails on.  Both rails are live
on the header while the supply is on; connect only the rail the target uses.
Do not tie the two rails together and do not back-drive them.  For a
self-powered target, leave both rails unconnected.

| Step | Command | Result |
|---|---|---|
| rails on | `W` | both rails on after a check; `POWER SUPPLIES ON`, `Clutch engaged!!!` |
| rails off, five signal pins parked (v3) / seven pins parked (v4) | `w` | `Clutch disengaged!!!`, `POWER SUPPLIES OFF`; stays in `DIO>` |
| state bit 6 set | a state value | same rails, switched silently and without the check |

The rail check is the shared one: if either rail does not come up, the supply
is switched off and the reply is `VREG too low, is there a short?` followed by
`POWER SUPPLIES OFF`.  The full park behaviour is in
[park-and-reset.md](park-and-reset.md).

## Pull-up: not required

Direct I/O is not an open-drain bus, so **no pull-up is required**.  The
on-board pull-up resistors are optional; they connect the I/O lines to
whatever voltage is on the `VPU` pin.  That reference must be supplied
externally on both boards: direct I/O is push-pull only, and the v4 on-board
3.3 V/5.0 V source cannot be selected in this mode.

| Step | Input | Result |
|---|---|---|
| pull-ups on | `P` | `WARNING: pins not open drain (HiZ)`, then `Pull-up resistors ON` |
| pull-ups off | `p` | `Pull-up resistors OFF` |
| pull-ups on, silently | state bit 5 set | resistors on; no warning and no VPU sample |
| pull-ups off, silently | state bit 5 clear | resistors off |

The open-drain warning on `P` is informational: `P` still enables the
resistors in a push-pull mode.  `(HiZ)` in that string names the open-drain
high state, not the `HiZ` mode.  `P` then samples VPU once and adds
`Warning: no voltage on Vpullup pin` if the pin is low; the resistors stay on
either way.  A state value with bit 5 set does the same without the sample, so
it can enable the resistors with no high reference and no warning.

With no voltage on `VPU` (a plain example):

```
DIO>P
WARNING: pins not open drain (HiZ)
Pull-up resistors ON
Warning: no voltage on Vpullup pin
DIO>p
Pull-up resistors OFF
DIO>
```

Feed `VPU` from the target's logic supply (0–5 V) or another reference in that
range.  On v4, `e` is refused in direct I/O with `WARNING: pins not open drain
(HiZ)` plus a syntax error; on v3 there is no `e` command.  Entering the mode
also deselects any on-board source chosen in an earlier mode.

## Output type and voltage

Outputs are **push-pull only**.  There is no output-type prompt and no
open-drain setting in direct I/O.

The I/O pins are 5 V class: keep applied signals within 0–5 V and keep ground
common.  The on-board rails are 5.0 V and 3.3 V, and `Vpullup` is specified as
0–5 V.  The exact absolute-maximum rating of the pins is not documented here
and remains unverified.

## Safety

- Do not short the rails to ground or to each other.  `W` reports
  `VREG too low, is there a short?` if a rail does not come up.
- Never drive a pin against an external source.  Set the pin to input (its
  direction bit = 1) before connecting a driver to the same net.
- Direct I/O does not detect contention.  An output drives its level
  regardless of what else is on the net, and the firmware gives no warning.
- Park the five signal pins with `w` on v3, or all seven (the five signal pins
  plus `AUX1` / `AUX2`) on v4.  `#` releases every I/O pin and resets the
  selector; confirm with `v` before rewiring.  See
  [park-and-reset.md](park-and-reset.md).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| header pins | 10 | 12 |
| signal pins in a value | `AUX` `MOSI` `CLK` `MISO` `CS` | same |
| `AUX1` / `AUX2` | — | on the header, not in a value |
| supply | on-board regulator via `W` | same |
| `w` park scope | five signal pins | five signal pins plus `AUX1` / `AUX2` |
| pull-up reference | external only | external only |
| output type | push-pull only | push-pull only |
| `e` | absent | refused |
| pull-up required | no | no |
