# DIO docs — round 1 fixes

Scope: `docs/protocol/dio/**` only. No firmware, `docs/generic/`, `docs/dev/`,
`docs/SPEC.md` or WORKLOG edits. No commit.

**Result:** 18 findings: 17 fixed in-set, 0 rejected, 1 code-owner candidate
(finding 1) and 1 pure cross-set finding (18). Finding 7 also needs one
cross-set consistency sentence in `docs/generic/navigation.md`, and finding 18
needs one in `docs/generic/values.md`.

## Per-finding record

| # | Decision | Change (file + short quote) | Evidence |
|---:|---|---|---|
| 1 | fixed + code-owner | `README.md` five-minute check now splits the two cases: “after `#` every I/O pin must be `I`; after `w` only the five signal pins are released”. `park-and-reset.md` defines parked as “every pin it drives is released” and its verification says “After `#` … every I/O pin … including `AUX1` / `AUX2`” but “After `w` … `AUX1` / `AUX2` are unchanged”. `wiring.md`, `entry-and-setup.md` and `edge-cases.md` use the same five-pin definition. | `Firmware/protocol_dio/dio.c:dio_cleanup()` sets `IODIR \|= AUX \| MOSI \| CLK \| MISO \| CS`, disables the pull-ups and the v4 VPU source, clears those five latches and sets 8 bits; it never touches `AUX1`/`AUX2`. `Firmware/core_system/proc_menu.c:switch_psu_off()` calls `cleanup()` then disables the regulator. `Firmware/core_system/base.c:bp_reset_board_state()` sets `BP_AUX1_DIR`/`BP_AUX2_DIR` input and clears their latches; `#` and mode changes call it. |
| 2 | fixed | `pin-control.md` worked session is now `0xC4` then `0x1B`: “Only `CLK` is driven; the other four stay inputs.” `read-back.md` external read is `0x1F`: “no pin drives the target.” `park-and-reset.md` and `macros.md` use the same `0xC4`/`0x1B` pair. The only all-outputs mention is the direction reference with “Only use `0x00` when all five nets are safe to drive.” | `bitbang_pin_direction_set()` uses bits 4–0 only; `0x1B` has bit 2 clear and bits 4, 3, 1, 0 set, so only `CLK` is an output. `0x1F` sets all five direction bits to input. `0xC4` is state write + supply + `CLK` latch high. |
| 3 | fixed | `pin-control.md`: “`0x40` or `0x20` is a direction write, not "supply on" or "pull-ups on" … Both make all five pins outputs”. `edge-cases.md` has the matching row with `0xC0` / `0xE0` / `0xA0` recovery. | `Firmware/protocol_dio/dio.c:dio_write()` selects on bit 7; `Firmware/protocol_binary/binary_io.c:bitbang_pin_direction_set()` inspects only bits 4–0 and never calls `bp_set_voltage_regulator_state()` / `bp_set_pullup_state()`. `0x40` and `0x20` have bit 7 clear and the low five bits zero. |
| 4 | fixed | `aux-pin.md`: “For `AUX` and `CS`, `r` and `v` both show the result; for `AUX1` and `AUX2`, only `v` does, because `r` reads the five signal pins.” The interaction table says “A numeric value cannot reach `AUX1` or `AUX2`.” “the other four pins” became “the other pins”. | `Firmware/protocol_dio/dio.c:dio_read()` returns `bitbang_pin_state_get()`, which reads `BP_AUX0`, `BP_MOSI`, `BP_CLK`, `BP_MISO`, `BP_CS` on bits 4–0 (`Firmware/protocol_binary/binary_io.c`). `bp_aux_pin_read()` has cases for `AUX0`, `CS`, `AUX1`, `AUX2`; there is no numeric path for the last two. |
| 5 | fixed | Removed `AUX0` from `pin-control.md` and `aux-pin.md`; both now say `AUX` (`A0`). `aux-pin.md` help-token column says “`A0` on v4, `aux` on v3”; `README.md` says “`A0` / `CS` / `A1` / `A2` on v4, and `aux` / `CS` on v3” and links to `generic/commands.md#aux-targets-and-pin-names`. | `Firmware/core_messages/messages_v4.s` help line: `c/C/k/K  AUX assignment (A0/CS/A1/A2)`; `messages_v3.s`: ` c/C  AUX assignment (aux/CS)`. `Firmware/hardwarev4.h` defines `AUX` as `AUX0` internally; the user-visible pin name is `AUX`. |
| 6 | fixed | `aux-pin.md` new §Which target is selected?: “Run `i`.  The status block prints `a/A/@ controls <pin> pin` … `v` shows the pin states, not the selection.” The README five-minute AUX bullet links to it. | `Firmware/core_system/proc_menu.c:print_status_info()` prints the `alternate_aux` message (`BPMSG1086`, `BPMSG1087`, `BPMSG1263`, `BPMSG1264`) after the status table. `print_pins_information()` / `v` prints pin states only. |
| 7 | fixed (+ cross-set) | `entry-and-setup.md`: “`W` … runs the mode's setup hook, which is empty … it does not change directions, levels, latches or pull-ups.” `park-and-reset.md`: “It runs the mode's setup hook, which is empty in direct I/O.” `edge-cases.md` says the hook is empty. | `Firmware/core_system/core.c` DIO descriptor: `.setup_execute = silent_null_operation_callback`; that callback only clears `mode_configuration.command_error`. `Firmware/core_system/proc_menu.c:switch_psu_on()` calls `setup_execute()` after the rail check. |
| 8 | fixed | `README.md` wiring bullet: “`GND`, the signal pins you use, and at most one supply rail … For a self-powered target, leave both rails unconnected.” | `Firmware/core_system/proc_menu.c:switch_psu_on()` enables the single regulator that feeds both rails; there is no per-rail selection. The requirement is stated in full in `wiring.md` §Ground and target supply. |
| 9 | fixed | `park-and-reset.md` §`w` versus `#` is now three-way (`w` \| mode change \| `#`) with rows for supply, five signal pins, `AUX1` / `AUX2`, pull-ups, VPU source, latches, width, bit order, AUX target, display, prompt and macros. Prose: “`w` leaves three settings alone: the bit order, the AUX target and the display format … A mode change also resets the bit order and the AUX target … but keeps the display format and the macros.” The v3 macro note is prose. | `dio_cleanup()` resets width, latches and pull-ups and does not touch `little_endian`, `alternate_aux` or `display_mode`. `changemode()` calls `bp_reset_board_state()` / `clear_mode_configuration()` (MSB, `alternate_aux = 0`) but not `reset_terminal_state()`; `#` calls `reset_terminal_state()`, which sets `display_mode = HEX`. `user_macros[][]` is not cleared by `reset_terminal_state()`. |
| 10 | fixed | `edge-cases.md`: “the pins stay inputs and the latches stay low (the pin levels float)”. | `dio_cleanup()` writes `IODIR` (inputs) and `IOLAT` (latches low); `bitbang_pin_state_get()` reads pin levels. |
| 11 | fixed | `read-back.md` and `edge-cases.md` now use the two-line reply: “`ERROR: command has no effect here`, then … `Syntax error at char 1`”; the edge case notes that `(n)` adds a blank line. | `{` routes to `start_with_read` = `null_operation_callback`, which prints `MSG_COMMAND_HAS_NO_EFFECT` and sets `command_error`; the dispatcher then prints `Syntax error at char <n>` (`BPMSG1110`) for the same line. `(n)` routes to `null_macro_callback` and `bpBR`. |
| 12 | fixed | `read-back.md` adds the tab caveat and the real row order: “The device emits tabs … `Pinstates:` is the title. The first row … is the probe-colour legend; it is omitted above. After the legend come the column labels, the directions and the levels.” The capture now has all 12 columns. | `print_pins_information()` prints `Pinstates:`, `MSG1256` legend, `MSG1257` labels, `print_pins_state()` (`MSG_SPI_PINS_STATE`) and `MSG1228` (`P\tP\tP\tI\tI\t`); the voltage and level rows use `\t` too. The old level row had 11 entries. |
| 13 | fixed | `README.md` availability row: “pins a value can address \| `AUX` (4), `MOSI` (3), `CLK` (2), `MISO` (1), `CS` (0)”. | `bitbang_pin_direction_set()` and `bitbang_pin_state_set()` test bit 4 `AUX`, 3 `MOSI`, 2 `CLK`, 1 `MISO`, 0 `CS`. `print_pins_information()` v4 order is `AUX2 AUX1 AUX CS MISO CLK MOSI`. |
| 14 | fixed | Rewrapped `README.md` intro, `entry-and-setup.md` `W` paragraph and `park-and-reset.md` `AUX1` / `AUX2` paragraph so no line ends in a single word. | Text-only change; no command or state change. |
| 15 | fixed | `edge-cases.md` now has §Symptom index and four-column trigger / symptom / behaviour / recovery tables. The five deep cases keep their headings: LSB-first, width, `w` leaves `AUX1` / `AUX2`, wrappers, contention. | Structural change only; every original entry is retained (including the `0xE0` pull-up side effect, the `;4` hidden action bit and the `w` `AUX1` / `AUX2` trap). |
| 16 | fixed | Removed the repeated “The v3 column is not verified” footer from `wiring.md`, `pin-control.md`, `aux-pin.md`, `park-and-reset.md` and `macros.md`; kept it in `README.md` and `entry-and-setup.md`; `read-back.md` now says only that the v3 table is unverified. | `docs/TEMPLATE.md` §10 and §11: keep the caveat at the availability / entry point, not as a per-page footer. The v3 claims themselves are unchanged. |
| 17 | fixed | `README.md` entry bullet: “On a custom build, run `i` and look for `DIO` in the bracketed mode list, or use the number the `m` menu prints; the numbers above are the default image.” | `print_version_info()` prints `[` + every `bp_protocol_by_index(i)->name` + `]`; `changemode()` lists the compiled protocols and uses that index. `core.c` lists `DIO` after `PIC`, which gives 10 on the default v4 build and 9 on the default v3 build. |
| 18 | cross-set | No DIO edit: `park-and-reset.md` already says “restore the width to 8 bits” and `edge-cases.md` says “`w` … resets the width”. Required in `docs/generic/values.md`. | `dio_cleanup()` sets `mode_configuration.numbits = 8`; `switch_psu_off()` calls `cleanup()`. `docs/generic/values.md` currently says the width stays “until you change it, or until a mode change or reset restores 8 bits.” |

