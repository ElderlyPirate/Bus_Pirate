# OpenOCD adapter — reader feedback, round 1

**Reviewer:** first-time reader; owns a v3 and a v4; wants JTAG through the
OpenOCD adapter path. I read the nine files in `docs/tools/openocd/`, the
`docs/protocol/jtag/` pages they cross-link, the `docs/generic/` pages they
lean on, and `docs/TEMPLATE.md` / `docs/SPEC.md` for context. I did not read
the audit artifacts in `docs-sweep/`.

**Verdict: yes with fixes.** The set is well organized and the byte-level
detail is mostly strong, but its central power-state claim is inverted and
repeated in eight places, and the host-setup path stops at “build OpenOCD”
for the Windows user this board attracts. Fix the power model and add a real
“get a working host” block; the rest is polish.

## Task log

**1. First impression from the README alone.** ~2 min. I opened only
`README.md`. I could say what the adapter path is (a host-driven `0x06` /
`OCD1` TAP shifter), that v4 is supported and v3 is present but unverified,
that the host must be POSIX, and where the five JTAG wires go. That worked.
What did not: there is no “start here” pointer — `workflow.md` is the sixth
link, and the five-minute bullets do not say “then follow the workflow.” The
XSVF difference from the README alone is only “v4-only, file-driven”; the
real comparison is one click away in `overview.md`. The bullet “or set
`buspirate mode normal`” is missing the 3.3 V-only qualifier, which is a
safety problem for 1.8 V / 2.5 V targets.

**2. Follow the workflow.** ~15 min read. I followed `workflow.md` from wire
to triage with a 3.3 V target in mind. The sequence is complete: wire, drive
mode/power, prepare, connect, health check, run, exit, failure table. The
startup-order table matches what the OpenOCD driver actually does. The
POSIX-only warning is unmissable and correct; my complaint is the next step,
not the warning. Native Windows is out, Cygwin is “runtime unverified,” and
there is no build/install command, so a Windows user stops at step 4. The
health-check `V33` row also contradicts the no-back-drive wiring rule. And
the “leave cleanly” reasoning is wrong because the `0x00` exit side effects
are wrong (finding 1), even though the final state is right.

**3. Use the command table.** ~20 min. I used `commands.md` with
`exchanges.md` and `entry-and-exit.md` open. Reply shapes are clear:
`0x01` / `0x02` / `0x08` have no reply, `0x03` is ten bytes, `0x05` is the
3-byte echo plus TDO, `0x07` is two bytes. The pin-mode “both resets are
asserted” warning and the action `1` release table are excellent. The `0x07`
v3 handshake is the one place I would still get it wrong: the docs say to
switch the port before the confirmation, but not “send the selector and the
confirmation in separate writes.” A custom host that writes all four bytes in
one call will fail. The `0x05` post-shift TCK/TDI/TMS levels are only in
`wiring.md`, not in the command section. The two exits are clearly separated
in the text, but the `0x00` side effects are backwards.

**4. Set up the host.** ~10 min. The version matrix, option table, interface
and target configs, and custom-host rules are copy-paste ready. The fallback
“open the installed `interface/buspirate.cfg` and copy its spelling” is
practical. It is not executable end-to-end: there is no command to check a
packaged build for the driver, no build/install command, and no Windows route
beyond an unverified Cygwin build. The README’s “No configuration is shipped”
also reads as “you must write the interface file,” while `host-setup.md` says
to source the one OpenOCD ships.

**5. Hit the limits/errors.** ~15 min. The 8192 clamp, zero-count behavior,
surplus-payload desync, v3 fast-baud trap, and driver-message table are clear
and actionable. The weak points: the 8192 limit is defined as “one scan,”
“every scan including TAP moves,” and “per `0x05` block” in three places; the
“1024-byte buffer” is not explained; a clamped shift also returns fewer TDO
bytes, which is not stated; and the VREG/pull-up side effect is wrong
(finding 1), so the recovery table’s “leaves them as they were” is wrong.

