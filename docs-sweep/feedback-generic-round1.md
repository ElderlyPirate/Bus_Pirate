# Generic prompt layer — reader feedback, round 1

## Verdict

**Yes with fixes.** The set is unusually good at the things that usually go wrong
in firmware docs — exact device strings, naming maps for AUX/VPU, macro and
bootloader traps — but a reader who follows the text literally will hit four or
five real contradictions (BASIC/`#`, the read-wrapper pair, the direct-mode
summary tokens, and 2WIRE's `I2C` messages). Fix those and this is a strong set.

## Task log

### 1. First impression from the README alone

- **Where:** `docs/generic/README.md`, no other file.
- **How fast:** under a minute for the title, status, link table and five-minute
  version. The five-minute section starts at line 35, so it is early.
- **Success:** yes for "this is the prompt layer" and "v4 is primary". No for
  "does it apply to my v3?" — see below.
- **What was unclear:** `v3 is build-only` is undefined and sounds like v3 is
  not a usable/documented board, while the table says v3 `yes` and every page
  carries v3 deltas. The README never says the captures are v4. A v3 owner can
  reasonably stop at the first paragraph. `Clutch` is used in the five-minute
  version before the glossary defines it.

### 2. Enter and leave a mode, mode menu, prompt navigation

- **Where:** README link table → `navigation.md` (prompt, line editing, `h`,
  `i`, `?`, mode menu) → `usage.md` §Mode change.
- **How fast:** about three minutes; the headings and tables are easy to scan.
- **Success:** yes for `m`, `m3`, `m1`, `#`, `x`, line editing, `i` and `?`.
  The `x`/`m1`/`#` distinctions are all there. The `m0`, Enter-equals-HiZ,
  `m11`, `mx` and incomplete-answer cases are covered better than most docs.
- **What was unclear:** there is no "leave a mode" heading — I found `m1` in
  the menu table, then confirmed it in the reset section. `m1` is described as
  "does not leave BASIC" but `edge-cases.md` says every BASIC line goes to the
  interpreter and `EXIT` must come first; I could not tell whether `m1` is
  runnable in BASIC. The 21-row help table is v4-only but is not captioned, and
  the v3 diff table below it is harder than the help screen itself. The
  `s`/`S` and `o`/`O` case pairs are not called out.

### 3. Value and format layer

- **Where:** `values.md` (value model, `:N`, `;N`, `o`, read letters, `l`/`L`,
  `=`/`|`) → `commands.md` §Transaction wrappers → `usage.md` UART and 2WIRE →
  `macros.md` for a multi-value `WRITE:`.
- **How fast:** about four minutes; the value model is compact and the tables
  answer most questions in one place.
- **Success:** mostly. I could set the format, type decimal/hex/binary/string,
  use `:N` and `;N`, understand that width persists, and see why `l` is
  MSB-first. The README's "display format vs output type" warning is the single
  most useful clarification in the set.
- **What was unclear:** "`rx` followed by a space behaves like a plain `r`"
  contradicts the table that says `rx` is HEX. There is no worked write of a
  real value in a bus mode — the `WRITE:` examples are a parser demo (`12a`),
  a string (`"OK"`) and a macro replay; I assembled the full exchange from
  three pages. The read example uses `{` … `]` while `commands.md` documents
  `}` as the read-stop, so I did not know which close to type. `o1` in the UART
  session is the default and has no effect on `v`, but the sentence reads as if
  it does.

### 4. Known quirks — macros and `$`

- **Where:** `macros.md`; `usage.md` §Bootloader jump; `edge-cases.md` §Macros
  and §Bootloader; `commands.md` §Firmware-level.
- **How fast:** about three minutes; this is the strongest part of the set.
- **Success:** yes. The recall-vs-run distinction is explicit, the example shows
  the body landing on the prompt and waiting for Enter, the body length and
  first-`>` traps are clear, and the v3/v4 persistence split is stated. The `$`
  section warns about the one-byte read, the `no`+Enter trap, the SUMP exit and
  the older v3 loader. I would trust this page before flashing.
- **What was unclear:** `macros.md` sends me to `usage.md` for "a mode session
  with macros", but there is no such session. The `<x>` in the help quote and
  `<n>` in the tables are not explicitly mapped (it is obvious from context, but
  the naming contract is otherwise so careful that the omission stands out).
  `#`/BASIC is contradictory (see finding 1), and the bootloader section does
  not mention BASIC at all.

### 5. End-to-end session through a mode

- **Where:** `usage.md` §Enter UART and §Wrappers and bit ops (2WIRE);
  `navigation.md` for the menu numbers.
- **How fast:** about five minutes to read; the command sequence is short.
- **Success:** **mechanically yes, as a template no.** I can copy the UART
  entry, `W`, `P`, `o1`, `v` and get the pictured flow. I can copy the 2WIRE
  `[ / - _ ^ \ ]` sequence and the `{ r ] r:2` read sequence and follow what
  each line does. But I cannot adapt the 2WIRE setup because `m6 2 1` and
  `R2W (spd hiz)=( 1 1 )` are not decoded; I cannot tell whether `r:2` after
  `]` is a legal unframed read; and the session starts at `HiZ>` after the
  previous session ended at `UART>`.
- **What was unclear:** what the two 2WIRE setup answers mean, what `R2W` is,
  what `spd`/`hiz` values mean, why the start/stop lines say `I2C`, whether
  `P`/Vpullup is needed for the demo (the text says no source, but the command
  sequence never shows a real target), and where the write exchange is. The
  UART session is the better end-to-end example, but it never sends a byte.

### 6. Rendering check

- **Where:** all seven files; raw scan for TABs, trailing whitespace, line
  length, fences, links and anchors; visual check of every capture.
- **How fast:** about ten minutes, mostly automated.
- **Success:** no TAB characters anywhere; no dates, no bench/rig/capture
  narratives, no pass counts, no positive "tested"/"verified" claims, no
  internal log names. The only `verified` strings are negative caveats
  (`not verified`), which the template allows. Every internal link and every
  anchor resolves. The `v` capture columns line up on both boards; the UART
  option list is right-aligned; no prose line exceeds 80 columns. Wide lines
  are only tables and captures, which is acceptable.
- **Problems:** the v3 help diff table (finding 5), the `v` colour legend
  (finding 11), two captures with no prompt (finding 9), the opaque direct-mode
  summary (finding 3), the `rx` sentence (finding 10), the `o1` causation
  (finding 14), and the `P` warning parenthetical (finding 15). I did not find
  a TAB, date, bench-prose, or naming-by-invention problem.

### 7. README navigation test

- **Where:** README link table → `commands.md`; README → `usage.md`; README →
  `SPEC.md`; cross-links from `macros.md`, `values.md` and `README.md`.
- **How fast:** reference tables in one click (`commands.md`), worked sessions
  in one click (`usage.md`), mode numbers in one click (`navigation.md`). The
  link table at the top is the fastest part of the README.
- **Success:** yes for reference and sessions. No for "evidence": there is no
  evidence page (by design), and the only confidence signal is the `not
  verified` caveat. If I wanted proof that the v3 legend or the loader claim is
  right, I had nowhere to go. That is a deliberate boundary, not a broken link,
  but it is the one navigation target the README cannot satisfy.
- **Dead-ends / doubling back:** `macros.md` → `usage.md` "a mode session with
  macros" has no target content. `values.md` → `usage.md` "worked value and
  read sessions" is only half true (the reads are there; the value session is
  not). README's "the elapsed timer … their wiring and limits are on their own
  pages" points at ADC/AUX/self-test but the timer's limit is in
  `commands.md`. The README mode table, the navigation mode table, and the
  README availability table repeat the same v3/v4 deltas; I visited two pages
  for one answer.

