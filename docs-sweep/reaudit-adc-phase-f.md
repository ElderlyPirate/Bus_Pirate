# ADC — Phase F re-audit (RA-10 in-set findings)

**Scope:** in-set findings 1 and 3 from `feedback-adc-phase-f.md` only.
Cross-set findings 2, 4 and 5 remain for the selftest and core sets and were
not reviewed here. Current docs head is `d5e5814c`.

## Verdict

**2 of 2 filled in full.** The four `probe.md` passages and the `README`
summary now use the two-rail model; the probe's own ratiometric statement is
unchanged and correct. The ADC set is clean for the in-set RA-10 findings.

## Per-item table

| # | Status | Evidence (current docs) | Remainder |
|---|---|---|---|
| 1 | **Filled in full** | **A — `probe.md:27-29`:** "The `3.3V` channel is not that rail: it watches the separate switched 3.3 V output through a divider, measured against AVdd". **B — `probe.md:111-122`:** "The `3.3V` channel watches the switched 3.3 V output (the header `+3.3V` branch) through its own divider.  It is not the rail that supplies the converter: the converter and the ADC reference run from the separate main 3.3 V rail (AVdd). ... It can show that the switched output is present and catch a gross droop; a small droop may be within the divider and reference tolerance, and the reading cannot verify the AVdd rail itself." **C — `probe.md:81-82`:** "reference \| the main 3.3 V rail (AVdd, converter supply), nominal" and "3.3 V channel \| switched 3.3 V output, measured against AVdd; branch-present/gross-droop check". **D — `probe.md:100`:** "3.3 V channel (`3.3V`) ... (switched output; see below)". `probe.md:19-20` also says "the main 3.3 V rail — the converter's own supply, AVdd". | None. |
| 3 | **Filled in full** | `README.md:87-89`: "The switched 3.3 V output (the `3.3V` channel), the 5.0 V rail, VPU and (v4) Vusb channels are housekeeping, displayed by `v` and read by the self-test and the features that check them." | None. |

## Probe ratiometric statement

`probe.md:25-26` is unchanged:

> The probe reading is ratiometric to the 3.3 V rail: if that rail is not at
> its nominal value, the probe and the other external channels shift with it.

This is correct. The probe is measured against the main 3.3 V rail (AVdd),
named explicitly at `probe.md:19-20`; the following sentence (`27-29`) now
makes clear that the `3.3V` channel is a different branch, so the unchanged
statement no longer carries the old self-reference implication.

## Presentation

- **TAB characters:** 0 in all six ADC files.
- **Tables:** no pipe-count mismatches in any ADC file. The new
  `probe.md:82` row (98 characters) and channel row `100` (133 characters) are
  table cells, not prose wraps; the README Related table is unchanged and also
  within table cells.
- **~80-column prose:** no non-table, non-link-definition line exceeds 80
  characters. The new paragraph at `probe.md:111-122` is 70–78 characters per
  line; the README summary is ≤80.
- **Links/anchors:** every relative link and anchor in the six ADC files
  resolves, including `probe.md#the-channels`, `terminal.md#smps-owns-the-adc`
  and `binary.md#smps-owns-the-adc`.
- **Dates / bench / pass counts:** none in the changed passages. The only
  verification language is the existing "not verified"/"unverified" caveat
  (`README.md:3-4`, `wiring.md:120`), which is the allowed inline kind.

## Residual note (not a finding)

The new channel paragraph is written for the recovered V4 board and does not
say "on v4"; the v3 caveat at `probe.md:167-169` covers the v3 divider and safe
input range, not the channel topology. This does not affect the in-set RA-10
finding, which was about removing the false same-rail model. If the author
wants the scope explicit, add "on v4" to the first sentence; optional.
