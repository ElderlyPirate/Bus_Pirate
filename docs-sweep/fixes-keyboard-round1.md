# KEYB docs — round 1 fixes

Scope: `docs/protocol/keyboard/**` only.  No firmware, generic-set,
`docs/SPEC.md`, `docs/dev/` or WORKLOG edits.  No commit.

**Result:** 22 findings: 21 fixed in-set, 1 rejected as not a defect
(finding 12), 0 code-owner candidates, 0 required cross-set changes.  Two
cross-set consistency follow-ups are recorded below; neither is needed for
the KEYB pages to be correct.  Extra turn: the re-auditor's items 3, 5 and the
keypad-5 note are fixed (3 of 3); the original counts are unchanged.

## Per-finding record

| # | Decision | Change (file + short quote) | Evidence |
|---:|---|---|---|
| 1 | fixed | `edge-cases.md` check order now reads “DATA (it must idle high through a pull-up; CLOCK is driven low by the mode between exchanges)”; the recovery says “check its high phase during a frame, not at the idle prompt”.  `wiring.md` pin summary: “DATA high through a pull-up; CLOCK driven low between exchanges”. | `Firmware/protocol_keyboard/pc_at_keyboard.c`, `pc_at_keyboard_execute()` sets `KBCLK_TRIS = OUTPUT; KBCLK = LOW`; `read_byte()` always ends `KBCLK_TRIS = OUTPUT`.  `Firmware/core_system/proc_menu.c`, `print_pin_direction()` / `print_pin_state()` then show CLK as `O` / `L`. |
| 2 | fixed | `wiring.md` Direction and levels: “The header I/O is **5 V class**: 5.5 V is the maximum on DATA or CLOCK.  The keyboard's open-drain 5 V pull-up is within that.”  A push-pull source is rejected because it contends, not because the pin cannot tolerate 5 V. | `Firmware/hardwarev4.h` maps MOSI/CLK to RD1/RD2; `hardwarev3.h` maps them to RB9/RB8.  `Firmware/configuration.h` selects PIC24FJ256GB106 (v4) / PIC24FJ64GA002 (v3); both device families rate 5 V-tolerant digital pins at 5.5 V maximum.  The sibling DIO, SPI and JTAG pages state the same 5 V class / 5.5 V limit. |
| 3 | fixed | `wiring.md` Supply: “**Never connect an external supply to the `5.0V` pin.**”  Adds the three pull-up paths (external resistors, external Vpullup + `e1` + `P`, or `W`/`e3`/`P` feeding only Vpullup), states that `e3` routes only the on-board rail, and recommends `W` for the idle inhibit.  README step 3 and usage step 2 now qualify the “needs `W`” statement for externally powered keyboards. | `Firmware/core_system/proc_menu.c`, `switch_psu_on()` enables the regulator and calls `setup_execute()`; `set_pullup_voltage()` case 1/2/3 selects external/none, 3.3 V or 5.0 V and refuses when Vpullup already has voltage.  `pc_at_keyboard.c` `write_byte()` / `read_byte()` do not require a prior `W`; the mode's CLOCK-low inhibit is set by `pc_at_keyboard_execute()` only. |
| 4 | fixed | `usage.md` new “First bytes after power-up”: “The first `r` or `(1)` may return `0xAA` (or `0xFC`/`0xFD`); that is not a scancode.”  `scancodes.md` adds `AA` (BAT passed), `FC`/`FD` (BAT failed) and `FA` rows and says they are responses, not scancodes. | `Firmware/protocol_keyboard/pc_at_keyboard.c`, `read_scancode()` accepts any 8-bit value; `read_byte()` / `pc_at_keyboard_run_macro()` do not filter BAT or `0xFA` bytes.  The keyboard, not the firmware, generates `AA`/`FC`/`FD`; the mode prints them as received. |
| 5 | fixed | `edge-cases.md` new “Is a keyboard there?”: write `0xEE`, expect `WRITE: 0xEEACK`, then `r` and expect `READ: 0xEE`; “An `ACK` alone is weak: a DATA line stuck low acknowledges every write.”  `NONE` recovery points at this test. | `Firmware/protocol_keyboard/pc_at_keyboard.c`, `write_byte()` returns ACK solely from the acknowledge-slot DATA level; `pc_at_keyboard_send()` reports it.  Echo is an ordinary host byte, so a returned `0xEE` proves both directions and a live keyboard. |
| 6 | fixed | `protocol.md` new “Two bit orders”: wire is always LSB-first; `l` sends the typed value as-is and prints reads as received; `L` reverses the 8-bit value before/after.  `scancodes.md`, `entry-and-setup.md` and `usage.md` now repeat the distinction and link it.  Entry table says “prompt MSB (`l`); wire is LSB-first”. | `Firmware/protocol_keyboard/pc_at_keyboard.c`, `write_byte()` sends `(value & (1 << bit_index))` for bit 0 first; `read_scancode()` assembles bits by shifting the byte right and ORing `0x80` into it.  `Firmware/core_system/proc_menu.c` reverses only the value: `bp_reverse_integer(sendw, numbits)` before `send`, and after `read` when `little_endian == YES`. |
| 7 | fixed | `usage.md` section is now “Sending several raw bytes (a string)”: “`a` is `0x61`, not the `A` key”; the capture's ACK “only means each frame was accepted, not that a character was typed”; use a string only for raw host bytes. | `Firmware/core_system/proc_menu.c` case `'"'`: echoes each character, takes its byte value and calls `bp_current_protocol()->send(sendw)`.  `pc_at_keyboard_send()` passes that byte to `write_byte()`; there is no character or scancode translation. |
| 8 | fixed | `scancodes.md` adds a code-sorted single-byte index, a prefixed-code index, and a full numeric-keypad table.  The key-name tables stay for the reverse question. | Standard PC-AT Set 2 make codes; the added keypad codes follow the same set (`69`/`72`/`7A`/`6B`/`73`/`74`/`6C`/`75`/`7D`/`70`, `71`, `E0 5A`, `79`, `7B`, `7C`, `E0 4A`).  The firmware has no scancode table; `read_scancode()` returns the byte unchanged. |
| 9 | fixed | `usage.md` adds `KEYB>0xEE` / `WRITE: 0xEENACK` and, in Two-byte commands: “If the command byte reports `NACK` or `TIMEOUT`, do not send the data byte: the keyboard is not waiting for it.  Retry the command.” | `Firmware/protocol_keyboard/pc_at_keyboard.c`, `pc_at_keyboard_send()` switches on `write_byte()` result and prints `MSG_ACK`, `MSG_NACK` or `MSG_KEYBOARD_ERROR_TIMEOUT`; `write_byte()` returns NACK when the acknowledge-slot DATA level is high. |
| 10 | fixed | `usage.md` adds a successful `KEYB>r:2` / `READ: 0x1C 0xF0` capture ahead of the failure capture, and labels the `NONE` capture as two failed reads. | `Firmware/core_system/proc_menu.c`, `case 'r'`: `repeat = getrepeat() + 1u`, one `read()` per `--repeat`; `getrepeat()` returns the typed count (default 1).  `bp_current_protocol()->read()` is `pc_at_keyboard_read()`, one read window per call. |
| 11 | fixed | `entry-and-setup.md` `v` section now shows the full 12-column signal-name row (`GND … MOSI`) and labels it “with the firmware's tabs shown as spaces”; it also states that after `W`, CLK is an output low and MOSI an input. | `Firmware/core_system/proc_menu.c`, `print_pins_information()` prints `BPMSG1257` (`GND\t5.0V\t3.3V\tVPU\tADC\tAUX2\tAUX1\tAUX\t`), then `pc_at_keyboard` `.print_pins_state = hiz_print_pins_state` prints `MSG_SPI_PINS_STATE` (`CS\tMISO\tCLK\tMOSI`). |
| 12 | not a defect | No change.  The current `W`, `w` and refused-`W` captures have no blank line because the firmware prints none. | `Firmware/core_messages/messages_v4.h`: `MSG_POWER_SUPPLIES_ON` and `MSG_POWER_SUPPLIES_OFF` are `bp_message_write_buffer` (no newline); `bpBR` is `bp_write_line("")` and terminates that same line.  `MSG_CLUTCH_ENGAGED` / `MSG_CLUTCH_DISENGAGED` are `bp_message_write_line`.  `switch_psu_on()` therefore prints `POWER SUPPLIES ON` + newline + `Clutch engaged!!!`; it does not print a blank line between them. |
| 13 | fixed | `protocol.md` “What the mode does not do”: “No retry after the command returns: the read window may re-sample after a frame error; issue `r` again for a fresh window”. | `Firmware/protocol_keyboard/pc_at_keyboard.c`, `read_byte()` breaks the polling loop only on `KEYBOARD_SCANCODE_READ_SUCCESS` or `KEYBOARD_SCANCODE_READ_TIMEOUT_ERROR`; startbit, parity and stopbit errors continue the loop and can be re-sampled. |
| 14 | fixed | `usage.md` `0xF3` row gives the byte layout and a following paragraph expands bits 5–6 (`00` 250 ms, `01` 500 ms, `10` 750 ms, `11` 1000 ms; bits 0–4 rate 0–31).  `0xFF` row says “self-test; usually `0xFA`, then `0xAA`”. | The firmware sends the raw command and optional data byte; the layout is the PC-AT keyboard command definition, not firmware state.  `pc_at_keyboard_send()` has no per-command special case. |
| 15 | fixed | `scancodes.md` uses “Set 2” throughout; `usage.md` adds “Two different acknowledgements are in play.  The frame's ACK/NACK slot is reported on the `WRITE:` line.  The keyboard's `0xFA` response is a data byte and is read with `r`.” | `Firmware/core_messages/messages_v4.s` defines `MSG_ACK` / `MSG_NACK` as the frame reply; `0xFA` is not special-cased in `pc_at_keyboard.c`, so it arrives through `read_scancode()` as data. |
| 16 | fixed | `edge-cases.md` replaces “Unverified paths” with a short “What is not established” table and drops the scattered device-specific `unverified` tags; the remaining `unverified` wording is limited to the v3 build and host-write timing, where it changes a decision. | Documentation structure; the consolidated table names the keyboard-dependent pull-ups, current draw, host timing, buffer depth, scancode set/BAT and the optional v3 build.  No code behavior changed. |
| 17 | fixed | `README.md` “First key in five minutes” is a numbered five-step flow (wire → enter → `W` → `e3`/`P` on v4 or external resistors on v3 → monitor/read), with “On a default v3 build `m8` selects PIC” in step 2. | `Firmware/core_system/proc_menu.c`, case `'e'` is inside `#ifdef BUSPIRATEV4`; `Firmware/configuration.h` leaves `BP_ENABLE_PC_AT_KEYBOARD_SUPPORT` undefined for `BUSPIRATEV3` in the default feature block. |
| 18 | fixed | `entry-and-setup.md`: “After the mode number there are no questions to answer … the `(1)>` in the menu capture below is the shared mode menu's own prompt for the mode number.” | `Firmware/core_system/proc_menu.c`, `changemode()` prints the numbered menu and `getnumber()` prompt `(1)>`; `pc_at_keyboard_prepare()` only sets `mode_configuration.high_impedance = ON` and asks nothing. |
| 19 | fixed | `wiring.md`: “CLOCK stays driven low (it was set low by `W`)”. | `Firmware/core_system/proc_menu.c`, `switch_psu_off()` calls `pc_at_keyboard.cleanup` (`reset_mode_to_8_bits`) and disables the regulator; it does not touch `KBCLK_TRIS`. |
| 20 | fixed | `README.md` availability table: v3 build is “optional (not in the default image)”, menu slot “8 when enabled”, monitor/writes “available in a custom build”; the note says the default v3 image omits KEYB (PIC 8, DIO 9). | `Firmware/configuration.h` defines `BP_ENABLE_PC_AT_KEYBOARD_SUPPORT` for v4 and undefines it for v3 in the default block; the `BP_CUSTOM_FEATURE_SET` block defines it for either build.  `Firmware/core_system/core.c` orders KEYB after 3WIRE and before PIC. |
| 21 | fixed | `protocol.md` slot table says “stop — DATA released (pull-up high)” for both directions; `usage.md` NACK row says “released DATA (pulled high) in the acknowledge slot”. | `Firmware/protocol_keyboard/pc_at_keyboard.c` is open drain: `write_bit()` sets `KBDIO_TRIS` (input for high, output for low) and never drives high; `read_scancode()` samples the released level. |
| 22 | fixed | `wiring.md`: “With the keyboard driving the start bit low, a missing pull-up alone does not cause `*startbit error`; a floating DATA line can.” | `Firmware/protocol_keyboard/pc_at_keyboard.c`, `read_scancode()` returns `KEYBOARD_SCANCODE_READ_START_BIT_ERROR` only when `KBDIO != LOW`; a floated high DATA line satisfies that check, while a keyboard-driven low start bit does not. |

