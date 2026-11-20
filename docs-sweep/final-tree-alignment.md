# Final docs-tree alignment — 2026-11-19

Worktree `C:\Users\jschl\dev\bpv4-docssweep`, branch `docs/combined-sweep`,
tip `729939b`.  Scope: verify the accepted tree against
`WORKLOG-DOCUMENTATION.md` §6 (target tree + source→docs mapping); fix only
navigation/path/mapping issues.  No commit, no push.

## Checks run

- Relative links over `docs/**/*.md` + root `README.md`: **200 files,
  2343 links, 0 missing**, 0 case mismatches (Windows case check), 0 broken
  page or same-page anchors.
- Reachability from root `README.md`: **199/199 docs pages reachable**.
- Map coverage: every non-README page is listed in its folder `README.md`;
  22 cluster set READMEs present.
- Source mapping: all 26 source groups / build+tests map to a docs location
  (`core/`, `protocol/*`, `tools/*`, `capabilities/*`, `programmers/*`,
  `dev/`); no gaps.  `generic/`, `assets/`, `refcard/` are the user-facing /
  migrated layers.
- Target tree: all 9 protocol, 3 tool, 5 capability, 2 programmer, 5 core and
  the generic/dev sets are present with the §6 names (`auxiliary/`, `lcd/`,
  `selftest/`, `wire/` children).
- No `Documentation/` or `docs-sweep/` references anywhere in the docs tree;
  no `Firmware/` or source-file references in user sets.
- Boundaries checked: JTAG owns the TAP/in-mode protocol; XSVF and OpenOCD
  are tool sets riding it; BASIC is a tool; capability sets own their
  hardware.  Parallel protocol/tool detail pages are the accepted
  audience-based views (XSVF audit precedent), now cross-linked both ways.
- Pre-existing working-tree content edits (`docs/programmers/avrisp/license.md`,
  `docs/protocol/jtag/edge-cases.md`, `docs/protocol/jtag/xsvf.md`) were not
  touched.

## Issues found → fix applied

| # | Issue | Fix (file) |
|---|---|---|
| 1 | `docs/TEMPLATE.md` was the only unreachable page (no inbound link) | added the template bullet to the Documentation section of `README.md` |
| 2 | `docs/protocol/binary/sub-protocols.md` linked `commands.md#wait-for-an-edge`; the heading is `Wait for an edge (0x70)` → anchor `wait-for-an-edge-0x70` | corrected the target and rewrapped the sentence (`docs/protocol/binary/sub-protocols.md`) |
| 3 | Same stale anchor used as a same-page link in the command map | corrected `#wait-for-an-edge` → `#wait-for-an-edge-0x70` (`docs/protocol/binary/commands.md`) |
| 4 | `docs/tools/openocd/README.md` cross-linked the JTAG TAP/entry pages but not the JTAG set's OpenOCD byte-protocol page (the XSVF set does link its own) | added the `protocol/jtag/openocd.md` row to Related docs (`docs/tools/openocd/README.md`) |
| 5 | DIO said the AUX/ADC controls were "documented in their own sets" but linked neither, leaving the capability boundary one-way | added AUX and ADC capability links to Related docs (`docs/protocol/dio/README.md`) |

## Remaining

- One stale pointer outside the docs tree: `Firmware/configuration.h:275`
  says `see Documentation/avrisp_mkii.md` (retired folder).  Not edited —
  firmware source is outside this docs-tree pass; the docs side points at
  `docs/programmers/avrisp/`.
- `docs/protocol/wire/README.md` is the protocol_wire parent index, reachable
  from its three child sets (not linked from SPEC, which links the three
  child sets directly).  Intentional; no action.
- Audience-based protocol/tool parallel pages (JTAG ↔ XSVF/OpenOCD,
  binary ↔ capability fronts, DIO ↔ AUX) are cross-linked, not merged.

**Tree aligned (5 fixes in 5 files). Remaining question: whether to clean the
one stale `Documentation/` comment in `Firmware/configuration.h` in a firmware
change; there are no open docs-tree mapping questions.**
