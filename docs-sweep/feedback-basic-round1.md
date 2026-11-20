# BASIC docs — round 1 reader feedback

## Verdict

**Yes with fixes.** The set is usable and unusually strong on mode reset and HiZ safety, but a v3 user or an I2C user can be actively misled by a handful of factual contradictions (NUL/SUMP boundary, `AUXPIN` on v3, `TIME` units, the I2C example's own workaround, SMPS ownership). None is a blocker for a careful reader; several will stop the recipe that hits them.

## Task log

### 1. First impression from README alone

- **Where I looked:** README only, one pass (~3 min).
- **How fast:** under a minute to answer "what is it"; ~3 min to decide whether it applies to my board.
- **Success:** yes. I could say what BASIC is (on-board interpreter entered with `s`), that it is in the v3 and v4 images, that v3 is built but not verified, where to start (five-minute bullets), and the big limits.
- **Unclear:** "build-only" is jargon. The limits list stopped at no arrays / no strings / no `STEP`; it did not warn me that common BASIC things like `ABS`, `INT`, `RND`, `AND`, `OR` are absent, or that the terminal's frequency counter `f` has no BASIC equivalent while `FREQ` means PWM output. I found those later.

### 2. Write and run a small program

- **Where I looked:** README five-minute version → `entry-and-prompt.md` → `examples.md` §Hello.
- **How fast:** ~5 min.
- **Success:** yes. `10 PRINT "HI"`, `20 END`, `LIST`, `RUN`, retype line 10, `EXIT` all worked on paper.
- **Clear:** line-number model; replace/delete; one statement per line; `Ready` for each handled line but not for `s`; retype-whole-line editing.
- **Unclear:** the `15 bytes.` / "16-byte program" explanation. I read it three times. The breakdown (line 10, line 20, marker) is not shown, so "15 is a 16-byte program" looks like an arithmetic mistake. The `65535 END` marker is quoted with one space in prose and two in the listing, so I was not sure which string is real.

### 3. Use the language

- **Where I looked:** `language.md`, `examples.md`, `edge-cases.md`.
- **How fast:** ~8–10 min.
- **Success:** yes for expressions, `IF`, `GOSUB`, `FOR`/`NEXT`, `DATA`/`READ`/`RESTORE`, `PRINT`/`INPUT`.
- **Clear:** the operator precedence is honestly weird but explicitly explained; the `PRINT 1,2` trap; the two `INPUT` forms; `DATA` scan order; `NEXT` must name its variable; the loop/call limits.
- **Unclear:** no worked `GOTO` example (only the rule); no compound-condition idiom (no `AND`/`OR`, and `&`/`|` are bitwise and bind with `*`); `PRINT` unsigned vs `DATA -1` signed is split across pages.
- **Error table:** easy to find. It is compact and useful, but it is run-time-only. I had to infer that typing `Syntax error` and running `Error(3)` are two different error surfaces. Code 1 is absent; code 2's wording is narrower than the `INPUT A, B` example; code 3 covers far more triggers than its line says.

### 4. Use the bus

- **Where I looked:** `bus-access.md`, `examples.md` §I2C, `board-pins.md` wiring table, `generic/commands.md`, `generic/usage.md`.
- **How fast:** ~8 min.
- **Success:** mostly. The mode-number table and the two matrices are the fastest part of the set.
- **Clear:** `MODE` reset list; mode numbering; `SEND`/`RECEIVE` vs `READ`; `START`/`STOP` mapping; `STARTR` write-with-read; `STOPR` same as `STOP`; 1WIRE `DAT`/`BITREAD` special case; silent no-ops.
- **Unclear:** the reset contract never says `MODE` also runs the mode's engage/setup hook (the thing `W` runs), so I kept asking "do I need `PSU 1` before `START`?" The I2C example says avoid `MODE` if you use the terminal setup, but the example program itself begins with `MODE 4`. `MACRO` can hang, and the recovery says "reset the board" without saying that `#` is a syntax error at the BASIC prompt. The UART row says `START`/`STOP` "yes", but in UART those keys start/stop the live display and have no wire effect.

### 5. Use the board pins

- **Where I looked:** `board-pins.md`, `examples.md` §Supply / PULLUP / AUX / PWM / Timed pause, `capabilities/auxiliary/pwm-servo.md`, `capabilities/adc/README.md`.
- **How fast:** ~7 min.
- **Success:** yes. `PSU`, `PULLUP`, `AUX`/`AUXPIN`, `FREQ`/`DUTY`, `ADC`, `TIME`, `DELAY` are easy to find; PULLUP's inverted expression is impossible to miss.
- **Clear:** PULLUP inversion and the `IF PULLUP=0` idiom; AUX read releases the pin; PWM always drives AUX regardless of `AUXPIN`; ADC raw formula; `TIME` wrap; `DELAY` unsigned.
- **Unclear / risky:** the HiZ cleanup line offers `AUX 0` as if it were a safe release; it drives the selected pin low. `PSU 1` is described as the `W` switch without checks, but the page does not repeat "do not back-drive a self-powered target." The SMPS sentences conflict with the SMPS/ADC and PWM pages. The "see pwm-servo" link does not cover BASIC's 1–4000 Hz range.

### 6. Rendering check

- **Scanned:** all 8 files in `docs/tools/basic/`, plus the linked generic / AUX / ADC / binary / SUMP pages.
- **Tabs:** none.
- **Bench/provenance:** no dates, bench/rig/battery narratives, pass counts, capture provenance, or internal log names. The only provenance is the `not verified` / `unverified` caveat, repeated in six places.
- **Captures:** readable; no misaligned columns. One prose line is over the ~80-column target: `board-pins.md:162`, 82 characters.
- **Dense bullet chain:** `board-pins.md` §PWM is 10 bullets; it would scan much faster as a table (range, clamp, separators, storage, output pin, units, accuracy, SMPS).
- **Naming/format mismatches:** `65535 END` vs `65535  END`; `mN` vs `m<N>`; `2-Wire` vs `2WIRE`; `TIME` vs terminal `t` units; `SEND` row showing `"text"` while `SEND "A"` is rejected. Details in Findings.

### 7. README navigation test

- **Language:** one click (row 2), ~5 s. **Bus matrix:** one click (row 3) then scroll to §Protocol operations, ~15 s; there is no direct anchor from README. **Board pins:** one click, ~5 s; sections are in task order. **Examples:** one click, but 386 lines with no task index; finding the I2C or PWM session means scanning headings, ~10 s if I know the heading and longer if I do not.
- **Dead-ends / doubling back:** README says "Full tables" but points at two summary tables; `bus-access.md` says "mode sets linked from the README" but the BASIC README does not link the mode pages; `board-pins.md` says "generic supply page" but links the generic README, which is a prompt map, not a supply page. I doubled back from examples to bus-access to check the UART matrix, and from `language.md` §Errors to `edge-cases.md` for the concrete triggers.

## Findings

1. **[severity: major]** `docs/tools/basic/entry-and-prompt.md:199` — "| five or more NULs, then the Left-arrow key (`0x02`) | enter the logic analyzer (SUMP) |". **Problem:** 20 NULs enter binary; the SUMP window is 5–19, and `0x02` after 20 NULs selects binary I2C. A reader following this table can land in the wrong layer. **Fix:** "five to nineteen NULs"; repeat the boundary in `edge-cases.md:35`, which currently says "20 NULs, or NULs plus Left arrow".

2. **[severity: major]** `docs/tools/basic/bus-access.md:131` — "| `TIME` (expression) | `t` | elapsed timer, 64 µs units |", with `board-pins.md:145` — "the shared microsecond timer divided by 64, so one unit is **64 µs**." **Problem:** the terminal `t` prints microseconds (`generic/commands.md:94`: "plain decimal microseconds, no unit"), not 64 µs units. A user mapping `t` to `TIME` will be off by 64×, and the wraps differ (35.8 min vs 4.19 s). **Fix:** state both in one line: "terminal `t` = 1 µs, 31-bit; BASIC `TIME` = `t`/64, 16-bit, 4.19 s wrap."

3. **[severity: major]** `docs/tools/basic/examples.md:247` — "HiZ(BASIC)> 10 MODE 4" … `examples.md:270-272` — "Either feed Vpullup externally, or complete the I2C setup at the terminal … enter `s`, and do not use `MODE` in the program." **Problem:** the example program itself uses `MODE 4`; the second workaround only works if the reader deletes line 10, which is never said. On a v4 with no external Vpullup, following the shown program clears the on-board VPU source and the pull-ups have nothing to tie to. **Fix:** show two program variants ("with `MODE` + external Vpullup" and "terminal setup, no `MODE` line"), or add an explicit "delete line 10 for this path."

4. **[severity: major]** `docs/tools/basic/board-pins.md:191` — "add `PSU 0`, `PULLUP 0`, `AUX 0` or `FREQ 0 50` as needed." **Problem:** `AUX 0` drives the selected AUX target low (`board-pins.md:76`: "`AUX 0` and `AUX 1` drive the selected target low and high"); it does not release it. The read form releases (`board-pins.md:77-79`: "The read sets the pin to an input first, so it **releases** the pin"). Listing `AUX 0` beside `PSU 0`/`PULLUP 0` reads like a safe-off recipe and can hold a connected circuit low. **Fix:** separate the two ideas: "`PSU 0`, `PULLUP 0`, `FREQ 0 50` de-energize; to release AUX, read it (`PRINT AUX;`) or reset. `AUX 0` is a drive-low, not a release."

5. **[severity: major]** `docs/tools/basic/board-pins.md:134-135` — "When the optional SMPS owns the ADC, `ADC` stops the program with `Error(3)` instead of taking a sample." **Problem:** the SMPS page says the opposite for the probe: `capabilities/smps/edge-cases.md:84-90` — "run `d`/`D` … while the SMPS is running … the reading follows the SMPS feedback, not a source connected to the `ADC` pin". BASIC may genuinely differ, but the docs do not say that; a reader cannot tell whether the token errors or returns the SMPS feedback. **Fix:** say "BASIC checks the SMPS flag and raises `Error(3)`; the terminal `d` behaves as the ADC page describes" and link the SMPS page, or mark the BASIC behavior unverified.

6. **[severity: major]** `docs/tools/basic/board-pins.md:120-121` — "If SMPS owns the channel, `FREQ`/`DUTY` do nothing and report no error." **Problem:** `capabilities/auxiliary/pwm-servo.md:190-194` says the SMPS shares the output-compare channel and "each reprograms the channel"; it does not say a terminal generator is silently ignored. If BASIC `FREQ` actually reprograms OC5, the "do nothing" promise is wrong and can disturb the SMPS. **Fix:** confirm and state the BASIC-specific guard, or say "unverified; do not run both."

7. **[severity: major]** `docs/tools/basic/bus-access.md:263` — "An `AUXPIN` value of 2 or 3 is a syntax error on v3." vs `board-pins.md:80-82` — "`AUXPIN` out of range stops the program with `Error(3)` … v3 accepts only 0 and 1; 2 and 3 are syntax errors there." vs `edge-cases.md:112` — "| `AUXPIN 2` on v3 | `Error(3)` |". **Problem:** same trigger, two different symptoms: syntax error or `Error(3)`, with `board-pins.md` saying both in the same bullet. A v3 user debugging this has no single answer. **Fix:** pick the observed behavior; if 2/3 are rejected at parse time on v3, say "syntax error (not `Error(3)`)" everywhere and correct edge-cases; if runtime, correct bus-access/board-pins.

8. **[severity: major]** `docs/tools/basic/bus-access.md:47-48` — "`MODE` runs the previous mode's cleanup, the shared board-state reset, and then the new mode's setup with every prompt taking its default answer." and `:62-63` — "mode settings | the new mode's defaults … supplies | **not** turned on; `MODE` does not run `W`'s voltage checks or messages". **Problem:** the table never says `MODE` also runs the mode's engage/setup-execute hook—the thing `W` runs. A terminal-trained reader asks "do I need `PSU 1` before `START`?" and the page both says "order `MODE`, then `PSU`" and shows an I2C program that never calls `PSU`. **Fix:** add a row: "bus engagement | the mode's setup/engage routine runs (the `W` hook), without `W`'s supply checks; supplies remain off; no `Clutch engaged!!!`." Then say explicitly whether `PSU 1` is needed for a self-powered target.

9. **[severity: major]** `docs/tools/basic/language.md:284-305` — "A run that stops on an error prints:" … table 2..10. **Problem:** this is only the run-time error surface. While typing, the same user sees `Syntax error` or `Too long!` (`entry-and-prompt.md:38-40`, `:68`, `edge-cases.md:12`); there is no explanation that `Syntax error` at entry and `Error(3)` at run are different presentations of a bad line. Also code 1 is absent, code 2 says "after a statement that takes no argument" while `edge-cases.md:75` shows `INPUT A, B` → `Error(2)` (INPUT takes an argument), and code 3's one-line description covers only a fraction of its triggers. **Fix:** open §Errors with a two-row "where errors appear" table (entry vs run), then broaden 2/3 and say whether 1 is unused.

10. **[severity: major]** `docs/tools/basic/board-pins.md:37-39` — "Wiring: the regulator feeds the board's `5.0V` / `3.3V` supply pins; the target's ground must be common.  A self-powered target does not need it." **Problem:** `PSU 1` deliberately skips `W`'s checks (`board-pins.md:33-35`), and the page never repeats the generic warning "do not back-drive the rails" (`generic/commands.md:17`). A self-powered target plus `PSU 1` is exactly the back-drive case. **Fix:** add "Do not enable `PSU` into a self-powered target; `PSU 1` bypasses `W`'s fault checks."

11. **[severity: major]** `docs/tools/basic/README.md:40-42` — "The board-level set is `PSU`, `PULLUP`, `AUX`, `AUXPIN`, `FREQ`, `DUTY`, `DELAY`, `ADC` and `TIME`." **Problem:** the terminal's frequency counter is `f`; in BASIC `FREQ` is the PWM output (`board-pins.md:92-96`). The set never says there is no frequency-counter token, so a reader can reasonably try `PRINT FREQ;` expecting a measurement. **Fix:** call it "PWM `FREQ`" in the list and add a limits line: "terminal `f` (frequency counter), `v`, `~`, `O`, `o` have no BASIC equivalents; use them before `s` or after `EXIT`."

12. **[severity: minor]** `docs/tools/basic/bus-access.md:119` — "| `SEND value` | value entry (`123`, `0x7B`, `0b1111011`, `"text"`) | send a value |" vs `:202-203` — "`SEND` takes a number, not a string; `SEND "A"` is a syntax error." **Problem:** the mapping row includes the terminal string form and hex/binary forms, but BASIC is decimal-only (`README.md:82`). A reader scanning the table may try `SEND 0x7B` or `SEND "A"`. **Fix:** mark the terminal-only forms, or add "BASIC: decimal number only" in the row.

13. **[severity: minor]** `docs/tools/basic/README.md:79-81` — "No arrays, no string variables or string expressions … no user-defined functions, no `STEP`, no multi-statement lines." **Problem:** the "what can't it do" list omits the two things a BASIC user tries first: no built-in numeric functions (`ABS`, `INT`, `RND`) and no `AND`/`OR` keywords (both only in `limits-and-variants.md:75-76,59`). **Fix:** add "no `ABS`/`INT`/`RND`, no `AND`/`OR`" to the README limits bullet; leave the details in limits.

14. **[severity: minor]** `docs/tools/basic/README.md:73-75` — "Full tables: [bus-access.md §Wiring per mode] … and [board-pins.md §Pins and rails]." **Problem:** those are summary tables, not full per-mode pin tables; the real mode pages are not linked. `bus-access.md:256-257` compounds it: "Details and the full pin tables are in the mode sets linked from the README"—the BASIC README's Related docs table does not link the mode sets. **Fix:** say "summary tables" and add the mode-set links (or point at `generic/README.md`'s mode table, which does link them).

