# I2C documentation — round 1 reader feedback

**Verdict — yes with fixes.** The map-and-detail structure is strong and the
exact firmware captures are unusually good, but a v3 owner is told the wrong
pull-up story, the board-applicability statements disagree with each other, and
one binary worked exchange leaves a host parser one byte out of sync.

## Task log

### 1. First impression from the README alone

- **Where I looked:** `docs/protocol/i2c/README.md`, cover to cover.
- **How fast:** about 4 minutes.
- **Success:** yes. I knew this was a manual I2C master with a scanner, sniffer,
  binary `I2C1` path and a v4 on-board EEPROM, and I knew to start with `m4`.
- **What was unclear:** the opening status says “v3 is build-only”, which I read
  first as “source-only, no shipped firmware”. The first paragraph says the
  hardware engine is “on v4”, while the availability table 50 lines later says
  “optional build, not the default” for v3. The hardware paragraph says v3 needs
  an “external pull-up”, which contradicts the rest of the set. There is no
  single end-to-end first session, so I had to decide for myself whether to read
  entry-and-setup or bus-operations first.

### 2. Enter I2C, choose a speed, wire, and run START/write/read/STOP

- **Where I looked:** `entry-and-setup.md`, `wiring.md`, `bus-operations.md`;
  then `docs/generic/navigation.md` and `docs/generic/commands.md` for `m` and
  `e`.
- **How fast:** about 15 minutes including the cross-checks.
- **Success:** yes, with two wrong turns. The one-line form and the zero-based
  header table made `m4 1 1` → `I2C (mod spd)=( 0 0 )` immediately readable.
  The start/stop table and the worked transaction made `[`, `0xA0`, `r`, `]` and
  a repeated start followable.
- **What was unclear:** whether `m0` opens the menu (entry says only `m` does).
  The generic bring-up page tells me to answer a `Select output type:` prompt
  that I2C never asks. The README quick start says `W` powers the target rails
  but never says to skip `W` for a self-powered target. The bring-up table calls
  `w` “park and remove power”, but it does not remove the pull-ups or the
  on-board VPU source. The repeated-start example stops mid-transaction, so I
  had to find the worked session to see it finished.

### 3. Clock stretching: wait, three outcomes, recovery, `0x0A`

- **Where I looked:** `clock-stretching.md`, the read section of
  `bus-operations.md`, and the clock-stretch rows of `edge-cases.md`.
- **How fast:** about 5 minutes.
- **Success:** yes. I found the ~25 ms bound, the `I2C bus error` captures, the
  pin-release statement, the nine-clock hardware recovery and the binary `0x0A`
  status table without doubling back.
- **What was unclear:** the “Three outcomes” table has only two real outcomes;
  the third row is what the next command does. The recovery bullet says the next
  operation “retries from the current line state”, which reads like an automatic
  retry of the abandoned command; it is not. The edge-case page says `W` loses
  the pending ACK state, while this page says only timeout, `w` and a mode change
  drop it.

### 4. Macros and the on-board EEPROM

- **Where I looked:** `macros.md`, `eeprom.md`, the scanner and worked-session
  sections of `bus-operations.md`, then `edge-cases.md`.
- **How fast:** about 10 minutes.
- **Success:** yes. I found the `(0)` list, `(1)`/`(2)`, the exact
  `Onboard EEPROM requires hardware I2C` refusal, the `(3)` → `(4)` ordering,
  the 24xx64 geometry, the page size, the write-protect re-assertion and the
  poll/read-back sequence.
- **What was unclear:** the README macro bullet says `(3)` and `(4)` require
  Hardware but only says `(3)` prints the refusal; a Software user who types
  `(4)` sees nothing. The EEPROM walkthrough says the pointer must be set again
  before “every read”, but its own example reads two bytes without resetting it.
  The Windows tool section calls `0x4C` “supply and pull-ups on” without saying
  it also drives AUX low, and its write retry has no timeout.

