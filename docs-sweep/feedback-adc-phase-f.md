# ADC — Phase F delta audit (RA-10 topology)

**Scope:** only the RA-10 topology claim and adjacent changed facts in the
`adc` set and the selftest ADC checks. Docs head `d5e5814c`; the local
`Firmware/` tree is a plain tree (not a nested Git repo), and the relevant
mappings are `hardwarev4.h` `BP_ADC_3V3 = AN0/ADC3/33V` and `base.c`
`VCFG = AVdd`. The topology itself is the recovered V4 schematic fact in
`PHASE-F-DOCS.md` and `WORKLOG-DOCUMENTATION.md` §2F/Phase A: VR1/3V3 feeds
PIC VDD/AVDD; VR2/SWV33 feeds ADC3 through RN3C.

## Verdict

**No — not ready.** The exact "same rail measured against itself" model is
still the primary explanation in `probe.md`, and the selftest `3.3V` row still
calls the channel ratiometric to the rail. The monitored 3.3 V branch is the
switched SWV33 output, not AVdd; the ADC set needs the targeted rewrite below.
The rest of the ADC set is stable and should not be reopened.

## Findings

### 1. [major] `probe.md` still teaches the same-rail model in four places

**Location A — `probe.md:27-28`:**

> The 3.3 V channel is the exception: it is the same rail through a divider,
> so it is measured against itself (see [The channels](#the-channels)).

**Location B — `probe.md:110-118`:**

> The 3.3 V channel is the rail that supplies the converter, brought in through
> its own divider.  It is therefore ratiometric too: the displayed figure is the
> divider ratio expressed on the firmware scale, not an independent measurement
> of the rail.  A rail that sags while the board still runs can still read about
> 3.3 V, because the reference moves with it.  The `v` column, the `W` rail
> check and the self-test 3.3 V window are go/no-go checks on the divider and
> on the channel being present; they cannot prove the rail is at 3.3 V or catch
> a small droop.  The 5.0 V, Vusb and VPU channels are measured against the
> 3.3 V reference, so those readings do move with it.

**Location C — `probe.md:80-81`:**

> | reference | the board's 3.3 V rail (converter supply), nominal |
> | 3.3 V channel | ratiometric to that rail; divider/rail-present check only |

**Location D — `probe.md:99`:**

> | 3.3 V rail | yes | yes | `v`, the `W` rail check, the self-test, OpenOCD
> `buspirate adc` (ratiometric; see below) |

**Why:** on the recovered V4 board the converter and the ADC reference run from
the main 3.3 V rail (VR1/3V3, AVdd), while the `3.3V` channel monitors the
separate switched 3.3 V branch (VR2/SWV33) through RN3C. The channel is
measured against AVdd, but its input is not AVdd; "the same rail ... measured
against itself" is false. The real limit is: the channel can show the switched
3.3 V output is present and catch a gross droop, but it cannot verify the AVdd
rail, and a small droop is limited by the divider/reference tolerance and the
0.01 V display. A reader can currently believe the `v`/`W`/self-test 3.3 V
reading proves the converter supply is at 3.3 V.

**Exact fix:** rewrite the four passages around the two rails.

- Replace Location A with: "The `3.3V` channel is not that rail: it watches
  the separate switched 3.3 V output through a divider, measured against
  AVdd."
- Replace Location B with: "The `3.3V` channel watches the switched 3.3 V
  output (the header `+3.3V` branch) through its own divider.  It is not the
  rail that supplies the converter: the converter and the ADC reference run
  from the separate main 3.3 V rail (AVdd).  The channel is measured against
  AVdd like the others, so its displayed figure is the switched output's
  divider ratio expressed on the firmware scale.  It can show that the
  switched output is present and catch a gross droop; a small droop may be
  within the divider and reference tolerance, and the reading cannot verify
  the AVdd rail itself.  The `v` column, the `W` rail check and the self-test
  3.3 V window are therefore checks on the switched 3.3 V output, not on the
  converter supply.  The 5.0 V, Vusb and VPU channels are measured against
  the same AVdd reference, so those readings also move with it."
- Replace the two table rows with:

  ```
  | reference | the main 3.3 V rail (AVdd, converter supply), nominal |
  | 3.3 V channel | switched 3.3 V output, measured against AVdd; branch-present/gross-droop check |
  ```

- Replace the channel-table row with:

  ```
  | 3.3 V channel (`3.3V`) | yes | yes | `v`, the `W` rail check, the self-test, OpenOCD `buspirate adc` (switched output; see below) |
  ```
- Also change `probe.md:19-20` to "the main 3.3 V rail — the converter's own
  supply, AVdd" so the reference rail is visibly distinct from the monitored
  branch.

### 2. [major, cross-set] The selftest `3.3V` check repeats the stale ratiometric-to-the-rail model

**Location — `docs/capabilities/selftest/checks.md:122`:**

> | `3.3V` | reads the 3.3V rail channel in the 3.3V window; the channel is
> ratiometric to the rail, so it proves the divider/channel is present, not the
> absolute rail voltage | near zero: divider or channel fault; a small droop is
> not detectable |

**And `checks.md:129-130`:**

> The `3.3V` channel is ratiometric to the rail: it is a divider/presence
> check, not an absolute rail measurement.

**Why:** the same RA-10 error. The check reads the switched 3.3 V output
(SWV33/VR2) through a divider, measured against AVdd; it is not ratiometric to
the rail it monitors. A small droop in the switched output is a real input
change and is not "undetectable" by construction; it may simply stay inside
the 2.8–3.8 V window or be lost in divider/reference tolerance. What the check
cannot do is verify the main 3.3 V rail/AVdd. This is a cross-set page for the
selftest set, but it is part of the ADC/self-test claim the task asks to check.

**Exact fix:** change the row to "reads the switched 3.3 V output (header
`+3.3V` branch) through a divider, measured against the internal AVdd
reference"; change its failure cell to "near zero: switched output off or
divider/channel fault; a small droop stays inside the window; the check cannot
verify AVdd." Change the paragraph to "The `3.3V` channel measures the switched
3.3 V output against the internal AVdd reference: it is a branch-present and
gross-droop check, not a check of the AVdd rail."

### 3. [minor] The ADC README summary still calls the channel "the 3.3 V rail"

**Location — `docs/capabilities/adc/README.md:87`:**

> The 3.3 V rail, 5.0 V rail, VPU and (v4) Vusb channels are housekeeping,
> displayed by `v` and read by the self-test and the features that check them.

**Why:** after the correction the `3.3V` channel is the switched 3.3 V output,
not the main 3.3 V rail that supplies the converter. The summary would
otherwise contradict the corrected `probe.md` and keep the main-vs-switched
ambiguity alive.

**Exact fix:** "The switched 3.3 V output (the `3.3V` channel), the 5.0 V rail,
VPU and (v4) Vusb channels are housekeeping, displayed by `v` and read by the
self-test and the features that check them."

### 4. [minor, cross-set] The selftest labels still say "3.3V rail" / "one rail" where the branch is the switched output

**Locations:**

- `docs/capabilities/selftest/checks.md:19`: "| `+3.3V` | `3.3V` | the 3.3V
  rail; `3.3V VPU` is its pull-up source (v4) |"
- `docs/capabilities/selftest/wiring.md:59`: "measures the 5V and 3.3V rails,
  and switches them off at the end."
- `docs/capabilities/selftest/edge-cases.md:143-149`: "### v4 3.3V rail off
  ... seven failures from the one rail: the internal checks, the probe jumper
  and the pull-up source."

**Why:** the "one rail" in these passages is the switched 3.3 V output that
feeds the `3.3V` channel, the `ADC` jumper and the `3.3V VPU` source — not the
main AVdd rail. The labels are adjacent to the corrected `checks.md` row and
will otherwise keep implying that the self-test verifies the converter supply.

**Exact fix:** use "the switched 3.3 V output (header `+3.3V`)" in the Names
row; "measures the 5V rail and the switched 3.3 V output" in `wiring.md`; and
"v4 switched 3.3 V output off" / "seven failures from the one switched branch"
in `edge-cases.md`. This is the selftest set's edit, not the ADC set's.

### 5. [minor, cross-set] The core W-rail description needs the same branch qualifier

**Location — `docs/core/system.md:457-459`:**

> `switch_psu_on()` reads `BP_ADC_3V3` and `BP_ADC_5V0` after a 2 ms settle and
> requires both above `V33L`/`V5L`; otherwise it disables the regulator and
> reports a too-low VREG.

**Why:** `BP_ADC_3V3` is the switched 3.3 V branch, not AVdd. The code-map
sentence is accurate as far as it goes, but a reader can take the `W` rail
check as a check of the main 3.3 V rail. This is a cross-set page for the core
set; no ADC change.

**Exact fix:** say "`switch_psu_on()` reads `BP_ADC_3V3` (the switched 3.3 V
output) and `BP_ADC_5V0` ...". If the core set wants to make the limit
explicit, add "this checks the switched target branch, not AVdd."

