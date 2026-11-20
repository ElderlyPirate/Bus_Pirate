# Self-test re-audit — round 1

**Scope:** the six `docs/capabilities/selftest/` pages against the 16 findings in
`feedback-selftest-round1.md`, plus the presentation rules. I did not run a new
full review; the four cross-set findings are checked only for their current
state in the cited pages.

**12 of 16 items filled in full.**

The selftest set itself is 12 of 12 in-scope fixes. The four cross-set-only
findings (3, 4, 14, 15) remain for the main session, and the SMPS-page half of
finding 1 is the fifth cross-set correction in the fix record.

## Per-item status

| # | Status | Evidence (current) | Remainder |
|---|---|---|---|
| 1 | **Filled in full** | `terminal.md:176`: "The refusal is a binary-layer condition"; `checks.md:239`: "The reachable sequence is:"; `edge-cases.md:23-25`: "both routes back to `HiZ>`, `#` and the binary `0x0F` leave, reset the board and stop the feedback loop"; `terminal.md:197-198`: "`0xF1` is a raw-wire command, so a terminal-only user cannot send it from `HiZ>`." | Selftest set. The cross-set `smps/README.md:34` still says the v4 terminal reset "is not confirmed to" stop the SMPS. That is the recorded SMPS half; the selftest pages are internally coherent. The `HiZ>~` block is now a conditional fallback, not the primary sequence. |
| 2 | **Filled in full** | `README.md:37-39`: "Send `0xFF` to leave the test (reply `0x01`); the host is still in `BBIO1`. Send `0x0F` to return to the terminal (reply `0x01`, then the banner and `HiZ>`)."; `README.md:41-42`: "leaves the pins in HiZ; the terminal run returns to `HiZ>`, while a binary session stays in `BBIO1` until `0x0F`." | None. The bullet also adds that both passes need the target disconnected and the board powered (`README.md:34-36`). |
| 3 | **Not addressed (cross-set)** | `protocol/binary/commands.md:220`: "without the jumper-only pin group." | Recorded cross-set item 2. The selftest `checks.md:143-145` is correct: "`Bus Hi-Z 1` runs only in the full pass, even on v4 where no external jumper is needed." |
| 4 | **Not addressed (cross-set)** | `protocol/binary/commands.md:237`: "commands report a count of `0x01` (one error) without running the test". | Recorded cross-set item 3. The selftest `checks.md:266` and `binary.md:54` correctly call it a refusal count. |
| 5 | **Filled in full** | `checks.md:47-72` is a 5-column matrix; `checks.md:85`, `:116`, `:137` are 3-column detail tables; `edge-cases.md` is now per-case Symptom/Behaviour/Recovery. | The old 255-character `READ&WRITE` row and 332-character edge-case row are gone. The widest remaining row is 170 characters (`terminal.md:163`), down from 255/332; no 7-column or 4-column table remains. |
| 6 | **Filled in full** | `terminal.md:160`: "`ADC(...) FAIL` alone"; `edge-cases.md:80`: "`ADC(...) FAIL` (for example `ADC(0.00) FAIL`)". | The `(...)` is a placeholder; the edge-cases row supplies a literal example. |
| 7 | **Filled in full** | `checks.md:97-104`: "`WP` reads the line high first; `READ&WRITE` then drives the same line low for the write and restores the saved level ... That is why the two adjacent checks do not contradict: they test the same line in opposite states." | None. The row at `checks.md:95` also says the check "writes, reads back and restores one EEPROM byte." |
| 8 | **Filled in full** | `checks.md:106-109`: "A `PULLUP L` pass only proves the control line read low after the pull-ups were requested off. A line stuck low also passes it. Read it with `PULLUP H` and `Bus Hi-Z 1`"; `checks.md:225-226` repeats the limit under "What is not checked." | None. The row now says "requests the pull-ups off," not "disables the pull-ups." |
| 9 | **Filled in full** | `terminal.md:138-150`: "The run (v3, full test)"; "`ADC and supply`: `5V`, `VPU`, `3.3V`, `ADC`"; the three pin groups; the v3 LED prompt and `Found <n> errors.` | No literal v3 capture; the order list is source-derived and the page still says "not verified on v3." The final `HiZ>` is only in the v4 capture, but the run's return is stated in the limits and the README. |
| 10 | **Filled in full** | `wiring.md:25`: `[terminal.md](terminal.md#setup)`; `terminal.md:171`: `[checks.md](checks.md#voltage-checks)`; `terminal.md:180` and `binary.md:65`: `[checks.md](checks.md#when-the-run-does-not-start)`. | A link/anchor check over all six files resolves every relative link and anchor. |
| 11 | **Filled in full** | `wiring.md:95`: "`- [binary.md](binary.md) — the `0x10` / `0x11` passes.`" | None. |
| 12 | **Filled in full** | `README.md:51`: "`Vusb`, `5V VPU`, `3.3V VPU` \| — \| yes" (the availability row). | "Two internal pull-up rails" is gone; the labels match the check matrix. |
| 13 | **Filled in full** | `checks.md:23-24`: "The `?` help lists the command as `Selftest`; this set writes self-test in prose." | None. |
| 14 | **Not addressed (cross-set)** | `generic/README.md:75`: "Nothing is driven except the selected AUX pin:". | Recorded cross-set item 4. The selftest `README.md:11-13` is clear that the run "takes over the pins and rails." |
| 15 | **Not addressed (cross-set)** | `generic/edge-cases.md:20`: "`d`, `D`, `P`, `e`, `g`, `S` or `v` while the optional SMPS feedback owns the ADC". | Recorded cross-set item 5. The selftest pages now say the terminal `~` refusal is normally unreachable; the generic list still omits `~`. |
| 16 | **Filled in full** | `wiring.md:71-73`: "On v3 remove any external voltage from `Vpu` before fitting the `Vpu`→`+5V` jumper; on v4 leave `Vpu` unconnected."; `wiring.md:39`: "v4: no jumper, no external voltage." | None. |