### 5. Binary `I2C1`: enter, `0x08`, `0x09`, sniffer framing

- **Where I looked:** `binary.md`, then `docs/protocol/binary/commands.md` and
  `sub-protocols.md` for the shared groups, and `eeprom.md` for the tool.
- **How fast:** about 15 minutes.
- **Success:** mostly. The entry/exit sequence is clear, the transaction-shape
  table answers the “which `0x08` form do I send?” question quickly, and the
  `0x09` table plus the sniffer framing table are findable.
- **What was unclear:** the worked exchange omits the `0x01` reply to `0x60`, so
  a host following it reads the next frame wrong. The `0x09` section calls the
  first reply byte “success” and then says an unknown sub-command returns
  `0x01 0x00`, which is not a success result. There is no side-by-side mapping
  from the terminal wrappers to the binary bytes, so I built that table myself.
  The difference from the terminal wrapper is therefore clear only after reading
  both pages, not from one place.

### 6. Rendering check

- **Where I looked:** all ten `docs/protocol/i2c/*.md` files, plus the generic
  pages used above; mechanical scan for tabs, trailing whitespace, prose over
  ~80 columns and broken code blocks.
- **How fast:** about 10 minutes.
- **Success:** yes. No TAB characters, no prose line over 82 columns, no broken
  wraps, no unreadable option lists. Captures are plain and aligned. The two
  apparent oddities — `READ:  ACK 0xBB` and `WRITE: 0xA0I2C bus error` — are
  intentional and explained; do not “fix” them.
- **What did not work:** the dedicated “Unverified and caveats” section reads
  like an audit trail and repeats a caveat that is already printed under every
  v3/v4 table. The README “Traffic” bullet is a six-fact paragraph in bullet
  clothing. “Vpullup” / “Vpu” / “VPU” has a mapping table but then a sentence
  that contradicts the mapping.

### 7. README navigation test

- **Reference tables:** the file table is at the top (`README.md:19-30`); one
  click to the setup, speed, wiring, bus-operation, binary and edge-case tables.
  Fastest part of the set.
- **Worked sessions:** `bus-operations.md` is advertised as “a worked session”
  and `binary.md` as “a worked exchange”; both are one click. The EEPROM page
  has a good example but its README row does not say so.
- **Edge cases:** one click to `edge-cases.md`, and its topic headings make the
  lookup fast.
- **Dead ends:** “Names used in this set” ends with a paragraph headed
  “**Buffers**”, which is not a name; I would not look for buffer limits there.
  The `Vpullup` row says the set uses one term, then `wiring.md` says it uses
  another.
- **Doubling back:** wiring → entry-and-setup → generic/commands for one `e`
  step, and the last hop contradicts the I2C page on the output-type prompt.
  The first full session is stitched from three pages because there is no single
  end-to-end capture.

## Findings

1. **[major]** `README.md:72-75` — “v3 needs an external 0–5 V feed and an
   external pull-up.”
   **Problem:** the v3 has switched on-board pull-ups too; `docs/generic/README.md:151`
   says `P` connects the I/O lines through the on-board resistors, and the
   v3/v4 table on the same I2C wiring page only says the v3 on-board value is
   “not covered”. A v3 owner will add unnecessary external resistors, or think
   `P` is unavailable.
   **Fix:** “v3 has its own switched pull-ups (value not documented here) and
   needs an external 0–5 V feed on Vpullup; an external pull-up is optional.”
   Keep the v3 feed caveat, drop the requirement.

