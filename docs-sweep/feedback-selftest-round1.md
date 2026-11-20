# Self-test docs — fresh-reader feedback (round 1)

Read as a new v3/v4 owner: the six `docs/capabilities/selftest/` pages, the
cross-linked `docs/generic/` and `docs/protocol/binary/` pages, and
`TEMPLATE.md` / `SPEC.md` for context. I had no prior context and no hardware
attached; the "runs" below are paper traces of the documented paths and byte
sequences.

**Verdict: yes with fixes.** The default v4 terminal path is clear and
complete; the README's binary quick start strands the host in `BBIO1`, and the
optional SMPS refusal path contradicts itself and the SMPS set.

## Task log

### 1. First impression from the README

**Where I looked:** `capabilities/selftest/README.md`, first screen and the
availability table.

**How fast:** under a minute for all three questions.

**Success:** yes. The scope paragraph answers what is covered (control lines,
`AUX`, v4 EEPROM, rails, ADC, four I/O pins; `OK`/`FAIL` per check). The
availability table answers the board question (v4 documented, v3 present but
not verified). The five-minute version answers the disconnect question
("Disconnect every target").

**What was unclear:** "v4 is documented; v3 is build-only (not verified on
v3)" initially read as "v3 is not supported" rather than "compiled but
untested"; the table row "present, not verified on v3" fixes it. The sentence
"Fit `ADC`→`+3.3V` on v4, or `Vpu`→`+5V` and `ADC`→`+3.3V` on v3" packs two
boards and three jumpers into one bullet; I reread it to be sure v3 needs
**both** jumpers. The wiring table confirms it, but the quick start could say
"on v3, both".

### 2. Wire the jumpers, enter `~`, answer the prompts, run clean

**Where I looked:** README → `wiring.md` → `terminal.md` §Setup and §The run.

**How fast:** about four minutes to be confident.

**Success:** yes for v4 on paper. The wiring table is explicit (`ADC` →
`+3.3V`, one jumper; v3 needs two). The prompt text in `terminal.md` matches
the table, and the "no external pull-up / no external supply" statements are
repeated at the point of use. The note that `~` is a single-character command
and a queued CR answers the setup prompt is genuinely useful; I would otherwise
have pressed Space twice.

**What was unclear:** "Press any key — the prompt asks for Space" is a
self-contradicting sentence; it means any byte, but I paused. The v3 output
order is described, but there is no v3 run example (finding 9), so a v3 owner
has to assemble the output from the v4 capture and a table row. The wiring
table assumes the header is labelled `ADC` / `+3.3V` / `Vpu` / `+5V`; that was
fine on my board, but there is no pin-location aid.

### 3. Read the check reference

**Where I looked:** `checks.md`, starting at §Names, then the three check
tables and §Result and count.

**How fast:** about three minutes to find a check, its jumper, its expected
level, and the failure action.

**Success:** yes, and the arithmetic is consistent. v4 full: 5 control + 4
EEPROM + 6 supply + 12 pin = 27; short = 27 − (ADC + four `Bus Hi-Z 1`) = 22.
v3 full: 5 + 4 + 12 = 21; short = 21 − (VPU + ADC + four `Bus Hi-Z 1`) = 15.
The `0x10` column is the right mechanism for the split.

**What was unclear:** the table is the densest page in the set; the "If it
fails" column is terse and often not actionable ("check the supply and the
enable line"). The per-check causes here and the symptom triage in
`terminal.md` overlap, so I bounced between pages. Two adjacent checks look
contradictory: `WP` reads the write-protect line high, then `READ&WRITE` says
it wrote (finding 7). `PULLUP L` reads stronger than it is (finding 8).

### 4. Use the binary layer

**Where I looked:** README binary bullet → `binary.md` → the `0x10` / `0x11`
rows in `protocol/binary/commands.md` → `entering-and-framing.md`.

**How fast:** about three minutes for the page; the README quick start failed
me on the leave path.

**Success:** yes on `binary.md`. The two passes are distinct: `0x10` short/no
jumpers, `0x11` full/jumpers, and the `0x10` column in `checks.md` is the exact
split. The count byte, the idle echo table, the `0x00` + 2 errors = `0x02`
trap, and the `0xFF` vs `0x0F` distinction are all clear. The worked exchanges
are aligned and easy to follow.

**What was unclear:** the README says "Send `0xFF` to leave the test" and does
not say the host is still in `BBIO1` (finding 2). `binary.md` does say it, two
lines later, but a reader who trusts the quick start will type terminal
commands into the binary layer. The binary page's rationale for the short pass
also says "jumper-only pin group" on v4, where `Bus Hi-Z 1` needs no jumper
(finding 3).

