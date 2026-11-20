# XSVF wiring, supplies and levels

Part of the XSVF set.  This page is what to connect for a firmware-player run
and which voltages are safe.  The authoritative pin map for both JTAG paths is
[protocol/jtag/wiring.md](../../protocol/jtag/wiring.md); this page repeats
only what an XSVF run needs.

## Signal map

| Bus Pirate | Direction | JTAG | Role |
|---|---|---|---|
| MOSI | to | TDI | test data in |
| MISO | from | TDO | test data out |
| CLK | to | TCK | test clock |
| CS | to | TMS | test mode select |
| GND | — | GND | signal ground |

TDI, TCK and TMS are outputs driven by the board; TDO is an input driven by
the target.  The ground wire is part of the interface, not an optional extra:
without it TDO is unreliable or absent.

The player uses only these five wires.  It does not drive SRST or TRST.

## Pull-ups

**No pull-up is needed.**  The player drives TDI, TCK and TMS as 3.3 V
push-pull outputs, so there is nothing to pull high.  TDO is driven by the
target and is always an input on the Bus Pirate side.

Entry does not enable the on-board pull-up resistors; binary entry switches
them off.  A custom host can turn them on again with a pin write inside
`BBIO1`; the player selection does not clear them, so they would stay on for
the run.  They tie the lines to **Vpullup**, not to the target's rail, and are
not a substitute for the push-pull drive.  `bp xsvf` does not enable them.

## Supply

The on-board regulator (**VREG**) drives the two target-supply rails.

| Bus Pirate pin | Role |
|---|---|
| `3.3V` | 3.3 V target supply, on while VREG is on |
| `5.0V` | 5.0 V target supply, on while VREG is on |

- **Wire the target's VCC to one rail only, never both.**  The two rails are
  separate outputs; joining them back-drives one supply from the other.
- **Entry always switches VREG on.**  The player has no way to leave it off,
  so **disconnect the target's own supply** before the run and never
  back-drive a powered target.
- **The player cannot select a rail voltage.**  VREG brings both rails up at
  their fixed levels; there is no 1.8 V or 2.5 V setting.
- **The rails stay on after the player returns.**  Leaving the player for
  `BBIO1` does not switch VREG off; only the terminal reset or a board reset
  does.  Do not unplug the target while the rails are on.
- **A faulted rail shows no player error.**  From inside the player the only
  symptom is a target that does not answer.  To see the check, press reset,
  enter a mode (`m`), then `W`; `W` is refused in `HiZ>`.

## Levels

- Logic level is **3.3 V**: high is 3.3 V, low is ground.
- The interface is **5 V safe**; the maximum applied voltage is **5.5 V**.
- **A 1.8 V or 2.5 V target cannot use the player.**  It drives 3.3 V
  push-pull and has no open-drain mode, and it switches VREG on.  Use the
  OpenOCD path in open-drain mode with pull-ups to the target's own rail.
  Do not drive a 3.3 V push-pull output into a 1.8 V chain.
- **A 5 V target may not see 3.3 V as a valid high.**  The `5.0V` rail can
  power it, but the JTAG signals remain 3.3 V; check the target's input
  threshold before connecting a push-pull output.
- TDO is an input.  Do not drive it from the Bus Pirate side.

## Reset lines and the unused pins

The player does not drive **SRST** or **TRST**.  Two leftovers can still hold
a target in reset:

| Leftover | Effect | Fix |
|---|---|---|
| AUX (SRST) left low by a `BBIO1` pin write | target held in system reset | reset, then `c`, `A` |
| AUX1 (TRST, v4) left low by a terminal AUX1 write | test logic in reset; all-zero reads | reset, then `k`, `A` |

At the terminal, `c` makes `a`/`A` control AUX and `k` makes them control
AUX1; `A` drives the selected pin high.  A board reset releases both.  There
is no TRST pin on the v3 main header (TRST is on the programming header
there), and the player is v4 only.

## Related

| Doc | Why |
|---|---|
| [protocol/jtag/wiring.md](../../protocol/jtag/wiring.md) | the full pin map, both paths, pull-up and Vpullup detail |
| [workflow.md](workflow.md) | the run sequence around this wiring |
| [limits-and-errors.md](limits-and-errors.md) | failure symptoms and recovery |
| [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md) | the player's electrical setup |
