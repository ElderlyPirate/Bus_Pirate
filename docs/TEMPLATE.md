# Documentation template — minimum structure for every function page

**Status:** draft, 2026-11-01 — this is the **minimum** every function's
documentation must cover.  Writers (agents or humans) may expand it; they must
never go below it.  Governing contract: CodeGuidelines
`project_guidelines/documentation_guidelines.md` — read it **in full** before
writing, every time.

## What to produce
- `docs/<function>/README.md` — **required**: the end-to-end page for the
  function (what it is, wiring, entry, usage, reference, edge cases,
  verification, limits).
- `docs/<function>/<subfunction>.md` — one dedicated file per sub-function
  where separation helps (examples: `uart/hduart.md`, `sump/extended.md`,
  `avrisp/isp.md`).  Same standard, same sections.

## Minimum sections (in this order, when applicable)
1. **Scope** — what is covered and what is not; who the reader is.
2. **Availability** — v3 / v4, default build or optional, hardware needed.
   State plainly when the bench cannot confirm behavior.
3. **Wiring** — one line per wire, pins named.  **Always state explicitly
   whether a pull-up / supply is required** ("pull-up 1–10 kΩ to 3.3 V
   required" or "no pull-up needed").  If it is not known, mark
   `Requires verification` — never imply.
4. **Entry & setup** — how the function is entered (menu number, binary entry
   byte, hidden sequence), the settings menu with its defaults.
5. **Usage** — realistic terminal sessions: exact commands → exact replies.
   Shared CLI grammar (`r`, `W`, `P`, wrappers, macros) is **not** repeated —
   cite `docs/generic/` and lean on it.
6. **Command / macro reference** — table: input → action → notes (v3/v4
   deltas marked).
7. **Binary protocol** (if any) — byte values, framing, one worked exchange.
8. **Sub-functions** — one `##` section each with usage + limits (or their own
   dedicated file when large).
9. **Edge cases & failure modes** — concrete: trigger → symptom → behavior →
   recovery; cite `Firmware/<file>:<line>` and/or test/log names.
10. **Verification** — what was proven, and how: bench battery + log file
    name, native test, or `Required, not run` + reason.  v3 is build-only on
    this bench — say that instead of implying it was tested.
11. **Limitations & out of scope** — known limits, unsupported cases, `tbi`.
12. **Authority & references** — source files with line refs, related docs,
    and the `docs/SPEC.md` row(s) this page expands.

## Rules (the contract, applied to this repo)
- Confirm behavior from `Firmware/` sources before writing; label statements
  `Verified` / `Assumption` / `Unknown` / `Requires verification` when not
  self-evident.
- No marketing language, no placeholders, no invented commands, pins, or
  menus.
- Exact paths and names; tables over prose for mappings.
- This page is the single source for its function — stale claims are removed,
  not duplicated.
- The CodeGuidelines §13 quality gate must pass before the page is called
  complete.

## README skeleton (copy, then fill)
```md
# <Function>

## Scope
## Availability
| | v3 | v4 |
|---|---|---|
| build | … | … |
| hardware | … | … |

## Wiring
## Entry & setup
## Usage
## Command reference
## Edge cases
## Verification
## Limitations
## References
```