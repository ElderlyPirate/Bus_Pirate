# Generic prompt layer — round 1 implementation record

**Scope:** `docs/generic/` plus the two `docs/SPEC.md` lines cited in the
feedback.  Worktree `bpv4-docssweep`, branch `docs/combined-sweep` at
`ef69ec1`; firmware ground truth is the `Firmware/` tree at that commit.
No firmware changes, no commits, no WORKLOG edits.

## Decisions

| # | Decision | What changed (file + short quote) | Evidence |
|---|---|---|---|
| 1 | fixed | README: "`#` also restores HEX and prints the banner; `m1` does neither ... Both are unavailable inside BASIC". navigation: "Neither runs inside BASIC ... type `EXIT` first". usage: "Neither `#` nor `m1` can be typed inside BASIC". commands: "Inside BASIC it is a syntax error; type `EXIT` first". edge-cases row: "`#`, `$`, `m`, `m1` in BASIC ... type `EXIT` first" | `Firmware/core_system/proc_menu.c` `serviceuser()` runs `bp_basic_enter_interactive_interpreter()` before the command switch whenever `bus_pirate_configuration.basic` is set, so `#`/`m1` never reach their cases; `Firmware/tool_basic/basic.c` `EXIT` clears the flag; `proc_menu.c` `case '#'` calls `reset_terminal_state()` only when not in BASIC |
| 2 | fixed | commands wrapper table: "`]` stop; closes either start", "`}` stop from read; runs the same stop as `]`", plus "`]` and `}` both stop either start ... `{` differs from `[` only in SPI and 3WIRE ... `r` and `r:N` also work outside a start/stop". usage read session: "either `]` or `}` stops it ... `r:2` after the stop is an ordinary unframed read" | `Firmware/core_system/core.c` protocol table: `2WIRE` has `.start_with_read = raw2wire_start`, `.stop = raw2wire_stop`, `.stop_from_read = raw2wire_stop`; every mode aliases `stop_from_read` to `stop`. `proc_menu.c` sends `]` to `stop()` and `}` to `stop_from_read()`; the `r` case has no transaction check |
| 3 | fixed | usage: seven-row token table (`spd`, `brg`, `dbp`, `sb`, `rxp`, `hiz`, `dup`), "Six answers produce seven fields because `brg` is computed, not typed", `R2W` prefix note, link to the 2-Wire entry page; 2WIRE intro now says "second speed (`~60kHz`)" | `Firmware/protocol_uart/uart.c` `UARTsettings()` prints `speed`, `UART_BRG_SPEED[speed]`/`U2BRG`, `databits_parity`, `stop_bits`, `receive_polarity`, `high_impedance`, `half_duplex`; `uart_setup_prepare()` maps menu answers with `-1`. `Firmware/protocol_wire/raw2wire.c` `raw2wire_print_settings()` prints `R2W (spd hiz)` from `speed`/`high_impedance` |
| 4 | fixed | README Name note adds "`2WIRE` reuses the firmware's `I2C START BIT` / `I2C STOP BIT` strings ... prefixed `R2W`". usage: "the 2WIRE firmware displays the shared I2C wrapper wording (`I2C START BIT` / `I2C STOP BIT`)" | `Firmware/protocol_wire/raw2wire.c` `raw2wire_start()`/`raw2wire_stop()` call `MSG_I2C_START_BIT`/`MSG_I2C_STOP_BIT`; strings in `Firmware/core_messages/messages_v4.s`; mode name `2WIRE` in `core.c` |
| 5 | fixed | navigation: help table captioned "transcribes the v4 screen"; v3 diff table replaced with neutral rows 4, 8, 9, 10, 17 and "`#` has a row only on v3 and `e` only on v4. On v3 `o` is row 17; on v4 it is row 4" | `Firmware/core_messages/messages_v3.s` HLP1005 row 4 `# Reset the BP`, HLP1009 row 8 `123`, HLP1010 row 9 `c/C ... aux/CS`, HLP1011 row 10 `Send value`, HLP1018 row 17 `o Set output type`; `messages_v4.s` same IDs with `o`, `Send integer value`, `c/C/k/K`, `Send binary value`, `e` |
| 6 | fixed | SPEC SUMP row: "hidden: 5–19× `0x00` then raw STX (`0x02`); 20 NULs enter binary instead" | `Firmware/core_system/proc_menu.c` `case ASCII_NUL` enters binary at counter 20; `handle_left_arrow()` enters SUMP at counter `>= 5`; `Firmware/protocol_binary/binary_io.c` `BITBANG_COMMAND_I2C = 0x02` |
| 7 | fixed | navigation notes: "Case matters" (`s` BASIC vs `S` servo, `o` format vs `O` 1-Wire speed, related pairs); "`t`, `T` and `O` are working commands with no help row; on v4 so is `#`" | `Firmware/core_system/proc_menu.c` `case 's'` sets `basic = ON`; `case 'S'` calls `bp_servo_setup()`; `case 'o'` calls `set_display_mode()`; `case 'O'` requires `BP_1WIRE`. Help strings contain no `O` row |
| 8 | fixed | README status: "v4 is the primary documented board; the v3 build is covered but not verified ... Captures in this set are v4; inline notes and the delta table in navigation.md list the differences". SPEC legend: "the v3 build is documented but not verified on v3 hardware" | All terminal captures show the v4 banner and the 12-column `v` table; `Firmware/core_system/main.c` defaults `display_mode = HEX`; v3 deltas already listed in navigation. Removes the undefined "build-only" term from the generic set |
| 9 | fixed | usage top: "Every session starts at `HiZ>` and the capture shows that prompt ... run `m1` first"; mode-change wiring "none beyond the USB link"; `HiZ>` added before the `v` and `#` captures | The two bare captures were the only ones without a prompt; the other sessions already began `HiZ>`. `m1` returns to HiZ (`Firmware/core_system/proc_menu.c` `changemode()`) |
| 10 | fixed | values: "A space after `r` is not a display letter either: `r ` is a plain read in the current `o` format" | `Firmware/core_system/proc_menu.c` `READ_DISPLAY_BASE = {'x','d','b','w'}`; `change_read_display()` only consumes those; a following space is ignored by the dispatcher's whitespace case. Firmware never drops a consumed `x` |
| 11 | fixed | usage: colour-code table (Bk/Wt/Gy/Pu/Bu/Gn/Yw/Or/Rd/Br) and v3 note "`0` is the firmware's label for the tenth column (`MISO`), not a typo and not a separate pin" | `Firmware/core_messages/messages_v4.s` BPMSG1256 and `messages_v3.s` BPMSG1233 print the literal codes; v3 header strings BPMSG1227 plus `MSG_SPI_PINS_STATE` fix the ten-column order; codes are the standard probe-cable colours |
| 12 | fixed | commands HiZ list adds "`c` / `C` (v4 `k` / `K`) choose the target, and `&` / `%` run their delays" | `Firmware/core_system/proc_menu.c` cases `c`, `C`, `k`, `K`, `&`, `%` have no bus-mode gate; `a`/`A`/`@` act on the selected target |
| 13 | fixed | macros Related docs: replaced "usage.md — a mode session with macros" with "edge-cases.md — macro traps and recovery" | `docs/generic/usage.md` contains no macro session or macro example; the only recall example is in macros.md. The old link was a dead end |
| 14 | fixed | usage: "`o1` re-selects HEX, the default (no change); `v` is independent of the display format" | `Firmware/core_system/proc_menu.c` `set_display_mode()` changes only `display_mode`; `print_pins_information()` uses `bp_write_voltage()` and `print_pin_state()`, not `display_mode` |
| 15 | fixed | edge-cases row and commands `e` section: "`(HiZ)` here is the open-drain high state, not the HiZ mode" | `Firmware/core_system/proc_menu.c` `case 'P'` emits `BPMSG1209` when `mode_configuration.high_impedance == NO`; the `(HiZ)` text is literal. It names the high-impedance high state, not the current mode |
| 16 | fixed | README/navigation/usage/edge-cases: `W` "runs the mode's bus setup", "does not repeat the setup prompts"; entry remains "setup prompts" / "setup answers" | `Firmware/core_system/proc_menu.c` `switch_psu_on()` calls `setup_execute()`; `bp_change_mode()` runs `setup_prepare()` (prompts) then `setup_execute()`. `W` never runs `setup_prepare()` |
| 17 | fixed | navigation: "`BRG` is the baud-rate generator divider, not a rate: usable values are 1–32767, 34 is the firmware's default (the 115200 bps preset) and 127 is 31250 bps (MIDI). The `(34)` is that default, not the rate currently in force"; link to the UART BRG section. usage `brg` row defined | `Firmware/core_system/proc_menu.c` `set_baud_rate()` v3 path: `getnumber(34, 0, 32767, 0)`, then `user_serial_set_baud_rate(brg)`; `Firmware/core_system/base.c` `user_serial_initialise()` leaves `U1BRG` when `terminal_speed == 9`; BRG table values 34, 416, 127 |
| 17-side | code-owner | No user-doc change beyond "usable values are 1–32767"; docs do not recommend 0 | `Firmware/core_system/proc_menu.c` `set_baud_rate()` accepts BRG 0 at the prompt because `getnumber(34, 0, 32767, 0)` has a minimum of 0. `Firmware/core_system/base.c` `user_serial_set_baud_rate()` writes it straight to `U1BRG`. Divider 0 is not a usable terminal rate; `wait_baud_confirmation()` should restore the previous rate, but the validator should reject 0. Low severity |
| 18 | not a defect | No change | `1W01` is the firmware's own binary 1-Wire identifier: `Firmware/core_messages/messages_v4.s` `MSG_1WIRE_MODE_IDENTIFIER` prints `1W01`; `docs/protocol/binary/sub-protocols.md` names it for command `0x04`, and `docs/protocol/wire/1wire/binary.md` defines it. It is a device string, not a typo or a mode name |

