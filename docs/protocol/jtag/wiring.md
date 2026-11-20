# JTAG — wiring, levels and power

Part of the JTAG documentation set.  This page is the single home for the
physical mapping, the supply rules, the logic levels, the reset lines and
every pull-up requirement.  The protocol pages refer back here.

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

TDI, TCK and TMS are driven by the Bus Pirate once a path drives them; TDO
is an input driven by the target.  SRST and TRST are driven only after the
OpenOCD path's pin mode makes them outputs.  The XSVF player does not use
them.

## Names

The terminal prints pin names; the AUX commands use target tokens.  They
name the same pins:

| JTAG signal | Terminal pin | Target token | Where |
|---|---|---|---|
| SRST | `AUX` | `A0` | main header |
| TRST on v4 | `AUX1` | `A1` | main header |
| TRST on v3 | `PGD` | — | programming header |
| not used | `AUX2` | `A2` | main header (v4) |

`c` selects `A0`, `k` selects `A1` and `K` selects `A2`; `a` / `A` drive the
selected pin low / high.  The `AUX2` target is not used by JTAG.

The pull-up reference pin has these forms:

| Context | Printed form |
|---|---|
| prose in this set | `Vpullup` |
| terminal `v` table | `VPU` |
| OpenOCD `buspirate adc` line | `VPullup` |

They are the same pin: the one marked `Vpu` on the reference card.

## Power

The on-board regulator (**VREG**) drives the two target-supply rails.  It is
off at binary entry.  The OpenOCD path leaves it off until the host sends
feature `0x02 0x02 0x01`; the XSVF entry switches it on.

| Bus Pirate pin | Direction | Role |
|---|---|---|
| `3.3V` | out | 3.3 V target supply, on while VREG is on |
| `5.0V` | out | 5.0 V target supply, on while VREG is on |
| GND | — | common ground with the target |

**Wire the target's VCC to one rail only, and only when the board powers
the target.**  Never connect both rails.  If the target has its own supply,
leave its VCC unconnected from the board rails and keep VREG off; never
back-drive a powered target.  The XSVF entry cannot leave VREG off, so
disconnect the target's own supply before a player run.

Neither path can select a rail voltage: VREG brings both rails up at their
fixed levels.  There is no 1.8 V or 2.5 V setting.

The XSVF player cannot drive a target below 3.3 V: it powers VREG on entry
and its outputs are 3.3 V push-pull.  For a 1.8 V or 2.5 V chain, use the
OpenOCD path in open-drain mode with pull-ups to the target's own rail (and
a POSIX host — see [README.md](README.md#start-here)).

If a rail is shorted, the player reports nothing: the only symptom is a
target that does not answer.  To check it, press reset, enter a mode (`m`),
then `W`; `W` prints `VREG too low, is there a short?` and leaves the
supplies off.

## Levels and voltage

- Logic level is **3.3 V**: high is 3.3 V, low is ground.
- The interface is **5 V safe**; the maximum applied voltage is **5.5 V**.
- A 5 V target may not see 3.3 V as a valid high.  Check its input threshold
  before connecting a push-pull output.
- A 1.8 V or 2.5 V target **cannot use the XSVF player**: it drives 3.3 V
  push-pull and has no open-drain mode.  Use the OpenOCD **open-drain** pin
  mode with pull-ups to the target's own rail.
- Do **not** drive a 3.3 V push-pull output into a 1.8 V chain.
- TDO is an input on the Bus Pirate side.  Never drive it from the board.

## Pull-ups

| Path / mode | Pull-up requirement |
|---|---|
| XSVF player (`0x18`) | **no pull-up needed** — pins are push-pull |
| OpenOCD push-pull (`0x01 0x01`) | **no pull-up needed** |
| OpenOCD open-drain (`0x01 0x02`) | **2–10 kΩ** on TDI/TMS/TCK |
| OpenOCD open-drain, SRST/TRST used | the same **2–10 kΩ** on each |
| OpenOCD before a pin mode | pins high-Z; nothing is driven |

In open-drain mode the Bus Pirate only pulls each line **low**; the pull-up
sets the high level, so it can be tied to the target rail for level
shifting.  SRST and TRST also become open-drain outputs in that mode and
cannot go high without their pull-ups, even after a feature releases them.
TDO is always an input and needs no Bus Pirate pull-up.

The on-board pull-up resistors (feature `0x10`) are a separate, optional
circuit.  They tie the lines to **Vpullup**, not to a per-line rail, and a
voltage in the 0–5 V range must be present there.  Feed Vpullup externally:
the v4 on-board source is not selectable from the JTAG path (binary entry
switches it off and terminal `e` is unreachable), and v3 has no on-board
source.

The terminal `P` command controls the same resistors, but it is only a
pre-entry check: binary entry switches them off again, and `P` is refused in
`HiZ`.  In a session, use feature `0x10`.  A BBIO1 pin write can switch them
on before the XSVF selection; the player does not clear them.  They are not
a substitute for the per-line pull-ups in open-drain mode.

## Reset lines

- The OpenOCD pin mode drives SRST and TRST **low** when it makes them
  outputs.  If they are wired to active-low reset inputs, release them with
  feature `0x02 0x08 0x01` (SRST) and `0x02 0x04 0x01` (TRST).
- The feature action is the pin level, not an "assert" flag, and the reset
  lines take only its low bit: `0x00` is low, `0x01` is high.
- The XSVF player does not drive SRST or TRST.  A pin write inside `BBIO1`
  can leave AUX (SRST) low; a terminal AUX1 write on v4 can leave AUX1
  (TRST) low even after binary entry.  To release them at the terminal,
  run `#`, then `c`, `A` for SRST, or `#`, then `k`, `A` for TRST.
- A bare `0x00` exit from the OpenOCD path does not release TRST; it keeps
  its last level.  Send `0x01 0x00` first, or complete the terminal reset.
- A board reset makes every header pin an input and releases both resets.

On v3, TRST is the `PGD` pin on the ICSP/programming header, not a
main-header pin.  It needs a clip or a soldered header to use.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| TDI / TDO / TCK / TMS | MOSI / MISO / CLK / CS | MOSI / MISO / CLK / CS |
| SRST | AUX | AUX |
| TRST | PGD | AUX1 |
| XSVF player | — | available |
| `bp xsvf` host command | no player to drive | works |

The v3 OpenOCD path is build-only and its hardware behaviour is not verified.

## Related

- [entry.md](entry.md) — entry, settings and leaving.
- [openocd.md](openocd.md) — pin modes, features and the TAP shift.
- [xsvf.md](xsvf.md) — the XSVF player and its pins.
- [README.md](README.md) — availability summary and limits.
