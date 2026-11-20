# OpenOCD adapter — round 1 fixes record

**Worktree:** `C:\Users\jschl\dev\bpv4-docssweep`, branch `docs/combined-sweep`
(firmware image `c57e223`).  **Set:** `docs/tools/openocd/`.  **Scope:** every
numbered finding in `docs-sweep/feedback-openocd-round1.md`, verified against
the firmware and the OpenOCD 0.12 driver source before editing.  No firmware or
WORKLOG files were changed and nothing was committed.

## Decisions

| # | Decision | Change (file + short quote) | Evidence |
|---|---|---|---|
| 1 | fixed | `entry-and-exit.md`: entry state rows now read “**off; the binary entry and `0x06` both switch it off**”; the entry prose says “Both steps run the same BBIO pin reset”; the `0x00` row says VREG and pull-ups “switch off”; `README.md` says “`0x00` … switches the regulator and the on-board pull-ups off.  `0x0F` adds the return to the terminal”; `overview.md` says “off; entry switches it off”; `limits-and-errors.md` says “`0x00` already switches the regulator and the on-board pull-ups off”. | `Firmware/protocol_binary/binary_io.c`: `enter_binary_bitbang_mode()` calls `reset_state()`; `handle_bitbang_command()` case `BITBANG_COMMAND_OPENOCD` calls `reset_state()` before `binOpenOCD()` and again after it. `reset_state()` calls `bitbang_pin_state_set(0x00)`, which calls `bp_set_voltage_regulator_state(OFF)` and `bp_set_pullup_state(OFF)`; `bp_disable_voltage_regulator()` / `bp_disable_pullup()` in `Firmware/hardware.h`. TRST is not in `bitbang_pin_direction_set`/`bitbang_pin_state_set`, so it keeps its level. |
| 2 | fixed | `README.md`: “or, for a 3.3 V target only, set `buspirate mode normal`.  A 1.8 V or 2.5 V target must stay open-drain with pull-ups to its own rail.” | `Firmware/tool_openocd/openocd.c` `binOpenOCDPinMode()`: `MODE_JTAG` clears the open-drain control bits and makes the pins push-pull outputs, so the high level is the board's 3.3 V. `wiring.md` already carried the 1.8 V / 2.5 V warning. |
| 3 | fixed | `README.md`: “One `0x05` request carries at most **8192 bits**, including every TAP move in that request”; `overview.md`: “keep the whole `0x05` block, including its TAP moves, inside 8192 bits” and “TDI and TMS lane buffers are 1024 bytes each”; `commands.md`: “8192 bits per `0x05` request (`0x2000`), including TAP moves” and “two 1024-byte lane buffers, TDI and TMS”; `limits-and-errors.md`: “two 1024-byte lane buffers, one for TDI and one for TMS, so the request holds at most 8192 bits”; `host-setup.md` and `workflow.md` use the same whole-request wording. | `Firmware/configuration.h`: `BP_JTAG_OPENOCD_BIT_SEQUENCES_LIMIT 0x2000`. `Firmware/tool_openocd/openocd.c` clamps `j` to that value and echoes the requested `inByte`/`inByte2`. OpenOCD 0.12 `src/jtag/drivers/buspirate.c`: `BUSPIRATE_BUFFER_SIZE 1024`, separate `tms_chain[1024]` and `tdi_chain[1024]`, `buspirate_tap_make_space(1, scan_size+8)`, and `buspirate_tap_append()` logs `tap_chain overflow, bad things will happen` past 1024 bytes per lane. |
| 4 | fixed | `commands.md`: the `0x07` table row says “1 byte: selector; then a separate `0xAA 0x55` write” and the section gives four numbered steps, including “Send `0xAA 0x55` in a separate write”; `host-setup.md` custom-host table adds “on v3, send the `0x07` selector and the `0xAA 0x55` confirmation in separate writes”; `limits-and-errors.md` and `exchanges.md` repeat the split. | `Firmware/tool_openocd/openocd.c` `case CMD_UART_SPEED`: reads the selector, immediately calls `user_serial_set_baud_rate(UART_FAST_SPEED)` or normal, then reads `inByte`/`inByte2` and checks `0xAA 0x55`. A failed check sets `i = SERIAL_NORMAL`, restores the normal rate, and replies `0x07 0x00`. |
| 5 | fixed | `host-setup.md` adds “Check the build” with `openocd -c "adapter list" -c "shutdown"`, the `interface_list` note for 0.10, `./configure --enable-buspirate` / `make` / `sudo make install`, and a Windows route table (Cygwin, WSL, Linux VM); `workflow.md` step 4 adds the same check; `README.md` adds the check. | OpenOCD 0.12 `configure.ac`: `--enable-buspirate` exists, defaults to `auto`, and MinGW/MSYS forces `enable_buspirate=no` and errors if explicitly `yes`. `src/jtag/adapter.c` registers `adapter list` as `COMMAND_ANY`; 0.10 uses `interface_list`. 0.12 ships `tcl/interface/buspirate.cfg`. |
| 6 | fixed | `workflow.md` health check: “`V33` is the board's `3.3V` rail; 0 with VREG off and no board rail connection”; new sentence: “do not connect a self-powered target's supply to a rail just to make it read.  That is back-driving.” | `Firmware/tool_openocd/openocd.c` `CMD_READ_ADCS` reads `BP_ADC_PROBE`, `BP_ADC_VPU`, `BP_ADC_3V3`, `BP_ADC_5V0`; `bp_read_adc(BP_ADC_3V3)` measures the board rail, not the target's own supply. `wiring.md` forbids back-driving. |
| 7 | fixed | `exchanges.md`: every code block was re-padded; all request/reply comments start at the same column (31) and continuation lines use the same indent. | Checked mechanically: all comment-bearing lines in code blocks start at column 31; zero TAB characters. |
| 8 | fixed | `commands.md` `0x05` steps: new step 5 says “After the last bit TCK is left high and TDI/TMS hold their last values.  In a final 9–16-bit group the unused high bits are ignored; still send all four payload bytes.  A final 1–8-bit group takes two payload bytes.” | `Firmware/tool_openocd/openocd.c` v4 loop sets `OOCD_CLK = HIGH` after the last bit and leaves the `IOLAT` TDI/TMS values; `bits_to_process = 16 < bit_sequences ? 16 : bit_sequences` ignores higher payload bits. `Firmware/tool_openocd/openocd_asm.s` `_binOpenOCDTapShiftFast` does `bset … OOCD_CLK_BIT` and processes `min(15, bits-1)+1` bits per group while always reading four payload bytes. |
| 9 | fixed | `commands.md` clamp section: “The TDO reply is sized for the clamped count, so expect fewer TDO bytes than the requested count.” `limits-and-errors.md` and `exchanges.md` repeat the short-TDO fact. | `Firmware/tool_openocd/openocd.c` v4 path clamps `j` before `bit_sequences = (int16_t)(j & 0x7FFF)` and emits TDO per processed group; v3 path computes `i = (j + 7) / 8` from the clamped `j` and passes the clamped `j` to `binOpenOCDTapShiftFast`, so the TX byte count follows the clamped count. |
| 10 | fixed | `entry-and-exit.md` and `limits-and-errors.md` name “the five BBIO pins (`AUX`, `CS`, `MISO`, `CLK`, `MOSI`)”; `commands.md` now says “all five JTAG lines” and “the five JTAG output latches (TDI, TMS, TCK, SRST, TRST)”; `workflow.md` says “all five JTAG outputs”. | `binary_io.c` `bitbang_pin_direction_set()` / `bitbang_pin_state_set()` touch AUX, MOSI, CLK, MISO and CS. `openocd.c` `binOpenOCDPinMode()` touches TDI (MOSI), TMS (CS), TCK (CLK), SRST (AUX0), TRST (PGD/AUX1); TDO (MISO) stays an input. |
| 11 | cross-set | Not edited here.  `docs/protocol/jtag/entry.md:75-76` should read: “`0x06` entry resets the pin mode, VREG, pull-ups, SRST level and TCK delay; TRST, the mode LED and the link rate are not changed.” | `Firmware/protocol_binary/binary_io.c` `reset_state()` does not touch `BP_PGD` or `BP_AUX1`; only SRST (`BP_AUX0`) is reset.  `Firmware/tool_openocd/openocd.c` `binOpenOCDPinMode()` is the first code that drives TRST. |
| 12 | cross-set | Not edited here.  `docs/protocol/jtag/tap.md:201` should say “feature selector `0x04` (sent as `0x02 0x04 action`)”; line 203 should say “`#`, reset button, `0x00` then `0x0F` on the OpenOCD path, `0x0F` twice on the XSVF path”. | `openocd.c` `FEATURE_TRST = 0x04` is reached only through `CMD_FEATURE = 0x02`; `0x04` alone is an unknown command. `Firmware/protocol_jtag/jtag.c` `case 0x0F: … return;` returns the player to `BBIO1`; `binary_io.c` `BITBANG_COMMAND_RETURN_TO_TERMINAL` is the second `0x0F`. |
| 13 | fixed + cross-set | Fixed here: `README.md` replaces “build-only” with “compiled in but not verified on v3”; `commands.md` adds “It is the same on-board regulator that the terminal `W` command switches (`POWER SUPPLIES ON`)”; `README.md` adds “the XSVF set calls the same thing the **firmware player**”.  Cross-set: `docs/protocol/jtag/openocd.md:32` should say “leave the OpenOCD path” instead of “leave OpenOCD mode”. | `Firmware/configuration.h` defines `BP_JTAG_OPENOCD_SUPPORT` for both v3 and v4; `Firmware/core_system/proc_menu.c` `switch_psu_on()` calls the same `bp_enable_voltage_regulator()` as feature `0x02`; the XSVF set's naming is the firmware player. |
| 14 | fixed | `README.md`: “No project configuration is shipped.  OpenOCD's installed `interface/buspirate.cfg` is the starting template”; `limits-and-errors.md` row changed to “no project config shipped — OpenOCD's installed interface file is the starting point”; `host-setup.md` reference comment now says “OpenOCD ships interface/buspirate.cfg”. | OpenOCD 0.12 ships `tcl/interface/buspirate.cfg`; `host-setup.md` already sources `[find interface/buspirate.cfg]`. |
| 15 | fixed | `limits-and-errors.md` headings are now “Driver messages and startup symptoms” and “Target and chain messages and symptoms”; both first columns are “Message or symptom”; “unknown adapter driver at startup” moved to the driver table. | The driver table now contains only quoted OpenOCD messages plus that startup condition; the target table keeps the chain symptoms. |
| 16 | fixed | Removed the `## Unverified` section.  The v3 caveat remains inline in the README/overview availability tables and as “The v3 path is compiled into the build; its hardware behaviour is not verified on v3” in `limits-and-errors.md`; the Cygwin caveat remains in the host-setup tables and host-driver limits; the TCK-rate caveat remains in the `0x08` section. | No `## Unverified` heading remains; `grep` for “build-only” in the set returns no hits. |