### 5. Hit the SMPS-owned ADC refusal and a failure case

**Where I looked:** `terminal.md` §When the run does not start → `checks.md`
§When the run does not start → `smps/README.md` → `protocol/binary/edge-cases.md`
§ADC is busy.

**How fast:** about five minutes, and I left less confident than I started.

**Success:** the symptom is clear. `ADC busy: SMPS feedback active; command
rejected.` then `Syntax error at char 1`, no setup prompt, no `Found` line, no
check. The explanation that the syntax error is the dispatcher's response to
the flagged command error is genuinely helpful. The `0x01` ambiguity is
explained. The failure case (missing `ADC` jumper → one failure, six on v3 with
both jumpers missing) is clear.

**What was unclear:** the recovery. The docs say `0xF1` in raw wire or `#`, but
the SMPS page says the v4 terminal reset is **not confirmed** to stop the
SMPS, and a terminal-only user cannot send 20 NULs to reach raw wire. I could
not tell which action actually works (finding 1). For the failure case, the
table says `ADC FAIL`, but the printed line is `ADC(0.00) FAIL` (finding 6).

### 6. Rendering check

**Tabs:** none in the selftest, generic, or binary pages.

**Captures:** the terminal capture is plain text; the binary exchange comments
all start at column 25 in all three blocks. No misalignment.

**Wraps:** selftest prose wraps at about 80 columns; the only long lines are
table rows and link definitions. No broken wraps.

**Bench prose / dates / pass counts:** none in the selftest set. The v3 "not
verified" caveats are consistent and are caveats, not bench claims. No
"captured on", run dates, pass counts, or log names. `docs/dev/verification.md`
is the right place for that material.

**Presentation problems:** the 7-column check table and the 4-column edge-case
tables are the main issue (finding 5). The bullet list in `checks.md` §When the
run does not start mixes terminal and binary cases and would scan better as a
3-column table, but it is short enough to survive.

**Naming:** the `Vpu`/`VPU`, `+5V`/`5V`, `MODE`/`MODE LED`, and
`short`/`long test` mappings are explicit and work. Two silent mismatches:
"two internal pull-up rails" (finding 12) and `Selftest`/`self-test`
(finding 13).

### 7. README navigation test

**Where I looked:** README file table, five-minute version, and Related docs.

**How fast:** wiring, checks, binary, and edge cases are one click each; the
file table is at the top. Good.

**Dead ends:** no broken links. The friction is the "§Section" references that
land at the top of the target file instead of the section (finding 10), and
`wiring.md` not linking `binary.md` even though it mentions the binary full
pass (finding 11).

**Doubling back:** `checks.md` ↔ `terminal.md` for failure guidance, and
`edge-cases.md` repeats the same cases. Not fatal, but the set would be faster
with one failure owner.

## Findings

1. **[blocker]** `docs/capabilities/selftest/binary.md:153-155` says "The v4
   banner and `HiZ>` follow the last `0x01`, and the terminal reset stops the
   SMPS, so a later `~` runs normally." `docs/capabilities/selftest/terminal.md:164-172`
   then shows `HiZ>~` → `ADC busy: SMPS feedback active; command rejected.`
   and recommends "Stop the SMPS (`0xF1` in raw wire) or reset the board
   (`#`)". `docs/capabilities/smps/README.md:32-34` says the opposite:
   "only `0xF1` sent while still in raw wire is confirmed to stop it (a
   hardware reset also stops it; the v4 terminal reset is not confirmed to)."

   **Problem:** the SMPS is reachable only through raw wire, and the only
   documented way back to `HiZ>` is `0x0F`. If `0x0F` stopped the SMPS, the
   `HiZ>~` refusal could never happen. If it does not, the binary page's
   "so a later `~` runs normally" and the `#` recovery are wrong. A terminal
   user also cannot send the 20 NULs needed to reach raw wire for the `0xF1`
   route, so the recommended recovery is not actionable from `HiZ>`.

   **Fix:** pick one truth and make it consistent. Either remove the
   "not confirmed" caveat from the SMPS page and explain why the `HiZ>~`
   refusal can still occur, or remove "the terminal reset stops the SMPS" from
   `binary.md`; change the `terminal.md` / `checks.md` recovery from `#` to the
   v4 NORMAL button or power-cycle (hardware reset), and give `0xF1` as a
   host-programmer action with the 20-NUL / `0x05` preamble. Add a sentence
   for the terminal-only user.

