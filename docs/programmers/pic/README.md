# PIC (ICSP)

**Status:** v4 is documented here; v3 is included in the default build and is
not verified on hardware in this set.

PIC mode is the Bus Pirate's **ICSP** bit engine.  It drives the programming
clock and data lines by hand and clocks out the opcodes and payloads a host
gives it.  The **terminal mode** carries no PIC part database and no erase,
program, verify or configuration-word algorithm; a host tool supplies the
sequence.  The binary paths add fixed command engines, not a part algorithm.
The mode displays as `PIC`; the binary loop prints `PIC1`.

**Who it is for:** an operator or host program driving a PIC's ICSP sequence.
The Bus Pirate is the bit engine; it is not a stand-alone programmer.

**Start here:** wire with the power off, enter `m9` (v4) or `m8` (v3), select
the VPU source on v4 (v3 feeds `Vpu` externally), power the target, enable the
pull-ups, apply the
programming-mode condition (VPP for HVPP, MCLR at VDD for LVPP), then run
`(1)`.  The sequence and the wiring are in [wiring.md](wiring.md); `(1)` is
in [macros.md](macros.md).

This README is a map, not the manual; the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | mode number, prompts, defaults, width |
| [wiring.md](wiring.md) | signals, pull-up, supplies, LVPP/HVPP, adapter |
| [bus-operations.md](bus-operations.md) | `[`/`]`, writes, reads, framing, timing |
| [macros.md](macros.md) | `(0)`, `(1)` devID, no-answer signatures |
| [config-words.md](config-words.md) | configuration words, host sequence, lock-out |
| [sessions.md](sessions.md) | worked terminal sessions |
| [binary.md](binary.md) | `PIC1` and the raw-wire `0xAx` group |
| [picprog.md](picprog.md) | the external host tool and its route |
| [edge-cases.md](edge-cases.md) | failure modes and limits |

## The five-minute version

| Step | Do | Result |
|---|---|---|
| 1. Wire | Power off.  `PGC (CLK)` to PGC, `PGD (MOSI)` to PGD, `GND` to VSS; a pull-up on each line is **required**. | wired |
| 2. Enter | `m9` on v4 or `m8` on v3; answer the prompts, or press Enter for 6b/14b, 1 ms. | `PIC>` |
| 3. VPU and power | `e2`/`e3` on v4, then `W` if the target or the pull-up feed uses the rails; v3 feeds VPU externally. | VPU and rails on |
| 4. Pull-up and programming mode | `P`, then apply VPP for HVPP or tie MCLR to the target's VDD for LVPP; do this before `(1)`. | pull-up on; target in programming mode |
| 5. Command or data | `[` selects command state; `]` selects data state.  A value is an opcode or a payload. | `CMD6`/`CMD4`, `DTA` |
| 6. Read or ID | `r` clocks in 14 bits (6b/14b) or 16 bits (4b/16b); `(1)` runs the 6-bit ID read. | `READ: ...`, `DevID = ...` |

The mode drives both lines open drain: **a high is a released line and only
the pull-up makes it high.**  A self-powered target with external pull-ups
skips `e`, `W` and `P`; apply VPP/MCLR and run `(1)`.  `(1)` does not drive
MCLR.  Binary: enter `BBIO1`, send `0x07`, get `PIC1`; the loop has its own
commands in [binary.md](binary.md).

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | included in the default build | included in the default image |
| mode number in the default image | 8 | 9 |
| terminal `[`/`]`, `(1)` devID, `r` | present | present |
| `PIC1` binary (`0x07`) | present | present |
| raw-wire PIC group (`0xAx`) | present | present |
| VPU source (`e`) | external feed only | `e1` external/none, `e2` 3.3 V, `e3` 5.0 V |
| required pull-up | yes | yes |

The mode number is the **default-image** position.  A custom build with a
different enabled-mode set shifts it; the menu is dynamic.

## Names used in this set

The firmware's own strings are the authority.  Two forms exist for some
items; the mapping is stated here once and used consistently below.  The
host tool's banner is the tool's own string, not firmware.

| Item | Source form | Used here |
|---|---|---|
| mode | `PIC` | `PIC` |
| binary loop | `PIC1` | `PIC1` |
| command banner | `CMD6` / `CMD4` | `CMD6` / `CMD4` |
| data banner | `DTA` | `DTA` |
| pins in the `v` row | `PGC` / `PGD` | `PGC (CLK)` / `PGD (MOSI)` |
| setup choice | `6b/14b` / `4b/16b` | `6b/14b` / `4b/16b` |
| setup header | `PIC(mod dly)=(6 1)` | `6` = 6b/14b, `4` = 4b/16b |
| pull-up reference | `Vpullup pin`; `VPU` in `v` | `VPU` |
| host tool | `picprog` / PiratePICprog | `picprog` |
| host tool banner | `(Bus) Pirate Pic Programmer` | `picprog` (the tool's own spelling) |

The one-line header prints the opcode width alone (`6` or `4`); the prompt
names the same setting with both widths (`6b/14b`, `4b/16b`).  The `e3`
message says `5V`; the rail is the `5.0V` rail.

## Limits and out of scope

| Limit | What it means | Where |
|---|---|---|
| No PIC algorithm in the terminal mode | no erase, program, verify, configuration-word or part-database logic; the only built-in sequence is `(1)`, 6b/14b only | [bus-operations.md](bus-operations.md), [macros.md](macros.md) |
| No VPP generator | the Bus Pirate supplies at most 5 V; no pin may see VPP; HVPP needs an external supply and switch | [wiring.md](wiring.md) |
| No configuration-word decode | a configuration word is a 14/16-bit payload; its value and meaning belong to the part | [config-words.md](config-words.md) |
| No speed menu | the line rate is fixed at about 100 µs per bit (roughly 10 kHz) | [bus-operations.md](bus-operations.md) |
| MCLR is not driven by the terminal bus operations | the terminal can drive CS after selecting it with `C` (the command family is called AUX; the pin is CS); `PIC1` has `0x18`–`0x1F` | [wiring.md](wiring.md), [binary.md](binary.md) |
| 4b/16b has no devID macro | use the host tool | [macros.md](macros.md) |

Shared prompt grammar (`r`, `W`, `P`, `(n)`, `e`, `v`, `#`, `$`) is owned by
the generic layer and not repeated here.

## Related docs

- [The generic prompt layer](../../generic/README.md) — shared commands.
- [Generic values](../../generic/values.md) — `;N`, `l`/`L`, display formats.
- [The binary layer](../../protocol/binary/README.md) — `BBIO1` and framing.
- [The AVRISP-MKII personality](../avrisp/README.md) — AVR targets.
- [SPI](../../protocol/spi/README.md) — the other open-drain mode.
- [SPEC](../../SPEC.md) — the capability index.
