# Core — Phase F delta audit

**Docs head:** `6129a812`.  **Verified against:** the docs worktree's own
`Firmware/` (combined `9b179cb5`): `core_system/base.c` has `BP_AUX1_ODC = OFF`
and `T2CONbits.TON = OFF` in `bp_reset_board_state()`; `core_messages` has
`MSG_UART_BAUD_OVERFLOW` = `** Baud>150k: The BP cannot measure above 150000, Done.`;
`proc_menu.c` bounds the v3 raw BRG to 1–32767.

I read the core/generic row and cross-set carry-in in `PHASE-F-DOCS.md`,
`WORKLOG-DOCUMENTATION.md` §2F, the auditor prompt, and the six core pages.
This is a delta audit of the three repaired behaviours and the reset-path
claims, not a full writer review.

**Verdict: Yes with fixes.**  The core reset reference still omits both C7
steps, and the v3 raw-BRG boundary is not carried into the core menu table.
The auto-baud message has no core quote; the stale quotes are in the UART set,
and the new AUX1-ODC clear contradicts three SMPS pages.

## Findings

1. **[major] `docs/core/system.md:536-547` — the ordered reset list omits the
   Timer2 stop and the V4 AUX1 ODC clear.**
   Quote: "2. v4: release AUX1/AUX2; on SMPS builds run `smps_stop()`; detach
   the AUX1/AUX2 peripheral outputs and clear their latches."
   and "5. Detach the source from the currently selected AUX pin."
   The final tree's `bp_reset_board_state()` also executes `BP_AUX1_ODC = OFF;`
   in the same AUX1/AUX2 block, and `T2CONbits.TON = OFF;` after the EEPROM
   initialization and before `bp_aux_pin_reset_state()`.  The list says
   "Order:", so a maintainer using it to reorder or reason about reset will
   not know either state change happens.  This is the binary audit's C7 item.
   **Fix:** in step 2 add ", and clear `AUX1`'s open-drain bit
   (`BP_AUX1_ODC = OFF`)" before "clear their latches"; insert a new step
   after 4: "Stop Timer2 (`T2CONbits.TON = OFF`); leave its pairing and an
   independent Timer3 unchanged."  Renumber the current 5–8 to 6–9.

2. **[minor] `docs/core/system.md:509` — the v3 raw BRG is still described as
   blank/default only.**
   Quote: "entry 10 asks for the raw `U1BRG` (blank default 34)".
   The final tree accepts 1–32767; zero, malformed input, and values above
   32767 go through `MSG_RAW_BRG_VALUE_INPUT` and `getnumber(34,1,32767,0)`,
   i.e. the bounded `(34)>` prompt.  The core menu table is the implementation
   reference and already states the blank default, so leaving out the new
   boundary makes the table incomplete after RA-1.
   **Fix:** "entry 10 asks for the raw `U1BRG` (1–32767; blank uses 34; zero,
   malformed or overflow re-prompts at `(34)>`)."

## Cross-set carry-in

### C1 — [major] UART set still quotes the old auto-baud message

The core set has no quote of this text; `messages.md` correctly defers the
wording to the user sets.  The owning UART pages are stale:

- `docs/protocol/uart/entry-and-setup.md:252`:
  "`** Baud>16m: The BP cannot measure above 16000000, Done.`"
- `docs/protocol/uart/entry-and-setup.md:262-263`:
  "The printed 16,000,000 is the message text, not the check."
- `docs/protocol/uart/errors.md:111`: the same old message in the auto-baud
  table.
- `docs/protocol/uart/macros.md:94-96`: the old message plus "The printed
  16,000,000 is the message text; the check is at 150 kbaud."
- `docs/protocol/uart/edge-cases.md:80`: the old message and "the printed
  ceiling is the message text; the check is at 150 kbaud" in the one-line
  table.

**Why:** the final `MSG_UART_BAUD_OVERFLOW` is
"`** Baud>150k: The BP cannot measure above 150000, Done.`".  The printed
number now matches the 150000 check, so the "16,000,000"/"printed ceiling"
caveats are not just stale, they are wrong.
**Fix:** replace the old message in all four pages and remove the three
stale "message text"/"printed ceiling" caveats in `entry-and-setup.md`,
`macros.md` and `edge-cases.md`.  Keep the entry-time table note (a reading up
to the 256000 table end still maps to 128000 or 256000) as table-mapping
behaviour, not as the message's measurement limit.

### C2 — [major] SMPS set says a terminal reset does not clear AUX1 ODC

- `docs/capabilities/smps/edge-cases.md:138-139`:
  "A v4 terminal reset does not clear it either; a power cycle or hardware
  reset does."
- `docs/capabilities/smps/wiring.md:74-75`: the same claim for `#` and `0x0F`.
- `docs/capabilities/smps/README.md:46-48`: says the board "needs a power cycle
  or hardware reset before the start" after an open-drain `AUX1` use.
- `docs/capabilities/smps/exit.md:108-115`: the path table says the terminal
  reset "releases `AUX1`" but does not include the ODC clear.

**Why:** the final `bp_reset_board_state()` sets `BP_AUX1_ODC = OFF`, and `#`,
the v4 `0x0F` return and a mode change all reach it.  The three statements
above are now false or needlessly restrictive; the exit table is incomplete.
**Fix:** say the v4 terminal reset (`#`, `0x0F`) or a mode change clears
`AUX1`'s open-drain bit; a power cycle or hardware reset also clears it.  Keep
the pull-up warning for binary paths that reach SMPS without a terminal reset.
Add the ODC clear to the reset rows in `exit.md`.

### C3 — [minor] Binary/OpenOCD reset summaries omit the new AUX1 ODC clear

- `docs/protocol/binary/commands.md:211-219`: the v4 terminal-reset summary
  lists pins to inputs, pull-ups, supplies, SMPS, AUX source, Timer2 and OC5,
  but not the `AUX1` ODC clear.
- `docs/protocol/binary/edge-cases.md:118-123` and
  `docs/protocol/binary/entering-and-framing.md:190-194`: same omission.
- `docs/tools/openocd/entry-and-exit.md:97-100`: same summary.

**Why:** these are summaries, not false statements, but the reset state list
is now incomplete and an `AUX1` open-drain user cannot see that the reset
recovers the pin.
**Fix:** add "clears the V4 `AUX1` open-drain bit" to the v4 terminal-reset
description in each.  No other binary behaviour changes.

## Presentation (changed passages only)

- No TAB characters and no trailing whitespace in the six core files.
- The ordered reset list is within ~80 columns and remains a numbered
  procedure; the v3 baud row is a table row (203 characters), not prose.
- No dates, bench/rig narratives, pass counts, provenance or internal log
  names in the changed core passages.  The only "verified" occurrences are
  the allowed "not verified on v3 hardware" caveats.
- All core table pipe counts are consistent; no misaligned capture was
  introduced.  The `\|` in the dispatcher table is an intentional escaped
  pipe, not a broken column.