2. **[major]** `docs/capabilities/selftest/README.md:33-35` says "Binary: enter
   `BBIO1` with 20 × `0x00`, then send `0x10` (short, no jumpers) or `0x11`
   (full, jumpers). The reply is one byte: the error count; `0x00` is clean.
   Send `0xFF` to leave the test." Lines 36-37 add "leaves the board in HiZ".

   **Problem:** `0xFF` leaves the self-test, not the binary layer. The reply is
   `0x01` and the host is still in `BBIO1` with no prompt; the next terminal
   command is consumed as a binary byte (`binary.md:86-90` and `:178-184`).
   "Leaves the board in HiZ" describes the pins, not the mode, and reads as
   "back at `HiZ>`". The bullet also drops the target-disconnect requirement
   for `0x10` (`binary.md:38-39`).

   **Fix:** "Send `0xFF` to leave the test (reply `0x01`); the host is still in
   `BBIO1`. Send `0x0F` to return to the terminal (`0x01`, then the banner and
   `HiZ>`)." Add: "`0x10` needs no jumpers but still needs every target
   disconnected and the board powered."

3. **[minor]** `docs/protocol/binary/commands.md:219-220` calls `0x10` "the
   pass without the external-jumper steps and without the jumper-only pin
   group." `docs/capabilities/selftest/checks.md:92-93` says "`Bus Hi-Z 1`
   runs only in the full pass, even on v4 where no external jumper is needed."

   **Problem:** on v4, `Bus Hi-Z 1` is not jumper-only, yet `0x10` skips it.
   A host author reading only the binary page will expect `Bus Hi-Z 1` to run
   on a v4 short pass. The exact table is correct; the summary is not.

   **Fix:** "the pass without the jumper-fed checks and without the `Bus Hi-Z
   1` group (skipped on both boards; on v3 it also needs the `Vpu` jumper)."

4. **[minor]** `docs/protocol/binary/commands.md:236-238` says "both commands
   report a count of `0x01` (one error) without running the test", while
   `docs/capabilities/selftest/checks.md:196-198` says "The `0x01` is a
   refusal count, not one failed check."

   **Problem:** a host author may log one failed check and display the wrong
   diagnosis. The value is the same; the meaning is not.

   **Fix:** change "(one error)" to "(refusal count; no check ran)".

5. **[minor]** `docs/capabilities/selftest/checks.md:60` is a 255-character
   table row (the `READ&WRITE` row); `docs/capabilities/selftest/edge-cases.md:39`
   is a 332-character row. The 7-column check table and the 4-column edge-case
   tables run off a normal terminal.

   **Problem:** the check name and the "If it fails" text are at opposite ends
   of the row; on an 80–100 column screen the reader scrolls horizontally and
   loses the row identity. This is the core reference table, so the cost is
   recurring.

   **Fix:** split the check table into a compact matrix (`Check | Jumper |
   0x10 | v3 | v4`) and a separate `Check | What it does | If it fails` table,
   or use a short definition per check. For `edge-cases.md`, split
   `Trigger/Symptom` from `Behaviour/Recovery`, or use per-case definitions.

6. **[minor]** `docs/capabilities/selftest/terminal.md:146` says "`ADC FAIL`
   alone" and `:147` says "`VPU FAIL` and four `Bus Hi-Z 1` failures", but
   `terminal.md:126` says measurement checks print as `name(v.vv) FAIL`, and
   the clean capture at `:92` prints `ADC(3.32) OK`.

   **Problem:** the actual failing line is `ADC(0.00) FAIL` (with whatever
   reading), not `ADC FAIL`. A user scanning for the literal table string may
   think the symptom is different.

   **Fix:** show `ADC(...) FAIL` / `VPU(...) FAIL`, or add "(the printed form
   includes the reading, e.g. `ADC(0.00) FAIL`)".

7. **[minor]** `docs/capabilities/selftest/checks.md:59` says `WP` "reads the
   on-board EEPROM write-protect line high" and failure means "the EEPROM is
   not write-protected"; `:60` says `READ&WRITE` "writes the inverse of a
   stored EEPROM byte".

   **Problem:** if `WP` is high (write-protected), how does `READ&WRITE`
   write? The check reference never says the test drives or releases WP for the
   write. Two adjacent checks appear to contradict.

   **Fix:** state the order/action: "`WP` reads the line high; for
   `READ&WRITE` the test drives WP low (or releases it), writes, restores the
   byte, and restores WP." If the hardware does something else, say what.

8. **[minor]** `docs/capabilities/selftest/checks.md:55` says "`PULLUP L` |
   disables the pull-ups and reads the control line low ... | the line did not
   return low; the pull-ups may stay on."

   **Problem:** a control line stuck low passes `PULLUP L` even though the
   pull-ups never switched on. The table presents the check as "disables the
   pull-ups", which is more than a low read proves. A reader may take
   `PULLUP L OK` as proof the pull-ups turned off.

   **Fix:** add "a line stuck low also passes this check; read it together
   with `PULLUP H` and `Bus Hi-Z 1`", and add that limit to §What is not
   checked.

9. **[minor]** `docs/capabilities/selftest/terminal.md:67` is "The run (v4,
   full test)"; the v3 supply order exists only as a table row at `:186`
   ("`5V`, `VPU`, `3.3V`, `ADC`").

   **Problem:** a v3 owner can wire the board and read the check table, but has
   no end-to-end output to compare against. The v3 column is marked
   unverified, so a full capture might overclaim; a short order sketch would
   still help.

   **Fix:** add a v3 block showing the group order and one line per group (not
   voltages), or an explicit "v3 prints the same lines in this order" list,
   marked source-derived/unverified.

10. **[minor]** Section references link to the file, not the section:
    `docs/capabilities/selftest/wiring.md:24-25` ("[terminal.md](terminal.md)
    §Setup"), `terminal.md:156` ("[checks.md](checks.md) §Voltage checks"),
    `terminal.md:173-174` and `binary.md:62` ("[checks.md](checks.md) §When
    the run does not start").

    **Problem:** the text promises a section but the link lands at the top of a
    220-line page, so the reader scrolls to find it. This is the only routine
    doubling back in the set.

    **Fix:** use anchors: `terminal.md#setup`, `checks.md#voltage-checks`,
    `checks.md#when-the-run-does-not-start`, and the equivalent for the binary
    edge-case section.

