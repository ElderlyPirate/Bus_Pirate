# ADC probe docs — fresh-reader feedback (round 1)

**Reader:** an engineer with an old Bus Pirate v3/v4, no prior context. I read
the ADC set in reader order, then the generic, binary and SMPS pages it links,
then `TEMPLATE.md` and `SPEC.md` for context.

## Verdict

**Yes with fixes.** The core is usable — the scale table, terminal flow and
binary exchange are good enough to take a reading and script it — but I would
not call it strong until the over-voltage and self-test-jumper wording stops
reading like "it only clips", and the SMPS-owned story is reconciled: the ADC
pages say the probe commands refuse, the SMPS page says they read the feedback,
and the recovery instructions disagree about whether `0x0F` stops the loop.

## Task log

### 1. First impression from the README alone

- **Where:** `docs/capabilities/adc/README.md`, top to bottom.
- **How fast:** about 2 minutes.
- **Success:** yes. I knew it was the pin labelled `ADC`, a 2:1 divider, 6.6 V
  nominal full scale, 10-bit, about 6.45 mV per count, works on v3 and v4, the
  v3 divider is unverified, and the order is wire → terminal → binary.
- **Unclear:** the first example says `3.29V (raw 512)` while the formula
  beside it gives 3.30 V; the reading order never sends me to `probe.md`, the
  page that explains the scale; and the SMPS bullet sends a terminal user to
  `binary.md`. I also counted four labels in the "three ways" paragraph.

### 2. Terminal

- **Where:** `terminal.md`, the README five-minute bullets, and the terminal
  section of `edge-cases.md`.
- **How fast:** about 5 minutes.
- **Success:** yes for `d`, `d:N` and `D`. The prompt/output shape is clear,
  the header-once rule is shown, and the stop key is explicit (any byte,
  consumed, one byte only).
- **Unclear:** the `D` "CR+LF terminator" case says "drain the input and
  retry" — I do not know how to drain a terminal that always sends both.
  `d:N` has no stop and the command table does not say so; I found that only
  in the edge cases. The table's "header once, then N readings" reads like
  N+1 lines; the example shows N. The `D` transcript is idealized (no `0.00V`
  placeholder or backspaces), though the prose does explain them.

### 3. Convert a reading

- **Where:** `probe.md` §The divider and the scale, §Why 3.3 V can print
  3.29 V, §Resolution and accuracy, §v3/v4; `terminal.md` §Reading the value.
- **How fast:** about 4 minutes.
- **Success:** yes. I can reproduce 1.64 V for raw 256, 3.29 V for raw 512 and
  6.59 V for raw 1023 from the formula and the fixed-point truncation
  explanation. The table's "Ideal input" vs "Printed by `d`" columns are the
  useful part.
- **Unclear:** the absolute trust stops at "overall accuracy not specified";
  divider tolerance and input impedance are unknowns, so I cannot tell
  whether a 3.29 V reading is 1 % or 10 % off. More importantly, the stated
  reference (the 3.3 V rail) makes the 3.3 V channel self-referential, so I
  do not see how the `v` table/self-test 3.3 V check can detect a low rail. I
  also tripped over "full scale 6.6 V" vs "full-scale reading 6.59 V".

### 4. Binary layer

- **Where:** `binary.md`; `protocol/binary/commands.md` §ADC,
  `entering-and-framing.md` §Reply and ACK rules, `examples.md` §ADC,
  `sub-protocols.md` §`0x1x`; `smps/binary.md`.
- **How fast:** about 6 minutes.
- **Success:** yes. `0x14` is two bytes big-endian, `0x15` is a stream, no
  ack, valid high byte `0x00`–`0x03`, stop byte consumed, keep reading until
  timeout, `0xFF 0xFF` is an impossible sample. I could write a host loop
  from this.
- **Unclear:** how long "your read timeout" should be; the worked exchange
  does not show the drain completing before the next `0x00`; and after
  `0xFF 0xFF` I do not know whether to send `0xF1` directly (the page says
  "while still in raw wire", but I am in `BBIO1`) or re-enter raw wire first.
  The SMPS page says `0x0F` may not stop the loop, which contradicts the ADC
  page.