## Code-owner candidate

**`dio_cleanup()` leaves `AUX1` / `AUX2` driven on v4.** The cleanup comment
says “Release the header before removing bias or clearing output latches”, but
the mask is `AUX | MOSI | CLK | MISO | CS`
(`Firmware/protocol_dio/dio.c:dio_cleanup()`). `AUX1` (RD8) and `AUX2` (RD0)
are distinct pins in `Firmware/hardwarev4.h`, and `bp_aux_pin_set_high()` /
`bp_aux_pin_set_low()` (`Firmware/capability_aux/aux_pin.c`) drive them when
`mode_configuration.alternate_aux` is 2 or 3. `w` reaches the cleanup through
`switch_psu_off()` (`Firmware/core_system/proc_menu.c`), so a pin driven with
`k` / `K` then `A` stays driven after the supply goes off. The docs now state
this and tell the reader to use `#` or a mode change. A code owner may want
`dio_cleanup()` to release and unlatch `AUX1` / `AUX2` under
`#ifdef BUSPIRATEV4`; if that changes, update `README.md`,
`park-and-reset.md`, `wiring.md` and `edge-cases.md` together.

## Cross-set changes for the main session

1. `docs/generic/values.md` §Value entry, lines 96–98 — add the mode cleanup
   to the width rule: “The width is **not** per line: it stays in force for
   later values and reads until you change it, until a mode change or reset
   restores 8 bits, or until the mode's cleanup runs (`w` in DIO does this).”
   Evidence: `dio_cleanup()` sets `mode_configuration.numbits = 8`;
   `switch_psu_off()` calls `cleanup()`. This is finding 18.
