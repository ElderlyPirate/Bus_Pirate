# UART — round-1 re-audit

**14 of 15 items filled in full.** Finding 6 is partial: the bridge page is
corrected, but one edge-case row still assigns `b` to the host side. Two
minor new problems are listed at the end; no new major finding.

## Per-item status

| # | Status | Current evidence (quote) | Remainder |
|---|---|---|---|
| 1 | Filled in full | `terminal-usage.md:157-163`: "A full-duplex loopback (TX tied to RX) at 9600 bps, 8/NONE, normal output (the looped line needs no pull-up)" and `HiZ>m3 5 1 1 1 2 1` → header `( 4 416 0 0 0 0 0 )` (`hiz 0`). | None. |
| 2 | Filled in full | `binary.md:30-32`: "on **v4** send the VPU feed and enable power + pull-ups; on **v3** there is no feed command — feed `Vpu` externally and enable power + pull-ups, or select normal output"; `binary.md:166-169` gives the v3/v4 bring-up table; `edge-cases.md:98` now has the v3 branch. | None. |
| 3 | Filled in full | `bridge.md:34-37`: "On **v4** the exit prints a blank line and the `UART>` prompt returns... On **v3** the reset reboots the board to `HiZ>`: the UART settings and any queued words are lost." | None. |
| 4 | Filled in full | `errors.md:46-57`: "The count is internal: no command or status field prints it." and "The MODE LED is the LED the firmware turns on when a non-HiZ mode is entered (not the USB LED)." | None. |
| 5 | Filled in full | `terminal-usage.md:22-24`: "A `;N` suffix after a value sets that value's width and stays in force for later values until changed — for example `0x1FF;9`". | None. |
| 6 | **Partial** | `bridge.md:79-83`: "`b` changes the **Bus Pirate** side, not the computer side: after the rate selection it prints `Adjust your terminal`..." is correct. | `edge-cases.md:73` still says "set the host rate first with `b` (default 115200), then start the bridge". This repeats the conflation the bridge page just fixed. Change to "set the terminal to the Bus Pirate's current rate first; `b` changes the Bus Pirate side." Residual severity: minor. |
| 7 | Filled in full | `README.md:3`: "documented for v3 and v4; v3 hardware behaviour is unverified." The eight per-page "v3 column is not verified" footers and `edge-cases.md` §Unverified paths are gone; only the specific `-p` and 5.5 V caveats remain. | None. |
| 8 | Filled in full | `README.md:69-70` uses "`Live monitor`" and "`Bridge with flow control`"; `README.md:102-103` maps both firmware names. "raw monitor" / "raw live monitor" prose is gone. | None. |
| 9 | Filled in full | `entry-and-setup.md:369-371`: "If any **required** field (through `output`) is missing or out of range, the firmware falls back to the full prompt chain..." | None. |
| 10 | Filled in full | `entry-and-setup.md:325`: "`9` is custom baud or an auto-baud result"; `entry-and-setup.md:336-341` adds the menu-number → one-line → header table. | None. |
| 11 | Filled in full | `wiring.md:126`: "A pull-up to the target's logic voltage (3.3 V typical) is required"; `hduart.md:46` has the same wording. "stated explicitly" is gone. | None. |
| 12 | Filled in full | `wiring.md:17-20`: "A **non-self-powered target** also needs its supply..."; `wiring.md:37` now says "The three required signal wires". | None. |
| 13 | Filled in full | `README.md:26-47` adds the Reference index and Worked sessions; `README.md:124-125` adds the `0x0F` limit; `binary.md:11-15` opens with the one-way-bridge warning and `binary.md:100-106` has the trap section. | None. |
| 14 | Filled in full | `terminal-usage.md:124-127` owns the full ring description; `errors.md:37-39` links to it and keeps only the two overrun events; `hduart.md:142-145` links to both. | None. |
| 15 | Filled in full | `hduart.md:74-78` is now a Moment/What-happens table; `entry-and-setup.md:246-253` is a Condition/Result table; `terminal-usage.md:103-112` is a Point/Behaviour table. | None. |

## Code-owner candidates

- **Auto-baud overflow message text — docs accurate; code decision
  deferred.** `entry-and-setup.md:259-263` says "A measured rate above 150000
  prints the overflow message instead of the `Estimated:`/`Calculated:` pair...
  The printed 16,000,000 is the message text, not the check"; `macros.md:94-96`
  and `errors.md:111` repeat the same explanation. The entry-time path can
  still select 128000 or 256000, while the `(4)` macro only reports; the
  `Calculated:` list no longer claims 256000. This matches the firmware
  behaviour and correctly flags the misleading message text.
- **Stale RX ring after `w` — docs accurate; code decision deferred.**
  `entry-and-setup.md:401-404` says "A word left in the ring by an earlier
  session can still be returned by `r` until `W` clears it"; `edge-cases.md:37`
  says "the port is disabled but the ring is not cleared until the next
  enable; `W` clears it." That matches `uart2_disable()` leaving
  `uart2_rx_head`/`uart2_rx_tail` and `uart2_enable()` resetting them.

## Presentation check

- No TAB characters and no stray control characters in any UART file.
- Captures are space-aligned; the speed-menu indentation matches the firmware
  (options 1–9 indented, 10–11 not), and the settings-header captures are
  consistent.
- Menus and option lists are tables where they are reference material: the
  mode menu, speed menu, frame settings, duplex options, `(0)` macros and the
  new condition/result tables. The remaining code blocks are firmware
  transcripts.
- No non-table prose line exceeds 80 columns.
- No dates, bench/rig narratives, pass counts, evidence sections, capture
  provenance or internal log names. The three remaining "unverified" lines
  (`README.md:3`, `errors.md:125`, `wiring.md:182`) are short status caveats,
  not test-result prose.

## New problems (max 5)

1. **[minor] `hduart.md:84-99` — the new HDUART transcript starts at the wrong
   prompt.** The text says "With the mode selected and `W` run, a write and the
   empty read look like this", but the transcript starts `UART>m3 5 1 1 1 1 2`
   and then runs `W`. A fresh user is at `HiZ>`. Fix: start `HiZ>m3 ...`, or
   drop the `m3` line and start from the already-engaged `UART>`. Also state
   that the required pull-up is fitted.
2. **[minor] `binary.md:169` — the v3 `0x90` alternative leaves the target rail
   off.** The table says "feed `Vpu` externally..., then `0x4C`; or send `0x90`
   for normal output". `0x90` changes the frame to normal output but does not
   enable the regulator; the `0x4x` power bit does. Fix: say "for a
   self-powered target, send `0x90`; otherwise send `0x4C` for power, then
   `0x90`."

## Still open

- Finding 6: partial — `edge-cases.md:73` still says "set the host rate first
  with `b`".
- New minor: HDUART transcript start/pull-up note; binary v3 `0x90` power
  sequence.
