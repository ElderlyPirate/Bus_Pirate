# SUMP — Phase F delta audit (RA-8 v3 overrun recovery)

**Verdict: No — fix required.** Two pages still describe the pre-RA-8
behaviour ("commands can be ignored until the board is power-cycled") and the
new auto-clear / partial-config-discard / retry-complete-command rule is not
documented anywhere in the set.

Verified against the final tree in this worktree
(`Firmware/protocol_sump/sump.c`, `enter_sump_mode()`): on v3 the loop checks
and clears the UART overrun at the **top of the loop, before
`user_serial_ready_to_read()`** — therefore including the first pass. It also
sets `command_processor_state = RX_COMMAND_IDLE` and calls `sump_reset()`, so a
partial command and the capture configuration are discarded. There is no
power-cycle requirement for an overrun.

## Findings

### 1. [major] `docs/protocol/sump/readout.md` — stale wedge/power-cycle claim

> A long burst can overrun the v3 UART: bytes beyond the receiver are lost, and
> later commands can be ignored until the board is power-cycled.  After the
> readout, send five `0x00` resets or wait one second before the next command to
> realign the parser ([commands.md](commands.md#parser-rules)).  If the board is
> still silent, power-cycle it.

**Why:** this is exactly the RA-8 symptom, and it is now false. The v3 loop
clears the overrun before the next command poll (first pass included), drops the
partial command and capture configuration, and then accepts the next complete
command. The paragraph tells a v3 user to power-cycle a board that recovers by
itself, and it omits the resend rule.

**Fix:** replace the paragraph with:

> A single byte sent during the readout is handled after the last sample.  A
> long burst can overrun the v3 UART; bytes beyond the receiver are lost.  The
> v3 loop checks and clears the overrun before it polls for the next command,
> including on the first pass, and discards any partial command and the capture
> configuration.  Resend the complete command; no power cycle is needed.  The
> five `0x00` reset run or the one-second gap still realigns a parser that is out
> of step for another reason ([commands.md](commands.md#parser-rules)).

### 2. [major] `docs/protocol/sump/edge-cases.md` — same stale fallback in the table

> | a byte sent during the readout | it is handled after the last sample | one
> byte waits; on v3 a long burst can overrun the UART, losing bytes and possibly
> stopping reception.  Realign with five `0x00` resets or a one-second gap;
> power-cycle if the board stays silent ([commands.md](commands.md#parser-rules)) |

**Why:** "possibly stopping reception" and "power-cycle if the board stays
silent" describe the old overrun wedge. With RA-8 the overrun is cleared by the
loop; a silent board is not the expected result, and the recovery is to resend
the complete command, not to cycle power.

**Fix:**

> | a byte sent during the readout | it is handled after the last sample | one
> byte waits; on v3 a long burst can overrun the UART and lose bytes.  The loop
> clears the overrun before polling the next command (including the first pass),
> discards the partial command and capture configuration, and reads the next
> complete command; resend it.  No power cycle is needed
> ([commands.md](commands.md#parser-rules)) |

### 3. [minor] `docs/protocol/sump/commands.md` — parser rules omit the overrun row

> The one-second gap is what heals a lost sync.  Real clients send the five-byte
> commands back to back and never hit it; a script that dies mid-command does,
> and the next connection can recover without a power cycle.

**Why:** the parser-rules table covers a stalled long command and an unknown
command, but not the v3 UART overrun. The positive half of RA-8 (overrun is
cleared before polling, partial command/config discarded, host retries a
complete command) is not stated anywhere, and both pages above link here for
the parser rules.

**Fix:** add a table row:

> | v3 UART overrun | the loop checks and clears the overrun before polling for
> a command, including on the first pass; a partial command and the capture
> configuration are discarded, and the next complete command is read normally.
> The host should resend the command; no power cycle is needed |

## Cross-set list

- `docs/protocol/uart/errors.md` — §Overruns already states the correct rule:
  "After an overrun, the link is still alive.  Read out what is queued and
  continue; nothing needs to be reset." This is consistent with RA-8 and is the
  authority the SUMP pages should align with. **No edit needed.**
- `docs/protocol/uart/bridge.md` — "the link does not stop" (overruns during the
  bridge). Consistent; no edit.
- `docs/generic/` — no SUMP overrun wording; the entry/exit pages cover the
  NUL/STX boundary only. No stale copy; no edit.
- `docs/protocol/binary/edge-cases.md` — SUMP entry boundary only; no overrun
  claim. No edit.
- `docs/dev/code-map.md` — lists `uart2_rx_overrun_count()`; maintainer page, no
  change required for this item.
- No cross-set page repeats the stale "power-cycled until it recovers" claim;
  the two copies are both in the SUMP set (findings 1 and 2).

## Presentation check (changed passages only)

- No TAB characters in `readout.md`, `edge-cases.md` or `commands.md`.
- Prose wraps at ~80 columns; the long lines are table rows, which is allowed.
  The three passages above are ordinary paragraphs/table cells.
- No dates, bench prose, pass counts, or "tested/verified" result prose. The
  only status language is the existing "not verified on v3 hardware" caveat.
- The table rows in `edge-cases.md` are single-line cells, consistent with the
  rest of that page.

## Checked, not findings

- `entry.md` "the board is unresponsive to everything | power-cycle it" and
  `edge-cases.md` "power-cycle if the link itself is lost" are generic
  last-resort / lost-link fallbacks, not overrun claims. They can stay, but the
  implementer may qualify the first as "after trying the exit bytes" if a strict
  no-blanket-power-cycle reading is wanted.
- `readout.md` line 112 ("during the readout | ... a long burst can overrun the
  v3 UART") is a factual precondition, not the recovery; it becomes correct once
  the paragraph below it is fixed.
- The v3 capacity/4096-byte figures in `README.md` and `depth.md` match the
  final tree's buffer sizing and are outside this delta.
