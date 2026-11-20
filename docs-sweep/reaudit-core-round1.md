# Core documentation — re-audit, round 1

I re-read the current `docs/core/` files plus the changed passages in
`docs/generic/commands.md`, `docs/generic/navigation.md`,
`docs/dev/building.md` and `docs/dev/tooling.md`.  This is an item-by-item
check of the round-1 findings, not a fresh full review.

**Verdict: 20 of 20 items filled in full.**  Four new minor issues were
introduced by the fixes; they are listed at the end.  No Major findings, so the
two-round cap is not extended.

## Per-item re-audit

| # | Finding | Status | Current evidence | Remainder |
|---|---|---|---|---|
| 1 | User-macro syntax | Filled in full | `system.md:273`: "`<n=text>` ... the first `>` ends the body ... see [macros](../generic/macros.md)" | None. Matches `generic/macros.md` and the source. |
| 2 | `t` timer range contradiction | Filled in full | `system.md:137`: "returns a 31-bit microsecond count ... wraps every 35.8 minutes"; `generic/commands.md:97`: "`t` reads a 31-bit microsecond count, so it wraps after about 35.8 minutes." | New minor N1: the parenthetical explanation of the shift is wrong. |
| 3 | Board reset buried under the reset/exit heading | Filled in full | `system.md:526`: "### Board-state reset (`bp_reset_board_state()`)"; `:531`: "1. Release the bus pins ..."; `:544-553`: caller table; lifecycle `:433` points here. | New minors N2/N3 in step 4. |
| 4 | 617-line `system.md` with no contents | Filled in full | `system.md:13`: "## Contents"; grouped anchors cover Startup/state, Runtime, Modes, Recovery. | Split deferred; page is 667 lines and the rationale is recorded. The contents block makes the sections reachable. |
| 5 | README module map and layer table were cell walls | Filled in full | `README.md:74`: "Module map" is now three columns (`Group`, `Source files`, `Detail`); `:117-126` replaces the 477-character layer row with one row per source group. | README is 150 lines (target ≈120); the maps are now scannable. |
| 6 | README had no failure-mode route and a dead-end test paragraph | Filled in full | `README.md:17`: "## Maintainer index" with rows for hung terminal, mode engage, I2C timeout, EEPROM; `:109-113` points to "dev/tooling.md#test-suites". | Index has nine rows; links resolve. |
| 7 | Registry table contradicted its LCD paragraph / repeated menu 9 | Filled in full | `system.md:329`: "Default v4 image (KEYB on, LCD off)"; `:347-352`: four-combination KEYB/LCD matrix; numbers are distinct in every row. | New minor N4: the matrix header is ambiguous. |
| 8 | Descriptor null-callback contract was ambiguous | Filled in full | `system.md:408-410`: "If a field is omitted ... it stays NULL ... A descriptor the dispatcher can reach must assign every required field"; `:412-424`: Required?/Reusable body table; `:421-424`: DIO exception and NULL consequence. | The NULL claim is verified: `clock_pulses` is the only dispatcher NULL test. |
| 9 | USB lock/span/host-not-reading recovery was missing | Filled in full | `usb.md:179-180`: "The lock is one flag, not a reference count: do not call `setLock()` twice and then `freeLock()` once"; `:198-200`: state-drop/SET_CONFIGURATION invalidation; `:219`: prompt-stalls/recover-by-draining row. | Failure table names static `putda_cdc()` as a blocking point, which is accurate but not a public API. |
| 10 | Messages regeneration omitted concrete commands / cwd trap | Filled in full | `messages.md:131-132`: "The output prefix is relative to the process working directory; run the generator from `tools/packstrings/`"; `:135-137`: both invocations inline. | None. |
| 11 | Bitbang sticky-error recovery lacked the caller | Filled in full | `bitbang.md:112-115`: "The I2C terminal wrappers ... clear it at the start of each transaction ... a raw caller must call `bitbang_i2c_clear_error()` ... `bitbang_setup()` does not clear it"; `:177`: table row repeats the per-transaction recovery. | None. |
| 12 | Stretch counter had two names | Filled in full | `bitbang.md:20`: "`bitbang_clock_stretch_timeouts()` reads the internal saturating count"; `:93`: same function in the API group. | None. |
| 13 | Pipe key written as "pipe" | Filled in full | `system.md:263`: the key cell now contains `=` and the escaped pipe, not the word "pipe". | The escaped pipe is correct; a naive pipe counter sees an extra delimiter, but the table renders with three cells. |
| 14 | Exact `1-WIRE speed:` message split across a code span | Filled in full | `system.md:363-365`: "`1-WIRE speed: standard` and `1-WIRE speed: overdrive`." | Two exact payloads on one line each. |
| 15 | Device ID/revision called "flash configuration words" | Filled in full | `system.md:116-119`: "device ID/revision words in the config-memory region (`0x00FF0000` and `0x00FF0002`), not from the user `CFG0`..`CFG2` words". | None. |
| 16 | Core/dev disagreement about `test_native.py` | Filled in full | `README.md:109-111`: "the only runner is that per-suite script ... There is no aggregate runner and no `test_native.py` in this tree"; `dev/tooling.md:45`: "## Test suites"; no `test_native` reference remains in `docs/dev/`. | None. |
| 17 | Direct `m<N>` omission stated differently in generic | Filled in full | `generic/navigation.md:202`: "the `Clutch` line and the finish-setup prompt are printed only by the menu form". | The lower paragraph only mentions the Clutch line, but it is not contradictory; the table row is now correct. |
| 18 | Failure modes were bullet chains | Filled in full | `system.md:576` starts the four columns `Trigger`, `Symptom`, `Behaviour`, `Recovery`; the same table is in `usb.md:216`, `bitbang.md:171`, `eeprom.md:148` and `messages.md:149`. | All five pages are tables; no failure bullet chain remains. |
| 19 | Drift-prone catalog/self-test counts | Filled in full | `messages.md:25-30`: "The v4 catalog carries the shared set plus ... Counts are build artifacts, not a contract"; `eeprom.md:180`: "the full self-test value/WP-entry matrix". | A test-input range (`lengths 0..512`) remains in `messages.md:182`; it is not a catalog or pass count. |
| 20 | README had no "start here" | Filled in full | `README.md:9-10`: "New to the core: read `system.md` first, then the module you are changing; each page ends with its suite and related pages." | None. |