## Extra turn — re-audit follow-up

| Re-audit item | Decision | Change (file + short quote) | Evidence |
|---|---|---|---|
| 3 — external-Vpullup order (Major) | fixed | `wiring.md` table: “select `e1`, feed `Vpullup`/`VPU` from the external supply (0–5 V), then `P`”.  The `P`-warning recovery is now “select `e1`, connect the external feed, then `P`”; `edge-cases.md` has the same recovery wording. | `Firmware/core_system/proc_menu.c`, `set_pullup_voltage()` reads Vpullup first and refuses with `MSG_VOLTAGE_VPULLUP_ALREADY_PRESENT` when `bp_read_adc(BP_ADC_VPU) > 0x100`; `e1` must therefore be selected while the pin is unpowered.  `docs/generic/commands.md:19` and `docs/protocol/spi/wiring.md:111` state the same order. |
| 5 — Echo/BAT interaction (Major) | fixed | `edge-cases.md` presence test now captures `READ: 0xAA` then a second `r` with `READ: 0xEE`, and says “The first read after power-up may be a BAT byte … Read again; a BAT byte means the keyboard is alive, but it is not the echo.”  It only fails on `NACK`/`TIMEOUT` or reads that never return `0xEE`. | `Firmware/protocol_keyboard/pc_at_keyboard.c`, `read_byte()` returns the next valid frame with no BAT filtering; `pc_at_keyboard_run_macro()` / the `r` dispatcher print whatever byte arrives.  `usage.md` already states that the first read after `W` can be `0xAA`/`0xFC`/`0xFD`. |
| Keypad-5 note (Minor) | fixed | `scancodes.md`: “With Num Lock off, keypad 1–4 and 6–9 send the navigation codes above; keypad 5 has no navigation assignment, and its Num Lock-off behavior is keyboard-dependent.” | The Set 2 navigation table has no keypad-5 entry; keypad 5's only listed make code is `73`, and its Num Lock-off behavior is not represented by a separate navigation code. |

