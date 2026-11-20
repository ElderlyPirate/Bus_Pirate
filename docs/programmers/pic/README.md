# PIC (ICSP)

**Status:** current · the ICSP bit engine; v4 is the reference build, and the
v3 build is included but not verified.

PIC mode is the Bus Pirate's **ICSP** (in-circuit serial programming) bit
engine.  It drives the programming clock and data lines by hand and clocks out
the opcodes and payloads a host gives it.  The **terminal mode** carries no PIC
part database and no erase, program, verify or configuration-word algorithm; a
host tool supplies the sequence.  The binary paths add fixed command engines,
not a part algorithm.  The mode displays as `PIC`; the binary loop prints
`PIC1`.

Terms used in this set: **PGC** is the target's program clock, **PGD** its
program data, **MCLR** its reset pin, **VPP** the programming voltage applied to
MCLR (6-13 V), **HVPP** high-voltage programming (VPP on MCLR) and **LVPP**
low-voltage programming (MCLR at the target's VDD).

**Who it is for:** an operator or host program driving a PIC's ICSP sequence.
The Bus Pirate is the bit engine; it is not a stand-alone programmer.

## Choose your route

| I want to... | Use | Read |
|---|---|---|
| read the device ID | terminal `(1)` | [macros.md](macros.md) |
| clock one opcode or word | terminal `[`/`]` and `r` | [bus-operations.md](bus-operations.md) |
| script a byte sequence | the `PIC1` loop (`0x07`) | [binary.md](binary.md) |
| program a whole part | the `picprog` host tool | [picprog.md](picprog.md) |
| write my own host | raw wire (`0x05`) plus `0xA7` | [binary.md](binary.md) |

## Start here

1. Wire with the power off: PGC/PGD, ground and the pull-up.
   See [wiring.md](wiring.md).
2. Enter `m9` (v4) or `m8` (v3); select the VPU source on v4 (v3 feeds `Vpu`
   externally).
3. Power the target, connect the pull-ups, then apply the programming-mode
   condition (VPP for HVPP, MCLR at VDD for LVPP).
4. Run `(1)` for the first success; the worked bring-up is in
   [sessions.md](sessions.md).

This README is a map, not the manual; the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | mode number, prompts, defaults, width, the engine-number map |
| [wiring.md](wiring.md) | signals, pull-up, supplies, LVPP/HVPP, the HVPP adapter |
| [bus-operations.md](bus-operations.md) | `[`/`]`, writes, reads, framing, timing |
| [macros.md](macros.md) | `(0)`, `(1)` devID, no-answer signatures |
| [config-words.md](config-words.md) | configuration words, the host sequence, lock-out |
| [sessions.md](sessions.md) | worked terminal sessions |
| [binary.md](binary.md) | `PIC1` and the raw-wire `0xAx` group |
| [picprog.md](picprog.md) | the external host tool and its route |
| [edge-cases.md](edge-cases.md) | failure modes and limits |

## The five-minute version

| Step | Do | Result |
|---|---|---|
| 1. Wire | Power off.  PGC to CLK, PGD to MOSI, GND to VSS.  A pull-up is **required**. | wired |
| 2. Enter | `m9` on v4 or `m8` on v3; Enter through the prompts for 6b/14b, 1 ms. | `PIC>` |
| 3. Pull-up feed | `e2`/`e3`, then `W`, then `P` for the board feed; a self-powered target with external pull-ups skips all three. | VPU and pull-ups on if used |
| 4. Programming mode | VPP for HVPP, or MCLR at the target's VDD for LVPP. | in programming mode |
| 5. Read the ID | `(1)`. | `DevID = ... Rev = ...` |
| 6. Command or data | `[` command, `]` data; send `]` before `r`. | `CMD6`/`CMD4`, `DTA`, `READ: ...` |

The pull-up is 2-10 kΩ on each line; use `P` only for the board pull-ups.  The
mode drives both lines open drain: **a high is a released line and only the
pull-up makes it high.**  A self-powered target with external pull-ups skips
`e`, `W` and `P`; apply VPP/MCLR and run `(1)`.  `(1)` does not drive MCLR.
Binary: enter `BBIO1`, send `0x07`, get `PIC1`; that loop has its own commands
in [binary.md](binary.md).

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

The firmware's own strings are the authority.  Where two forms exist, the
mapping is stated here once and used consistently below.  The three width
numbering schemes are mapped in [entry-and-setup.md](entry-and-setup.md).

| Item | Source form | Used here |
|---|---|---|
| mode | `PIC` | `PIC` |
| binary loop | `PIC1` | `PIC1` |
| command banner | `CMD6` / `CMD4` | `CMD6` / `CMD4` |
| data banner | `DTA` | `DTA` |
| pins in the `v` row | `PGC` / `PGD` | `PGC (CLK)` / `PGD (MOSI)` |
| setup choice | `6b/14b` / `4b/16b` | `6b/14b` / `4b/16b` |
| setup header | `PIC(mod dly)=(6 1)` | `6` = 6b/14b, `4` = 4b/16b |
| pull-up reference | `Vpu` (board); `Vpullup` (warnings); `VPU` (`v`) | `VPU` |
| host tool | executable `picprog`; project PiratePICprog | `picprog` |
| host tool banner | `(Bus) Pirate Pic Programmer` | quoted as the banner |

The one-line header prints the opcode width alone (`6` or `4`); the prompt
names the same setting with both widths (`6b/14b`, `4b/16b`).  The `e3` message
says `5V`; the rail is the `5.0V` rail.

## Limits and out of scope

| Limit | Effect |
|---|---|
| [Terminal mode has no PIC algorithm](bus-operations.md) | no erase, program, verify or configuration-word logic; `(1)` is the only sequence, 6b/14b only |
| [No VPP generator](wiring.md) | at most 5 V; no pin may see VPP; HVPP needs the adapter or your own switch |
| [No configuration-word decode](config-words.md) | the word is a payload; its value and meaning belong to the part |
| [No speed menu](bus-operations.md) | fixed at about 100 µs per bit (roughly 10 kHz) |
| [MCLR is not driven by the bus operations](wiring.md) | `(1)`, `[`/`]` and `r` touch PGC/PGD only |
| [4b/16b has no devID macro](picprog.md) | use `picprog` for a 4b/16b part |

Shared prompt grammar (`r`, `W`, `P`, `(n)`, `e`, `v`, `#`, `$`) is owned by
the generic layer and not repeated here.

## Related docs

- [The generic prompt layer](../../generic/README.md) — shared commands.
- [Generic values](../../generic/values.md) — `;N`, `l`/`L`, display formats.
- [The binary layer](../../protocol/binary/README.md) — `BBIO1` and framing.
- [I2C wiring](../../protocol/i2c/wiring.md) — the on-board pull-up values.
- [The AVRISP-MKII personality](../avrisp/wiring.md) — the v4 header pin numbering.
- [SPEC](../../SPEC.md) — the capability index.
