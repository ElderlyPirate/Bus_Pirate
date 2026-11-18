# JTAG — wiring and levels

Part of the JTAG documentation set.  This page is the single home for the
physical mapping, levels, reset lines and pull-up/supply requirements.  The
protocol pages refer back here.

## Signal map

Both paths use the same four signals plus ground.  SRST and TRST are
optional.  The `3.3V` and `5.0V` pins are supplies, not signals; see
[Power](#power).

| Bus Pirate pin | Direction | JTAG signal | Role |
|---|---|---|---|
| MOSI | → | TDI | test data in |
| MISO | ← | TDO | test data out |
| CLK | → | TCK | test clock |
| CS | → | TMS | test mode select |
| AUX | → | SRST | system reset (optional) |
| AUX1 (v4) / PGD (v3) | → | TRST | test reset (optional) |
| GND | — | GND | signal ground |

TDI, TCK and TMS are driven by the Bus Pirate; TDO is an input driven by
the target.  SRST and TRST are driven only after the OpenOCD path's pin
mode is set; the XSVF player does not use them.

**Naming note.**  The terminal's pin name is `AUX` (the v3 help token is
`aux`); its AUX-assignment selection token is `A0`.  The v4 `AUX1` pin is
`A1`.  On v3, TRST is the `PGD` pin on the programming header, not a
main-header pin.

| JTAG signal | Terminal pin | Selection token |
|---|---|---|
| SRST | `AUX` | `A0` |
| TRST (v4) | `AUX1` | `A1` |
| TRST (v3) | `PGD` | — |

The v4 `AUX2` target is not used by JTAG.

## Power

The on-board regulator (**VREG**) drives the two target-supply rails.  It is
off at binary entry.  The OpenOCD path leaves it off until the host sends
feature `0x02 0x02 0x01`; the XSVF entry switches it on.

| Bus Pirate pin | Direction | Role |
|---|---|---|
| `3.3V` | out | 3.3 V target supply, on while VREG is on |
| `5.0V` | out | 5.0 V target supply, on while VREG is on |
| GND | — | common ground with the target |

Wire the target's VCC to **one** rail only, never both.  Neither JTAG path
can select a rail voltage: VREG brings up both rails at their fixed levels.
Leave VREG off in the OpenOCD path for a target with its own supply, and
never back-drive a powered target.  The XSVF entry cannot leave it off.

The XSVF player cannot drive a target below 3.3 V: it powers VREG on entry
and its outputs are 3.3 V push-pull.  Use the OpenOCD open-drain mode with
pull-ups to the target's own rail for a 1.8 V or 2.5 V chain.

## Levels and voltage

- Logic level is **3.3 V** (high = 3.3 V, low = ground).
- The interface is **5 V safe**; the maximum applied voltage is **5.5 V**.
- A 5 V target may not see 3.3 V as a valid high; check its input
  threshold before connecting a push-pull output.
- A 1.8 V or 2.5 V target **cannot use the XSVF player**: it drives
  3.3 V push-pull and switches VREG on.  Use the OpenOCD **open-drain**
  pin mode with pull-ups to the target's own rail.  Do **not** drive a
  3.3 V push-pull output into a 1.8 V chain.
- **Power.**  VREG drives the `3.3V` and `5.0V` header rails; see
  [Power](#power).  The regulator stays on after the XSVF player exits;
  a raw-serial host can send `0x0F` twice (the second returns to the
  terminal); `#` is then available and also clears it.  See
  [xsvf.md](xsvf.md) §Entry and exit.

## Pull-ups

| Path / mode | Pull-up requirement |
|---|---|
| XSVF player (`0x18`) | **no pull-up needed** — the pins are driven push-pull |
| OpenOCD path, pin mode `0x01` | **no pull-up needed** — push-pull outputs |
| OpenOCD open-drain (`0x02`) | **pull-ups required**, **2–10 kΩ** on TDI/TMS/TCK |
| OpenOCD, before a pin mode | pins high-Z; nothing driven until `0x01` |

In open-drain mode the Bus Pirate only pulls each line **low**; the pull-up
sets the high level, so it can be tied to the target rail for level
shifting.  SRST and TRST, when used, also become open-drain outputs and
need the same pull-ups; without them they cannot go high even after the
feature releases them.  TDO is always an input and is driven by the
target, so it needs no Bus Pirate pull-up.

The on-board pull-up resistors (feature `0x10`, or `P` at the terminal)
are a separate, optional feature; they tie the lines to the **Vpullup**
supply, not to a per-line rail.  Vpullup is the pin marked `Vpu` on the
reference card and printed as `VPU` in the terminal `v` table.  A voltage
in the 0–5 V range must be present on Vpullup: feed it externally, because
the on-board v4 source is not selectable from the JTAG path (entering the
binary layer switches it off, and the terminal `e` command is not
reachable here).  See [openocd.md](openocd.md) §Features.

## Reset lines

- The OpenOCD pin mode drives SRST and TRST **low** when it makes them
  outputs.  If they are wired to active-low reset inputs, release them
  with feature `0x02 0x08 0x01` (SRST) and `0x02 0x04 0x01` (TRST).
- The feature action is the pin level, not an "assert" flag.  Check the
  target schematic; the firmware does not invert either line.
- The XSVF player does not drive SRST or TRST.  A pin write inside `BBIO1`
  can leave AUX (SRST) low; a terminal AUX write on the v4 `A1` target can
  leave AUX1 (TRST) low even after binary entry.  At the terminal, reset,
  then press `c` and `A` for AUX (SRST) or `k` and `A` for AUX1 (TRST); or
  reset the board before entering the player.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| TDI / TDO / TCK / TMS | MOSI / MISO / CLK / CS | MOSI / MISO / CLK / CS |
| SRST | AUX | AUX |
| TRST | PGD | AUX1 |
| XSVF player | — | available |

The v3 column is **not verified** (build-only).

## Related

- [entry.md](entry.md) — entry, settings and exit.
- [openocd.md](openocd.md) — pin modes, features, the TAP shift.
- [xsvf.md](xsvf.md) — the XSVF player (no SRST/TRST use).
- [README.md](README.md) — availability summary and limits.
