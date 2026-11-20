# SUMP set — re-audit (round 1)

Scope: the ten pages in `docs/protocol/sump/`, checked against the 19 findings
in `feedback-sump-round1.md` and the per-item record in `fixes-sump-round1.md`.
I did not re-review the set from scratch; I only checked whether each finding is
now resolved, plus the required presentation rules.

**Verdict: 18 of 19 items filled in full.** One item is partial: finding 4's
opening absolute still contradicts the corrected text that follows it. The
replacement wording is a two-line fix, not a protocol change. No new major
problems and no regressions were found.

Counts: **18 full, 1 partial, 0 not addressed, 0 regressed.**

## Per-item record

| # | Status | Current evidence | Remainder |
|---:|---|---|---|
| 1 | **Filled in full** | `depth.md:146` — `8188` / `4096` / `4092`; `depth.md:156` — "8188 pads 4092 bytes and still records every channel"; `commands.md:71-72` — "There is no way to express 8191, 4095 or any other count that is not a multiple of four."; `readout.md:78` — "a count of 8188, for example, captures 4096 samples and pads the other 4092 bytes."; `edge-cases.md:46` — "an 8188 count ... 4092 padding bytes". | None. The tier ranges now end at 8188 / 16 380 / 32 764 and the cliff examples are expressible. |
| 2 | **Filled in full** | `rates.md:102-103` — "**no divider is set**. The default timer period is 1601 ticks, which gives about 9.994 kHz."; `rates.md:104-105` — "sending `0x80` with `x=1601` would select a different, much faster rate (about 62 kHz)"; `commands.md:159` — "not set; the default timer period is 1601 ticks, about 9.994 kHz"; `entry.md:81` — "1601-tick default timer period (about 9.994 kHz)". | None. The timer period and divider are now separate, and `x=10 000` is given as the explicit default period. |
| 3 | **Filled in full** | `client.md:117-119` — "at a capture tier of 8192 or deeper, where **high** channel bits are dropped ... bits 0–3 ... are kept and CS (bit 4) is dropped"; `trigger.md:147-151` — "drops **high** channel bits ... CS (bit 4) is dropped; a CS trigger still fires"; `depth.md:64-65` — "The dropped bits are the **high** channel bits, not the low ones."; `examples.md:78-79` — "CS (bit 4) is dropped". | None. |
| 4 | **Partial** | `entry.md:134` — "The exit bytes are commands only when the analyzer is waiting for a command." The following lines now say: `entry.md:135-139` — "A byte sent while the sampler waits for a trigger, or while a timer-paced capture runs, aborts the capture and is then handled as a command ... Inside a five-byte command an exit byte is a parameter: the command completes and **takes effect**". | The opening absolute still contradicts the corrected cases: a byte during a trigger wait or timer capture does abort and is then handled as a command. Replace the first sentence with "At a command boundary, the three exit bytes are exits. A byte during a trigger wait or timer capture aborts the capture first; inside a long command it is a parameter." |
| 5 | **Filled in full** | `trigger.md:84-100` — "The fill runs in whole bursts; a burst holds 4 ms worth of samples, or 16 samples if that is more ... Even a zero-length pre-trigger part costs one burst"; the rate table lists 1 MHz / 4000 / 4 ms, 1 kHz / 16 / 16 ms, 100 Hz / 16 / 160 ms and 5.96 Hz / 16 / 2.7 s; `trigger.md:98-101` — "The trigger arms at the end of the burst that completes the pre-fill ... Start the capture more than one burst before the event, use a free-run capture ... or accept the loss."; `edge-cases.md:61` links the same table. | None for the original finding. See the minor new-problem note about a non-zero pre-trigger below. |
| 6 | **Filled in full** | `trigger.md:124-127` — "not polled until the window and the readout finish; the window is about 1 ms on v4 (2.05 ms for 4096 samples at 2 MHz on v3), and the readout follows it (about 0.36 s for 4096 bytes at 115200 baud on v3)"; `readout.md:111` — "the window and the readout finish, then the byte is handled"; `edge-cases.md:91` matches both numbers. | None. |
| 7 | **Filled in full** | `rates.md:41` — "Common requests and the rate they achieve"; `rates.md:22-23` — "the timer period or engine band that `x` falls in, which is not always the nearest rate"; `rates.md:46` — "8.33 MHz (nominal)"; `rates.md:60` — "The 8.33 MHz row is a nominal request: divider 11 selects the 8 MHz engine."; `rates.md:63-64` — "300 kHz becomes 296.3 kHz at divider 332 (period 54 ticks)." | None. |
| 8 | **Filled in full** | `client.md:28` — "ancillary version string `enh`, the enhanced-profile marker"; `commands.md:96` — "ancillary version string `enh` (the enhanced-profile marker)"; `README.md:40` — "ancillary version string; the marker the enhanced profiles match". | None. |
| 9 | **Filled in full** | `entry.md:165` — "the board answered `BBIO1` ... send `0x0F` to return to the terminal"; `entry.md:166` — "the board answered a sub-mode identifier (`I2C1`, `SPI1`, …) ... send `0x00`, read `BBIO1`, then send `0x0F`" with the binary edge-case link. | None. |
| 10 | **Filled in full** | `edge-cases.md:36` — "a trigger wait longer than the 32-bit sample counter"; symptom "no visible symptom; the reply length, order and pre-trigger split are unchanged"; recovery "the counter is rebased at a whole number of ring revolutions; no action needed". | None. |
| 11 | **Filled in full** | `readout.md:122-127` — "A long burst can overrun the v3 UART: bytes beyond the receiver are lost and the next command may be ignored ... send five `0x00` resets or wait one second ... If the board stays silent, the receive path itself is stopped; power-cycle it."; `edge-cases.md:92` matches. | None for the original finding. The code-owner caveat is stated (reception can stop until power-cycle); see the minor wording note below. |
| 12 | **Filled in full** | `client.md:48` — "the terminal's baud rate (default 115200), 8 data bits, no parity, one stop bit"; `client.md:50-52` — "If `b` was used ... set the client to the same rate; a mismatch gives no `1ALS` or a garbled one." | None. |
| 13 | **Filled in full** | `README.md:75-81` — "Install sigrok or PulseView ... set 1 MHz and 4096 samples, leave the trigger off, and run with a known square wave on one probe pin (common ground). The exact menu names vary by release"; `README.md:113-114` — "No SUMP client ships with the firmware; First capture names the client and the first settings." | None. The install link and the five-line first capture are present. |
| 14 | **Filled in full** | `commands.md:21-22` — "an exit additionally prints the terminal banner and the `HiZ>` prompt, which are terminal output rather than a SUMP reply." | None. |
| 15 | **Filled in full** | `client.md:66-69` — "A request above the advertised 16 MHz is not an error; it selects the 16 MHz engine. The slow end is the exception: a request slower than about 6 Hz wraps the divider, so clamp it before computing the request." | None. |
| 16 | **Filled in full** | `depth.md:97-99` — "A request below the tier size fills only that many slots; a request inside a tier fills the tier size and pads the rest." | None. |
| 17 | **Filled in full** | `examples.md:92-94` — "This block is for v4: `c0 01` selects AUX2. On v3 the same byte selects CS (bit 0), which the 8192 tier keeps; change the mask deliberately there."; the 1 MHz block also has `examples.md:53-54` for `c0 10` / AUX on v3. | None. |
| 18 | **Filled in full** | `client.md:16-17` — "The `0x02` that enters SUMP is answered with the four bytes `1ALS`; each later `0x02` request returns it again."; `readout.md:136` — "a later `0x02`" returns "`1ALS`". | None. |
| 19 | **Filled in full** | `README.md:87` — "present in the build"; `README.md:95-96` — one note: "The v3 feature is present in the build and **not verified on v3 hardware**. Fast capacities, 4 MHz jitter and readout timing are the v3-specific caveats."; `depth.md:172` — "present in the build, not verified"; the old `edge-cases.md` "Unverified paths" section and the trailing caveats in `trigger.md` and `rates.md` are gone. | None. The status line and availability table still mention v3, but the set no longer repeats the caveat in every page; "build-only" is gone from the SUMP set. |

