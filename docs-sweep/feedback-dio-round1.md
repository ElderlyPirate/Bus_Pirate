# DIO — reader feedback, round 1

**Verdict: yes with fixes.** The core is clear and the command/bit tables are
good, but the safe-park section contradicts itself in several places and the
only full worked session drives all five pins as outputs — both are unsafe
guides for a live target.

I read this as a first-time user with a v3/v4 board and no prior DIO context.
Read-only; no hardware was used for this review.

## Task log

### 1. First impression from the README alone

- **Where I looked:** `README.md` only, then the file table.
- **How fast:** 2–3 minutes.
- **Success:** Yes. I could say what DIO is (direct pin control, no protocol
  engine), that it is in the default image, what the prompt is, and what the
  first commands do.
- **Unclear:** The README says a build can leave DIO out, but not how to check
  my own build. `i` would show the compiled mode list; `m` would show the
  menu. Also, `m10`/`m9` are stated absolutely even though a custom build can
  renumber. Finally, the five-minute wiring bullet does not say whether to
  connect a target supply rail or leave it unconnected.

### 2. Enter, wire, set a pin, read it back

- **Where I looked:** README → `wiring.md` → `pin-control.md` → `read-back.md`
  → `park-and-reset.md`.
- **How fast:** 8–10 minutes.
- **Success:** Partly. The entry forms, the bit-7 split, the direction/state
  bit maps and the absolute-state rule are all followable after reading
  `pin-control.md` §State values and §Direction and level are separate. The
  "state first, direction second" advice is excellent.
- **What failed:** There is no worked example for **one** pin. The only full
  session makes all five pins outputs (`0x00`), and the read example makes
  four pins outputs (`0x02`). With a target connected, that can drive against
  the target. The safety section says the opposite, but I found it after the
  tutorial. I would not follow the tutorial on a live target without checking
  every pin first.

### 3. AUX controls and target naming

- **Where I looked:** README names table → `aux-pin.md`; `generic/commands.md`
  for the target mapping.
- **How fast:** 4–5 minutes.
- **Success:** At the moment of selection, yes. The `c` / `C` / `k` / `K`
  table is clear, and the page explicitly warns that the `a` / `A` / `@`
  replies always say `AUX` even when the target is `CS`, `AUX1` or `AUX2`.
- **Unclear:** After the confirmation line scrolls, I have no read-only way to
  see the current target. Re-issuing `c` / `C` / `k` / `K` changes it. I also
  saw `AUX0` in two places with no mapping, and a v3 user sees `aux/CS` in the
  `?` help while the DIO page says `A0`/`CS`.

### 4. Park and reset

- **Where I looked:** README quick start → `park-and-reset.md` → `edge-cases.md`.
- **How fast:** 6–8 minutes.
- **Success:** Partly. The `w` vs `#` table is clear, and the `AUX1`/`AUX2`
  exception is explained well.
- **What failed:** The contract is not consistent. `park-and-reset.md:51`
  says the difference is "not whether the header is released"; the exception
  says `w` does **not** release `AUX1`/`AUX2`. The verification section says
  after `w` or `#`, `AUX1`/`AUX2` "must also show `I`" — false after `w`.
  The README quick start says `w` or `#` and "every I/O pin must be `I`", and
  the `m`+`x` recovery says "use `w` or `#` if you wanted to park." I would
  follow the quick start, see `O` on a driven `AUX1`/`AUX2` (or not check),
  and start rewiring. This is the one section I most needed to be
  unambiguous.

### 5. Write-with-read, macros, `v`/`r`, one edge case

- **Where I looked:** `read-back.md`, `macros.md`, `edge-cases.md`.
- **How fast:** 6 minutes.
- **Success:** Yes. `{` does not enable read-back is clear; `r` is levels only
  and `v` is directions/rails is clear; the LSB-first + `:N` alternation is a
  good trap to document. The macro recall (store then press Enter) is clear.
- **Unclear:** `{` is described with a syntax error in `edge-cases.md` but
  without one in `read-back.md`. `W` after `w` says "levels stay low", but
  the pins are inputs and float; the latches are what stay low. The
  `0x40`/`0x20` direction-write trap is not listed. The blank line after
  `<1>` is explained in `generic/macros.md`, not on the DIO page.

### 6. Rendering check

