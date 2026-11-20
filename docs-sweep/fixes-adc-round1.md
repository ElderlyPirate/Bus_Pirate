# ADC probe — round 1 implementation record

**Scope:** `docs/capabilities/adc/` plus this record.  Worktree
`bpv4-docssweep`, branch `docs/combined-sweep`; the `Firmware/` tree is the
combined image.  No firmware changes, no commits, no WORKLOG edits.

**Result:** 15 findings.  12 are fixed in the ADC set.  0 are rejected.  One
design question is recorded as a code-owner candidate (inside finding 5).
3 findings are cross-set only (3, 14, 15); findings 2, 4 and 5 also carry
cross-set halves.

## Decisions

| # | Decision | What changed (file + short quote) | Evidence |
|---|---|---|---|
| 1 | fixed | `wiring.md` §Input range: "Above 6.6 V the reading stops at 6.59 V and cannot tell you how far over the input is; treat any voltage above 6.6 V as potentially damaging, not as a harmless clip, and disconnect it."  The table now says "measurement range (nominal)" and "largest reading".  `edge-cases.md` §Input above 6.6 V: "Above 6.6 V is not a harmless clip: the pin's absolute maximum is undocumented and the input may be overstressed." | `Firmware/core_system/base.c` `bp_write_voltage()` scales `adc * 29 / 45`, so the top count 1023 is 659 centivolts; `bp_read_adc()` returns `ADC1BUF0` with no range flag.  `Firmware/hardwarev3.h` / `hardwarev4.h` define the probe channel only; no absolute-maximum rating exists in the sources. |
| 2 | fixed (+ cross-set) | `wiring.md` hazard table: "Leave the self-test `ADC`→`+3.3V` jumper fitted with a source connected \| Plain wire to the `+3.3V` rail; the source back-drives it and can damage the board or source.  Remove it after the test."  `edge-cases.md`: "the jumper is a plain wire from the probe pin to the `+3.3V` rail, not a resistor.  It hard-ties the two, so a connected source drives the rail and can damage the board or the source."  The "about raw 512" symptom is now limited to a high-impedance source. | `Firmware/capability_selftest/selftest.c` `perform_selftest()` with `jumper_test` prompts `BPMSG1163` and measures `BP_ADC_PROBE`; `Firmware/core_messages/messages_v4.s` BPMSG1163 is `"Disconnect any devices\r\nConnect (ADC to +3.3V)"`.  The jumper is external wiring, not switched or current-limited by firmware.  `docs/capabilities/selftest/wiring.md` calls it "a plain wire between two header pins, not a resistor". |
| 3 | cross-set | No ADC change: `probe.md` and `binary.md` already say `d` / `D` are refused and `0x14` / `0x15` return `0xFF 0xFF` once.  The SMPS page must change; see cross-set item 1. | `Firmware/core_system/base.c` `bp_adc_foreground_available()` returns `IEC0bits.AD1IE == OFF`; `bp_adc_check_foreground()` prints `"ADC busy: SMPS feedback active; command rejected."`.  `Firmware/capability_adc/adc.c` `bp_adc_probe()` and `bp_adc_continuous_probe()` call that guard before reading.  `Firmware/protocol_binary/binary_io.c` `read_and_transmit_adc_measurement()` sends `0xFF 0xFF` when unavailable; `handle_read_adc_one_shot()` / `handle_read_adc_continuously()` return without enabling the ADC.  No probe path reads `smps_state.voltage_reading`. |
| 4 | fixed (+ cross-set) | `binary.md` §SMPS owns the ADC: "After `0xFF 0xFF` the host is in `BBIO1`; send `0x05` first to enter `RAW1`, then `0xF1` (stop; reply `0x01`), then `0x00` to return to `BBIO1`.  Do **not** send `0xF1` in `BBIO1`: there it is a pin-state byte that switches the target supply and pull-ups on, not a stop."  "Alternatively send `0x0F` from `BBIO1`: it replies `0x01` and runs the v4 terminal reset — pins to inputs, target supply and pull-ups off, board state reset, SMPS loop stopped — then prints the banner and `HiZ>`."  `probe.md` ownership step 4 gives the same `0x0F` answer once; `terminal.md` marks the `HiZ>d` transcript as defensive and gives `#` or reset as the terminal recovery. | `Firmware/protocol_binary/binary_io.c`: `BITBANG_COMMAND_RAW_WIRE = 0x05`; `SMPS_COMMAND_STOP = 0xF1`; `BITBANG_COMMAND_RETURN_TO_TERMINAL = 0x0F`; `handle_bitbang_command()` for `0x0F` calls `REPORT_IO_SUCCESS()` (sends `0x01`), `reset_state()`, returns `-1` on v4; `enter_binary_bitbang_mode()` breaks; `Firmware/core_system/proc_menu.c` then calls `reset_terminal_state()` -> `bp_reset_board_state()` -> `smps_stop()`.  A byte with the high bit set in `BBIO1` goes to `bitbang_pin_state_set()`; `0xF1` sets bit 6 (VREG) and bit 5 (pull-ups).  `binary_io_raw_wire_mode_handler()` returns to `BBIO1` on `0x00` without `smps_stop()`. |
| 5 | fixed (+ cross-set; code-owner CO-1) | `probe.md` §The converter: "The 3.3 V channel is the exception: it is the same rail through a divider, so it is measured against itself".  §The channels: "A 3.3 V rail that sags while still running the board still reads about 3.3 V.  The `v` column, the `W` rail check and the self-test 3.3 V window are go/no-go checks on the divider and on the channel being present; they cannot prove the rail is at 3.3 V or catch a small droop.  The 5.0 V, Vusb and VPU channels are measured against the 3.3 V reference, so those readings do move with it."  Resolution table adds "3.3 V channel \| ratiometric to that rail; divider/rail-present check only". | `Firmware/core_system/base.c` sets `AD1CON2 = 0x0000`, i.e. `VCFG = 000` (`VR+ = AVdd`, `VR- = AVss`), and no other code writes `AD1CON2`.  `Firmware/hardwarev4.h` maps `BP_ADC_3V3` to channel 0 and `BP_ADC_3V3_CFG` to `AN0/ADC3/33V`; `Firmware/hardwarev3.h` maps it to `AN10/ADC3/33V`.  `Firmware/hardware.h` sets `V33BASE 0x200` with the comment `(((3.3/2)/3.3)*1024)`, confirming a 2:1 divider.  No bandgap or external reference is selected anywhere. |
| 6 | fixed | `terminal.md` §Continuous output: "Set the terminal to send CR only for Enter; a terminal that always sends CR+LF leaves the LF waiting and `D` exits immediately every time.  Pressing more keys adds bytes; it does not drain the line.  If the terminal cannot send CR alone, use `d` or a host program for `D`."  `edge-cases.md` recovery: "set the terminal to send CR only (not CR+LF), then retry; or use a host program that sends one byte." | `Firmware/capability_adc/adc.c` `bp_adc_continuous_probe()` prints `bp_write_voltage(0)` and then enters `while (!user_serial_ready_to_read())`; after the loop it calls `user_serial_read_byte()` once and `bpBR`.  A CR+LF arrival leaves one byte for that check. |
| 7 | fixed | `binary.md` worked exchange adds `host <- (no sample for timeout)  drain complete`, and the prose says "The timeout is chosen by the host; the firmware has no fixed drain time." | `Firmware/protocol_binary/binary_io.c` `handle_read_adc_continuously()` does `do { read_and_transmit_adc_measurement(); if (user_serial_ready_to_read()) { user_serial_read_byte(); should_loop = false; } } while (should_loop);` — the stop is seen at the loop boundary, and the loop has no end marker. |
| 8 | fixed | `README.md` five-minute bullet: "`VOLTAGE PROBE: 3.29V (raw 512)` (example; the display truncates, so 3.30 V prints as `3.29V`)".  Intro and tables now say "nominal full-scale input 6.6 V" and "largest reading 6.59 V (raw 1023)"; `probe.md`, `wiring.md` use the same split. | `Firmware/core_system/base.c` `bp_write_voltage()` computes `centivolts = (adc * 42235) >> 16`, exactly `floor(adc * 29 / 45)`: 512 -> 329, 1023 -> 659.  The ideal `512 / 1024 * 6.6` is 3.300 V. |
| 9 | fixed | `terminal.md` command table: "`d:N` \| N conversions, N = 1..32767; not interruptible \| N readings total; the header is on the first line".  `README.md`: "`d:N` takes N readings ... prints N lines total with the header on the first; the repeat cannot be interrupted, so use a small N." | `Firmware/core_system/proc_menu.c` `case 'd'` sets `repeat = getrepeat() + 1u`, prints the header once, then `while (--repeat) { bp_adc_probe(); bpBR; }`; the loop never reads the host.  `getrepeat()` returns 0 for an invalid count, so `repeat` is 1 and the loop body does not run. |
| 10 | fixed | `wiring.md` and `edge-cases.md` use the firmware form `+3.3V`, not `+3.3 V`, for the self-test jumper. | `Firmware/core_messages/messages_v4.s` BPMSG1163: `"Disconnect any devices\r\nConnect (ADC to +3.3V)"`; `messages_v3.s`: `"Disconnect any devices\r\nConnect (Vpu to +5V) and (ADC to +3.3V)"`. |
| 11 | fixed | `README.md` reading order now goes "wire -> take one reading -> understand the scale -> script it", with `probe.md` in the path.  The SMPS bullet links the terminal symptom to `terminal.md#smps-owns-the-adc` and the bytes to `binary.md#smps-owns-the-adc`. | Headings `## SMPS owns the ADC` exist in both pages; a link/anchor check over the six ADC files resolves every relative link and anchor. |
| 12 | fixed | `README.md`: "The firmware uses several labels for the same input" (instead of "three ways"), and "Only the probe channel has dedicated commands (`d` / `D`, binary `0x14` / `0x15`).  The 3.3 V rail, 5.0 V rail, VPU and (v4) Vusb channels are housekeeping, displayed by `v`..." | `Firmware/core_messages/messages_v4.s` help line `d/D\tMeasure ADC (once/CONT.)`; `MSG_ADC_VOLTAGE_PROBE_HEADER` is `"VOLTAGE PROBE: "`; `MSG_ADC_VOLTMETER_MODE` is `"VOLTMETER MODE"`.  `Firmware/core_system/proc_menu.c` `case 'v'` calls `print_pins_information()`, which is a user command that reads the housekeeping channels. |
| 13 | fixed | `wiring.md` §What not to connect is now a two-column `Do not \| Because` table with six rows: over-voltage, negative, mains/non-isolated, probe-as-output, fitted self-test jumper, and source during the SMPS loop. | Documentation-only safety change; each row is backed by the range/reference facts above or by the firmware ownership guard. |
| 14 | cross-set | No ADC change.  The cross-linked binary README must use "unverified on v3 hardware" instead of "not tested on v3 hardware"; see cross-set item 6. | The ADC/generic/binary pages use "unverified" for the v3 build.  The code facts are unchanged. |
| 15 | cross-set | No ADC change.  The SMPS pages must say the no-SMPS start path consumes its operand; see cross-set item 3. | `Firmware/protocol_binary/binary_io.c` `handle_smps_command()` `#else` branch: `if ((uint8_t)command >= SMPS_COMMAND_START) { user_serial_read_byte(); } REPORT_IO_FAILURE();`.  `docs/protocol/binary/sub-protocols.md` and `docs/protocol/binary/edge-cases.md` already say the operand is consumed. |

