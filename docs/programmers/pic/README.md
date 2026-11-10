# PIC programmer (PIC mode)

**Status:** draft — available on v3 and v4; v3 is build-only and unverified.

The **PIC** mode turns the Bus Pirate into the ICSP "programmer" half of a
Microchip PIC programming link.  It drives the two In-Circuit Serial
Programming lines (clock and data) by hand and lets a host sequence the PIC
programming algorithm.  The Bus Pirate carries **no part database and no
algorithm**: it clocks bits out and in on the PIC's PGD/PGC lines on command.
The companion host program is `picprog` — see the
[picprog companion card](picprog.md).

The mode's spine is two on-bus states:

- **Command mode** — entered with `[`.  The next value is clocked out as a
  6-bit or 4-bit programming **opcode**.
- **Data mode** — entered with `]`.  The next value is clocked out as a
  14-bit or 16-bit **payload**; `r` reads a payload back.

The mode displays as `PIC`.  The mode's `v` signal row prints the two names
**PGC** (the clock, on the Bus Pirate `CLK` pin) and **PGD** (the data, on
`MOSI`) in the clock/data columns.  This set writes the mode label first with
the pin in parentheses: **PGC (CLK)** and **PGD (MOSI)**, and uses those names
throughout.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | entry, prompts, defaults, one-line form, width ↔ family |
| [wiring.md](wiring.md) | PGC/PGD/VPP/MCLR, the required pull-up, HVPP and LVPP, v3/v4 |
| [bus-operations.md](bus-operations.md) | command/data, writes, reads, timing, a session |
| [macros.md](macros.md) | the `(0)` list and the `(1)` devID macro |
| [binary.md](binary.md) | `PIC1` (`0x07`) and the raw-wire PIC group |
| [picprog.md](picprog.md) | companion card for the external host tool |
| [edge-cases.md](edge-cases.md) | failure modes and current limitations |

## The five-minute version

**Start here:** wire first (power off) → `m9` on v4 (or `m8` on v3) → `W`,
`e2`/`e3`, `P` → `(1)`.

- Enter PIC with `m` then **9** on v4 (or **8** on v3), or directly `m9`/`m8`;
  answer the two prompts.  Defaults: opcode width **6b/14b**; delay **1 ms**
  (the `PIC1` binary default is **2 ms**).
- Wiring headline: **PGC (CLK) → PGC, PGD (MOSI) ↔ PGD, plus ground, and a
  pull-up — required** (the mode drives both lines open-drain; high =
  released).
  For **low-voltage** parts tie MCLR to the target's VDD (or drive it high);
  the Bus Pirate does not drive it.  For **high-voltage** parts the VPP entry
  needs a separate supply and switching circuit — see the HVPP subsection in
  [wiring.md](wiring.md).  No Bus Pirate pin is VPP-tolerant.
- `W` engages the bus; select the on-board pull-up source *before* `P` — v4
  `e2` (3.3 V) or `e3` (5.0 V), then `P`.
- `[` selects **command** mode (prints a `CMD6`/`CMD4` banner); `]` selects
  **data** mode (prints `DTA`).  A value written is clocked out as an opcode or
  a payload, depending on the state.
- `(1)` reads the device ID and revision: for example
  `DevID = 0x0012 Rev = 0x1F`.
- The Bus Pirate is a bit engine here; the erase/write/verify algorithm lives
  in the host tool.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | present (build-only here) | present (default image) |
| mode number in the default image | 8 (KEYB absent) | 9 |
| command/data `[`/`]`, devID macro | present | present |
| binary `PIC1` (`0x07`) | present | present |
| on-board pull-up source `e` | — (feed VPU externally) | `e1` / `e2` / `e3` |
| required pull-up | yes | yes |

The mode number is the **default-image** position; a custom build with a
different enabled-mode set shifts it.  See the mode-menu note in
[the generic navigation page](../../generic/navigation.md).

## Limits & out of scope

- **No PIC algorithm.**  No erase, program, verify, configuration-word or
  part-database logic is built into the mode; a host program must supply it.
  The only built-in intelligence is the `(1)` devID macro, and it works in
  **6b/14b** only.
- **No VPP generator.**  The Bus Pirate supplies at most 5 V and no pin is
  VPP-tolerant; parts that need a high-voltage VPP entry require an external
  supply and switching circuit.  The legacy high-voltage adapter is **v3
  pinout only** — see [wiring.md](wiring.md).
- The **4b/16b** path has no devID macro: `(1)` prints `Not implemented (yet)`.
- The mode's own line rate is fixed and slow (tens of microseconds per clock
  phase); there is no speed menu.  See [bus-operations.md](bus-operations.md).
- Shared prompt grammar (`r`, `W`, `P`, `(n)`, wrappers, `e`, `v`, safe reset
  `#`, bootloader `$`) lives in [the generic layer](../../generic/README.md)
  and is not repeated here.
- v3 behaviour throughout this set is **not verified** (v3 is build-only
  here).

## Related docs

- [The generic prompt layer](../../generic/README.md) — shared commands.
- [Generic commands](../../generic/commands.md) — `W`, `P`, `e`, `v`, `r`,
  wrappers, macros.
- [SPEC](../../SPEC.md) — the capability index.
- [The binary layer](../../protocol/binary/README.md) — `PIC1` = binary
  command `0x07`.
- Sibling programmer pages: [the AVRISP-MKII personality](../avrisp/README.md)
  and [the JTAG protocol](../../protocol/jtag/README.md).
