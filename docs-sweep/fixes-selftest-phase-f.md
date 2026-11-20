# Self-test — Phase F delta repair record (RA-10 topology)

**Scope:** `docs/capabilities/selftest/**` only.  Branch `docs/combined-sweep`,
docs head `91b560e7`, final combined `Firmware/` `9b179cb5` inside the docs
worktree.  **No commit / push.**  No firmware, core page, or other set edited.

**Audit:** `docs-sweep/feedback-selftest-phase-f.md` — 6 in-set findings
(2 major, 4 minor) + 1 core cross-set item + one adjacent wording note.

**Verdict:** **6 / 6 findings fixed** (0 rejected).  Finding 6 was optional and
is applied; the audit's adjacent `3.3V VPU` row was aligned with it.  The
core page is **not edited** — the cross-set note is recorded below for the
main session.  `adc` was not reopened; `probe.md` is the wording authority.

| # | Sev | Decision | One-line |
|---|---|---|---|
| 1 | major | fixed | `3.3V` row now says switched 3.3 V output vs AVdd; real limit stated |
| 2 | major | fixed | the paragraph no longer says “ratiometric”; same AVdd wording |
| 3 | minor | fixed | Names row calls `+3.3V` the switched 3.3 V output |
| 4 | minor | fixed | wiring says “5V rail and the switched 3.3 V output” |
| 5 | minor | fixed | v4 edge case is one switched branch, not “one rail” |
| 6 | minor | fixed (optional) | terminal row says “suspect the switched 3.3 V output” |

---

## Per-finding record

### 1 — major — `checks.md` `3.3V` row (was line 122) → **fixed**

**Changed:** `docs/capabilities/selftest/checks.md`, supply/probe table row.
New text:

> | `3.3V` | reads the switched 3.3 V output (header `+3.3V`) through a
> divider, measured against the internal AVdd reference; a branch-present and
> gross-droop check | near zero: switched output off or divider/channel fault;
> a small droop may stay inside the window or in divider/reference tolerance;
> the check cannot verify AVdd |

The three-column table shape is unchanged.

**Evidence (final tree `bpv4-docssweep/Firmware/`):**

- `Firmware/hardwarev4.h:162-170` — `BP_ADC_3V3 0`,
  `BP_ADC_3V3_CFG = AD1PCFGbits.PCFG1`, commented `AN0/ADC3/33V`.
- `Firmware/core_system/base.c:241-253` — `AD1CON2 = 0x0000`; the register
  comment states `VCFG: VR+ is AVdd and VR- is AVss`.
- `Firmware/capability_selftest/selftest.c:224` —
  `perform_adc_test(BP_ADC_3V3, V33L, V33H)`.
- Accepted wording authority: `docs/capabilities/adc/probe.md:81-82` and
  `:111-122` — switched 3.3 V output, measured against AVdd;
  branch-present/gross-droop check; small droop can be lost in divider and
  reference tolerance.
- The VR1/3V3 vs VR2/SWV33/RN3C split is the recovered-schematic fact recorded
  by the accepted ADC page (same source as the Phase F brief), not derivable
  from the firmware defines alone.

### 2 — major — `checks.md` paragraph (was lines 129-130) → **fixed**

**Changed:** `docs/capabilities/selftest/checks.md`, same section, paragraph
now reads:

> The `3.3V` channel measures the switched 3.3 V output against the internal
> AVdd reference: it is a branch-present and gross-droop check, not a check of
> the AVdd rail.

The preceding generic near-zero sentence was kept, as the audit allowed.  The
old “rationally/ratiometric to the rail” model is gone.

**Evidence:** same as finding 1 — `base.c:251-253` (AVdd reference) and
`selftest.c:224` (`BP_ADC_3V3`); `adc/probe.md:111-122` states the same limit.

### 3 — minor — `checks.md` Names row (was line 19) → **fixed**

**Changed:** `docs/capabilities/selftest/checks.md`, Names table:

> | `+3.3V` | `3.3V` | the switched 3.3 V output (header `+3.3V`);
> `3.3V VPU` is its pull-up source (v4) |

**Evidence:** `hardwarev4.h:162-170` maps the channel to `AN0/ADC3/33V`;
`adc/probe.md:100` calls it “the 3.3 V channel (`3.3V`) ... (switched output;
see below)”; `adc/probe.md:111-113` distinguishes it from AVdd.

### 4 — minor — `wiring.md` supply sentence (was line 59) → **fixed**