- **Where I looked:** every DIO file, plus the generic pages they lean on.
- **How fast:** 5 minutes.
- **Success/failures:** No TAB characters found. No bench/rig/date/test-result
  prose found; the only verification language is the negative "not verified" /
  "unverified" caveat, which is appropriate but repeated eight times. No
  unreadable option lists. Captures are mostly fixed-column and readable. The
  `v` capture lacks the "real output is tab-separated" caveat that
  `generic/usage.md` gives. One README list is mislabeled as value order. A few
  paragraphs end with a single orphan word. `AUX0` / `A0` / `aux` are three
  forms for the same pin without a complete mapping.

### 7. README navigation test

- **Where I looked:** README file table → `wiring.md` (pin map), `read-back.md`,
  `park-and-reset.md`, `edge-cases.md`.
- **How fast:** 1–2 minutes to reach each; no dead links or broken anchors.
- **Success:** Yes, the file table is a good front door.
- **Doubling back:** The pin map is split across `wiring.md` (header position),
  `pin-control.md` (value bit) and `generic/usage.md` (v3 `v`-table order).
  The quick start's absolute mode numbers do not match a custom build. The
  safe-park check sends me to `v`, but `v` cannot show the selected AUX target.

## Findings

### 1. [blocker] The safe-park contract contradicts the AUX1/AUX2 exception

- `park-and-reset.md:51`: "The difference is how much state is reset, not
  whether the header is released:"
- `park-and-reset.md:107–110`: "`w` parks the five signal pins.  It does
  **not** release `AUX1` or `AUX2` … it stays driven through `w`."
- `park-and-reset.md:122–125`: "After `w` or `#`, `v` is the check: … `AUX1` /
  `AUX2` must also show `I` if they were used;"
- `README.md:44–46`: "Before touching the wiring, run `w` or `#` and check
  `v`: every I/O pin must be `I` …"
- `edge-cases.md:170`: "Recovery: use `w` or `#` if you wanted to park."
- `entry-and-setup.md:130–131`: "`w` runs the mode's safe park, which releases
  the header before the rails go off."

**Problem:** The page contains the right fact in the exception, then denies it
in the definition, the verification and the recovery. `w` releases only the
five signal pins. If `AUX1` or `AUX2` was driven, `v` after `w` shows `O`, and
the check tells me it "must" show `I`. A user following the quick start can
believe the header is parked and rewire while a pin is still driven.

**Fix:** Define "parked" as **the five signal pins** throughout. Say in the
`w` row: "releases the five signal pins; `AUX1`/`AUX2` are unchanged." Split
the check:

- after `w`: five signal pins `I`, rails `0.00V`;
- after `#` (or a mode change): every I/O pin `I`, including `AUX1`/`AUX2`.

Change "run `w` or `#`" to "run `#` if `AUX1`/`AUX2` may have been driven; for
the five signal pins `w` is enough." Change the `m`+`x` recovery to "`#` (or a
mode change) if `AUX1`/`AUX2` may be driven; `w` only releases the five."
Remove the "not whether the header is released" clause or replace it with
"`w` does not release `AUX1`/`AUX2`."

### 2. [blocker] The only full worked session drives all five pins as outputs

- `pin-control.md:100–101`: "engage the supply, load `CLK` high while the pins
  are still inputs, then make all five pins outputs:"
- `pin-control.md:111–112`: "DIO>0x00 / WRITE: 0x00"
- `park-and-reset.md:90–94`: the same `0xC4` then `0x00` sequence.
- `read-back.md:98`: "`0x02` makes `MISO` an input and the other four pins
  outputs; …"
- `wiring.md:121–124`: "Never drive a pin against an external source. … Direct
  I/O does not detect contention."
- `README.md:33`: "Wiring: the five signal pins plus common ground."

**Problem:** The tutorial's only complete recipe, and the read example, make
pins outputs. With a target connected, `MISO` is normally driven by the
target, and `CS` low can make it drive; the Bus Pirate then drives against it.
The safety section says never to do this, but it is easy to read the tutorial
first. The quick-start wording "the five signal pins" also implies all five
should be wired.

**Fix:** Split the example by what is connected. For a no-target test, the
all-outputs `0x00` step is fine, but label it "nothing connected." For a
connected target, drive only `CLK`:

```
DIO>0xC4
WRITE: 0xC4
DIO>0x1B
WRITE: 0x1B
DIO>v
```