## Blocker and sequence check

The blocker is resolved in the selftest set. `checks.md:239-262` gives a
reachable binary sequence end to end:

```
BBIO1 -> 0x05 -> RAW1 -> 0xF2 0x58 -> 0x00 -> BBIO1
      -> 0x10 -> 0x01 refusal (idle loop) -> 0xFF -> BBIO1
      -> 0x05 -> RAW1 -> 0xF1 (stop) -> 0x00 -> BBIO1
      -> 0x0F -> terminal
```

The exits are distinct and exact:

- `0xFF` leaves the self-test to `BBIO1`, reply `0x01`, no `BBIO1` reprint.
- `0x0F` leaves the binary layer, reply `0x01`, then the banner and `HiZ>`.
- `0x0F` inside the self-test idle loop is data, not a leave; `0xFF` is the
  only self-test leave (`binary.md:87-92`).
- `0xF1` in `BBIO1` is a pin-state byte, not an SMPS stop
  (`checks.md:271-273`).
- The terminal `~` at `HiZ>` is now a conditional fallback: both `#` and the
  `0x0F` terminal reset stop the SMPS, so a normal `HiZ>` prompt cannot see a
  running SMPS (`terminal.md:182-185`, `edge-cases.md:22-25`).
- The terminal `~` refusal, if the guard trips, still has the full exit text:
  no setup prompt, no `Found` line, `Syntax error at char 1`, then `HiZ>`
  (`terminal.md:186-198`).

The fix record correctly rejects the audit's NORMAL-button suggestion: the
NORMAL button is the bootloader button, not a reset. The pages use the v4 reset
button or a power-cycle. That is the right correction.

## Presentation verification

- **Tabs:** 0 TAB characters in all six files.
- **Captures:** every `host ->` / `host <-` comment in `binary.md` and
  `checks.md` starts at column 25; no misaligned columns.
- **Tables / option lists:** no 7-column or 4-column table remains. The check
  matrix is 5 columns, the detail tables 3, and the README/wiring/terminal
  tables 2–5. The set has no option menus; the v3 order is a numbered
  procedure, which is appropriate. The edge-cases are per-case definitions,
  not menus.
- **Wraps:** no prose line exceeds 82 columns; no code-block line exceeds 80.
- **Bench prose / dates / pass counts:** none. The only status prose is the
  allowed "not verified on v3" caveat. There are no dates, "captured on"
  provenance, evidence sections, internal log names, or pass-count claims. The
  `21 / 15` and `27 / 22` rows are check totals, not pass counts.
- **Table width:** the widest row is 170 characters (`terminal.md:163`), down
  from 255 and 332. The fix record says "under 170"; this row is exactly 170,
  a one-character difference, not a reader problem.

## New problems caused by the fixes (max 5)

1. **[minor]** `README.md:65` still says the self-test does not test "the v4
   NORMAL button", while the recovery prose uses "the v4 reset button"
   (`terminal.md:196-197`, `checks.md:277`). The fix record says the NORMAL
   button is the bootloader button, so these are presumably different buttons,
   but the set never says so. Add "(bootloader)" once to the README line, or
   use the same name if they are the same button.

2. **[minor]** `edge-cases.md:18-28` presents "`~` reports the SMPS is busy" as
   a routine case, while `terminal.md:182` and `checks.md:278` say a terminal
   `~` does not normally meet it. The edge-case's Behaviour sentence explains
   the normal state, but the heading and Symptom still read as a normal
   trigger. Add "(unexpected; the guard tripped)" to the heading or the
   Symptom line so all three pages agree on reachability.

3. **[minor]** `edge-cases.md` grew from 77 lines to 263 lines of per-case
   `###` headings with no index. The per-case format reads better than the old
   332-character rows, but scanning for one case is slower. Add a short top
   index (the three `##` groups plus their case headings), or keep one compact
   trigger table at the top that links to the cases.

4. **[minor]** `edge-cases.md:104-106` ("binary `0x11` with a missing jumper")
   lists v4 `0x01` for the `ADC` jumper, v3 `0x05` for `Vpu`, and `0x06` for
   both, but not the v3 ADC-only case. A v3 full `0x11` with only the `ADC`
   jumper missing also returns `0x01`; add that third v3 case.

## Still open (cross-set only)

- Finding 3: `protocol/binary/commands.md:220` still says "jumper-only pin
  group."
- Finding 4: `protocol/binary/commands.md:237` still says "(one error)."
- Finding 14: `generic/README.md:75` still says "Nothing is driven except the
  selected AUX pin."
- Finding 15: `generic/edge-cases.md:20` still omits `~`.
- Finding 1 dependency: `capabilities/smps/README.md:34` still says the v4
  terminal reset "is not confirmed to" stop the SMPS. The selftest pages now
  say it does, matching the code trace.
