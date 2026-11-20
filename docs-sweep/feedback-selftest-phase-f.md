# Self-test — Phase F delta audit (RA-10 topology)

**Scope:** only the RA-10 topology claims the ADC audit flagged in the
`selftest` set.  Docs head `c15e8dd0`; the six `selftest` files are unmodified
in the worktree (not in `git status`), so these quotes are at the head.  Verified
against the final tree in `C:\Users\jschl\dev\bpv4-docssweep\Firmware\`.  Read
only; no edits.

**Basis.** `Firmware/hardwarev4.h:162-170` maps `BP_ADC_3V3` to AN0/ADC3/33V
(`BP_ADC_3V3_CFG = AD1PCFGbits.PCFG1`).  `Firmware/core_system/base.c:251-253`
sets `AD1CON2 = 0x0000` with the comment "VCFG: VR+ is AVdd and VR- is AVss".
`Firmware/capability_selftest/selftest.c:224` calls
`perform_adc_test(BP_ADC_3V3, V33L, V33H)`.  The recovered V4 schematic has
VR1/3V3 feeding PIC VDD/AVDD and VR2/SWV33 feeding ADC3 through RN3C.  The
`3.3V` channel is therefore the switched 3.3 V output measured against AVdd,
not a rail measured against itself.  The accepted ADC page already says this
(`adc/probe.md:82`, `:111-122`).

## Verdict

**No — not ready.**  The two major `3.3V` passages still teach the disproved
same-rail/ratiometric model, and the labels still call the switched output a
"rail."  The rest of the set is not reopened.

## Findings

### 1. [major] `checks.md:122` — the `3.3V` row still says the channel is ratiometric to the rail

**Quote:**

> | `3.3V` | reads the 3.3V rail channel in the 3.3V window; the channel is
> ratiometric to the rail, so it proves the divider/channel is present, not the
> absolute rail voltage | near zero: divider or channel fault; a small droop is
> not detectable |

**Why:** this is the exact RA-10 error.  The channel reads the switched 3.3 V
output (header `+3.3V`, VR2/SWV33 via RN3C) through a divider, measured
against the internal AVdd reference.  It is not ratiometric to the rail it
monitors; a small droop in the switched output is a real input change, not an
undetectable one.  It may simply stay inside the 2.8–3.8 V window or be lost
in divider/reference tolerance.  What the check cannot do is verify AVdd.
The current failure cell "a small droop is not detectable" is false by
construction and hides the real limit.  The accepted `adc/probe.md:82` says
"switched 3.3 V output, measured against AVdd; branch-present/gross-droop
check"; the selftest row contradicts it.

**Exact fix:** replace the row with:

> | `3.3V` | reads the switched 3.3 V output (header `+3.3V`) through a
> divider, measured against the internal AVdd reference; a branch-present and
> gross-droop check | near zero: switched output off or divider/channel fault;
> a small droop may stay inside the window or in divider/reference tolerance;
> the check cannot verify AVdd |

Keep the existing 3-column table; do not add a column.

### 2. [major] `checks.md:129-130` — the paragraph repeats the same model

**Quote:**

> The `3.3V` channel is ratiometric to the
> rail: it is a divider/presence check, not an absolute rail measurement.

**Why:** the same error in prose.  "Ratiometric to the rail" is the model the
ADC set removed.  The channel is measured against AVdd; it is a branch-present
and gross-droop check, not a check of the AVdd rail.  Leaving this sentence
means the set still teaches that a low `3.3V` reading is self-cancelling,
which is exactly what the check cannot prove.

**Exact fix:** replace the two lines with:

> The `3.3V` channel measures the switched 3.3 V output against the internal
> AVdd reference: it is a branch-present and gross-droop check, not a check of
> the AVdd rail.

The preceding "A reading near `0.00` on a rail" sentence can stay; it is a
generic near-zero note, not the same-rail claim.

### 3. [minor] `checks.md:19` — the Names row calls the switched output "the 3.3V rail"

**Quote:**

> | `+3.3V` | `3.3V` | the 3.3V rail; `3.3V VPU` is its pull-up source (v4) |

**Why:** the header `+3.3V` is the switched 3.3 V output, not the main 3.3 V
rail that supplies the converter (AVdd).  The Names table is the set's mapping
authority; "the 3.3V rail" keeps the two-rail ambiguity alive even after the
check row is fixed.  `adc/probe.md:100` calls the channel "3.3 V channel
(`3.3V`) ... (switched output; see below)".

**Exact fix:**

> | `+3.3V` | `3.3V` | the switched 3.3 V output (header `+3.3V`); `3.3V VPU`
> is its pull-up source (v4) |

### 4. [minor] `wiring.md:59` — "measures the 5V and 3.3V rails"

**Quote:**

> The test enables the board's regulator itself (the
> `VREG` check), measures the 5V and 3.3V rails, and switches them off at
> the end.

**Why:** the self-test measures the 5 V rail and the switched 3.3 V output.
The main 3.3 V rail/AVdd is the converter reference, not a measured channel.
"the 5V and 3.3V rails" implies the run verifies the converter supply, which
the `3.3V` check cannot do.

**Exact fix:**

> The test enables the board's regulator itself (the
> `VREG` check), measures the 5V rail and the switched 3.3 V output, and
> switches them off at the end.

### 5. [minor] `edge-cases.md:143-149` — "v4 3.3V rail off" and "one rail"

**Quote:**

> ### v4 3.3V rail off
>
> - **Symptom:** `3.3V(...) FAIL`, `3.3V VPU(...) FAIL`, `ADC(...) FAIL` and
>   the four `Bus Hi-Z 1` checks fail.
> - **Behaviour:** seven failures from the one rail: the internal checks, the
>   probe jumper and the pull-up source.
> - **Recovery:** fix the 3.3V rail, then rerun.

**Why:** the seven failures all belong to the switched 3.3 V branch: the
`3.3V` check, the `3.3V VPU` source, the `ADC` jumper to header `+3.3V`, and
the four `Bus Hi-Z 1` checks that use the 3.3 V pull-up source.  They are not
seven failures of the main AVdd rail.  "v4 3.3V rail off" and "the one rail"
keep the old model and invite a reader to think the self-test has verified the
converter supply.

**Exact fix:**

> ### v4 switched 3.3 V output off
>
> - **Symptom:** `3.3V(...) FAIL`, `3.3V VPU(...) FAIL`, `ADC(...) FAIL` and
>   the four `Bus Hi-Z 1` checks fail.
> - **Behaviour:** seven failures from the one switched branch: the `3.3V`
>   check, the `3.3V VPU` source, the `ADC` jumper and the four `Bus Hi-Z 1`
>   checks.
> - **Recovery:** fix the switched 3.3 V output, then rerun.

### 6. [minor] `terminal.md:160` — "suspect the rail" (optional adjacent wording)

**Quote:**

> | `ADC(...) FAIL` alone | the `ADC` jumper is missing or loose, or the probe
> input is faulty | fit/reseat the jumper; if `3.3V(...)` also fails, suspect
> the rail |

**Why:** this is the ADC audit's optional adjacent item.  The `3.3V` check is
the switched output, so "suspect the rail" is ambiguous between the switched
branch and AVdd.  If the `ADC` and `3.3V` checks fail together, the common
cause is the switched 3.3 V output (or the ADC jumper), not necessarily the
converter supply.  Include this only because the checks page is being edited;
otherwise the row is not wrong.

**Exact fix:**

> | `ADC(...) FAIL` alone | the `ADC` jumper is missing or loose, or the probe
> input is faulty | fit/reseat the jumper; if `3.3V(...)` also fails, suspect
> the switched 3.3 V output |

## Cross-set list (for the main session)

- `docs/core/system.md:457-459` — the W-rail description reads
  `BP_ADC_3V3` without saying it is the switched 3.3 V output:

  > `switch_psu_on()` reads `BP_ADC_3V3` and `BP_ADC_5V0` after a 2 ms settle
  > and requires both above `V33L`/`V5L`; otherwise it disables the regulator
  > and reports a too-low VREG.

  **Why:** `BP_ADC_3V3` is the switched 3.3 V branch, not AVdd.  A reader can
  take the `W` rail check as a check of the main 3.3 V rail.  This is the ADC
  audit's finding 5; it is owned by `core`, not `selftest`.
  **Exact fix:** "`switch_psu_on()` reads `BP_ADC_3V3` (the switched 3.3 V
  output) and `BP_ADC_5V0` after a 2 ms settle ..."; optionally add "this
  checks the switched target branch, not AVdd."

**Adjacent wording, not counted:** `wiring.md:79` says "a fitted
`ADC`→`+3.3V` jumper is a plain wire to the 3.3 V rail".  For the header
`+3.3V` branch, "the switched 3.3 V output" is more accurate.  It is the same
label family but was not in the ADC audit's list; change it only if the
implementer is already editing the sentence.  Also, `checks.md:123` (the
`3.3V VPU` row) ends "suspect the rail"; if finding 6 changes
`terminal.md:160`, use "suspect the switched 3.3 V output" there too.  The ADC
audit otherwise left that row unchanged.

## Presentation (changed passages only)

- **Tabs:** 0 in all six `selftest` files; the changed passages are
  space-indented.
- **Tables:** the `checks.md:19` and `checks.md:122` rows are 3-column rows;
  the replacements keep 3 columns.  `terminal.md:160` is a 3-column table row;
  keep the shape.  The `edge-cases.md` replacement keeps the existing
  per-case heading + Symptom/Behaviour/Recovery bullets.
- **Width:** `checks.md:122` is currently 246 characters — the widest row in
  the set.  It is a table cell, not a prose wrap; the replacement should not
  add a fourth column or a narrative.  If width matters, keep the row cells
  short and put the limit in the paragraph.  The `checks.md:129-130` prose is
  74–76 characters per line; keep the replacement at ~80.  `wiring.md:59` and
  the `edge-cases.md` bullets are also ~80.
- **Dates / bench / pass counts:** none in the changed passages.  The existing
  "not verified on v3" caveat is outside them and is the allowed inline kind.
  No capture provenance, run dates, pass counts, or internal log names.

## What I did not reopen

- The SMPS refusal sequence, the check matrix, the v3 order, the binary exit
  semantics, and the other labels are stable in this set; only the RA-10
  claims above are in scope.
- The `adc` set is already corrected at `c15e8dd0`; its `probe.md` is the
  authority for the wording, not a page to re-audit here.
- `terminal.md:168-170` ("wide band ... dead or badly wrong rail") is the
  generic voltage-window statement; it does not repeat the same-rail model and
  the ADC audit explicitly left it alone.
- The `checks.md:121` `ADC` row is correct for RA-10 (the jumper ties the probe
  to header `+3.3V`); it is not the primary stale claim.

**Finding count:** 6 selftest findings (2 major, 4 minor) plus 1 cross-set
item.
