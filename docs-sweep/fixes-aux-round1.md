# AUX round 1 — implementation record

Scope: `docs/capabilities/auxiliary/` only.  Verified against the `Firmware/`
tree in this worktree (combined image `c57e223`).  No firmware was changed.

Summary: 15 findings fixed in the AUX set, 0 rejected, 4 findings with no AUX
defect (all four need cross-set edits), 8 findings with cross-set work, and 4
code-owner candidates (2 attached to findings, 2 additional).

Host fixture tests run against this tree (no hardware): `test_core_sump_return.py`
(PASS: SUMP exit resets on v3/v4), `test_binary_capabilities.py` (PASS:
"all disabled SMPS opcodes consume exactly their declared operands", binary
PWM framing, capture timeout), `test_core_reset.py` (PASS: mode changes, `#`
and the binary return share the board reset).

## Per-finding record

| # | Decision | What changed (file + short quote) | Evidence |
|---|---|---|---|
| 1 | fixed + cross-set | `binary.md`: command-map reply is now "`0x01`, optional text for drive/release, then the sub-result"; the "always two bytes" paragraph became a per-sub-command table; the `0x09 0x02` exchange now shows `0x01`, `"AUX INPUT/HI-Z"`, `0x01`; the BBIO1-pin fallback is gone. `edge-cases.md`: the `0x09 00/01/02` case states the exact text. | `protocol_i2c/i2c.c` case 9 calls `REPORT_IO_SUCCESS()` first, then `bp_aux_pin_set_low/high/high_impedance`; `BPMSG1041`/`BPMSG1040` are `bp_message_write_line`, `BPMSG1039` is `bp_message_write_buffer`; `0x03`/`0x10`/`0x20` set `fr` and print nothing. |
| 2 | not a defect in AUX + cross-set | No AUX change needed: `binary.md` already says `0x12` "consumes its five-byte frame and replies `0x00`"; `edge-cases.md` keeps the same. | `protocol_binary/binary_io.c` `handle_setup_pwm()` reads `prescale`, the duty word and the period word before `bp_adc_foreground_available()`; the disabled handler reads one operand for every `>= 0xF2`; `tests/test_binary_capabilities.py` `SMPS_FRAME_TEST` asserts `reads == (c >= 0xf2)`. |
| 3 | fixed | `aux-pin.md`: SUMP exit "prints the banner, runs the terminal reset … target back to `AUX`, and the generator flag cleared"; reset section lists the SUMP exit. `pwm-servo.md`: stop table gains the SUMP-exit row; layer note and SUMP bullet now say the exit clears the flag. `edge-cases.md`: the old "state survives" block became "A SUMP exit runs the terminal reset". | `core_system/proc_menu.c` `handle_left_arrow()` calls `enter_sump_mode(); reset_terminal_state();`; `reset_terminal_state()` calls `bp_reset_board_state()`, which calls `clear_mode_configuration()` (`alternate_aux = 0`) and `bp_aux_pin_reset_state()` (`AUX_RPOUT = OFF`, `state.mode = AUX_MODE_IO`); `tests/test_core_sump_return.py` asserts the reset state after the exit. |
| 4 | fixed + cross-set | `pwm-servo.md`: new three-layer table (generator flag, output path, timer registers) and the sentence "A v4 reset … clears the generator flag and detaches the output, but it does not stop Timer2". Stop-table and SUMP rows use the same wording. `binary.md`: `0x0F` says "clears the generator flag and detaches the output; the reset does not stop Timer2". | `core_system/base.c` `bp_reset_board_state()` sets the AUX pins to inputs and calls `bp_aux_pin_reset_state()`, but never writes `T2CON`; `capability_aux/aux_pin.c` `bp_aux_pin_reset_state()` only clears `AUXPIN_RPOUT` and `state.mode`; `bp_pwm_setup()`/`bp_servo_setup()` call `stop_timers()` at their own start. |
| 5 | fixed | `frequency.md`: adds the timeout output block ("`autorange  autorange Frequencies < 1Hz are not supported.`"), says the five-second bound is the period capture only (about seven seconds total), and adds step 6. `edge-cases.md`: new "`f` catches an edge but the period capture times out" case. | `bp_frequency_counter_setup()` prints `BPMSG1245` before `average_sample_frequency()`; a zero return prints `MSG_PWM_FREQUENCY_TOO_LOW`; `average_sample_frequency()` stamps `started = bp_timer_microseconds()` and `goto capture_done` at every `>= 5000000UL` wait; two `poll_frequency_counter_value()` gates run before it. |
| 6 | fixed (+ optional cross-set) | `aux-pin.md`: new "Checking the target" section — `i` prints only the banner in HiZ, `v` lists names/directions but not the selection, reset or select-and-read the line. `edge-cases.md`: new "confirmation line is gone at `HiZ>`" case. | `core_system/proc_menu.c` `case 'i'` calls `print_version_info()` then `print_status_info()` only when `bus_mode != BP_HIZ`; `print_status_info()` prints `BPMSG1086/1087/1263/1264`; `print_pins_information()` prints pin names and directions only. |
| 7 | fixed | `binary.md`: removes "or the BBIO1 pin controls"; states "There is no per-pin write in BBIO1: its state byte rewrites all five latches plus the supply and pull-up bits, and its direction byte rewrites all five directions." `edge-cases.md`: the `0x09` case recommends only `0x03`/`0x10`/`0x20` or terminal drive/read. | `protocol_binary/binary_io.c` `bitbang_pin_state_set()` writes the regulator bit, the pull-up bit and all five latches; `bitbang_pin_direction_set()` writes all five directions. |
| 8 | fixed | `edge-cases.md` rewritten: the two 4-column tables are gone; cases are grouped into Terminal AUX pin, PWM and servo, Frequency, Binary AUX, and Resets and persistence, each as Trigger / You see / Why / Do. All old rows are retained, plus the frequency-timeout, SUMP-exit and trailing-space cases. | Content cross-check against the old 36 rows; the page uses no 4-column table. |
| 9 | not a defect in AUX + cross-set | No AUX change: `pwm-servo.md` only says a board reset stops the SMPS, which matches the code and the SMPS README. | `capability_smps/smps.c` `smps_stop()` sets `BP_AUX1_DIR = INPUT`, then `BP_AUX1_RPOUT = NULL_IO` and `BP_AUX1_LAT = LOW`; `base.c` `bp_reset_board_state()` calls `smps_stop()` under `BP_ENABLE_SMPS_SUPPORT`. |
| 10 | not a defect in AUX + cross-set | No AUX change: the AUX pages do not describe the ADC probe or claim it stops the SMPS. | `core_system/base.c` `bp_adc_foreground_available()` returns false while `AD1IE` is on; `capability_adc/adc.c` `bp_adc_probe()`/`bp_adc_continuous_probe()` return on `bp_adc_check_foreground()`; `protocol_binary/binary_io.c` `read_and_transmit_adc_measurement()` sends `0xFF 0xFF` once when unavailable. |
| 11 | fixed | `pwm-servo.md`: servo table renamed and extended — `Compare ticks`, `Nominal width`, `v4 high time` (63/95/127 ticks; 1.01/1.52/2.03 ms); paragraph says the compare value is what the firmware writes and v4 is one tick longer. | `bp_servo_setup()` sets `duty_cycle = ((position * 5) / 14) + 62` and `PR2 = 1250`; the v4 OC falling comparison takes effect on the next clock, so the output is compare + 1 tick; 16 µs per tick. |
| 12 | fixed | `frequency.md`, `binary.md`, `README.md`: the trigger is now the first-gate count ("16,384 or more" terminal, "256 or more" binary); the Hz values are marked nominal, with the phase caveat at each boundary. | `bp_frequency_counter_setup()`: `frequency > 0x3FFF` keeps the prescaler; `bp_measure_frequency()`: `frequency > 0xFF`; both counters run a one-second gate. |
| 13 | fixed | `aux-pin.md`: "The replies print the literal `AUX`; the `v` table lists the header pins by name but shows directions and levels, not the selected target. Only the confirmation line names the selection." (The auditor's proposed "the v table prints the selected pin's own name" is not what the firmware does.) | `print_pins_information()` prints `BPMSG1257`/`BPMSG1227` plus `MSG_SPI_PINS_STATE`, then a direction for every header pin; `BPMSG1041`/`BPMSG1040`/`BPMSG1095` hard-code `AUX`. |
| 14 | not a defect in AUX + cross-set | No AUX change: `binary.md` already uses duty `0x0063` (99) with period `0x00C7` (199) and says "50 % high time on v4". | `handle_setup_pwm()` writes raw counts; on v4 the high time is duty + 1, so 99 + 1 = 100 of 200 ticks = 50 %; 100 + 1 = 101 of 200 = 50.5 %. |
| 15 | fixed | `README.md`: status is "The v3 features are present in the v3 build; their behaviour on v3 hardware is `unverified`"; table v3 cells say `present`; closing caveat removed. `wiring.md`: `build-only, unverified` and the closing v3 caveat removed. | The AUX code has no feature guard (`capability_aux/aux_pin.c`, the binary AUX cases, the I2C `0x09` case); `configuration.h` shows the default v4/v3 feature sets. |
| 16 | fixed | `README.md`: keeps the Files table and the Command map, adds a four-stage reading order, and replaces the Related-docs table with a short See also paragraph. | File structure; 121 lines, at the template's ~120-line target. |
| 17 | fixed | `edge-wait.md`: both `0x70` reply lines gained one space; comments now start at column 32 in both blocks. | Rendered column check: `host ->` and `host <-` comment columns are 31 (0-based) in both exchanges. |
| 18 | fixed + cross-set | `pwm-servo.md`: the 100 % duty sentence is rewrapped; the long link line is split. All AUX files now pass the 80-column prose check with no tabs. | Line-length scan of `docs/capabilities/auxiliary/*.md`. |
| 19 | fixed | `README.md`: SMPS row is "optional build; off in the default image"; `wiring.md` table matches. | `configuration.h` default v4 block has `#undef BP_ENABLE_SMPS_SUPPORT`. |

## Cross-set changes for the main session

These pages were not edited.  Apply the stated truth; do not simply copy the
AUX wording if the page has its own authority.

1. `docs/protocol/i2c/binary.md` — `0x09` authority.
   - Line 72 command-map row: reply is `0x01`, optional text for drive/release,
     then the sub-result.
   - Lines 232-253: replace "always **two bytes**" and "No text line is
     printed" with the per-sub-command reply shape:
     `0x00` → `AUX LOW` CR LF then `0x01`; `0x01` → `AUX HIGH` CR LF then
     `0x01`; `0x02` → `AUX INPUT/HI-Z` (no line break) then `0x01`;
     `0x03`/`0x10`/`0x20` → `0x01` then the sub-result; unknown → `0x01 0x00`.
   - Lines 292-294 worked exchange: insert `host <- "AUX INPUT/HI-Z"` between
     the acknowledgement and the sub-result.
   - Line 126: the release advice `0x09 0x02` also prints that text; a host
     must consume it.
   - Truth: `protocol_i2c/i2c.c` case 9 sends `REPORT_IO_SUCCESS()` before
     reading the sub-command, then calls the terminal set-low/set-high/
     set-high-impedance functions, which print `BPMSG1041`/`BPMSG1040`
     (line writers) or `BPMSG1039` (buffer writer).

2. `docs/capabilities/smps/binary.md` — no-SMPS build and reset.
   - Lines 31-35 and table row 48: a start on a build without SMPS **consumes**
     its operand and replies `0x00`.  Delete "its operand byte is not
     consumed" / "leaves its operand unread".  Add the aligned example:
     `host -> 0xF2 0x58` / `host <- 0x00` / `host -> 0x01` (reprints `RAW1`).
   - Lines 21-24: the v4 terminal reset does stop the SMPS loop through
     `bp_reset_board_state()` → `smps_stop()`; remove `unverified`.
   - Truth: `protocol_binary/binary_io.c` `handle_smps_command()` disabled
     branch reads one byte for `>= 0xF2`; `tests/test_binary_capabilities.py`
     `SMPS_FRAME_TEST` asserts it.

3. `docs/capabilities/smps/edge-cases.md` — no-SMPS build, stop pin, probe.
   - Lines 10-18: same operand fix as item 2.  Delete the `0xF2 0x58` →
     pull-up example and the "disabled path does not consume" sentence.
   - Lines 73-78: stop leaves `AUX1` an **input**.  Delete "the pin keeps its
     port direction and output latch state" and "stays an output at its last
     level".  Truth: `smps_stop()` starts with `BP_AUX1_DIR = INPUT`, then
     clears `BP_AUX1_RPOUT` and `BP_AUX1_LAT`.
   - Lines 82-90: align with the ADC pages.  While the SMPS owns the
     converter, terminal `d`/`D` are refused with
     `ADC busy: SMPS feedback active; command rejected.`; binary `0x14`/`0x15`
     return `0xFF 0xFF` once and `0x15` does not stream; the loop continues
     until `0xF1` or a reset.  Delete "the reading follows the SMPS feedback"
     and "turns the converter off".

4. `docs/protocol/binary/commands.md` — timer layer and two long lines.
   - Lines 213-215 and 281-287: use the same three layers as
     `pwm-servo.md`.  `0x0F` on v4 clears the generator flag and detaches the
     output but does not stop Timer2; send `0x13` first if the timer must be
     idle.  Remove "A PWM timer setup survives this reset"; if the register
     state is meant, say "Timer2 is left running".
   - Line 228: split the self-test paragraph at "If the SMPS owns the ADC"
     (142-character line).
   - Lines 265-271: use duty `0x0063` (99) for the exact 50 % example; note
     that `0x0064` (100) is 50.5 % on v4, or drop that example.

5. `docs/protocol/binary/examples.md` lines 144-148: same duty example —
   `0x0063` for 50 %, not `0x0064`.

6. `docs/protocol/dio/aux-pin.md` lines 43-48 (optional, from finding 6):
   qualify "Run `i`" — in DIO it prints the status block; at `HiZ>` only the
   banner.  `v` never shows the selection.

## Code-owner candidates

1. `0x09` drive/release text in a binary reply (finding 1).  `protocol_i2c/i2c.c`
   case 9 calls `bp_aux_pin_set_low()`/`set_high()`/`set_high_impedance()`;
   those print `BPMSG1041`/`BPMSG1040`/`BPMSG1039` on the same wire before the
   sub-result.  If a fixed-size two-byte reply is wanted, the handler must
   update the latch/direction without the terminal message.  Documentation now
   states the implemented wire format.
2. A board reset detaches the AUX output and clears the mode flag but does not
   stop Timer2 (finding 4).  `capability_aux/aux_pin.c`
   `bp_aux_pin_reset_state()` only clears `AUXPIN_RPOUT` and `state.mode`;
   `core_system/base.c` `bp_reset_board_state()` never writes `T2CON`.  If a
   reset is expected to idle the timer, add the timer stop there.  Docs now say
   to send `0x13` before `0x0F`.
3. Bare `g` / `S` detection does not skip whitespace (additional).  In
   `capability_aux/aux_pin.c`, `bp_pwm_setup()` and `bp_servo_setup()` test
   `cmdbuf[((cmdstart + 1) & CMDLENMSK)] == 0x00`.  A trailing space therefore
   takes the parameter path: `g ` prints `PWM disabled` and prompts, `S `
   stops silently and prompts.  Docs now warn; the parser could call
   `consumewhitechars()` before the test.
4. Zero-timeout `0x70` race (additional, from the auditor's unknowns).
   `protocol_binary/binary_io.c` `handle_wait_command()` clears `CNIF`, reads
   the level, then checks `CNIF` before evaluating the zero-timeout expiry, so
   a change in that window can return `0x01`.  Docs now say "normally `0x00`";
   if an exact single sample is required, evaluate the timeout before the
   edge check.

## Remaining questions

- The absolute maximum on the AUX pins and the maximum frequency input remain
  hardware limits not stated in the docs; no finding can resolve them.
- The v3 hardware behaviour of the servo compare-direct path and the v3
  logic-level class remains `unverified`; the set says so once in the README.
- The `0x09` text is common code on both boards; the v3 wire behaviour is
  code-derived, not hardware-confirmed.
- The i2c and SMPS cross-set fixes above must land together with this set;
  until then those pages still contradict the implemented reply/frame
  contracts.
- Authority split for `0x09`: either keep the full reply table in
  `auxiliary/binary.md` and make `i2c/binary.md` link to it, or make
  `i2c/binary.md` the owner and reduce the AUX page to a summary.  The two
  pages must not both define the shape differently; this record assumes the
  i2c page is corrected to match the AUX table.
