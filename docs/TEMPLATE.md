# Documentation template — minimum structure for every function page set

**Status:** draft, 2026-11-01 — this is the **minimum** every function's
documentation set must cover.  Writers (agents or humans) may expand it; they
must never go below it.  Governing contract: CodeGuidelines
`project_guidelines/documentation_guidelines.md` — read it **in full** before
writing, every time.

**Shape (user decision 2026-11-01):** the **README is a clean overview + index**
— details never live in it.  Every layer of detail goes into **dedicated,
parallel files** inside the function folder.  Nobody reads a 600-line README.

## What to produce
- `docs/<function>/README.md` — **clean front door** (target ≤ ~120 lines):
  what the function is, availability summary, wiring headline, a five-minute
  quick start, limits, and a **link table to the detail files**.  It carries
  the short form of the required sections and points at the long form.
- `docs/<function>/<topic>.md` — one **dedicated file per topic / sub-function**
  (examples: `navigation` vs `commands` vs `usage`; or per sub-function:
  `hduart`, `extended`).  Each detail file is written at full standard and
  owns its substance.
- Shared files allowed per folder: `verification.md` (all evidence in one
  place).  **No `references.md` in user folders** — maintainer code maps live
  in `docs/dev/`.

## Minimum sections (the document SET must cover them all)
1. **Scope** — what is covered and what is not; who the reader is.
2. **Availability** — v3 / v4, default build or optional, hardware needed.
   State plainly when the bench cannot confirm behavior.  (README: summary;
   detail file: the full table.)
3. **Wiring** — one line per wire, pins named.  **Always state explicitly
   whether a pull-up / supply is required** ("pull-up 1–10 kΩ to 3.3 V
   required" or "no pull-up needed").  If unknown: `Requires verification` —
   never imply.  (README: headline; detail file: the full table.)
4. **Entry & setup** — how the function is entered (menu number, binary entry
   byte, hidden sequence), the settings menu with its defaults.
5. **Usage** — realistic terminal sessions: exact commands → exact replies.
   Shared CLI grammar (`r`, `W`, `P`, wrappers, macros) is **not** repeated —
   cite `docs/generic/` and lean on it.
6. **Command / macro reference** — table: input → action → notes (v3/v4
   deltas marked).
7. **Binary protocol** (if any) — byte values, framing, one worked exchange.
8. **Sub-functions** — one section or dedicated file each, with usage + limits.
9. **Edge cases & failure modes** — concrete: trigger → symptom → behavior →
   recovery; cite `Firmware/<file>:<line>` and/or test/log names.
10. **Verification** — what was proven, and how: bench battery + log file
    name, native test, or `Required, not run` + reason.  v3 is build-only on
    this bench — say that instead of implying it was tested.
11. **Limitations & out of scope** — known limits, unsupported cases, `tbi`.
12. **Related docs** — links to the pages this set leans on (`docs/generic/`,
    `docs/SPEC.md`, sibling functions).  **User documents carry NO code
    references** — no source files, no line numbers, no symbol names;
    maintainer code maps live in `docs/dev/`.  Point-in-time line refs are
    fine only in local audit records (`docs-sweep\*`).

## Rules (the contract, applied to this repo)
- Confirm behavior from `Firmware/` sources before writing; label statements
  `Verified` / `Assumption` / `Unknown` / `Requires verification` when not
  self-evident.
- No marketing language, no placeholders, no invented commands, pins, or
  menus.
- Exact paths and names; tables over prose for mappings.
- **User docs are human docs:** no `Firmware/` files, no line numbers, no
  symbol names.  Code maps for maintainers live in `docs/dev/` (file +
  symbol; no line numbers); local audit records may use file:line.
- The set is the single source for its function — stale claims are removed,
  not duplicated.
- The CodeGuidelines §13 quality gate must pass before the set is called
  complete.

## README skeleton (copy, then fill — keep it short)
```md
# <Function>

**Status:** … · one-line what it is.

This README is a map, not the manual; details live in the files below.

| File | Covers |
|---|---|
| [<topic>.md](<topic>.md) | … |
| [<topic2>.md](<topic2>.md) | … |
| [verification.md](verification.md) | bench evidence / not verified |
| [references.md](references.md) | source map |

## The five-minute version
- … (≤ 6 bullets: entry, key commands, wiring headline)

## Availability (summary)
| | v3 | v4 |
|---|---|---|
| build | … | … |
| hardware | … | … |

## Limits & out of scope
- …
```