15. **[severity: minor]** `docs/tools/basic/entry-and-prompt.md:130` — "the four-byte `65535 END` marker" and `examples.md:34` — "the `65535  END` line", while the capture at `entry-and-prompt.md:116` prints `65535  END` (two spaces). **Problem:** the same displayed line is quoted with one and two spaces, so a reader cannot tell whether the second space is real firmware output or prose formatting. **Fix:** quote the two-space form everywhere, or add "(shown with two spaces)" once.

16. **[severity: minor]** `docs/tools/basic/bus-access.md:116` — "| `MODE` / `MODE n` | `m` / `mN` | report / change the mode |" and `entry-and-prompt.md:178` — "change mode with `m` or `mN`". **Problem:** the rest of the docs (`generic/navigation.md:202`) write the command as `m<N>` / `m N`; `mN` reads like a literal token. **Fix:** use `m<N>` or `m N` consistently.

17. **[severity: minor]** `docs/tools/basic/examples.md:280` — "A 2-Wire program has the same pull-up and `MODE`-reset caveat". **Problem:** every matrix and mode number says `2WIRE`; the prose form appears with no local mapping (the mapping is back in `bus-access.md:106-107`). A reader landing on examples first may not connect `2-Wire` to `MODE 6`. **Fix:** write `2WIRE` (or "2-Wire (`2WIRE`)") in the example.

