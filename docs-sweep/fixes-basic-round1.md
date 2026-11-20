# BASIC docs — round 1 fixes

Scope: `docs/tools/basic/**` only.  No firmware, generic-set, protocol-set,
SPEC or WORKLOG edits.  No commit.

**Result:** 27 findings processed — 26 fixed in-set, 1 rejected as not a
defect (finding 6; the BASIC text is correct and the conflicting page is in
another set).  Finding 5 is fixed in-set *and* needs a cross-set correction.
0 confirmed firmware defects, so no code-owner candidate from the writer's
list.  Two required cross-set corrections (plus one optional row-range check)
are listed at the end.

Verification was against the combined firmware tree, the checked-in native
BASIC tests and a scratch harness that runs the production interpreter with
the same stubs (not added to the tree).

## Per-finding record

| # | Decision | Change (file + short quote) | Evidence |
|---:|---|---|---|
| 1 | fixed | `entry-and-prompt.md`: “five to nineteen NULs, then the Left-arrow key (`0x02`)” and “The two windows do not overlap: the twentieth NUL enters binary first”. `edge-cases.md`: “20 NULs, or 5–19 NULs plus Left arrow”. | `Firmware/core_system/proc_menu.c`: `update_binary_prefix()` clears the counter on any byte except NUL/STX; `case ASCII_NUL` enters binary at `== 20`; `handle_left_arrow()` enters SUMP at `>= 5`. `Firmware/protocol_binary/binary_io.c`: `BITBANG_COMMAND_I2C` = `0x02`, so a Left arrow after 20 NULs selects binary I2C. |
| 2 | fixed | `board-pins.md`: “The terminal `t` prints the same timer in microseconds: 1 µs units, 31-bit, wrapping after about 35.8 minutes; `T` resets it.  BASIC `TIME` is `t / 64`: 16-bit, wrapping about every 4.19 s.” `bus-access.md` row: “BASIC counts 64 µs units (16-bit), terminal `t` is 1 µs (31-bit)”. | `Firmware/tool_basic/basic.c`, `case TOK_TIME`: `(uint16_t)(bp_timer_microseconds() >> 6)`. `Firmware/core_system/base.c`, `bp_timer_print()` prints that microsecond value directly (the 31-bit range); `Firmware/core_system/proc_menu.c`, `case 't'`. `docs/generic/commands.md` already says 1 µs and 31-bit. Native `TIME` check (harness) returns 1000 for a 64000 µs stub. |
| 3 | fixed | `examples.md`: “The program above therefore needs an external Vpullup supply on v4, and on v3 in every case.” and “delete line 10 and do the I2C setup at the terminal … `W` also switches the target supply on”. | `proc_menu.c`, `bp_change_mode()` runs `bp_reset_board_state()`, which calls `bp_disable_3v3_pullup()`; `Firmware/hardwarev4.h` sets both PUVSEL directions to input there, so the on-board source is off. `Firmware/protocol_i2c/i2c.c`, `i2c_setup_execute()` configures pins only, never a Vpullup source. |
| 4 | fixed | `board-pins.md`: “`AUX 0` is a **drive-low**, not a release.  To release the AUX pin, read it (`PRINT AUX;`) or reset the board.” | `Firmware/capability_aux/aux_pin.c`: `bp_aux_pin_set_low()` sets `LAT` low and `DIR = OUTPUT`; `bp_aux_pin_read()` sets `DIR = INPUT` first. |
| 5 | fixed (+ cross-set) | `board-pins.md`: “The terminal probe commands use the same check and are refused the same way; a sample needs the SMPS stopped (see SMPS).” `edge-cases.md`: “stop the SMPS first; the terminal probe is refused too”. | `basic.c`, `case TOK_ADC` tests `bp_adc_foreground_available()` and sets the expression error. `core_system/base.c`, `bp_adc_check_foreground()` returns false while `IEC0bits.AD1IE` is on and prints “ADC busy: SMPS feedback active; command rejected.” `capability_adc/adc.c`: both `bp_adc_probe()` and `bp_adc_continuous_probe()` call it. Native tests: `test_basic.py` ADC group, `test_core_adc_ownership.py`. |
| 6 | not a defect | No correction to the claim; the row now reads “if the optional SMPS owns the output-compare channel, `FREQ`/`DUTY` do nothing and report no error; the terminal `g` is refused instead”. | `capability_aux/aux_pin.c`, `bp_update_pwm()` returns before touching the timer when `!bp_adc_foreground_available()`; `bp_update_duty_cycle()` routes through it. `bp_pwm_setup()` calls `bp_adc_check_foreground()` and refuses the terminal path. The stale “each reprograms the channel” sentence is in `docs/capabilities/auxiliary/pwm-servo.md` (cross-set). |
| 7 | fixed | `bus-access.md`: “`AUXPIN` accepts 0 and 1 on v3 and 0..3 on v4; a larger value stops the program with `Error(3)` and leaves the target unchanged.” `board-pins.md`: “v3 accepts 0 and 1; v4 accepts 0..3.” `edge-cases.md`: “`Error(3)` at run time … the line is stored without complaint”. | `basic.c`, `case TOK_AUXPIN`: the bounds check sets `STATUS_CODE_SYNTAX_ERROR` (3) at run time; there is no entry-time validation. `test_basic.py`, `auxpin()` expects `Error(3)` for -1/4/256 on the v3 build and accepts 0..3 on v4. Harness check: `10 AUXPIN 2` stores silently, then `Error(3)` on v3, valid on v4. |
| 8 | fixed | `bus-access.md` reset table adds “bus engagement \| the new mode's setup/engage step runs (the step `W` runs), without `W`'s voltage checks or `Clutch engaged!!!` message”; adds “`MODE` leaves the bus engaged but unpowered.  A target with its own supply needs no `PSU` statement”. | `proc_menu.c`, `bp_change_mode()` = cleanup → `bp_reset_board_state()` → `setup_prepare()` → `setup_execute()`. `switch_psu_on()` does the ADC/voltage checks and then calls `setup_execute()` plus `MSG_CLUTCH_ENGAGED`. The terminal `changemode()` only calls `setup_prepare()`, which is why the terminal path needs `W` and `MODE` does not. |
| 9 | fixed | `language.md` §Errors opens with an entry/run table: “typing a line \| `Syntax error`…”, “typing a line \| `Too long!`…”, “running \| `Error(N) @line:L @pgmspace:P`”.  Code 2 is “trailing text after a complete statement (`START 1`, `INPUT A, B`)” and code 3 adds the `AUXPIN`, `FREQ`/`DUTY` and SMPS-ADC triggers; “Code 1 is not an error: it is the interpreter's internal "program finished" state.” | `basic.c`: `basic_status_code_t` (1 = `STATUS_CODE_FINISHED`, never printed; 2 = partial; 3 = syntax). `bp_basic_enter_interactive_interpreter()` prints `BPMSG1052` (`Syntax error`) and `BPMSG1051` (`Too long!`). Harness: `START 1` and `INPUT A, B` → `Error(2)`. |
| 10 | fixed | `board-pins.md`: “**Do not use `PSU 1` into a self-powered target.**  It bypasses `W`'s voltage checks, so it can back-drive the target's supply.” | `basic.c`, `case TOK_PSU` calls `bp_set_voltage_regulator_state()` directly. `proc_menu.c`, `switch_psu_on()` is the checked path and is what `W` uses. |
| 11 | fixed | `README.md`: “PWM `FREQ`/`DUTY`” in the five-minute list; new limit bullet “Terminal commands with no BASIC token: `f` (frequency counter), `v` (status), `~` (self-test), `O` (1-Wire overdrive) and `o` (display mode).” | `basic.c` token table: no token for any of those commands; `FREQ` is `TOK_FREQ` and calls `bp_update_pwm()`; `f` is the terminal counter in `proc_menu.c`. |
| 12 | fixed | `bus-access.md` row: “value entry: `123` (the terminal prompt also takes `0x7B`, `0b1111011` and `"text"`)” and the note “`SEND 0x7B` sends `0` and then stops with `Error(2)` … `SEND "A"` stops with `Error(3)`”. `edge-cases.md` adds the `SEND 0x7B` row. | `basic.c`, `assign()`/`get_number_or_variable()` parse decimal only; the leftover `x` is a partial instruction. Harness: `SEND 0x7B` → `Error(2)`, `SEND "A"` → `Error(3)`. |
| 13 | fixed | `README.md` limits: “no user-defined or built-in numeric functions (`ABS`, `INT`, `RND`), no `AND`/`OR` keywords”. | `basic.c` token table has no such tokens; `language.md` and `limits-and-variants.md` already said so. |
| 14 | fixed | `README.md`: “Summary tables:” (was “Full tables”) plus “The per-mode sets are linked from the mode table in [generic/README.md]”. `bus-access.md`: “The per-mode sets, which own the full pin tables, are linked from the mode table in [generic/README.md]”. | `docs/protocol/README.md` does not exist, so the old “mode sets linked from the README” claim could not resolve. `docs/generic/README.md` §Modes has the per-mode link table. |
| 15 | fixed | `entry-and-prompt.md`: “the `65535  END` marker” (two spaces) and “Line 10 is 8 bytes, line 20 is 4 bytes and the `65535  END` marker is 4, 16 in all; `LIST` prints `15` because the count stops at the terminating zero byte.” `limits-and-variants.md` now cross-refs this instead of repeating it. | `basic.c`, `list()` prints `bp_write_dec_word(basic_program_counter - 1)` after the walk stops on the first zero byte. Harness checked the stored headers: 8 + 4 + 4 = 16, listing “15 bytes.” and the two-space marker line. |
| 16 | fixed | `bus-access.md` row “`m` / `m<N>`”; `entry-and-prompt.md` “change mode with `m` or `m<N>`”. | `proc_menu.c`, `changemode()` accepts `m4` and `m 4`; `docs/generic/navigation.md` writes `m<N>`. |
| 17 | fixed | `examples.md`: “A `2WIRE` program has the same pull-up and `MODE`-reset caveat”. | `Firmware/core_system/core.c` names the mode `2WIRE`; the same file's mode table in `bus-access.md` uses `2WIRE`. |
| 18 | fixed | General v3 caveat now appears once in `README.md` status and once in `limits-and-variants.md`; removed from `bus-access.md` §v3/v4 and the old `edge-cases.md` §Unverified behaviour.  “unverified” remains only on the v3 ADC divider (a specific number). | Documentation status only; no behaviour claim. |
| 19 | fixed | `board-pins.md` TIME paragraph is rewrapped; the whole set now has no prose line over 80 columns (tables excluded by design). | Presentation scan. |
| 20 | fixed | `board-pins.md` §PWM is now a range/option table (frequency, duty, clamping, separators, operands, stored form, output, accuracy, SMPS, HiZ) followed by the two examples. | `basic.c` `TOK_FREQ`/`TOK_DUTY` clamp to `PWM_MINIMUM_FREQUENCY`/`PWM_MAXIMUM_FREQUENCY` and 1..100 %; the tokenizer stores the space between two decimals as `,`; the output is hard-wired to `AUXPIN_RPOUT = OC5_IO` (the `AUX` pin); `bp_update_pwm()` owns the SMPS guard. |
| 21 | fixed | `examples.md` gains a “Task / Section” index and two sections: “A GOTO loop” and “Bit-banging clock and data” (3WIRE, `MODE 7`). | Harness ran the GOTO program (prints `123`) and the 3WIRE program (no error). `raw_common.h`: `DAT` writes/reads `MOSI`/`MISO`, `CLK 2` pulses; `raw3wire_stop()` returns CS to idle but leaves the clock/data outputs at their last levels. |
| 22 | fixed | `language.md` code 2 is now “trailing text after a complete statement (`START 1`, `INPUT A, B`)”; the section says code 1 is the internal finished state. | Harness: `START 1` and `INPUT A, B` → `Error(2)`; `basic_status_code_t` code 1 = `STATUS_CODE_FINISHED`. |
| 23 | fixed | `board-pins.md` accuracy row: “whole timer ticks, at or above the requested frequency and at or below the duty; 1 Hz is 62,500 counts of 16 µs, 100 Hz 20,000 of 0.5 µs, 4 kHz 4,000 of 62.5 ns”; the `pwm-servo.md` link is qualified as written for the terminal kHz range. | `capability_aux/aux_pin.c`, `setup_prescaler_divisor()` (1:256/1:64/1:8/1:1, with the 1 Hz case forced to a 62,500 Hz tick) and `bp_update_pwm()`'s `period = timer_frequency / frequency - 1`. |
| 24 | fixed | `bus-access.md`: “Press the board's reset button or power-cycle to recover from a macro that waits for input; `#` cannot be typed while the macro holds the interpreter.” `edge-cases.md` repeats the reset/`EXIT` note. | `proc_menu.c`: `#` is only a terminal command; `bp_basic_enter_interactive_interpreter()` knows only `RUN`/`LIST`/`EXIT`/`NEW` (plus build options), so `#` and `EXIT` are unavailable while the macro owns the input loop. |
| 25 | fixed | `entry-and-prompt.md` now shows the breakdown (8 + 4 + 4 = 16, listing 15) instead of “15 is a 16-byte program”; `limits-and-variants.md` cross-refs it. | Same evidence as finding 15. |
| 26 | fixed | `bus-access.md` matrix cells: “UART \| display only \| display only”, plus the bullet “In `UART`, `START` and `STOP` only switch the terminal's live display and echo on and off; neither puts a byte on the wire.” | `Firmware/protocol_uart/uart.c`, `uart_start()`/`uart_stop()` set `echo_uart` and `mode_configuration.periodicService` and print the live-display messages; no bus write. |
| 27 | fixed | “Vpullup source” used consistently in `README.md`, `bus-access.md`, `board-pins.md` and `limits-and-variants.md`; `board-pins.md` adds the mapping “the `v` table prints the pin as `VPU`”. | `Firmware/hardwarev4.h` `BP_PUVSEL33`/`BP_PUVSEL50`; `messages_v4.s` `"VPU"` and the `v` table header; `docs/generic/README.md` glossary already maps Vpullup to `VPU`. |

