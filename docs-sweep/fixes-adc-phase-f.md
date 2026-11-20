# ADC — Phase F implementation record (RA-10 delta)

**Scope:** `docs/capabilities/adc/` plus this record.  Worktree
`bpv4-docssweep`, branch `docs/combined-sweep`, docs head `d5e5814c`; verified
against the worktree's `Firmware/` tree (combined `9b179cb5`).  No firmware
changes, no commits, no pushes, no WORKLOG edits.

**Result:** 5 findings.  2 are fixed in the ADC set (1, 3).  0 are rejected.
3 are cross-set only (2, 4, 5).  No code-owner candidate.

## Decisions

| # | Decision | What changed (file + short quote) | Evidence |
|---|---|---|---|
| 1 | fixed | `probe.md` reference bullet: "the main 3.3 V rail — the converter's own supply, AVdd".  Ratiometric bullet: "The `3.3V` channel is not that rail: it watches the separate switched 3.3 V output through a divider, measured against AVdd".  Resolution table: "reference \| the main 3.3 V rail (AVdd, converter supply), nominal" and "3.3 V channel \| switched 3.3 V output, measured against AVdd; branch-present/gross-droop check".  Channel table: "3.3 V channel (`3.3V`) ... (switched output; see below)".  Channel paragraph: "It is not the rail that supplies the converter: the converter and the ADC reference run from the separate main 3.3 V rail (AVdd). ... It can show that the switched output is present and catch a gross droop; a small droop may be within the divider and reference tolerance, and the reading cannot verify the AVdd rail itself." | `Firmware/hardwarev4.h`: `BP_ADC_3V3 0`, `BP_ADC_3V3_CFG AD1PCFGbits.PCFG1 // AN0/ADC3/33V`; `Firmware/core_system/base.c`: `AD1CON2 = 0x0000` with `VCFG: VR+ is AVdd and VR- is AVss`; `Firmware/hardware.h`: `V33BASE 0x200` with the 2:1 divider comment.  The recovered V4 topology (VR1/3V3 feeds PIC VDD/AVdd; VR2/SWV33 feeds ADC3 through RN3C) is the Phase F source fact in `feedback-adc-phase-f.md` and `PHASE-F-DOCS.md`; the firmware confirms the channel mapping and AVdd reference. |
| 2 | cross-set | No ADC change.  The selftest set owns `checks.md:122` and `:129-130`; see cross-set item 1. | `Firmware/hardwarev4.h` and `base.c` as above.  The monitored 3.3 V branch is the switched output, measured against AVdd; it is not ratiometric to the rail it monitors. |
| 3 | fixed | `README.md` limits summary: "The switched 3.3 V output (the `3.3V` channel), the 5.0 V rail, VPU and (v4) Vusb channels are housekeeping, displayed by `v` and read by the self-test and the features that check them." | Same topology and mapping evidence as finding 1.  This removes the last ADC-set summary that called the channel "the 3.3 V rail". |
| 4 | cross-set | No ADC change.  The selftest set owns `checks.md:19`, `wiring.md:59` and `edge-cases.md:143-149`; see cross-set items 2-4. | The switched-output branch feeds the `3.3V` channel, the `ADC` jumper and the `3.3V VPU` source; it is not the main AVdd rail. |
| 5 | cross-set | No ADC change.  The core set owns `docs/core/system.md:457-459`; see cross-set item 5. | `switch_psu_on()` reads `BP_ADC_3V3`, which is the switched 3.3 V branch, not AVdd. |

## Counts

- fixed in the ADC set: 2 (findings 1, 3).
- rejected / not a defect: 0.
- code-owner candidates: 0.
- cross-set: 3 findings (2, 4, 5); five required correction items below.

## Files changed

- `docs/capabilities/adc/probe.md`
- `docs/capabilities/adc/README.md`
- `docs-sweep/fixes-adc-phase-f.md` (this record)

## Cross-set changes for the main session

1. **Selftest set (finding 2).**  `docs/capabilities/selftest/checks.md:122`
   still says "reads the 3.3V rail channel in the 3.3V window; the channel is
   ratiometric to the rail".  Replace with "reads the switched 3.3 V output
   (header `+3.3V` branch) through a divider, measured against the internal
   AVdd reference"; change the failure cell to "near zero: switched output off
   or divider/channel fault; a small droop stays inside the window; the check
   cannot verify AVdd."  `checks.md:129-130` still says "The `3.3V` channel is
   ratiometric to the rail"; replace with "The `3.3V` channel measures the
   switched 3.3 V output against the internal AVdd reference: it is a
   branch-present and gross-droop check, not a check of the AVdd rail."
2. **Selftest set (finding 4).**  `docs/capabilities/selftest/checks.md:19`
   Names row says "the 3.3V rail"; change to the switched 3.3 V output (header
   `+3.3V`).
3. **Selftest set (finding 4).**  `docs/capabilities/selftest/wiring.md:59`
   says the test "measures the 5V and 3.3V rails"; change to "measures the 5V
   rail and the switched 3.3 V output".
4. **Selftest set (finding 4).**  `docs/capabilities/selftest/edge-cases.md:143-149`
   says "v4 3.3V rail off" and "seven failures from the one rail"; change to
   "v4 switched 3.3 V output off" and "seven failures from the one switched
   branch".
5. **Core set (finding 5).**  `docs/core/system.md:457-459` says
   "`switch_psu_on()` reads `BP_ADC_3V3` and `BP_ADC_5V0`".  Qualify the
   first as "`BP_ADC_3V3` (the switched 3.3 V output)" and, if the core set
   wants the limit explicit, add "this checks the switched target branch, not
   AVdd."

Optional consistency only: `docs/capabilities/selftest/terminal.md:160` says
"if `3.3V(...)` also fails, suspect the rail"; it is correct as far as it goes
and needs no separate change, but the "switched output" wording should be
used if that row is touched.

## Verification notes

- `probe.md` and `README.md` contain no code references, dates, bench prose,
  pass counts or TAB characters.
- Every relative link and anchor in the six ADC files resolves.
- The ADC tables keep consistent column counts.
- The probe's own ratiometric statement is unchanged: the probe is measured
  against AVdd, so a change in the main 3.3 V rail still shifts its reading.