18. **[severity: minor]** `docs/tools/basic/README.md:3-4`, `bus-access.md:263-264`, `board-pins.md:131-132`, `limits-and-variants.md:101/104`, `edge-cases.md:139-140` — the same v3 "not verified" caveat six times. **Problem:** it is honest, but repeated in every section it stops reading as a caveat and starts reading as boilerplate; it also makes the pages longer than they need to be. **Fix:** state it once in the README status and once in limits (with the ADC divider), then drop the repeats; keep "unverified" only where a specific number could be wrong.

19. **[severity: minor]** `docs/tools/basic/board-pins.md:162` — "`RUN` prints the elapsed time in 64 µs units — about 1562 for the 100 ms delay." **Problem:** 82 columns, just over the ~80-column prose target; the only such line in the set. **Fix:** wrap after "units —".

20. **[severity: minor]** `docs/tools/basic/board-pins.md:90-123` — the `## PWM: FREQ and DUTY` section is a 12-bullet chain covering range, clamping, expression separators, storage format, output pin, units, accuracy and SMPS. **Problem:** the reader has to hold ten different rules in a list; a table would let them scan. **Fix:** a table with rows Range / Clamping / Separators / Stored form / Output / Units / Accuracy / SMPS, keeping the two examples.

21. **[severity: minor]** `docs/tools/basic/examples.md:1-6` — "Each example shows the program and the replies it produces." **Problem:** the page has 12 H2 sections but no task index, and it has no `GOTO`, `CLK`/`DAT`/`STOPR`, or `MACRO` example; those are exactly the bus/language operations a reader may come looking for after the reference pages. **Fix:** add a "Find an example by task" table at the top, and add a 3-line `GOTO` loop plus a short 2WIRE/3WIRE bit-bang or `MACRO` session.