2. **[major]** `README.md:9-10`, `README.md:64`, `entry-and-setup.md:41-42`,
   `SPEC.md:59` — the hardware engine is described four ways.
   > “Two drivers are available: a software (bit-banged) driver on both boards
   > and, on v4, the on-chip hardware engine.”
   > `| hardware engine | optional build, not the default | yes, always offered |`
   > “On v4, and on a v3 build that compiles the hardware engine in (not the
   > default v3 build), the first prompt chooses the driver:”
   > `| · hardware engine | - | x | 100k/400k/1M | ✓ |`
   **Problem:** a v3 owner cannot tell whether hardware I2C exists on their
   board. The README sentence points only at v4, and SPEC marks the engine absent
   for v3; the README table and entry page say optional. The same page also says
   “Choose Hardware … for the on-board EEPROM macros 3/4” without saying those
   macros are v4-only, which is another board trap for a v3 hardware build.
   **Fix:** use one formula everywhere: “Software on both boards; hardware
   standard on v4, optional on v3 and not in the default v3 build.” Change the
   SPEC v3 cell to `off`, and add “(v4 only)” to the EEPROM-macro sentence. Also
   replace “v3 is build-only” with “v3 is included in the build; its hardware
   behaviour is not verified.”

3. **[major]** `binary.md:291-293` — the worked exchange skips the `0x60` reply.
   > `host -> 0x60            speed = software profile 0`
   >
   > `; write frame [ 0xA0 0x00 0x20 0xAA 0xBB ]`
   **Problem:** the command table says `0x6x` replies `0x01` (`binary.md:78`).
   The exchange jumps straight to the write frame, so a host that follows it
   byte-for-byte reads the next response as the speed acknowledgement and is one
   byte out of sync for the rest of the session.
   **Fix:** insert `host <- 0x01` after the `0x60` line.

4. **[major]** `entry-and-setup.md:181` — “park and remove power” does not
   remove the pull-ups or VPU.
   > | park and remove power | `w` | `Clutch disengaged!!!`, then `POWER SUPPLIES OFF` |
   **Problem:** `w` removes the target rails and parks the mode; it does not turn
   off the `P` pull-up switch or the `e2`/`e3` on-board source. The page itself
   says only a mode change, `#` or reset clears those. A user who trusts the
   table may leave 3.3 V/5 V on Vpullup and the resistors connected while
   rewiring.
   **Fix:** “`w` removes the target rails; it leaves `P` and the VPU source as
   they were — use `p` and `e1` to clear them.”

5. **[major]** `docs/generic/commands.md:37-40` and
   `entry-and-setup.md:163-168` — the linked `e` procedure requires a prompt
   that I2C never asks.
   > “Enter a non-HiZ mode whose setup answers
   > `Select output type:` with open drain (most bus modes default to it; DIO
   > has no such prompt), then issue `e2`/`e3`, switch the on-board rails on with
   > `W`, then `P`.”
   **Problem:** the I2C page says “I2C has **no `Select output type:` prompt**”.
   A reader who follows the link from `entry-and-setup.md:213` looks for a step
   that does not exist, or assumes they entered the mode incorrectly.
   **Fix:** in the generic page, say “a non-HiZ mode that is open drain (I2C is
   always open drain and asks no output-type prompt)”. This is the page the I2C
   set sends people to, so it should not contradict the mode.

6. **[major]** `README.md:40-42` — the quick start does not say to skip `W` for
   a self-powered target.
   > “**Bring-up:** `W` powers the target rails. On v4 select the Vpullup source
   > first (`e2` 3.3 V or `e3` 5 V), then enable the pull-ups with `P`. Match VPU
   > to the target's logic level.”
   **Problem:** `entry-and-setup.md:217-221` correctly warns to leave the 3.3 V
   and 5.0 V header pins disconnected from a self-powered target and to skip `W`
   in that case. A reader who only uses the five-minute version can back-power a
   self-powered target.
   **Fix:** add one sentence: “Skip `W` for a self-powered target; `e`/`P` still
   work.” This is the highest-value safety line missing from the front page.

7. **[minor]** `entry-and-setup.md:158-160` and `edge-cases.md:233` — “the header
   is typed back” does not run the prompts.
   > “Typing the header back verbatim also fails, because its labels, brackets
   > and `=` are not numbers; the prompts then run.”
   **Problem:** at the `I2C>` prompt the line begins with `I`, which is not a
   command, so it aborts with a syntax error before any setup prompt can run.
   The fallback-to-prompts behavior happens only when the header text is typed
   as the arguments after `m4`. As written, this is a false recovery route.
   **Fix:** “Typing the header as the arguments to `m4` fails and the prompts
   run; typing it at the prompt is a syntax error.”