## Partial item in detail

**Finding 4.** The behavioural half is fixed: `commands.md:134` now says an
exit byte mid-command is "consumed as a parameter; the exit does not happen, and
the command completes and takes effect (unless its opcode is unknown)", and
`entry.md:135-139` says a byte during a trigger wait or timer capture aborts and
is then handled as a command. The only remaining problem is the **opening
absolute** at `entry.md:134`:

> The exit bytes are commands only when the analyzer is waiting for a command.

That sentence is contradicted two lines later by the capture-abort case, and by
`readout.md:109-110` and `trigger.md:136-143`, which say a byte during a wait or
timer capture aborts and is handled. It is a wording partial, not a missing
behaviour. Replace it with "At a command boundary, the three exit bytes are
exits" or delete it and let the two cases stand.

## Presentation rules

| Rule | Result | Evidence |
|---|---|---|
| No TAB characters | **Pass** | `tabs=0` in all ten files; no trailing whitespace. |
| Aligned captures | **Pass** | `examples.md` comments start at column 31; `entry.md` and `readout.md` at column 32. `readout.md:68` is a one-line diagram, not a comment column. |
| Menus / option lists as tables | **Pass** | No menus or option lists in this set; the settings, metadata, masks, rates and tiers are tables. |
| ~80-column wraps | **Pass** | No prose line over 80 characters. The only 89-character line is the link at `README.md:118`, a table-adjacent link, not a broken wrap. |
| No bench prose, dates, tested/verified-result prose | **Pass** | No dates, pass counts, "captured on" provenance, evidence sections or internal log names. The only "not verified" strings are short inline caveats in `README.md:4/95`, `depth.md:129`, `rates.md:74` and `readout.md:89`; no positive "tested/verified" claims. |
| Naming consistency | **Pass** | `ancillary version string` is used in `client.md`, `commands.md` and `README.md`; `1ALS`/`SLA1` and `A0`/`AUX` mappings remain explicit; high/low channel-bit wording is fixed. |
| Relative links and anchors | **Pass** | All `.md` targets and heading anchors in the ten pages resolve, including the new `entry.md` binary link and the `README.md#first-capture` link. |