**Changed:** `docs/capabilities/selftest/wiring.md` §Pull-ups and supply:

> - **No external supply is required.**  Power the Bus Pirate as usual (USB is
>   the normal source).  The test enables the board's regulator itself (the
>   `VREG` check), measures the 5V rail and the switched 3.3 V output, and
>   switches them off at the end.

**Evidence:** the measured channels are `BP_ADC_5V0` and `BP_ADC_3V3`
(`selftest.c:192,224`); the latter is the switched branch, not AVdd
(`hardwarev4.h:162-170`, `base.c:251-253`).

### 5 — minor — `edge-cases.md` v4 case (was lines 143-149) → **fixed**

**Changed:** `docs/capabilities/selftest/edge-cases.md`, case heading and
bullets:

> ### v4 switched 3.3 V output off
>
> - **Symptom:** `3.3V(...) FAIL`, `3.3V VPU(...) FAIL`, `ADC(...) FAIL` and
>   the four `Bus Hi-Z 1` checks fail.
> - **Behaviour:** seven failures from the one switched branch: the `3.3V`
>   check, the `3.3V VPU` source, the `ADC` jumper and the four `Bus Hi-Z 1`
>   checks.
> - **Recovery:** fix the switched 3.3 V output, then rerun.

The heading/Symptom/Behaviour/Recovery shape is unchanged.

**Evidence:** `selftest.c:219-225` reads `BP_ADC_VPU` and `BP_ADC_3V3`;
`:233` reads the `3.3V VPU` source; `:264` reads the `ADC` probe with the
`+3.3V` jumper; the four `Bus Hi-Z 1` checks use the same 3.3 V pull-up source
(`:288-310`).  All seven belong to the switched 3.3 V branch, not AVdd.

### 6 — minor — `terminal.md` failure row (was line 160) → **fixed (optional)**

**Changed:** `docs/capabilities/selftest/terminal.md`, “Reading a failure”
table row:

> | `ADC(...) FAIL` alone | the `ADC` jumper is missing or loose, or the probe
> input is faulty | fit/reseat the jumper; if `3.3V(...)` also fails, suspect
> the switched 3.3 V output |

**Adjacent consistency (audit note):** the `3.3V VPU` row in `checks.md` now
also ends “suspect the switched 3.3 V output” instead of “suspect the rail”.
The `5V VPU` row keeps “suspect the rail” because that case really is the 5V
rail.  `wiring.md:79` (“a fitted `ADC`→`+3.3V` jumper is a plain wire to the
3.3 V rail”) was left unchanged: the audit gated it on already editing that
sentence, and it describes the physical header wire, not a measurement claim.

**Evidence:** `ADC` and `3.3V` are separate channels (`selftest.c:215,224`);
the `ADC` jumper ties the probe to the `+3.3V` header branch, the same
switched output the `3.3V` check watches (`hardwarev4.h:162-170`;
`adc/probe.md:111-120`).  A joint failure therefore points at that branch (or
the jumper), not at the converter supply.

---

## Cross-set note (do not edit here)

**Core page for the main session:** `docs/core/system.md:457-459` currently
reads:

> `switch_psu_on()` reads `BP_ADC_3V3` and `BP_ADC_5V0` after a 2 ms settle
> and requires both above `V33L`/`V5L`; otherwise it disables the regulator
> and reports a too-low VREG.

The `BP_ADC_3V3` channel is the switched 3.3 V branch, not AVdd.  Suggested
fix for the `core` owner:

> `switch_psu_on()` reads `BP_ADC_3V3` (the switched 3.3 V output) and
> `BP_ADC_5V0` after a 2 ms settle ...

optionally followed by “this checks the switched target branch, not AVdd.”
This is the ADC audit's finding 5 and is owned by `core`, not `selftest`.
**Not edited in this pass.**

---

## Verification performed

- Re-read the final tree's `Firmware/hardwarev4.h` ADC block,
  `Firmware/core_system/base.c` converter setup, and
  `Firmware/capability_selftest/selftest.c` measurement calls before editing.
- Checked the new rows against the accepted
  `docs/capabilities/adc/probe.md` wording (not re-audited).
- 0 TAB bytes in the six `selftest` files; changed prose lines ≤ 80 columns
  (table rows excepted); all Markdown tables have a consistent column count.
- All relative links in the six files resolve (0 missing).
- No dates, bench prose, pass counts, or code references added to user pages.
- **Not run:** firmware build/hardware (documentation-only repair); v3
  behaviour remains labelled unverified on the pages that say so.