**6. Rendering check.** ~20 min scan of all nine OpenOCD files plus the
cross-linked JTAG and generic pages. I found no TAB characters, no run or
test dates, no pass counts, no bench/rig/capture narratives, no “captured
on…” provenance, and no positive “tested”/“verified” claims. The allowed
“not verified / unverified” caveat is repeated a lot, and the dedicated
`## Unverified` section reads like an evidence ledger. The one clear layout
defect is `exchanges.md`: the inline comments start at different columns in
every block. The generic `v` captures are perfectly aligned, so the set can
do it. Naming is mostly disciplined (`VREG`/regulator, `Vpullup`/`Vpu`/`VPU`,
`AUX`/`A0`, `normal`/push-pull are all mapped once); the outliers are
“OpenOCD mode” vs “OpenOCD path,” “build-only,” and the missing `VREG` to
terminal-`W` mapping.

**7. README navigation test.** Under a minute to the top table; `workflow`,
`commands`, `wiring`, and `limits` are one click away, and every link I tried
resolves. No dead ends. I doubled back because the five-minute bullets repeat
the wiring and limits, and the bottom “Related docs” table repeats the top
index. `workflow.md` is row six, not marked “start here.” The “No
configuration is shipped” line sends a reader looking for files that
`host-setup.md` says exist in the OpenOCD install.

## Findings

1. **[blocker] The set’s central power-state claim is backwards.**
   - `README.md:94-95`: “`0x00` leaves `OCD1` but does not turn the regulator or the on-board pull-ups off; the `0x0F` terminal reset does.”
   - `entry-and-exit.md:44-45`: “| regulator (VREG) | **not changed by entry** |” and “| on-board pull-ups | **not changed by entry** |”.
   - `entry-and-exit.md:62-64`: “**The regulator and the on-board pull-ups are
     not reset by `0x06`.**  A session started from a board that a terminal
     left with the supplies on keeps them on until the host writes the
     matching feature.”
   - `limits-and-errors.md:109`: “**VREG and the on-board pull-ups keep
     their state**”; `limits-and-errors.md:113-114`: “Only the `0x0F` step
     switches the regulator and the on-board pull-ups off; a hand-driven
     exit that stops after `0x00` leaves them as they were.”
   - The same claim appears at `overview.md:67`, `entry-and-exit.md:70-71`, and `entry-and-exit.md:79`.

   **Problem:** The board turns the regulator and the on-board pull-ups off
   before `0x06` and again when `0x00` returns to `BBIO1`. The cross-linked
   `protocol/jtag/README.md:70` and `protocol/jtag/entry.md:146` both say
   “VREG and pull-ups off” after `0x00`; the binary entry table says the same.
   I checked the firmware reset path to break the tie: it clears both
   latches at entry and on the `0x00` return. A reader who trusts the
   OpenOCD set will be wrong about whether the target is still powered after
   exit — exactly the kind of thing that leads to touching a live rail or
   expecting a target to stay up.

   **Fix:** Make `entry-and-exit.md` the single owner and state: “`0x06`
   entry resets the BBIO pins, VREG and the on-board pull-ups; `0x00`
   returns to `BBIO1` and leaves them off; `0x0F` only adds the terminal
   return/reset. TRST is the exception and keeps its last level.” Delete every
   “keep their state” / “not changed by entry” sentence. Update the README
   limit bullet to say `0x00` already removes power; `0x0F` is for the
   prompt, not for the supply.

2. **[major] The README’s “or set `buspirate mode normal`” advice is unsafe
   without a voltage qualifier.**
   `README.md:42-43`: “fit **2–10 kΩ** pull-ups on TDI/TMS/TCK to the target
   rail, or set `buspirate mode normal`.”

   **Problem:** `normal` is push-pull 3.3 V. A 1.8 V or 2.5 V target must stay
   open-drain with pull-ups to its own rail. The detailed warning is in
   `wiring.md:105-106`, but the first-screen advice does not carry it. A
   fresh reader with a 1.8 V target can pick the wrong branch before reaching
   the detail page.

   **Fix:** “...or, for a 3.3 V target only, set `buspirate mode normal`. A
   1.8 V or 2.5 V target must stay open-drain with pull-ups to its own rail.”