`0x1B` leaves `AUX`, `MOSI`, `MISO` and `CS` inputs and makes only `CLK` an
output. Confirm it with `v` (CLK is `O` and `H`), not with `r`: the other four
inputs may float, so `r` need not be exactly `0x04`. For the external-read
example, say `0x02` drives the other four, so use it only if those nets are
safe to drive; use `0x1F` instead and expect the unconnected input bits to
float. Add a one-line warning before any all-outputs example: "Only do this
if all five nets are safe to drive." Add a single-pin example — the task is
"set a pin", and the only recipe is all five.

### 3. [major] `0x40` and `0x20` are direction writes, not "supply on" / "pull-ups on"

- `pin-control.md:14–20`: bit 7 chooses direction (`0x00–0x7F`) or state
  (`0x80–0xFF`).
- `pin-control.md:40–42`: "Bits 4–0 select each pin's direction; … Bits 6–5
  are ignored."
- `edge-cases.md:18–24`: the "level value without bit 7" example uses `0x04`,
  not `0x40`.

**Problem:** A reader who has just learned "bit 6 = supply, bit 5 = pull-ups"
from the state table can send `0x40` to power the target. Bit 7 is clear, so
`0x40` is a **direction** write: all five pins become outputs, and the supply
does not change. `0x20` is the same with all outputs, not pull-ups. This is a
silent, dangerous surprise.

**Fix:** Add an edge case: "`0x40` is not supply on — bit 7 is clear, so it is
the direction byte `0x00` (all five outputs); the supply stays as it was. Use
`0xC0`. `0x20` is not pull-ups; use `0xE0` (supply on) or `0xA0` (supply
off)." Put it next to "A level value without bit 7 is a direction write."

### 4. [major] `r` does not show `AUX1` or `AUX2`

- `aux-pin.md:56–58`: "The three commands affect only the selected pin: they
  do not change the other four pins … so `r` and `v` show the result."
- `aux-pin.md:118–120`: "A later direction value can override the direction
  set by `a` / `A` / `@` for the same pin …"

**Problem:** `r` reads the five signal pins only. After `k` then `A`, `r` will
not show `AUX1`; only `v` will. The "other four pins" wording also assumes the
selected pin is one of the five, which is false for `AUX1`/`AUX2`. A numeric
direction value cannot override `AUX1`/`AUX2` at all.

**Fix:** "For `AUX` and `CS`, `r` and `v` show the result; for `AUX1`/`AUX2`,
only `v` does. A numeric direction value can override `AUX`/`CS`, not
`AUX1`/`AUX2`." Use "the other pins" rather than "the other four" when the
target may be one of the extra pins.

### 5. [major] `AUX0` is a third name, and the v3 `aux` token is missing

- `pin-control.md:187`: "| `AUX` bit drives | `AUX` pin | `AUX` (`AUX0`) pin |"
- `aux-pin.md:113`: "| numeric bit 4 | `AUX` (`AUX0`), regardless of `c` /
  `C` / `k` / `K` |"
- `README.md:70`: "| `AUX` (`A0`) | auxiliary header pin; bit 4 of a value |"
- `aux-pin.md:34`: "The `?` help lists the assignment tokens `A0` / `CS` /
  `A1` / `A2`;"
- `generic/commands.md:128`: "| `A0` (`aux` on v3) | `c` | …"

**Problem:** `AUX0` is a third form for the same pin. It is not in the names
table, and I cannot tell whether it is the `AUX` pin, the `A0` token, or a
separate pin. On v3 the help actually prints `aux`, not `A0`; the DIO set
never says so.

**Fix:** Use `AUX` (`A0`) everywhere in user prose and remove `AUX0`. Add
"on v3 the help token is `aux`" to the names table or the AUX page, and link
to `generic/commands.md#aux-targets-and-pin-names`.

### 6. [major] After the confirmation line scrolls, I cannot tell which pin I am touching

- `aux-pin.md:52–54`: "The drive and read replies always say `AUX`, even when
  the selected target is `CS`, `AUX1` or `AUX2`; the confirmation line … is
  the record of the real target."
- `generic/navigation.md:106`: "AUX assignment | `a/A/@ controls AUX pin` (or
  `CS` / `AUX1` / `AUX2`)"

**Problem:** The warning is correct, but it leaves me with only scrollback.
`v` shows pin states, not the selection. Re-issuing `c` / `C` / `k` / `K`
changes the target. I can drive the wrong pin after a few commands.

**Fix:** Add a "Which target is selected?" note: "Run `i`; the status block
prints `a/A/@ controls <pin> pin`. `v` shows the pin states, not the
selection. Do not use `c`/`C`/`k`/`K` just to check, because they change it."
Cross-link it from `aux-pin.md` §Selecting the target and from the README
five-minute bullet.