22. **[severity: minor]** `docs/tools/basic/language.md:297` — "partial instruction — trailing text after a statement that takes no argument, or a corrupt stored line". **Problem:** `INPUT A, B` is `Error(2)` (`edge-cases.md:75`) even though `INPUT` takes an argument; the description is too narrow. **Fix:** "trailing text after a complete statement (for example `START 1` or `INPUT A, B`)." Also explain the table starts at 2: is code 1 unused/reserved?

23. **[severity: minor]** `docs/tools/basic/board-pins.md:116-119` — "The actual frequency and duty are approximated by whole timer ticks; the output can differ slightly from the request.  The timer, prescaler and accuracy details are in [pwm-servo.md]." **Problem:** the linked page's prescaler table starts at 1–3 kHz and its examples are in kHz/MHz; BASIC's range is 1–4000 Hz. A reader setting 10 Hz or 200 Hz gets no accuracy guidance from the link. **Fix:** add a short BASIC-range note (period counts at 1 Hz / 100 Hz / 4 kHz) or extend the linked page's table down to 1 Hz.

24. **[severity: minor]** `docs/tools/basic/bus-access.md:176` — "Reset the board to recover from a macro that waits for input." and `edge-cases.md:115` — "reset the board". **Problem:** at the BASIC prompt `#` is a syntax error (`entry-and-prompt.md:38-40`); if the program is already stuck in the macro, `EXIT` is also unavailable. The only recovery is a physical reset/power cycle, but the page does not say so. **Fix:** "press the board's reset button or power-cycle; `#` cannot be typed while the macro holds the interpreter."