3. **[major] The 8192-bit limit is described in three different units, and
   the safest wording is not the one in the README.**
   - `README.md:47-48`: “Keep one scan at or below **8192 bits**; OpenOCD does not split a longer scan.”
   - `limits-and-errors.md:59-60`: “Keep every scan, including its TAP state moves, inside 8192 bits.”
   - `commands.md:208-211`: “Keep every shift at or below 8192 bits and split
     longer scans.  OpenOCD flushes its queue at the buffer boundary but does
     not split one longer scan; a single scan above 8192 bits overflows the
     driver’s buffer and aborts the session.”

   **Problem:** A `0x05` block, a scan, and an OpenOCD batch are not the same
   thing. OpenOCD packs several scans plus their TAP state moves into one
   block. A scan of 8192 bits plus the moves around it can still overflow,
   even though the scan itself is “at or below 8192.” The “1024-byte buffer”
   at `limits-and-errors.md:54` is also ambiguous: it is per lane (TDI and
   TMS), not 1024 bytes total; a reader doing the arithmetic gets 4096 bits
   and loses confidence.

   **Fix:** Define one term: “One `0x05` block is one request; it carries at
   most 8192 bits, including every TAP move in that block. OpenOCD batches
   scans into one block; keep the whole batch under 8192 bits with margin.
   The driver has two 1024-byte lane buffers (TDI/TMS).” Use that wording in
   README, overview, commands and limits.

4. **[major] The v3 `0x07` handshake is described, but the “do not pipeline”
   rule is missing.**
   `commands.md:227-231`: “On **v3** the firmware changes its UART rate as
   soon as it reads the selector, **before** the confirmation bytes.  The
   host must switch its own port to the new rate before sending `0xAA 0x55`.”
   `commands.md:41`: “| `0x07` | 1 byte: selector, then `0xAA 0x55` | set the
   host link speed | `0x07`, selector |”

   **Problem:** A custom host can read “selector, then `0xAA 0x55`” as one
   write. The firmware changes rate after the selector; the confirmation must
   be sent after the host switches, in a separate write. If the host writes
   `0x07 0x01 0xAA 0x55` in one call, the confirmation is at the old rate and
   the handshake fails. The reply table also says “selector” unconditionally;
   a bad confirm returns `0x00`.

   **Fix:** Write the sequence as three steps: send `0x07 selector`; switch
   the host port; send `0xAA 0x55`; read `0x07 selector` (or `0x07 0x00`
   after a bad confirm). Add “Do not send the selector and confirmation in
   one write” to the custom-host table.

5. **[major] Host setup is not executable for the reader who most needs it.**
   `host-setup.md:48-49`: “Use a build that includes it, or build OpenOCD
   with the adapter enabled.”
   `workflow.md:72-74`: “Check that the OpenOCD build includes the Bus Pirate
   driver.  The driver is POSIX-only: native Windows (MinGW/MSYS) builds do
   not contain it.  Use Linux, macOS, BSD or a Cygwin build (runtime
   unverified).”

   **Problem:** There is no command to check a packaged build, no
   configure/build command, and no Windows next step. A Linux user with a
   distro OpenOCD cannot tell whether the driver is present until startup
   fails; a Windows user is told “Cygwin, unverified” and has nowhere to go.
   The warning is correct and prominent; the missing part is the action after
   it.

   **Fix:** Add a short “Get a working host” block:
   `openocd -c "adapter list"` to check for `buspirate`; if missing, build
   with `./configure --enable-buspirate` (or the flag your release uses) and
   confirm the configure summary lists Bus Pirate. On Windows, say native
   builds are out, Cygwin/WSL are unverified and need USB passthrough, or use
   a Linux VM. Put it before the option matrix.

