# XSVF docs — round 1 re-audit

**Scope:** the 17 numbered findings in `feedback-xsvf-round1.md`, re-read
against the current `docs/tools/xsvf/` pages, plus the presentation rules and
the three cross-set edits.  This is not a fresh full review.

**Status key:** *Filled in full* = the documentation issue is closed and the
fix reads naturally.  *Partial* = some of the original issue remains.
*Not addressed* = unchanged.  *Regressed* = the fix introduced a new defect.
"Full" is about the documentation item; the five code-owner candidates are
tracked separately and are not counted as doc remainders.

## Per-item results

| # | Status | Current evidence (quote) | Remainder |
|---|---|---|---|
| 1 | **Filled in full** | `players.md:159`: "**Do not combine `-x` with `-f` in one invocation.** ... Run `-x` in its own invocation, press reset ... then run the file with `-r -f`."  `workflow.md:75`: "**Do not add `-x` to this command.** ... a short read leaves bytes that the run reads as its result."  The Windows worked command is now `-r` only (`players.md:271`). | CO-1 (host scan sizing/drain) remains a code candidate; the docs now warn and route the safe sequence. |
| 2 | **Filled in full** | `README.md:44-46`: "**Press reset** to get the terminal back; the rails go off.  To re-power them, enter a mode (`m`) and then `W`; `W` also runs that mode's setup and is refused in `HiZ>`." | — |
| 3 | **Filled in full** | `workflow.md:178-187`: table "reset button / always", "`0x0F` twice / command loop only", plus "The reset button is the everyday exit.  The `0x0F` path is for a custom host or a script: the shipped players close the port without sending it..." and the pySerial recipe at `workflow.md:191`.  `player.md:70-72`: "The two steps need a host that can write raw bytes; the shipped players close the port and cannot send them." | CO-5 (`XCOMMENT` reset-only) remains a code candidate; the docs already keep the reset-only behaviour. |
| 4 | **Filled in full** | `players.md:225-232`: "`0` is a clean result, **or** `End of file reached.` ... treat it as a failure even though the status is `0`"; "`-1` is a setup failure ... A Unix shell reports it as `255`"; `players.md:238-239`: "The success check is `End of operation reply: XSVF_ERROR_NONE` followed by `Success!`; do not trust `0` alone." | CO-3 (cross-platform status) remains a code candidate; the docs now match the shipped mapping.  New minor: the `-1` bullet does not mention the Windows `-x`-only/`-r`-only case (see N3). |
| 5 | **Filled in full** | `workflow.md:19-23`: "An XSVF written directly by iMPACT has no listing: keep the SVF and convert it with `-a` for the check, inspect the XSVF with a parser, or treat `XSVF_ERROR_DATAOVERFLOW` (`0x06`) as the check after a run..."  `converter.md:110-114` repeats it. | — |
| 6 | **Filled in full** | `wiring.md:29-34`: "Entry does not enable the on-board pull-ups; binary entry switches them off. ... `0x18` does not clear them ... A board reset or the return to the terminal switches them off."  The cross-set page now aligns: `protocol/jtag/edge-cases.md:15`: "the header pins reset, VREG switches off and the on-board pull-ups switch off; AUX1/AUX2 keep their state." | XS-1 applied; no contradiction left. |
| 7 | **Filled in full** | `README.md:48-55`: "The host players and the converters are in the firmware repository under `scripts/BP4_XSVFplayer_Win/` and `scripts/BPXSVFPlayer/` ... The repository does not install them: run the executable from its folder, or build it with the included project or `Makefile`." | — |
| 8 | **Filled in full** | `converter.md:68-70`: "`-r 0` disables retries.  The parenthetical in the option table means a non-zero retry count is meaningful only for XC9500/XL/XV; the other family recipes pass `-r 0` to turn retries off." | — |
| 9 | **Filled in full** | `players.md:203`: "`Sending N Bytes (total)...` ... the number in parentheses is the running total after the chunk, hex"; `players.md:296-298`: "the last full chunk totals `D000` (53248).  The final 41 bytes bring the total to `D029` (53289), the whole file." | — |
| 10 | **Filled in full** | `players.md:24-26`: "Stops when the file is used up, or at a result on the cross-platform build; the Windows build sends one more chunk after an early error and then ends with `No reply.... Quitting.`"  `workflow.md:164-167` repeats the per-build difference. | CO-2 (Windows extra chunk) remains a code candidate; the docs now state it.  New minor: `players.md:204` still says the result is read "after the file" (see N2). |
| 11 | **Filled in full** | `players.md:47`: "no `ok` after the two entry lines; run `ok` suffixes still appear." | — |
| 12 | **Filled in full** | `players.md:150`: "Chain Scan Result: 04 93 40 60 59"; `players.md:153-155`: "`93 40 60 59` is the example IDCODE `0x59604093`."  The `FC` rows are split at `players.md:182-183` and `limits-and-errors.md:92-93`: all-zero bytes = TDO stuck low; plausible IDCODEs = real chain over 63. | — |
| 13 | **Filled in full** | `players.md:296-298`: "The middle `Sending` lines are omitted"; `players.md:307` and `workflow.md:145` use `$` for the Linux command; `players.md:59-61` splits the converter cell into Windows/Unix/ReadMe rows; no prose line exceeds 80 columns. | — (two new presentation nits: `workflow.md:146` bare `...`; 123-column pySerial line — see N4, N5). |
| 14 | **Filled in full** | `README.md:16-17`: "The host programs' banner also reads `BusPirate XSVF Player`; that is the PC tool.  The firmware mode answers only `XSV1`." | — |
| 15 | **Filled in full** | `limits-and-errors.md:124`: "`Error opening file` \| fix the `-f` path; the cross-platform build exits, the Windows build can terminate after the message."  `players.md:234-236` repeats the Windows warning. | CO-4 (null-handle dereference) remains a code candidate; the docs warn. |
| 16 | **Filled in full** | `workflow.md:225-228`: "To check the regulator, press reset, enter a mode (`m`), then `W` ... If it prints `VREG too low, is there a short?`, check the rail for a short."  `wiring.md:79-83` gives "reset, then `c`, `A`" and "reset, then `k`, `A`"; the cross-set `protocol/jtag/wiring.md:110-113` now says the same. | XS-3 applied; exact terminal steps present. |
| 17 | **Filled in full** | `README.md:65-67`: "**v3:** there is no XSVF player.  Use OpenOCD's own `svf` command through the `0x06` path ...; it plays SVF, not XSVF."  `README.md:114` labels the link "the v3 and larger-device alternative". | — |

