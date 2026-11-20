# BASIC docs — round 1 re-audit

**Verdict: 26 of 27 items filled in full.**

Re-read scope: the 27 findings in `feedback-basic-round1.md`, against the current `docs/tools/basic/` files. The two cross-set residuals from findings 5 and 6 are noted separately; they are outside the BASIC-page count.

## Per-item table

| # | Status | Evidence (current passage) | Remainder |
|---:|---|---|---|
| 1 | Filled in full | `entry-and-prompt.md:199`: “five to nineteen NULs, then the Left-arrow key (`0x02`)”; `:201`: “The two windows do not overlap: the twentieth NUL enters binary first”; `edge-cases.md:35`: “20 NULs, or 5-19 NULs plus Left arrow” | — |
| 2 | Filled in full | `board-pins.md:150-152`: “The terminal `t` prints the same timer in microseconds: 1 µs units, 31-bit, wrapping after about 35.8 minutes; `T` resets it.  BASIC `TIME` is `t / 64`: 16-bit, wrapping about every 4.19 s.”; `bus-access.md:138`: “BASIC counts 64 µs units (16-bit), terminal `t` is 1 µs (31-bit)” | — |
| 3 | Filled in full | `examples.md:310`: “The program above therefore needs an external Vpullup supply on v4, and on v3 in every case.”; `:312`: “delete line 10 and do the I2C setup at the terminal” | New minor: the `RUN` block at `:299-303` still shows `I2C`; the no-`MODE` variant will not print that line. |
| 4 | Filled in full | `board-pins.md:196-197`: “`AUX 0` is a **drive-low**, not a release.  To release the AUX pin, read it (`PRINT AUX;`) or reset the board.” | — |
| 5 | Filled in full (BASIC) | `board-pins.md:134-137`: “The terminal probe commands use the same check and are refused the same way; a sample needs the SMPS stopped”; `edge-cases.md:61`: “the terminal probe is refused too” | Cross-set residual: `capabilities/smps/edge-cases.md:84-90` still says `d`/`D` read the SMPS feedback. |
| 6 | Filled in full (BASIC) | `board-pins.md:112`: “`FREQ`/`DUTY` do nothing and report no error; the terminal `g` is refused instead” | Cross-set residual: `capabilities/auxiliary/pwm-servo.md:191-194` still says “each reprograms the channel”. |
| 7 | Filled in full | `bus-access.md:277-279`: “`AUXPIN` accepts 0 and 1 on v3 and 0..3 on v4; a larger value stops the program with `Error(3)`”; `edge-cases.md:113`: “`Error(3)` at run time … the line is stored without complaint” | — |
| 8 | Filled in full | `bus-access.md:47-48`: “the new mode's setup, including the engage step”; `:64`: “bus engagement … the step `W` runs”; `:77-78`: “`MODE` leaves the bus engaged but unpowered” | — |
| 9 | Filled in full | `language.md:290-292`: “Errors appear in two places.  A line rejected as you type prints one of the entry messages; a stored line that fails while it runs prints the `Error(N)` line.”; `:305-306`: code 2/3 broadened; `:316`: “Code 1 is not an error…” | New minor: `language.md:296` says “typing a line” for `Syntax error` with “not `RUN`/`LIST`/`NEW`/`EXIT`”, which reads as if `10 A=1` is rejected at entry; it is stored and fails at run. |
| 10 | Filled in full | `board-pins.md:37-39`: “**Do not use `PSU 1` into a self-powered target.**  It bypasses `W`'s voltage checks, so it can back-drive the target's supply.” | — |
| 11 | Filled in full | `README.md:47`: “PWM `FREQ`/`DUTY`”; `:96-98`: “Terminal commands with no BASIC token: `f` (frequency counter), `v` (status), `~` (self-test), `O` (1-Wire overdrive) and `o` (display mode).” | New minor: the list is not exhaustive (`e`, `L`/`l`, `;N`, `b`, `h`, `#`, `$` also have no BASIC token); say “including” or “for example”. |
| 12 | Filled in full | `bus-access.md:215-216`: “`SEND` takes a decimal number only.  `SEND 0x7B` sends `0` and then stops with `Error(2)` … `SEND "A"` stops with `Error(3)`”; `edge-cases.md:63`: “`SEND 0x7B` … decimal only” | — |
| 13 | Filled in full | `README.md:87-90`: “no user-defined or built-in numeric functions (`ABS`, `INT`, `RND`), no `AND`/`OR` keywords” | — |
| 14 | Filled in full | `README.md:79`: “Summary tables:”; `:82-83`: “The per-mode sets are linked from the mode table in [generic/README.md]”; `bus-access.md:270-272` repeats the pointer | — |
| 15 | Filled in full | `entry-and-prompt.md:129`: “the `65535  END` marker”; `:128-130`: “Line 10 is 8 bytes, line 20 is 4 bytes…”; `examples.md:50`: “`65535  END` line” | — |
| 16 | Filled in full | `bus-access.md:123`: “`m` / `m<N>`”; `entry-and-prompt.md:178`: “`m` or `m<N>`” | — |
| 17 | Filled in full | `examples.md:324`: “A `2WIRE` program has the same pull-up and `MODE`-reset caveat” | — |
| 18 | Filled in full | `README.md:3-4` and `limits-and-variants.md:102-103`: the general v3 caveat remains once in each; it was removed from `bus-access.md` and `edge-cases.md`. “unverified” remains only on the specific ADC divider. | — |
| 19 | Filled in full | `board-pins.md:165-166`: “`RUN` prints the elapsed time in 64 µs units — about 1562 counts for the 100 ms delay.”; no prose line over 80 columns in the set | — |
| 20 | Filled in full | `board-pins.md:96-118`: §PWM is now a two-column `Item` / `Behaviour` table (frequency, duty, out of range, separators, operands, stored form, output, accuracy, SMPS, HiZ) | — |
| 21 | **Partial** | `examples.md:8-22`: task index now includes “jump with `GOTO`” and “bit-bang clock and data”; `:260-282`: GOTO loop; `:327-357`: 3WIRE bit-bang | No `MACRO` example or task-index row. The original finding listed `MACRO` as missing; add a short `MACRO n` session or state that interactive macros are terminal-only. |
| 22 | Filled in full | `language.md:305`: “trailing text after a complete statement (`START 1`, `INPUT A, B`)”; `:316`: “Code 1 is not an error…” | — |
| 23 | Filled in full | `board-pins.md:111`: accuracy row gives 1 Hz / 100 Hz / 4 kHz counts; `:122-123`: “The timer and prescaler description is written for the terminal's kHz range” | — |
| 24 | Filled in full | `bus-access.md:186-187`: “Press the board's reset button or power-cycle … `#` cannot be typed while the macro holds the interpreter.”; `edge-cases.md:116` repeats the reset/`EXIT` note | — |
| 25 | Filled in full | `entry-and-prompt.md:128-130`: “Line 10 is 8 bytes, line 20 is 4 bytes and the `65535  END` marker is 4, 16 in all; `LIST` prints `15` because the count stops at the terminating zero byte.”; `limits-and-variants.md:46-49` cross-refs it | — |
| 26 | Filled in full | `bus-access.md:179-181`: “In `UART`, `START` and `STOP` only switch the terminal's live display and echo on and off; neither puts a byte on the wire.  `SEND` and `RECEIVE` need no wrapper.”; matrix cells say “display only” | — |
| 27 | Filled in full | `README.md:61`: “on-board Vpullup source (`e`)”; `board-pins.md:55-56`: “the `v` table prints the pin as `VPU`”; `bus-access.md:56`: “on-board Vpullup source (`e`, v4)” | — |

