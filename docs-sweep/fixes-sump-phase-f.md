# SUMP — Phase F repair record (RA-8 v3 overrun recovery)

**Scope:** `docs/protocol/sump/` only.  Branch `docs/combined-sweep`, docs
head `641339d3`; verified against the worktree's final combined
`Firmware/` (`9b179cb5`).  No commit, no push.

**Input:** `docs-sweep/feedback-sump-phase-f.md` — 2 major + 1 minor, all
about the RA-8 v3 UART-overrun recovery.  All three are fixed;
none rejected.

**Result:** 3 / 3 fixed.  Files changed: `readout.md`, `edge-cases.md`,
`commands.md`.

## Evidence used (all findings)

Verified in this worktree, not in an external PR worktree:

- `Firmware/protocol_sump/sump.c` — `enter_sump_mode()`: on v3 the loop
  checks `user_serial_check_overflow()` at the top of the loop, **before**
  `user_serial_ready_to_read()`, so the check includes the first pass
  (lines 614–624).
- The same block calls `user_serial_clear_overflow()` (clearing OERR also
  discards the RX FIFO), sets `command_processor_state = RX_COMMAND_IDLE`
  (drops a partial long command or parameter run), and calls `sump_reset()`
  (drops the capture configuration).
- `sump_reset()` restores the default rate/read/delay/mask/storage state
  (lines 653–706), so an overrun leaves a clean analyzer that accepts the
  next complete command; there is no wedge and no power-cycle requirement.

## 1 — major — `readout.md`: stale wedge / power-cycle paragraph — fixed

**Before (quoted from the audit):**

> A long burst can overrun the v3 UART: bytes beyond the receiver are lost,
> and later commands can be ignored until the board is power-cycled.  After
> the readout, send five `0x00` resets or wait one second before the next
> command to realign the parser…  If the board is still silent, power-cycle
> it.

**After (`docs/protocol/sump/readout.md`, §Aborts and bytes during a reply):**

> A single byte sent during the readout is handled after the last sample.  A
> long burst can overrun the v3 UART; bytes beyond the receiver are lost.  The
> v3 loop checks and clears the overrun before it polls for the next command,
> including on the first pass, and discards any partial command and the capture
> configuration.  Resend the complete command; no power cycle is needed.  The
> five `0x00` reset run or the one-second gap still realigns a parser that is
> out of step for another reason ([commands.md](commands.md#parser-rules)).

**Why:** the old paragraph described the pre-RA-8 symptom and told a v3 user
to cycle power on a board that now recovers by itself.  The replacement keeps
the factual precondition (a long burst can overrun the v3 UART and lose
bytes), states the auto-clear including the first pass, states the partial
command/config discard, and states the host action (resend the complete
command).  The five-reset / one-second realignment is retained only for a
parser that is out of step for another reason.

**Evidence:** `sump.c` `enter_sump_mode()` 614–624 (auto-clear before the
command poll, first pass included; parser state and analyzer reset) and
`sump_reset()` 653–706.

## 2 — major — `edge-cases.md`: stale table fallback — fixed

**Before (quoted from the audit):**

> | a byte sent during the readout | it is handled after the last sample | one
> byte waits; on v3 a long burst can overrun the UART, losing bytes and possibly
> stopping reception.  Realign with five `0x00` resets or a one-second gap;
> power-cycle if the board stays silent… |

**After (`docs/protocol/sump/edge-cases.md`, §Readout and transfer):**

> | a byte sent during the readout | it is handled after the last sample | one
> byte waits; on v3 a long burst can overrun the UART and lose bytes.  The loop
> clears the overrun before polling the next command (including the first
> pass), discards the partial command and capture configuration, and reads the
> next complete command; resend it.  No power cycle is needed
> ([commands.md](commands.md#parser-rules)) |

**Why:** "possibly stopping reception" and "power-cycle if the board stays
silent" were the old overrun wedge.  With RA-8 the loop clears the overrun,
drops the partial command/config, and reads the next complete command; the
recovery is to resend it.  A silent board is not the expected result of an
overrun.  The table-cell shape of the page is preserved.

**Evidence:** same `sump.c` block (614–624) and `sump_reset()` (653–706);
`docs/protocol/uart/errors.md` §Overruns already states the same rule (link
stays valid, no edit needed there).

## 3 — minor — `commands.md`: parser rules lacked the overrun row — fixed

**Before (quoted from the audit):**

> The one-second gap is what heals a lost sync.  Real clients send the
> five-byte commands back to back and never hit it; a script that dies
> mid-command does, and the next connection can recover without a power cycle.

The parser-rules table covered a stalled long command and an unknown command,
but not the v3 UART overrun.

**After (`docs/protocol/sump/commands.md`, §Parser rules — new last row):**

> | v3 UART overrun | the loop checks and clears the overrun before polling
> for a command, including on the first pass; a partial command and the
> capture configuration are discarded, and the next complete command is read
> normally.  The host should resend the command; no power cycle is needed |

**Why:** both of the pages above link here for the parser rules, so the
positive half of RA-8 must be stated here too.  The existing one-second-gap
paragraph was left in place: it still correctly describes lost-sync recovery
and already says a power cycle is not needed.  The new row is the authority
the two pages point at.

**Evidence:** `sump.c` `enter_sump_mode()` 614–624 (`OERR` check/clear at the
top of the loop; `RX_COMMAND_IDLE`; `sump_reset()`).

## Cross-set / checked, not findings

- `docs/protocol/uart/errors.md` §Overruns already says the link stays alive
  after an overrun; consistent with RA-8, no edit.
- `docs/protocol/uart/bridge.md` ("the link does not stop") and
  `docs/protocol/binary/edge-cases.md` (SUMP entry boundary only) are
  consistent; no edit.
- `docs/generic/` has no SUMP overrun wording; no edit.
- `docs/dev/code-map.md` is a maintainer page and needs no change for this
  item.
- `entry.md` "the board is unresponsive to everything | power-cycle it" and
  `edge-cases.md` "power-cycle if the link itself is lost" remain generic
  last-resort/lost-link fallbacks, not overrun claims; the audit explicitly
  allows them.  Left unchanged so the repair stays inside the three findings.
- `readout.md` line 112 ("during the readout | … a long burst can overrun the
  v3 UART") is the factual precondition and is correct now that the recovery
  paragraph is fixed.

## Presentation / link check

- No TAB characters in the three changed files.
- No dates, bench prose, pass counts, or "tested/verified" result prose; the
  existing "not verified on v3" caveat is unchanged.
- Prose wraps at ~80 columns; the long lines are table rows, consistent with
  the rest of the set.
- `commands.md#parser-rules` exists and both new links resolve.
- Only the three named files changed; no firmware or other worktree edits.