**17 of 17 items filled in full.**

## Cross-set verification

The three recorded cross-set changes are visible and consistent with the XSVF
pages:

- `protocol/jtag/edge-cases.md:15` now says the header pins, VREG and the
  on-board pull-ups switch off on binary entry; AUX1/AUX2 keep their state.
  This matches `tools/xsvf/wiring.md:29-34` (finding 6).
- `protocol/jtag/xsvf.md:155-158` now says the stock host tools close the port
  and a raw-serial host sends `0x0F` twice; this matches the XSVF exit table
  and the pySerial recipe (finding 3).
- `protocol/jtag/wiring.md:110-113` now gives `c`+`A` for AUX and `k`+`A` for
  AUX1, matching `tools/xsvf/wiring.md:79-83` (finding 16).

Minor residue, not one of the 17: `protocol/jtag/wiring.md:73` still says
"send `0x0F` twice" without the "raw-serial host" qualifier.  It is a
protocol-level power note rather than a human instruction, and the XSVF pages
qualify it; I would leave it unless the protocol set is being swept for the
same wording.

## Presentation checks

| Rule | Result |
|---|---|
| No TAB characters | **Pass** — 0 tab-containing lines in all seven files. |
| ~80-column prose wraps | **Pass** — 0 non-table, non-code prose lines over 80.  The only long text is table rows and the new pySerial one-liner (N5). |
| Captures aligned | **Pass with one nit** — output lines are space-aligned; the failure transcript's elision marker at `workflow.md:146` is a bare `...` at column 0 (N4). |
| Menus / option lists as tables | **Pass** — options, results, limits and exit statuses are tables; the What-ships converter cell was split (finding 13). |
| No bench prose / dates / tested-verified-result prose | **Pass** — no dates, pass counts, "captured on" provenance, evidence sections or "tested" claims.  The only verification language is the allowed "code-derived and **unverified** on hardware" caveat. |
| No internal log names | **Pass** — the only "log" string is the shipped `run_log.bat` batch file (players.md:74), not an internal log. |

## New problems (max 5)

1. **[minor] `players.md:165-166`** — "the Windows build prints the extra reply
   and then sends **the chunks**."  The shipped Windows host sends **one more
   chunk**, as `players.md:25`, `:204` and `:242` correctly say.  Fix: "sends
   one more chunk."
2. **[minor] `players.md:204`** — "the result byte read **after the file**; an
   early error stops the cross-platform build..."  An early error is read
   before the file is fully sent.  Fix: "the result byte read after the run"
   or "after the stream."
3. **[minor] `players.md:231-232`** — the `-1` bullet says "a run with neither
   `-f` nor `-x`/`-r`", but the new scan-only workflow also makes the Windows
   build exit `-1` for `-x` alone (and `-r` alone).  Fix: add "or a
   scan/reset-only run on Windows" to the `-1` examples.
4. **[minor] `workflow.md:146`** — the failure transcript starts with a bare
   `...` at column 0; the output lines have one leading space, and the lead-in
   is not explained.  Fix: use " ..." and say "banner lines omitted", or move
   the note outside the fence.
5. **[minor] `workflow.md:191`** — the new pySerial one-liner is 123 columns;
   it wraps in a normal terminal and is harder to read than the rest of the
   set.  Fix: break it into a short multi-line snippet or a small `.py` block.

No major new problems; the process cap does not need to extend for this round.

## Verdict

The blocker is closed in the docs: `-x` and `-f` are explicitly separated,
the Windows worked command is `-r` only, and the probe-only run is a separate
invocation.  The five code-owner
candidates remain open as code work, but the XSVF pages now state the shipped
behaviour accurately.  Only the five minor nits above remain.