2. `docs/generic/navigation.md` §Mode entry, lines 213–215 — replace “`W`
   starts the supplies and runs the mode's bus setup (the engage routine)” with
   “`W` starts the supplies and runs the mode's setup hook; the hook is empty
   in DIO, so `W` there does not change pins.” Evidence: the DIO descriptor
   sets `.setup_execute = silent_null_operation_callback`
   (`Firmware/core_system/core.c`), and that callback only clears
   `command_error`. This is the cross-set half of finding 7.

Optional consistency (not required by a finding):
`docs/generic/edge-cases.md` §Power, pull-ups and VPU, the “`w` in DIO” row,
could say that `w` releases only the five signal pins, so a driven
`AUX1` / `AUX2` survives it. The row already defers to the DIO page for pin
meaning, so it is not wrong as written.

The phrase “runs the mode's bus setup” also appears in
`docs/generic/README.md:57`, `docs/generic/usage.md:165` and
`docs/generic/edge-cases.md:86`. Those are general statements; only the
`navigation.md` sentence is required by finding 7.

## Structural notes and remaining questions

- Adopted the README task-path table (wire/drive one pin, use `AUX1`/`AUX2`,
  read back, park, recover) and the per-board pin cross-reference table in
  `wiring.md` (header position, value bit, both `v` columns, AUX token).
- Did not merge `entry-and-setup.md` and `park-and-reset.md`. Entry keeps the
  availability/entry material and the supply table; park owns the full
  `w` / `#` / mode-change contract. The duplication is now one consistent
  sentence in each place, cross-linked, with the three-way table in one page.
  A future lifecycle merge is a structural preference, not a correctness gap.
- The v3 columns remain unverified on v3 hardware; only `README.md`,
  `entry-and-setup.md` and the specific `read-back.md` note carry that caveat.
- If the code owner changes `w` to release `AUX1` / `AUX2`, the `w` row in the
  three-way table, the `w` verification bullets, the README quick-start check
  and the `w` `AUX1` / `AUX2` edge case all need the new behaviour.