## Cross-set changes for the main session

No cross-set change is required for the KEYB pages to be correct.  Two
consistency follow-ups were noticed while checking sibling pages:

1. **`docs/capabilities/auxiliary/wiring.md`** says “A 5 V push-pull source is
   not documented as safe to connect directly; the board's input tolerance is
   **unverified**.”  The paragraph is scoped to the AUX pins, but the phrase
   “board's input tolerance” reads board-wide and now conflicts with the 5 V
   class / 5.5 V statement on the KEYB, DIO, SPI and JTAG pages.  Recommended:
   scope the sentence to AUX, or align it with the 5 V class / 5.5 V wording.
2. **`docs/generic/values.md`** describes `l`/`L` as reversing the pattern
   “before it goes to the bus (writes)” and “the byte on the bus is the bit
   reverse”.  That is correct as a value-level statement, but a fixed-order
   protocol such as KEYB still sends LSB-first.  Recommended: add one sentence
   that `l`/`L` reverses the value, not the mode's wire order, and point at
   mode pages that state their order.

## Remaining questions

- The host-write timing (about 60 µs inhibit; DATA changes while CLOCK is high)
  is still only code-derived.  The KEYB pages keep the `unverified` caveat and
  warn that a strict keyboard may `TIMEOUT` or `NACK`; no real keyboard was
  exercised in this pass.