### 5. Wiring safely

- **Where:** `wiring.md`; `edge-cases.md` (over-range, jumper, SMPS source);
  `selftest/terminal.md`, `selftest/checks.md`;
  `programmers/avrisp/wiring.md` for the v4 pin numbering.
- **How fast:** about 5 minutes.
- **Success:** yes on the basics: signal to `ADC`, source ground to `GND`,
  no pull-up, no supply, 0–6.6 V measurement range, v4 pin 8 / GND 12, v3
  silkscreen. The SMPS feedback-node and self-test-jumper warnings are
  present.
- **Unclear:** over-range reads as "the scale clips" with no "may damage the
  pin"; the jumper warning says "corrupts any measurement", not that it
  hard-ties the probe to the 3.3 V rail; the SMPS warning says "do not" but
  not what the loop does if you do. I hit the over-range edge case and the
  `D` immediate-`0.00V` case: the first has no damage symptom, the second's
  recovery is vague.

### 6. Rendering

- **Where:** all six ADC files, plus the generic/binary/SMPS pages read above.
- **How fast:** about 5 minutes.
- **Success:** no TAB characters in the ADC/generic/binary/SMPS pages; fences
  and tables are balanced; prose wraps at ~80; the `host ->` capture columns
  are aligned (column 34 in `binary.md`, column 41 in `examples.md`); the ADC
  set has no dates, bench/battery/capture narratives, pass counts or log
  names. The "not verified"/"unverified" caveats are the right kind of
  caveat.
- **Unclear:** the README file-table cells (147 characters) are hard to scan
  in a narrow viewer; `wiring.md` "What not to connect" is a six-bullet chain
  that wants a two-column table; the cross-linked
  `protocol/binary/README.md` uses "not tested on v3 hardware" where the rest
  of the set says "unverified"; and `+3.3 V` vs the firmware's `+3.3V` is a
  live naming slip.

### 7. README navigation

- **Where:** README only, then clicked every link in the set.
- **How fast:** about 3 minutes.
- **Success:** one click to each of `probe`, `terminal`, `binary`, `wiring`,
  `edge-cases`; every relative link and anchor in the ADC files resolves; no
  404s or dead ends.
- **Unclear / doubling back:** the stated reading order skips `probe.md`; the
  terminal SMPS refusal bullet links to `binary.md`; the SMPS ownership
  explanation is repeated in six places (README, probe, terminal, binary,
  edge-cases, SMPS set), so the reader bounces between them. The
  `binary.md`/`edge-cases.md` cross-links are the ones I used most, and they
  worked.

## Findings

### 1. [severity: major] Over-voltage is described as clipping, not as a possible damage event

`docs/capabilities/adc/edge-cases.md:19-24`:

> **Symptom:** the reading stops at `6.59V` (raw 1023).
> **Behaviour:** the converter is at full scale; there is no over-range flag.
> **Recovery:** divide the source down; never rely on the reading above 6.6 V.

`docs/capabilities/adc/wiring.md:91-93`:

> More than 6.6 V.  The scale stops there, and the pin's absolute maximum is
> not documented here; 6.6 V is the limit that can be relied on.  On v3 this
> limit is unverified (see above).

**Problem:** both passages describe over-voltage as a measurement problem, not
a possible damage problem. The wiring sentence is self-contradictory: the
absolute maximum is not documented, yet 6.6 V is "the limit that can be relied
on." A reader can reasonably conclude that 12 V only pins the reading at
6.59 V. The under-voltage entry next door says "the pin may be damaged";
over-voltage deserves at least the same.

**Fix:** separate "measurement range" from "safe input": "0–6.6 V is the
firmware's scale; the pin's absolute maximum is not documented, so treat
>6.6 V as potentially damaging and disconnect it. The reading clips at 6.59 V
and cannot tell you how far over you are." Add the same sentence to the
edge-case "Input above 6.6 V" symptom/recovery.

### 2. [severity: major] The self-test jumper hazard is understated

`docs/capabilities/adc/wiring.md:98-101`:

> A source while the self-test `ADC` → `+3.3 V` jumper is still fitted —
> remove the jumper first.  The full self-test asks for it; leaving it in
> ties the `ADC` pin toward 3.3 V and corrupts any measurement.

