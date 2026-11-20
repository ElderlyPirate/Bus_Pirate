# Core documentation — round 1 implementation record

**Scope:** `docs/core/` plus the cross-set lines in `docs/generic/` and
`docs/dev/` cited by the feedback.  Worktree `bpv4-docssweep`, branch
`docs/combined-sweep` at `164b9bf`; firmware ground truth is the `Firmware/`
tree at that commit.  No firmware changes, no commits, no WORKLOG edits.

## Decisions

| # | Decision | What changed (file + short quote) | Evidence |
|---|---|---|---|
| 1 | fixed | `system.md` dispatcher row: "`<n=text>` ... the first `>` ends the body ... see [macros]" | `proc_menu.c` `case '<'` scans for the first `>`, then `getint()` and an optional `=body`; `generic/macros.md` already states the first `>` terminates the body |
| 2 | fixed | `system.md` base helpers: "31-bit ... the final right shift discards the top bit ... wraps every 35.8 minutes"; `generic/commands.md`: "31-bit ... about 35.8 minutes" | `base.c` `bp_timer_microseconds()` returns `((timer_overflows << 16) \| TMR1) >> 1`; 2^31 µs is 35.79 minutes |
| 3 | fixed | `system.md` heading is now "Terminal reset and exit paths"; new "Board-state reset (`bp_reset_board_state()`)" has the 8-step order, a caller table and links to `eeprom.md`/`smps`; lifecycle step 2 points to it | `base.c` `bp_reset_board_state()`; callers in `main.c` `initialize_board()`, `proc_menu.c` `changemode()`/`bp_change_mode()`/`reset_terminal_state()`/`$`, and `selftest.c` `perform_selftest()` |
| 4 | fixed | `system.md` gains "## Contents" with grouped anchors for every H2 | All anchors check against the headings; the page was not split (see remaining questions) |
| 5 | fixed | `README.md` module map is one row per group with a Detail cell; entry-point lists removed (detail pages own them); "Where the other layers live" is a grouped table, no 477-character cell | `core.c` registry groups; each detail page's API/Scope tables; the README is 150 lines |
| 6 | fixed | `README.md` gains an 8-row "Maintainer index" (mode, key, reset order, string, hung terminal, mode engage, I2C timeout, EEPROM); test paragraph now points to `dev/tooling.md#test-suites` | Failure sections exist on every core detail page; the test inventory is the `dev/tooling.md` suite table |
| 7 | fixed | `system.md` registry is a default-v4 table plus a four-combination KEYB/LCD matrix; the duplicate "9" annotation row is gone | `core.h` enum order: HIZ, 1WIRE, UART, I2C, SPI, 2WIRE, 3WIRE, [KEYB], [LCD], PIC, DIO; `configuration.h` default v4 KEYB on/LCD off, v3 KEYB off |
| 8 | fixed | `system.md` descriptor section adds a Required?/Reusable body table, separates `null_*` operation callbacks from `reset_mode_to_8_bits`, names DIO's silent `setup_execute`, and states the NULL consequence | `core.c` initializers and null callbacks; `proc_menu.c` `^` is the only NULL test (`clock_pulses`); `silent_null_operation_callback()` clears `command_error` |
| 9 | fixed | `usb.md` states the lock is one flag (do not `setLock()` twice and `freeLock()` once), identifies span invalidation via `usb_device_state`/`SET_CONFIGURATION`, and adds host-not-reading symptom/recovery | `cdc.c` `setLock()`/`freeLock()`/`WaitInReady()`/`putda_cdc()`; `usb_stack.c` `usb_handle_reset()`; `usb_stack.h` state values |
| 10 | fixed | `messages.md`: "the contents are deterministic. The output prefix is relative to the process working directory"; the two `packstrings.py` invocations and `cd tools/packstrings` are inline | `packstrings.py` writes `args.outbase + '.s'/'.h'` relative to the cwd; includes resolve from the input file |
| 11 | fixed | `bitbang.md`: the I2C wrappers clear at the start of each transaction, raw callers call `bitbang_i2c_clear_error()`, and `bitbang_setup()` does not clear | `i2c.c` `i2c_start()`/`i2c_write()`/`i2c_read()`/`i2c_stop()` clear first; `bitbang.c` `bitbang_setup()` only sets `miso_pin`/`delay_profile` |
| 12 | fixed | `bitbang.md` data model, flow and invariants use `bitbang_clock_stretch_timeouts()` and say it reads the internal saturating counter | `bitbang.c` static `bitbang_clk_stretch_timeouts` and accessor `bitbang_clock_stretch_timeouts()` |
| 13 | fixed | `system.md` key cell is now "`=` / `\|`" | `proc_menu.c` `case '='` calls `convert_value(NO)`; `case '\|'` calls `convert_value(YES)` |
| 14 | fixed | `system.md` names the two real payloads: "`1-WIRE speed: standard` and `1-WIRE speed: overdrive`" | `messages_v4.s` `MSG_1WIRE_SPEED_STANDARD` and `MSG_1WIRE_SPEED_OVERDRIVE` |
| 15 | fixed | `system.md`: device type/revision come from the config-memory ID words (`0x00FF0000`/`0x00FF0002`), not user `CFG0`..`CFG2` | `main.c` `bp_read_from_flash(DEV_ADDR_UPPER, DEV_ADDR_TYPE/REV)`; `hardware.h` `0x00FF` + `0x0000`/`0x0002`; `hardwarev4.h` `CFG_ADDR_UPPER`/`CFG_ADDR_0` |
| 16 | fixed | `README.md`: "the only runner is that per-suite script ... there is no aggregate runner and no `test_native.py`"; `dev/building.md` and `dev/tooling.md` drop the missing runner and document direct suite runs | `Firmware/tests/` contains only per-suite scripts; no `test_native.py` exists in the tree |
| 17 | fixed (cross-set) | `generic/navigation.md` `m<N>` row adds "and the finish-setup prompt"; core already described the direct form correctly | `proc_menu.c` menu path prints `MSG_CLUTCH_DISENGAGED` and `MSG_FINISH_SETUP_PROMPT`; the direct path prints only `BPMSG1085` ("Ready") |
| 18 | fixed | `system.md`, `usb.md`, `bitbang.md`, `eeprom.md` and `messages.md` failure sections are four-column trigger/symptom/behaviour/recovery tables | Each row re-checked against the source cited in this table; no content dropped without a home |
| 19 | fixed | `messages.md` replaces the 288/308 count table with the shared-set relationship; `eeprom.md` says "the full self-test value/WP-entry matrix" | `messages_v3.s`/`messages_v4.s` currently label 288/308 entries, but the count is a build artifact; the EEPROM fixture iterates the value/WP matrix |
| 20 | fixed | `README.md`: "New to the core: read `system.md` first, then the module you are changing; each page ends with its suite and related pages." | All five detail pages have Native test coverage and Related docs sections |

