# DIO — Phase F re-audit (RA-7)

Inputs: `feedback-dio-phase-f.md`, `fixes-dio-phase-f.md`, and the current
DIO pages.  Rechecked against the worktree's final combined
`Firmware/` (`9b179cb5`): `dio_cleanup()` uses
`AUX | MOSI | CLK | MISO | CS` plus `AUX1 | AUX2` under `BUSPIRATEV4`,
sets those directions to input, disables the pull-ups and v4 VPU source,
clears the matching latches, and restores 8-bit width; `switch_psu_off()`
calls it.  The AUX selector, ODC and output-routing/PPS settings are not
changed by `w`.

## Verdict

**7 of 7 filled in full** (0 partial, 0 not addressed, 0 regressed).  The
DIO set is clean.  One cross-set row remains (`docs/dev/verification.md`),
which was recorded as a handoff and not edited by this implementer.

## Per-item verdicts

| # | Finding (severity) | Status | Evidence from current docs | Remainder |
|---|---|---|---|---|
| 1 | `park-and-reset.md` five-pin contract (**blocker**) | **Filled in full** | “The mode's cleanup parks seven pins on v4 and five on v3: the five signal pins on both boards, plus `AUX1` and `AUX2` on v4.” … “`w` performs this park … On v4 the park includes both `AUX1` and `AUX2`, regardless of which AUX target is selected.” The action table says “five signal pins input (v3); seven pins input (v4)”; the `w`/mode/`#` table lists `AUX1`/`AUX2` as input for `w`; verification checks all seven. | None. |
| 2 | `wiring.md` five-pin safety text (**blocker**) | **Filled in full** | Table row: “rails off, five signal pins parked (v3) / seven pins parked (v4)”. Safety bullet: “Park the five signal pins with `w` on v3, or all seven (the five signal pins plus `AUX1` / `AUX2`) on v4.  `#` releases every I/O pin and resets the selector…” The v3/v4 table adds a `w` park-scope row: five signal pins for v3, five plus `AUX1` / `AUX2` for v4. | None. |
| 3 | `README.md` front-page park/limits (**major**) | **Filled in full** | “`w` turns them off and parks the five signal pins on v3 and all seven pins — the five plus `AUX1` / `AUX2` — on v4.” … “after `w` the five signal pins (v3) or all seven pins (v4) are released.” … “The v4 `AUX1` / `AUX2` pins are not in the value, but `w` releases them along with the five signal pins…” | None. |
| 4 | `entry-and-setup.md` table/paragraph/v3-v4 row (**major**) | **Filled in full** | `w` row: “rails off **and five pins (v3) / seven pins (v4) parked**”. Paragraph: “`w` runs the mode's safe park, which releases the five signal pins (and, on v4, `AUX1` / `AUX2`) before the rails go off.” v3/v4 row: “`W` same; `w` parks five pins” (v3) / “`W` same; `w` parks seven pins” (v4). | None. |
| 5 | `edge-cases.md` obsolete AUX1/AUX2-driven case (**major**) | **Filled in full** | The old symptom-index row is gone. The “Parking and leaving” row says “`w` parks the five signal pins (v3) or seven pins (v4) and resets the width”. The replacement section says “On v4, `w` sets both `AUX1` and `AUX2` to input and clears their latches, regardless of which AUX target is selected.” | None. |
| 6 | `aux-pin.md` v3/v4 `w` release row (**minor**) | **Filled in full** | The row now says `w` releases a driven `AUX1` / `AUX2` target on v4: “yes (both, regardless of the selected target)”. The surrounding “selecting `AUX2` does not release a previously driven `AUX1`” statement remains true for selection alone. | None. |
| 7 | `pin-control.md` value-reference `w` row (**minor**) | **Filled in full** | “`w` parks the five signal pins (v3) or seven pins including `AUX1` / `AUX2` (v4); see [park-and-reset.md](park-and-reset.md)”. | None. |

## Cross-set item (not one of the 7)

- **`docs/dev/verification.md:42` — still stale, not edited.** Quote:
  “the safe-park contract covers the five signal pins, not
  `AUX1`/`AUX2`; v3 unverified”.  Replace with “five signal pins on v3;
  seven pins including `AUX1`/`AUX2` on v4; v3 unverified”.  This is a `dev`
  pass item, not a DIO defect.
- `docs/capabilities/auxiliary/wiring.md:147–149` already says DIO parks
  five signal pins and, on v4, `AUX1`/`AUX2` — seven in total.  No
  contradiction; no edit needed.
- `docs/capabilities/auxiliary/aux-pin.md:167` and the auxiliary edge-case
  page carry no DIO-specific five-pin claim; no contradiction.

## Presentation and links

- No TAB characters in the DIO set.
- The changed tables remain pipe-aligned; changed prose wraps within the
  set's existing ~80-column style.  No dates, bench/test prose, pass counts or
  newly long unwrapped prose in the changed passages.
- Every relative link in the DIO set resolves, including
  `pin-control.md#direction-and-level-are-separate`, `wiring.md#the-five-signal-pins`,
  `aux-pin.md#selecting-the-target`, `park-and-reset.md#what-parked-means`
  and `edge-cases.md#symptom-index`.
- No new DIO defect observed.
