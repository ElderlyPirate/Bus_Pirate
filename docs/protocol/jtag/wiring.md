# JTAG — wiring and levels

Part of the JTAG documentation set (`docs/jtag/`).  This page is the single
home for the physical mapping; [openocd.md](openocd.md) and [xsvf.md](xsvf.md)
refer back here instead of repeating it.

## Signal map

Both JTAG paths use the same four signal lines plus ground.  The mapping is
fixed in the firmware and applies to the OpenOCD path and the XSVF player
alike.

| Bus Pirate pin | Direction | JTAG signal | Role |
|---|---|---|---|
| MOSI | → | TDI | test data in (from the host) |
| MISO | ← | TDO | test data out (to the host) |
| CLK | → | TCK | test clock |
| CS | → | TMS | test mode select |
| AUX0 | → | SRST | system reset (optional) |
| AUX1 | → | TRST | test reset (v4) |
| PGD | → | TRST | test reset (v3) |
| GND | — | GND | signal ground |

TDI, TCK and TMS are driven by the Bus Pirate; TDO is an input driven by the
target.  SRST and TRST are driven outputs when used.

**Naming note.**  The firmware's line names differ from the terminal's pin
names.  SRST and TRST are the only auxiliary lines JTAG uses; they map as:

| JTAG signal | Firmware line | Terminal pin |
|---|---|---|
| SRST | AUX0 | `AUX` |
| TRST | AUX1 (v4) / PGD (v3) | `AUX1` (v4) / `PGD` (v3) |

The v4 terminal also prints a third reassignable target, `AUX2`; JTAG does not
use it.  SRST and TRST are optional: the OpenOCD path can toggle them as
features, and the XSVF player does not use them at all.

## Levels and voltage

- Logic level is **3.3 V** (high = 3.3 V, low = ground).
- The interface is **5 V safe**; the maximum applied voltage is **5.5 V**.
- A lower-voltage target should be driven through the OpenOCD **open-drain**
  pin mode with pull-ups to the target's own rail (see below).  Do **not**
  drive a 3.3 V push-pull output into a 1.8 V chain.

## Pull-ups

| Path / mode | Pull-up requirement |
|---|---|
| XSVF player (`0x18`) | **no pull-up needed** — the pins are driven push-pull |
| OpenOCD path, pin mode `1` (JTAG) | **no pull-up needed** — push-pull outputs |
| OpenOCD path, pin mode `2` (JTAG open-drain) | **pull-ups required** on TDI, TMS and TCK (and on SRST/TRST if used), **2 kΩ–10 kΩ** to the target's logic voltage |
| OpenOCD path, before a pin mode is set | pins are inputs (high-impedance); nothing is driven until `0x01` selects a mode |

In open-drain mode the Bus Pirate only pulls each line **low**; the pull-up
sets the high level, so it can be tied to the target rail for level shifting.
TDO is always an input and is driven by the target, so it needs no Bus Pirate
pull-up.  The on-board pull-up resistors (`P`, or OpenOCD feature `PULLUP`) are
a separate, optional feature; they tie the lines to the VPU supply, not to a
per-line rail.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| TDI / TDO / TCK / TMS | MOSI / MISO / CLK / CS | MOSI / MISO / CLK / CS |
| SRST | AUX0 | AUX0 |
| TRST | PGD | AUX1 |
| XSVF player | — | available |

The v3 column is **not verified** (build-only).

## Related

- [openocd.md](openocd.md) — pin modes, features, the TAP shift.
- [xsvf.md](xsvf.md) — the XSVF player (no SRST/TRST use).
- [README.md](README.md) — availability summary and limits.