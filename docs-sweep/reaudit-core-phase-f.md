# Core — Phase F re-audit

**Docs head:** `1f0752bb`.  **Firmware:** worktree `Firmware/` (combined
`9b179cb5`).  I re-read the changed `docs/core/system.md` passages and the
cross-set pages named in the five Phase F findings.  The two in-set fixes are
present; the three cross-set items remain recorded for the main session.

**Verdict: 2 of 5 filled in full** — both in-set findings are complete.  The
core set itself is clean; C1–C3 are cross-set carry-in and are still open in
their owning sets.

## Per-item re-audit

| # | Finding | Status | Evidence quote | Remainder |
|---|---|---|---|---|
| 1 | Core ordered reset list omitted Timer2 stop and V4 `AUX1` ODC clear | Filled in full | `system.md:537-539`: "2. v4: release AUX1/AUX2; on SMPS builds run `smps_stop()`; detach the AUX1/AUX2 peripheral outputs, and clear `AUX1`'s open-drain bit (`BP_AUX1_ODC = OFF`), then clear their latches."  `system.md:544-545`: "5. Stop Timer2 (`T2CONbits.TON = OFF`); leave its pairing and an independent Timer3 unchanged." | None. The list is renumbered 1–9, and the summary at `:438` states "stops SMPS and Timer2, clears the v4 `AUX1` ODC bit" without claiming AUX2 ODC. |
| 2 | v3 raw BRG omitted the 1–32767 bound and bounded re-prompt | Filled in full | `system.md:510`: "entry 10 asks for the raw `U1BRG` (1–32767; blank uses 34; zero, malformed or overflow re-prompts at `(34)>`)." | None. Matches `set_baud_rate()`'s `getnumber(34,1,32767,0)` path. |
| C1 | UART auto-baud pages still quote the old 16m message | Not addressed (cross-set) | `docs/protocol/uart/entry-and-setup.md:252`: "`** Baud>16m: The BP cannot measure above 16000000, Done.`"; `:262`: "The printed 16,000,000 is the message text, not the check."  Same old message in `errors.md:111`, `macros.md:95`, and `edge-cases.md:80`. | Recorded in `fixes-core-phase-f.md` for the main session; not a core edit. Needs the new `Baud>150k`/`150000` text and removal of the 16,000,000 caveats. |
| C2 | SMPS pages say a terminal reset does not clear `AUX1` ODC | Not addressed (cross-set) | `docs/capabilities/smps/edge-cases.md:138-139`: "A v4 terminal reset does not clear it either; a power cycle or hardware reset does."  Same claim in `wiring.md:74-75`; `README.md:47-48` still says power cycle/hardware reset; `exit.md:108-115` omits the ODC clear. | Recorded for the main session; not a core edit. The final reset clears `BP_AUX1_ODC`, so these pages need the correction. |
| C3 | Binary/OpenOCD reset summaries omit the new `AUX1` ODC clear | Not addressed (cross-set) | `docs/protocol/binary/commands.md:211-219` lists pins to inputs, supplies off, SMPS stop, AUX source detach and Timer2, but no `AUX1` ODC clear; `edge-cases.md:118-123`, `entering-and-framing.md:190-194` and `tools/openocd/entry-and-exit.md:97-100` have the same omission. | Recorded for the main session; incomplete reset summaries, not false claims. Add "clears the V4 `AUX1` open-drain bit." |

## Summary-sentence check

`system.md:438` now reads: "the bus and AUX pins, stops SMPS and Timer2, clears
the v4 `AUX1` ODC bit, parks the onboard EEPROM ...".  This states both required
facts and does not claim an AUX2 ODC clear.  Pass.

## Presentation

- No TAB characters in the changed core passage or the six core files.
- The ordered reset list is still a numbered list, 1–9, and its prose is within
  ~80 columns.  The 261-character v3 baud row is a table row, not prose.
- Links and anchors resolve, including
  `#board-state-reset-bp_reset_board_state`.
- No dates, bench/rig narratives, pass counts, provenance or internal log
  names in the changed passage.  The only "verified" text is the allowed
  "not verified on v3 hardware" caveat.

## Bottom line

The core set is clean for the two in-set findings.  C1–C3 remain open in the
UART, SMPS and binary/OpenOCD sets and were correctly recorded rather than
edited from core.

## Focused follow-up — `switch_psu_on()` sentence

| # | Item | Status | Evidence quote | Remainder |
|---|---|---|---|---|
| F1 | `BP_ADC_3V3` must be qualified as the switched 3.3 V output, not AVdd | Filled in full | `system.md:457-460`: "`switch_psu_on()` reads `BP_ADC_3V3` (the switched 3.3 V output) and `BP_ADC_5V0` after a 2 ms settle and requires both above `V33L`/`V5L`; otherwise it disables the regulator and reports a too-low VREG.  This checks the switched target branch, not AVdd." | None. The final `Firmware/` maps `BP_ADC_3V3` to the `ADC3/33V` channel on both boards (`hardwarev4.h:163/170`, `hardwarev3.h:113/119`), and `switch_psu_on()` reads that channel after the 2 ms settle. The sentence is 68–76 columns, adds no code reference, adds/removes no links, and contains no TAB or bench/date/pass-count prose. |

**Follow-up verdict: 1 of 1 filled in full; the follow-up is clean.**

**Final verdict:** 2 of 5 original Phase F findings filled in full, plus the
focused follow-up 1/1.  The core set is clean; C1–C3 remain cross-set open.
