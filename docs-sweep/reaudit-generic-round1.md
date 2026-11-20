# Generic prompt layer — re-audit of round 1 findings

Scope: the 18 numbered findings in `feedback-generic-round1.md`, re-read
against the current `docs/generic/` set and the two `docs/SPEC.md` lines that
were cited. This is not a fresh full review.

**Verdict: 17 of 18 items filled in full** — 17 fixed, 0 partial, 0 regressed;
1 rejected as a false positive with credible firmware evidence.

## Per-item table

| # | Status | Current evidence | Remainder |
|---|---|---|---|
| 1 | Filled in full | `README.md:88-89`: "Both are unavailable inside BASIC: the interpreter receives every line until `EXIT`." `navigation.md:220-221`: "Neither runs inside BASIC ... so both are syntax errors there; type `EXIT` first." `usage.md:341-343`: "Neither `#` nor `m1` can be typed inside BASIC." `edge-cases.md:88`: "`#`, `$`, `m`, `m1` in BASIC ... type `EXIT` first." | The old "`#` leaves BASIC" claim is gone; all four pages agree. |
| 2 | Filled in full | `commands.md:151-152`: "`]` stop; closes either start" / "`}` stop from read; runs the same stop as `]`"; `commands.md:162-167`: "`]` and `}` both stop either start ... `r` and `r:N` also work outside a start/stop"; `usage.md:261-263`: "either `]` or `}` stops it ... `r:2` after the stop is an ordinary unframed read." | The pair question and the `r:2`-after-`]` question are both answered. |
| 3 | Filled in full | `usage.md:180-181`: "Six answers produce seven fields because `brg` is computed, not typed"; `usage.md:183-191` decodes `spd`, `brg`, `dbp`, `sb`, `rxp`, `hiz`, `dup`; `usage.md:203-207`: "`R2W` is the firmware's settings-header prefix for `2WIRE`; the two answers are speed and output type, decoded in the 2-Wire entry page." | All summary tokens are mapped; the 2-Wire link resolves and its section exists. |
| 4 | Filled in full | `README.md:126-128`: "`2WIRE` reuses the firmware's `I2C START BIT` / `I2C STOP BIT` strings for its start and stop wrappers ... the prompt and menu still say `2WIRE`." `usage.md:243-244`: "the 2WIRE firmware displays the shared I2C wrapper wording." | The false "the wording is 2WIRE's" sentence is gone. |
| 5 | Filled in full | `navigation.md:118`: "the table below transcribes the v4 screen"; `navigation.md:163-173`: neutral `Help row` table for rows 4, 8, 9, 10, 17, ending "On v3 `o` is row 17; on v4 it is row 4." | The v4-slot labels that made the v3 row read backwards are gone. |
| 6 | Filled in full | `SPEC.md:99`: "hidden: 5–19× `0x00` then raw STX (`0x02`); 20 NULs enter binary instead." | Matches `commands.md:239` and `edge-cases.md:101`; the upper bound is explicit. |
| 7 | Filled in full | `navigation.md:155-159`: "**Case matters.** `s` enters BASIC, while `S` starts the servo generator on AUX; `o` sets the display format, while `O` toggles the 1-Wire speed"; `navigation.md:154`: "`t`, `T` and `O` are working commands with no help row; on v4 so is `#`." | Both unrelated case pairs and the no-help-row list are covered; `edge-cases.md:91` was updated too. |
| 8 | Filled in full | `README.md:3-6`: "v4 is the primary documented board; the v3 build is covered but not verified on v3 hardware.  Captures in this set are v4; inline notes and the delta table in navigation.md list the differences." `SPEC.md:13-14`: "the v3 build is documented but not verified on v3 hardware." | The undefined "build-only" term is gone from both pages. |
| 9 | Filled in full | `usage.md:7-9`: "Every session starts at `HiZ>` and the capture shows that prompt.  The sessions are independent; if you are following straight through ... run `m1` first"; the `v` and `#` captures now start `HiZ>v` (`usage.md:273`) and `HiZ>#` (`usage.md:321`). | No bare-start captures remain; "Wiring: unchanged" is gone from the mode-change session. |
| 10 | Filled in full | `values.md:164-166`: "A space after `r` is not a display letter either: `r ` is a plain read in the current `o` format." | The contradictory "`rx` followed by a space" sentence is gone. |
| 11 | Filled in full | `usage.md:295-303`: "The colour codes in row 1 are the probe-cable colours" with a full `Bk/Wt/Gy/Pu/Bu/Gn/Yw/Or/Rd/Br` table; `usage.md:311-314`: "`0` is the firmware's label for the tenth column (`MISO`), not a typo and not a separate pin." | Colour legend and the v3 `0`/pin-10 question are both answered. |
| 12 | Filled in full | `commands.md:227-230`: "`c` / `C` (v4 `k` / `K`) choose the target, and `&` / `%` run their delays" in the HiZ paragraph. | The HiZ works/refused list no longer leaves target selection or delays unclassified. |
| 13 | Filled in full | `macros.md:91`: "[edge-cases.md](edge-cases.md) — macro traps and recovery." | The false "usage.md — a mode session with macros" reference is gone. |
| 14 | Filled in full | `usage.md:106-108`: "`o1` re-selects HEX, the default (no change); `v` is independent of the display format and shows the UART signal names." | The causal wording is gone. |
| 15 | Filled in full | `edge-cases.md:37`: "`(HiZ)` here is the open-drain high state, not the HiZ mode"; `commands.md:115-116`: "`(HiZ)` in that warning names the high-impedance high state of an open-drain output; it does not mean the board is in the HiZ mode." | The parenthetical is explained in both the `P` and `e` contexts. |
| 16 | Filled in full | `README.md:57-58`: "`W` starts the supplies, runs the mode's bus setup ... It does not repeat the setup prompts"; `navigation.md:213-214`: "`W` ... runs the mode's bus setup (the engage routine); it does not repeat the setup prompts"; `usage.md:165-166` adds the same qualifier. | Entry "setup prompts" and the `W` engage routine are now distinct. |
| 17 | Filled in full | `navigation.md:244-249`: "`BRG` is the baud-rate generator divider, not a rate: usable values are 1–32767, 34 is the firmware's default (the 115200 bps preset) and 127 is 31250 bps (MIDI).  The `(34)` is that default, not the rate currently in force." `navigation.md:251-252` links the UART formula/rate table. | Meaning, usable range, default and `(34)` are fixed. The firmware accepts `0` at the prompt but the page calls 1–32767 usable; the implementer already records the validator as a code-owner candidate, not a doc defect. |
| 18 | Not addressed — rejected as a false positive | `SPEC.md:114` still reads "SPI / I2C / UART / 1W01 / RAW sub-protocols". The rejection evidence is credible: `protocol/binary/sub-protocols.md:21` maps `0x04` to 1-Wire with identifier `1W01`; `protocol/wire/1wire/binary.md:20` says the mode prints `1W01`, and line 25 explains "1-Wire protocol version 1". | `1W01` is a distinct binary sub-protocol identifier, not a second spelling of the `1WIRE` mode. My original finding was wrong; no change needed. |

