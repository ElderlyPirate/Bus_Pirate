# DIO — Phase F delta audit (RA-7)

**Worktree:** `C:\Users\jschl\dev\bpv4-docssweep`, docs head `dddcb02b`;
final combined firmware `9b179cb5`.  The DIO cleanup was checked in the
worktree's own `Firmware/protocol_dio/dio.c`: `dio_cleanup()` clears
`AUX | MOSI | CLK | MISO | CS` and, under `BUSPIRATEV4`, adds
`AUX1 | AUX2`; it sets those directions to input, disables the pull-ups and
the v4 VPU source, clears the matching latches, and restores 8-bit width.
`switch_psu_off()` calls that cleanup.  The selector, ODC and PPS settings are
not touched.  v3 therefore parks five pins; v4 parks seven.

## Verdict

**No** — the DIO set still documents the pre-RA-7 five-pin `w` park.  The
owner contract (`park-and-reset.md`) and the wiring safety text both state
that `w` does not release `AUX1`/`AUX2`, which is now false on v4; several
other DIO pages repeat the same five-pin claim.

## Findings

### 1. [severity: blocker] `docs/protocol/dio/park-and-reset.md` — the safe-park contract is still five-pin

**Quotes:**

> “For the five signal pins, the mode's cleanup does five things, in this order:” (line 11)

> “`w` performs this park and then switches the supply off.  It does **not** release `AUX1` or `AUX2`.” (lines 24–26)

> “`w` releases only the five signal pins; `#` releases every I/O pin:” (lines 52–53)

> “| `AUX1` / `AUX2` (v4) | **not changed** | input | input |” (line 59)

> “`w` parks the five signal pins.  It does **not** release `AUX1` or `AUX2`, because those pins are not part of the direct I/O value or its park mask.” (lines 114–115)

> “After `w`: only the five signal pins are released, so check that those five show `I`.  `AUX1` / `AUX2` are unchanged…” (lines 133–135)

> “| `w` parks the five signal pins | yes | yes |” (line 146)

**Why wrong:** `w` calls the DIO cleanup, and the v4 cleanup mask now includes
`AUX1` and `AUX2`.  Both are set to input and their latches cleared even when
the current AUX target is `AUX` or `CS`.  This page is the owner of the
safe-park contract and currently teaches the opposite of the shipped v4
behaviour.

**Fix:** rewrite the contract around **v3 = five, v4 = seven**:
`w` parks `AUX`/`MOSI`/`CLK`/`MISO`/`CS` on both boards, plus both `AUX1` and
`AUX2` on v4; it does not depend on which target is selected.  The cleanup
order and “latches after bias” explanation remain valid, but apply to seven
pins on v4.  Update the action table (line 34), the `w`/mode-change/`#` table
(line 59), the “what w resets / what it leaves” paragraph (lines 70–74), the
`AUX1`/`AUX2` section (replace lines 112–125), the post-`w` verification
(lines 133–135), and the v3/v4 table (line 146).  State explicitly that the
AUX selector, ODC and PPS settings are unchanged by `w`; `#`/mode change still
reset the selector and return it to `AUX` (`A0`).

### 2. [severity: blocker] `docs/protocol/dio/wiring.md` — the safety bullet still says `w` leaves the extra pins driven

**Quotes:**

> “| rails off, five signal pins parked | `w` | `Clutch disengaged!!!`, `POWER SUPPLIES OFF`; stays in `DIO>` |” (line 75)

> “Park the five signal pins with `w`, or release every I/O pin with `#`; confirm with `v` before changing the wiring.  A driven `AUX1` / `AUX2` is not released by `w`; see [park-and-reset.md](park-and-reset.md).” (lines 140–142)

**Why wrong:** on v4 `w` releases both `AUX1` and `AUX2` as well; the safety
instruction sends the reader to `#` for a pin that `w` already released.  It
also understates the park on the table row.

**Fix:** change the row to “rails off, five signal pins parked (v3) / seven
pins parked (v4)” and the safety bullet to “Park the five signal pins with
`w` on v3, or all seven (five signals plus `AUX1`/`AUX2`) on v4.  `#` releases
every I/O pin and resets the selector; confirm with `v` before rewiring.”
Remove “not released by `w`”.

### 3. [severity: major] `docs/protocol/dio/README.md` — front-page park and limits say five

**Quotes:**

> “`W` turns on the on-board `+5.0V` and `+3.3V` rails; `w` turns them off and parks the five signal pins.” (lines 42–44)

> “after `w` only the five signal pins are released, so use `#` if `AUX1` / `AUX2` may have been driven.” (lines 55–57)

> “The v4 `AUX1` / `AUX2` pins are not in the value; `w` does not release them.  Use `#` or a mode change before rewiring if `A` drove one.” (lines 112–113)

**Why wrong:** v4 `w` releases and clears both AUX pins; the README gives the
opposite safety advice on its front page and again in “Limits & out of scope”.

**Fix:** change the five-minute bullet to “parks the five signal pins on v3 and
seven pins — the five signal pins plus `AUX1`/`AUX2` — on v4”.  Change the
pre-rewiring check to “after `w` the five signal pins (v3) or all seven (v4)
are released; use `#` only when you also need the selector/bit-order/display
reset.”  Replace the limits bullet with the same v3/v4 split; remove “`w` does
not release them”.