`docs/capabilities/adc/edge-cases.md:100-104`:

> **Symptom:** the reading is pulled toward 3.3 V (about raw 512) and no
> longer reflects the source; a low-impedance source may fight the jumper.
> **Behaviour:** the jumper ties the probe pin to `+3.3 V`.

**Problem:** this understates the hazard. The jumper is a plain wire (the
self-test page says so) from `ADC` to the `+3.3V` rail; a low-impedance source
does not "fight" it, it back-drives the rail. "Corrupts any measurement" and
"pulled toward" read like a measurement error, not a current path into the
3.3 V rail. The symptom "about raw 512" is only true for a high-impedance
source; with a low-impedance 5 V source the reading follows the source and the
rail is overvolted. I also followed the link to `selftest/terminal.md` and
found the "Connect (ADC to +3.3V)" prompt but no instruction to remove the
jumper afterwards.

**Fix:** say "the jumper hard-ties the probe pin to the +3.3V rail; a
connected source will back-drive that rail and can damage the board or the
source. Remove the jumper after the self-test and before connecting anything
to `ADC`." Add the removal step to the self-test page as well.

### 3. [severity: major] The SMPS-owned behavior contradicts itself across the set

`docs/capabilities/adc/probe.md:116-131`:

> While the loop runs it **owns** the converter.  Commands that need a
> foreground measurement check ownership first and refuse rather than
> stealing the channel.
>
> | binary `0x14` | `0xFF 0xFF` once; no result byte |

`docs/capabilities/smps/edge-cases.md:82-93`:

> run `d`/`D`, or binary `0x14`/`0x15`, while the SMPS is running.
> **Symptom:** the reading follows the SMPS feedback, not a source connected
> to the `ADC` pin; a probe command also turns the converter off at the end,
> so the feedback loop stops updating until the next start.

**Problem:** these are mutually exclusive. One page says the probe commands
are refused and the converter is not stolen; the other says they read the SMPS
feedback and turn the converter off. A reader who follows the ADC page's link
into the SMPS set gets the opposite behavior. This is the single biggest trust
break in the set.

**Fix:** decide which behavior the firmware has (the guard/refusal is
described consistently in `probe.md`, `terminal.md`, `binary.md` and
`generic/edge-cases.md`, so the SMPS edge case is the likely outlier) and
correct the other page. If the SMPS page describes an older build, say so
explicitly and link the version condition.

### 4. [severity: major] The SMPS refusal recovery is not executable as written

`docs/capabilities/adc/binary.md:138-140`:

> Stop the SMPS with `0xF1` while still in raw wire, or leave the binary layer
> with `0x0F` (the v4 terminal reset stops the loop).  Returning from raw
> wire to `BBIO1` with `0x00` does **not** stop it, so the refusal continues.

`docs/capabilities/smps/binary.md:21-24`:

> Only `0xF1` sent while still in raw wire stops it; a hardware reset also
> stops it, but whether the terminal reset that returns from the binary layer
> stops it is `unverified`.

`docs/capabilities/adc/terminal.md:122-143` shows a `HiZ>d` refusal and says
"send `0xF1` while still in raw wire, or leave the binary layer with `0x0F`";
`docs/capabilities/selftest/terminal.md:182-185` says "both routes back to
`HiZ>` reset the board and stop the SMPS."

**Problem:** the recovery is not executable as written. After `0xFF 0xFF` the
host is in `BBIO1`; the ADC page says `0xF1` must be sent "while still in raw
wire" but never says to send `0x05` first, and never warns that `0xF1` in
`BBIO1` is a pin-state byte that switches the target supply and pull-ups on
(`protocol/binary/edge-cases.md:258-269`). Worse, the pages disagree on
whether `0x0F` stops the loop, so a host taking the easy route may leave the
SMPS running. The terminal `HiZ>d` transcript also has no reachable path: if
`0x0F`/`#` stop the loop, you should not be at `HiZ>` with it running.

