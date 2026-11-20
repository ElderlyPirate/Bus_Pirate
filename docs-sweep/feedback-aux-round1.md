# AUX capability — fresh-reader feedback (round 1)

Read as an engineer with a v3/v4 Bus Pirate who wants the AUX pin, PWM/servo,
and the frequency counter. Read order: all of `docs/capabilities/auxiliary/`,
then the cross-linked `docs/generic/`, `docs/protocol/binary/`,
`docs/capabilities/adc/` and `docs/capabilities/smps/` pages, with
`docs/TEMPLATE.md` and `docs/SPEC.md` as context. No files under `docs/` were
edited.

## Verdict

**Yes with fixes.** The terminal AUX, PWM/servo and frequency material is
genuinely usable: the ranges, the stop table, the display quirks and the
wiring are better than most hardware docs. The binary AUX surface is not
yet safe to implement from: two byte-flow contracts contradict each other on
the page a host author is sent to, and one reset claim is wrong in three AUX
files. A reader can succeed on the terminal tasks; a host author can build
the wrong frame.

## Task log

| Task | Where I looked | How fast | Success? | What was unclear |
|---|---|---|---|---|
| 1. First impression from README alone | `auxiliary/README.md` | ~1 min | Yes | “build-only” on the v3 column is undefined; no “read in this order” path for a seven-file set |
| 2. Terminal AUX selection and drive/read | README → `aux-pin.md` → `generic/commands.md#aux-targets-and-pin-names` | 2 clicks, ~3 min | Mostly | The replies always say `AUX`; that is clearly warned, but at `HiZ>` there is no non-destructive way to check which target is selected |
| 3. PWM and servo | README → `pwm-servo.md` → `edge-cases.md` → `smps/README.md` | 2 clicks, ~6 min | Yes for ranges/stop; no for reset side effects | “clears the generator state” vs “timer setup survives”; servo table calls the compare value “Pulse width” |
| 4. Frequency counter | README → `frequency.md` → `binary.md` → `protocol/binary/commands.md` | 2 clicks, ~5 min | Mostly | The 4.19 MHz / 65,536 Hz thresholds are input rates but the gate rule is an edge count; the 5 s give-up does not say what is printed |
| 5. Binary AUX surface | README → `binary.md` → `edge-wait.md` → `protocol/binary/commands.md` → `protocol/i2c/binary.md` | 3 clicks, ~7 min | No for `0x09`; yes for `0x12`/`0x13`/`0x16`/`0x70` | `0x09` text caveat contradicts the I2C authority; SMPS no-build operand consumption is documented both ways; `0x12` examples differ |
| 6. Rendering check | All required files | ~10 min | Mostly | No TABs, dates or bench narratives; but one enormous 4-column table, two misaligned comment columns, repeated v3 caveats, and a few broken wraps |
| 7. README navigation test | README link graph | 1 click to every topic | Yes | Dead ends/doubling back at `0x09`, SMPS stop and SUMP exit; all file links resolve |

## Findings

### Blockers

1. **[blocker]** — `docs/protocol/i2c/binary.md:232-235`, `:250`, `:292-294`;
   `docs/capabilities/auxiliary/binary.md:22`, `:135-160`
   > “The reply is always **two bytes**: `0x01` (the command was accepted) and then the sub-result. The first byte is not the sub-command's success flag; it is `0x01` even when the sub-command is unknown.”
   >
   > “No text line is printed.”
   >
   > `host -> 0x09 0x02       extended AUX: release AUX to high-Z`
   > `host <- 0x01            command accepted`
   > `host <- 0x01            sub-result`

   The AUX page says the opposite for the same sub-command: “The drive and
   release sub-commands (`0x00`, `0x01`, `0x02`) reuse the terminal pin
   messages, so they emit text between the leading `0x01` and the sub-result”
   (`auxiliary/binary.md:135-138`), and even says “The I2C binary page
   documents the two-byte shape; a host must also allow for this text”
   (`:159-160`). The I2C page does not document the text; it denies it and
   shows a clean two-byte exchange for `0x09 0x02`. A host that follows the
   I2C page will read `"AUX INPUT/HI-Z"` as the sub-result and lose framing;
   a host that follows the AUX page gets a different byte count from the page
   it was told is authoritative.

   Fix: make one page the authority and update the other. The I2C page needs
   a per-sub-command reply shape:
   `0x00`/`0x01` → `0x01`, `"AUX LOW" CR LF` or `"AUX HIGH" CR LF`, sub-result;
   `0x02` → `0x01`, `"AUX INPUT/HI-Z"` (no line break), sub-result;
   `0x03`/`0x10`/`0x20` → `0x01`, sub-result. Update the command-map row
   (`auxiliary/binary.md:22` and `i2c/binary.md:72`) from “`0x01`, then the
   sub-result” to “`0x01`, optional text for drive/release, then the
   sub-result” and fix the `0x09 0x02` worked exchange to show the text.
   If the text is not wanted on the wire, that is a firmware change, not a
   documentation choice.

