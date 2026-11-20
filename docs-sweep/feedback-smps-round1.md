# SMPS docs — fresh-reader feedback (round 1)

## Verdict

**Yes with fixes.** The map, the byte encoding and the loop arithmetic are strong,
but a disabled-build alignment error, an unresolved add-on identity/pinout gap,
and a missing freshness rule for `0xF0` will mislead a first-time user.

## Task log

### 1. First impression from the README

- **Where I looked:** README top, "The five-minute version", "Availability",
  "Wiring headline".
- **How fast:** Under two minutes to understand that SMPS is an optional v4
  build driving a legacy external add-on; under a minute to learn that the
  default v4 image has it off and v3 cannot enable it.
- **Success:** Partial.
- **What was unclear:** I could not tell whether the firmware already flashed on
  my v4 has the feature; the README has no check and the disabled-build section
  only explains the reply. The add-on is named only as "a legacy Dangerous
  Prototypes switched-mode power-supply add-on" — no model, revision,
  connector or supply rating — so I cannot confirm that the board in my hand is
  the one described. The README gives the start bytes before the wiring
  headline, so my first instinct was to start a supply before checking the
  wires.

### 2. Enter `RAW1`, start, read, stop

- **Where I looked:** README "Smallest safe session"; `binary.md` "Requested-value
  encoding", "Worked exchanges", the Python host.
- **How fast:** The encoding took seconds: `0x0258` is `0xF2 0x58`. The stop and
  leave sequence is easy to follow.
- **Success:** Yes for the byte sequence. No for "read a sample" as a fresh
  measurement: the first `0xF0` after a start can return the previous run's
  sample, and there is no instruction to wait, discard, or poll for a fresh
  conversion. The worked exchange shows `0xF0` immediately after the start and
  gets a plausible `0x01 0xD3`; I would have believed that was the new output.
- **What was unclear:** How long to wait for the first new sample; how to tell a
  stale sample from a fresh one; whether `0x01` from a start means only "setup
  applied" or also "output stable". The docs say the former, but the example
  looks like the latter.

### 3. Feedback loop, ranges and sample meaning

- **Where I looked:** `feedback-loop.md` (arithmetic table, "The sample",
  "Where the loop can regulate", "Ownership"); README "Limits"; `binary.md`
  "Limits"; ADC probe page for the scale.
- **How fast:** The arithmetic table is the fastest part of the set. I checked
  all five rows and the `0x526`/`0x527` boundary; the values agree with the
  formulas. The strict comparison and bang-bang behaviour are clear.
- **Success:** Mostly. The closed-loop/open-loop boundary is easy to see. But two
  things did not sit right:
  1. "Regulation is possible only for requests `0x200`-`0x526`" and "the useful
     closed-loop range" read as claims about the add-on's regulated output, when
     the docs elsewhere say the add-on's range and scaling are unknown.
  2. "target ... in ADC counts ... 3177" is not a possible 10-bit count. I had to
     work out that it is a wider comparison threshold, not an ADC count.