## Findings

### Major

1. **[severity: major] `docs/generic/README.md:84` vs `docs/generic/edge-cases.md:88` — the BASIC exit rule contradicts itself.**
   README: "`#` also restores HEX, leaves BASIC and prints the banner; `m1` does none of those and prints `Ready`."
   `edge-cases.md`: "| commands in BASIC | `Syntax error` | every line goes to the interpreter; `EXIT` first, then `#`, `$` or `m` |"
   `usage.md:296-297` also says "and BASIC is left" after `#`, while `usage.md:302-304` says `m1` "does not leave BASIC".
   **Problem:** I cannot tell whether `#` is handled before the BASIC interpreter (and exits it) or is a syntax error like every other line. A reader in BASIC may try `#`, get a syntax error, and conclude the reset is broken; or read `edge-cases.md`, type `EXIT`, and never learn that `#` would have worked. The `m1` wording is equally untestable: if `m1` cannot run in BASIC, "does not leave BASIC" is vacuous.
   **Fix:** pick one behaviour and use it in all three places. If `#` is handled before the interpreter, say "`#` is handled before the interpreter, so it exits BASIC"; remove `EXIT first, then #` from the edge case. If it is not, change README/usage to "`#` is a syntax error in BASIC; type `EXIT` first" and make `m1` say "not available in BASIC" rather than "does not leave BASIC".