### 7. [major] `W` is described as both "no setup" and "a silent setup step"

- `entry-and-setup.md:93–95`: "Direct I/O has no setup step, so the
  finish-setup line has nothing to finish; `W` only turns the rails on."
- `entry-and-setup.md:99–100`: "There is no prompt chain and no `W`-time
  setup.  Entering the mode applies a fixed base state; `W` starts the rails
  and runs a silent setup step."
- `generic/navigation.md:213–215`: "`W` starts the supplies and runs the
  mode's bus setup …"

**Problem:** Adjacent sentences say there is no `W`-time setup and that `W`
runs a silent setup step. I do not know whether `W` can change pins. The
page later says `W` does not change directions or levels, but I had to find
that separately, and the generic page suggests a bus setup exists.

**Fix:** Use one sentence in `entry-and-setup.md`, `park-and-reset.md` and
`generic/navigation.md`: "`W` starts the rails and runs the mode's setup hook,
which is empty in DIO; it does not change pin directions, levels, supply
bits or pull-ups." Or simply "`W` only turns the rails on" and drop "silent
setup step."

### 8. [major] The quick start does not qualify the target supply rails

- `README.md:33–35`: "Wiring: the five signal pins plus common ground.  **No
  pull-up is required.**  `W` turns on the on-board `+5.0V` and `+3.3V` rails;
  `w` turns them off *and parks the five pins*."
- `wiring.md:52–55`: "Both rails are live on the header while the supply is
  on; connect only the rail the target uses.  Do not tie the two rails
  together and do not back-drive them.  For a self-powered target, leave both
  rails unconnected."

**Problem:** The five-minute version says "the five signal pins plus common
ground" and "`W` turns on the rails", but not "connect only one rail" or "leave
both unconnected for a self-powered target." A reader following only the
quick start can back-drive a target supply. It also reads as if all five
signal pins must be wired.

**Fix:** Add one clause to the README bullet: "Connect GND, the signal pins
you use, and at most one rail to a bus-powered target; leave both rails
unconnected for a self-powered target." Say "the signal pins you use" rather
than "the five signal pins."

### 9. [minor] The "what survives" table has no mode-change column and miscounts its rows

- `park-and-reset.md:24–27`: "On v4 the board reset and a mode change add the
  rest of the board state: `AUX1` and `AUX2` become inputs, the AUX target
  returns to `AUX` (`A0`), the bit order returns to MSB-first, and the mode
  configuration is cleared."
- `park-and-reset.md:53`: the table is only "| | `w` | `#` on v4 |".
- `park-and-reset.md:57`: "| `AUX1` / `AUX2` | **not changed** | input |"
- `park-and-reset.md:67`: "The three "unchanged" rows in the `w` column …"
- `park-and-reset.md:65`: "| user macros | kept | kept on v4; cleared by the
  v3 reboot |"

**Problem:** The page is the "what survives" authority, but the table only
compares `w` and `#`. A mode change is described in prose but not in the
table, and the page never says that the display format survives a mode change
(generic/values.md says it does). The table has four "not changed/unchanged"
rows if you count `AUX1`/`AUX2`, so "three" reads wrong. The v3 macro note
sits in the `#` on v4 column.

**Fix:** Make the table three-way: `w` | mode change | `#`. Rows: five signal
pins, `AUX1`/`AUX2`, pull-ups, latches, width, bit order, AUX target, display,
prompt, user macros. Say explicitly: a mode change resets the pins, pull-ups,
latches, width, AUX target and bit order but keeps the display format and
macros; `#` restores HEX; `w` leaves bit order, AUX target and display but
resets width. Move the v3 macro note to a footnote and
name the three `w`-unchanged settings instead of counting rows.

### 10. [minor] "levels stay low" is the wrong noun

- `edge-cases.md:174–176`: "Symptom: the rails come up, but the pins stay
  inputs and levels stay low."

**Problem:** After `w`, the **latches** are low; the pins are inputs and float,
so `v` can show `H` or `L`. This contradicts the park page's "The `H` / `L`
row is not a park indicator."

**Fix:** "the pins stay inputs and the latches stay low (the pin levels
float)."

### 11. [minor] `{` is described with and without the syntax error