2. **[blocker]** — `docs/capabilities/smps/binary.md:32-35`,
   `docs/capabilities/smps/edge-cases.md:10-15` vs
   `docs/protocol/binary/sub-protocols.md:299-300`,
   `docs/protocol/binary/edge-cases.md:174-182`
   > “A start request is also refused with `0x00`, but its operand byte is **not** consumed, so it is read as the next raw-wire request.”
   >
   > “A start (`0xF2`–`0xFF`) leaves its operand byte unread, so the operand is then parsed as the next raw-wire request: `0xF2 0x00` answers `0x00` and then leaves raw wire (`BBIO1`); `0xF2 0x58` answers `0x00` and then treats `0x58` as a pull-up request.”
   >
   > “On a build without SMPS support every request is refused with `0x00`, and a start still consumes its operand byte so the following request stays aligned.”
   >
   > “Symptom: `0x00`, with the operand byte consumed rather than read as the next request. Behaviour: the no-SMPS start path still reads its operand before it reports the refusal, so the following byte is the next command.”

   These cannot both be true. One side says the next byte is a command; the
   other says it is the start operand. A host that sends `0xF2 0x58` and then
   a command will either eat the command as an operand or feed `0x58` as a
   pull-up request. The SMPS edge-case page even gives the second behavior as
   a concrete example, so this is not just a wording difference.

   Fix: pick the implemented behavior (the binary-layer pages say the operand
   is consumed and the frame stays aligned), update `smps/binary.md` and
   `smps/edge-cases.md`, and add a two-line refusal exchange:
   `host -> 0xF2 0x58` / `host <- 0x00` / `host -> 0x01` (reprints `RAW1`).
   Delete the `0xF2 0x58` → pull-up example.

### Major

3. **[major]** — `docs/capabilities/auxiliary/aux-pin.md:169-172`,
   `docs/capabilities/auxiliary/pwm-servo.md:227-230`,
   `docs/capabilities/auxiliary/edge-cases.md:68-80` vs
   `docs/generic/commands.md:133-135`, `docs/protocol/sump/entry.md:128-132`
   > “Entering SUMP also makes the AUX pins inputs, but the SUMP exit returns to HiZ without running the board reset. The target assignment and the generator state therefore survive a SUMP exit; use a reset or a new command to clear them.”
   >
   > “Entering SUMP makes the AUX pins inputs and removes a running waveform, but the SUMP exit does not run the board reset. The generator state survives the exit until a reset or a new generator command.”
   >
   > “After a board reset (`#`, `m` / `m1`, a mode change, the binary return or the SUMP exit) the default target is `A0` — the pin shown as `AUX`.”
   >
   > “All three exit bytes stop the analyzer, park the board in `HiZ`, print the version banner and the `HiZ>` prompt, and run the terminal reset (pins to inputs, target supplies and pull-ups off, display back to HEX).”

   The AUX set is the odd one out. A user leaving SUMP is told the target and
   generator state survive; the generic-command page and the SUMP entry page
   say the exit runs the terminal reset. That changes whether the next `a`
   hits `AUX1`/`CS` or `AUX`, and whether a bare `g` is a stop or a new setup.

   Fix: remove the “survive” claims from `aux-pin.md`, `pwm-servo.md` and
   `edge-cases.md`; state SUMP entry makes the pins inputs and the exit runs
   the terminal reset (target back to `A0`, generator state cleared). If there
   is a v3/v4 difference, say which board and which reset path.