## Counts

- fixed in the ADC set: 12 (findings 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  13).
- rejected / not a defect: 0 findings.
- code-owner candidates: 1 (CO-1, tied to finding 5).
- cross-set: 3 findings (3, 14, 15), plus cross-set halves in findings 2, 4
  and 5; six correction items in the list below.

## Files changed

- `docs/capabilities/adc/README.md`
- `docs/capabilities/adc/probe.md`
- `docs/capabilities/adc/terminal.md`
- `docs/capabilities/adc/binary.md`
- `docs/capabilities/adc/wiring.md`
- `docs/capabilities/adc/edge-cases.md`
- `docs-sweep/fixes-adc-round1.md` (this record)

## Code-owner candidate

**CO-1 — the 3.3 V rail check is ratiometric.**  `Firmware/core_system/base.c`
sets `AD1CON2 = 0x0000` (`VCFG = 000`, `VR+ = AVdd`), and
`Firmware/hardwarev4.h` / `hardwarev3.h` route `BP_ADC_3V3` from the `33V`
node through a 2:1 divider (`Firmware/hardware.h` `V33BASE`).  The self-test
(`Firmware/capability_selftest/selftest.c` `perform_adc_test(BP_ADC_3V3,
V33L, V33H)`) and the `W` rail check (`Firmware/core_system/proc_menu.c`
`switch_psu_on()`) treat that channel as an absolute rail measurement.  If the
`33V` node is the same rail that supplies the converter, a low rail moves the
input and the reference together and the window cannot detect it; an off or
shorted switched branch is still visible as a near-zero count.  Confirm the
board topology.  If the check is meant to catch a low rail, it needs a
different reference (the device has an internal bandgap) or a relabelled
"rail present / divider" check.  The user docs now state the limit; no
firmware was changed.