- `read-back.md:107–108`: "The `{` wrapper reports `ERROR: command has no
  effect here` and does not enable read-back."
- `edge-cases.md:199–201`: "Symptom: `ERROR: command has no effect here` and a
  syntax error; the write still prints `WRITE:` only."

**Problem:** One page says one error line; the other says two. A user who sees
the syntax-error line may think a second command failed.

**Fix:** Use the same two-line description in both places, or say "the
`ERROR:` line is followed by the dispatcher's `Syntax error …` line; nothing
is enabled."

### 12. [minor] The `v` capture lacks the real-output tab caveat

- `read-back.md:58–64`: the `v` capture is presented as fixed columns.
- `generic/usage.md:282–283`: "The device emits tabs between columns, so the
  exact spacing depends on your terminal's tab stops; the table above is
  transcribed with fixed columns."

**Problem:** The real `v` output is tab-separated. The DIO capture is aligned
in the doc but not in every terminal, so a user may think their spacing is
wrong. The generic page gives the caveat; the DIO page does not.

**Fix:** Add the same one-line caveat above the capture, and say "`Pinstates:`
is the title; the first row of the real output is the colour legend, omitted
here."

### 13. [minor] The README's "pins in a value" list is in `v` order, not bit order

- `README.md:57`: "| pins in a value | `AUX` `CS` `MISO` `CLK` `MOSI` | same |"

**Problem:** This is the `v`-table order. The value bit order is `AUX`,
`MOSI`, `CLK`, `MISO`, `CS` (bits 4–0). A reader comparing this row with
`pin-control.md` sees a different sequence and may think one is wrong.

**Fix:** Label the row "pins a value can address" and use commas or a set, or
list the bit order explicitly: `AUX` (4), `MOSI` (3), `CLK` (2), `MISO` (1),
`CS` (0).

### 14. [minor] Three paragraphs end with a single orphan word

- `README.md:10–11`: "… or driving a pin that no bus mode / exposes."
- `entry-and-setup.md:94–95`: "… `W` only turns the rails / on."
- `park-and-reset.md:109–110`: "… it stays driven through / `w`."

**Problem:** The single-word last lines look like rendering accidents and make
the raw Markdown harder to read.

**Fix:** Rewrap the paragraphs so the last line has at least a few words; no
content change.

### 15. [minor] `edge-cases.md` is a 224-line chain of four-bullet blocks

- `edge-cases.md:10–16` is one example; the same Trigger / Symptom / Behaviour
  / Recovery block repeats about twenty times.

**Problem:** The entries are well written, but scanning for a symptom is slow;
the generic edge-cases page uses a compact table.

**Fix:** Add a top "symptom index" table (Symptom | Command | Section), and
convert the simple entries to a four-column table. Keep the headings for the
five deep cases (LSB-first, width, parking, wrappers, contention) so links and
anchors still work.

### 16. [minor] The v3 caveat is repeated in eight files

- `README.md:63`, `entry-and-setup.md:149`, `wiring.md:142`,
  `pin-control.md:191`, `aux-pin.md:133`, `park-and-reset.md:143`,
  `macros.md:80`: "The v3 column is not verified on v3 hardware."
- `read-back.md:123–124`: "… it is not verified on v3 hardware."

**Problem:** The caveat is legitimate and correctly short, but after the first
two occurrences it is boilerplate. It also appears at the bottom of pages
where the v3 column is not the subject.

**Fix:** Keep it in `README.md` and `entry-and-setup.md`; on topic pages
either omit it or make it specific ("the v3 `v` order is in usage.md; not
verified").

### 17. [minor] The README does not say how to check whether my build has DIO

- `README.md:31`: "Enter with `m10` on v4 (`m9` on v3), or `m` then the
  number."
- `README.md:52–54`: "build | included in the default image … | build option |
  on by default; a build can leave the mode out … | mode number (default
  image) | 9 | 10"

**Problem:** The README says a build can leave DIO out and that the numbers
are for the default image, but the quick start gives absolute numbers and no
check. A custom build can omit or renumber DIO.

**Fix:** Add: "On a custom build, run `i` and look for `DIO` in the bracketed
mode list, or run `m` and use the number the menu prints; the numbers above
are the default image." Put it in the five-minute version, not only in the
detail page.

### 18. [minor] `w` resets the width, but the generic width page says only a mode change/reset does

- `park-and-reset.md:69–70`: "The width is the exception: `w` does reset it to
  8 bits."
- `generic/values.md:96–98`: "The width is **not** per line: it stays in force
  for later values and reads until you change it, or until a mode change or
  reset restores 8 bits."

**Problem:** `w` is neither a mode change nor a reset in the generic page's
sense, so a reader gets two rules. The DIO page is right; the generic page is
incomplete.

**Fix:** In `generic/values.md`, add "or when the mode's cleanup runs (DIO's
`w` does this)" so the two pages agree.

## What to keep

- The README front door: the file table, the five-minute version and the names
  table. The names table is also a quick pin map — keep that.
- `pin-control.md` §A value is a write and the two bit maps. The bit-7 split
  is the clearest explanation of DIO in the set.
- The state-value examples `0x80`, `0x84`, `0xC0`, `0xC4`, `0xE0`, `0xE4`,
  `0xFF` — I could see the absolute rule without running anything.
- `pin-control.md` §Direction and level are separate, especially "state first,
  direction second" and the explanation of the stale latch after `w`.
- `read-back.md`: `r` is levels only; `v` is the full picture; an output reads
  its own level and a floating input is not a fault. That answered my first
  read-back question.
- `aux-pin.md`: the warning that every reply says `AUX` and the confirmation
  line is the real target. The warning itself is excellent; it just needs the
  `i` check.
- `wiring.md` §Pull-up: "the warning is informational", "`(HiZ)` is the
  open-drain high state, not the mode", and the external-only VPU note. This
  is exactly the kind of thing a human gets wrong.
- `entry-and-setup.md`: the `o` "Set output type" misnomer explanation and the
  "no settings menu" table.
- `park-and-reset.md`: the cleanup-order rationale (release, then remove bias,
  then clear latches) and the `w` vs `#` table — the table is the right idea
  even though the surrounding prose contradicts it.