4. **[major]** — `docs/capabilities/auxiliary/binary.md:84-87`,
   `docs/capabilities/auxiliary/pwm-servo.md:124-125`, `:219-221` vs
   `docs/protocol/binary/commands.md:213-215`, `:281-287`
   > “On v4 it returns to the terminal and runs the board reset, which detaches the output and clears the generator state; on v3 it is a hardware reset.”
   >
   > “board reset detaches the output and clears the generator state; no `PWM disabled` line”
   >
   > “A PWM timer setup survives this reset; see [PWM](#pwm-0x12-0x13).”
   >
   > “A PWM or servo left running in the terminal is not cleared by entering BBIO1: entry makes the pins inputs, so the waveform no longer drives the header, but the timer setup stays until `0x12`, `0x13` or a hardware reset.  `0x0F` does not stop the PWM timer either; on v4 the terminal reset detaches the AUX output source, so the target stops being driven while the timer may still run…”

   The reader is asked to hold “clears the generator state” and “timer setup
   survives” at the same time. Without a definition of “generator state” vs
   “timer setup”, the practical questions are unanswerable from the docs:
   after `0x0F`, is a bare `g` a stop or a new setup? Is `0x13` required
   before leaving? The stop table in `pwm-servo.md` says a board reset clears
   the state, while the binary page says the timer survives.

   Fix: define the three layers once and use the same sentence everywhere:
   (a) the terminal generator flag / active mode, (b) the `AUX` output
   routing, (c) the Timer2/OC5 registers. Then say: a v4 terminal reset
   clears (a), detaches (b), and does not explicitly stop (c); send `0x13`
   before `0x0F` if the timer must be idle. After a reset a bare `g` starts a
   new setup. That preserves the binary page’s warning without contradicting
   the terminal page.

5. **[major]** — `docs/capabilities/auxiliary/frequency.md:75-79`,
   `:88-93`
   > “A sub-1 Hz reading can take longer because the period path waits for whole cycles; the acquisition gives up after about five seconds without the needed edges.”
   >
   > “A result from 1 to 3999 Hz goes through a second `autorange` step: the firmware measures the period over about one second of cycles and prints a decimal value.”
   >
   > “If the 1:1 count is zero, the firmware prints the `Frequencies < 1Hz are not supported.` line after the first `autorange` step.”

   The page says the acquisition gives up, but never says what the user sees
   when it does. The documented zero-count path prints one `autorange`; a
   slow signal that caught an edge in the 1:1 gate but times out in the
   period capture goes through the second `autorange` and then has to print
   something. The page’s “no edges” example covers only the zero-count path,
   so a user cannot predict the output or the number of `autorange` tokens
   after the five-second wait.

   Fix: add a timeout row or bullet: “If the period capture reaches the
   five-second bound without enough edges, the value is the same
   `Frequencies < 1Hz are not supported.` message, after the second
   `autorange`; no frequency is printed.” Also say whether the five seconds
   is the whole command or just the period path (the two one-second gates
   come before it), so the total wait is not misread as 5 s.

6. **[major]** — `docs/capabilities/auxiliary/aux-pin.md:49-51`,
   `docs/capabilities/auxiliary/edge-cases.md:29`,
   `docs/generic/navigation.md:69-71`
   > “In a bus mode, `i` prints the status block including the current confirmation line.  `v` shows pin directions and levels, not the selection; do not use a selection key just to check the target, because the key changes it.”
   >
   > “none; check the confirmation line when in doubt”
   >
   > “`i` prints the banner.  In HiZ that is all; in any other mode it appends a status block.”

   The docs correctly warn that the reply text always says `AUX`, but they
   do not give a safe way to recover the target at the one prompt where a
   user is most likely to be working: `HiZ>`. `i` does not show the status
   block there, and re-issuing a selection key changes the target. If the
   confirmation line has scrolled away, the reader cannot answer “which pin
   am I touching?” without changing something.

   Fix: add a short “checking the target” box: “At `HiZ>` there is no
   non-destructive readout. `#` on v4 (hardware reset on v3) returns the
   assignment to `AUX`; or select the target you want with `c`/`C` and read
   the confirmation line. In a bus mode, `i` shows the current target.” Add
   the same qualification to the DIO page’s unqualified “Run `i`.”

7. **[major]** — `docs/capabilities/auxiliary/binary.md:156-159` vs
   `docs/protocol/binary/commands.md:67-70`
   > “If a host needs a clean reply for the drive/release operations, use the terminal `a` / `A` / `@` commands or the BBIO1 pin controls instead.”
   >
   > “There is **no read-only five-pin command**.  Every state byte writes all five latches plus the supply and pull-up bits, then reports the levels; every direction byte rewrites all five directions, then reports the levels.”

   The suggested BBIO1 fallback is not a per-pin clean alternative. A state
   byte rewrites all five latches and switches the supply and pull-ups; a
   direction byte rewrites all five directions. A host that takes this advice
   to avoid the `0x09` text can clobber the other bus lines or the target
   supply. The only clean one-pin control is the terminal `a`/`A`/`@`.

   Fix: drop “or the BBIO1 pin controls”. Say “use the terminal `a`/`A`/`@`
   (one pin only), or parse the text; there is no per-pin BBIO1 write.” If a
   host really must use BBIO1, add the warning that all five lines and the
   supply/pull-up bits are rewritten.

8. **[major]** — `docs/capabilities/auxiliary/edge-cases.md:13-53`
   Example row, `:31`:
   > “| a read, frequency measurement or edge wait with nothing driving the pin | the level wanders, the frequency reads zero, or the wait times out unpredictably | the pin is high impedance as an input, and the firmware does not enable a bias for it | fit an external pull-up or pull-down to a permitted logic rail, or connect a source that always drives a defined level |”

   The terminal and binary failure-mode sections are 4-column tables with
   40 rows; most cells are full sentences and several rows are 200–367
   characters wide. In an 80-column terminal or a narrow browser pane the
   trigger, symptom, behaviour and recovery do not line up visually, so the
   table is harder to use than the bullet chains it replaced. It is a wall of
   text, not a readable failure index.

   Fix: split the page by function (terminal AUX, PWM/servo, frequency,
   binary AUX) and use one short block per case:
   `Trigger:` / `You see:` / `Why:` / `Do:` on four lines. If a table is
   required, use a two-column `Symptom | Do this` table and move the
   explanation to the topic page. At minimum, keep each cell to a short
   phrase and link the detail.

9. **[major]** — `docs/capabilities/smps/edge-cases.md:73-78`,
   `docs/capabilities/smps/binary.md:21-24`, `:110-112`,
   `docs/capabilities/smps/README.md:32-35`
   > “`AUX1` no longer follows the `a`/`A` controls, and a stop detaches the PWM — the pin keeps its port direction and output latch state, so from a GPIO-output state it stays an output at its last level.”
   >
   > “Stopping turns the ADC off, disables its interrupt and auto-sampling, sets the duty and period to 0, disables the output-compare channel, and frees the `AUX1` pin.”
   >
   > “a hardware reset also stops it, but whether the terminal reset that returns from the binary layer stops it is `unverified`”
   >
   > “A hardware reset also stops it, and the v4 terminal reset stops it through the board reset.”

   Two state claims conflict. “Frees the `AUX1` pin” reads as input/released;
   “keeps its port direction and output latch state” says the opposite. The
   same set also cannot decide whether the v4 terminal reset stops the SMPS
   loop: one page calls it unverified, two pages say it stops. This matters
   for the AUX reader because `pwm-servo.md` tells them to use a board reset
   or `0x0F` to recover from an SMPS-owned channel.

   Fix: state one answer for each: (a) terminal reset stops the loop (the
   README and binary edge-cases already say so); (b) stop leaves `AUX1` as an
   input, not at its last GPIO level, if that is the implemented behavior.
   If either differs by build, state the build. Remove the “unverified” and
   the GPIO-last-level claim once resolved.

10. **[major]** — `docs/capabilities/smps/edge-cases.md:82-90` vs
    `docs/capabilities/adc/terminal.md:126-130`,
    `docs/capabilities/adc/binary.md:132-137`
    > “run `d`/`D`, or binary `0x14`/`0x15`, while the SMPS is running… a probe command also turns the converter off at the end, so the feedback loop stops updating until the next start.”
    >
    > “On an SMPS-enabled v4 build, a running SMPS feedback loop owns the probe channel… The terminal probe commands are refused.”
    >
    > “`0xFF 0xFF` is sent once… `0x15` does **not** stream after the pair; the layer is ready for the next command.”

    One page says the probe reads the SMPS feedback and stops the loop; the
    ADC pages say the probe is refused and the loop keeps ownership. A user
    who types `d` to “take the channel back” will be surprised by a refusal,
    and a host that treats the `0xFF 0xFF` pair as a reading is already
    covered by the ADC page but contradicted here.

    Fix: align the SMPS page with the ADC pages: the probe commands are
    refused while the SMPS owns the converter; binary returns `0xFF 0xFF`
    once and does not stream; the loop continues until `0xF1` or a reset.
    Delete “the reading follows the SMPS feedback” and “turns the converter
    off” from the SMPS edge case.

### Minor

11. **[minor]** — `docs/capabilities/auxiliary/pwm-servo.md:180-189`
    > “| Position | Compare ticks | Pulse width |”
    > “| 0° | 62 | 0.99 ms |”
    > “| 90° | 94 | 1.50 ms |”
    > “| 180° | 126 | 2.02 ms |”
    >
    > “The compare value is what the firmware writes.  On v4 the output high time is one timer tick longer than that value; on v3 the value is used directly (unverified).”

    The table's “Pulse width” column is computed from the compare ticks, not
    from the v4 output. On v4 the actual high times are 63/95/127 ticks, or
    about 1.01/1.52/2.03 ms. A reader checking a servo datasheet sees a
    16 µs offset and a table that contradicts the paragraph below it.

    Fix: rename the column to “Compare ticks / nominal width” and add a
    “v4 output” column (63/95/127 ticks; 1.01/1.52/2.03 ms), or put a caption
    on the table: “compare values; v4 output is one tick longer.”

12. **[minor]** — `docs/capabilities/auxiliary/frequency.md:48-50`,
    `:85-86`, `:99`; `docs/capabilities/auxiliary/binary.md:93-96`
    > “A reading at or above 4.194304 MHz skips the rerun and prints no `autorange`; a reading below 4 kHz takes both steps and prints it twice.”
    >
    > “If the count is 16383 or less, the firmware prints `autorange`, switches to the 1:1 prescaler and measures again.”
    >
    > “From 65,536 Hz upward the single prescaled gate is used and the result is a multiple of 256 Hz.”

    The contract is a count of edges in the gate, not an input frequency. At
    exactly 4.194304 MHz a one-second gate can contain 16,383 or 16,384
    prescaled counts depending on phase; at exactly 65,536 Hz it can contain
    255 or 256. The “at or above” wording is therefore not guaranteed at the
    boundary.

    Fix: state the count rule as the contract and the Hz values as nominal:
    “no rerun when the first gate counts 16,384 or more (terminal) / 256 or
    more (binary), i.e. nominally at or above 4.194304 MHz / 65,536 Hz.”
    Keep the practical Hz note, but do not make the Hz value the trigger.

13. **[minor]** — `docs/capabilities/auxiliary/aux-pin.md:62-63`
    > “The `?` help labels the group `AUXPIN (low/HI/READ)`; the replies and the `v` table print the pin as `AUX`.”

    The `v` table prints the selected pin by its own name: `AUX`, `CS`,
    `AUX1`, `AUX2`. It does not print every pin as `AUX`. The next bullet
    correctly says the *reply text* always says `AUX`; this sentence blurs
    the two.

    Fix: “the replies print the literal `AUX`; the `v` table prints the
    selected pin's own name (`AUX`, `CS`, `AUX1`, `AUX2`).”

14. **[minor]** — `docs/capabilities/auxiliary/binary.md:63-64` vs
    `docs/protocol/binary/commands.md:265-266`,
    `docs/protocol/binary/examples.md:144-145`
    > “Example: prescaler `2` (1:64, tick 250 kHz), duty `0x0063` (99) and period `0x00C7` (199) give about 1250 Hz with a 50 % high time on v4”
    >
    > “Example: prescaler `2` (1:64, tick 250 kHz), duty `0x0064` (100), period `0x00C7` (199) gives about 1250 Hz at about 50 %”

    The same worked example uses two duty bytes. On v4, 99 gives exactly
    50 % (99 + 1 = 100 high ticks of 200), while 100 gives 50.5 %. A reader
    comparing pages cannot tell which is the intended example, and a host
    copying `0x64` gets a different duty than the AUX page promises.

    Fix: use `0x0063` for the exact 50 % example everywhere, and either add
    “`0x0064` is 50.5 % on v4” or remove the second example.

15. **[minor]** — `docs/capabilities/auxiliary/README.md:3-4`, `:70-76`,
    `:78`; `docs/capabilities/auxiliary/wiring.md:158`, `:163`
    > “**Status:** documented for the v3 and v4 builds; the v3 column is build-only here and is not verified on v3 hardware.”
    >
    > “| `a` / `A` / `@`; `c` / `C` | present, build-only | present |”
    >
    > “The v3 column is build-only here and **not verified on v3 hardware**.”

    “Build-only” is undefined. It could mean “present in the build but not
    the hardware,” “documented from the build only,” or “not in the default
    image.” For a v3 owner, the first line is the applicability answer, and
    it is the least clear phrase in the README. The same caveat is then
    repeated at the top, inside six table rows, and again below the table.

    Fix: write “present in the v3 build; not tested on v3 hardware” once in
    the status line, use a short “v3 untested” marker in the table, and drop
    the repeated closing sentence. `docs/TEMPLATE.md:55-61` asks for at most
    a short inline caveat; this is repeated verification-status prose.

16. **[minor]** — `docs/capabilities/auxiliary/README.md:21`, `:25-33`,
    `:37-45`, `:110-120`
    > “This README is a map, not the manual; details live in the files below.”

    The README has three overlapping indexes: the Files table, the Command
    map, and Related docs. They are all useful, but a fresh reader has to
    choose between them, and there is no “read in this order” line. The ADC
    set does this better: “Read in this order: wire, take one reading,
    understand the scale, then script it” (`adc/README.md:26-29`).

    Fix: keep the Files table and one command table; replace Related docs
    with a short “See also” line. Add a four-step path: wiring → pin/select →
    PWM or frequency → binary/edge cases. That also shortens the README below
    its current 121 lines.

17. **[minor]** — `docs/capabilities/auxiliary/edge-wait.md:60-61`,
    `:67-68`
    > `host -> 0x70 0x01 0x03 0xE8    wait for rising, 1000 ms`
    > `host <- 0x01 0x01             edge seen, pin is high`

    The comments start at column 32 on the `host ->` line and column 31 on
    the `host <-` line; the same one-space error appears in the second
    exchange. `protocol/binary/examples.md:14` says “Comments start in the
    same column in every block,” so this is a visible transcription slip.

    Fix: add one space after the `host <-` value (or remove one from the
    `host ->` line) so both comments start at the same column.

18. **[minor]** — `docs/capabilities/auxiliary/pwm-servo.md:73`;
    `docs/protocol/binary/commands.md:228`
    > “`DUTY` ([binary.md](binary.md), [BASIC board pins](../../tools/basic/board-pins.md)).”
    >
    > “errors).  If the SMPS owns the ADC, the reply is `0x01` and no check runs.  While the test is idle it reads input — any byte other than `0xFF`”

    The first line is 85 characters and the wrap is broken mid-sentence; the
    second is 142 characters and runs two separate self-test facts together.
    Both are prose lines, not tables, so the ~80-column rule applies.

    Fix: rewrap the `pwm-servo.md` sentence so the link is on its own line,
    and split the self-test paragraph at “If the SMPS owns the ADC” into a
    new sentence/paragraph.

