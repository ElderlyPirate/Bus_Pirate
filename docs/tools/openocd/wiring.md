# Wiring — pins, rails, levels and pull-ups

Part of the OpenOCD adapter set.  This page is the wiring home for the
`0x06` path: which header pin is which JTAG signal, what the target supply
connects to, which pull-ups are required, and how the two reset lines are
driven.  The shared JTAG pin map and the full level discussion are in
[protocol/jtag/wiring.md](../../protocol/jtag/wiring.md); entry and the
settings are in [commands.md](commands.md).

## Signal map

| Bus Pirate | Direction | JTAG | Role |
|---|---|---|---|
| MOSI | → | TDI | test data in |
| MISO | ← | TDO | test data out |
| CLK | → | TCK | test clock |
| CS | → | TMS | test mode select |
| AUX | → | SRST | system reset (optional) |
| AUX1 (v4) / PGD (v3) | → | TRST | test reset (optional) |
| GND | — | GND | signal ground |

TDI, TMS and TCK are driven by the Bus Pirate; TDO is an input driven by
the target.  SRST and TRST are driven only after a pin mode makes them
outputs.

### Pin names

The printed header names are the authority:

| JTAG signal | Printed name | AUX target |
|---|---|---|
| SRST | `AUX` | `A0` (`c`; the v3 help prints `aux`) |
| TRST (v4) | `AUX1` | `A1` (`k`) |
| TRST (v3) | `PGD` | — |

The v4 `AUX2` pin (`A2`, `K`) is not used by this path.  On v3, `PGD` is
the pin on the programming header, not a main-header pin.

## Power and the two rails

The on-board regulator (**VREG**) drives two fixed target-supply rails:

| Header pin | Direction | Supply |
|---|---|---|
| `3.3V` | out while VREG is on | 3.3 V |
| `5.0V` | out while VREG is on | 5.0 V |
| GND | — | common ground with the target |

**The target's VCC goes to one rail only, never both.**  Pick the rail that
matches the target and leave the other unconnected.

| Target supply | `buspirate vreg` | Wiring |
|---|---|---|
| the board powers the target | `1` | target VCC to `3.3V` or `5.0V`; the other rail unconnected |
| the target has its own supply | `0` | no rail connection; the target keeps its own supply |
| 1.8 V or 2.5 V target | `0` | no rail connection; use open-drain with pull-ups to the target rail |

- VREG is a single on/off switch.  The path **cannot select a rail voltage**;
  `0x02 0x02 0x01` brings up both rails at their fixed levels.
- Never back-drive a powered target: keep the target's own supply off any
  rail.  The safest wiring for a self-powered target is no rail connection at
  all.
- `buspirate vreg 1` is the host option that turns VREG on at init.
- The XSVF player switches VREG on at entry; this path switches it off at
  entry and the host must enable it with the VREG feature.

## Pull-ups

| Mode | Pull-up requirement |
|---|---|
| push-pull (`buspirate mode normal`) | **none** |
| open-drain (`buspirate mode open-drain`) | **2–10 kΩ** on TDI/TMS/TCK to the target rail |
| open-drain, SRST/TRST wired | the same **2–10 kΩ** on both reset lines |
| TDO | **none** on the Bus Pirate — the target drives it |

- In open-drain mode the Bus Pirate only pulls each line **low**; the
  pull-up sets the high level.  Tying the pull-ups to the target rail is
  what makes a 1.8 V or 2.5 V chain work.
- Without pull-ups, an open-drain line never goes high and the chain reads
  as stuck low.
- **The driver's default is open-drain.**  A configuration that only sets
  the port needs the pull-ups; set `buspirate mode normal` for a 3.3 V
  target that the board drives push-pull.
- The on-board pull-up resistors (feature `0x10`, `P` at the terminal) are a
  separate optional feature.  They tie the lines to **Vpullup**, not to a
  per-line rail, and a 0–5 V source must be present on that pin.  Binary
  entry switches the resistors off; enable them with feature `0x10`.  Feed
  Vpullup externally: the v4 on-board routing (`e`) is a terminal command
  that is not reachable from the binary path, and binary entry switches that
  source off.  Vpullup is marked `Vpu` on the reference card and printed
  `VPU` in the terminal status table.

## Levels

| Item | Value |
|---|---|
| logic high, push-pull | 3.3 V |
| interface tolerance | 5 V safe |
| maximum applied voltage | 5.5 V |
| 1.8 V / 2.5 V target | open-drain with pull-ups to its own rail only |
| 5 V target | check the input threshold; open-drain with 5 V pull-ups gives a 5 V high |

A 3.3 V push-pull high may not meet a 5 V target's input threshold.  An
open-drain mode with pull-ups to the target's 5 V rail does produce a 5 V
high, because the Bus Pirate only pulls low.

Do not drive a 3.3 V push-pull output into a 1.8 V or 2.5 V chain.  Use
open-drain and let the target rail set the high level.

## Reset lines

SRST is the `AUX` pin.  TRST is `AUX1` on v4 and `PGD` on v3.  Both are
optional; most targets reset the TAP from TMS alone and do not need TRST.

- Selecting either JTAG pin mode drives SRST and TRST **low** before it
  applies the direction.  With active-low reset inputs, that asserts both
  resets.
- Release them with the feature writes `0x02 0x04 0x01` (TRST) and
  `0x02 0x08 0x01` (SRST).  The action is the pin level: `0` low, `1` high.
- A later pin mode re-parks both lines low, so set the pin mode **before**
  releasing the resets.
- The 5.0 V / 3.3 V rails and the reset lines are independent.  A target
  held in reset still reads as all zeroes until the line is released.
- OpenOCD treats both lines as active-low: a reset request drives them low.
  The firmware does not invert them, so a target with active-high reset
  inputs needs external inversion.
- `reset_config` must match the wiring.  The interface file sets
  `reset_config srst_only`, which tells OpenOCD that only SRST is
  connected.  Use `reset_config trst_and_srst` when both are wired.

## The shift at the pins

Each bit is one TCK pulse:

| Step | Level |
|---|---|
| 1 | TCK low |
| 2 | TDI and TMS driven to the bit values |
| 3 | TCK high |
| 4 | TDO sampled |

TCK is left high after a shift and between shifts.  The host supplies the
TMS sequence for every TAP transition; TMS is the only TAP control beyond
TCK.

## v3 and v4

| Item | v3 | v4 |
|---|---|---|
| TDI / TDO / TCK / TMS | MOSI / MISO / CLK / CS | MOSI / MISO / CLK / CS |
| SRST | `AUX` | `AUX` |
| TRST | `PGD` | `AUX1` |
| pin modes `0x01` / `0x02` | present, not verified on v3 | present |
| Vpullup on-board source | none | off in the binary path |
| fast link | about 1 Mbaud UART | CDC rate unchanged |

The v3 column is **not verified on v3**.

## Related

| Doc | Why |
|---|---|
| [../../protocol/jtag/wiring.md](../../protocol/jtag/wiring.md) | the shared pin map, levels and supply detail |
| [commands.md](commands.md) | pin modes and features in byte form |
| [host-setup.md](host-setup.md) | `buspirate mode`, `vreg`, `pullup` options |
| [workflow.md](workflow.md) | the drive-mode decision table |
| [limits-and-errors.md](limits-and-errors.md) | electrical failure symptoms |