- **What was unclear:** What the reading means for the add-on output (the docs
  correctly refuse to map it, but I wanted a clearer "this is the ADC-node code,
  not the output voltage" sentence); whether any request actually regulates on
  my board.

### 4. Ownership, refusals and exit paths

- **Where I looked:** `feedback-loop.md` ownership table; `exit.md`; binary
  edge cases; ADC binary/terminal; AUX PWM/servo; self-test binary.
- **How fast:** The exit table is excellent: `0xF1` vs `0x00` vs `0x0F` vs `#`
  vs `m` vs hardware reset, including what happens to the stored sample, is
  distinguishable in one scan. The `0xF1`-in-`BBIO1` trap is also clear.
- **Success:** Yes for the exit paths and the binary `0x14`/`0x15` refusal. The
  self-test row stopped short: it says "refusal count `0x01`; the test does not
  run", but after that byte the self-test is in its idle echo loop. I only
  learned that from the linked self-test page; the SMPS table left me thinking I
  could send the next command.
- **What was unclear:** How to leave the self-test after the refusal without
  reading another page; whether the terminal `d`/`D` refusal is reachable at all
  (the SMPS pages say it is defensive and normal routes stop the loop, so I could
  not reproduce it); why terminal `W` is refused while the loop runs when the
  wiring page says `W` is not part of the SMPS group.

### 5. Wiring and one edge case

- **Where I looked:** `wiring.md`; AUX wiring; ADC wiring; `edge-cases.md`
  "No add-on, or no feedback connected".
- **How fast:** The Bus Pirate side is quick: `AUX1` pin 6, `ADC` pin 8, `GND`
  pin 12, 3.3 V logic, 0-6.6 V on `ADC`. The "what not to connect" table is
  actionable.
- **Success:** Partial. I can wire the Bus Pirate side. I cannot confidently wire
  the add-on side: the connector pinout, supply voltage/current and output range
  are external and not identified. The quick start says "No pull-up ... needed",
  while the wiring page says a previous open-drain state needs one, and there is
  no recovery. The no-feedback edge case is clear about the symptom and the
  recovery, but not about the output: with the loop still switching, the PWM may
  sit at the computed duty and the output is uncontrolled.
- **What was unclear:** Which add-on pin is PWM/feedback/ground/power; what
  supply to use; whether push-pull is guaranteed after a start; what happens to
  the add-on output when the feedback wire opens.

### 6. Rendering check

- **Where I looked:** All six files in `docs/capabilities/smps/`, plus the
  cross-linked `protocol/binary/`, `adc/` and `auxiliary/` pages.
- **What I found:**
  - No TAB characters in any doc file (checked the whole `docs/` tree).
  - Captures in `binary.md` and `exit.md` align at column 26; the ADC and binary
    examples align within their blocks. No broken capture columns.
  - No bench prose, run dates, pass counts, "captured on" provenance or
    "tested/verified" result claims in the SMPS set. The `unverified` and
    "not verified on v3" lines read as caveats, not test reports.
  - Two naming mismatches: the same `#` reset is "terminal reset" in one table
    row and "Bus Pirate reset" in the next (`exit.md:112-113`), and `0xF0` is
    "feedback sample" in the README but "output sample" in the command table
    (`README.md:44`, `binary.md:40`).
  - "Reply shapes" is a six-bullet result chain (`binary.md:75-88`); a table
    would be faster and would not duplicate the command table as much.
  - I did not count table rows over 80 columns as wrap defects; they render as
    tables. The SMPS prose itself wraps cleanly.
- **Success:** Rendering is clean overall; the two naming drifts and the bullet
  chain are the only presentation issues I would fix.

### 7. README navigation test

- **Where I looked:** README map table, five-minute version, smallest session,
  related-docs table.
- **How fast:** One click each to binary commands, feedback loop, wiring and
  exits; about five seconds per target. That is very good.
- **Dead ends:** Add-on identity/pinout (no link or model); build-presence check
  (no way to tell from the README whether my firmware has SMPS); "safe session"
  starts before the wiring headline.
- **Doubling back:** I re-read the same stop/sample rules in README, `binary.md`,
  `feedback-loop.md`, `exit.md` and `edge-cases.md`. The duplication is not fatal,
  but it is where the `0xF2 0x00` contradiction lives.

## Findings

### 1. [severity: blocker] Disabled-build start consumes `0x00` too — the second example is wrong

**Location:** `docs/capabilities/smps/binary.md:139` — "If the operand is
`0x00`, that byte is read as the next request and leaves raw wire:" followed by
`host <- "BBIO1"`. The same claim is at `docs/capabilities/smps/edge-cases.md:14`
— "`0xF2 0x00` gives `0x00`, and the `0x00` operand then leaves raw wire."

**Problem:** This contradicts the same page's rule at `binary.md:129` ("a start
consumes its operand before replying") and the same edge-case paragraph
("**consumes** its operand"). The firmware's disabled path reads and discards
one byte for every `0xF2`-`0xFF`, including `0x00`. After `0xF2 0x00` on a
disabled build the host is still in `RAW1`, not `BBIO1`. A host that follows the
example will believe it left and send the next bytes into the wrong layer.

**Fix:** Delete the `0xF2 0x00` example. State that the operand is always
consumed, including `0x00`, and that the next byte is the next `RAW1` request.
Correct the same sentence in `edge-cases.md`.

### 2. [severity: major] The add-on is a dead end: no board identity, connector or supply rating

**Location:** `docs/capabilities/smps/README.md:89` — "The add-on's own
connector, supply, scaling and output range are part of the external board and
are not defined here (`unverified`); do not derive a connector map from these
pages." Also `docs/capabilities/smps/wiring.md:89` — "The add-on's connector and
pinout are not defined by the firmware. Only the Bus Pirate pins above are
firmware-defined; do not derive a connector map from these pages."

**Problem:** I can wire the Bus Pirate side, but I cannot identify the add-on I
need or connect its PWM, feedback, ground and power pins. "Legacy Dangerous
Prototypes switched-mode power-supply add-on" is not a model or revision. The
docs correctly refuse to invent a connector map, but they offer no next step.

**Fix:** Add a short "Add-on identification" section: board name/model/revision
if known, a link to its schematic or connector if one exists, and an explicit
"this repo does not contain it; obtain X" if not. List the functional
requirements (PWM input, feedback output, ground, supply) so a user can match a
board. If no board can be identified, say so plainly and do not imply the wiring
page is complete.

### 3. [severity: major] The regulation range is stated as if the add-on's output range were known

**Location:** `docs/capabilities/smps/README.md:98` — "Regulation is possible
only for requests `0x200`-`0x526` ..."; `docs/capabilities/smps/binary.md:229`
— "The useful closed-loop range is `0x200`-`0x526`"; `feedback-loop.md:104` —
"Where the loop can regulate".

**Problem:** These read as statements about the add-on's regulated output. The
firmware's cutoff can only act in that request range; whether the add-on can
reach any target, and whether any request is "useful", is exactly what the docs
say is unknown. The caveat at `feedback-loop.md:116` ("the add-on must be able to
drive its feedback node") is buried and is not carried into the README or the
limits.

**Fix:** Everywhere say "the firmware's cutoff can act only for requests ..."
and add "whether the add-on reaches the target is unknown; this is not a claim
that any request produces a regulated output." Drop "useful".

### 4. [severity: major] "Smallest safe session" is not a safety claim I can rely on

**Location:** `docs/capabilities/smps/README.md:53` — "## Smallest safe session";
`README.md:59` — "start, requested `0x0258`".

**Problem:** The requested value has no defined unit and the add-on's output is
unknown; `0x0258` is not the smallest reachable value (`0x200` is) and is not
shown to be safe. A reader may connect a load and follow the table. The safety
caveats appear later, under "Limits".

**Fix:** Retitle "Shortest byte sequence (not a safety claim)". Use `0x200` for
the first start, say the output is undefined, and put the "load disconnected /
verify the output / no protection" warning immediately before the table. Keep
"safe" out of the heading.

### 5. [severity: major] `0xF0` has no freshness rule; the worked exchange shows a fresh-looking sample immediately after start

**Location:** `docs/capabilities/smps/binary.md:103-104` — `host -> 0xF0` then
`host <- 0x01 0xD3` right after a start; `feedback-loop.md:81` — "A start does
not clear the stored sample. The first read after a new start can still be from
the previous run."

**Problem:** The exchange is the model a host author will copy. It reads a
plausible sample immediately, but the first read can be the previous run's
value, and there is no freshness flag or timestamp. The text says `0xF0` "does
not take a new sample" but does not say what to do about that.

**Fix:** In the worked exchange, show the first read as stale or zero (or add
"after at least one conversion"), and add: "There is no freshness flag; after a
start, discard the first read or poll until the value changes before treating it
as the new output." If the conversion cadence is known, state it; otherwise say
"poll until it changes".

### 6. [severity: major] "No pull-up ... needed" contradicts the open-drain caveat, and the caveat has no recovery

**Location:** `docs/capabilities/smps/README.md:49` — "No pull-up and no Bus
Pirate switched supply are needed." vs `docs/capabilities/smps/wiring.md:24-28`
— "**No pull-up is required** ... in the normal case. ... If a previous mode left
`AUX1` open drain, a logic high is a released pin and the add-on needs its own
pull-up (`unverified`)."

**Problem:** The quick-start bullet is absolute; the wiring page says the
requirement depends on the pin's output type. The page never names a mode that
leaves `AUX1` open drain, never says how to check it, and never says how to clear
it. I cannot tell whether "no pull-up" applies to my board.

**Fix:** Add "in the normal (push-pull) case" to the README bullet. In wiring,
name the condition (for example, the OpenOCD open-drain mode) and the recovery
(what command or reset restores push-pull; if only a power cycle does, say so).
If the SMPS start does not force push-pull, state that precondition explicitly.

### 7. [severity: major] Self-test refusal omits the idle echo loop; a host can get stuck

**Location:** `docs/capabilities/smps/feedback-loop.md:132` — "| `0x10` /
`0x11` | `BBIO1` | refusal count `0x01`; the test does not run | [self-test
binary](../selftest/binary.md) |".

**Problem:** After the `0x01`, the self-test sits in its idle loop: every byte
except `0xFF` is echoed back with the error count added, and only `0xFF` leaves
(without re-printing `BBIO1`). The row says "the test does not run" but not that
the layer is now in a different loop. A host that sends its next command gets an
echo, not an error, and loses alignment. The linked self-test page has the
detail, but this ownership table is where the case is introduced.

**Fix:** Change the row to "refusal count `0x01`; no check runs; the self-test
then sits in its idle echo loop — send `0xFF` to leave (`BBIO1` is not
re-printed)". Add the same sentence to the SMPS edge-cases entry.

### 8. [severity: major] "Target ... in ADC counts ... 3177" is not a valid 10-bit count

**Location:** `docs/capabilities/smps/feedback-loop.md:53` — "`target` is a raw
feedback threshold in ADC counts, not a voltage. It ranges from 397 at `0x200`
to 3177 at `0xFFF`."

**Problem:** The sample is 10-bit (`0`-`1023`), so a target of `3177` cannot be
an "ADC count"; it is a comparison threshold on a wider integer. The sentence is
self-contradictory and makes the open-loop boundary harder to understand.

**Fix:** Say "`target` is a raw comparison threshold on the same numeric scale
as the 10-bit sample; values above 1023 can never be reached, which is why
requests above `0x526` are open loop." Or call it a "target code" and define it
as 16-bit.

### 9. [severity: major] The no-feedback edge case does not say the output is uncontrolled

**Location:** `docs/capabilities/smps/edge-cases.md:67` — "start with no add-on,
or with the feedback wire open"; `edge-cases.md:70` — "It samples whatever is on
`ADC` and keeps switching."

**Problem:** "Keeps switching" is not always true: if the floating feedback node
reads above the target, the strict comparison cuts the duty to zero; if it reads
below, the PWM stays at the computed duty. Either way the output is not
regulated, and the page does not say so. A reader can see a zero sample and
assume 0 V output, or see the PWM "running" and assume regulation. The recovery
("stop the loop and check the wiring") is right but does not say to remove the
load or treat the output as live until the feedback path is known good.

**Fix:** Say "the duty follows the floating reading: it may sit at the computed
duty or be cut to zero; the output is uncontrolled" and add "remove the load /
do not touch the output until the feedback path is verified; a zero sample is not
proof of 0 V output." Keep the existing recovery sentence.

### 10. [severity: minor] No build-presence check in the README

**Location:** `docs/capabilities/smps/README.md:37-38` — "**v4 only, off in the
default image.** Enabling SMPS is a build change ...".

**Problem:** I can learn that the default image is off, but not whether the
firmware on my board has SMPS. The README gives no check; the disabled-build
section explains the reply but not how to test for the feature, and `0xF0` is
ambiguous if read as one byte (an enabled build before start also returns
`0x00 0x00`).

**Fix:** Add a one-line check: "In `RAW1`, send `0xF1`: `0x01` = SMPS built in;
`0x00` = not built. It does not start the supply." Add "no prebuilt SMPS image
is provided; build and flash it" if that is true.

### 11. [severity: minor] The quick start reaches the byte session before the wiring

**Location:** `docs/capabilities/smps/README.md:35` — "## The five-minute
version" (build/start/read/stop) vs `README.md:83` — "## Wiring headline".

**Problem:** For a power supply, the first action should be wire, not start. A
reader following top-to-bottom meets the start bytes before the pin table and
the add-on power caveat. The README also has no "read in this order" line,
unlike the ADC and AUX sets.

**Fix:** Add "Read in this order: wiring → binary → feedback-loop → exit →
edge-cases" near the map table, and put the wiring headline (or a one-line
"wire first") before the five-minute bytes.

### 12. [severity: minor] The quick start does not say the NUL entry needs a host or script

**Location:** `docs/capabilities/smps/README.md:39` — "Enter binary bitbang with
20 x `0x00` (`BBIO1`)".

**Problem:** A human at a terminal cannot type NUL; the README's audience
includes "an owner checking the wiring and limits", and the five-minute version
reads as if the bytes could be typed. The host requirement is implied elsewhere
but not at the step.

**Fix:** Say "from a serial host/script; a terminal cannot send NULs" and point
to the Python example in `binary.md` at that step.

### 13. [severity: minor] The `A1`/`k` mapping makes selection look required

**Location:** `docs/capabilities/smps/README.md:18-20` — "the `?`-help assignment
token is **`A1`** (selected with `k`)".

**Problem:** I read this as "select AUX1 with `k` before starting". The SMPS
start routes the output-compare channel to the `AUX1` pin regardless of the
terminal's AUX target; `k` only changes what `a`/`A`/`@` and the wait act on.
The docs do not say that here.

**Fix:** Add "No `k` selection is needed for SMPS; the start routes the PWM to
`AUX1` regardless of the selected AUX target." Keep the token mapping, but
separate it from the start procedure.

### 14. [severity: minor] Terminal `W` is refused while the loop runs but is missing from the ownership table

**Location:** `docs/capabilities/smps/wiring.md:34` — "`W` and the binary supply
bit control the on-board target rails; they are not part of the SMPS command
group." The ownership table at `feedback-loop.md:126-134` has no `W` row.

**Problem:** A reader can infer that `W` is independent and available; in fact
the terminal `W` is refused with the same "ADC busy" message while SMPS owns the
ADC (it uses the converter for its rail check). The binary `0x4x` supply bit is a
different path and is not blocked. This should be stated, not left to inference.

**Fix:** Add `W` to the terminal refusal list with the one-line reason (rail
check uses the ADC), and state which supply path is refused and which is not. If
`w` (off) still works, say so.

### 15. [severity: minor] Same reset, two names: "terminal reset" and "Bus Pirate reset"

**Location:** `docs/capabilities/smps/exit.md:112-113` — "`0x0F` ... yes on v4
(terminal reset)" and "`#` ... yes on v4 (Bus Pirate reset)".

**Problem:** Both rows describe the same v4 board-state reset; the second name
sounds like the hardware reset in the next row. A reader comparing paths cannot
tell whether `#` and `0x0F` are the same operation or different.

**Fix:** Use one term, for example "terminal reset", for both, and reserve
"hardware reset" for the physical reset/power cycle. If "Bus Pirate reset" is the
firmware's own name, map it once: "terminal reset (the firmware's Bus Pirate
reset)".