## Code-owner candidates

1. **v3 zero-count `0x05` has no guard.**  `Firmware/tool_openocd/openocd.c`
   (`case CMD_TAP_SHIFT`, `BUSPIRATEV3` branch) computes
   `i = (j + 7) / 8` and calls `binOpenOCDTapShiftFast(..., j, ...)` even when
   `j == 0`.  `Firmware/tool_openocd/openocd_asm.s`
   `_binOpenOCDTapShiftFast` does `dec.w w2, w6` (0xFFFF for a zero count) and
   then loops over 16-bit groups, reading stale/undefined buffer bytes and
   emitting TDO.  The v4 branch in `openocd.c` has an explicit
   `while (bit_sequences > 0)` guard and a comment describing the old
   zero-count failure.  The user docs already say “v3 zero count is not
   supported; use at least one bit” and “no defined reply”, so no further doc
   change is needed.  A code owner should add the same zero-count guard to the
   v3 path.

## Cross-set changes for the main session

These are outside `docs/tools/openocd/` and were not edited.

1. `docs/protocol/jtag/entry.md:75-76` — replace the TRST clause:

   > `0x06` entry resets the pin mode, VREG, pull-ups, SRST level and TCK
   > delay; TRST, the mode LED and the link rate are not changed.

   The current “TRST/SRST levels” wording is wrong: the BBIO reset clears
   SRST (`AUX0`) but never touches TRST (`PGD` on v3, `AUX1` on v4).

