# OpenOCD adapter — round 1 re-audit

**Scope:** the 16 numbered findings in `feedback-openocd-round1.md`, checked
against the current `docs/tools/openocd/` files.  Cross-set pages were checked
only where a finding pointed at them.

**Verdict: 15 of 16 items filled in full.**  The one open item is the new
host-build check output in finding 5: the command and build/Windows routes are
present, but the sample `adapter list` output is not the real OpenOCD format,
and the 0.10 `interface_list` caveat is not on the two pages that print the
command.  No Major findings; no extra loop required.

## Per-item table

| # | Status | Evidence (current docs) | Remainder |
|---:|---|---|---|
| 1 | **Filled in full** | `README.md:107-109`: "`0x00` leaves `OCD1`, makes the BBIO pins inputs and switches the regulator and the on-board pull-ups off.  `0x0F` adds the return to the terminal ... it is not what removes the power."  `entry-and-exit.md:49-50`: "**off; the binary entry and `0x06` both switch it off**."  `limits-and-errors.md:118`: "**VREG and the on-board pull-ups switch off**." | None. |
| 2 | **Filled in full** | `README.md:45-48`: "...or, for a 3.3 V target only, set `buspirate mode normal`.  A 1.8 V or 2.5 V target must stay open-drain with pull-ups to its own rail." | None. |
| 3 | **Filled in full** | `README.md:53-56`: "One `0x05` request carries at most **8192 bits**, including every TAP move in that request.  OpenOCD batches scans into one request and does not split a longer scan; keep the whole request inside the limit."  `limits-and-errors.md:58-66`: "two 1024-byte lane buffers, one for TDI and one for TMS ... Keep the whole `0x05` request, including its TAP moves, at or below 8192 bits with margin." | None. |
| 4 | **Filled in full** | `commands.md:232-240`: "**v3 requires separate writes.** ... Do not put the selector and the confirmation in one write."  `host-setup.md:213`: "on v3, send the `0x07` selector and the `0xAA 0x55` confirmation in separate writes."  `exchanges.md:136-139` shows the port switch between the two writes. | None. |
| 5 | **Partial** | `host-setup.md:61-66` adds the check: "`openocd -c "adapter list" -c "shutdown"`" with output "The following debug adapters are available: ... N: buspirate".  `host-setup.md:72-74` adds the build steps.  `host-setup.md:44-54` adds Cygwin, WSL and Linux VM routes. | The check block's sample output is not the real `adapter list` format.  OpenOCD prints a Tcl dictionary (`buspirate { jtag swd }`), not a numbered "N: buspirate" list; "The following debug adapters are available:" is the `adapter driver` error text, not `adapter list`.  Also `README.md:41` and `workflow.md:73` print `adapter list` unconditionally, while only `host-setup.md:68` notes that 0.10 uses `interface_list`. |
| 6 | **Filled in full** | `workflow.md:121`: "`V33` is the board's `3.3V` rail; 0 with VREG off and no board rail connection."  `workflow.md:124-126`: "V33 and V50 measure the board's rails, not the target's own supply ... do not connect a self-powered target's supply to a rail just to make it read.  That is back-driving." | None. |
| 7 | **Filled in full** | `exchanges.md:36-40` and all other comment-bearing lines: every `host ->` / `host <-` comment starts at column 31; continuation lines use the same indent.  Mechanically checked. | None. |
| 8 | **Filled in full** | `commands.md:177-179`: "After the last bit TCK is left high and TDI/TMS hold their last values.  In a final 9–16-bit group the unused high bits are ignored; still send all four payload bytes.  A final 1–8-bit group takes two payload bytes." | None. |
| 9 | **Filled in full** | `commands.md:204-206`: "The TDO reply is sized for the clamped count, so expect fewer TDO bytes than the requested count."  `limits-and-errors.md:49-50` repeats it; `exchanges.md:189-195` shows the short TDO reply. | None. |
| 10 | **Filled in full** | `entry-and-exit.md:55`: "The five BBIO pins are `AUX`, `CS`, `MISO`, `CLK` and `MOSI`."  `entry-and-exit.md:88` names them in the exit row.  `commands.md:74` separately names "the five JTAG output latches (TDI, TMS, TCK, SRST, TRST)." | None. |
| 11 | **Filled in full** | `protocol/jtag/entry.md:75-77`: "`0x06` entry resets the pin mode, VREG, pull-ups, SRST (`AUX0`) level and TCK delay, but not the mode LED, the link rate or TRST (`AUX1` on v4, `PGD` on v3)." | None.  The cross-set edit is already present. |
| 12 | **Filled in full** | `protocol/jtag/tap.md:201`: "OpenOCD feature selector `0x04` (sent as `0x02 0x04 action`)"; `:203`: "`0x00` then `0x0F` on the OpenOCD path, `0x0F` twice on the XSVF path." | None.  The cross-set edit is already present. |
| 13 | **Filled in full** | `README.md:4-5`: "compiled into both builds; its v3 behaviour is not verified"; `README.md:15-17`: "the XSVF set calls the same thing the **firmware player**"; `commands.md:121-122`: "It is the same on-board regulator that the terminal `W` command switches (`POWER SUPPLIES ON`)"; `protocol/jtag/openocd.md:32`: "leave the OpenOCD path." | None.  The cross-set wording is already present. |
| 14 | **Filled in full** | `README.md:102-104`: "No project configuration is shipped.  OpenOCD's installed `interface/buspirate.cfg` is the starting template."  `host-setup.md:153`: "OpenOCD ships interface/buspirate.cfg." | None. |
| 15 | **Filled in full** | `limits-and-errors.md:154`: "Driver messages and startup symptoms"; `:158`: "Message or symptom"; `:177`: "Target and chain messages and symptoms."  The unquoted startup condition is now under the "Message or symptom" column. | None. |
| 16 | **Filled in full** | `limits-and-errors.md:27-28`: "The v3 path is compiled into the build; its hardware behaviour is not verified on v3."  No `## Unverified` heading remains. | None. |