**Fix:** give one recovery sequence with the layer named at each step: from
`BBIO1`, `0x05` → `RAW1`, `0xF1`, read `0x01`; or `0x0F` if (and only if) the
docs are sure it stops the loop. Add "do not send `0xF1` in `BBIO1`: it is a
pin-state byte." Resolve the `0x0F` question once, in `probe.md`, and make the
other pages quote that answer. Either explain how the `HiZ>d` refusal is
reached or mark it as a defensive message that should not normally appear.

### 5. [severity: major] The stated ADC reference makes the 3.3 V channel self-referential

`docs/capabilities/adc/probe.md:19-26`:

> The converter's reference is the board's 3.3 V rail; the firmware assumes
> 3.3 V. ... The reading is ratiometric to the 3.3 V rail: if that rail is
> not at its nominal value, every reading shifts with it.

`docs/capabilities/adc/probe.md:96`:

> | 3.3 V rail | yes | yes | `v`, the `W` rail check, the self-test,
> OpenOCD `buspirate adc` |

`docs/capabilities/adc/probe.md:104`:

> The firmware applies the same 2:1 scale to every channel it displays.

**Problem:** if the 3.3 V rail is both the reference and the 3.3 V channel's
input, that channel is measured against itself: a 3.0 V rail and a 3.3 V rail
produce the same count, so the `v` table and the self-test's 3.3 V window
cannot detect a low rail. Either the reference is not the rail for that
channel, or the channel table/self-test claims need correction. As a reader,
this is the point where I stopped trusting the number; the probe formula is
reproducible, but the rail-check story is not.

**Fix:** state the reference per channel, or add the explicit consequence:
"The 3.3 V channel uses the same reference, so it reads the divider ratio, not
an absolute rail voltage; the self-test's 3.3 V window is a go/no-go on the
divider, not the rail." If that is wrong, correct the sentence and the channel
table, and say which reference the 3.3 V channel actually uses.

### 6. [severity: major] The `D` CR+LF recovery is not actionable for a terminal user

`docs/capabilities/adc/terminal.md:90-95`:

> If a byte is already waiting (type-ahead, or a line terminator left by a
> terminal that sends both CR and LF), the loop exits at once and `0.00V` is
> the placeholder, not a reading.

`docs/capabilities/adc/edge-cases.md:73-81`:

> **Recovery:** drain the input and retry; the `0.00V` is not a reading.

**Problem:** "drain the input" is not something a terminal user can do if the
terminal always sends CR+LF; pressing more keys adds more bytes. This is a
common first-use failure (`D` prints 0.00 V and returns), and the documented
recovery does not fix it. The user is left thinking the probe is broken or
the board is at 0 V.

**Fix:** name the terminal setting: "set the terminal to send CR only (not
CR+LF), or use a host program that sends one byte; pressing more keys will not
drain a terminal that always sends both." Add this to the `D` section itself,
not only the edge case.

### 7. [severity: minor] The binary worked exchange skips the drain-complete point

`docs/capabilities/adc/binary.md:167-171`:

> host -> 0x00                     stop byte; consumed
> host <- 0x02 0x00                example: a buffered sample still arrives
>                                  (keep reading until the timeout)
> host -> 0x00                     reprint the layer identifier
> host <- "BBIO1"

**Problem:** the block calls itself "A full binary session" but skips the
drain-complete point. A host that copies it literally sends the second `0x00`
while buffered samples may still be arriving; the reply to that `0x00` can be
a stale sample, and the host loses alignment. The prose says "keep reading
until the timeout", but the code block shows the next command immediately.

**Fix:** add an explicit line between the buffered sample and the next
command, for example `host <- (no sample for <timeout>)    drain complete`,
and say the timeout is host-chosen. This is the one place where the exchange
is not executable as drawn.

### 8. [severity: minor] The README's first numbers do not match each other

`docs/capabilities/adc/README.md:43-45`:

> the reply is `VOLTAGE PROBE: 3.29V (raw 512)` (example).  Volts =
> raw / 1024 × 6.6; raw is 0..1023.

`docs/capabilities/adc/README.md:7-8`:

> the firmware's full scale is 6.6 V and one count is about 6.45 mV.

`docs/capabilities/adc/wiring.md:50`:

> | full-scale reading | 6.59 V (raw 1023) |