## Counts

- fixed: 20
- rejected / not a defect: 0
- code-owner candidates: 0

## Files changed

- `docs/core/README.md`
- `docs/core/system.md`
- `docs/core/usb.md`
- `docs/core/bitbang.md`
- `docs/core/messages.md`
- `docs/core/eeprom.md`
- `docs/generic/commands.md` (finding 2)
- `docs/generic/navigation.md` (finding 17)
- `docs/dev/building.md` and `docs/dev/tooling.md` (finding 16)

## Remaining questions

- Finding 4's preferred fix was a three-way split of `system.md`.  This pass
  added the contents block and kept the page intact: splitting would break
  existing `system.md#...` links from other sets and duplicate the mode-change
  and reset order.  The page is 667 lines; a later structural pass can still
  split it behind redirects.
- The README is 150 lines against the template's ~120 target.  The extra lines
  are the maintainer index and the two maps the round-1 feedback asked for; a
  later pass could move the layer-link table to `dev/code-map.md`.
- `Firmware/tests/test_native.py` is absent.  The dev pages now describe the
  per-suite model instead of a missing runner.  If the runner is intended, the
  code owner should restore it or confirm that the per-suite scripts are the
  complete model.
- Exact message counts no longer live in `core`.  A release check should count
  the committed `messages_v3.s`/`messages_v4.s` labels rather than copy the
  numbers into prose.
- `docs/dev/verification.md` still carries maintainer bench dates and pass
  counts.  That is its stated purpose; it was not edited from this set.
- The v3 hardware claims remain source-derived and caveated as
  "not verified on v3 hardware"; no v3 evidence was available.