### 4. [severity: major] `docs/protocol/dio/entry-and-setup.md` — `w` effect table and paragraph say five

**Quotes:**

> “| `w` | rails off **and the five signal pins parked** | `Clutch disengaged!!!` then `POWER SUPPLIES OFF`; stays in `DIO>` |” (line 124)

> “`w` runs the mode's safe park, which releases the five signal pins before the rails go off.  The full contract, including the `w` versus `#` difference and the `AUX1` / `AUX2` exception, is in [park-and-reset.md](park-and-reset.md).” (lines 133–135)

> “| `W` / `w` | same | same |” (line 149)

**Why wrong:** the `AUX1`/`AUX2` “exception” no longer exists; the park is
five pins on v3 and seven on v4.  The v3/v4 row saying `W`/`w` are “same” hides
the changed `w` scope.

**Fix:** update the table row to “rails off and the v3 five / v4 seven signal
pins parked”; change the paragraph to “releases the five signal pins (and, on
v4, `AUX1`/`AUX2`) before the rails go off” and remove the exception wording.
Change the v3/v4 row to “`W` same; `w` park scope 5 / 7”.

### 5. [severity: major] `docs/protocol/dio/edge-cases.md` — a whole edge case now documents the wrong outcome

**Quotes:**

> “| an extra pin stays driven | `w` after `k` / `K` then `A` | [`w` leaves `AUX1` / `AUX2` driven](#w-leaves-aux1--aux2-driven-v4) |” (line 27)

> “`w` parks the five signal pins and resets the width…” (line 84)

> “`#` (or a mode change) if `AUX1` / `AUX2` may be driven; `w` releases only the five signal pins if that is all you need.” (line 86)

> “### `w` leaves `AUX1` / `AUX2` driven (v4)” (line 89)

> “Symptom: the selected extra pin stays driven while the five direct I/O pins are released.” (lines 92–93)

> “Behaviour: the park covers the five signal pins only; `AUX1` / `AUX2` are not in the value or the park mask.” (lines 94–95)

**Why wrong:** `w` now releases `AUX1` and `AUX2` too.  The symptom-index row,
the recovery in the menu-cancel row, and the entire dedicated section describe
a failure that no longer occurs.

**Fix:** remove the symptom-index row and the “`w` leaves `AUX1` / `AUX2`
driven” section, or replace it with a short note that v4 `w` releases both
`AUX1`/`AUX2` regardless of the selected target.  Change line 84 to “`w` parks
the five signal pins (v3) or seven pins (v4) and resets the width”.  Remove the
`#`-for-AUX1/AUX2 advice from line 86.

### 6. [severity: minor] `docs/protocol/dio/aux-pin.md` — v3/v4 table says `w` does not release an AUX1/AUX2 target

**Quote:**

> “| `w` releases a driven `AUX1` / `AUX2` target | — | no |” (line 143)

**Why wrong:** on v4 `w` releases both `AUX1` and `AUX2` (direction input,
latch cleared), whether or not that target is selected.  The v3 cell stays
“—”; the v4 cell must be “yes”.

**Fix:** change the v4 cell to “yes (both, regardless of the selected target)”.
The surrounding statement that selecting `AUX2` does not release a previously
driven `AUX1` (lines 115–116) remains true; only the `w` row is stale.

### 7. [severity: minor] `docs/protocol/dio/pin-control.md` — value reference still says five

**Quote:**

> “| `W` / `w` | supply on / park and off | `w` parks the five signal pins; see [park-and-reset.md](park-and-reset.md) |” (line 174)

**Why wrong:** same v3/v4 split; the reference row is the quick lookup a user
will trust.

**Fix:** “`w` parks the five signal pins (v3) or seven pins including
`AUX1`/`AUX2` (v4); see [park-and-reset.md](park-and-reset.md)”.

## Cross-set list

- **[major] `docs/dev/verification.md:42`** — stale verification row:
  “the safe-park contract covers the five signal pins, not `AUX1`/`AUX2`;
  v3 unverified”.  Update to “five signal pins on v3; seven including
  `AUX1`/`AUX2` on v4; v3 unverified”.
- **[checked, correct] `docs/capabilities/auxiliary/wiring.md:147–149`** —
  already says DIO parks “five signal pins and, on v4, `AUX1` and `AUX2` —
  seven in total”; no change needed.  This is the cross-ref the DIO set should
  match.
- **[checked, no contradiction] `docs/capabilities/auxiliary/aux-pin.md:167`**
  — the general “`w` is not a board reset; release depends on the active
  mode's cleanup” wording remains true; no DIO-specific five-pin claim.
- **[checked, no contradiction] `docs/capabilities/auxiliary/edge-cases.md`**
  — its `w`/generator entries do not state the DIO park width.

## Presentation and scope

- Changed passages contain no TAB characters, no dates, no bench/test prose,
  no pass counts, and no new wide-capture issue; the affected material is
  prose and existing pipe tables.
- The `w` versus `#` distinction, the v3/v4 split, and the selector/ODC/PPS
  limits are the only behaviour changed by RA-7; this audit did not reopen the
  rest of the DIO set.
- Finding count: **7 DIO findings** (2 blockers, 3 majors, 2 minors) plus
  **1 cross-set finding** (`docs/dev/verification.md`).
