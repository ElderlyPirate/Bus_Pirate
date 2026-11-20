# AUX round 1 — re-audit

Scope: the 19 findings in `feedback-aux-round1.md`, checked against the
current AUX pages and the cross-set pages named in the fix record.

**15 of 19 items filled in full.**

The four partial items are cross-set or remainders: the SMPS no-build limits
bullet (2), the binary-layer timer wording (4), the binary PWM example (14),
and the binary long line (18). The two blockers are not both closed: item 1
is closed; item 2 is still open at `smps/binary.md:148`.

## Per-item table

| # | Status | Evidence (current quote) | Remainder |
|---|---|---|---|
| 1 | **Filled in full** | `auxiliary/binary.md:22` — “`0x01`, optional text for drive/release, then the sub-result”; `auxiliary/binary.md:134-135` — “The drive and release sub-commands reuse the terminal pin messages, so their reply is not two bytes”; `protocol/i2c/binary.md:232-235` — “It is **not always two bytes**: the drive/release sub-commands print a terminal text line on the same wire”; `:302-305` — `0x09 0x02` now shows `0x01`, `"AUX INPUT/HI-Z"`, `0x01`. | None. The AUX page and the I2C authority now agree on the text and the byte order. |
| 2 | **Partial** | Fixed in `smps/binary.md:31-34` — “it **consumes** its operand byte, so the next request stays aligned”; `smps/edge-cases.md:10-16` — “**consumes** its operand byte… the disabled path consumes a start's operand.” Still wrong at `smps/binary.md:148-149` — “A start on a feature-disabled build also returns `0x00` but does not consume its operand; the operand becomes the next request.” The recovery paragraph also has a dangling fragment, `smps/edge-cases.md:19-21`: “If you are unsure of the layer, send `0x01` to reprint the mode identifier (`RAW1`). / are back in `BBIO1` — re-enter raw wire with `0x05`.” | The limits bullet still states the opposite of the body and table. Delete it or change it to “consumes its operand”. Repair the dangling sentence (“If the operand was `0x00`, you are back in `BBIO1`…”). This is the remaining blocker. |
| 3 | **Filled in full** | `aux-pin.md:184-188` — “The SUMP exit prints the banner, runs the terminal reset… target back to `AUX`, and the generator flag cleared”; `pwm-servo.md:254-257` — “The SUMP exit prints the banner and then runs the terminal reset, so the generator flag is cleared”; `edge-cases.md:409-418` — “the exit prints the banner, runs the terminal reset… the target returns to `AUX`, the generator flag is cleared”. `generic/commands.md:133-135` and `sump/entry.md:128-132` already agreed. | `wiring.md:143-144` still lists only “`#`, a mode change (`mN`) or the binary return” and omits the SUMP exit. It does not contradict the reset claim, but the set no longer gives the same complete list everywhere. |
| 4 | **Partial** | The AUX pages now define the layers: `pwm-servo.md:219-226` — “generator flag… output path… timer registers” and “A v4 reset… clears the generator flag and detaches the output, but it does not stop Timer2”; `auxiliary/binary.md:84-88` — “clears the generator flag and detaches the output; the reset does not stop Timer2.” The cross-set page still says the old thing: `protocol/binary/commands.md:213-214` — “A PWM timer setup survives this reset”; `:281-283` — “the timer setup stays until `0x12`, `0x13` or a hardware reset.” | The cross-set page does not use the new “flag / output path / timer registers” vocabulary and does not say the flag is cleared. A reader coming from that page can still think the terminal generator remains active. See also the new table-column issue below. |
| 5 | **Filled in full** | `frequency.md:77-90` — “The five-second bound applies to the period capture only… If the period capture reaches the bound without enough edges, the firmware prints the unsupported message after the second `autorange` and no frequency value”; the exact output is shown; `edge-cases.md:216-228` — the new “catches an edge but the period capture times out” case gives two `autorange` tokens and about seven seconds. | None. The timeout output and the total wait are now stated. |
| 6 | **Filled in full** | `aux-pin.md:50-59` — “At `HiZ>` there is no non-destructive readout: `i` prints only the banner, and `v` shows pin directions and levels, not the selection”; `edge-cases.md:21-32` — “`i` prints only the banner; `v` lists pin names and directions but does not mark the selection”. | Optional DIO cross-set: `protocol/dio/aux-pin.md:45` still says “Run `i`. The status block prints…” without the `HiZ` caveat. The AUX page itself is correct. |
| 7 | **Filled in full** | `auxiliary/binary.md:153-157` — “If a host needs a clean reply for a drive or release, the terminal `a` / `A` / `@` commands act on one pin only. There is no per-pin write in BBIO1: its state byte rewrites all five latches plus the supply and pull-up bits, and its direction byte rewrites all five directions.” | None. The unsafe BBIO1 alternative is gone. |
| 8 | **Filled in full** | `edge-cases.md:3-4` — “Entries are grouped by function; each gives the trigger, what you see, why, and what to do”; the page is now Terminal AUX pin / PWM and servo / Frequency / Binary AUX / Resets and persistence, with per-case block, no 4-column table. | None. The old 36 rows are retained as readable blocks. |
| 9 | **Filled in full** | `smps/binary.md:21-24` — “the v4 terminal reset that returns from the binary layer stops it through the board reset”; `smps/edge-cases.md:72-76` — “stopping sets `AUX1` to input, clears the routing and drives the latch low”; `smps/README.md:32-35` — “the v4 terminal reset stops it through the board reset”. | None. The `AUX1` stop state and the reset path now agree. |
| 10 | **Filled in full** | `smps/edge-cases.md:81-89` — “the terminal `d`/`D` is refused… binary `0x14`/`0x15` return `0xFF 0xFF` once… the loop continues until `0xF1` or a reset”; this matches `adc/terminal.md:126-130` and `adc/binary.md:132-137`. | None. The false “probe turns the converter off” claim is gone. |
| 11 | **Filled in full** | `pwm-servo.md:187-197` — table columns “Compare ticks / Nominal width / v4 high time”, values 62/0.99/63 ticks 1.01 ms through 126/2.02/127 ticks 2.03 ms; text says the compare value is what the firmware writes and the v4 output is one tick longer. | None. The table and the text now agree. |
| 12 | **Filled in full** | `frequency.md:48-55` — “The trigger is the first gate's edge count, not the input frequency: a count of 16,384 or more skips the rerun… At exactly 4.194304 MHz the gate can hold 16,383 or 16,384 counts depending on phase”; `:113-121` uses “Nominal input”; `:146-156` — “A first gate of 256 or more… nominally 65,536 Hz… phase”. `auxiliary/binary.md:94-100` and `README.md:64-66` use “nominally”. | None in the AUX set. `protocol/binary/commands.md:318` still says “Above about 65.5 kHz” but gives the 255-count rule first; that is a rounded restatement, not a contradiction. |
| 13 | **Filled in full** | `aux-pin.md:70-73` — “The replies print the literal `AUX`; the `v` table lists the header pins by name but shows directions and levels, not the selected target. Only the confirmation line names the selection.” | None. This is the correct version; it no longer says the `v` table prints every target as `AUX`. |
| 14 | **Partial** | AUX page fixed: `auxiliary/binary.md:63-64` — “duty `0x0063` (99) and period `0x00C7` (199) give about 1250 Hz with a 50 % high time on v4”. Cross-set still uses 100: `protocol/binary/commands.md:265-266` — “duty `0x0064` (100)… about 50 %”; `protocol/binary/examples.md:144` — “duty `0x0064` = 100”. | The same worked example still shows two duty bytes. Standardize to `0x0063` for exact 50 %, or note that `0x0064` is 50.5 % on v4. This is cross-set, minor. |
| 15 | **Filled in full** | `README.md:3-4` — “The v3 features are present in the v3 build; their behaviour on v3 hardware is `unverified`”; `README.md:75` — v3 cells now say “present”; the closing “build-only” caveat is gone; `wiring.md:158` — “3.3 V outputs, 5 V class inputs”; `wiring.md:161` — “optional build; off in the default image”. | Inline `unverified` caveats remain in topic pages (for example `pwm-servo.md:115`, `frequency.md:134`); those are short reliance caveats, not bench-result prose. |
| 16 | **Filled in full** | `README.md:21-24` — “Read in this order: wiring.md, aux-pin.md, then pwm-servo.md or frequency.md, and finish with the binary pages… and edge-cases.md”; `README.md:109-121` is now a short “See also” paragraph, not a third table. | None. The README is 121 lines, at the template target. |
| 17 | **Filled in full** | `edge-wait.md:60-61` — `host -> 0x70 0x01 0x03 0xE8    wait for rising, 1000 ms` / `host <- 0x01 0x01              edge seen, pin is high`; both comments start at column 32. `:67-68` is the same. | None. |
| 18 | **Partial** | AUX page rewrapped: `pwm-servo.md:73-75` now breaks after “The binary command is in…” and the BASIC link is on its own line. Cross-set still long: `protocol/binary/commands.md:228` — “errors).  If the SMPS owns the ADC, the reply is `0x01` and no check runs.  While the test is idle it reads input — any byte other than `0xFF`” (142 characters). | The AUX set passes the ~80-column check; the named cross-set line did not change. Split it at “If the SMPS owns the ADC”. |
| 19 | **Filled in full** | `README.md:79` — “SMPS owns the output channel \| — \| optional build; off in the default image”; `wiring.md:161` — “SMPS supply PWM on `AUX1` \| — \| optional build; off in the default image”. | None. |