25. **[severity: minor]** `docs/tools/basic/entry-and-prompt.md:128-130` — "The number before `bytes.` is the number of stored bytes before the final terminator.  The sample's `15` is a 16-byte program: two stored lines plus the four-byte `65535 END` marker." **Problem:** the arithmetic is correct but the presentation makes it look wrong; "15 is a 16-byte program" needs the breakdown. **Fix:** show "line 10 = 8 bytes, line 20 = 4 bytes, marker = 4 bytes, total 16; `LIST` shows 15 because it omits the final 0x00" once, then cross-ref from limits.

26. **[severity: minor]** `docs/tools/basic/bus-access.md:156` — "| UART | yes | yes | yes | yes | yes |" (START/STOP/SEND/RECEIVE/MACRO). **Problem:** in UART, `[`/`{` start a live display and `]`/`}` stop it; `protocol/uart/terminal-usage.md:107` says "wire effect | none; neither character is sent on the UART wire". The matrix's "yes" for START/STOP is true only in the "handler exists" sense; a BASIC program calling `START` expecting bus framing gets a display control with no wire effect (and possibly a display loop). **Fix:** mark the UART START/STOP cells "display only — no wire effect" or add a footnote; say `SEND`/`RECEIVE` need no wrapper in UART.

27. **[severity: minor]** `docs/tools/basic/README.md:55` — "| on-board VPU source (`e`) | — | terminal only; `MODE` clears it |" vs `board-pins.md:48-52` — "The pull-up resistors tie the I/O lines to the **Vpullup** pin … On v4 the on-board source is selected with the terminal `e` menu". **Problem:** the BASIC set switches between "VPU source" and "Vpullup"/"on-board source" without a local mapping; the generic glossary has it (`Vpullup` is the pin printed `VPU` in the `v` table), but a reader inside the BASIC set may not connect them. **Fix:** write "Vpullup source (VPU)" once and use that form consistently, or add a one-line mapping in the README.