## Cross-set changes for the main session

1. **SMPS set (finding 3).**  `docs/capabilities/smps/edge-cases.md`
   §"The ADC probe is taken over while running" (lines 82-93) says the probe
   reading follows the SMPS feedback and that a probe command turns the
   converter off.  The code refuses instead: terminal `d` / `D` / `d:N` print
   `ADC busy: SMPS feedback active; command rejected.` plus
   `Syntax error at char 1`; binary `0x14` / `0x15` return `0xFF 0xFF` once
   (`Firmware/core_system/base.c` `bp_adc_check_foreground()`,
   `Firmware/capability_adc/adc.c` `bp_adc_probe()` /
   `bp_adc_continuous_probe()`, `Firmware/protocol_binary/binary_io.c`
   `read_and_transmit_adc_measurement()`).  Replace the entry and link to
   `docs/capabilities/adc/probe.md` §Who owns the converter.
2. **SMPS set (finding 4).**  `docs/capabilities/smps/binary.md` lines 21-25
   call the v4 terminal-reset stop "`unverified`".  The path is code-verified:
   `BITBANG_COMMAND_RETURN_TO_TERMINAL = 0x0F` -> `reset_terminal_state()` ->
   `bp_reset_board_state()` -> `smps_stop()` under `BUSPIRATEV4` +
   `BP_ENABLE_SMPS_SUPPORT`.  `docs/capabilities/smps/README.md` already says
   the v4 terminal reset stops it.  Remove the caveat; keep the add-on
   board's electrical behaviour `unverified`.  Also tighten
   `smps/edge-cases.md` lines 63-68: "only `0xF1` ... stops them" is followed
   by the hardware/terminal-reset exceptions; make the exceptions the rule.
