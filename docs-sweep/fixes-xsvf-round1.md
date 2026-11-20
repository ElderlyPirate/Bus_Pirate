# XSVF docs — round 1 implementation record

Worktree: `C:\Users\jschl\dev\bpv4-docssweep`, branch `docs/combined-sweep`
(workspace HEAD `ca88960`; the `Firmware/` tree is the combined image
`c57e223`).  No commit, no firmware change, no host-tool change, no WORKLOG
edit.  Edits are restricted to `docs/tools/xsvf/**`.

Decision key: **fixed** = user docs changed; **not a defect** = no change;
**code-owner** = genuine firmware or host-tool candidate recorded, no code
edit; **cross-set** = the defect is in another documentation set and is
recorded for the main session, not edited here.

| # | Decision | What changed (file + short quote) | Evidence |
|---|---|---|---|
| 1 | fixed; code-owner | `players.md` §The chain scan: "**Do not combine `-x` with `-f` in one invocation.** ... Run `-x` in its own invocation, press reset ... then run the file with `-r -f`."  `workflow.md` step 4: "**Do not add `-x` to this command.** ... a short read leaves bytes that the run reads as its result."  The Windows worked command is now `-r` only and the scan transcript is gone from it; both worked transcripts mark the elided chunks with `...` and explain them below the fence.  The batch-file paragraph warns that `run.bat`, `run2.bat` and `package/run.bat` combine `-x` with `-f`. | `scripts/BP4_XSVFplayer_Win/main.c`, `main()`: the scan loop's successful `serial_read(fd, buffer, sizeof(buffer))` is printed by `res` bytes; `buffer[0]` is never used as a count and there is no flush before `temp[0]=0x03`.  `scripts/BPXSVFPlayer/main.c`, `do_chain_scan()`: same one-read sizing, no count use, no drain.  Windows run loop indexes `XSVF_ERROR[c]` with `c=buffer[0]` and no bound (9 entries); cross-platform guards with `c > XSVF_ERROR_LAST`.  `scripts/BPXSVFPlayer/package/run.bat` and the two Windows batch files pass `-x` with `-f`.  See CO-1. |
| 2 | fixed | `README.md` five-minute exit bullet: "**Press reset** to get the terminal back; the rails go off.  To re-power them, enter a mode (`m`) and then `W`; `W` also runs that mode's setup and is refused in `HiZ>`."  `workflow.md` already had the correct mode-first wording and was kept. | `Firmware/core_system/proc_menu.c`, `switch_psu_on()`: `if (bus_pirate_configuration.bus_mode == BP_HIZ) { BPMSG1088; return; }`; `BPMSG1088` is `"Command not used in this mode"` (`Firmware/core_messages/messages_v4.s`).  A board reset returns to HiZ (`bp_reset_board_state()` sets `bus_mode = BP_HIZ`).  `switch_psu_on()` calls `bp_current_protocol()->setup_execute()` after the rail check. |
| 3 | fixed; code-owner; cross-set follow-up | `workflow.md` §6 now has a human-exit table (reset always; `0x0F` twice only at the command loop; `0x0F` during a run never) and a pySerial recipe: `python -c "import serial,time; p=serial.Serial('COM5',115200); ... p.write(b'\x0f') ..."`, with the Linux port and the ~5 s wait.  `players.md`, `player.md`, `README.md` and `limits-and-errors.md` now label the `0x0F` pair a raw-serial-host step; the stock players close the port. | `Firmware/protocol_jtag/jtag.c`, `jtag()` `case 0x0F`: `jtag_reset(); return;` — a player command only when the loop reads it.  The second `0x0F` is `BITBANG_COMMAND_RETURN_TO_TERMINAL` in `Firmware/protocol_binary/binary_io.c`; on v4 it sends `0x01`, calls `reset_state()`, returns, and `proc_menu.c` runs `reset_terminal_state()` (VREG off, version, `HiZ>`).  The shipped `main()` functions return after `Thank you for playing!` without sending `0x0F`.  Cross-set: XS-2.  Code-owner: CO-5 (the documented `XCOMMENT` reset-only trap). |
| 4 | fixed; code-owner | `players.md` exit table and bullets now state: Windows `0` after any run and `-1` for a setup failure; cross-platform `0` for a clean result **or** `End of file reached.`, `1` firmware error, `2` transport/no reply, `-1` (`255` in a Unix shell) for setup failure.  Added "The success check is `End of operation reply: XSVF_ERROR_NONE` followed by `Success!`; do not trust `0` alone" and the Windows bad-`-f` exception. | `scripts/BPXSVFPlayer/main.c`, `main()`: `int status=0`; `return -1` on `serial_open`, `BP_EnableBinary`, `BP_EnableMode`, `fopen`, `malloc` and the no-file branch; `status=1` for a non-NONE/non-READY result; `status=2` for a reset write failure, `do_chain_scan()` failure, or no reply; the `XSVF_READY_FOR_DATA` + `fileSize==0` branch prints `End of file reached.` and does not change `status`, so it returns 0.  Windows `main()` returns 0 at the end.  See CO-3. |
| 5 | fixed | `workflow.md` step 1: "An XSVF written directly by iMPACT has no listing: keep the SVF and convert it with `-a` for the check, inspect the XSVF with a parser, or treat `XSVF_ERROR_DATAOVERFLOW` (`0x06`) as the check after a run."  `converter.md` §Checking the shift size repeats the same limit and says keeping the SVF is the simplest way to get the number before the run. | `scripts/BPXSVFPlayer/SVF2XSVF/ReadMe.txt`: `-a output.txt` is an option of `svf2xsvf501/502`; iMPACT is the recommended direct generator and is a different tool.  `Firmware/tool_xsvf/lenval.h`: `MAX_LEN 50`; `micro.c` `xsvfDoXSDR()` / `xsvfDoXSIR()` return `XSVF_ERROR_DATAOVERFLOW` over `MAX_LEN` bytes. |
| 6 | cross-set (XSVF page verified) | `wiring.md` pull-up paragraph now says a custom host's `BBIO1` pin write survives `0x18` and "A board reset or the return to the terminal switches them off."  The substantive contradiction is in `docs/protocol/jtag/edge-cases.md`; recorded as XS-1, not edited. | `Firmware/protocol_binary/binary_io.c`, `enter_binary_bitbang_mode()` calls `reset_state()`; `reset_state()` calls `bp_disable_3v3_pullup()`, `bitbang_pin_direction_set(0xFF)` and `bitbang_pin_state_set(0x00)`; `bitbang_pin_state_set()` clears VREG (`0b01000000`) and pull-ups (`0b00100000`).  `case BITBANG_COMMAND_JTAG_XSVF` calls only `bp_enable_voltage_regulator()` before `jtag()`, so it does not clear a custom host's pull-up write.  `docs/protocol/binary/entering-and-framing.md` already says the XSVF entry "leaves the current pin state alone and only switches the on-board regulator on". |
| 7 | fixed | `README.md` new §Get the tools: "The host players and the converters are in the firmware repository under `scripts/BP4_XSVFplayer_Win/` and `scripts/BPXSVFPlayer/` ... The repository does not install them: run the executable from its folder, or build it with the included project or `Makefile`." | `scripts/BP4_XSVFplayer_Win/` carries the `.exe`, sources, Dev-C++ project, `Makefile.win`; `scripts/BPXSVFPlayer/` carries sources, `Makefile`, Code::Blocks project, `package/`.  `docs/dev/tooling.md` states the repository does not package or install the host tools. |
| 8 | fixed | `converter.md` option notes: "`-r 0` disables retries.  The parenthetical in the option table means a non-zero retry count is meaningful only for XC9500/XL/XV; the other family recipes pass `-r 0` to turn retries off." | `scripts/BPXSVFPlayer/SVF2XSVF/ReadMe.txt` line 26: `-r N = max "retry" count = N times. (XC9500/XL/XV only)`; the family recipes at lines 39-55 pass `-r 0` for CoolRunner/II, XC18V00/XCF00S, XCF00P and System ACE. |
| 9 | fixed | `players.md` output row is now `Sending N Bytes (total)...` — "the number in parentheses is the running total after the chunk, hex".  The transcript explanation: "the last full chunk totals `D000` (53248).  The final 41 bytes bring the total to `D029` (53289), the whole file." | `scripts/BP4_XSVFplayer_Win/main.c` and `scripts/BPXSVFPlayer/main.c`: `cnt=cnt+readSize; printf(" Sending %i Bytes (%04X)...",readSize, cnt);` — `cnt` is cumulative, not a chunk offset.  13 × 4096 = 53248 = `0xD000`; 53248 + 41 = 53289 = `0xD029`. |
| 10 | fixed; code-owner | `players.md` step 7: "Stops when the file is used up, or when the player returns a result; the cross-platform build stops at the result, while the Windows build sends one more chunk after an early error and then ends with `No reply.... Quitting.`"  The output row and exit section repeat the per-build difference.  The failure transcript in `workflow.md` now uses the cross-platform build (which stops at the result) and notes the Windows extra chunk and `0` exit. | Cross-platform `main()` sets `if (buffer[0] != XSVF_READY_FOR_DATA) timer_out=-1;` after a result, so it leaves the loop.  Windows `main()` sets `timer_out=-1` only in the no-reply branch; after an early non-`0xFF` result it prints the result, breaks the read loop, `fileSize != 0` and `timer_out == 0`, so it sends another chunk; the player is at its command loop and no ready byte follows, so the tool prints `No reply.... Quitting.` and returns 0.  The auditor's "stops on any result" is true only for the cross-platform build.  See CO-2. |
| 11 | fixed | `players.md` table cell: "no `ok` after the two entry lines; run `ok` suffixes still appear." | `scripts/BP4_XSVFplayer_Win/buspirate.c` prints `ok` after `BP_EnableBinary` and `BP_EnableMode`; the cross-platform `buspirate.c` prints the entry lines without `ok`; both `main()` run loops print `ok` after every successful read (`Waiting for first data request...ok`, `Sending ...ok`). |
| 12 | fixed | `players.md` and `workflow.md` examples now read `Chain Scan Result: 04 93 40 60 59`, explained as "the example IDCODE `0x59604093`" (bit 0 = 1).  The `FC` rows are split: count `FC` with all-zero bytes means TDO stuck low; count `FC` with plausible IDCODEs means a real chain over 63 devices, with only the first 63 reported; `limits-and-errors.md` chain-probe table uses the same split. | IEEE 1149.1 IDCODE bit 0 is 1; `0x59604093` has bit 0 set and decodes with the Xilinx manufacturer field.  `Firmware/protocol_jtag/jtag.c` `case 2`: the bypass count stops at 250 words, is clamped to 63, then 32 bits per device are read after `jtag_reset()`; a stuck-low TDO gives the cap and all-zero data, while a real >63 chain gives the first 63 IDCODEs. |
| 13 | fixed | Rewrapped the three short lines (`players.md` build paragraph and exit paragraph; `wiring.md` intro).  Removed `(chunk lines elided)` and `(the player aborts ...)` from inside the fences; the elisions are `...` lines explained below.  The cross-platform command now uses `$`.  The 139-character converter cell in §What ships is split into Windows/Unix/ReadMe rows. | Line-width and TAB checks over all seven files; the Linux commands use `$` in `players.md` and `workflow.md`; the corpus has no TAB characters. |
| 14 | fixed | `README.md` naming paragraph: "The host programs' banner also reads `BusPirate XSVF Player`; that is the PC tool.  The firmware mode answers only `XSV1`." | `scripts/BP4_XSVFplayer_Win/main.c` prints ` BusPirate XSVF Player V.02`; `scripts/BPXSVFPlayer/main.c` prints ` BusPirate XSVF Player V.01`; the firmware's `0x18` reply is `XSV1` (`Firmware/protocol_binary/binary_io.c`). |
| 15 | fixed; code-owner | `limits-and-errors.md`: "`Error opening file`: fix the `-f` path; the cross-platform build exits, the Windows build can terminate after the message".  `players.md` exit section: "On the Windows build, a bad `-f` path is not reported this way: it prints `Error opening file` and may terminate." | `scripts/BP4_XSVFplayer_Win/main.c`, `main()`: `XSVF = fopen(...); if (XSVF == NULL) { printf(" Error opening file\n"); } fseek(XSVF, 0, SEEK_END);` — no return, null handle; `fclose(XSVF)` later.  Cross-platform `main.c` returns `-1` at the same point.  See CO-4. |
| 16 | fixed; cross-set follow-up | `workflow.md` step 7: "To check the regulator, press reset, enter a mode (`m`), then `W`; `W` also runs that mode's setup.  If it prints `VREG too low, is there a short?`, check the rail for a short."  `wiring.md` reset table: AUX "reset, then `c`, `A`"; AUX1 "reset, then `k`, `A`", with the explanation that `c`/`k` select the `a`/`A` target and `A` drives it high.  The supply bullet points at the same reset/mode/`W` check. | `Firmware/core_system/proc_menu.c` `switch_psu_on()` refuses `BP_HIZ` with `BPMSG1088`; after the mode check it enables VREG, waits 2 ms, reads `BP_ADC_3V3` and `BP_ADC_5V0`, and prints `MSG_VREG_TOO_LOW` on failure.  `case 'c'` sets `alternate_aux=0`, `case 'k'` sets 2, `case 'A'` calls `bp_aux_pin_set_high()`; `Firmware/capability_aux/aux_pin.c` `bp_aux_pin_set_high()` sets the selected pin high and an output.  Messages `BPMSG1086`/`BPMSG1263` map `a`/`A` to AUX/AUX1.  Cross-set: XS-3. |
| 17 | fixed | `README.md` §Availability: "**v3:** there is no XSVF player.  Use OpenOCD's own `svf` command through the `0x06` path ([the OpenOCD set](../openocd/README.md)); it plays SVF, not XSVF."  Related-doc label changed to "the v3 and larger-device alternative". | `Firmware/configuration.h`: `BP_JTAG_XSVF_SUPPORT` is under `#ifdef BUSPIRATEV4`; `binary_io.c` `case BITBANG_COMMAND_JTAG_XSVF` calls `REPORT_IO_FAILURE()` on v3 (a `0x00` reply).  `BP_JTAG_OPENOCD_SUPPORT` is enabled for both v3 and v4. |