2. **[severity: major] `docs/generic/commands.md:149-151` vs `docs/generic/usage.md:237-238` — which stop closes `{`?**
   `commands.md`: "| `{` | start with read | mode-specific |" and "| `}` | stop from read / finish a read | mode-specific |".
   `usage.md`: "`{` is the read-framed start and its matching `]` is the stop." The capture at `usage.md:226-231` is `2WIRE>{` … `2WIRE>]`, and `}` never appears in any worked session.
   **Problem:** following the table, I would type `{ … }`; following the session, I type `{ … ]`. The page that owns the grammar and the page that shows the only read example disagree on the matching close. The `r:2` at `usage.md:232` then runs after `]` with no transaction, which raises the same question from the other side: are reads allowed outside a start/stop?
   **Fix:** make the wrapper table a pair table — `Start | Stop | Meaning | Example` — and state whether `[`/`]` and `{`/`}` are strict pairs, whether `]` also closes `{`, and whether `r`/`r:2` need an active transaction. Then make the 2WIRE session use the documented close; if `]` is valid, say so in the table.

3. **[severity: major] `docs/generic/usage.md:167,188` — direct-mode summary tokens are never decoded.**
   "UART (spd brg dbp sb rxp hiz dup)=( 4 416 0 0 0 1 1 )"
   "R2W (spd hiz)=( 1 1 )"
   The explanation at `usage.md:172-175` only says "The summary prints stored settings, not the menu numbers: the speed and data-bits/parity columns are zero-based."
   **Problem:** the direct form is the set's "skip the prompts" feature, but the summary is the only confirmation the answers took. Six answers produce seven fields (`brg` is derived), `hiz` is the output-type answer, `dup` is duplex, and `R2W` is not the mode name shown anywhere else. A reader cannot verify that `m3 5 1 1 1 1 2` or `m6 2 1` configured what they intended. `brg` is also undefined — baud-rate generator divisor? The `R2W` prefix is especially jarring because the prompt below it is `2WIRE>`.
   **Fix:** add a seven-row token table (`spd` speed index, 0-based; `brg` computed divisor; `dbp` data bits/parity, 0-based; `sb` stop bits; `rxp` receive polarity; `hiz` output type, with the value-to-open-drain mapping; `dup` duplex), say that the six answers expand to seven fields because `brg` is derived, and add a name-note row `2WIRE` → summary prefix `R2W`. Link the 2WIRE page's setup section from `m6 2 1` so the two answers are not magic.