### 16. [severity: minor] Same `0xF0` value, two names: "output sample" and "feedback sample"

**Location:** `docs/capabilities/smps/README.md:44` — "the last feedback
sample"; `docs/capabilities/smps/binary.md:40` — "read the last output sample
(raw counts)".

**Problem:** The value is the ADC feedback-node count, not necessarily the
add-on's output voltage. Calling it an "output sample" in the command table
invites the reader to treat it as the output measurement. The distinction
matters because the add-on scaling is unknown.

**Fix:** Use "feedback sample" everywhere, and add "this is the ADC-node count,
not a calibrated output voltage" at the command row or in the sample section.

### 17. [severity: minor] "Reply shapes" is a bullet chain where a table would be faster

**Location:** `docs/capabilities/smps/binary.md:75-88` — six bullets starting
"`0xF0` returns exactly two bytes ..." and "`0xF1` returns `0x01` ...".

**Problem:** This is a result-shape list; the reader has to scan bullets to
compare lengths and semantics. The main command table already carries most of
it, so the two lists also duplicate.

**Fix:** Fold the reply shape into the command table (add a "Notes" column for
the stale/zero cases), or make a small table: request | reply length | reply |
notes.

## What to keep

- **The map table and the five-minute version.** They got me from zero to bytes
  in under a minute.