2. `docs/protocol/jtag/tap.md:201` — replace “OpenOCD feature `0x04`” with
   “OpenOCD feature selector `0x04` (sent as `0x02 0x04 action`)”.

3. `docs/protocol/jtag/tap.md:203` — replace “`#`, reset button, or second
   `0x0F`” with “`#`, reset button, `0x00` then `0x0F` on the OpenOCD path,
   `0x0F` twice on the XSVF path”.

4. `docs/protocol/jtag/openocd.md:32` — replace “leave OpenOCD mode” with
   “leave the OpenOCD path”, matching the naming table in this set.

Optional consistency only (not required by a numbered finding): the phrase
“build-only” also appears in `docs/protocol/jtag/README.md:3`,
`docs/protocol/jtag/openocd.md:186`, and `docs/protocol/jtag/edge-cases.md:96`.
The OpenOCD set now uses “compiled in but not verified”; aligning those pages
is a terminology decision for the main session.

## Structural notes

The feedback's non-numbered structural suggestions were handled where they
intersected a finding: the README now has the "First session" pointer, and the
host build/check block sits before the option matrix.  The bottom "Related"
tables and the nine-file layout were left in place; trimming them is a
presentation decision outside the numbered findings.

## Verification

- All relative links in `docs/tools/openocd/` resolve.
- Zero TAB characters in the set; every comment-bearing line in the
  `exchanges.md` code blocks starts at column 31; code fences are balanced.
- Every changed claim was checked against the firmware symbols and the
  OpenOCD 0.12 source listed in the decisions table.  No bench run was made.
- Pre-existing working-tree modifications in `docs/protocol/jtag/edge-cases.md`
  and `docs/protocol/jtag/xsvf.md` were not touched; the only files edited for
  this task are listed in the final reply.

## Remaining questions

- OpenOCD 0.13 and later may change the `adapter list` output format and the
  driver buffer internals; this set documents 0.12 and earlier.  The check
  command still lists `buspirate`, but the exact text may differ.
- The v3 zero-count symptom is known to be undefined from the source; the docs
  deliberately say “no defined reply” rather than claiming a specific wire
  result.  Hardware confirmation would need a v3 board.
- Cygwin and WSL runtime behaviour remains unverified; the docs label both.
- No other numbered finding was rejected: every finding was reproduced against
  the firmware or the OpenOCD 0.12 driver source.
