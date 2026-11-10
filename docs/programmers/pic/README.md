# PIC programmer (PIC mode)

**Status:** draft — available on v3 and v4; v3 is build-only and unverified.

The **PIC** mode turns the Bus Pirate into the ICSP "programmer" half of a
Microchip PIC programming link.  It drives the two In-Circuit Serial
Programming lines (clock and data) by hand and lets a host sequence the PIC
programming algorithm.  The Bus Pirate carries **no part database and no
algorithm**: it clocks bits out and in on the PIC's PGD/PGC lines on command.
The companion host program is `picprog` — see [picprog.md](picprog.md).

The mode's spine is two on-bus states:

- **Command mode** — entered with `[`.  The next value is clocked out as a
  6-bit or 4-bit programming **opcode**.
- **Data mode** — entered with `]`.  The next value is clocked out as a
  14-bit or 16-bit **payload**; `r` reads a payload back.

The mode displays as `PIC`.  Its two lines are printed by the mode as `PGC`
(the clock, on the Bus Pirate `CLK` pin) and `PGD` (the data, on `MOSI`).
This set writes the mode label first with the pin in parentheses: **PGC
(CLK)** and **PGD (MOSI)**, and uses those names throughout.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | the mode number, the two setup prompts, defaults, the one-line form |
| [wiring.md](wiring.md) | the PGC, PGD, VPP and MCLR lines, the required pull-up, the 13 V question, v3/v4 |
| [bus-operations.md](bus-operations.md) | `[`/`]` command/data selection, writes, reads, timing, a session |
| [macros.md](macros.md) | the `(0)` macro list and the `(1)` devID macro |
| [binary.md](binary.md) | the `PIC1` binary sub-protocol (`0x07`) and the raw-wire PIC group |
| [picprog.md](picprog.md) | the host tool that drives the mode |
| [edge-cases.md](edge-cases.md) | failure modes and known quirks |

## The five-minute version

- Enter PIC with `m` then **9** on v4 (or **8** on v3), or directly `m9`/`m8`;
  answer the two prompts.  Defaults: opcode width **6b/14b**; delay **1 ms**
  (the `PIC1` binary default is **2 ms**).
- Wiring headline: **PGC (CLK) → PGC, PGD (MOSI) ↔ PGD, plus ground, and a
  pull-up — required** (the mode drives both lines open-drain; high =
  released).  For high-voltage parts, connect the **CS** line to the VPP
  switching circuit (see [wiring.md](wiring.md)) — the 13 V VPP itself is
  **not** supplied or sequenced by the Bus Pirate: the host (or a
  high-voltage adapter) drives it, and PIC terminal mode leaves CS alone.
  Low-voltage parts need no VPP.
- `W` engages the bus; select the on-board pull-up source *before* `P` — v4
  `e2` (3.3 V) or `e3` (5.0 V), then `P`.
- `[` selects **command** mode (prints a `CMD…` banner); `]` selects **data**
  mode (prints `DTA`).  A value written is clocked out as an opcode or a
  payload, depending on the state.
- `(1)` reads the device ID and revision: `DevID = 0x0012 Rev = 0x1F`.
- The Bus Pirate is a bit engine here; the erase/write/verify algorithm lives
  in the host tool.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | present (build-only here) | present (default image) |
| mode number in the menu | 8 (keyboard mode absent) | 9 |
| command/data `[`/`]`, devID macro | present | present |
| binary `PIC1` (`0x07`) | present | present |
| on-board pull-up source `e` | — (feed VPU externally) | `e1` / `e2` / `e3` |
| required pull-up | yes | yes |

Full per-item detail and the exact wording are in the topic files.

## Limits & out of scope

- **No PIC algorithm.**  No erase, program, verify, configuration-word or
  part-database logic is built into the mode; a host program must supply it.
  The only built-in intelligence is the `(1)` devID macro.
- **No VPP generator.**  The Bus Pirate supplies at most 5 V; parts that need
  a high-voltage VPP entry require an external supply or the high-voltage
  adapter — see [wiring.md](wiring.md).
- The **4b/16b** path has no devID macro: `(1)` prints `Not implemented (yet)`.
- The mode's own line rate is fixed and slow (tens of microseconds per clock
  phase); there is no speed menu.  See
  [bus-operations.md](bus-operations.md).
- Shared prompt grammar (`r`, `W`, `P`, `(n)`, wrappers, `e`, `v`, safe reset
  `#`, bootloader `$`) lives in [the generic layer](../../generic/README.md) and
  is not repeated here.
- v3 behaviour throughout this set is **not verified** (v3 is build-only
  here).

## Related docs

- [../generic/README.md](../../generic/README.md) — the shared prompt layer.
- [../generic/commands.md](../../generic/commands.md) — `W`, `P`, `e`, `v`, `r`,
  wrappers, macros.
- [../SPEC.md](../../SPEC.md) — the capability index (`docs/SPEC.md`).
- [../binary/README.md](../../protocol/binary/README.md) — the binary layer entry
  (`PIC1` = binary command `0x07`).
- Sibling programming pages: [../avrisp/README.md](../avrisp/README.md) (the
  AVRISP-MKII personality) and [../jtag/README.md](../../protocol/jtag/README.md).