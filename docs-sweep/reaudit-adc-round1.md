# ADC probe — round 1 re-audit

**Scope:** the 15 findings in `feedback-adc-round1.md`, checked against the
current six files in `docs/capabilities/adc/`. Cross-set items are marked as
such; the ADC set is the primary scope.

**12 of 15 items filled in full.** All 12 ADC-set items are full. Findings 3,
14 and 15 are cross-set-only and remain open in the SMPS/binary pages;
findings 2, 4 and 5 are full in the ADC pages but still carry their cross-set
halves (selftest removal step, `smps/binary.md` caveat, selftest rail-check
wording). No new major finding in the ADC set.

## Per-item table

| # | Status | Evidence (current docs) | Remainder |
|---|---|---|---|
| 1 | **Filled in full** | `wiring.md:54-59`: "it is the reliable measurement limit, not a documented absolute maximum for the pin. ... treat any voltage above 6.6 V as potentially damaging, not as a harmless clip, and disconnect it." `wiring.md:48-52` now says "measurement range (nominal)" and "largest reading". `edge-cases.md:22-28`: "the pin may be damaged. ... Above 6.6 V is not a harmless clip." | None. |
| 2 | **Filled in full (ADC)** | `wiring.md:107`: "Leave the self-test `ADC`→`+3.3V` jumper fitted with a source connected \| Plain wire to the `+3.3V` rail; the source back-drives it and can damage the board or source. Remove it after the test." `edge-cases.md:112-116`: "the jumper is a plain wire ... It hard-ties the two, so a connected source drives the rail and can damage the board or the source. ... Remove it after every self-test..." | Cross-set item 4: `selftest/wiring.md` still has no after-run removal step, and `selftest/terminal.md:160` still says "fit/reseat the jumper". |
| 3 | **Not addressed (cross-set only)** | ADC is correct: `probe.md:130-132`: "Commands that need a foreground measurement check ownership first and refuse rather than stealing the channel." `smps/edge-cases.md:86-90` is unchanged: "the reading follows the SMPS feedback ... a probe command also turns the converter off at the end, so the feedback loop stops updating until the next start." | Main-session cross-set item 1. The SMPS page is the outlier; the ADC pages are internally consistent. |
| 4 | **Filled in full (ADC)** | `binary.md:138-142`: "send `0x05` first to enter `RAW1`, then `0xF1` (stop; reply `0x01`), then `0x00` to return to `BBIO1`. Do **not** send `0xF1` in `BBIO1`: there it is a pin-state byte that switches the target supply and pull-ups on, not a stop." `binary.md:143-147`: "Alternatively send `0x0F` from `BBIO1` ... SMPS loop stopped". `probe.md:133-136` gives the same `0x0F` answer; `terminal.md:143-148` gives `#`/reset as the terminal recovery. | Cross-set item 2: `smps/binary.md:21-23` still calls the terminal-reset stop "`unverified`", which contradicts the ADC page. No ADC remainder. |
| 5 | **Filled in full (ADC)** | `probe.md:110-118`: "The 3.3 V channel is the rail that supplies the converter, brought in through its own divider. It is therefore ratiometric too ... measured against itself ... The `v` column, the `W` rail check and the self-test 3.3 V window are go/no-go checks on the divider and on the channel being present; they cannot prove the rail is at 3.3 V or catch a small droop." `probe.md:81`: "3.3 V channel \| ratiometric to that rail; divider/rail-present check only". | CO-1 is still conditional (board topology), and the docs state the same-rail topology as fact. Cross-set item 5: `selftest/checks.md:121-122` still says "`3.3V` is low" / "the rail is out of band: regulator fault or short". |
| 6 | **Filled in full** | `terminal.md:95-99`: "Set the terminal to send CR only for Enter; a terminal that always sends CR+LF leaves the LF waiting and `D` exits immediately every time. Pressing more keys adds bytes; it does not drain the line. If the terminal cannot send CR alone, use `d` or a host program for `D`." `edge-cases.md:86-88`: "set the terminal to send CR only (not CR+LF), then retry; or use a host program that sends one byte." | None. |
| 7 | **Filled in full** | `binary.md:174-176`: "host -> 0x00 ... host <- 0x02 0x00 ... host <- (no sample for timeout)  drain complete". `binary.md:182-183`: "The timeout is chosen by the host; the firmware has no fixed drain time." | None. The new line starts at column 33, aligned with the other comments. |
| 8 | **Filled in full** | `README.md:47-48`: "(example; the display truncates, so 3.30 V prints as `3.29V`)". `README.md:9`: "The largest reading is 6.59 V (raw 1023)". `README.md:71-72`: "nominal full scale" and "largest reading". | None. |
| 9 | **Filled in full** | `terminal.md:33`: "N conversions, N = 1..32767; not interruptible \| N readings total; the header is on the first line". `README.md:49-50`: "prints N lines total with the header on the first; the repeat cannot be interrupted, so use a small N." | None. |
| 10 | **Filled in full** | `wiring.md:107`: "`ADC`→`+3.3V` jumper". `edge-cases.md:108`: "`ADC` → `+3.3V` jumper". Both now use the firmware form. | None. |
| 11 | **Filled in full** | `README.md:25-29`: "Read in this order: wire ... take one reading ... understand the scale ... then script it." `README.md:58-60`: "[terminal recovery](terminal.md#smps-owns-the-adc), [binary bytes](binary.md#smps-owns-the-adc)". | None; both anchors exist. |
| 12 | **Filled in full** | `README.md:17`: "The firmware uses several labels for the same input." `README.md:86-89`: "Only the probe channel has dedicated commands ... housekeeping, displayed by `v` and read by the self-test..." | None. |
| 13 | **Filled in full** | `wiring.md:99-108`: "## What not to connect" is now a `Do not \| Because` table with six rows; the jumper row is the hard-tie/back-drive warning. | None. |
| 14 | **Not addressed (cross-set only)** | `protocol/binary/README.md:3-4` is unchanged: "v3 is unverified (built for v3, not tested on v3 hardware)". Line 82 still says "**not been tested on v3 hardware**". | Main-session cross-set item 6. |
| 15 | **Not addressed (cross-set only)** | `smps/binary.md:31-33` is unchanged: "its operand byte is **not** consumed, so it is read as the next raw-wire request." `protocol/binary/sub-protocols.md:299-300` still says a start "still consumes its operand byte". `smps/edge-cases.md:11-18` still says "leaves its operand byte unread". | Main-session cross-set item 3. |