## Counts

- fixed: 17 (findings 1–17)
- rejected / not a defect: 1 (finding 18)
- code-owner candidates: 1 (17-side, raw BRG accepts 0)

## Remaining questions

- The same SUMP entry precision issue remains outside this set:
  `docs/tools/basic/entry-and-prompt.md` says "five or more NUL bytes",
  `docs/core/system.md` says "at least 5", and
  `docs/protocol/sump/capture.md` says "five or more"/"≥5".  The upper bound is
  nineteen because twenty NULs enter binary first.  Not edited here.
- `docs/tools/basic/entry-and-prompt.md` says "A board reset also leaves it";
  at the `(BASIC)>` prompt only `EXIT` is accepted, while a hardware reset
  obviously restarts the board.  Worth one sentence in the BASIC set.
- The 2-Wire entry page shows `R2W (spd hiz)=( 1 1  )` with two spaces before
  the closing parenthesis; the firmware emits one.  Minor, 2-Wire set.
- Structural suggestions in the feedback (shorten the README, move the idle
  section to navigation, add a "start here" path) were not applied: they were
  structural opinion rather than numbered findings, and the fixes above were
  kept to the smallest change that makes each finding correct.
- The v3 hardware claims (loader 4.5, v3 table, v3 macro/history counts) remain
  as already caveated "not verified"; no new evidence was available.
- The `Pu` / `Yw` colour expansion is the standard probe-cable naming; no
  canonical repository source for the colour names was found.