- **The requested-value encoding.** The formula, the four-row table and the
  worked bytes are unambiguous; I computed `0xF2 0x58` -> `0x0258` and checked
  every row.
- **The duty/target arithmetic table.** This is the strongest page. Every value
  is checkable by hand, and the `0x526`/`0x527` boundary falls out of the
  numbers.
- **The exit-path comparison table.** It made `0xF1`, `0x00`, `0x0F`, `#`, `m`,
  hardware reset and the `0xF1`-in-`BBIO1` trap distinct, including what happens
  to the stored sample.
- **The `0xF1`-in-`BBIO1` trap explanation.** Exact side effects and a recovery
  path, in the right place.
- **The wiring "one line per wire" and "what not to connect" tables.** For the
  Bus Pirate side they are actionable and hard to misread.
- **The naming map at the top.** `AUX1`/`A1`, `ADC`/probe and "requested
  value"/"voltage" are exactly the mappings a reader needs; they stopped me from
  inventing a pin name.
- **The trigger/symptom/behaviour/recovery edge-case format.** Easy to scan and
  easy to reuse when debugging.
- **The honest `unverified` and "no unit" caveats.** They stopped me from
  assuming volts.

## Structural opinion

The six-file split is right. The README is a clean map, the binary page owns the
bytes, the loop page owns the arithmetic, the wiring page owns the pins, the
exit page owns the state, and the edge cases own recovery. I would keep the
layout.