4. **[severity: major] `docs/generic/usage.md:194,206,220` — the 2WIRE mode prints I2C wording and the text calls it 2WIRE's.**
   The prompt is `2WIRE>`, but the start/stop lines are "(\-/_\-)I2C START BIT" and "(\_/-)I2C STOP BIT". Line 220 says "The wrapper characters are shared; the wording is 2WIRE's."
   **Problem:** "the wording is 2WIRE's" is false on the page's own evidence: the firmware strings say `I2C`. A reader in `2WIRE>` who sees `I2C START BIT` will wonder whether they entered the wrong mode, whether 2WIRE is just I2C, or whether the capture is a typo. The README's careful `1WIRE`/`1-Wire` name note does not cover this.
   **Fix:** extend the README Name note (or add a row to the mode table): "`2WIRE` reuses the firmware's `I2C START BIT` / `I2C STOP BIT` strings for its wrappers; the prompt and menu still say `2WIRE`." Replace "the wording is 2WIRE's" with "the 2WIRE firmware displays the shared I2C wrapper wording."

5. **[severity: major] `docs/generic/navigation.md:159-165` — the v3 help diff table can be read backwards.**
   "| output-type row | `#  Reset the BP` where v4 has `o  Set output type` | `o  Set output type` |"
   "| pull-up-source row | `o  Set output type` where v4 has `e  Set Pullup Method` | `e  Set Pullup Method` |"
   **Problem:** the row labels are v4 functions, not neutral row positions. Under "pull-up-source row", the v3 cell is `o  Set output type`, which is the display-format command, not a pull-up source. A v3 reader skimming the table can conclude that v3 `o` sets the pull-up source and that `#` is the output-type command — the opposite of what the rest of the set says. The 21-row help table above it is also v4-only but not captioned, so a v3 reader reads the wrong screen first and gets the correction in this hard-to-parse form.
   **Fix:** give the 21-row table a "v4 help screen" caption, then use a neutral row-number table (`Help row | v4 | v3`) or list the v3 rows in order. Do not label a v3 row by the v4 function that used to occupy that slot.

6. **[severity: major] `docs/SPEC.md:98` vs `docs/generic/commands.md:230` / `docs/generic/edge-cases.md:101` — the SUMP entry count is wrong in SPEC.**
   SPEC: "| entry sequence | x | x | hidden: ≥5× `0x00` then raw STX (`0x02`) | ✓ [protocol/sump/](protocol/sump/README.md) |"
   `commands.md`: "| 5–19 consecutive `0x00` bytes, then a raw `0x02` (STX) | SUMP logic analyzer | …"
   `edge-cases.md`: "| 20 NUL bytes then `0x02` | enters binary bitbang (`BBIO1`), not SUMP | …"
   **Problem:** `≥5` includes 20, but the generic set says 20 NULs already switches to the binary layer, so the trailing `0x02` is a binary command, not a SUMP entry. A reader who starts from the capability matrix (which the generic README calls the capability index) can enter the wrong layer.
   **Fix:** change SPEC to "5–19× `0x00` then raw STX (`0x02`)" and, if the matrix must stay short, add "20 enters binary, not SUMP."

7. **[severity: major] `docs/generic/navigation.md:139,151` and `docs/generic/commands.md:194` — case pairs with unrelated meanings are not called out.**
   Help: "| `s` | Script engine | `:` | Repeat e.g. r:10 |" and "| `p/P` | Pullup resistors (off/ON) | `!` | Bit read |".
   `commands.md`: "| `O` | 1-Wire standard/overdrive toggle, in place.  Outside `1WIRE` it prints `Command not used in this mode`. |"
   **Problem:** the set uses case for many related pairs (`p`/`P`, `w`/`W`, `l`/`L`, `d`/`D`, `t`/`T`), but `s`/`S` are unrelated (BASIC vs servo) and `o`/`O` are unrelated (display format vs 1-Wire speed). There is no "case matters" statement. A novice who types `S` for BASIC can start a servo on the AUX pin; a reader who types `O` for output format toggles 1-Wire speed. The help table shows the commands but not the trap.
   **Fix:** add one short "Case matters" note in `navigation.md` or `commands.md`: `s` BASIC vs `S` servo; `o` display format vs `O` 1-Wire speed; `p/P`, `w/W`, `l/L`, `d/D`, `t/T` are related on/off or read/reset pairs. Add `O` to the "working commands with no help row" list at `navigation.md:153`.