## Counts

- Findings processed: 17.
- **Fixed** in `docs/tools/xsvf/**`: 16 (findings 1–5 and 7–17).
- **Rejected / not a defect**: 0.
- **Cross-set only**: 1 (finding 6; the XSVF wiring page was correct, one sentence
  was tightened, and the contradiction is in the JTAG set).
- **Code-owner candidates**: 5 (findings 1, 3, 4, 10 and 15; all five also have
  doc fixes, and finding 3's code-owner item is the documented `XCOMMENT` limit).
- **Cross-set changes required**: 3 (finding 6 plus follow-ups from findings 3
  and 16).

Finding 10's doc correction goes beyond the auditor's wording: the auditor said
the host stops on any result byte, which is true for the cross-platform build but
false for the Windows build.  The pages now state both behaviours.

## Code-owner candidates (no code changed)

### CO-1 — the chain scan is not sized or drained (finding 1)

- Files/symbols: `scripts/BP4_XSVFplayer_Win/main.c`, `main()`; and
  `scripts/BPXSVFPlayer/main.c`, `do_chain_scan()`.
- The Windows scan loop reads with `serial_read(fd, buffer, sizeof(buffer))`
  and prints `res` bytes.  It never reads the count byte and never flushes
  before `temp[0]=0x03`.  A short read leaves the count/IDCODE bytes in the
  port; the run loop treats the next byte as the result.  `XSVF_ERROR[c]` is
  indexed with the raw byte and the table has 9 entries, so a leftover byte
  above 8 is out of bounds.
- The cross-platform `do_chain_scan()` has the same one-read/no-drain shape
  (bounded to five tries), but its run loop stops on any non-`0xFF` byte; a
  leftover `0x00` therefore stops the run with the file unsent and status 0.
- Suggested owner fix: read one count byte, then exactly `count` bytes; or
  flush after the scan.  Bound the error-name index; abort or return a
  transport status when the scan fails.  The user docs now say to run `-x`
  alone and use `-r -f` for the run.

### CO-2 — the Windows host sends another chunk after an early error (finding 10)

- File/symbol: `scripts/BP4_XSVFplayer_Win/main.c`, `main()`.
- `timer_out` is reset to 0 at the top of each outer loop and set to -1 only
  in the no-reply branch.  A non-`0xFF` result prints the result and breaks the
  inner read loop; with `fileSize != 0` and `timer_out == 0`, execution falls
  through to `serial_write` another chunk.  The player is back at its command
  loop and cannot answer with a ready byte, so the host prints
  `No reply.... Quitting.` and returns 0.
- The cross-platform `main()` has the missing guard:
  `if (buffer[0] != XSVF_READY_FOR_DATA) timer_out=-1;`.
- Suggested owner fix: set `timer_out=-1` on any non-ready result and carry the
  result into the exit status.  The user docs now state the actual behaviour.

### CO-3 — the cross-platform exit status (finding 4)

- File/symbol: `scripts/BPXSVFPlayer/main.c`, `main()`.
- `int status=0`; setup failures `return -1`; firmware errors set `status=1`;
  no reply sets `status=2`.  The early-EOF branch (`XSVF_READY_FOR_DATA` with
  `fileSize==0`) prints `End of file reached.` and leaves `status` at 0.
- Suggested owner fix: return 2 on setup failures and on early EOF, or keep
  the current mapping (now documented) and make the README-level contract
  explicit.  The docs now list `0/1/2/-1` and tell scripts not to trust 0.

### CO-4 — the Windows `Error opening file` path dereferences a null handle (finding 15)

- File/symbol: `scripts/BP4_XSVFplayer_Win/main.c`, `main()`.
- `XSVF = fopen(param_XSVF, "rb"); if (XSVF == NULL) { printf(" Error
  opening file\n"); }` is followed immediately by `fseek(XSVF, 0, SEEK_END)`
  and later `fclose(XSVF)`, with no return.  The cross-platform `main.c`
  returns `-1` at the same point.
- Suggested owner fix: return a setup failure immediately after the message.
  The user docs now warn that the Windows build may terminate.

### CO-5 — a stall inside `XCOMMENT` defeats the stall recovery (finding 3, known limit)

- Files/symbols: `Firmware/tool_xsvf/micro.c`, `xsvfDoXCOMMENT()`;
  `Firmware/tool_xsvf/ports.c`, `xsvf_wait_byte()` and `readByte()`.
- `xsvfDoXCOMMENT()` loops `readByte(&ucText)` until a zero byte.
  `xsvf_wait_byte()` sets `xsvf_stream_stalled = true` after
  `XSVF_STREAM_TIMEOUT_MS` (5000) and returns `0xFF`; `readByte()` at the
  stalled/zero-length branch sets `buf[0]=0xff; bufBytes=1`, and every later
  call returns `0xFF` without clearing the flag.  `xsvfRun()` checks
  `xsvf_stream_stalled` only at the command boundary, which the comment loop
  never reaches.
- The user pages already document the reset-only exit.  Suggested owner fix:
  check the stall flag inside the comment loop, or bound the comment length.
  If the owner treats the reset-only behaviour as intentional, keep the current
  documentation.

## Cross-set changes for the main session

### XS-1 — `docs/protocol/jtag/edge-cases.md` (finding 6)

Current row:

> `| Entering binary from a mode | the header pins other than AUX1, VREG and the pull-ups reset | re-check wiring and target power; AUX1 needs a board reset |`

Needed:

> `| Entering binary from a mode | the header pins reset, VREG switches off and the on-board pull-ups switch off; AUX1/AUX2 keep their state | re-check wiring and target power; AUX1 needs a board reset |`

Evidence: `Firmware/protocol_binary/binary_io.c` `enter_binary_bitbang_mode()`
calls `reset_state()`; `reset_state()` calls `bp_disable_3v3_pullup()`,
`bitbang_pin_direction_set(0xFF)` and `bitbang_pin_state_set(0x00)`;
`bitbang_pin_state_set()` clears VREG and the pull-up bit.  The binary page
`docs/protocol/binary/entering-and-framing.md` already says entry sets VREG
and the pull-ups off and that the XSVF entry is the exception.  The XSVF
wiring page was correct; the edge-case row is the outlier.

### XS-2 — `docs/protocol/jtag/xsvf.md` (finding 3 follow-up)

Current §Host workflow:

> "The host tools stop after the result byte and do not leave the XSVF
> player, so the board stays in the player loop with the regulator on until it
> is reset or sent `0x0F` twice."

Needed: say that the stock tools close the port and that the `0x0F` pair is a
raw-serial-host step, e.g. "... until it is reset, or a raw-serial host sends
`0x0F` twice."

Current §Limits exit bullet:

> "**Exit leaves the regulator on.**  `0x0F` returns to `BBIO1` without
> resetting the pins or switching VREG off; use the second `0x0F`, a terminal
> reset or the reset button."

Needed: "... a raw-serial host can send it twice, or use the reset button."

Evidence: the firmware `0x0F` handler in `jtag.c` is a byte command; the host
tools return after `Thank you for playing!` and do not send it.  The XSVF set
now points at the raw-serial recipe.

### XS-3 — `docs/protocol/jtag/wiring.md` (finding 16 follow-up)

Current §Reset lines:

> "The XSVF player does not drive SRST or TRST.  A pin write inside `BBIO1`
> can leave AUX (SRST) low; a terminal AUX write on the v4 `A1` target can
> leave AUX1 (TRST) low even after binary entry.  Release it or reset the
> board before entering the player."

Needed: "At the terminal, reset, then press `c` and `A` for AUX (SRST) or `k`
and `A` for AUX1 (TRST); or reset the board."  `A1` is a selection/status
token, not a terminal command; the keys are `c`/`k`.

Evidence: `Firmware/core_system/proc_menu.c` `case 'c'` sets
`alternate_aux=0`; `case 'k'` sets 2; `case 'A'` calls
`bp_aux_pin_set_high()`; `Firmware/capability_aux/aux_pin.c`
`bp_aux_pin_set_high()` sets the selected pin high and an output.  The
messages `BPMSG1086` and `BPMSG1263` name the AUX and AUX1 targets.

## Remaining questions

- **Windows host parity (CO-2).**  Confirm whether the Windows player should
  get the cross-platform `timer_out=-1` guard.  Until it does, the docs warn
  that an early error sends one more chunk and still exits 0.
- **Windows scan-only status (finding 1/7).**  `-x` alone performs the scan
  and then exits -1 with `No file specified...`, although the help calls
  `-f` optional.  The cross-platform build returns 0.  The docs state the
  Windows behaviour; the owner can decide whether a scan-only run should
  return 0 and send `0x0F` to leave the player.
- **`XCOMMENT` stall (CO-5).**  The docs keep the reset-only exit.  If the
  owner wants the stall flag honoured inside the comment loop, the user pages
  would need to change to "the run ends with 0x01/0x04".
- **Unverified hardware limits.**  The 400-bit shift, 4096-byte chunk
  behaviour and ~5 s stall remain code-derived and **unverified** on
  hardware; no doc change.
- **Converter assumptions.**  The `-r 0` retry semantics and the family
  recipes are from the bundled `ReadMe.txt`; the translator executable was
  not run (its no-argument invocation did not return).  The `XSDRSIZE` values
  in the device-class table are the `lenval.h` figures.
- **Windows file-open path.**  The null-handle path was not executed; the
  warning and CO-4 are code-derived.
- **`0x0F` recipe.**  The pySerial sequence is the firmware command sequence;
  it was not run on hardware.
