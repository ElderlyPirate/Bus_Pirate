# Core — Phase F implementation record

**Scope:** `docs/core/` only.  Worktree `bpv4-docssweep`, branch
`docs/combined-sweep`, docs head `1f0752bb`; firmware verified against the
worktree's own `Firmware/` (combined `9b179cb5`).  No firmware changes, no
commit or push, no WORKLOG edits.

## Decisions

| # | Decision | File + quote | Evidence |
|---|---|---|---|
| 1 | fixed | `docs/core/system.md`: "2. v4: release AUX1/AUX2; on SMPS builds run `smps_stop()`; detach the AUX1/AUX2 peripheral outputs, and clear `AUX1`'s open-drain bit (`BP_AUX1_ODC = OFF`), then clear their latches." and new "5. Stop Timer2 (`T2CONbits.TON = OFF`); leave its pairing and an independent Timer3 unchanged."  The former steps 5–8 are now 6–9. | `Firmware/core_system/base.c` `bp_reset_board_state()` sets `BP_AUX1_ODC = OFF` in the v4 AUX1/AUX2 block, then `T2CONbits.TON = OFF` after `eeprom_initialize()` and before `bp_aux_pin_reset_state()` |
| 2 | fixed | `docs/core/system.md` menu table: "entry 10 asks for the raw `U1BRG` (1–32767; blank uses 34; zero, malformed or overflow re-prompts at `(34)>`)." | `Firmware/core_system/proc_menu.c` `set_baud_rate()` rejects `command_error`, `(uint16_t)brg == 0`, or `(uint16_t)brg > 32767`, prints `MSG_RAW_BRG_VALUE_INPUT`, then calls `getnumber(34, 1, 32767, 0)` |

## Cross-set carry-in — recorded, not edited

### C1 — UART auto-baud message

The final catalogs carry `MSG_UART_BAUD_OVERFLOW` as
"`** Baud>150k: The BP cannot measure above 150000, Done.`" in both
`messages_v3.s` and `messages_v4.s`.  Core has no quote of this text;
`messages.md` defers the wording to the user sets, so no core change is needed.
These owning pages still quote the old 16m message and/or the stale
"printed ceiling" caveat:

- `docs/protocol/uart/entry-and-setup.md` — old message in the measurement
  table and "The printed 16,000,000 is the message text, not the check."
- `docs/protocol/uart/errors.md` — old message in the auto-baud table.
- `docs/protocol/uart/macros.md` — old message plus "The printed 16,000,000 is
  the message text; the check is at 150 kbaud."
- `docs/protocol/uart/edge-cases.md` — old message and "the printed ceiling is
  the message text; the check is at 150 kbaud."

The printed number now matches the 150000 check; the three caveats are wrong
and should be removed.  The entry-time table note (a reading up to the 256000
table end still maps to 128000 or 256000) is table-mapping behaviour and
should stay separate from the message limit.

### C2 — SMPS open-drain recovery

`bp_reset_board_state()` clears `BP_AUX1_ODC`, and the v4 `#` path, the v4
`0x0F` return and a mode change all reach it.  These SMPS pages still say a
terminal reset does not clear the bit:

- `docs/capabilities/smps/edge-cases.md` — "A v4 terminal reset does not clear
  it either; a power cycle or hardware reset does."
- `docs/capabilities/smps/wiring.md` — the same claim for `#` and `0x0F`.
- `docs/capabilities/smps/README.md` — "the Bus Pirate needs a power cycle or
  hardware reset before the start."
- `docs/capabilities/smps/exit.md` — the path table says the terminal reset
  "Releases `AUX1`" but omits the ODC clear.

The correct statement is that the v4 terminal reset (`#`, `0x0F`) or a mode
change clears `AUX1`'s open-drain bit; a power cycle or hardware reset also
clears it.  Keep the pull-up warning for binary paths that reach SMPS without
a terminal reset, and add the ODC clear to the reset rows in `exit.md`.

### C3 — Binary and OpenOCD reset summaries

These v4 terminal-reset summaries do not mention the `AUX1` ODC clear; the
timer/OC5 wording is already present where it applies:

- `docs/protocol/binary/commands.md` — `0x0F` reset paragraph.
- `docs/protocol/binary/edge-cases.md` — "PWM after the layer is left".
- `docs/protocol/binary/entering-and-framing.md` — v4 `0x0F` summary.
- `docs/tools/openocd/entry-and-exit.md` — "After step 2" terminal reset
  summary.

Each needs "clears the V4 `AUX1` open-drain bit" added to its v4 terminal-reset
description.  No other binary behaviour changes.

### Follow-up — core reset summary

The mode-change lifecycle summary also omitted the two #209 facts.  It now
says the reset "stops SMPS and Timer2, clears the v4 `AUX1` ODC bit" before the
EEPROM and mode-configuration work.  No AUX2 ODC clear is claimed.

### Follow-up — `BP_ADC_3V3` rail

Decision: fixed in `core` (cross-set carry-in from the ADC/selftest audits).
`system.md` now qualifies `BP_ADC_3V3` as the switched 3.3 V output and states
that the `switch_psu_on()` check covers the switched target branch, not AVdd.

## Counts

- fixed: 2
- rejected / not a defect: 0
- cross-set items recorded for the main session: 3 (C1–C3)

## Files changed

- `docs/core/system.md`

## Notes

- The ordered reset list remains a numbered procedure; the new Timer2 step is
  inserted after the v4 EEPROM step and before the AUX detach, matching
  `bp_reset_board_state()`.
- The v3 raw-BRG row states the bounded prompt: `getnumber(34, 1, 32767, 0)`
  supplies the `(34)>` default and rejects zero, malformed and overflow input.
- No other core passage states the reset order or the raw BRG boundary, so no
  further in-set change was needed.
