# 1-Wire — wiring and electrical requirements

Part of the 1-Wire documentation set (`docs/1wire/`).  This page owns the
full wiring table and the pull-up statement; the README carries the headline
only.

## Wiring

| Bus Pirate pin | Direction | 1-Wire signal | Notes |
|---|---|---|---|
| MOSI | ↔ | data (shown as `OWD` in the pin table) | open-drain: the Bus Pirate pulls it low, the pull-up makes the high |
| GND | ⏚ | ground | common ground with the target |
| VPU (`Vpu`) | in | pull-up voltage reference | the voltage the pull-up ties the data line to; **not a wire to the data line** — see below |

The 1-Wire data pin is the **MOSI** line; the mode's own pin table (`v`) labels
it `OWD` and leaves the other pins unnamed.

VPU is not wired to the data line.  It is the reference voltage the Bus
Pirate's pull-up ties the data line to, fed by `e2`/`e3` on v4 and by an
external supply on v3.  If you fit your own pull-up from the data line to a
supply, you do not need `P`/VPU at all.

The v3 and v4 pin tables differ only in where `OWD` sits in the row (the
column order follows the board's pin order); the electrical function is the
same.

## The pull-up is required — stated explicitly

**A pull-up resistor on the data line is mandatory.**  1-Wire is an
open-drain bus: the Bus Pirate and every device can only pull the line low;
the high level comes from the pull-up.  With no pull-up there is no idle-high
and no device presence pulse, and every reset reports
`BUS RESET Warning: *No device detected` (see
[bus-operations.md](bus-operations.md)).

Value guidance:

| Situation | Pull-up |
|---|---|
| General devices | **2 kΩ – 10 kΩ**, from the data line to the pull-up voltage |
| Parasitically powered parts (bus-powered, no separate VCC) | **2 kΩ or smaller** |

Parasitically powered parts draw their operating current through the data
line while it is high, so the pull-up must be strong enough to supply them.
They may appear to work with a larger resistor but fail some operations (for
example EEPROM writes) that need more current.

The Bus Pirate's own on-board pull-up resistors can serve as the pull-up when
a voltage is present on VPU.  The v3a and later boards are documented with an
on-board 2 kΩ pull-up on the data line; older boards (for example v2go) need
an external 2 kΩ resistor.  **The on-board value has not been re-measured on
this v4 bench — treat the 2 kΩ figure as unverified for v4.**

## Feeding the pull-up voltage (VPU)

`P` connects the Bus Pirate's pull-up resistors between the I/O lines and the
physical **Vpullup** pin.  That pin must already be at the pull-up voltage:

- **v4** — the on-board source is selected with `e`:
  - `e1` External (or None),
  - `e2` on-board **3.3 V**,
  - `e3` on-board **5.0 V**.
  The bench battery uses `e2` (3.3 V on-board pull-up).  `e` is only accepted
  while the pins are open-drain HiZ and no voltage is already on VPU.
- **v3** — there is no `e`; feed the Vpullup pin from an external 0–5 V
  supply.  (Derived from source — **not verified on this bench**.)

If nothing is on VPU, `P` still enables the pull-ups but prints
`Warning: no voltage on Vpullup pin`, and the bus has no idle-high.  The
shared `P`/`e` commands and the VPU routing are documented in
`docs/generic/` — see [commands.md](../generic/commands.md).

## Bus engagement

Until `W` is sent, the bus stays parked (`Clutch disengaged!!!`): the mode's
I/O is not driven.  `W` starts the supplies and prints `Clutch engaged!!!`;
`w` stops them and re-parks the bus.  This is base-layer behaviour — see
`docs/generic/`.

## Voltage limit

The 1-Wire interface is 5 V safe; do not exceed 5.5 V on the data line.  The
Bus Pirate's own pull-up source is at most the on-board 5.0 V rail.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| data pin | MOSI | MOSI |
| pull-up required | yes | yes |
| VPU source selection | none — feed externally | `e1` / `e2` / `e3` |
| on-board pull-up | documented 2 kΩ (v3a+) | not re-measured here |

v3 wiring and the v3 VPU route are read from the firmware/hardware
documentation and are **not verified on this bench** (no v3 hardware).