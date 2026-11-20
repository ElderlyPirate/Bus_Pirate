# Generic prompt layer — Phase F delta audit

**Scope:** `docs/generic/` after the #209 repairs. Docs head `1f0752bb`;
firmware checked against the final tree in this worktree, `Firmware/`
(`9b179cb5`). Read-only; no docs edits, commits, or pushes.

**Verdict: yes with fixes.** The generic pages are mostly current for the
four changed behaviours, but two small generic passages still describe the
pre-#209 BRG/reset surface, and two cross-set pages remain materially stale.

## Findings

1. **[severity: minor] `docs/generic/usage.md:344-346` — the user reset summary is incomplete for the new reset.**
   Quote: "On v4 the reset also releases `AUX1` / `AUX2` and clears their output routing, so every header I/O pin is an input afterwards."
   Why: the final `bp_reset_board_state()` also clears the v4 `AUX1` open-drain bit (`BP_AUX1_ODC = OFF`) and stops Timer2 (`T2CONbits.TON = OFF`) before detaching the AUX source (`Firmware/core_system/base.c:172-205`). The page's "clears their output routing" is the RPOUT routing only; it does not state the AUX1 ODC clear or the Timer2 stop. The plural "their" can read as both AUX1 and AUX2 ODC, while only AUX1 ODC is cleared.
   Fix: add one sentence to the reset section, for example: "The board-state reset also stops Timer2, so any AUX PWM/servo output ends.  On v4 it releases `AUX1`/`AUX2`, clears their RPOUT routing and the `AUX1` ODC bit." Do not claim that AUX2 ODC is cleared; it is not.

2. **[severity: minor] `docs/generic/navigation.md:244-248` — "A complete `b10 <value>` skips the prompt" is stale for invalid values.**
   Quote: "A complete `b10 <value>` skips the prompt."
   Why: the final v3 `set_baud_rate()` accepts a BRG only in 1–32767. A supplied `0`, a malformed value, or an overflow prints `Enter raw value for BRG` and re-prompts at `(34)>` (`Firmware/core_system/proc_menu.c:1804-1843`). "Complete" therefore does not mean "skips" for those values, and the page never says that an invalid value re-opens the bounded prompt.
   Fix: "A valid `b10 <value>` (1–32767) skips the prompt; a missing, zero, malformed, or >32767 value reopens `Enter raw value for BRG` at the same `(34)>` prompt."

## Cross-set list (for the main session)

### C1 — `docs/core/system.md` reset order is missing the two #209 steps — **major**

Quotes:
- `docs/core/system.md:536-537`: "2. v4: release AUX1/AUX2; on SMPS builds run `smps_stop()`; detach the AUX1/AUX2 peripheral outputs and clear their latches."
- `docs/core/system.md:542`: "5. Detach the source from the currently selected AUX pin."
- `docs/core/system.md:438-440` summary: "It releases the bus and AUX pins, stops SMPS, parks the onboard EEPROM, clears the mode configuration, disables the pull-ups and regulator, and reconfigures the ADC …"

Why: the page calls this the full ordered sequence, but the final `bp_reset_board_state()` clears `BP_AUX1_ODC` in the v4 block and then executes `T2CONbits.TON = OFF` before `bp_aux_pin_reset_state()` (`Firmware/core_system/base.c:172-205`). The step list and the summary both omit the AUX1 ODC clear and the Timer2 stop; the core reset test already checks both (`Firmware/tests/test_core_reset.py` asserts the Timer2 stop and the AUX1 ODC mask).

Fix: add "clear the AUX1 ODC bit" to step 2; add "stop Timer2 (`T2CONbits.TON = OFF`), leaving its pairing and an independent Timer3 alone; then detach the source" to step 5; add "stops Timer2" and "clears the AUX1 ODC bit" to the line-438 summary. Do not claim AUX2 ODC is cleared.

### C2 — `docs/protocol/uart/` still quotes the old 16 M auto-baud overflow message — **major**