**Extra, not a numbered finding:** `language.md` now shows the compound-
condition idiom the language needs without `AND`/`OR` — parenthesise each
comparison and combine with `*` (both) or `+` (either).  Both forms were run
in the harness.  `README.md` also gained a one-line “Start here” path
(implementation of the audit's structural proposals 1–2).

## Verification run

- `python Firmware/tests/test_basic.py --group all` — PASS 219 checks on the
  BUSPIRATEV4 build and 217 on the BUSPIRATEV3 build.
- Scratch harness (same production `basic.c` and stubs, 21 added checks on each
  build): `AUXPIN 2` stored silently then `Error(3)` on v3 / valid on v4;
  `START 1` → `Error(2)`; `INPUT A, B` → `Error(2)`; `SEND 0x7B` → `Error(2)`;
  `SEND "A"` → `Error(3)`; `CLK 3` silent; the 8+4+4 byte breakdown;
  `FREQ 0 50` calls PWM with 0; the GOTO example prints `123`; the 3WIRE
  example runs clean; `INPUT , A` → `Error(3)`; `TIME` is µs/64; 31 statement
  bytes fit and 32 are rejected; both compound-condition idioms (`*` and `+`)
  print the expected branch.
- Local link/anchor check over all eight files: every relative link and `#`
  anchor resolves.
- Presentation scan: no TABs, no prose line over 80 columns.

## Writer-raised firmware candidates — classification

| Item | Verdict | Evidence |
|---|---|---|
| `PULLUP` inversion | not a firmware defect | `basic.c`, `case TOK_PULLUP: return !BP_PULLUP;`. The native `pullup()` test asserts the complement (`PULLUP 0` reads 1, `PULLUP 1` reads 0). The docs now state it as the token's contract; changing it would break the checked-in expectation. |
| `INPUT , A` rejection | not a firmware defect | `basic.c`, `case TOK_INPUT` requires `A`..`Z` after the optional quoted prompt; `test_basic.py` lists `INPUT , A` in its bad set and the harness confirms `Error(3)`. |
| BASIC PWM Hz / 4 kHz vs terminal kHz | not a defect | `bp_update_pwm()` is an Hz API used by BASIC; `bp_pwm_setup()` is the separate terminal kHz implementation. The 4000 ceiling is BASIC's `PWM_MAXIMUM_FREQUENCY`; the terminal prompt says `1kHz-4,000kHz`. |
| Partial-instruction errors | not a defect | `basic.c` returns `STATUS_CODE_PARTIAL_INSTRUCTION_ERROR` (2) when a completed statement is followed by a byte that is not a line terminator. The docs now give the concrete triggers. |
| `DEBUG` build | not a defect | `Firmware/configuration.h` `#undef BP_BASIC_DEBUG_COMMAND`; the native test builds the dump only with `--debug` and checks the hex output. |
| PWM in HiZ | not a defect (deliberate direct call) | `bp_update_pwm()` has no HiZ guard; the terminal `g` handler refuses HiZ in `proc_menu.c`. The board-pins HiZ section warns about exactly this. |
| `AUXPIN` 2/3 | not a defect | v3 bounds check returns status 3 at run time; the line stores without complaint. Verified above. |

No item on this list is a confirmed firmware bug, so there is no code-owner
candidate from the audit or the writer's list.

## Cross-set changes for the main session

1. **`docs/capabilities/smps/edge-cases.md` §“The ADC probe is taken over while
   running”** — it currently says `d`/`D` read the SMPS feedback.  They do not:
   `bp_adc_probe()` and `bp_adc_continuous_probe()` call
   `bp_adc_check_foreground()`, which returns false while `IEC0bits.AD1IE` is
   on and prints `ADC busy: SMPS feedback active; command rejected.`
   (`Firmware/capability_adc/adc.c`, `Firmware/core_system/base.c`,
   `Firmware/capability_smps/smps.c` `smps_start()`).  The BASIC page now says
   the probe is refused; the SMPS page should match.
2. **`docs/capabilities/auxiliary/pwm-servo.md` §Mode and timer side effects**
   — “each reprograms the channel” is stale.  `bp_pwm_setup()` and
   `bp_servo_setup()` both call `bp_adc_check_foreground()` and return before
   touching OC5 while the SMPS owns the ADC; `bp_update_pwm()` returns early
   for the same reason.  The page should say the terminal generator is refused,
   not that it reprograms the channel.
3. Optional, same page: the PWM table says the one-line `g` range is
   1–3999 kHz, but `bp_pwm_setup()` accepts `pwm_frequency <= 4000`, so
   `g4000` is accepted.  That set's owner should verify whether the table or
   the code is intended.

## Remaining questions / observations

- **`DUTY` after a terminal `g` (possible code-owner follow-up, not counted).**
  The BASIC generator's frequency lives in the static `state.pwm_frequency` in
  `Firmware/capability_aux/aux_pin.c`, which is only assigned by
  `bp_update_pwm()`.  The terminal `g` path never calls it, so `DUTY` in a
  program entered after a terminal `g10 50` calls `bp_update_pwm(0, duty)`,
  which detaches the output instead of changing the duty.  The same value also
  survives `MODE`/`bp_reset_board_state()`, so a later `DUTY` can reattach PWM
  at the old frequency.  The docs now state the contract (“uses the frequency
  of the last BASIC `FREQ`, even after a `MODE` switch … with no BASIC `FREQ`
  since power-on, `DUTY` detaches the output”).  If the module is meant to
  share one generator state with the terminal path,
  `Firmware/capability_aux/aux_pin.c` (`bp_update_duty_cycle`, `bp_pwm_setup`,
  `bp_aux_pin_reset_state`) needs an owner decision.
- **`PULLUP` polarity.**  The complement is documented and test-asserted, but
  it is the only state token that reads 0 for “on”.  If the project wants
  `PULLUP` to match `PSU` (1 = on), the firmware, the native test and this
  page must change together — not a doc-only fix.
- **EEPROM filesystem.**  `BP_BASIC_I2C_FILESYSTEM` is off on v4 and
  `#undef`-ed on v3; enabling it does not build because `PGMSIZE` has no
  definition (`Firmware/core_eeprom/README.md` records the same).  If it is
  ever revived, this set needs a persistence section; until then the current
  “not buildable / no persistence” wording is the correct contract.
- **Terminal ↔ BASIC one-table idea.**  The audit's structural proposal 4 was
  not adopted as a separate table: the bus mapping table in `bus-access.md`
  now carries the `TIME`/`SEND`/`m<N>` units and forms, and `board-pins.md`
  carries the PWM/AUX/PSU ranges.  A second table would duplicate both.
