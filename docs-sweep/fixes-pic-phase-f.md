# PIC — Phase F repair (RA-19 delta audit)

Repairs to `docs/programmers/pic/` against
`docs-sweep/feedback-pic-phase-f.md` (2 major + 1 minor).  Scope kept to the
three findings; no unrelated PIC content reopened.  Verified against the docs
worktree's final tree (`docs/combined-sweep@fe7da6d2`, `Firmware/` combined
`9b179cb5`).  No commit, no push.

## Evidence used

- `Firmware/programmer_pic/pic.c` — `pic_setup_prepare()`:
  `bool interactive = mode_configuration.command_error || !(((mode > 0) && (mode <= 2)) && ((delay >= 0) && (delay <= 3)));`
  A missing operand makes `getint_with_spaces()` set `command_error`, so the
  `interactive` branch runs both prompts and the partial values are never
  stored.
- `Firmware/tests/test_pic_setup.py` — the RA-19 regression asserts, for
  either missing operand: `mode_info.opcode_bits == 0` (6b/14b),
  `mode_info.delay == 1` (1 ms), `command_error == NO`, exactly two prompts,
  zero settings headers.  It also asserts parser/state equivalence against the
  legacy path.
- Confirmed the firmware prompt strings (`Commandmode?`, `1. 6b/14b`,
  `2. 4b/16b`, `Delay?`, `(1)>`) are unchanged, so the corrected transcript
  is exact.

## Findings

### 1 — [major] malformed one-line setup must fall back to both prompts — fixed

**File:** `docs/programmers/pic/entry-and-setup.md`

**Decision:** fixed; replaced the pre-fix paragraph.

**Quote (new):**

> A complete pair is the only form that skips the prompts.  An incomplete pair
> (`m 9 2`, missing delay) or any value outside the ranges is a parse error:
> both prompts run from the top, nothing from the partial line is applied,
> there is no settings header and no `Syntax error` line.  `m 9 2` therefore
> asks `Commandmode?` and `Delay?` and uses your answers.  A complete pair
> (`m 9 2 1`) is unchanged: it is applied and echoed.  See
> [edge-cases.md](edge-cases.md).

The old "missing delay is read as `0` … runs 4b/16b at 0 ms … syntax error"
claim is gone.

### 2 — [major] edge-case row and transcript teach the partial apply — fixed

**File:** `docs/programmers/pic/edge-cases.md`

**Decision:** fixed; replaced the row, the transcript and the note.

**Quote (new row):**

> | `m 9 2` (one number) | both prompts run; no settings header | nothing is applied; answer both, or give `m 9 2 1` |

**Quote (new block):**

> The incomplete-pair fallback in full:
>
> ```
> HiZ>m 9 2
> Commandmode?
> 1. 6b/14b
> 2. 4b/16b
> (1)>1
> Delay?
> (1)>1
> Ready
> PIC>
> ```
>
> Nothing from the typed `2` is applied.  After the two answers the mode runs
> 6b/14b at 1 ms; leaving both prompts at Enter gives the same defaults.  Give
> both numbers (`m 9 2 1`) to skip the prompts.

No header, no `Ready` before the answers and no `Syntax error at char 6`
remain.  The transcript is space-aligned / TAB-free.

### 3 — [minor] cause list pointed only at the missing width — fixed

**File:** `docs/programmers/pic/edge-cases.md`

**Decision:** fixed; corrected the row's cause list.

**Quote (new):**

> | One-line form not applied | the prompts run instead | check both; a missing or invalid width or delay triggers it |

The row names both operands.  ("invalid" is used for
missing-or-out-of-range so the row stays within the table's width.)

## Cross-set / presentation

- No cross-set edit needed: `docs/generic/navigation.md` and
  `docs/generic/edge-cases.md` already state the repaired incomplete-set
  fallback; `README.md` and `sessions.md` show only valid forms.
- No TAB characters in the changed passages; no dates, bench/provenance or
  pass counts.
- Changed prose is 76 columns max; the edited table rows are 116 and 117
  columns (the page's table already runs 98–115, and the corrected rows did
  not grow beyond the old 120-column row).
- Links/anchors resolve; `[edge-cases.md](edge-cases.md)` was already valid.

## Files changed

- `docs/programmers/pic/entry-and-setup.md`
- `docs/programmers/pic/edge-cases.md`
