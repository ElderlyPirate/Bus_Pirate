# JTAG docs — round 1 fixes

Scope: `docs/protocol/jtag/**` only.  No firmware, other doc sets or WORKLOG
edits.  No commit.

**Counts:** 13 findings fixed in-set; 0 rejected; 1 code-owner candidate
(extra, found while checking #14); 5 findings are cross-set only.  Five of the
13 in-set fixes also need a cross-set follow-up (#1, #2, #3, #6, #14), so 10
findings in total require edits outside this set.

## Per-finding record

| # | Decision | What changed (file + short quote) | Evidence |
|---:|---|---|---|
| 1 | fixed in-set; cross-set | `wiring.md` new §Power with `3.3V` and `5.0V` rows: "Wire the target's VCC to **one** rail only, never both." `README.md`: "powers the `3.3V` and `5.0V` header rails". `entry.md`, `openocd.md`, `xsvf.md`, `edge-cases.md`: every "VCC pin" replaced by the two rails. | `hardware.h` `bp_enable_voltage_regulator()` drives `BP_VREGEN`; `proc_menu.c` `switch_psu_on()` enables it and then requires `BP_ADC_3V3 > V33L` and `BP_ADC_5V0 > V5L`; `binary_io.c` `BITBANG_COMMAND_JTAG_XSVF` calls `bp_enable_voltage_regulator()` with no rail check. The project hardware overview also states both supplies are enabled by a microcontroller pin. |
| 2 | fixed in-set; cross-set | `wiring.md`: the "Firmware line" column and `AUX0` are gone; the table is "JTAG signal / Terminal pin / Selection token" with `AUX` (`A0`) and `AUX1` (`A1`), and "On v3, TRST is the `PGD` pin on the programming header". | `hardwarev4.h`: `#define AUX AUX0` and `BP_AUX0` "pin 5 (AUX) is RD5". `messages_v4.s`: "c/C/k/K AUX assignment (A0/CS/A1/A2)". `hardwarev3.h`: `BP_PGD` "PGD pin on programming header". |
| 3 | fixed in-set; cross-set | `xsvf.md` Limits: "**3.3 V push-pull only.** ... a 1.8 V or 2.5 V target cannot use it." `wiring.md`: "A 1.8 V or 2.5 V target **cannot use the XSVF player**". `edge-cases.md`: "OpenOCD only: open-drain and pull-ups; the XSVF player cannot". | `tool_xsvf/ports.c` `xsvf_setup()` makes TDI/TCK/TMS outputs; the player writes no open-drain control bits. `binary_io.c` 0x18 calls `bp_enable_voltage_regulator()`. `micro.c` has no open-drain path. |
| 4 | fixed in-set | `README.md` availability: "OpenOCD host \| POSIX only (native Windows builds exclude the driver)" and "XSVF host \| — \| Windows or Linux"; hardware line: "native Windows builds of OpenOCD exclude the Bus Pirate driver, and a Cygwin/POSIX layer may work." | `docs/tools/openocd/README.md` already states it. Upstream OpenOCD `src/jtag/drivers/buspirate.c` includes `<termios.h>` and `<sys/ioctl.h>` and carries "TODO add support for WIN32". |
| 5 | fixed in-set | `tap.md` worked walk now shows the exchange: "Send the header, wait for the 3-byte echo, then send the payload as three 4-byte groups", with the three payload lines and "6 TDO bytes". | `openocd.c` `CMD_TAP_SHIFT` writes the 3-byte echo before reading payload, reads 4 bytes per 16-bit group when `bit_sequences > 8`, and subtracts 16 per group. 43 bits = 16 + 16 + 11, so 12 payload bytes and 6 TDO bytes. |
| 6 | fixed in-set; cross-set | `README.md`: "The XSVF tool set calls these the **firmware player** and the **host tools**; they are the same pair." `xsvf.md`: "That set calls the firmware sub-mode the **firmware player**". | `docs/tools/xsvf/README.md` and `limits-and-errors.md` use "firmware player"/"host tools" for the same two things. |
| 7 | fixed in-set | `README.md` status: "OpenOCD path on v3 (build-only, unverified) and v4; the XSVF player is v4 only. v3 behaviour in this set is unverified." | `configuration.h` puts `BP_JTAG_XSVF_SUPPORT` inside `#ifdef BUSPIRATEV4`; `binary_io.c` 0x18 is likewise v4-only. `BP_JTAG_OPENOCD_SUPPORT` is unconditional. |
| 8 | fixed in-set | `tap.md` Capture-IR purpose: "load status bits into the IR shift path". | IEEE 1149.1 Capture-IR loads a status pattern; the page's later sentence already said "Capture-IR loads device status bits". |
| 9 | fixed in-set | "bit sequences" replaced by "bits" in `README.md`, `openocd.md` (count and worked exchange) and `tap.md` ("43 bits"). | `openocd.c` shifts one TDI/TMS pair per bit; `configuration.h` `BP_JTAG_OPENOCD_BIT_SEQUENCES_LIMIT 0x2000`; "bit sequence" was the firmware loop term, not a user unit. |
| 10 | cross-set only | No in-set change. Required: `docs/tools/xsvf/README.md:38` "Wire TDI→MOSI..." becomes "Wire MOSI→TDI, MISO←TDO, CLK→TCK, CS→TMS". | The same page's table at lines 67-73 is "Bus Pirate / JTAG" with `MOSI / TDI`, `MISO / TDO`, `CLK / TCK`, `CS / TMS`; that is the convention every JTAG table uses. |
| 11 | cross-set only | No in-set change. Required: `converter.md:52` remove "(XC9500/XL/XV only)" or add a note that the bundled ReadMe's recipes also pass `-r 0` (no retries) for other families. | `scripts/BPXSVFPlayer/SVF2XSVF/ReadMe.txt` line 26 restricts `-r N` to XC9500/XL/XV, but lines 39-55 pass `-r 0` for CoolRunner/II, XC18V00/XCF00S, XCF00P and System ACE. |
| 12 | cross-set only | No in-set change. Required: `docs/tools/xsvf/workflow.md:82-83` "A clean Windows run of the shipped sample..." becomes "An example Windows session:"; `players.md:69-72` drop the `run_log.bat` paragraph. | Template §10/§12: user docs carry no capture provenance and no internal log names. The same example is already in `players.md#reading-the-output`. |
| 13 | fixed in-set | Added `## Related` blocks to `entry.md`, `tap.md` and `clock.md` (README, entry, wiring, tap, clock, openocd, xsvf, edge cases; the page itself omitted). | All relative links and anchors in the set were rechecked and resolve. |
| 14 | fixed in-set; cross-set; code-owner | `xsvf.md` stalls: results `0x01` at the next command boundary, `0x04` when `0xFF` is the command byte, `0x06`/`0x05` for a shift size or TAP state; `edge-cases.md` row lists the same. Added the XCOMMENT exception. | `ports.c` `xsvf_wait_byte()` sets `xsvf_stream_stalled`; `readByte()` then serves `0xFF`. `micro.c` `xsvfRun()` returns `XSVF_ERROR_UNKNOWN` (1) when the flag is seen at entry; `xsvfDoIllegalCmd()` gives 4 for command `0xFF`; `xsvfDoXSDRSIZE()`/`XSIR2` give 6; `xsvfGotoTapState()` gives 5 for an invalid state. The XCOMMENT loop is the code-owner item below. |
| 15 | cross-set only | No in-set change. Required: `docs/tools/xsvf/players.md:112` "four bytes per device in chain order" becomes "nearest-TDO device first". | `jtag.c` probe shifts 32 bits per device and transmits them in the order received; the first device out is nearest TDO. `xsvf.md` already says "the device nearest TDO first". |
| 16 | cross-set only | No in-set change. Required: `docs/tools/xsvf/workflow.md:153-155` "the terminal power-supply check reports..." becomes "After resetting to the terminal, `W` reports ...". | The player loop consumes bytes as commands and does not run the terminal; `W` is unreachable until the board is reset or returned to the prompt. |
| 17 | fixed in-set (mechanism corrected) | `wiring.md` reset bullet: "A pin write inside `BBIO1` can leave AUX (SRST) low; a terminal AUX write on the v4 `A1` target can leave AUX1 (TRST) low even after binary entry." `entry.md`: "SRST / TRST \| not driven by the player". | `binary_io.c` `reset_state()` resets AUX0/MOSI/CLK/MISO/CS but never AUX1. `aux_pin.c` `bp_aux_pin_set_low()` drives whichever AUX target is selected. `base.c` `bp_reset_board_state()` clears `alternate_aux` and AUX1 only on a board reset. The auditor's "binary entry resets it" is true only for the default AUX target. |
| 18 | fixed in-set | `openocd.md` worked exchange line: "VREG on only if the board powers target". | `openocd.c` `FEATURE_VREG` calls `bp_set_voltage_regulator_state()` with no rail check. The warning above the example already says to leave it off for a self-powered target. |

## Extra in-set correction

| Item | Change | Evidence |
|---|---|---|
| No standalone TAP-engine mode | `README.md`: "There is no standalone TAP-engine mode: the OpenOCD path is a raw shifter and the XSVF player owns the TAP." | `openocd.c` `0x05` only shifts TDI/TMS and returns TDO; `jtag.c` walks the TAP for XSVF. |

## Code-owner candidate

| Item | Evidence | Suggested direction (not applied) |
|---|---|---|
| A chunk-wait stall inside an `XCOMMENT` string traps the player | `Firmware/tool_xsvf/micro.c` `xsvfDoXCOMMENT()` runs `do { readByte(&ucText); } while (ucText);`. `Firmware/tool_xsvf/ports.c` `readByte()` returns `0xFF` indefinitely once `xsvf_stream_stalled` is set by `xsvf_wait_byte()`. `0xFF` is never the NUL terminator, so the loop never ends; `jtag()` never regains its command loop and `0x0F` is not read. Recovery is a board reset. | Make `xsvfDoXCOMMENT()` stop when `xsvf_stream_stalled` is set, or make `readByte()` return `0x00` after a stall; then the run can unwind with the normal `0x01` result. |

## Cross-set changes for the main session

Do not edit these from this set.

| Finding | File | Required change | Evidence |
|---:|---|---|---|
| 1 | `docs/tools/xsvf/README.md` lines 41-42, 61-63 | Replace "feeds **VCC**" / "feeds the target's VCC from the on-board regulator" with the two rails: "drives the `3.3V` and `5.0V` header rails. Wire the target's VCC to one rail, never both." | Same as #1: `BP_VREGEN` plus the `W` check of both ADC rails. |
| 1 | `docs/tools/xsvf/workflow.md` lines 43-45, 119, 145, 153-155 | Same rail wording.  In §7, "the firmware player feeds VCC on entry; check the VCC pin" becomes "the firmware player drives the `3.3V`/`5.0V` rails; check the rail, ground and regulator". | Same as #1. |
| 1 | `docs/tools/openocd/workflow.md` line 61 | "the regulator feeds VCC" becomes "the regulator drives the `3.3V`/`5.0V` rails; connect the target to one". | Same as #1. |
| 1 (optional) | `docs/tools/basic/README.md` line 79 | "target VCC on the VCC pin" should name the rails too. | Consistency with the other sets; same firmware path. |
| 2 | `docs/tools/openocd/README.md` lines 31 and 66; `workflow.md` line 23 | Replace `AUX0` with `AUX`; keep `AUX1 (v4) / PGD (v3)` for TRST. | Same as #2: the terminal and help use `AUX`; `A0` is the selection token, `AUX0` is internal. |
| 3 | `docs/tools/xsvf/README.md` Limits | Add "Lower-voltage target: not supported. The player is 3.3 V push-pull and switches VREG on; use OpenOCD open-drain with external pull-ups." | Same as #3. |
| 3 | `docs/tools/xsvf/workflow.md` lines 46-48 | Change "Do not drive a lower-voltage chain push-pull" to state that the XSVF player cannot drive a 1.8 V or 2.5 V target at all. | Same as #3. |
| 3 (optional) | `docs/tools/xsvf/limits-and-errors.md` firmware-limits table | Add a lower-voltage row next to the 400-bit and chunk rows. | Same as #3. |
| 6 | `docs/tools/xsvf/README.md` lines 14-17; `limits-and-errors.md` lines 9-10 | Add the reverse map: "Elsewhere these are the XSVF player / XSVF host player." | This set's README and `xsvf.md` now carry the other half. |
| 10 | `docs/tools/xsvf/README.md` line 38 | "Wire TDI→MOSI, TDO→MISO, TCK→CLK, TMS→CS" becomes "Wire MOSI→TDI, MISO←TDO, CLK→TCK, CS→TMS". | Same as #10. |
| 11 | `docs/tools/xsvf/converter.md` line 52 and the notes after the option table | Remove "(XC9500/XL/XV only)" or explain that the bundled ReadMe's recipes pass `-r 0` (no retries) for the other families. | `scripts/BPXSVFPlayer/SVF2XSVF/ReadMe.txt` lines 26 and 39-55. |
| 12 | `docs/tools/xsvf/workflow.md` lines 82-83 and 104; `players.md` lines 69-72 | "An example Windows session:"; "The same example, with more lines, is in players.md"; drop the stale `run_log.bat` paragraph. | Template §10/§12. |
| 14 | `docs/tools/xsvf/limits-and-errors.md` lines 48-65; `workflow.md` lines 125 and 138 | Replace "normally ends with `0x04`" with the code-accurate order (`0x01` at the next command boundary, `0x04` for a command byte, `0x05`/`0x06` for state/size fields). Add the XCOMMENT exception and say reset is the only exit there. | Same as #14 and the code-owner candidate. |
| 15 | `docs/tools/xsvf/players.md` line 112 | "in chain order" becomes "nearest-TDO device first". | Same as #15. |
| 16 | `docs/tools/xsvf/workflow.md` lines 153-155 | "check the VCC pin, ground and the regulator (the terminal power-supply check reports...)" becomes "After resetting to the terminal, `W` reports ...". | Same as #16. |

## Remaining questions

- The XCOMMENT hang is a firmware defect, not just a doc issue.  The docs
  now tell the reader to reset the board; the firmware owner should make the
  comment loop terminate on a stalled stream.
- Both header rails are documented as VREG-switched on the strength of the
  `W` rail check and the project hardware overview.  No schematic is in this
  tree; if a board revision leaves one rail always on, the `wiring.md` Power
  table needs that qualifier.
- The v3 column remains build-only and unverified.  No v3 claim was
  strengthened.
- The XSVF result ordering in #14 is code-derived; no hardware run was made.
  The tool set's current "normally `0x04`" wording should be aligned with it.
- The converter `-r` contradiction exists in the bundled ReadMe as well as
  the tool page; the tool owner should decide whether the recipes or the
  option table is authoritative.
- The native-Windows/OpenOCD limitation is kept as the tool set states it.
  Cygwin behaviour was not tested here.