Quotes:
- `docs/protocol/uart/edge-cases.md:80`: "`** Baud>16m: The BP cannot measure above 16000000, Done.`" and "the printed ceiling is the message text; the check is at 150 kbaud".
- `docs/protocol/uart/entry-and-setup.md:252`: "`** Baud>16m: The BP cannot measure above 16000000, Done.`"; line 262: "The printed 16,000,000 is the message text, not the check."
- `docs/protocol/uart/macros.md:95-96`: "`** Baud>16m: The BP cannot measure above 16000000, Done.`  The printed 16,000,000 is the message text; the check is at 150 kbaud."
- `docs/protocol/uart/errors.md:111`: "`** Baud>16m: The BP cannot measure above 16000000, Done.` | the measured rate exceeded the 150 kbaud check".

Why: the final firmware prints `** Baud>150k: The BP cannot measure above 150000, Done.` in both builds (`Firmware/core_messages/messages_v3.s:1559`, `messages_v4.s:1655`; the check in `protocol_uart/uart.c:702` is `detected > 150000`). The old 16 M text and the "printed 16,000,000 is not the check" explanation are now wrong. A user who sees `Baud>150k` will think the firmware disagrees with the docs.

Fix: replace every old string with `** Baud>150k: The BP cannot measure above 150000, Done.` and delete the old "printed 16,000,000 is the message text, not the check" wording. The new message's `150000` is the 150 kbaud check; no separate 16 M ceiling remains.

### C3 — `docs/protocol/uart/entry-and-setup.md` omits the v3 BRG re-prompt — **minor**

Quotes:
- `docs/protocol/uart/entry-and-setup.md:165-167`: "On **v4** … A value below 62 is rejected and the prompt re-asks.  Options above 999999 are rejected the same way."
- `docs/protocol/uart/entry-and-setup.md:169-171`: "On **v3** the value is the divider itself."

Why: the v3 custom-baud range is correctly 1–32767, but the page does not say that 0, malformed, or overflow values re-ask at `(34)>`; the v4 paragraph gives the behaviour, the v3 paragraph does not. The final `protocol_uart/uart.c:270` uses `getnumber(34, 1, 32767, 0)` for the v3 path.

Fix: add to the v3 paragraph: "A missing, zero, malformed, or >32767 value reopens the prompt at `(34)>`; the accepted range is 1–32767."

## Verified unchanged

- **v3 raw BRG bounds:** `docs/generic/navigation.md:245` says "usable values are 1–32767"; this matches `getnumber(34, 1, 32767, 0)`. No old `0–32767` claim remains in the generic set.
- **Terminal `a`/`A`/`@`:** `docs/generic/commands.md:88,139-140` still says `AUX LOW`, `AUX HIGH` and `AUX INPUT/HI-Z, READ: `. The final `capability_aux/aux_pin.c` prints `BPMSG1041`/`BPMSG1040`/`BPMSG1039` ("AUX LOW"/"AUX HIGH"/"AUX INPUT/HI-Z") from the terminal helpers. That is correct and should stay.
- **Binary I2C `0x09`:** the generic set does not imply that the binary command prints terminal text. The current `docs/protocol/i2c/binary.md:232-235` also says the reply is exactly `0x01` plus the sub-result; the firmware deliberately avoids the terminal helpers (`Firmware/protocol_i2c/i2c.c:1151-1160`). No stale generic claim.
- **Auto-baud text in the generic set:** none. The only generic reference is the UART setup-menu label "11. Auto-Baud Detection (Activity Required)" in `usage.md:53`, which is the correct, unchanged menu text.

## Presentation on changed passages

- No TAB characters in the generic files.
- No changed prose line exceeds 80 columns; the changed passages are prose, not tables or captures, so there is no new column alignment to check.
- No dates, bench/rig/battery narratives, pass counts, capture provenance, or tested/verified-result prose in the changed generic passages. The only `verified` strings are the existing negative `not verified` caveats (`README.md:6`, `usage.md:315,369`), which are allowed.
- The existing wide `v` captures are unchanged and remain column-aligned; no new capture was added for these repairs.