## Rows checked, no change

- `probe.md:100` 5.0 V row and `probe.md:102` Vusb row: measured against AVdd;
  no same-rail claim. Correct as written.
- `probe.md:101` VPU row: measured against AVdd; correct.
- `probe.md:98` probe row: the probe divider and AVdd reference; correct.
- `selftest/checks.md:118-120` `Vusb`, `5V`, `5V VPU` rows: separate rails
  measured against AVdd; no stale claim.
- `selftest/checks.md:121` `ADC` row: the jumper ties the probe to the header
  `+3.3V` (the switched output); the row is correct for RA-10. If the selftest
  set edits its failure cell, it could add "or the switched 3.3 V output is
  off"; the `3.3V` row is the primary fix.
- `selftest/checks.md:123` `3.3V VPU` row: the source is on the switched 3.3 V
  branch; the row is correct once the branch label in finding 4 is fixed.
- `selftest/terminal.md:168-170`: the "wide band / dead or badly wrong rail"
  statement is about the self-test windows generally; it does not repeat the
  same-rail model, so no change is needed beyond the `checks.md` correction.
- The probe's own ratiometric statement (`probe.md:25-26`) is correct: the
  probe is measured against AVdd. Keep it; only the false 3.3 V "exception"
  and the channel table/paragraph change.