8. **[minor]** `entry-and-setup.md:25` — “Only `m` with no number on the same
   line opens the menu.”
   **Problem:** `docs/generic/navigation.md:204` says `m0` also opens the menu
   because zero counts as “no argument”. A reader who types `m0` gets the menu,
   not the error this sentence predicts.
   **Fix:** “Only `m` with no number (or `m0`) opens the menu.”

9. **[minor]** `bus-operations.md:279` — “The pointer must be set again before
   every read”.
   **Problem:** the page's own read-back at `bus-operations.md:269-272` does two
   `r` commands after one pointer set and gets `0xAA` then `0xBB`. The pointer
   must be set before each read **transaction**, not before each byte.
   **Fix:** “Set the pointer again before each read transaction; bytes within one
   transaction auto-increment.”

10. **[minor]** `clock-stretching.md:32-38` and `:76-77` — “Three outcomes” and
    “retries”.
    > “| the next command runs | it starts from a fresh error state; if SCL is
    > still held it waits and times out again |”
    > “the next bus operation clears the sticky error and retries from the
    > current line state.”
    **Problem:** the first two rows are outcomes of the stretch wait; the third
    is a follow-on command. “Retries” reads as automatic retry of the abandoned
    operation, which the text elsewhere denies.
    **Fix:** make the table “two outcomes” and add a separate “After a timeout”
    note: “the next operation starts fresh; it does not retry the abandoned
    command, and times out again if SCL is still held.”

11. **[minor]** `binary.md:232-233` and `:248-249` — `0x09` calls the first
    reply byte “success”.
    > “The reply is always **two bytes**: `0x01` (success) and then the
    > sub-result.”
    > “An unknown sub-command returns `0x01 0x00`, not a success result.”
    **Problem:** the first byte is always `0x01`, including the unknown case, so
    a host that treats it as the success flag accepts an unknown sub-command.
    **Fix:** call byte 1 the “command accepted / frame reply” and byte 2 the
    “sub-result”; state that `0x00` in byte 2 means unknown or failure.

12. **[minor]** `eeprom.md:154-155` — the tool's `0x4C` also drives AUX low.
    > “The tool sends 20 NUL bytes, then `0x02` (enter I2C), `0x51` (on-board
    > 3.3 V pull-up feed, v4) and `0x4C` (supply and pull-ups on).”
    **Problem:** `binary.md:123-127` warns that `0x4C` drives AUX low and that a
    jumpered AUX clamps SDA/SCL until `0x09 0x02` releases it. The tool does not
    send `0x09 0x02`, so a user with AUX jumpered gets a dead bus and no
    explanation in the tool section. The worked exchange releases AUX; the tool
    does not.
    **Fix:** add: “`0x4C` also drives AUX low; the tool never releases it, so
    leave AUX unjumpered or release it before running the tool.”

13. **[minor]** `eeprom.md:159-160` — the write retry has no timeout.
    > “A byte is retried until the chip acknowledges it; the summary line reports
    > the byte count and the retry count.”
    **Problem:** the sentence describes an unbounded loop but does not warn about
    it. With a missing, unpowered or miswired chip no summary line is printed,
    and a user can mistake the retry loop for a hung Bus Pirate. The limits
    section does not mention it.
    **Fix:** state “there is no retry limit; if the chip never ACKs, interrupt
    the tool.” If the tool can be changed, add a limit and report it.

14. **[minor]** `wiring.md:82-83` — “this set calls it VPU” contradicts the
    README's primary term.
    > “The firmware calls the pin "Vpullup"; the reference card marks it `Vpu`;
    > this set calls it VPU.”
    **Problem:** `README.md:85` makes `Vpullup` the primary form and lists `Vpu`
    and `VPU` as aliases; nearly every page uses `Vpullup`. A reader cannot tell
    which name the set actually uses.
    **Fix:** “The `v` table labels the column `VPU`; this set uses `Vpullup` in
    prose and quotes the other forms where they appear.”