### Minor

8. **[severity: minor] `docs/generic/README.md:3-4` — "v3 is build-only" is undefined.**
   "**Status:** current · the shared command layer.  v4 is the documented board; v3 is build-only and not verified on v3 hardware."
   **Problem:** I read "build-only" as "only builds, not usable/documented", but the mode table says v3 `yes` and every detail page has v3 deltas. The same jargon is in `SPEC.md:13` ("v3 is build-only"). A v3 owner may stop at the first paragraph. The README also never says the captures are v4, so a v3 reader has to infer it from the 12-column `v` table.
   **Fix:** "v4 is the primary documented board; the v3 build is covered but has not been verified on v3 hardware. Captures in this set are v4; inline notes and `navigation.md` §v3/v4 delta list the differences." Define "build-only" once if it must be kept.

9. **[severity: minor] `docs/generic/usage.md:110-116` and `241-247` — sessions start at a prompt the previous session did not leave.**
   "## Mode change and clean exit — menu form" / "Wiring: unchanged." / "HiZ>m" — the UART session ended at `UART>` (`usage.md:94`).
   "## `v` in HiZ" / "v" — the 2WIRE session ended at `2WIRE>` (`usage.md:234`), and the capture has no prompt at all.
   **Problem:** following the page linearly, I am at the wrong prompt; "Wiring: unchanged" implies continuity, so I expected `UART>m`, not `HiZ>m`. The `v` capture starts with a bare `v`, so I cannot see the starting mode. The `#` capture (`usage.md:282-284`) also has no prompt.
   **Fix:** add `Starting point: HiZ>` (or `m1` first) to every session. If the sessions are meant to be independent, say so once at the top and still show the prompt. Add `HiZ>` before `v` and `#` so the captures are self-contained.

10. **[severity: minor] `docs/generic/values.md:164-165` — "`rx` followed by a space behaves like a plain `r`."**
    The section defines `rx` as HEX, then says "`rW` performs one read and then turns the supplies on; `rx` followed by a space behaves like a plain `r`."
    **Problem:** this is a second rule, not an example of "the letter is immediate": it says the `x` is ignored when a space follows. I cannot tell whether `rx ` is HEX (the table) or the `o` format (this sentence). It is probably a typo for "`r` followed by a space".
    **Fix:** correct to "`r` followed by a space behaves like a plain `r`", or show the exact `rx ` input/reply and explain why the letter is dropped.

11. **[severity: minor] `docs/generic/usage.md:90,264,275-276` — the `v` colour legend is never expanded.**
    "12.(Bk)  11.(Wt)  10.(Gy)  9.(Pu)   8.(Bu)   7.(Gn)   6.(Yw)   5.(Or)   4.(Rd)   3.(Br)   2.(Bk)   1.(Wt)"
    "| 1 | pin number and colour legend |"
    "Its legend runs `1.(Br)` through `0.(Bk)`."
    **Problem:** `Bk`, `Wt`, `Gy`, `Pu`, `Bu`, `Gn`, `Yw`, `Or`, `Rd`, `Br` are not decoded. `Pu` and `Yw` are not the usual resistor-colour abbreviations, so I cannot be sure whether they mean purple/violet and yellow. On v3 there are ten columns but the legend ends at `0.(Bk)`, so I cannot map the header to pin numbers. The page calls it a "colour legend" but never gives the legend.
    **Fix:** add a small 10-row legend table (`Bk` black, `Wt` white, `Gy` grey, `Pu` purple/violet, `Bu` blue, `Gn` green, `Yw` yellow, `Or` orange, `Rd` red, `Br` brown) and a footnote for the v3 `0`/pin-10 label (or correct it if `0` is a typo).

