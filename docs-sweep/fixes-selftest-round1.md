# Self-test — round 1 implementation record

**Scope:** `docs/capabilities/selftest/` plus this record.  Worktree
`bpv4-docssweep`, branch `docs/combined-sweep`; the `Firmware/` tree is the
combined image `c57e223`.  No firmware changes, no commits, no WORKLOG edits.
The v3 build status in the pages keeps the existing "not verified on v3"
caveat.

**Result:** 16 findings.  12 are fixed in the selftest set (finding 1 also
needs the cross-set SMPS correction).  0 are rejected outright.  There is no
code-owner candidate: no finding exposed a firmware bug.  4 findings are
cross-set only (3, 4, 14, 15), and finding 1 has a cross-set SMPS half.  One
recommendation inside finding 1 was rejected after tracing the firmware: the
`NORMAL` button is the bootloader button, not a reset, so the pages use the v4
reset button or a power-cycle instead.

## Decisions

| # | Decision | What changed (file + short quote) | Evidence |
|---|---|---|---|
| 1 | fixed (+ cross-set) | `checks.md` owns the reachable sequence: "The reachable sequence is:" (BBIO1 → `0x05` → `0xF2` → `0x00` → `0x10` refusal → `0xFF` → `0x05` → `0xF1`), and "A terminal user does not normally meet the refusal: reaching `HiZ>` already reset the board".  `terminal.md`: "The refusal is a binary-layer condition" and "Send `#` to reset the board and stop the SMPS".  `binary.md`: "that terminal reset stops the SMPS" and "The refusal itself is a binary-layer condition".  `edge-cases.md`: "A normal `HiZ>` prompt means the SMPS is not running" plus separate terminal/binary cases.  `README.md`: "a terminal `~` does not normally meet it because reaching `HiZ>` resets the board".  Stop/recovery prose now says "v4 reset button", not `NORMAL` button. | `Firmware/core_system/proc_menu.c`: `case '#'` calls `reset_terminal_state()`; `reset_terminal_state()` calls `bp_reset_board_state()`.  `Firmware/core_system/base.c`: `bp_reset_board_state()` calls `smps_stop()` under `BUSPIRATEV4` + `BP_ENABLE_SMPS_SUPPORT`.  `Firmware/protocol_binary/binary_io.c`: `handle_bitbang_command()` `0x0F` returns `-1` on v4, then the NUL entry in `proc_menu.c` calls `reset_terminal_state()`.  `Firmware/capability_selftest/selftest.c`: `perform_selftest()` first calls `bp_adc_foreground_available()`.  `base.c`: that returns `IEC0bits.AD1IE == OFF`.  `Firmware/capability_smps/smps.c`: `smps_start()` sets `AD1IE = ON`, `smps_stop()` clears it and disables OC5/ADC.  `Firmware/hardwarev4.h`: `BP_BUTTON` is RC14 and `BP_BUTTON_SETUP()` leaves `IEC1bits.CNIE = 0`; the NORMAL button is not polled by the blocking self-test and is not a reset. |
| 2 | fixed | `README.md` binary bullet: "Send `0xFF` to leave the test (reply `0x01`); the host is still in `BBIO1`.  Send `0x0F` to return to the terminal (reply `0x01`, then the banner and `HiZ>`)."  Added "both need every target disconnected and the board powered".  `binary.md` limits: "the header pins are left in HiZ, but the host is still in `BBIO1`". | `Firmware/protocol_binary/binary_io.c` `binary_io_self_test()` sends `0x01` on `0xFF` and returns to `BBIO1`; `handle_bitbang_command()` `0x0F` leaves the layer.  `perform_selftest(false, jumper_test)` is the same full/short engine. |
| 3 | cross-set | No selftest change.  `checks.md` already marks `Bus Hi-Z 1` as skipped in the `0x10` column, with v3 `Vpu` and v4 yes, and says "`Bus Hi-Z 1` runs only in the full pass, even on v4 where no external jumper is needed." | `Firmware/capability_selftest/selftest.c`: the third pin group is inside `if (jumper_test)`, so `0x10` skips it on both boards; v3 `Vpu` is the pull-up reference.  `docs/protocol/binary/commands.md` calls it "the jumper-only pin group", which is wrong for v4. |
| 4 | cross-set | No selftest change.  `checks.md`: "The `0x01` is a refusal count, not one failed check"; `binary.md` count table row for "`0x01` while the SMPS owns the ADC" says "refusal; no check ran". | `Firmware/capability_selftest/selftest.c` `perform_selftest()` returns 1 before `errors = 0` and before any check; `binary_io_self_test()` sends that return value as the count.  `docs/protocol/binary/commands.md` says "(one error)", which reads as a failed check. |
| 5 | fixed | `checks.md`: the 7-column block tables are now a 5-column matrix (check, jumper, `0x10`, v3, v4) plus 3-column detail tables (check, what it does, if it fails).  `edge-cases.md`: the 4-column tables are now per-case headings with Symptom/Behaviour/Recovery bullets.  No check content was dropped. | The former 255-character `READ&WRITE` row and 332-character edge-case row are gone; every table is 2–5 columns and the widest row is under 170 characters. |
| 6 | fixed | `terminal.md`: "`ADC(...) FAIL` alone", "`VPU(...) FAIL`", "if `3.3V(...)` also fails".  `edge-cases.md`: "`ADC(...) FAIL` (for example `ADC(0.00) FAIL`)", plus `VPU(...)`, `5V(...)`, `3.3V(...)`, `Vusb(...)`, `5V VPU(...)`, `3.3V VPU(...)`. | `Firmware/capability_selftest/selftest.c` `perform_adc_test()` prints `'('`, `bp_write_voltage()`, `')'` before `check_result()`; `check_result()` prints `" OK"` or `" FAIL"`.  Non-measurement checks print only the label. |
| 7 | fixed | `checks.md`: `WP` "reads the on-board EEPROM write-protect line high"; `READ&WRITE` "writes, reads back and restores one EEPROM byte"; the paragraph says "`WP` reads the line high first; `READ&WRITE` then drives the same line low for the write and restores the saved level (high after a normal `WP` pass)." | `Firmware/core_eeprom/onboard_eeprom.c` `eeprom_test()` saves `EEPROM_WP_LAT`, sets it HIGH, reads the original, sets it LOW for the write, and restores `write_protection` at the end.  `selftest.c` calls `eeprom_test()` for `READ&WRITE`. |
| 8 | fixed | `checks.md` `PULLUP L` row: "requests the pull-ups off and reads the control line low".  The paragraph: "A `PULLUP L` pass only proves the control line read low after the pull-ups were requested off.  A line stuck low also passes it.  Read it with `PULLUP H` and `Bus Hi-Z 1`, which exercise the same control line".  Added the same limit to "What is not checked". | `Firmware/capability_selftest/selftest.c`: `bp_disable_pullup(); BPMSG1168; check_result(BP_PULLUP, LOW);`; a stuck-low line reads LOW.  `PULLUP H` calls `bp_enable_pullup()` and expects HIGH; the `Bus Hi-Z 1` group expects the pulled-up pins HIGH. |
| 9 | fixed | `terminal.md` new "The run (v3, full test)" order list: "`Ctrl`: `AUX`, `MODE LED`, `PULLUP H`, `PULLUP L`, `VREG`"; "`ADC and supply`: `5V`, `VPU`, `3.3V`, `ADC`"; then the three pin groups; ends "The v3 column is **not verified on v3**." | `Firmware/capability_selftest/selftest.c` `#elif defined(BUSPIRATEV3)` order: 5V, VPU (if `jumper_test`), 3.3V, ADC (if `jumper_test`).  The EEPROM block is inside `#ifdef BUSPIRATEV4`. |
| 10 | fixed | `wiring.md`: `[terminal.md](terminal.md#setup)`.  `terminal.md`: `[checks.md](checks.md#voltage-checks)` and `[checks.md](checks.md#when-the-run-does-not-start)`.  `binary.md`: `[checks.md](checks.md#when-the-run-does-not-start)`. | Headings `## Setup`, `## Voltage checks`, `## When the run does not start` exist.  A link/anchor checker over all six files resolves every relative link and anchor. |
| 11 | fixed | `wiring.md` Related docs adds "`- [binary.md](binary.md) — the `0x10` / `0x11` passes.`" | `binary.md` describes both passes and the binary entry/leave. |
| 12 | fixed | `README.md` availability row lists `Vusb`, `5V VPU` and `3.3V VPU` as v4-only. | `Firmware/capability_selftest/selftest.c` v4 block prints `Vusb` (BPMSG1270), `5V VPU` (BPMSG1171 + BPMSG1172), `3.3V VPU` (BPMSG1173 + BPMSG1172); v3 has none. |
| 13 | fixed | `checks.md` Names: "The `?` help lists the command as `Selftest`; this set writes self-test in prose." | `Firmware/core_messages/messages_v4.s` and `messages_v3.s` help strings are `"~\tSelftest..."`; the set title and prose use "self-test". |
| 14 | cross-set | No selftest change.  The selftest README already says the run "takes over the pins and rails". | `docs/generic/README.md` "Nothing is driven except the selected AUX pin" is contradicted by a `~` self-test in HiZ.  It needs the qualification in the cross-set list below. |
| 15 | cross-set (caveated) | No selftest change.  `checks.md` now says the terminal refusal is normally unreachable; the binary case is the reachable one. | `docs/generic/edge-cases.md` lists `d`, `D`, `P`, `e`, `g`, `S`, `v` but not `~`.  `perform_selftest()` does call the ADC guard, so `~` is technically affected, but a normal `HiZ>` prompt means the reset that returned there stopped the SMPS.  Add `~` only with that caveat, or leave the list and link to `checks.md`. |
| 16 | fixed | `wiring.md` Before step 2: "On v3 remove any external voltage from `Vpu` before fitting the `Vpu`→`+5V` jumper; on v4 leave `Vpu` unconnected."  Pins table `Vpu` row: "v4: no jumper, no external voltage". | The v3 `Vpu` pin is the pull-up reference (wiring table; `bp_enable_pullup()`); an external voltage on it would be tied to `+5V`.  v4 selects internal 3.3V/5V sources (`hardwarev4.h` `bp_enable_3v3_pullup()` / `bp_enable_5v0_pullup()`).  Documentation-only safety note; the firmware does not detect it. |