15. **[minor]** `edge-cases.md:153-163` — the “Unverified and caveats” section
    is an evidence block, not user guidance.
    > “## Unverified and caveats
    > - All v3 behaviour in this set is **not verified**.”
    > “The exact software and hardware clock rates on a given board and bus are
    > **unverified**; the labels are nominal.”
    > “The on-board EEPROM macros, the chip's geometry and the write-protect path
    > require a v4 with a populated on-board EEPROM and are **not verified**.”
    > “The sniffer's maximum usable rate is **unverified**; an overflow-free
    > capture is not proof of lossless capture at an arbitrary rate.”
    > “The 5 V class limit is stated from the reference card and the on-board VPU
    > source; the exact absolute maximum of the I/O pins is **unverified**.”
    **Problem:** it repeats the caveat already printed under every v3/v4 table,
    gives the reader no action, and makes the set read as provisional. It is the
    only section that behaves like an audit record rather than a user page. The
    same provenance flavour survives in “On the documented v4 build”
    (`clock-stretching.md:28`) and “at the documented build” (`speeds.md:44`).
    **Fix:** delete the section; keep one short caveat in the README/availability
    table and one under each table only where the caveat changes what the reader
    can rely on. Replace “the documented v4 build” with “on v4” and “at the
    documented build” with “at the default build”.

16. **[minor]** `README.md:44-48` — the “Traffic” bullet is six facts in one
    paragraph.
    > “**Traffic:** `[` start, `]` stop; `{`/`}` are the same as `[`/`]`. `0xNN`,
    > `0b…` or a decimal writes a byte and prints the slave's ACK. `r` reads a
    > byte; its ninth bit is deferred to the next bus operation — a read/write
    > sends ACK, a start/stop sends NACK. A second `[` before the stop is a
    > repeated start.”
    **Problem:** this is the densest part of the first screen and mixes four
    different command classes. It is hard to scan and easy to misread.
    **Fix:** split into a small table (start/stop, write, read, repeated start)
    or four short bullets.

17. **[minor]** `edge-cases.md:78` vs `clock-stretching.md:104-106` — `W` and
    the pending ACK.
    > “| `W` is issued with a read ACK pending | the driver is re-initialised;
    > the ACK state is lost | finish the read with `]` first |”
    **Problem:** `clock-stretching.md` lists only a timeout, `w` (mode cleanup)
    and a mode change as dropping the pending bit; `W` is the power-on command
    and re-runs the driver setup without that cleanup. The two pages cannot both
    be right, and a user who does `W` mid-read may get a stray ACK/NACK on the
    next operation.
    **Fix:** say “`W` re-runs the driver setup but does not clear the pending
    bit; resolve the read with `]` first” — and keep the recommendation.

18. **[minor]** `edge-cases.md:52-53` — adjacent rows give opposite width advice.
    > “A value above `0xFF` is written | the sent byte is the low 8 bits | set
    > the width deliberately or send bytes”
    > “`;N` is set to anything other than 8 | software: N clocked bits; hardware:
    > 8 clocked bits, wide display | leave the width at 8 in I2C”
    **Problem:** one row says to set the width deliberately; the next says leave
    it at 8. A reader cannot tell which is the recommendation.
    **Fix:** “send multiple bytes; change the width only if you intend a
    bit-level transfer.”

19. **[minor]** `README.md:48-51` — the macro bullet hides the `(3)` → `(4)`
    precondition.
    > “`(3)` and `(4)` drive the on-board EEPROM and require the mode entered
    > with **2 Hardware**; from Software, `(3)` prints
    > `Onboard EEPROM requires hardware I2C`.”
    **Problem:** from Software `(4)` prints nothing; in Hardware, `(4)` also does
    nothing unless `(3)` was run first (`macros.md:106-107`). The quick start
    only names the `(3)` refusal.
    **Fix:** “Run `(3)` first, then `(4)`; from Software `(3)` is refused and
    `(4)` is silent.”