## Focus checks

- **Actual power state:** consistent.  README, overview, entry-and-exit,
  limits and workflow all say the 20-NUL entry and `0x06` switch VREG and the
  on-board pull-ups off, `0x00` leaves them off, and `0x0F` only adds the
  terminal return.  No "not changed by entry" / "keep their state" sentence
  remains.
- **Single 8192-bit bound:** consistent.  Every page now says one `0x05`
  request, including its TAP moves, is capped at 8192 bits; limits explains
  the two 1024-byte lane buffers and that an 8192-bit scan with preceding TAP
  moves can still overflow.  No page still says "one scan at or below 8192"
  as the whole rule.
- **Separate-write handshake:** consistent.  `commands.md`, `host-setup.md`,
  `limits-and-errors.md` and `exchanges.md` all say the selector and the
  `0xAA 0x55` confirmation are separate writes, with the port switch between
  them.
- **Host setup:** mostly accurate.  The build check, `--enable-buspirate`
  build steps and Windows routes are present and placed before the option
  matrix.  The one inaccuracy is the sample `adapter list` output and the
  unqualified 0.12+ command in README/workflow (finding 5).

## Cross-set edits

All the recorded edits are already present in the worktree:

- `docs/protocol/jtag/entry.md:75-77` — TRST removed from the 0x06 reset list.
- `docs/protocol/jtag/tap.md:201` — "feature selector `0x04` (sent as
  `0x02 0x04 action`)".
- `docs/protocol/jtag/tap.md:203` — OpenOCD exit now "`0x00` then `0x0F`",
  XSVF exit "`0x0F` twice".
- `docs/protocol/jtag/openocd.md:32` — "leave the OpenOCD path".

No remainder from the OpenOCD side.

## Code-owner candidate

The v3 `0x05` zero-count guard remains a code item; the docs now say it
correctly:

- `commands.md:196-198`: "**v3:** a zero count is not supported; the v3 path
  has no zero-count guard.  Use at least one bit; the v3 outcome is
  unverified."
- `limits-and-errors.md:72-73`: "a zero count is not supported.  The v3 path
  has no zero-count guard; use at least one bit.  The v3 outcome is
  unverified."
- `limits-and-errors.md:139`: "no defined reply; the shift does not complete
  normally | the v3 path has no zero-count guard | send at least one bit."

No doc change is needed; the guard belongs to the code owner.

## Presentation check

- **TAB characters:** 0 across the nine OpenOCD files.
- **Capture alignment:** every comment-bearing `host ->` / `host <-` line in
  `exchanges.md` starts at column 31; no jagged block remains.
- **Menus / option lists:** all option and route lists are tables; no dense
  bullet-chain menu.
- **Prose wrap:** no non-table prose line outside code exceeds 84 characters.
- **Bench/dates/tested claims:** no dates, pass counts, bench/rig/battery
  narratives, "captured on" provenance, internal log names or positive
  "tested"/"verified" claims.  The only verification language is the allowed
  "not verified / unverified" caveat for v3, Cygwin and WSL.  The dedicated
  `## Unverified` section is gone.

## New problems

1. **[minor] `host-setup.md:61-66` shows a fake `adapter list` output.**
   The real output is a Tcl dictionary (`buspirate { jtag swd }`), not
   "The following debug adapters are available:" with "N: buspirate".  The
   command is right, so the check still finds `buspirate`; the expected-output
   block will confuse a reader who compares it.  `README.md:41` and
   `workflow.md:73` also print the 0.12+ command without the 0.10
   `interface_list` caveat that `host-setup.md:68` carries.  Fix: show the
   dictionary line and add "(0.10: `interface_list`)" next to the command in
   README/workflow.
