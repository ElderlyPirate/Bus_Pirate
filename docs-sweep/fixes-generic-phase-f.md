# Generic prompt layer — Phase F implementation record

**Scope:** `docs/generic/` only.  Docs head `1f0752bb`; firmware checked
against the final tree in this worktree (`Firmware/`, combined `9b179cb5`).
No firmware edits, no cross-set edits, no commits or pushes.

## In-set findings

| # | Decision | File + quote | Evidence |
|---|---|---|---|
| 1 | fixed | `docs/generic/usage.md` reset section: "The board-state reset also stops Timer2, so any AUX PWM/servo output ends.  On v4 it releases `AUX1`/`AUX2`, clears their RPOUT routing and the `AUX1` ODC bit, so every header I/O pin is an input afterwards." | `Firmware/core_system/base.c` `bp_reset_board_state()`: `BP_AUX1_ODC = OFF` only (there is no AUX2 ODC clear), `BP_AUX1_RPOUT = NULL_IO` and `BP_AUX2_RPOUT = NULL_IO`, then `T2CONbits.TON = OFF` before `bp_aux_pin_reset_state()`.  Timer2 drives the AUX PWM/servo generator (`Firmware/capability_aux/aux_pin.c`), so stopping it ends that output.  The old sentence only said "clears their output routing" and could be read as clearing both ODC bits |
| 2 | fixed | `docs/generic/navigation.md` terminal-baud section: "A valid `b10 <value>` (1–32767) skips the prompt; a missing, zero, malformed, or >32767 value reopens `Enter raw value for BRG` at the same `(34)>` prompt." | `Firmware/core_system/proc_menu.c` `set_baud_rate()` v3 path: `brg = getint()`; when `mode_configuration.command_error` is set, or `(uint16_t)brg == 0`, or `(uint16_t)brg > 32767u`, it prints `MSG_RAW_BRG_VALUE_INPUT` and calls `getnumber(34, 1, 32767, 0)`.  `getint()` sets `command_error` for a missing value or overflow.  The old "A complete `b10 <value>` skips the prompt" was false for those inputs |

## Cross-set list (not edited here)

### C1 — `docs/core/system.md` reset order/summary — now updated; verify only

The worktree already contains the core implementer's fix.  The ordered list
under **Board-state reset** has step 2 clearing `AUX1`'s open-drain bit and
step 5 stopping Timer2, and the mode-change summary (around lines 436–440)
now reads:

> "It releases the bus and AUX pins, stops SMPS and Timer2, clears the v4
> `AUX1` ODC bit, parks the onboard EEPROM, clears the mode configuration,
> disables the pull-ups and regulator, and reconfigures the ADC"

No further summary edit is needed.  Keep the claim to `AUX1` only; do not add
an `AUX2` ODC clear.  The core file is modified in this worktree but is not
committed by this set; confirm the change is included in the core commit.

Evidence: `Firmware/core_system/base.c` `bp_reset_board_state()` clears
`BP_AUX1_ODC` and stops `T2CONbits.TON`;
`Firmware/tests/test_core_reset.py` asserts both the Timer2 stop and the AUX1
ODC mask.

### C2 — `docs/protocol/uart/` still quotes the old 16 M auto-baud message

Stale text remains in:

- `docs/protocol/uart/edge-cases.md`: "`** Baud>16m: The BP cannot measure
  above 16000000, Done.`" and "the printed ceiling is the message text; the
  check is at 150 kbaud".
- `docs/protocol/uart/entry-and-setup.md`: the same message and "The printed
  16,000,000 is the message text, not the check."
- `docs/protocol/uart/macros.md`: the same message and the same
  16,000,000-is-message-text explanation.
- `docs/protocol/uart/errors.md`: the same message in the error table.

The final firmware prints `** Baud>150k: The BP cannot measure above 150000,
Done.` in both builds (`Firmware/core_messages/messages_v3.s`,
`messages_v4.s`); the check in `Firmware/protocol_uart/uart.c` is
`detected > 150000`.  Replace every old string and delete the
16,000,000-is-not-the-check explanation; the printed `150000` is the check.

### C3 — `docs/protocol/uart/entry-and-setup.md` omits the v3 BRG re-prompt

The v3 custom-baud paragraph says only "the value is the divider itself" and
gives the rate relationship.  It does not say that invalid input reopens the
prompt.  Add: "A missing, zero, malformed, or >32767 value reopens the prompt
at `(34)>`; the accepted range is 1–32767."

Evidence: `Firmware/protocol_uart/uart.c` v3 custom-baud path uses
`getnumber(34, 1, 32767, 0)`; the v4 paragraph already states the equivalent
re-ask behaviour.

## Counts

- fixed: 2 (findings 1–2)
- rejected: 0

## Verification

- Reset claims checked against `bp_reset_board_state()` in the final tree:
  Timer2 stop on both builds, `AUX1` ODC clear only, both RPOUT clears.
- BRG claims checked against `set_baud_rate()` and `getint()` in the final
  tree.
- No TAB characters; no prose line exceeds 80 columns; all generic links and
  anchors resolve.
- No dates, bench/pass-count prose, or positive tested/verified claims added.
- No commit or push.

## Re-audit presentation fixes

| # | Decision | File + quote | Evidence |
|---|---|---|---|
| P1 | fixed | `docs/generic/usage.md` reset sentence now reads "clears their output routing and the `AUX1` open-drain bit" | User pages must not carry code identifiers; `RPOUT` and `ODC` were replaced with the user-facing terms without changing the meaning.  Only AUX1's open-drain bit is cleared |
| P2 | fixed | `docs/generic/navigation.md` now reads "a missing, zero, malformed, or over 32767 value reopens ..." | A line starting with `>` renders as a Markdown blockquote; "over 32767" keeps the re-prompt statement without escaping.  No other leading `>` lines or code identifiers remain in the changed generic passages |