- The read window re-samples after startbit/parity/stopbit errors because
  `read_byte()` only stops on success or timeout.  The docs now describe that
  behavior.  Whether the re-sampling is intentional is a code-owner question,
  not a documentation defect.
- The custom v3 build remains unverified.  The pages state that where it
  changes a build decision, but no v3 hardware or build was run.
- The 5.5 V maximum is taken from the PIC24FJ device family rating and the
  sibling protocol pages; it was not measured on a board.
- `e1` + external Vpullup is documented from `set_pullup_voltage()` and the
  `e` menu text; the external-feed path itself was not bench-tested.

## Verification

- Read all seven `docs/protocol/keyboard/*.md` files and checked each finding
  against `Firmware/protocol_keyboard/pc_at_keyboard.c` / `.h`,
  `Firmware/core_system/proc_menu.c`, `base.c`, `core.c`,
  `Firmware/core_messages/messages_v3.s` / `messages_v4.s` / `.h`,
  `Firmware/configuration.h`, `hardwarev3.h` and `hardwarev4.h`.
- Checked every relative link and heading anchor in the set; no broken targets.
- Checked for TAB bytes (0 in every file), balanced code fences, consistent
  table column counts, and prose lines over 80 columns outside tables and code
  blocks; none.
- Checked the new code index and keypad table against the standard PC-AT Set 2
  codes already used by the set.
- No commit; no WORKLOG edits; no edits outside `docs/protocol/keyboard/` and
  this record.

## Files changed

- `docs/protocol/keyboard/README.md`
- `docs/protocol/keyboard/entry-and-setup.md`
- `docs/protocol/keyboard/wiring.md`
- `docs/protocol/keyboard/protocol.md`
- `docs/protocol/keyboard/usage.md`
- `docs/protocol/keyboard/scancodes.md`
- `docs/protocol/keyboard/edge-cases.md`
- `docs-sweep/fixes-keyboard-round1.md` (this record)