- `edge-cases.md`: the LSB-first + `:N` alternation, the `;4` hidden action
  bit, the `0xE0` pull-up side effect, and the `w` AUX1/AUX2 trap. These are
  real traps, not filler.
- The consistent use of exact firmware strings and the v3/v4 delta columns.
  I trusted the command/reply examples.

## Structural opinion

The nine-file layout broadly works, but this function is small enough that the
split creates more navigation than it saves. Three concrete changes:

1. **Add a task-path table at the top of the README.** Five rows: "wire and
   drive one pin", "use AUX1/AUX2", "read back", "park safely", "recover".
   Each row should link the exact section, not just the file. That would have
   saved most of my doubling back.

2. **Consolidate the lifecycle.** `entry-and-setup.md` and
   `park-and-reset.md` both own `W`/`w`, and that split produced the
   "no setup"/"silent setup" contradiction and the safe-park contradiction.
   Either merge them into one `lifecycle.md`, or move every `W`/`w`/park
   statement to `park-and-reset.md` and leave entry with a link. Use a
   three-column survival table (`w` | mode change | `#`) in that one place.

3. **Add one pin cross-reference table per board.** Put it in `wiring.md`:
   header position | `v`-table column | value bit | AUX token | direction /
   level meaning. Right now the header order, the `v` order and the bit order
   are three different sequences in three files; a v3 user has to assemble
   them mentally.

A fourth, smaller change: give `edge-cases.md` a symptom index and table-ize
the simple entries. The current headings are good for deep links; a table is
better for "my pin does X, what is it?"

I would not collapse the whole set into one page. `pin-control`, `aux-pin`,
`read-back`, `wiring`, `macros` and `edge-cases` each own a distinct job. The
lifecycle split is the one that actively hurts.

## Unknowns

- Whether the v3 hardware matches any of the v3 column; the docs say it is
  unverified, and I have no v3 to check.
- Whether `P` always prints the open-drain warning in DIO on both boards; the
  text says yes, but I did not run it.
- Whether the `v` capture's spacing matches a real terminal's tab stops; the
  generic page says the device emits tabs, the DIO page does not, and I did
  not run it.
- Whether `AUX0` appears on any screen. I only saw it in two prose lines, not
  in a capture; it may be an internal name that leaked.
- Whether `i`'s status block shows the AUX target in DIO. `generic/navigation.md`
  says it does, but the DIO set has no `i` example.
- Whether the "about 5 µs" settle time is meaningful on both boards. The doc
  gives a number and then says it is not a timing guarantee; I cannot judge
  the hardware.
- Whether a custom build renumbers DIO exactly as the text says. The menu is
  dynamic; I can only trust the statement.
- Whether `#` on v3 clears the user macros and restores the display/bit order
  exactly as the table says. The v3 column is unverified.