Concrete proposals:

1. **Make `exit.md` the single owner of stop/leave/reset state.** The same facts
   are repeated in README, `binary.md`, `feedback-loop.md`, `exit.md` and
   `edge-cases.md`. The `0xF2 0x00` contradiction is a symptom of that
   duplication. Other pages should link to one state matrix instead of restating
   it.
2. **Add a first-run checklist at the top of README:** check the build presence,
   wire the Bus Pirate side, keep the add-on output disconnected, start at
   `0x200`, read after a fresh conversion, stop, leave. That answers "where do I
   start?" in one screen and puts wiring before power.
3. **Split the add-on side from the Bus Pirate side.** A short "Add-on" section
   should either identify the board and its connector/power, or say plainly
   "this repository does not contain the add-on schematic; obtain it separately"
   and list the functional requirements. The current wiring page mixes
   firmware-defined pins with external unknowns, which makes it feel complete
   when it is not.
4. **Add a sample-lifecycle table:** before first start, immediately after a
   start, while running, after stop, after a terminal reset, after a hardware
   reset. One table would remove the "is this stale?" question and the repeated
   prose in `binary.md`, `feedback-loop.md` and `exit.md`.
5. **Separate "firmware cutoff range" from "add-on regulated range" in the
   regulation table.** The current table is accurate at the firmware level but
   reads as an output guarantee.
6. **Use one naming convention for resets and one for the sample.** The table
   rows are otherwise excellent; the drift is easy to fix.

## Unknowns

- The add-on's model/revision, connector pinout, supply voltage/current, output
  range, feedback scaling and isolation. I cannot judge whether the board in my
  hand matches the description.
- Whether `0x0258` (or `0x200`) is safe on any add-on; whether `0x200` is below
  the add-on's minimum.
- The actual switching frequency (the docs say about 125 kHz, `unverified`) and
  the divider tolerance.
- Whether a terminal reset (`#`/`0x0F`) clears an open-drain state on `AUX1`, or
  whether only a power cycle does. The docs give no recovery, so I cannot tell.
- Whether the terminal `d`/`D` refusal is reachable at all. The docs call it
  defensive and say normal routes stop the loop; I could not reproduce it.
- Whether the v3 "group routed; answers `0x00`" behaviour is true on v3
  hardware; the docs mark it unverified and I have no v3 board.
- Whether any prebuilt SMPS-enabled image exists. The README says "build
  change"; the build page implies a source build, but I did not see a
  "no binary is provided" statement in the SMPS set.
- Whether the add-on's PWM input is push-pull, open-collector or opto-isolated
  for the specific board; the wiring page gives all three as possibilities.