12. **[severity: minor] `docs/generic/commands.md:219-222` — the HiZ command list leaves `c/C/k/K` and `&/%` unclassified.**
    "The AUX pin is the exception: `a` / `A` drive it and `@` reads it in HiZ.  `i`, `?`, `m`, `h`, `b`, `o`, `v`, `t` / `T`, `d` / `D`, `f`, `l` / `L`, `=` / `|`, `<n>`, `s`, `#` and `$` also work from HiZ (with their own limits)."
    **Problem:** `c/C/k/K` choose which pin `a/A/@` act on; if they work in HiZ, they belong in this list; if they are refused, the page should say so. `edge-cases.md:18` says "`a`/`A` in HiZ … the selected AUX pin is driven", which implies selection is possible. `&`/`%` delays are also neither listed as working nor listed as refused.
    **Fix:** add `c`/`C` (and v4 `k`/`K`) and `&`/`%` to the working list, or state explicitly that the HiZ target is fixed at `A0`/`AUX` and target selection is refused; do the same for delays.

13. **[severity: minor] `docs/generic/macros.md:91` — "a mode session with macros" does not exist.**
    "- [usage.md](usage.md) — a mode session with macros."
    **Problem:** `usage.md` has no macro section or macro example; the only example is on `macros.md` itself. The link resolves, so it is not a broken anchor, but it is a dead end for the reader who follows it.
    **Fix:** add a short macro session to `usage.md` (the UART example is the natural place), or change the link text to "the recall example above" and drop the usage claim.

14. **[severity: minor] `docs/generic/usage.md:102` — "`o1` selects HEX; `v` then shows the UART signal names."**
    **Problem:** `o1` is the default HEX (`values.md:126`), and the display format does not apply to `v` (`values.md:128-129`). The sentence reads as causation — as if `o1` is needed to make `v` show mode names — but `v` would show them anyway, and its voltages stay decimal. A reader may think `o1` is a required step for `v`.
    **Fix:** "`o1` re-selects the default HEX (no change); `v` is independent and always shows the mode's signal names." Or use `o2`/`o3` if the session is meant to demonstrate a change.

15. **[severity: minor] `docs/generic/edge-cases.md:37` — "`WARNING: pins not open drain (HiZ)`" in a push-pull mode.**
    "| `P` in a push-pull mode | `WARNING: pins not open drain (HiZ)`, then `Pull-up resistors ON` | informational; the pull-ups still switch on.  Choose open drain in the mode setup |"
    **Problem:** the message says the pins are *not* open drain, then labels them `(HiZ)`. In a push-pull mode, a reader may think the board is in HiZ (or that the warning contradicts the mode). The doc quotes the string exactly but does not explain the parenthetical.
    **Fix:** add a short note: "`(HiZ)` in this message is fixed firmware text for the high state the command expects, not the current mode." The same string appears in `commands.md:114`; fix both or link.

16. **[severity: minor] `docs/generic/navigation.md:205-206` and `docs/generic/usage.md:160` — "setup" means both the entry prompts and the W engage routine.**
    `navigation.md`: "`W` starts the supplies and runs the mode's setup; the mode's page states what it drives and when (DIO, for example, writes pins without `W`)."
    `usage.md`: "the supplies and runs the mode's setup."
    **Problem:** earlier, "setup" is the chain of questions `m3` asks (`usage.md:33`); here `W` "runs the mode's setup", which sounds like the prompts run again. They do not. A novice may wait for prompts or think `W` repeats the configuration.
    **Fix:** reserve "setup prompts" for entry and call the `W` action the "engage" or "bus setup" routine; then the `Clutch engaged!!!` line makes sense.