## Cross-cutting judgement

**Safety limit: accurate.** `wiring.md:54-59` now separates the firmware's
nominal 6.6 V scale from the pin's undocumented absolute maximum, says the
reading clips at 6.59 V and cannot show how far over the input is, and tells
the reader to treat >6.6 V as potentially damaging and disconnect it. The edge
case repeats the damage possibility (`edge-cases.md:22-23`). The table labels
"measurement range (nominal)" and "largest reading" are the right split. This
is no longer a "harmless clip" story.

**Jumper hazard: accurate in the ADC pages.** `wiring.md:107` and
`edge-cases.md:112-116` now say plain wire, hard tie to `+3.3V`, source
back-drives the rail, can damage board or source, and remove it after every
self-test. The old "pulled toward 3.3 V / may fight" wording is gone; the
high-impedance vs low-impedance symptom split is explicit. The only remaining
gap is outside the ADC set: the linked self-test page still has no removal
step (cross-set item 4).

**Executable SMPS recovery: accurate in the ADC pages.** `binary.md:138-142`
gives the exact layer-by-layer sequence from `BBIO1`: `0x05` → `RAW1`, `0xF1`
→ `0x01`, `0x00` → `BBIO1`; it warns that `0xF1` in `BBIO1` is a pin-state
byte that switches the supply and pull-ups on, and `binary.md:143-147` gives
the `0x0F` alternative with the reset effects. `edge-cases.md:240-246` and
`wiring.md:95-97` repeat the sequence; `terminal.md:143-148` marks the `HiZ>d`
refusal defensive and gives `#`/reset. The ADC story is now executable. The
cross-set contradiction remains: `smps/binary.md:21-23` still calls the
terminal-reset stop `unverified`, and `smps/edge-cases.md:86-90` still says the
probe commands read the feedback instead of refusing (cross-set items 2 and 1).