## Presentation check

| Rule | Result |
|---|---|
| No TAB characters | None in the six core files, or in the changed `generic/` and `dev/` passages. |
| Aligned captures | No capture was altered; code blocks are intact. The only trailing space is the documented trailing space in the generic banner (`navigation.md:78`), which is part of the displayed firmware string. |
| Menus / option lists as tables | All menus, the registry matrix and all five failure-mode sections are tables. No option bullet chain remains. |
| ~80-column wraps | Core prose is within ~80 columns. The only >80 lines are the two command lines in the `messages.md:135-137` code block (91 characters), which are code, not prose. |
| No bench prose, dates, tested/verified-result prose | No dates, bench/rig/capture narratives, pass counts, provenance or internal log names. The only "verified" occurrences are the allowed "not verified on v3 hardware" caveats. The per-page "Native test coverage" paragraphs describe suites, not run results. |
| Table/link integrity | All table pipe counts are consistent apart from the intentional escaped `\|` in `system.md:263`; all core links and anchors resolve, including the new `#board-state-reset-bp_reset_board_state`. |

## New problems (minor, caused by the fixes)

1. **[minor] `system.md:137` — the explanation of the 31-bit width is wrong.**
   Quote: "the final right shift discards the top bit".
   `bp_timer_microseconds()` returns `((overflows << 16) | ticks) >> 1`;
   the right shift drops the **low** bit and zero-fills the top bit, leaving a
   31-bit count.  The range (35.8 minutes) is right; the parenthetical is not.
   **Fix:** "the final right shift drops the low bit and leaves a 31-bit count."

2. **[minor] `system.md:535` — "set up the internal I2C pins" names the wrong pins.**
   Quote: "v4: set up the internal I2C pins and button, disable the onboard 3.3 V pull-up source and call `eeprom_initialize()`".
   `BPV4_HWI2CPINS_SETUP()` sets TRISE5-7, the secondary hardware-I2C/SPI
   pins (its source comment says "secondary MISO/CLK/MOSI connection setup"),
   not the onboard EEPROM's I2C1 on RD9/RD10.  The EEPROM bus is handled by the
   `eeprom_initialize()` call in the same step.
   **Fix:** "set up the secondary hardware-I2C pins and the button, then ...".

3. **[minor] `system.md:535` — the 5 V VPU source is also disabled, not just 3.3 V.**
   Quote: "disable the onboard 3.3 V pull-up source".
   `bp_disable_3v3_pullup()` sets both `BP_PUVSEL50_DIR` and `BP_PUVSEL33_DIR`
   to input, so both onboard VPU sources are released.  The old summary said
   "3.3 V/5 V source released"; the new step under-describes it.
   **Fix:** "disable both onboard VPU sources (3.3 V and 5 V)".

4. **[minor] `system.md:347` — the registry-matrix header is ambiguous.**
   The header is:
   ```
   | KEYB | LCD | `KEYB` | `LCD` | `PIC` | `DIO` |
   ```
   The first two columns are build flags (`off`/`on`), the next four are menu
   numbers (`8`..`11`), but both use the same labels.  A reader can infer it
   from the values, but the table does not say which is which.
   **Fix:** "KEYB built | LCD built | KEYB menu | LCD menu | PIC menu | DIO menu".

## Bottom line

All 20 round-1 findings are resolved.  The four remaining issues are minor
accuracy/clarity defects in the newly added text, not failures of the original
fixes.  No Major findings; no additional audit turn is required by the process
cap.