## Presentation check

- **TAB characters:** none in the 7 AUX files (scan: `CLEAN`).
- **Aligned captures:** `edge-wait.md` comments are 32/32 in both exchanges;
  `auxiliary/binary.md` `0x09` comments are all 31/31. Pass.
- **Menus / option lists as tables:** no menu or option list in the AUX set.
  The PWM interactive form is a transcript, not an option list. Pass.
- **~80-column wraps:** no AUX prose line over 80 columns. The only named
  cross-set overflow is `protocol/binary/commands.md:228` (142), item 18.
- **Bench prose / dates / test-result prose:** no dates, pass counts, bench/
  rig narratives, capture provenance or internal log names. Inline
  `unverified` caveats remain at `pwm-servo.md:115,198`, `frequency.md:134,139`,
  `aux-pin.md:195`, `edge-wait.md:90`, `binary.md:60`; these are short
  reliance caveats allowed by the template, not test-result reporting.
- **Naming:** the A0/CS/A1/A2 mapping is still explicit; the new
  “generator flag / output path / timer registers” vocabulary is used
  consistently across the AUX pages. No new naming mismatch found.

## New problems (max 5)

1. **[minor] — dangling recovery sentence.**
   `docs/capabilities/smps/edge-cases.md:19-21`:
   “If you are unsure of the layer, send `0x01` to reprint the mode
   identifier (`RAW1`). / are back in `BBIO1` — re-enter raw wire with
   `0x05`.”
   The SMPS operand edit cut the subject from the second sentence. Restore
   “If the operand was `0x00`, you are back in `BBIO1`…” or delete the
   fragment.

2. **[minor] — the new three-layer table's “Cleared or stopped by” column
   mixes clear/stop with replace/restore.**
   `docs/capabilities/auxiliary/pwm-servo.md:219-223`:
   “| generator flag | … | a bare `g` / `S`; a new `g` / `S` with values
   replaces the setup; … |” and “| output path | … | `@` on `AUX`; … `a` /
   `A` on `AUX`, which make it an output again; … |”.
   A new `g`/`S` with values leaves the generator flag **set**; `a`/`A`
   restores the output path rather than clearing it. Rename the column to
   “Changed by” or split it into “Detached by / restored by”; move the
   `a`/`A` cell to the restore side.

3. **[minor] — the AUX set's reset list is now incomplete in one place.**
   `docs/capabilities/auxiliary/wiring.md:143-144`:
   “On v4 that reset is `#`, a mode change (`mN`) or the binary return; on
   v3 `#` is a hardware reset.”
   The SUMP exit also runs the terminal reset on both boards. Add it to the
   list, or say “for example” so the list is not read as exhaustive.