**Problem:** the first example does not match the formula beside it:
512 / 1024 × 6.6 = 3.30, not 3.29. The explanation (fixed-point truncation)
is in `probe.md`, but the README's five-minute version is the first thing a
reader sees. The same page says full scale 6.6 V while the wiring page says
full-scale reading 6.59 V; both are explained later, but the first page reads
as if the numbers disagree.

**Fix:** add "(the display truncates, so 3.30 V prints as 3.29 V)" to the
README bullet, or use a raw value that is not on a boundary (raw 500 →
3.22 V). Rename the two concepts: "nominal full-scale input 6.6 V" and
"maximum reading 6.59 V (raw 1023)". A one-line pointer to
`probe.md#why-33-v-can-print-329-v` would also do it.

### 9. [severity: minor] `d:N` is presented as an ordinary repeat, but it cannot be stopped

`docs/capabilities/adc/terminal.md:30-34`:

> | `d:N` | N conversions, N = 1..32767 | the header once, then N readings |

`docs/capabilities/adc/edge-cases.md:51-57`:

> **Behaviour:** the repeat loop does not watch the host for a stop.
> **Recovery:** press the reset button; a typed `#` is not read until the loop
> ends.

**Problem:** the command table and the README's five-minute version present
`d:N` as an ordinary repeat; the "cannot stop it" fact is buried in the edge
cases. A user who types a large valid N has to reset the board, and a user
reading "then N readings" may expect N+1 lines (the example shows N total).

**Fix:** add "the repeat is not interruptible; use a small N" to the
table/README bullet, and change the output cell to "N readings total; the
header precedes the first." Keep the edge case as the recovery detail.

### 10. [severity: minor] `+3.3 V` is not the firmware's `+3.3V`

`docs/capabilities/adc/wiring.md:98`:

> the self-test `ADC` → `+3.3 V` jumper

`docs/capabilities/adc/edge-cases.md:101`:

> `ADC` → `+3.3 V` jumper

Versus `docs/capabilities/selftest/terminal.md:43` — "Connect (ADC to
+3.3V)" — and `docs/protocol/binary/commands.md:224` — "`ADC`→`+3.3V`".

**Problem:** the ADC set writes `+3.3 V` (with a space) where the firmware
prompt and the other sets write `+3.3V` (no space). The backticks make it look
like a literal pin label. A reader matching the silkscreen/prompt has to guess
whether it is the same thing.

**Fix:** use the firmware form `+3.3V` (or the silkscreen form `+3V3`, with
one mapping sentence) in the ADC set, and make the jumper name consistent
across the linked self-test pages.

### 11. [severity: minor] The README's reading order skips the scale page, and the terminal SMPS link points at the binary page

`docs/capabilities/adc/README.md:25-27`:

> Read in this order: wire ([wiring.md](wiring.md)), take one reading
> ([terminal.md](terminal.md)), then script it ([binary.md](binary.md));
> failures are in [edge-cases.md](edge-cases.md).

`docs/capabilities/adc/README.md:53-55`:

> On an SMPS-enabled v4 build, a running SMPS feedback loop owns the probe
> channel.  `d` / `D` are refused and `0x14` / `0x15` return `0xFF 0xFF`
> once; stop the SMPS first ([binary.md](binary.md#smps-owns-the-adc)).

**Problem:** the reading order skips `probe.md`, the page that explains the
scale/divider/resolution and the 3.29 V display; I had to double back to the
Files table to find it. The SMPS bullet is about a terminal symptom (`d` /
`D`) but links to the binary page; `terminal.md` has the terminal refusal and
the recovery text I needed.

**Fix:** put `probe.md` in the order (wire → one reading → scale → script) and
link the terminal SMPS bullet to `terminal.md#smps-owns-the-adc` (or to
`probe.md#who-owns-the-converter` for the model), keeping `binary.md` for the
byte-level refusal.

### 12. [severity: minor] "Only the probe channel has user commands" and "three ways" are both imprecise

`docs/capabilities/adc/README.md:80-82`:

> Only the probe channel has user commands.  The 3.3 V rail, 5.0 V rail, VPU
> and (v4) Vusb channels are housekeeping, read by `v`, the self-test and the
> features that check them.

`docs/capabilities/adc/README.md:16-22`:

> The firmware names the same input three ways. ... `ADC` ... the `?` help
> lists the commands as `d/D` ... `VOLTAGE PROBE: ` ... `VOLTMETER MODE`.

**Problem:** `v` is a user command that reads those channels, so "only the
probe channel has user commands" reads as a contradiction. I think the
intended meaning is "only the probe has dedicated commands." Also, the
"three ways" sentence lists `ADC`, `d/D`, `VOLTAGE PROBE: ` and `VOLTMETER
MODE`; I counted four labels and had to re-read to separate the command pair
from the display strings.

**Fix:** say "Only the probe channel has dedicated commands (`d`/`D`, binary
`0x14`/`0x15`); the other channels are housekeeping, displayed by `v`..."
and change "three ways" to "three display labels (plus the `d`/`D` command
pair)" or simply "the firmware uses several labels".