6. **[major] The health check tells the reader to look for a target-driven
   3.3 V rail, which contradicts the no-back-drive rule.**
   `workflow.md:119`: “| `V33` | the `3.3V` rail, either from VREG or from a
   target that drives it |”
   `wiring.md:55-56`: “the target has its own supply | `0` | no rail
   connection; the target keeps its own supply”

   **Problem:** If the target has its own supply, the docs say not to connect
   it to the board rails. The only way `V33` reads a target voltage is if the
   target is connected to the 3.3 V rail, i.e. back-driving. The row invites
   exactly the wiring the rest of the set forbids.

   **Fix:** “`V33` is the board’s 3.3 V rail. With VREG off and no board rail
   connection it reads 0; do not connect a self-powered target’s supply here
   just to measure it.” Keep the no-back-drive sentence next to it.

7. **[minor] The worked exchanges have jagged comment columns.**
   `exchanges.md:36-40`:
   ```
   host -> 0x01 0x01         pin mode: JTAG push-pull
   host -> 0x02 0x02 0x00    VREG off
   host -> 0x02 0x10 0x00    on-board pull-ups off
   host -> 0x02 0x04 0x01    TRST high: released
   host -> 0x02 0x08 0x01    SRST high: released
   ```

   **Problem:** The comments start at columns 17, 22, 22, 22 and 22; other
   blocks start at 12 or 16. The eye cannot scan the values. This is the one
   page that looks like a scratchpad.

   **Fix:** Pad every request to a fixed width and put all comments at one
   column (for example 25), or make each block a three-column table:
   Host sends | Meaning | Board replies.

8. **[minor] `0x05` post-shift line levels and final-group padding are
   missing from the command section.**
   `commands.md:173-175`: “Each bit sets TDI and TMS while TCK is low, pulses
   TCK high, and samples TDO.”
   `wiring.md:140`: “TCK is left high after a shift and between shifts.”

   **Problem:** A custom host needs to know the idle levels after the last
   bit. The command page is the natural place, but the fact is only in the
   wiring page. The same section never says what happens to the unused high
   bits of a final 9–16-bit group; a reader sending 9 bits sends 4 payload
   bytes and may wonder whether the upper 7 bits are clocked.

   **Fix:** Add: “After the last bit TCK is high and TDI/TMS hold their last
   values. In a final 9–16-bit group the unused high bits are ignored; still
   send the full four payload bytes.”

9. **[minor] A clamped shift also returns fewer TDO bytes, and the docs only
   mention the surplus payload.**
   `commands.md:200-206`: “A count above 8192 is clamped to 8192.  The echo
   still repeats the requested count, so a host cannot see the clamp in the
   reply.” ... “the surplus bytes stay in the serial stream and are read as
   command bytes.”

   **Problem:** The TDO reply is sized for the clamped 8192 bits. A host that
   expects TDO for the requested count will be short by the same amount and
   can hang before it gets to the surplus-command symptom.

   **Fix:** Add “the TDO reply is sized for the clamped count; expect fewer
   TDO bytes than the requested count.” Put it next to the echo sentence.

10. **[minor] “The five bitbang pins” is undefined, and the sentence reads
    as a contradiction.**
    `entry-and-exit.md:79`: “the five bitbang pins become inputs; TRST keeps
    its last level; VREG and the on-board pull-ups keep their state”

    **Problem:** A JTAG reader counts TDI, TMS, TCK, SRST and TRST as the five
    outputs. If all five became inputs, TRST could not “keep its last level.”
    The five here are the BBIO pins (AUX, CS, MISO, CLK, MOSI), not the JTAG
    five. The same “five” is used for the pin-mode outputs in
    `commands.md:74`, so the reader has two different fives.

    **Fix:** Name them: “the five BBIO pins (AUX, CS, MISO, CLK, MOSI) become
    inputs; TRST (AUX1/PGD) is separate and keeps its last level.” (After
    fixing finding 1, drop the VREG/pull-up clause.)

