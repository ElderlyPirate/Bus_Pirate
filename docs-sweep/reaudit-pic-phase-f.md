# PIC — Phase F re-audit (RA-19)

Re-audit of the 3 findings in `feedback-pic-phase-f.md` against the current
`docs/programmers/pic/` (uncommitted repair), with the final tree
`docs/combined-sweep@fe7da6d2` / `Firmware/` combined `9b179cb5` as the
behaviour authority.  Only the changed passages were checked.

| # | Finding (from the delta audit) | Verdict | Evidence |
|---|---|---|---|
| 1 | `entry-and-setup.md:90-93` — old claim: missing delay read as `0`, `m 9 2` runs 4b/16b at 0 ms, then a syntax error; only invalid width/delay fall back | **Filled in full** | Now: "A complete pair is the only form that skips the prompts.  An incomplete pair (`m 9 2`, missing delay) or any value outside the ranges is a parse error: both prompts run from the top, nothing from the partial line is applied, there is no settings header and no `Syntax error` line.  `m 9 2` therefore asks `Commandmode?` and `Delay?` and uses your answers.  A complete pair (`m 9 2 1`) is unchanged: it is applied and echoed."  No partial apply, both prompts, and the valid form is explicitly unchanged. |
| 2 | `edge-cases.md:52,60-66` — row said "the header and a syntax error"; transcript showed `PIC(mod dly)=(4 0)`, `Ready`, `Syntax error at char 6` | **Filled in full** | Row now: "both prompts run; no settings header \| nothing is applied; answer both, or give `m 9 2 1`".  Block now starts "The incomplete-pair fallback in full:" and shows `Commandmode?` / `1. 6b/14b` / `2. 4b/16b` / `(1)>1` / `Delay?` / `(1)>1` / `Ready` / `PIC>` — no header, no early `Ready`, no syntax-error line.  The note states "Nothing from the typed `2` is applied.  After the two answers the mode runs 6b/14b at 1 ms; leaving both prompts at Enter gives the same defaults." |
| 3 | `edge-cases.md:58` — cause list named only "a missing width" | **Filled in full** | Now: "check both; a missing or invalid width or delay triggers it" — both operands named, and "invalid" covers missing/out-of-range. |

**Verdict: 3 of 3 filled in full.**  The set is clean for the RA-19 delta; no
regressions or new findings in the changed passages.

## Independent check of the behaviour

- `Firmware/programmer_pic/pic.c` — `pic_setup_prepare()`:
  `bool interactive = mode_configuration.command_error || !(((mode > 0) && (mode <= 2)) && ((delay >= 0) && (delay <= 3)));`
  A missing operand sets `command_error`, so the interactive branch runs both
  `getnumber()` prompts; the parsed partial values are not stored.
- `Firmware/tests/test_pic_setup.py` — the RA-19 regression asserts, for
  either missing operand: `opcode_bits == 0` (6b/14b), `delay == 1` (1 ms),
  `command_error == NO`, exactly two prompts, zero settings headers.  This
  matches the corrected transcript and note.

## Presentation (changed passages only)

- No TAB characters in either changed file.
- Changed prose in `entry-and-setup.md` is ≤79 columns; the `edge-cases.md`
  note is ≤76; the transcript is a space-aligned code block.
- The changed table rows are 116/117 columns — the page's table already runs
  98–115, and the old row was 120, so the rows are consistent with the page's
  established wide-table style and did not grow.
- All links in the set resolve; `[edge-cases.md](edge-cases.md)` lands.
- No dates, bench/run prose, provenance or pass counts in the changed
  passages.  The remaining `unverified` wording is outside the delta and is
  the existing allowed caveat.
- Diff contains only the intended `entry-and-setup.md` and `edge-cases.md`
  edits; no unrelated PIC content was reopened.
