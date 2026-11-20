# Documentation template — minimum structure for every set

**Status:** current template.  This is the **minimum** every documentation set
must cover.  Writers (agents or humans) may expand it; they must never go below
it.  Read the repository documentation guidelines in full before writing, every
time.

**Shape:** the **README is a clean overview + index** — details never live in
it.  Every layer of detail goes into **dedicated, parallel files** inside the
set folder.  Nobody reads a 600-line README.

## Where a set lives

A set is its folder under `docs/`: the generic set is `docs/generic/`; the
other user-facing clusters are `protocol/`, `tools/`, `capabilities/` and
`programmers/`, and the maintainer sets are `core/` and `dev/`.  Nested
clusters keep their parent, so grouped sets use `docs/<cluster>/<set>/` — for
example `docs/protocol/i2c/`, `docs/protocol/wire/1wire/`,
`docs/programmers/pic/`.  `docs/assets/` and `docs/refcard/` hold repository
material, not documentation sets.  The folder name is the set's name in the
tree; the function and capability index is [docs/SPEC.md](SPEC.md).

## What to produce
- The set's `README.md` — **clean front door** (target ≤ ~150 lines; the
  generic base layer may run to ~170): what the set covers, availability
  summary, wiring headline, a five-minute quick start, limits, and a **link
  table to the detail files**.  It carries the short form of the required
  sections and points at the long form.
- The set's `<topic>.md` — one **dedicated file per topic / sub-function**
  (examples: `navigation` vs `commands` vs `usage`; or per sub-function:
  `hduart`, `sniffer`).  Each detail file is written at full standard and
  owns its substance.
- **No `verification.md` or `references.md` in user sets** — maintainer
  records (evidence, code maps) live in `docs/dev/`.

## Minimum sections (the document SET must cover them all)
1. **Scope** — what is covered and what is not; who the reader is.
2. **Availability** — v3 / v4, default build or optional, hardware needed.
   State plainly where a build is not verified.  (README: summary;
   detail file: the full table.)
3. **Wiring** — one line per wire, pins named.  **Always state explicitly
   whether a pull-up / supply is required** ("pull-up 1–10 kΩ to 3.3 V
   required" or "no pull-up needed").  If unknown: `Requires verification` —
   never imply.  (README: headline; detail file: the full table.)
4. **Entry & setup** — how the function is entered (menu number, binary entry
   byte, hidden sequence), the settings menu with its defaults, if any.
5. **Usage** — realistic terminal sessions: exact commands → exact replies.
   Shared CLI grammar (`r`, `W`, `P`, wrappers, macros) is **not** repeated —
   cite `docs/generic/` and lean on it.
6. **Command / macro reference** — table: input → action → notes (v3/v4
   deltas marked).
7. **Binary protocol** (if any) — byte values, framing, one worked exchange.
8. **Sub-functions** — one section or dedicated file each, with usage + limits.
9. **Edge cases & failure modes** — concrete: trigger → symptom → behavior →
   recovery.  Maintainer evidence (code refs, logs) lives in `docs/dev/` —
   never in user pages.
10. **Verification basis** — a user set has **no verification page**; no
    bench prose, no dates, no tested/verified-result prose in user docs: no
    bench/rig/battery/capture narratives, no run or test dates (dated status
    lines included), no pass counts, no "tested"/"verified" claims, no
    "captured on…" provenance, no evidence sections.  Maintainer summaries
    live in `docs/dev/` / the development workspace.  User docs keep at most
    a short inline caveat where it changes what the reader can rely on — e.g.
    "not verified", "unverified".
11. **Limitations & out of scope** — known limits, unsupported cases, `tbi`.
12. **Related docs** — links to the pages this set leans on (`docs/generic/`,
    `docs/SPEC.md`, sibling sets).  **User documents carry NO code
    references** — no firmware source files, no line numbers, no symbol names;
    maintainer code maps live in `docs/dev/`.  Point-in-time line refs are
    fine only in local audit records.

## Rules (the contract, applied to this repo)
- Confirm behavior from the firmware sources before writing; label
  uncertainty with short `unverified` / `Requires verification` caveats where
  it is not self-evident — never with bench-result reporting.
- **Naming/mapping contract:** the firmware's own displayed strings are the
  authority for names.  Where the firmware uses two forms for the same item
  (e.g. a `?`-help token vs a status-print label such as AUX targets
  `A0`/`A1`/`A2` vs pins printed `AUX`/`AUX1`/`AUX2`), state the mapping
  explicitly once and use it consistently; never invent names or silently
  mix forms.
- No marketing language, no placeholders, no invented commands, pins, or
  menus.
- Exact paths and names; tables over prose for mappings.
- **User docs are human docs:** no firmware source files, no line numbers, no
  symbol names.  Code maps for maintainers live in `docs/dev/` (file + symbol;
  no line numbers).
- **Presentation:** **no TAB characters** — captures are transcribed
  space-aligned (monospace-safe) or as tables; menus / option lists / result
  chains are tables, not bullet chains; prose wraps at ~80 columns; garbled
  capture artifacts get a footnote.  **No bench prose, no dates, no
  tested/verified-result prose** — no bench/rig/capture narratives, no run or
  test dates, no pass counts, no evidence sections, capture provenance or
  internal log/test-file names; examples are presented simply as examples.
- The set is the single source for its function — stale claims are removed,
  not duplicated.
- The documentation quality gate must pass before the set is called complete.

## README skeleton (copy, then fill — keep it short)
```md
# <Set>

**Status:** … · one-line what it is.

This README is a map, not the manual; details live in the files below.

| File | Covers |
|---|---|
| [<topic>.md](<topic>.md) | … |
| [<topic2>.md](<topic2>.md) | … |

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