### 13. [severity: minor] "What not to connect" is a dense six-bullet chain that wants a table

`docs/capabilities/adc/wiring.md:89-102`:

> ## What not to connect
>
> - More than 6.6 V. ...
> - A negative voltage.
> - Mains, or any circuit that is not isolated from the mains. ...
> - The probe as an output. ...
> - A source while the self-test `ADC` → `+3.3 V` jumper is still fitted — ...
> - An external source while the SMPS feedback loop is running ...

**Problem:** this is a chain of six independent hazards; the eye skims it, and
the most dangerous one (the hard tie to 3.3 V) is buried in the middle. A
two-column table would be easier to scan and would force the "why" for each
row.

**Fix:** convert to `Do not connect | Because` rows. The `terminal.md` §What
the terminal does not show (four bullets) would also read better as `Symptom |
Explanation`, though it is less urgent.

### 14. [severity: minor] Cross-linked binary README uses "not tested" where the set says "unverified"

`docs/protocol/binary/README.md:3-4`:

> **Status:** v4 documented; v3 is unverified (built for v3, not tested on v3
> hardware).

`docs/protocol/binary/README.md:82`:

> The v3 features are in the build but have **not been tested on v3
> hardware**.

**Problem:** the rest of the ADC/binary/generic pages consistently use "not
verified"/"unverified"; "not tested" reads as a test-status claim and is the
kind of bench-result wording the set otherwise avoids. It is a cross-linked
page, not the ADC set itself, but the ADC pages link it for the binary layer.

**Fix:** use "unverified on v3 hardware" throughout; keep the fact, drop the
test-status phrasing.

### 15. [severity: minor] The SMPS start-operand handling contradicts itself across the binary and SMPS pages

`docs/capabilities/smps/binary.md:31-35`:

> A start request is also refused with `0x00`, but its operand byte is
> **not** consumed, so it is read as the next raw-wire request.

`docs/protocol/binary/sub-protocols.md:299-300`:

> On a build without SMPS support every request is refused with `0x00`, and a
> start still consumes its operand byte so the following request stays
> aligned.

`docs/protocol/binary/edge-cases.md:176-182` says the operand is consumed;
`smps/edge-cases.md:11-18` says it is left unread.

**Problem:** a host that resynchronizes after a refused start will be off by
one byte in one of the two worlds. This is not the ADC refusal itself, but it
is in the SMPS pages the ADC set links, and it is the same duplication-drift
pattern as finding 3.

**Fix:** resolve the behavior and use one contract in all four places. If the
build-dependent behavior really differs, say which build does which and make
the resync step explicit.

## What to keep

- The raw→volts table in `probe.md` with both "Ideal input" and "Printed by
  `d`" columns, and the "Why 3.3 V can print 3.29 V" section. That is the part
  that let me reproduce the number instead of guessing.
- The terminal `D` section's "What the terminal does not show": the 0.00 V
  placeholder, the five backspaces, "only the voltage", and "the displayed
  value is the most recent sample". This is exactly the detail a first-time
  user needs and would not infer.
- The binary no-ack warning and the stop/drain contract, especially "the
  board checks for the stop at the loop boundary" and "a byte already waiting
  stops after the first sample". That is the kind of mechanism that makes the
  exchange executable.