## Code-owner and cross-set notes

- **v3 UART overrun.** The SUMP pages now state the consequence and the fallback:
  `readout.md:122-127` and `edge-cases.md:92` say bytes can be lost, the parser
  can need five `0x00` resets or a one-second gap, and a silent board needs a
  power-cycle. That is consistent with the recorded code-owner candidate (the
  SUMP loop never clears `OERR`); the pages do not claim a firmware fix. The
  exact reset-button versus power-cycle action remains a hardware unknown, but
  power-cycle is the safe instruction.
- **Core system "at least 5 NUL" wording.** This is outside the SUMP set. The
  SUMP pages still say five to nineteen NULs in `README.md:53-58` and
  `entry.md:10-28`; no SUMP change is needed.
- **5.5 V authority.** The SUMP pages still state "5 V safe, 5.5 V maximum" as
  fact (`README.md:70`, `entry.md:89/108`). That remains the pending cross-set
  decision; it is an electrical-limit question, not a SUMP behaviour
  contradiction.
- **"build-only" terminology.** The SUMP set now uses "present in the build";
  the remaining build-only wording is in other sets and was not touched here.

## New problems from the fixes

1. **[minor] `trigger.md:89-99` — the new blind-window table is per burst, not
   total pre-fill.** The table's `Time before the trigger arms` is the time of
   the burst that completes the pre-fill, which is correct for a zero pre-trigger
   (the case the finding targeted). With a non-zero pre-trigger the trigger is
   not armed until the pre-fill finishes: at 10 kHz a 3072-sample pre-trigger is
   about 307 ms, not the table's 4 ms, and "start more than one burst before the
   event" is then not enough. Add "for a non-zero pre-trigger, add the pre-fill
   time" or label the table "zero pre-trigger / final burst".
2. **[minor] `readout.md:124` — "the next command may be ignored" understates
   the v3 OERR case.** The code-owner candidate says the receiver can stay
   stopped until a reset/power-cycle, so later commands, not only the next one,
   can be ignored. The following sentence does give the power-cycle fallback, so
   this is only a wording precision issue; "later commands" would match the
   candidate.

No other new problems were seen in the changed pages.