20. **[minor]** `bus-operations.md:174-175` — the scanner description omits the
    stop after each address.
    > “The scanner checks every 8-bit address from `0x00` to `0xFF` with a start,
    > the address byte, and an ACK read.”
    **Problem:** the scan does a stop after every probe, and the behavior list
    only mentions a stop for read addresses. A reader may think the scanner
    leaves the bus open between addresses or that only read addresses are
    terminated.
    **Fix:** add “then a stop” to the sentence, and say “a stop follows each
    probe” in the behavior list.

21. **[minor]** `README.md:100-102` — “no device-specific command layer” reads
    as “no EEPROM support”.
    > “**No device-specific command layer** (no EEPROM page-write helper, no
    > register decode). I2C here is a manual master plus the scanner, sniffer and
    > on-board EEPROM macros.”
    **Problem:** the sentence denies EEPROM helpers and then lists the on-board
    EEPROM macros and the external EEPROM tool. A reader looking for the EEPROM
    support may stop at the denial.
    **Fix:** “No generic device layer; the only chip-specific support is the
    on-board EEPROM macros 3/4 and the external EEPROM tool.”

22. **[minor]** `README.md:37-39` and `wiring.md:62-64` — the quick start makes
    `P` look sufficient at every speed.
    > “**Pull-ups on both lines are mandatory — 2 kΩ to 10 kΩ to Vpullup.**”
    > “The 10 kΩ SCL value gives a slow rise on a loaded bus, so fast profiles
    > often need a stronger external SCL pull-up; roughly **1.5–2.2 kΩ** is a
    > starting point.”
    **Problem:** on v4 `P` alone gives 10 kΩ on SCL. A reader who follows the
    five-minute version, selects `400kHz` or `1MHz`, and sees failures has no
    cue that the on-board SCL pull-up is the likely cause.
    **Fix:** add one sentence to the quick start: “`P` is enough at the slow
    profiles; for 400 kHz/1 MHz add a 1.5–2.2 kΩ external pull-up on SCL.”

