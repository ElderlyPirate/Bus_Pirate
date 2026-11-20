# SUMP — Phase F re-audit (RA-8 v3 overrun recovery)

**3 of 3 filled in full.** All three Phase F findings are resolved; the set is
clean for this delta. No regressions.

Verified against the final tree in this worktree:
`Firmware/protocol_sump/sump.c` `enter_sump_mode()` still has the v3-only
check at the top of the loop, before `user_serial_ready_to_read()` —
`user_serial_check_overflow()` → `user_serial_clear_overflow()` →
`command_processor_state = RX_COMMAND_IDLE` → `sump_reset()`. That matches the
docs: first-pass clear, partial command/config discard, complete-command retry,
no power-cycle requirement.

## Per-item verdicts

| # | Status | Evidence (current docs) | Remainder |
|---|---|---|---|
| 1 | **Filled in full** | `readout.md` §Aborts and bytes during a reply: "A long burst can overrun the v3 UART; bytes beyond the receiver are lost.  The v3 loop checks and clears the overrun before it polls for the next command, including on the first pass, and discards any partial command and the capture configuration.  Resend the complete command; no power cycle is needed." The old "ignored until the board is power-cycled" sentence is gone. | None. The remaining `power cycle` wording is the negation ("no power cycle is needed") and the optional five-reset/one-second parser realignment for other sync faults. |
| 2 | **Filled in full** | `edge-cases.md` §Readout and transfer: "one byte waits; on v3 a long burst can overrun the UART and lose bytes.  The loop clears the overrun before polling the next command (including the first pass), discards the partial command and capture configuration, and reads the next complete command; resend it.  No power cycle is needed". "possibly stopping reception" and "power-cycle if the board stays silent" are gone. | None. |
| 3 | **Filled in full** | `commands.md` §Parser rules, new last row: "| v3 UART overrun | the loop checks and clears the overrun before polling for a command, including on the first pass; a partial command and the capture configuration are discarded, and the next complete command is read normally.  The host should resend the command; no power cycle is needed |". | None. The row is the authority both other pages link to. |

## Presentation / link check

- No TAB characters in `readout.md`, `edge-cases.md` or `commands.md`.
- Prose wraps at ~80 columns; the only long lines are table rows, consistent
  with the rest of each page. The new rows keep the table shapes.
- `commands.md#parser-rules` resolves: the heading is `## Parser rules`, and
  both `readout.md` and `edge-cases.md` link to it.
- No dates, bench prose, pass counts, or positive "tested/verified" claims. The
  existing "not verified on v3 hardware" caveats are the only status language.

## Cross-set / checked

- `protocol/uart/errors.md` §Overruns ("the link is still alive ... nothing
  needs to be reset") and `protocol/uart/bridge.md` ("the link does not stop")
  remain consistent with the new SUMP wording; no edits needed.
- `generic/`, `protocol/binary/edge-cases.md` and `docs/dev/code-map.md` were
  checked in the delta audit; no copy of the stale SUMP claim.
- `entry.md` "the board is unresponsive to everything | power-cycle it" and
  `edge-cases.md` "power-cycle if the link itself is lost" remain generic
  last-resort/lost-link fallbacks, not overrun claims; allowed and unchanged.