## Cross-set list

| Page | Owns | Item |
|---|---|---|
| `docs/capabilities/selftest/checks.md` | selftest | Finding 2: `3.3V` row and `129-130` paragraph still say ratiometric to the rail; replace with switched-output vs AVdd. |
| `docs/capabilities/selftest/checks.md:19` | selftest | Finding 4: Names row says "the 3.3V rail"; say switched 3.3 V output. |
| `docs/capabilities/selftest/wiring.md:59` | selftest | Finding 4: "measures the 5V and 3.3V rails"; say switched 3.3 V output. |
| `docs/capabilities/selftest/edge-cases.md:143-149` | selftest | Finding 4: "v4 3.3V rail off / one rail"; say switched 3.3 V output. |
| `docs/core/system.md:457-459` | core | Finding 5: qualify `BP_ADC_3V3` as the switched 3.3 V branch, not AVdd. |
| `docs/capabilities/selftest/terminal.md:160` | selftest | Optional adjacent: "`ADC` FAIL alone ... if `3.3V` also fails, suspect the rail" is correct; if the checks page is edited, keep the "switched output" wording consistent. No separate change required. |

## Presentation

- **TAB characters:** none in `probe.md` or `selftest/checks.md`; the quoted
  passages in `wiring.md`/`edge-cases.md`/`core/system.md` are space-indented.
- **Tables:** `probe.md` tables are aligned (3-column resolution/channel
  tables); `selftest/checks.md` is a 3-column table, and row 122 is a wide
  table cell, not a wrap failure. Keep the column count when replacing it.
- **~80 columns:** `probe.md` prose at locations A/B is 71–78 characters; the
  replacement should stay at ~80. The selftest table row is intentionally a
  table cell; the prose paragraph is ≤80.
- **Dates / bench / pass counts:** none in the changed passages. The only
  verification language is the existing "not verified"/"unverified" caveat,
  which is the allowed inline kind. No capture provenance, run dates, pass
  counts, or internal log names.

## What I did not reopen

The probe scale/truncation table, the terminal `D`/`d:N` material, the binary
`0x14`/`0x15` exchange, the SMPS ownership section, and the unrelated
self-test checks are unchanged and out of this delta's scope.