**Reference limit: accurate as written, with CO-1 still conditional.**
`probe.md:19-28` now says the reference is the converter's 3.3 V supply (AVdd),
the probe and the other external channels are ratiometric, and the 3.3 V
channel is the exception measured against itself. `probe.md:110-118` spells out
the consequence: a sagging rail still reads about 3.3 V, and the `v` column,
`W` rail check and self-test 3.3 V window are divider/rail-present checks, not
absolute rail measurements. The resolution table says the same
(`probe.md:81`). This directly answers the trust question. CO-1 remains open:
the "same rail supplies the converter" topology is stated as fact in the user
page while the code-owner record still asks for board confirmation. If the
schematic confirms it, the text is correct; if not, the 3.3 V channel paragraph
needs a correction. The selftest page still frames the 3.3 V check as a
regulator/rail check (cross-set item 5).

## Presentation check

- **TAB characters:** 0 in all six ADC files.
- **Aligned captures:** the `host ->` block in `binary.md` has every comment at
  column 33, including the new `host <- (no sample for timeout)  drain
  complete` line at column 33. No misalignment.
- **Menus / option lists as tables:** the wiring hazard list is now a
  `Do not | Because` table; the command, file, channel and related tables are
  balanced (no pipe-count mismatches, no odd fences). There are no raw option
  menus in the ADC set.
- **~80-column prose:** no non-table prose line exceeds 80 columns. The long
  lines are table rows and link definitions. `wiring.md:107` is a 200-character
  table row; it renders, but it is heavy in raw Markdown. The `README` Related
  table rows run to 139 characters. This is a readability note, not a wrap
  failure.
- **Bench prose, dates, tested/verified-result prose:** none in the ADC set.
  The only "verified" language is the allowed "not verified"/"unverified"
  caveat (`README.md:4`, `README.md:67-68`, `wiring.md:120`). No dates, pass
  counts, capture provenance or internal log names.

## Cross-set items still open (for the main session)

1. **Finding 3 / cross-set item 1:** `smps/edge-cases.md:86-90` still says the
   probe reading follows the SMPS feedback and that a probe command turns the
   converter off. The ADC guard/refusal is the correct behavior.
2. **Finding 4 / cross-set item 2:** `smps/binary.md:21-23` still calls the
   terminal-reset stop `unverified`; `smps/edge-cases.md:63-66` says "only
   `0xF1` ... stops them" and then names the terminal reset as an exception.
3. **Finding 15 / cross-set item 3:** `smps/binary.md:31-33` and
   `smps/edge-cases.md:11-18` still say a no-SMPS start leaves its operand
   unread; `protocol/binary/sub-protocols.md:299-300` and
   `protocol/binary/edge-cases.md` say it is consumed.
4. **Finding 2 / cross-set item 4:** `selftest/wiring.md` still has no
   after-run removal step for the `ADC`→`+3.3V` jumper;
   `selftest/terminal.md:160` still says "fit/reseat".
5. **Finding 5 / cross-set item 5:** `selftest/checks.md:121-122` still says
   the `ADC` failure can mean "`3.3V` is low" and the `3.3V` failure is "the
   rail is out of band: regulator fault or short", which the new ratiometric
   note in `probe.md` contradicts for a small droop.
6. **Finding 14 / cross-set item 6:** `protocol/binary/README.md:3-4` and line
   82 still use "not tested" instead of "unverified".

## New issues introduced by the fixes

These are polish only; none changes the verdict.

1. **[minor]** `wiring.md:55-56` repeats the absolute-maximum disclaimer:
   "not a documented absolute maximum for the pin.  The pin's absolute maximum
   is not documented here." Keep one sentence.
2. **[minor]** `terminal.md:144-145` has an awkward construction: "the terminal
   `#` reset and the binary `0x0F` leave both do". Rewrite "both stop the
   loop."
3. **[minor]** `wiring.md:107` is a 200-character table row. It renders, but
   splitting the "Because" cell or moving the removal step below the table
   would make the raw source easier to read.

## Method

I re-read all six current ADC files, quoted the current text, and checked the
cross-set pages the findings named (`smps/edge-cases.md`, `smps/binary.md`,
`protocol/binary/README.md`, `protocol/binary/sub-protocols.md`,
`selftest/wiring.md`, `selftest/terminal.md`, `selftest/checks.md`). The
15-item count is the original finding list; the 12/15 result matches the
implementer's own split (12 ADC-set fixes, 3 cross-set-only items).