19. **[minor]** — `docs/capabilities/auxiliary/README.md:76`
    > “| SMPS owns the output channel | — | optional build |”

    The AUX README says `optional build`, but the SMPS README says it is
    “off in the default image” (`smps/README.md:3-5`). A v4 owner on the
    default image may read the AUX page and expect the `g`/`S` refusal to
    apply, then be confused when it does not. The refusal only exists on an
    SMPS-enabled build.

    Fix: “optional build; off in the default image” in the AUX availability
    table, matching the SMPS page.

## What to keep

- The README's five-minute version and command map. I could pick a command
  and reach the owning page in one click; the first line (“no AUX mode”) saves
  a search.
- The explicit token-to-pin mapping in `generic/commands.md` and the repeated
  warning that `a`/`A`/`@` replies always say `AUX`. That is the single best
  safeguard against a wrong-pin mistake.
- The PWM/servo stop table (`pwm-servo.md:116-128`). It answers the
  bare-`g`-is-not-a-no-op trap, the `PWM disabled`-then-`PWM active` sequence,
  the silent stop on invalid `S`, and the `w`/reset/`0x13` differences in one
  place.
- The frequency display notes: the doubled `autorange`, the two spaces, the
  thousands separators, and the explicit terminal-vs-binary threshold split.
  Without those I would have thought the output was garbled.
