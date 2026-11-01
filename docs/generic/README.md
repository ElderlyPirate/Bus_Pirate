# Using the prompt (generic layer) — first draft

**Status:** draft, 2026-11-01 — "how to use the prompt" as its own usage
document (part of the central `docs/` plan).  The capability matrix (v3/v4)
lives in `SPEC.md` §1; per-mode command details live with each mode
(`docs/<mode>/`).  This page explains how to drive the prompt itself.

Applies to v3 and v4 unless noted.

## 1. Getting around
| Key | What it does |
|---|---|
| `i` | banner / info: firmware version, chip revision, enabled modes |
| `?` | help screen (command map) |
| `m` | mode menu — change mode; `x` leaves without changing |
| `h` | command history (v4: 15 entries, v3: 7) |
| `b` | terminal link baud rate: 300…115200 plus custom BRG |

## 2. Entering values and text
- Numbers are decimal by default; `0x…` hex, `0b…` binary.
- `"text"` sends a string; plain numbers send a value (per mode).
- `;` injects extra bits into the previous value.
- `=` converts a value between bases; `|` reverses the bit order.
- `o` selects the display format: HEX / DEC / BIN / RAW (+ line length).

## 3. Controlling & inspecting the elements
| Key | Element |
|---|---|
| `W` / `w` | power supplies on / off (`W` prints the voltages) |
| `P` / `p` | pull-up resistors on / off (mind the VPU warning) |
| `e` | pull-up (VPU) source: External / 3.3 V / 5 V — **v4 only** |
| `a` / `A` / `@` | AUX pin: low / high / read |
| `c` / `C` (`k` / `K` v4) | AUX assignment: A0 / CS / A1 / A2 (via `k`/`K` on v4) |
| `L` / `l` | bit order: MSB-first / LSB-first |
| `v` | show pin states and voltages |
| `d` / `D` | ADC: one-shot / continuous; `d:N` repeats N times |
| `f` | frequency counter (AUX pin) |
| `g` / `S` | PWM / servo output (AUX pin; not in HiZ) |
| `t` / `T` | elapsed timer: read / reset (µs) |
| `~` | self-test (HiZ only) |
| `&` / `%` | delay N µs / N ms |

## 4. Macros
- `<n>` — user macros: list / assign / run (slot counts differ between v3/v4).
- `(n)` — mode macros: run; `(0)` shows the current mode's macro menu.

## 5. Wrappers, reads and bit ops
- `[` `{` `]` `}` — start / start-with-read / stop / stop-with-read.
- `r` — read (arguments `r:N`, `r;bits`); exact semantics per mode.
- `/` `\` `^` `-` `_` `.` `!` — clock hi / clock lo / tick / data hi /
  data lo / read / bit read (meaning is mode-dependent).

## 6. Reset and firmware
- `#` — reset; v3: hardware reset, v4: simulated (returns to HiZ).
- `$` — jump to the bootloader (confirm with `y`) — the enabler for
  firmware updates.
- `s` — BASIC interpreter (documented under `docs/basic/`).

## 7. Notes / TODO (draft)
- Verify the v3/v4 deltas: history depth, `c`/`C` vs `k`/`K`, `#` behaviour,
  macro slot counts.
- The help label for `o` ("Set output type") is a misnomer — it sets the
  *display* format; earmarked for the firmware help cleanup, not just docs.