- The `0xFF 0xFF` refusal framed as an impossible 10-bit sample. It gives a
  host a checkable rule rather than a special case to remember.
- The one-line-per-wire table and the explicit "No pull-up is required / No
  supply is required" statements. I did not have to guess about `P` or `W`.
- The v3/v4 availability table and the repeated "unverified" caveat. As a v3
  owner, I knew exactly what I could and could not trust.
- The `d:N` header-once transcript and the `dx` / `o1` edge cases; those are
  the small behaviors that otherwise generate support questions.
- The naming map at the top of the README (ADC, VOLTAGE PROBE, VOLTMETER
  MODE, VPU/Vpullup). Even with the "three ways" stumble, it is the right idea
  and the reason the rest of the set is readable.

## Structural opinion

The layout works for this function: a map README, one page for the
input/scale/ownership model, one page per front end, one for wiring, one for
failures. That is the right shape for an ADC probe, because the questions
split cleanly into "what does it measure", "how do I type it", "how do I
script it", "how do I wire it", "what went wrong".

The one structural problem is the SMPS-owned condition. It is a cross-cutting
state, but it is explained in the README, `probe.md`, `terminal.md`,
`binary.md`, `edge-cases.md`, `smps/README.md`, `smps/binary.md` and
`smps/edge-cases.md`. That duplication is why the set now disagrees with
itself. Proposal: make `probe.md` §Who owns the converter the single model;
keep only the exact refusal message/bytes in `terminal.md` and `binary.md`;
keep only the recovery in `edge-cases.md`; have the SMPS pages link to the
model instead of restating the probe behavior. One fact, one home.

The README's "Read in this order" should match the questions, not the file
table: wiring → terminal (one reading) → probe (scale/trust) → binary
(scripting) → edge cases. Right now it sends me to the front ends before the
page that explains the number, which is why the 3.29 V example feels wrong on
first contact.

The "Files" and "Related docs" tables overlap; the Related table is 13 rows
and some rows are the same pages as the Files table. Proposal: keep the Files
table for the set, and group Related into "Shared layer" (generic/binary),
"Sibling capabilities" (SMPS/AUX/self-test), and "Other readers"
(BASIC/AVRISP/OpenOCD). That would cut the doubling back without losing the
links.

The edge-cases page is long but well structured by Terminal/Binary; I would
not split it. The trigger/symptom/behaviour/recovery shape is consistent and
is the reason I could use it while debugging.

## Unknowns

- Whether the firmware's fixed-point ratio is exactly the one described; I can
  reproduce the table, not verify the code.
- Whether the v3 divider is really 2:1 and whether the v3 `ADC` pin is safe at
  6.6 V; the docs say unverified, and I have no v3 board to check.
- The actual divider resistor values, input impedance, and maximum source
  impedance; the docs say unverified, and I cannot measure them.
- The real ADC reference for each channel. The stated 3.3 V rail reference
  makes the 3.3 V channel self-referential; I cannot resolve that without
  firmware or a bench check.
- Whether `0x0F` actually stops the SMPS loop. The ADC, binary and self-test
  pages say yes; `smps/binary.md` says unverified. I cannot test it.
- Whether the `HiZ>d` SMPS refusal is reachable at all. If `0x0F`/`#` stop the
  loop, the transcript should not occur; if there is a path, the docs do not
  describe it.
- Whether `0xF1` after leaving raw wire really is a pin-state byte that
  switches the supply and pull-ups on; the binary pages say so, and it is
  consistent with the state-byte layout, but I cannot test it.
- The real sample rate and backlog of `0x15`; the docs say host-dependent, and
  I cannot judge a concrete timeout.
- How common the CR+LF terminal setting is among the target users; I can see
  the trap, but I cannot tell whether it is the majority case.
- The exact silkscreen labels on the v3/v4 boards (`+3.3V` vs `+3V3`, the
  `ADC` pin location); the docs point at the silkscreen, which I do not have
  in front of me.
- Whether the self-test jumper is a plain wire on both boards and what current
  path it creates; the self-test page says plain wire, but I cannot inspect
  the hardware.