23. **[minor]** `bus-operations.md:49-58` — the repeated-start example ends with
    the transaction open.
    > “I2C>[
    > I2C START BIT
    > I2C>0xA0
    > WRITE: 0xA0 ACK
    > I2C>0x00
    > WRITE: 0x00 ACK
    > I2C>[
    > I2C START BIT
    > I2C>0xA1
    > WRITE: 0xA1 ACK”
    **Problem:** the example shows the switch to the read address but no read or
    stop, so a novice cannot tell whether the missing `]` is intentional. The
    worked session later closes a similar transaction, but this first example
    should stand alone.
    **Fix:** add `I2C>r` and `I2C>]`, or one line: “the transaction is still
    open; read and stop as in the worked session.”

## What to keep

- The README file table and five-minute version. I reached every topic page in
  one click, and the map never pretended to be the manual.
- The one-line settings form plus the zero-based `mod`/`spd` table. This is the
  clearest explanation of a firmware header I have read in this tree.
- The wiring page's “open drain: only pulls low; the pull-up makes the high”
  line and the false-positive scanner symptom. That pair makes the mandatory
  pull-up memorable.
- The **“Quick tell”** for the two-space `READ:  ACK` line. It turns the most
  confusing part of the deferred ACK model into a visible cue.
- The failure captures, especially `WRITE: 0xA0I2C bus error`. That is what a
  bench user actually sees, and the explanation not to read column positions is
  exactly right.
- The EEPROM geometry table, the poll-until-ACK step and the repeated-start read
  sequence. I could follow it without translating from another source.
- The binary transaction-shape table. It answers the “which `0x08` form?” question
  before the reader has to parse the prose.
- The host/Pirate arrow exchange in `binary.md`; it is the right format for a
  host programmer. It only needs the missing `0x60` reply.
- The sniffer framing table and the v3/v4 overflow split. The distinction between
  “bytes dropped silently” and “writer waits, edges missed” is useful.
- The exact firmware strings in `macros.md`, including the `(3)` refusal and the
  `(4)` confirmation. Quoting the device is better than paraphrasing it.
- The central edge-case trigger/symptom/recovery tables. They are the fastest
  lookup in the set.

## Structural opinion

The layout works for this function. The README is 123 lines, the topic files are
parallel, and the reader can choose “setup”, “wire”, “traffic”, “recover”,
“host” without reading the whole set. I would keep the map-and-detail split.

Concrete changes:

1. **Add one end-to-end first session.** The first ACK currently comes from three
   pages: setup in `entry-and-setup`, bring-up in `wiring`/`entry-and-setup`,
   traffic in `bus-operations`. A single capture from `HiZ>` through `m4`, the
   driver/speed answers, `e2`/`W`/`P`, `(1)`, `[ 0xA0 … ]`, `r`, `]` would remove
   the stitching. Put it in the README or a `first-session.md`.
2. **Give the scanner one owner.** `README.md:26` says `macros.md` covers “the
   `(0)` list and macros 1–4”, but macro 1's format lives in `bus-operations` and
   macro 2 in `sniffer`. Either move the scanner section to `macros.md` or change
   the README row to say where each macro lives.
3. **Move “Buffers” out of “Names used in this set.”** It is a limits topic, not
   a naming table, and a reader looking for buffer sizes will not scan a names
   section.
4. **Deduplicate the v3/v4 and edge-case tables.** The same `m4 2 4`, `(3)` in
   Software, `P` without VPU and `0x0F` rows appear in two or three places. Keep
   the central `edge-cases.md` as the index and let topic pages link to the
   relevant section, or keep the per-page rows and remove the central duplicate.
   Right now the set has 30+ rows to keep in sync and the hardware-engine
   contradiction is exactly the kind of drift that follows.
5. **Add a terminal ↔ binary mapping table** in `binary.md`: `[` → `0x02`,
   `]` → `0x03`, `r` → `0x04`, ACK/NACK → `0x06`/`0x07`, value write → `0x1x`,
   `W`/`P`/`e` → `0x4x`/`0x5x`, speed → `0x6x`. That directly answers “how is
   this different from the terminal wrapper?” without making the reader hold two
   pages side by side.
6. **Add a six-line ASCII wiring sketch** to `wiring.md`: BP MOSI/CLK to
   SDA/SCL, each line through a resistor to Vpullup, common ground, target VCC
   separate. The table is correct, but the pull-up/Vpullup relationship is the
   one thing a new user can miswire.
7. **Keep the worked examples where they are.** The EEPROM example in
   `eeprom.md` and the binary exchange at the end of `binary.md` are well placed
   for a reader who has already chosen a path. Only the first session is missing.

## Unknowns

- I did not run the firmware or the Windows tool. The `W`/pending-ACK and
  “header typed back” findings are from reading the command behaviour and the
  set's own statements, not from a live board.
- I cannot verify the actual v3 on-board pull-up value, whether every v4 has a
  populated on-board EEPROM, or whether a v3 hardware-engine build is shipped
  rather than only source-buildable. The docs and SPEC disagree on the last one.
- I cannot verify the real SCL rates, the ~25 ms stretch bound, or the sniffer's
  usable ceiling on either board.
- I did not execute `I2CEEPROMWIN`; its retry loop has no limit in the source,
  so I cannot say how long it retries before a user gives up.
- I did not test `m0` on hardware; the generic navigation page and the I2C entry
  page disagree, and I trusted the generic page for the finding.
- I cannot judge whether the 5 V class limit is sufficient for every v3/v4
  revision; the docs label the absolute maximum unverified, and I have no
  schematic in front of me.