17. **[severity: minor] `docs/generic/navigation.md:232-236` — the raw BRG option has no range or meaning.**
    "| 10 | `BRG raw value` | … Option 10 asks `Enter raw value for BRG` with the prompt `(34)>`, then applies the value.  A complete `b10 <value>` skips that prompt.  The default is 9 (115200), the rate the firmware boots at."
    **Problem:** `(34)>` is unexplained (current divisor? default?), and there is no accepted range or unit. A reader who wants a non-standard rate cannot use option 10 safely. The UART summary's `brg 416` uses the same undefined term.
    **Fix:** state that the parenthesized value is the current BRG (34 at 115200) and give the accepted range, or link the UART page's BRG description. Define BRG once as the baud-rate generator divisor.

18. **[severity: minor] `docs/SPEC.md:113` — "SPI / I2C / UART / 1W01 / RAW sub-protocols".**
    **Problem:** `1W01` is not a name used anywhere else; the menu says `1WIRE` and the prose says `1-Wire`. It reads as a typo and breaks the naming contract the rest of the set follows.
    **Fix:** `1WIRE` (or `1-Wire` with the same mapping note as the generic README).

## What to keep

- **The AUX target table** (`commands.md:124-134`) is the best thing in the set. `A0`/`AUX`, `CS`, `A1`/`AUX1`, `A2`/`AUX2`, the select commands, the confirmation strings and the `v` table names are all in one place, plus the warning that the drive messages still say `AUX`. I never had to guess which pin I was on.
- **The `$` section** is the model for a bootloader warning: one-byte read, `no`+Enter trap, SUMP exit, v3 loader caveat, port re-enumeration. I would trust it before flashing.
- **The macro recall-vs-run example** made the distinction click: `<1=0x55:3>`, `<0>`, `<1>`, then the body on the prompt line waiting for Enter. The body-length and first-`>` traps are practical, not theoretical.
- **The `v` row-by-row explanation and the `P` vs `v` sampling story** (warning below about 0.5 V, later stray 2.19 V on a floating pin) turned a confusing table into something I could read. That is exactly the kind of "why do these two readings disagree?" note a human needs.
- **The `m` menu table** covers the cases I would actually hit: `m0`, Enter=HiZ, `x`, `m11`, `mx`, incomplete inline answers, and no `Clutch` line on the direct form. It is more complete than most mode-menu docs.
- **The firmware-misnomer notes** — `o` vs output type, `PSU` vs `POWER SUPPLIES`, `s`/Script engine, `Set Pullup Method` — are the right kind of translation. They tell me what to type, not just what the label says.
- **The edge-cases trigger→symptom→recovery format** is a good lookup. `h5`, `12a`, `-5`, space-before-`:`/`;`, width persistence, macro `>` and the HiZ `(0)` blank line are all traps I would have hit.
- **The honesty about examples:** "nothing connected", "one possible result, not a device value", "your values will differ". The set does not invent a target or a passing capture to make the 2WIRE demo look better.
- **The mechanical consistency:** no TABs, no dates, no bench/test-result prose, no internal log names; every link and anchor I tried resolves; the `v` columns line up.

## Structural opinion

The six-file split is the right shape for this function: `navigation` owns the prompt,
`values` owns the value model, `macros` owns both macro families, `commands` owns
the command/wiring tables, `usage` owns the sessions, `edge-cases` owns the
trigger lookup. The README is a genuine map, and the link table at the top is the
fastest way into the set.

The weakness is **duplication of cross-cutting behaviour**. Reset appears in
README, navigation, values, commands, usage and edge-cases; format persistence
appears in README, navigation, values and usage; the `o` misnomer appears in
README, navigation, values, commands and edge-cases. That is where the
BASIC/`#` contradiction and the 2WIRE wording drift come from: each page has its
own sentence, and they are not all the same sentence. Concrete proposal:

- **One owner per behaviour.** `usage.md` §Reset owns the full reset story;
  `values.md` owns format/width/bit-order persistence; `navigation.md` owns the
  prompt/menu/help; `commands.md` owns the command/wiring/name tables;
  `macros.md` owns macro persistence; `edge-cases.md` is a trigger table that
  links to the owner. The other pages keep one-line summaries, not full
  restatements. This would have caught finding 1 mechanically.