## Presentation check

- **TAB characters:** none in any of the eight `docs/tools/basic/*.md` files.
- **Capture alignment:** `LIST`, `INPUT`, PWM, I2C and the new bit-bang session all use space alignment; no tabs and no obvious column drift. The `LIST` captures consistently show two spaces after the line number.
- **Menus / option lists as tables:** the PWM settings, AUX targets, mode numbers, protocol matrix, bit-bang matrix, INPUT forms and the examples task index are tables. No dense option-list bullet chain remains.
- **~80-column wraps:** no prose line in the set exceeds 80 columns. Long rows are table rows, which are intentionally wider.
- **Bench prose / dates / results:** no dates, bench/rig/battery narratives, pass counts, capture provenance, evidence sections or internal log names in the user docs. The only provenance language is the permitted “not verified” / “unverified” caveat in the README, limits and ADC divider rows.

## Still open

**In-set partial: #21.** The examples page has GOTO and bit-bang sessions but still no `MACRO` example or task-index row for macros.

**New minor problems (not counted in the 26):**

1. **[severity: minor]** `language.md:296` — “| typing a line | `Syntax error` | not `RUN`/`LIST`/`NEW`/`EXIT`, or a byte that cannot be stored as text |”. This reads as if any non-command line is rejected at entry. A numbered line such as `10 A=1` is stored (`Ready`) and fails only at run with `Error(3)`, as `examples.md:91-96` shows. Say “an unnumbered line whose first word is not `RUN`/`LIST`/`NEW`/`EXIT`”.
2. **[severity: minor]** `README.md:96-98` — “Terminal commands with no BASIC token: `f` …”. The colon makes the list sound exhaustive, but `e`, `L`/`l`, `;N`, `b`, `h`, `#` and `$` also have no BASIC token (several are covered in `bus-access.md`). Say “including” or “for example”.
3. **[severity: minor]** `examples.md:299-303` — the I2C `RUN` block still shows `I2C` before `255`. If the reader follows the new “delete line 10” path, no `MODE` statement runs, so that line is not printed. Label the block as the `MODE 4` variant or show the alternative output.

**Cross-set residuals (recorded by the implementer, outside the BASIC count):**

- `capabilities/smps/edge-cases.md:84-90` still says `d`/`D` read the SMPS feedback; the BASIC page now says the terminal probe is refused. The BASIC page is the accurate one per the fix record, but the linked page contradicts it.
- `capabilities/auxiliary/pwm-servo.md:191-194` still says the terminal generator and SMPS “each reprograms the channel”; the BASIC page now says `FREQ`/`DUTY` do nothing and the terminal `g` is refused. The BASIC page is the accurate one per the fix record, but the linked page contradicts it.