11. **[minor] The cross-linked JTAG entry page says `0x06` resets TRST; the
    OpenOCD page says it does not.**
    `protocol/jtag/entry.md:75-76`: “`0x06` entry resets the pin mode, VREG,
    pull-ups, TRST/SRST levels and TCK delay, but not the mode LED or the
    link rate.”

    **Problem:** `entry-and-exit.md:52-54` says “TRST ... the binary reset
    does not touch; a pin mode takes it over.” A reader following the
    “Related” link gets two answers. The rest of the cross-link line (VREG,
    pull-ups) is right and is the authority for finding 1; TRST is the one
    word too many.

    **Fix:** “...resets the pin mode, VREG, pull-ups, SRST level and TCK
    delay; TRST is not changed by entry.”

12. **[minor] Two cross-linked TAP details are easy to misread as OCD1
    commands.**
    `protocol/jtag/tap.md:201-202`: “optional pin; OpenOCD feature `0x04`” /
    “optional pin; OpenOCD feature `0x08`”
    `protocol/jtag/tap.md:203`: “Bus Pirate board reset | pins, supplies,
    pull-ups | `#`, reset button, or second `0x0F`”

    **Problem:** In the OCD1 command table, `0x04` is the retired shift byte
    and `0x0F` is not a command at all. Calling them “feature `0x04`” and
    “second `0x0F`” without the `0x02` prefix or the XSVF context invites the
    exact mistakes the OpenOCD set warns against.

    **Fix:** “feature selector `0x04` (sent as `0x02 0x04 action`)” and
    “`0x00` then `0x0F` on the OpenOCD path; `0x0F` twice on the XSVF path.”

13. **[minor] Four naming/terminology gaps.**
    - `protocol/jtag/openocd.md:32`: “leave OpenOCD mode” — the README’s
      naming table defines “OpenOCD path” (firmware) and “OpenOCD adapter”
      (host); “OpenOCD mode” is a third form with no mapping.
    - `README.md:4-5` and `README.md:92`: “v3 is build-only” — a fresh reader
      does not know whether the path is absent, compiled but untested, or
      limited. The table says “present, not verified on v3,” which is clearer.
    - `commands.md:106`: “regulator (VREG), driving the `3.3V` and `5.0V`
      rails” — the terminal calls the same thing `POWER SUPPLIES`
      (`generic/README.md:149`), and `W` is the switch. A reader coming from
      the terminal has to infer that `buspirate vreg 1` is the same control.
    - `README.md:15`: “The other JTAG path is the **XSVF player** (`0x18`,
      answers `XSV1`, v4 only).” The XSVF set calls the same thing the
      “firmware player”; the mapping is in `protocol/jtag/README.md`, not in
      this README.

    **Fix:** Use “OpenOCD path” everywhere; replace “build-only” with
    “compiled into the v3 build, not hardware-verified”; add “VREG is the
    same on-board regulator that `W` switches (`POWER SUPPLIES ON`)” and
    “the XSVF set calls this the firmware player.”

14. **[minor] “No configuration is shipped” reads as “you must write the
    interface file,” which the next page contradicts.**
    `README.md:90`: “- **No configuration is shipped.**  Interface and target
    files are reference material.”
    `host-setup.md:141`: “source [find interface/buspirate.cfg]”

    **Problem:** OpenOCD ships `interface/buspirate.cfg`; the reference target
    config sources it. The README means “this project ships no config,” but a
    fresh reader may spend time recreating a file that is already installed.

    **Fix:** “No project config is shipped; OpenOCD’s installed
    `interface/buspirate.cfg` is the starting template, and the target configs
    below are examples.”

15. **[minor] The limits page mixes exact driver messages with symptom
    descriptions.**
    `limits-and-errors.md:174-175`: “| IDCODE found, then examine or flash
    fails | ... |” and “| unknown adapter driver at startup | ... |”

    **Problem:** The table is headed “Driver messages” / “Target and chain
    messages,” but these two rows are not messages; one is a sequence of
    events, the other is a startup condition. A reader scanning for the
    literal text has nothing to match.

    **Fix:** Rename the first column “Message or symptom” or move the two
    descriptions to the target/chain table and keep only quoted text in the
    message table.

