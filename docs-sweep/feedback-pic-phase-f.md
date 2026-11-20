# PIC — Phase F delta audit (RA-19)

Scope: only the statements the #234 RA-19 repair changed — the malformed
one-line setup (`m 9 2`), the fallback to both prompts, and the no-partial-
setting rule.  Verified against the docs worktree's final tree
(`docs/combined-sweep@fe7da6d2`, `Firmware/` combined `9b179cb5`):
`Firmware/programmer_pic/pic.c` `pic_setup_prepare()` treats a parse error as
interactive (`mode_configuration.command_error || …`), and
`Firmware/tests/test_pic_setup.py` asserts two prompts, zero headers and the
default `6b/14b`, 1 ms state for either missing operand.  No other PIC content
was re-opened.

## Verdict

**Yes with fixes.**  The code and the generic layer already use the repaired
behaviour, but the two PIC pages that own the trap still teach the pre-fix
partial apply — `m 9 2` is documented as running 4b/16b at 0 ms and then
reporting a syntax error.  Two major findings, one minor wording item.

## Findings

1. **[major]** `entry-and-setup.md:90-93` —
   "A missing delay is read as `0`, which is in range: `m 9 2` runs 4b/16b at
   0 ms, then the terminal reports a syntax error.  Only an invalid width
   (including no width) or a delay outside `0`–`3` falls back to the two
   prompts.  The full trap is in [edge-cases.md](edge-cases.md)."
   **Why wrong:** This is the pre-RA-19 behaviour.  In the final tree a missing
   operand is a parse error, so `interactive` is true; the mode prompts for
   **both** width and delay, applies nothing from the typed `2`, prints no
   settings header and no `Syntax error` line.  The paragraph also tells the
   reader that a missing delay is *not* a fallback case, which is the reverse
   of the repair.
   **Fix:** Replace the paragraph with: "A complete pair is the only form that
   skips the prompts.  An incomplete pair (`m 9 2`, missing delay) or any
   value outside the ranges is a parse error: both prompts run from the top,
   nothing from the partial line is applied, there is no settings header and
   no `Syntax error` line.  `m 9 2` therefore asks `Commandmode?` and `Delay?`
   and uses your answers.  A complete pair (`m 9 2 1`) is unchanged: it is
   applied and echoed.  See [edge-cases.md](edge-cases.md)."  Keep the prose
   at ≤80 columns.

2. **[major]** `edge-cases.md:52` and `:60-66` —
   "| `m 9 2` (one number) | the header and a syntax error | the missing delay
   is read as 0 and applied; give both numbers |" and the transcript:
   "`HiZ>m 9 2` / `PIC(mod dly)=(4 0)` / `Ready` / `Syntax error at char 6` /
   `PIC>`", followed by "The mode is 4b/16b at 0 ms, not the interactive
   defaults."
   **Why wrong:** There is no header, no `Ready` before the answers and no
   `Syntax error at char 6`.  The final code runs both prompts and uses the
   answers; the interactive defaults are 6b/14b and 1 ms.  A user following
   this transcript would expect the partial `2` to take effect and would
   misread the two prompts as a failure.
   **Fix:** Change the row to "| `m 9 2` (one number) | the two prompts run; no
   settings header | the pair is incomplete, so nothing is applied; answer
   both prompts, or give both numbers (`m 9 2 1`) |".  Replace the block with:
   "The incomplete-pair fallback in full:" followed by
   ```
   HiZ>m 9 2
   Commandmode?
   1. 6b/14b
   2. 4b/16b
   (1)>1
   Delay?
   (1)>1
   Ready
   PIC>
   ```
   and the note: "Nothing from the typed `2` is applied.  After the two
   answers the mode runs 6b/14b at 1 ms; leaving both prompts at Enter gives
   the same defaults.  Give both numbers (`m 9 2 1`) to skip the prompts."
   Keep the transcript space-aligned (no TABs).

3. **[minor]** `edge-cases.md:58` —
   "| One-line form not applied | the prompts run instead | check the two
   numbers; a missing width is the usual cause |".
   **Why incomplete:** After RA-19 a missing **delay** is just as direct a
   cause as a missing width (`m 9 2`).  The row is otherwise correct (it says
   the prompts run), but the cause list now points only at the width.
   **Fix:** "check both numbers; a missing or out-of-range width **or delay**
   triggers the fallback".  Keep the row in the table's existing width.

## Cross-set list

- **No stale cross-set claim found.**  The generic layer already states the
  repaired rule and is the authority the PIC pages should echo:
  - `docs/generic/navigation.md:203` — "a complete answer set skips the
    prompts … an incomplete set falls back to the prompts."
  - `docs/generic/edge-cases.md:85` — "incomplete inline answers … the mode's
    setup prompts start from the top."
  No edit is needed to either page; link one of them from the corrected
  `entry-and-setup.md` paragraph if a shared statement is wanted.
- `docs/programmers/pic/README.md:42` and `docs/programmers/pic/sessions.md:85`
  show only the valid forms (`m9` / `m 9 2 1`); no change.
- A search over `docs/` for `m 9 2`, "missing delay" and the partial-apply
  wording found the stale text only in the two PIC pages above.

## Presentation (changed passages only)

- No TAB characters in the affected passages; no dates, bench/run prose or
  pass counts.
- `entry-and-setup.md:90-93` is prose at 70–73 columns; the replacement should
  stay under 80.
- `edge-cases.md:52` is a 120-column table row; the page's table already runs
  98–115 columns, so this is consistent but at the wide end — do not let the
  corrected row grow, and prefer shortening the symptom/recovery cells.
- The corrected `edge-cases.md` transcript is a code block and must stay
  space-aligned, as the existing captures are.
