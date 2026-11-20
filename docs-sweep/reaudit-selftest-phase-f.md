# Self-test — Phase F re-audit (RA-10 topology)

**Scope:** the six Phase F findings against the current `selftest` set at
`91b560e7` (working-tree edits, no commit).  The cross-set core page is checked
only for status.  Read only; no edits.

**Verdict line: 6 of 6 filled in full.**  The selftest set is clean for the
RA-10 claim.  The one cross-set `core/system.md` W-rail item remains for the
main session.

## Per-item status

| # | Status | Evidence (current) | Remainder |
|---|---|---|---|
| 1 | **Filled in full** | `checks.md:122`: "reads the switched 3.3 V output (header `+3.3V`) through a divider, measured against the internal AVdd reference; a branch-present and gross-droop check"; failure cell: "near zero: switched output off or divider/channel fault; a small droop may stay inside the window or in divider/reference tolerance; the check cannot verify AVdd" | None in content. The row is now 329 characters (the widest in the set) but remains a 3-column table cell; see the presentation note. |
| 2 | **Filled in full** | `checks.md:129-130`: "The `3.3V` channel measures the switched 3.3 V output against the internal AVdd reference: it is a branch-present and gross-droop check, not a check of the AVdd rail." | None. The preceding generic near-zero sentence is unchanged, as allowed. |
| 3 | **Filled in full** | `checks.md:19`: "the switched 3.3 V output (header `+3.3V`); `3.3V VPU` is its pull-up source (v4)" | None. The Names table keeps its 3 columns. |
| 4 | **Filled in full** | `wiring.md:59`: "measures the 5V rail and the switched 3.3 V output, and switches them off at the end." | None. |
| 5 | **Filled in full** | `edge-cases.md:143`: "### v4 switched 3.3 V output off"; `:147`: "seven failures from the one switched branch: the `3.3V` check, the `3.3V VPU` source, the `ADC` jumper and the four `Bus Hi-Z 1` checks." | None. The per-case heading + Symptom/Behaviour/Recovery shape is unchanged. |
| 6 | **Filled in full** | `terminal.md:160`: "if `3.3V(...)` also fails, suspect the switched 3.3 V output" | The optional finding is applied. |

**Cross-set status (not part of the 6):**

| Item | Status | Evidence | Remainder |
|---|---|---|---|
| `docs/core/system.md:457-459` W-rail | **Not addressed (by design)** | "`switch_psu_on()` reads `BP_ADC_3V3` and `BP_ADC_5V0` after a 2 ms settle and requires both above `V33L`/`V5L`; otherwise it disables the regulator and reports a too-low VREG." | Owned by `core`; fix recorded in `fixes-selftest-phase-f.md` for the main session. Not counted in 6/6. |

**Adjacent checks:**

- `checks.md:123` (`3.3V VPU`) now ends "suspect the switched 3.3 V output" —
  aligned with finding 6, as the audit's adjacent note suggested.  The `5V VPU`
  row at `:120` correctly keeps "suspect the rail" because that case is the 5 V
  rail.
- `wiring.md:79` still says "a fitted `ADC`→`+3.3V` jumper is a plain wire to
  the 3.3 V rail."  The audit gated this on editing that sentence; the fix
  record left it, and it describes the physical header wire rather than a
  measurement claim.  No issue.

## Verification against the final Firmware tree

The final tree still supports the corrected model:

- `Firmware/hardwarev4.h:162-170` — `BP_ADC_3V3 0`,
  `BP_ADC_3V3_CFG = AD1PCFGbits.PCFG1`, commented `AN0/ADC3/33V`.
- `Firmware/core_system/base.c:251-253` — `AD1CON2 = 0x0000`; the register
  comment states `VCFG: VR+ is AVdd and VR- is AVss`.
- `Firmware/capability_selftest/selftest.c:224` —
  `perform_adc_test(BP_ADC_3V3, V33L, V33H)`.

The recovered-schematic split (VR1/3V3 feeding PIC VDD/AVDD; VR2/SWV33 feeding
ADC3 through RN3C) is the accepted ADC-page fact, and the new selftest wording
matches `adc/probe.md:82` and `:111-122`.

## Presentation

- **Tabs:** 0 in all six `selftest` files.
- **Table columns:** consistent.  The Names and supply/probe tables are 3
  columns; the check matrix is 5; the terminal failure table is 3.  No column
  drift in the changed rows.
- **~80-column prose:** `checks.md:129-131` is 74/76/67 characters;
  `wiring.md:59` is within 80; the `edge-cases.md` bullets are within 80.  The
  `checks.md:122` row is 329 characters, but it is a table cell, not a prose
  wrap; the task's ~80 rule is prose.  It is the widest row in the set.  If the
  set's wide-row policy matters, move the "small droop ... cannot verify AVdd"
  limit into the paragraph and keep the row cells short; this is cosmetic, not
  a content error.
- **Links/anchors:** a link and anchor check over the six files resolves every
  relative link and anchor (0 broken).
- **Dates / bench / pass counts:** none in the changed passages.  The existing
  "not verified on v3" caveat is unchanged and is the allowed inline kind.  No
  capture provenance, run dates, pass counts, or internal log names.

## Regression check

No content regression.  The `checks.md:122` row grew from 246 to 329 characters
because the exact audit fix was applied; the model is now correct, the table
still has 3 columns, and the failure cell states the real limit.  The
`checks.md:123` adjacent row was aligned, and the `5V VPU` row was correctly
left alone.  The core page was not edited, as the fix record states.

**Final: 6 of 6 filled in full.  The selftest set is clean; the cross-set
`core/system.md` W-rail item remains for the main session.**