## What to keep

- The README as a map plus the five-minute version. The availability table and mode-number summary answered the board question immediately.
- The `MODE` reset table and the "order a program as `MODE`, then `PSU`/`PULLUP`/`AUXPIN`/`FREQ`, then the bus statements" paragraph. Even with the missing engage row, this is the single most useful paragraph in the set.
- The `PULLUP` inversion callout. It is unusual enough that I would have got it wrong; `IF PULLUP=0 THEN` is the right kind of concrete idiom.
- The two matrices and the "silent no-op / read token returns 0" rule. The 1WIRE `DAT`/`BITREAD` distinction is excellent.
- The HiZ safety table and the "a run does not reset the board" warning. This is the part I would trust with a connected circuit, once the `AUX 0` cleanup wording is fixed.
- The edge-case tables and the exact `LIST` / `INPUT` / `DATA` captures. The `PRINT 1,2` trap and the `SEND` vs `READ` disambiguation are the kind of details that save a real session.
- The explicit naming map at the top (Script engine / BASIC, `A0` / `AUX`, `1WIRE` / `1-Wire`). Keep that pattern.

## Structural opinion

The layout works for this function. One README plus topic pages for entry, language, bus, board pins, examples, limits and edge cases is the right shape; I never felt lost for long.

Concrete proposals:

1. **Make `examples.md` task-indexed or split it.** It is 386 lines and does two jobs (language examples and bus examples). Either add a "Find an example by task" table at the top (Hello, replace/delete, arithmetic, INPUT, DATA, subroutine, I2C, PWM, board pins, timing), or split into `examples-language.md` and `examples-bus.md`. Add the missing `GOTO`, bit-bang/`STOPR`, and `MACRO` sessions there.
2. **Add a three-line "start here" path at the top of the README.** Not a manual; just "First program → examples §Hello; line model → entry-and-prompt; language → language." The current file table is good, but a reader still has to choose.
3. **Put direct anchors in the README's bus bullet** to `bus-access.md#protocol-operations` and `#bit-bang-operations`, and in the board-pins link to `#hiz-safety`. The matrices are the two things I reopened most; make them one click.
4. **Add a single "terminal ↔ BASIC" table with units and availability.** This is where the current errors cluster: `TIME` vs `t`, `FREQ` vs `f`, UART `START`/`STOP` vs the live display, `SEND` string/hex vs decimal-only. One table with columns "terminal form | BASIC form | unit / caveat" would prevent all four.
5. **Add the missing "bus engagement" row to the `MODE` reset table.** It is the one missing piece of the mental model; put it beside "power supplies" and "mode settings".
6. **Keep one canonical statement-byte / byte-count explanation.** It is currently in `entry-and-prompt.md` and again in `limits-and-variants.md`, with the "15 is a 16-byte program" arithmetic. Keep the full breakdown in the line-model page and cross-ref from limits.
7. **Keep limits and edge-cases separate**, but cross-link them from `language.md` §Errors. The error table is the index; the edge-case tables are the examples. Right now the reader has to know to jump.

## Unknowns

- The exact v3 `AUXPIN 2` symptom: parse-time `Syntax error` or run-time `Error(3)`. The set says both; I could not run v3 hardware.
- Whether BASIC `ADC` under SMPS ownership raises `Error(3)` or returns the feedback sample. The BASIC page and the SMPS page disagree; BASIC may be a deliberate special case.
- Whether BASIC `FREQ`/`DUTY` no-ops under SMPS ownership or reprograms the shared output-compare channel. The BASIC page and the PWM page disagree.
- Whether `MODE`'s early engage is safe for modes whose setup expects target power (UART auto-baud, KEYB). The docs say "order `MODE`, then `PSU`," but do not discuss this.
- The exact `TIME` / `t` relationship on hardware. I believe the 64× difference from the two pages, but I could not confirm it.
- Whether `FREQ` at 1–999 Hz is as accurate as the "whole timer ticks" sentence implies; the linked PWM page starts at 1 kHz.
- Whether `START` in a UART BASIC program blocks in the live display or is just a suppressed flag. The matrix says "yes"; the UART page says "no wire effect."
- Whether error code 1 exists. The table starts at 2 and never says why.
- Whether `CLK 3` is a silent no-op or an error; `edge-cases.md:111` says "nothing happens."
- Whether `DATA -1` then `PRINT` shows `65535` (implied by the unsigned print rule, but no example).
- Whether `FREQ 0` alone is an error (the limits table says "off (0)", the form table says `FREQ 0 duty`).
- Whether `AUX 0` as a "cleanup" is safe for a particular connected circuit; it is a drive-low, not a release, and the right state is circuit-dependent.