3. **SMPS set (finding 15).**  `docs/capabilities/smps/binary.md` lines
   31-35 and `docs/capabilities/smps/edge-cases.md` lines 8-23 say a start on
   a no-SMPS build leaves its operand unread.  It is consumed:
   `Firmware/protocol_binary/binary_io.c` `handle_smps_command()` `#else`
   branch reads one byte when the command is `>= 0xF2`.  Update both pages
   and the `0xF2 0x00` / `0xF2 0x58` examples;
   `docs/protocol/binary/sub-protocols.md` and
   `docs/protocol/binary/edge-cases.md` are correct.
4. **Self-test set (finding 2).**  Add a removal step to the self-test
   setup/after-run instructions: the `ADC` -> `+3.3V` jumper is a plain wire
   to the rail and must be removed after the run, before connecting anything
   to `ADC`.  `docs/capabilities/selftest/wiring.md` §Before and after the
   run has no removal step; `docs/capabilities/selftest/terminal.md` line 160
   only says "fit/reseat the jumper".  The firmware does not remove external
   wiring.
5. **Self-test set (finding 5).**  `docs/capabilities/selftest/checks.md`
   rows `ADC` (line 121) and `3.3V` (line 122) present these as rail checks:
   "the rail is out of band: regulator fault or short" and "`3.3V` is low".
   With `VCFG = AVdd`, the `3.3V` and jumper-fed `ADC` counts are ratiometric;
   they can show a missing/off/shorted divider input, not a small rail droop.
   Soften both "If it fails" cells to "divider or rail-present path" and add
   the same caveat to `docs/capabilities/selftest/terminal.md` line 160 and
   §Voltage checks (`checks.md` lines 193-198).  `selftest/edge-cases.md`
   §v4 3.3V rail off can keep the off/switch case; the word "regulator"
   should be qualified.
6. **Binary set (finding 14).**  `docs/protocol/binary/README.md` lines 3-4
   and 82 use "not tested on v3 hardware".  Replace with "unverified on v3
   hardware" to match the rest of the set; keep the fact that the features
   are present in the build.

## Remaining questions

- CO-1: confirm whether the `33V` divider input is the same electrical node
  that supplies the converter on both boards.  The docs now state the safe
  limit (divider/rail-present check) either way, but the code-owner decision
  (bandgap reference vs relabelled check) depends on the board schematic.
- The divider resistor values, input impedance, maximum source impedance and
  the pin's absolute maximum remain undocumented; the ADC pages keep those as
  `unverified`.
- The `0x15` drain timeout has no firmware value; it stays host-chosen, as
  the worked exchange now says.
- Cross-set: the SMPS pages are the only remaining place where the probe
  refusal is described as "reads the feedback"; the ADC set is now internally
  consistent.