- The `0x70` payload/reply table and direction contract. The `0x70` frame,
  the `0x00` timeout read trick, and the “does not change direction” rule are
  executable as written.
- The wiring page's “signal, not a supply” framing and the v4 pin table
  (`AUX` 5, `AUX1` 6, `AUX2` 7, `CS` 4). The pin numbering is consistent with
  the ADC and SUMP pages.
- The “f is not the ADC” separation and the wiring headline that no `W`/`P`
  is needed. That avoids the usual bus-pirate supply confusion.
- All relative file links in the AUX set resolve. There were no dead links.

## Structural opinion

The one-page-per-function split is right for this capability and should stay.
The weak points are duplication and page weight, not the basic shape.

1. **Make the README a two-index map.** Keep the Files table and a compact
   command table; replace Related docs with a “See also” line. Add a four-step
   reading order (wire → select/drive → generator or counter → binary/edge
   cases). The current three indexes are all useful, but a new reader has to
   choose between them.

2. **One authority per binary contract.** `binary.md` currently summarizes
   `0x70` and `0x09`, while `edge-wait.md`, `protocol/binary/commands.md` and
   `protocol/i2c/binary.md` own overlapping pieces. That is how the `0x09`
   text and SMPS operand contradictions survived. Pick one owner per command
   (for example, `auxiliary/binary.md` for `0x12`/`0x13`/`0x16`/`0x70`,
   `protocol/i2c/binary.md` for `0x09`), and make every other page say
   “summary; authority is X” with no byte-level table of its own.