16. **[minor] The dedicated `## Unverified` section reads like a verification
    ledger and repeats inline caveats.**
    `limits-and-errors.md:177-187`: “## Unverified” ... “The v3 build
    includes the path; its hardware behaviour is **not verified on v3**.” ...
    “A Cygwin build ... runtime is unverified.”

    **Problem:** The same caveats already appear in the availability tables
    and in the v3 column. A whole section at the end invites the reader to
    treat “unverified” as a status report rather than a caveat. It is not
    bench prose, but it is the one place the set looks like an audit record.

    **Fix:** Fold the first two bullets into the availability table (they are
    already there) and the last two into the relevant limits sections; keep
    one short inline caveat where it changes what the reader can rely on.

## What to keep

- The naming table at the top of the README (path vs adapter vs XSVF;
  `AUX` / `AUX1` / `PGD`) did the most work for orientation. Keep it exactly
  where it is.
- The pin-mode “both resets are asserted” warning and the action `1` release
  table. That is the single most useful trap in the set.
- The `0x05` grouping / echo / bit-order section and the 8-, 16-, 17- and
  43-bit examples. The 43-bit TAP walk in `tap.md` is the best explanation of
  the TAP I read.
- The v3 fast-baud trap and the driver-message table. They name the exact
  symptom and the recovery, which is what a stuck user needs.
- The host option matrix and the “copy the spelling from the installed
  `interface/buspirate.cfg`” fallback. Practical and release-tolerant.
- The XSVF comparison table in `overview.md`. It answers “why not just use
  XSVF?” in one screen.

## Structural opinion

The nine-file layout is right for this function. Keep the README as a map,
the overview as the XSVF comparison, the workflow as the end-to-end path,
the commands page as the byte reference, and the limits page as the failure
home. Do not collapse it into a manual.

The one structural problem is that the same volatile facts are restated in
too many files. The VREG error is a direct symptom: the entry state is stated
in README, overview, entry-and-exit, commands, workflow, exchanges and limits,
so one wrong copy became five. Concrete proposal:

- `entry-and-exit.md` owns the entry/exit state and the `0x00` vs `0x0F`
  side effects. Other pages keep at most a one-line pointer.
- `limits-and-errors.md` owns the 8192 block, the buffer accounting, zero
  counts, desync and recovery. README and overview keep one sentence and a
  link, not a second definition.
- `wiring.md` owns the pin map, rails, pull-ups and resets. README and
  workflow keep the headline table only.

Navigation: add one line after the five-minute version — “First session:
[workflow.md](workflow.md); host build: [host-setup.md](host-setup.md); byte
lookup: [commands.md](commands.md).” Move `workflow.md` above `host-setup.md`
in the link table, or add a “Read first” column. The table is currently a
file index, not a reading order.

`host-setup.md` is two pages in one: “get a working OpenOCD with the driver”
and “configure OpenOCD.” Split it, or put a `## Get a working host` section
before the option matrix. That is the only place the setup is not executable.

`exchanges.md` is useful but reads like a scratchpad. Align the comments or
turn each block into a table. It is the one page where the presentation gets
in the way of the content.

Trim the bottom “Related” tables to the two or three links that matter for
that page. The README already indexes the set; the repeated tables create the
doubling-back feeling in the navigation test.

## Unknowns

- I could not verify the exact OpenOCD buffer accounting from the docs. The
  1024-byte figure and the 8192-bit figure need a source note or a worked
  arithmetic example; I would not trust my own arithmetic from the current
  wording.
- I could not verify v3 behavior; I have no v3 hardware, and the docs say it
  is unverified. “Build-only” is doing a lot of work there.
- I could not verify the Cygwin runtime, or whether a packaged OpenOCD on my
  distro includes the Bus Pirate driver.
- I could not verify the exact driver message wording across 0.10, 0.11 and
  0.12; the docs caveat that, which is fair.
- I could not judge the electrical limits (2–10 kΩ, 5.5 V maximum, 3.3 V
  push-pull into a 5 V target) without hardware; they are plausible but not
  testable from the page.
- I did not run a real session. All task results are document-level: I read,
  cross-checked links, and followed the workflow as a reader.