## Presentation check (current docs)

- **TABs:** none in `docs/generic/*.md` or `docs/SPEC.md`.
- **Aligned captures:** the two `v` captures still align at columns 0, 9, 18,
  27, 36, 45, 54, 63, 72, 81, 90, 99 on all four rows; the UART setup option
  list is still right-aligned. The added `HiZ>v` and `HiZ>#` prompts do not
  disturb either capture.
- **Menus / option lists as tables:** the help screen, mode menu, `b` rates,
  `e` sources, output formats, colour legend and the new settings-token table
  are tables. The UART and 2WIRE setup blocks are device-output captures, not
  bullet chains.
- **~80-column wraps:** no prose line exceeds 80 columns outside tables,
  headings and code captures. The only >80 lines are the 100–105-column `v`
  captures, which are inherently wide device output.
- **Bench prose / dates / tested or verified results:** none found. The only
  `verified` strings are negative caveats (`not verified`) at `README.md:4`,
  `SPEC.md:13`, `commands.md:43`, `usage.md:314`/`368` and
  `edge-cases.md:98`; there are no dates, pass counts, capture provenance, or
  internal log/test names. The scope sentence "Captures in this set are v4"
  is an availability note, not a bench provenance claim.
- **Links / anchors:** every internal link and anchor in the generic set and
  SPEC resolves, including the new `README.md#v3--v4-delta`,
  `navigation.md` → `../protocol/uart/entry-and-setup.md#custom-baud-option-10`,
  and `usage.md` → `../protocol/wire/2wire/entry-and-setup.md`.
- **Trailing spaces:** only inside firmware captures (the banner line, the
  `READ:` lines and the UART option-list padding); no layout defect.

## New problems outside the item list (max 5)

1. **[severity: minor] `docs/protocol/wire/2wire/entry-and-setup.md:93`** — the
   page now linked from `usage.md:206` shows `R2W (spd hiz)=( 1 1  )` with two
   spaces before `)`, while the firmware and the generic capture
   (`usage.md:211`) use one. Fix the one space. This is outside the generic
   set and is already noted in `fixes-generic-round1.md`; it is not counted in
   the 17/18.

No other new problems were obvious within the item scope. The structural
suggestions from round 1 (README length, moving the idle section, a "start
here" path) were intentionally not applied; they were opinion, not numbered
findings, and are not scored here.

## Counts

- Filled in full: 17
- Partial: 0
- Not addressed: 1 (rejected as a false positive — `1W01`)
- Regressed: 0