3. **Split `edge-cases.md` by function.** The 4-column table is the only page
   that is genuinely hard to read. Use short per-case blocks:
   condition, what you see, why, what to do. If the table must stay, make it
   two columns (`Symptom | Do this`) and move the explanation to the topic
   page. The current row lengths make a failure lookup slow, which is exactly
   when a reader needs it fast.

4. **Add one “measurement contract” table to `frequency.md`.** Columns:
   input band, first gate, rerun condition, display, `autorange` count,
   elapsed time. That would answer the whole task-4 question at a glance and
   make the 5 s timeout and the count-vs-Hz boundary explicit.

5. **Add a target-state box to `aux-pin.md`.** Put the confirmation strings,
   the `i` rule (bus mode only), the `v` rule (names, not selection), and the
   reset-to-`A0` behavior in one small table. This is the page where a wrong
   target is most likely to damage a session, and the information is currently
   spread across two sections and the DIO page.

6. **Reduce the v3 caveat to one table.** Keep a single “v3: present in the
   build, not tested on hardware” note in the README and the full availability
   table; drop the closing sentence from every topic file. The caveat is
   honest and should stay, but repeating it at the bottom of each page makes
   the set look less certain than the evidence warrants.

## Unknowns

- Whether the v3 hardware actually behaves as documented. The pages say the
  v3 column is “build-only” and unverified; I cannot judge that as a reader,
  and the repeated caveat does not tell me which specific behaviors are most
  likely to differ.
- The exact absolute maximum on the AUX pins and the maximum frequency input.
  The docs deliberately say these are not documented; I could not fill them
  in from these pages.
- Whether the servo table's 0.99/1.50/2.02 ms are the actual v4 output or the
  compare values. The table and the following paragraph disagree by one timer
  tick, so this needs a scope or a firmware confirmation.
- Whether a `0x70` timeout of `0x0000` can ever return `0x01` because an edge
  lands between arming the change notification and the first sample. The docs
  say it always returns `0x00`; that is the kind of race I cannot judge
  without hardware.
- Whether `f` and `0x16` have any electrical or timing effect on a running
  SMPS. The frequency page says the SMPS “does not block” them, but that is
  not the same as “does not disturb”; the shared timer and output-compare
  ownership are not discussed.
- Whether the `0x09` drive/release text is present on v3 as well as v4. The
  AUX page shows it as the terminal string; the I2C page does not mention it;
  the v3 behavior is unverified.
- Whether `i` always includes the AUX confirmation line in every bus mode.
  The docs say it does; I did not see a mode-specific exception in the AUX
  pages.
- Whether the SMPS stop path leaves `AUX1` as an input or at its last GPIO
  level. The SMPS pages say both; I could not resolve it from the docs alone.