## Counts

- fixed in the selftest set: 12 (findings 1, 2, 5, 6, 7, 8, 9, 10, 11, 12,
  13, 16).  Finding 1 also has a cross-set half.
- rejected / not a defect: 0 findings.  One sub-recommendation in finding 1
  was rejected: the `NORMAL` button is the bootloader button, not a reset.
  Finding 15's premise is qualified rather than rejected.
- code-owner candidates: 0.  No firmware bug was found.  The `0x01`
  refusal/count ambiguity is a design limitation, not a defect.
- cross-set: 4 findings (3, 4, 14, 15) plus the SMPS-page half of finding 1;
  five correction items in the list below.

## Files changed

- `docs/capabilities/selftest/README.md`
- `docs/capabilities/selftest/wiring.md`
- `docs/capabilities/selftest/terminal.md`
- `docs/capabilities/selftest/checks.md`
- `docs/capabilities/selftest/binary.md`
- `docs/capabilities/selftest/edge-cases.md`
- `docs-sweep/fixes-selftest-round1.md` (this record)

## Cross-set changes for the main session

1. **SMPS set (finding 1).**
   `docs/capabilities/smps/README.md` lines 32–35, `docs/capabilities/smps/binary.md`
   lines 21–25, and `docs/capabilities/smps/edge-cases.md` lines 63–68 all say
   the v4 terminal reset is "not confirmed" to stop the SMPS.  The code says it
   does: `Firmware/core_system/base.c` `bp_reset_board_state()` calls
   `smps_stop()` under `BUSPIRATEV4` + `BP_ENABLE_SMPS_SUPPORT`;
   `proc_menu.c` `#` and the v4 `0x0F` leave both reach `reset_terminal_state()`
   → `bp_reset_board_state()`.  `docs/protocol/binary/commands.md` §Reset
   ("an SMPS feedback loop stopped") and
   `docs/protocol/binary/entering-and-framing.md` §Leaving ("On an
   SMPS-enabled build it also stops the SMPS feedback loop") already agree,
   and `docs-sweep/fixes-binary-round1.md` finding 3 recorded the same path.
   Replace the caveat with the code-verified statement: `0xF1` in raw wire
   stops it; a hardware reset stops it; on v4 the terminal reset (`#` or the
   `0x0F` leave) also stops it.  The add-on board's electrical behavior can
   stay `unverified`; the firmware stop path cannot.
2. **`docs/protocol/binary/commands.md` `0x10` summary (finding 3).**  Replace
   "the pass without the external-jumper steps and without the jumper-only pin
   group" with "the pass without the jumper-fed checks and without the
   `Bus Hi-Z 1` group (skipped on both boards; on v3 that group also needs the
   `Vpu` jumper)".
3. **`docs/protocol/binary/commands.md` ADC-busy sentence (finding 4).**
   Replace "both commands report a count of `0x01` (one error) without running
   the test" with "both commands report a count of `0x01` (refusal count; no
   check ran) without running the test".
4. **`docs/generic/README.md` §Idle mode (HiZ) (finding 14).**  "Nothing is
   driven except the selected AUX pin" needs the qualification "in normal HiZ
   operation; a `~` self-test temporarily drives the pins and rails."  The
   next paragraph already says the self-test runs there.
5. **`docs/generic/edge-cases.md` ADC-busy row (finding 15).**  The row lists
   `d`, `D`, `P`, `e`, `g`, `S`, `v` but not `~`.  Add `~` with the caveat
   that a normal `HiZ>` prompt means the SMPS is already stopped, or leave the
   list and link to
   `docs/capabilities/selftest/checks.md#when-the-run-does-not-start`.  Do not
   present a routine terminal `~` refusal.

## Additional corrections (not numbered findings)

- Replaced "NORMAL button" with "v4 reset button" in the run-stop and SMPS
  recovery prose.  The NORMAL button is the bootloader button
  (`Firmware/hardwarev4.h` `BP_BUTTON` / `BP_BUTTON_SETUP()`); the self-test
  never polls it.  This also rejects the audit's finding 1 suggestion to use
  the NORMAL button.
- `edge-cases.md` "binary `0x11` with a missing jumper": the old "with no
  jumpers" row said v3 `0x05` or `0x06`; it now says v4 `0x01` for the `ADC`
  jumper, v3 `0x05` for `Vpu`, and `0x06` for both.
- `edge-cases.md` sub-protocol recovery now notes that the XSVF player ignores
  `0x00`, matching `docs/protocol/binary/edge-cases.md`.

## Remaining questions / observations

- **NORMAL button vs reset button.**  The audit's finding 1 proposed the "v4
  NORMAL button or power-cycle" as the hardware reset.  The firmware defines
  `BP_BUTTON` on RC14 for bootloader entry and leaves `IEC1bits.CNIE = 0`;
  the self-test never polls it.  The pages use "v4 reset button or
  power-cycle".  If the board silkscreen or the project's naming prefers
  another term, use it consistently.
- **v3 order and counts.**  The new v3 order list and the v3 failure counts are
  source-derived; no v3 hardware was available.  The pages keep "not verified
  on v3".
- **SMPS electrical behavior.**  The firmware stop path is code-verified, but
  the legacy add-on board is not available, so its electrical response remains
  `unverified`.  The `0xF2 0x58` request is an example, not a calibration.
- **`0x01` ambiguity.**  A refusal and one failed check both return `0x01`.
  The host resolves it by knowing whether the SMPS is running; the binary
  page says so and tells the host how to leave the self-test and the layer.
  This is inherent in the one-byte protocol; no firmware change was made.
- **Physical header labels.**  The wiring pages assume the board labels match
  `ADC`, `+3.3V`, `Vpu`, `+5V`.  No pin-location map was added.
- **Display state.**  The post-run table covers pins, rails, AUX and bus mode;
  it does not claim anything about `o`/`l`/`;` display state.  No new evidence
  was available, so no claim was added.
- **`PULLUP L`.**  The stuck-low limitation is documented in `checks.md`; it is
  a check-coverage limit, not a firmware fault.
- **No commit / no WORKLOG.**  The changes are left in the working tree for the
  main session.