- **Move the README's `Idle mode (HiZ)` section to `navigation.md`** (or a
  `hiz.md`). The README is 159 lines against the set's own ~120-line target, and
  the idle-mode detail is the largest block that is not part of the map. The
  README can keep the glossary and the 10-row default menu; the full HiZ reset
  belongs with the prompt.
- **Add a three-line "Start here" path** above the five-minute version:
  `i` (first contact) → `?` (what exists) → `m` (choose a mode), with links to
  `usage.md#first-contact` and `navigation.md#mode-menu`. The current
  five-minute version is a command list, not a sequence; a novice still has to
  guess which page to open first.
- **Add a "v3 readers" box to the README:** captures are v4; v3 deltas are
  inline and in `navigation.md` §v3/v4 delta; v3 hardware is unverified. This
  fixes finding 8 without repeating the delta table.
- **Make the worked sessions independently runnable:** each session gets
  `Starting point: HiZ>` (or the current prompt), a wiring line, and an
  explicit "if you are following from the previous session, run `m1` first".
  The sessions are the set's main feature; they should not require the reader to
  reconstruct the starting mode.
- **Centralise the device-string names.** The AUX table is the model; extend the
  same idea to `1WIRE`/`1-Wire`, `2WIRE`/`2-Wire`/`R2W`/`I2C START BIT`,
  `VPU`/`Vpullup`/`Vpu`, `PSU`/`POWER SUPPLIES`, `o`/display format, `s`/BASIC
  vs `S`/servo, `o`/format vs `O`/1-Wire. One table in `README.md` or
  `commands.md`, with a "firmware displays X, this set writes Y" column, would
  remove findings 3, 4, 7 and 11.
- **Give the v3 help screen its own table.** The v3/v4 diff table is too clever
  by half; neutral row numbers or a straight v3 list would be clearer than
  "the row where v4 has X". The 21-row table above it should be captioned
  "v4 help screen" before a v3 reader starts reading it.
- **Make the wrapper table a pair table.** `Start | Stop | Meaning | Example`
  would expose the `{`/`}`/`]` question instead of hiding it in four
  independent rows. It is the only table where the reader has to infer a pair
  from row order.

## Unknowns

- **Which side of each contradiction is true.** I cannot verify on hardware or
  against firmware source whether `#` is handled before the BASIC interpreter,
  whether `}` or `]` closes `{`, whether `rx ` is HEX or plain `r`, or whether
  `hiz=1` means open drain. The docs need a source/hardware check, not a reader
  vote. I have stated the conflict, not guessed the winner.
- **The `P` warning threshold.** The "below about 0.5 V" figure and the "samples
  once, right after switching on" explanation are plausible but not something I
  can confirm from the page; they may be right and still deserve a source label
  if they are approximate.
- **The v3 legend `0.(Bk)`.** I cannot tell whether the firmware prints `0` for
  pin 10, whether the transcription truncated `10`, or whether the legend is
  ordered differently. This needs a v3 capture or a code check.
- **The 4.5 loader claim.** The v3 bootloader section names a specific loader
  version without a date or source. I cannot judge whether it is current; the
  caveat "not verified on v3 hardware" is honest, but the version number itself
  is unverified.
- **The v3 build counts** (9 modes, 7 history entries, 3 macro slots, 128 B
  buffer) are presented as build facts; I did not have a v3 build to check them.
- **The 2WIRE example's `0x00` reads** are explicitly "one possible result, not
  a device value"; I cannot judge whether a real target would read differently,
  and the page does not claim otherwise.
- **The `1W01` spelling in SPEC** may be a deliberate internal abbreviation I do
  not know. I treated it as a typo because no other page uses it.
- **Scope.** I did not read the audit artifacts under `docs-sweep/`, other doc
  sets, or the firmware source; this is a reader review of `docs/generic/`,
  `docs/SPEC.md` where it is linked, and `docs/TEMPLATE.md` for context. I did
  not run the firmware or connect a board.