11. **[minor]** `docs/capabilities/selftest/wiring.md:13-14` says "The binary
    full pass assumes they are already fitted", but the Related docs at
    `:89-95` list terminal, checks, edge-cases, and auxiliary, not `binary.md`.

    **Problem:** a reader who lands on the wiring page for jumpers and then
    wants the binary full pass has to go back to the README.

    **Fix:** add `- [binary.md](binary.md) — the 0x10 / 0x11 passes.`

12. **[minor]** `docs/capabilities/selftest/README.md:46` says "`Vusb` and the
    two internal pull-up rails | — | yes"; `checks.md:18-19` calls the same
    items `5V VPU` and `3.3V VPU` ("the 5V rail; `5V VPU` is its pull-up
    source (v4)").

    **Problem:** "internal pull-up rails" is a new name for checks the reader
    will see as `5V VPU` / `3.3V VPU`; there is no mapping. A reader scanning
    the check labels cannot connect the two.

    **Fix:** use the labels in the README row, or write "(`5V VPU`, `3.3V
    VPU`)".

13. **[minor]** `docs/generic/navigation.md:125` shows the help row "`~` |
    Selftest", while the selftest set writes "self-test" throughout
    (`docs/capabilities/selftest/README.md:1`, "# Self-test").

    **Problem:** the firmware's displayed string is `Selftest`; the docs
    silently use the hyphenated prose form. A reader searching the help screen
    for "self-test" will not find it. The naming contract asks for one explicit
    mapping.

    **Fix:** add "the `?` help prints `Selftest`; this set writes self-test" to
    the selftest §Names or the help table note.

14. **[minor]** `docs/generic/README.md:72-77` says "Every header I/O pin is an
    input ... Nothing is driven except the selected AUX pin"; the selftest
    README `:11-13` says the self-test "takes over the pins and rails for the
    run".

    **Problem:** `~` runs in HiZ, so the generic page's absolute statement
    contradicts the self-test. The selftest page is clear; the HiZ page is not
    qualified.

    **Fix:** add "— and during a `~` self-test, which temporarily takes over
    the pins and rails."

15. **[minor]** `docs/generic/edge-cases.md:20` lists "`d`, `D`, `P`, `e`,
    `g`, `S` or `v` while the optional SMPS feedback owns the ADC" but omits
    `~`, which `docs/capabilities/selftest/terminal.md:160-168` says refuses
    with the same message.

    **Problem:** a user looking up the busy message in the generic edge cases
    will not see the self-test, even though it is one of the affected
    commands.

    **Fix:** add `~` to the list and link the selftest refusal section.

16. **[minor]** `docs/capabilities/selftest/wiring.md:71` says "Remove any
    other jumper, probe or external pull-up from the I/O pins", but the v3
    setup at `:19` fits `Vpu` → `+5V`, a direct connection to the pull-up
    reference pin.

    **Problem:** a v3 user with an external voltage on `Vpu` from a previous
    session is told to fit a jumper from that pin to `+5V`; the prep list only
    says to clear the I/O pins, not `Vpu`. The v4 row says "no jumper" but not
    "no external voltage".

    **Fix:** add "Remove any external voltage from `Vpu` before fitting the
    `Vpu`→`+5V` jumper; on v4 leave `Vpu` unconnected."

## What to keep

- The README is a real map: file table first, then a five-minute version,
  availability, limits, related docs. I never had to read it as a manual.
- The §Names table in `checks.md` is the single best thing in the set. It
  prevents the `Vpu`/`VPU`, `+5V`/`5V`, `MODE`/`MODE LED`, and
  `5V VPU`/`3.3V VPU` traps before they happen.
- The v4 full-run capture plus the line-by-line reading table. I could match
  every output line to a meaning, including the group headers and the
  measurement parentheses.
- The `Found 1 errors.` grammar note. Small, but it stops a "is my board
  broken?" reaction.
- The binary worked exchanges, especially the idle echo examples (`0xFE` + 2
  → `0x00`, `0x00` + 2 → `0x02`) and the `0xFF`/`0x0F` leave distinction. This
  is the clearest part of the set.
- The edge-case format (trigger → symptom → behaviour → recovery) and the
  concrete cascades: v4 3.3V rail off → 7 failures; v3 `Vpu` missing → 5;
  both jumpers missing → 6. These are numbers I can check against my own run.
- The "no external pull-up / no external supply / no abort key" warnings
  placed where they matter, not buried in a limits page.
- The "What is not checked" honesty, especially the one-byte EEPROM limit and
  the `MODE LED` drive-line distinction.

## Structural opinion

The six-file split is right for this function: README (map), `wiring`,
`terminal`, `checks`, `binary`, `edge-cases`. Do not merge them.

The main structural problem is **duplication without one owner**. The v3/v4
matrix and check counts appear in README, `terminal.md`, `checks.md`, and
`binary.md`; the SMPS refusal appears in `terminal.md`, `checks.md`,
`binary.md`, and `edge-cases.md`; the failure guidance appears in `checks.md`,
`terminal.md`, and `edge-cases.md`. That duplication is already producing
contradictions (findings 1, 3, 4). Proposed owners:

- `checks.md` owns the check matrix, the pass counts, and the SMPS refusal.
  README carries a three-row summary; `terminal.md` and `binary.md` link to it
  instead of repeating the full table.
- `edge-cases.md` owns the cross-cutting trigger/symptom/recovery cases;
  `terminal.md`'s failure table shrinks to a link plus two or three rows that
  are truly terminal-specific.
- The README quick start becomes two recipes: "Terminal" (four steps,
  including the LED prompt and the return to `HiZ>`) and "Binary" (four steps,
  including `0xFF` then `0x0F`). The current mixed bullets are where the leave
  path got lost.

Split the 7-column check table into a compact matrix and a separate
"what it does / if it fails" table. That makes the `0x10` split scannable and
removes the horizontal scrolling.

Add one short v3 output sketch (marked source-derived) or a compact "v3 order"
block. The v3 owner currently has wiring and a table but no end-to-end example.

Deep-link the section references. It is a small change that removes the only
routine doubling back.

## Unknowns

- I could not run the firmware. I cannot confirm the exact v4 capture, the
  printed volt strings, or the LED behaviour.
- The SMPS terminal-reset behaviour is the open contradiction; I cannot tell
  which page is right.
- Whether every v4 has the on-board EEPROM the checks assume. The selftest set
  says "yes"; the I2C set hedges "not verified" and "populated".
- Whether `WP` is driven low for `READ&WRITE`; the check table does not say.
- Whether `PULLUP L` is intended as a control-line-only check; the table reads
  stronger.
- Whether the v3 output order and counts are correct (the docs say unverified;
  I cannot test).
- Whether an external voltage on `Vpu` interferes with the v4 `5V VPU` /
  `3.3V VPU` checks; the wiring page says "no jumper", not "no external
  voltage".
- Whether the physical header labels match the doc names on both boards; I
  assumed they do.
- Whether the self-test preserves `o` / `l` / `;` display state; the post-run
  table lists pins, rails, AUX, and bus only.
- Whether the binary idle echo is modulo 256 on both boards (the docs say
  "wrapping"; I cannot test).
