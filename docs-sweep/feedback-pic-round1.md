# PIC (ICSP) docs — fresh-reader feedback, round 1

**Verdict: Yes with fixes.** The set explains the bit-engine idea and the
entry/read path well, but I would not ship it as-is: the config-word opcode
table contains wrong classic-6-bit rows, and the five-minute start omits the
MCLR/VPP step that makes `(1)` answer. The adapter and `picprog` boundaries
also need one more pass.

## Task log

| # | Task | Where I looked | Speed | Success | What was unclear |
|---|---|---|---|---|---|
| 1 | First impression | README lines 1–50, availability table | Fast for "what is it" (two paragraphs); slow for "where do I start" (line 52) | Yes for what/not and "is it in my build"; no for the first action | The status line reads as v4-only; the naming table comes before the start; the start omits VPP |
| 2 | Enter and set up | `entry-and-setup.md`, the trap in `edge-cases.md`, generic navigation | Fast | Yes; the two prompts, defaults, width/family table and one-line form are followable | The one-line input uses `1`/`2` but the header prints `6`/`4`; `m 9 2` prints a header before the syntax error, so it looks half-successful |
| 3 | Wire a target | `wiring.md`, README quick start, AVRISP wiring for v4 pin numbers, generic `e` page | Medium; one context switch and one contradiction | Signal wiring and the pull-up rule: yes. HVPP adapter: no, not enough to wire safely | `W` vs `e` order; whether the adapter's `VPP/MCLR` row means CS carries VPP; no adapter part/link/schematic; v3 pin numbers are not in the set |
| 4 | Do a session | `bus-operations.md`, `macros.md`, `config-words.md`, `sessions.md` | Fast for framing and reads; medium for config words | Write/read framing and the no-answer signature: yes. Config-word table: no | `l` vs `L`; display width vs transfer width; the `0x0A`/`0x0C`/`0x0E` rows; whether `(1)` alone enters programming mode |
| 5 | Binary / `picprog` boundary | `binary.md`, `picprog.md`, README | Medium; the command set is detailed, the tool boundary is a dead end | `PIC1` and `0xAx` command sets: yes. Which interface `picprog` uses: no | The tool's route is explicitly undocumented; no download/source link; the `0xA5` row crams two protocols together |
| 6 | Rendering check | All PIC files, plus SPI/generic/AVRISP pages; scanned tabs and long lines | Medium | No TAB characters; captures are space-aligned; prose wraps; most names are mapped | The `v` row table is confusing; edge-case tables are 150+ chars wide; the v3 caveat is repeated in nearly every file; `picprog` has result prose and a "trimmed example"; `Programer` spelling |
| 7 | README navigation | README, file table, related docs | Fast to the map (line 38); medium to "start" (line 52) | One click to each page; all internal links resolve | The first screen has no start; the map sits below a 9-row naming table; dead ends at the AVRISP header page, the adapter and `picprog` |

## Findings

### 1. [severity: blocker] The config-word opcode table is wrong for the set it claims to show

`docs/programmers/pic/config-words.md:40-42`

```text
| Load Address | `0x0A` | 14-bit address |
| Read Address | `0x0C` | — |
| End Programming | `0x0E` | — |
```

The page introduces this as **"the classic 6-bit set"** (`config-words.md:30`),
but on the common PIC16F/PIC12F 6-bit map `0x0A` is **End (Externally Timed)
Programming**, not "Load Address"; `0x0C` is not "Read Address"; and `0x0E` is
not "End Programming". The worked example only uses `0x00`, `0x02`, `0x08` and
`0x04`, so the obvious path is fine — but a host author who uses this table for
a full sequence will send the wrong command at the worst possible moment
(ending or mis-sequencing a programming cycle).

The same section also calls `0x18` a "program-only cycle"
(`config-words.md:44`). On the common map `0x18` is **Begin Externally Timed
Programming**, not a separate "program-only" cycle.

**Fix:** replace this table with a verified 6-bit map from the exact part's
programming specification, or delete every row not used by the worked example
until it is verified. If the table is intentionally family-specific, name the
family in the heading; do not label it "the classic set."

### 2. [severity: major] The README's one-line start skips the step that makes `(1)` work

`docs/programmers/pic/README.md:52-53`

```text
**Start here:** wire first with the power off, then enter, power, pull up,
then run `(1)`.
```

`macros.md:40-42` is explicit that `(1)` **does not drive MCLR**:

```text
It does not drive MCLR: apply VPP for an HVPP part or hold MCLR at VDD for an
LVPP part before running it.
```

A reader who follows the README literally on an HVPP part wires PGC/PGD/GND,
powers up, pulls up, runs `(1)`, and gets the all-ones no-answer signature —
exactly the signature the same README calls "no target." The quick start is
therefore wrong for many PIC10/12/16/18 parts.

**Fix:** add a programming-mode step before `(1)`: "apply VPP (HVPP) or tie
MCLR to VDD (LVPP), then run `(1)`," with a link to the LVPP/HVPP section.
Alternatively, change the sentence to "enter programming mode, then run
`(1)`" and make the link mandatory.

### 3. [severity: major] The `W` / `e` / `P` bring-up order contradicts the generic and SPI pages

`docs/programmers/pic/wiring.md:62-67`

```text
| 1 | `W` | `POWER SUPPLIES ON`; the `3.3V`/`5.0V` rails come up |
| 2 | `e2` / `e3` (v4) | `3.3V` / `5V on-board pullup voltage enabled` |
| 3 | `P` | `Pull-up resistors ON` (warns if VPU is dead) |
```

`README.md:62-64` says the same: "`W` if ... the on-board pull-up feed is used;
then `e2` ... then `P`." But `docs/generic/commands.md:37-41` says the opposite:

```text
Enter a non-HiZ open-drain mode ... then issue `e2`/`e3`, switch the on-board
rails on with `W`, then `P`.
```

`docs/protocol/spi/wiring.md:100-107` also puts `e` before `W` and then `P`.
A reader cannot tell whether `e` must precede `W`, whether both orders work, or
whether one page is stale. The order can matter when an external Vpullup feed
is already present, because `e` refuses in that state.

**Fix:** state one canonical order for PIC and align the generic/SPI pages. If
both orders are accepted, say so explicitly ("select the source before or after
`W`; `P` last") and remove the numbered table's implication that only one order
works.

### 4. [severity: major] The adapter table reads as if VPP is wired to CS, contradicting the safety rule

`docs/programmers/pic/wiring.md:22-26`

```text
| CS | out | **VPP/MCLR** | only if you wire a VPP switch; the terminal bus operations never drive it (`PIC1` `0x18`–`0x1F` can) |
```

`docs/programmers/pic/wiring.md:105-107`

```text
| VPP/MCLR | CS | switched VPP to MCLR | `C` selects CS, then `A`/`a` | `0x18`–`0x1F` bit 0 |
```

The literal reading is: the adapter's VPP/MCLR line goes to the Bus Pirate's
CS pin. That directly conflicts with `wiring.md:90-91`: "**No Bus Pirate pin is
VPP-tolerant**; VPP must never reach a header pin." If CS is only the adapter's
switch-control input, the table must say that; if the table is meant to show
the VPP output node, the row is dangerous.

**Fix:** split the table into **control inputs** (CS, MISO, AUX, VPU) and
**programming outputs** (adapter VPP → target MCLR, PGC → target PGC, PGD →
target PGD, VDD/VSS). Add one sentence above it: "CS drives the adapter's
switch control; the switched VPP output goes only to the target's MCLR and
never to a Bus Pirate pin." A small block diagram would be better than the
five-column table.

### 5. [severity: major] The `picprog` ownership boundary is clear, but the operational boundary is a dead end

`docs/programmers/pic/picprog.md:26-27`

```text
`picprog` is a separate tool; its exact interface is the tool's own and is
not documented in this repository.
```

`docs/programmers/pic/picprog.md:110-111`

```text
- The exact interface `picprog` drives is not documented in this repository
  (**unverified**).
```

A user is told the firmware is a bit engine and `picprog` is the programmer,
which is the right conceptual split. But the "Other host routes" table lists
terminal PIC, `PIC1` and raw wire and never maps `picprog` to one of them. So I
cannot tell whether `picprog` needs the terminal PIC mode, `PIC1` (`0x07`), or
the raw-wire `0xAx` group; I cannot tell whether a build without PIC support
will work; and I cannot debug a failed first connection.

**Fix:** add a "Route" row/sentence for `picprog` (or say "unknown; the tool is
external") and a three-step way to identify it: enter binary, watch for `PIC1`
vs `RAW1`, and compare the first bytes. Add the source/download link next to
the tool name. Also revisit `picprog.md:96` ("A Bus Pirate with PIC mode in the
build"): if the tool uses raw wire, the PIC terminal mode is not required.

### 6. [severity: major] `picprog` carries tested/verified-result prose in a user page

`docs/programmers/pic/picprog.md:42-52`

```text
| 18Fxx5x | 18F2550 | high voltage (13 V) | ID/erase only; write/read not complete |
| 18F2/4xJxx | 18F24J50 | low voltage | ID/erase/write/read documented |
| 24FJXXGAXXX | 24FJ64GA002 | low voltage | read valid; verify can fail on the HEX format |

The 18F2550 needs a high-voltage adapter for its header, or an external VPP
supply.  The 24FJ verify failure is the tool's HEX parser, which does not
understand the PIC24F firmware format.
```

"ID/erase only; write/read not complete", "read valid; verify can fail" and the
parser diagnosis are exactly the pass/fail-result prose the template says to
keep out of user docs. There is no date or bench rig, but these are still
tested/verified claims about a tool's behaviour, presented as facts.

**Fix:** keep only a short "the tool's part table is the authority; this set
does not re-verify it" note, or move this table to `docs/dev/`. If the support
summary is essential, label it "the tool's published claims, not verified here"
and remove the specific failure diagnosis.

### 7. [severity: major] The 135-line README is not just long; it starts in the wrong place

`docs/programmers/pic/README.md:16-18`

```text
**Names used in this set.**  The firmware's own strings are the authority.
Two forms exist for some items; the mapping is stated here once and used
consistently below.
```

`docs/programmers/pic/README.md:38-39`

```text
| File | Covers |
|---|---|
```

`docs/programmers/pic/README.md:50`

```text
## The five-minute version
```

Yes, 135 lines is slightly over the template's ~120-line target, but the count
is not the main problem. The first screen is status plus a 9-row naming table;
the file map starts at line 38 and the first "Start here" is at line 52. A user
who unboxes the docs and asks "what is it / is it in my build / where do I
start?" has to scroll past reference material before reaching the answer.

**Fix:** put a two-line "Start here" immediately after the intro, then the file
map, then availability. Move the naming table to `entry-and-setup.md` or a
separate `naming.md` (or to the bottom of the README). Cut the duplicated
"Wiring headline" and compress the limits bullets into a table. Target
~95–105 lines, not 135.

### 8. [severity: major] The same `L` command is described as LSB-first in generic and MSB-first in PIC

`docs/programmers/pic/bus-operations.md:38-42`

```text
- The mode shifts **LSB-first**.  The generic bit-order setting is applied
  *before* the mode sees a write: the default `l` (MSB-first, no reversal)
  leaves the mode's native LSB-first order intact.  Do not select `L`; it
  reverses the value within the display width, so the wire order becomes
  MSB-first.
```

`docs/generic/values.md:178-186`

```text
| `L` | `LSB set: LEAST sig bit first` | least-significant bit first |
...
With LSB-first the byte on the bus is the bit reverse of the number shown for a
write, and a read is reversed before it is shown.
```

Both statements can be true at different layers — `L` is the generic reversal,
and PIC's native shifter is LSB-first, so the reversed value goes out MSB-first
— but the page never reconciles the two vocabularies. A human who checks the
generic page after reading PIC will conclude one of them is wrong, and bit order
is a classic silent-failure area.

**Fix:** in both pages, distinguish "generic reversal setting" from "wire order
in this mode." Add one sentence to `generic/values.md`: "In a mode with a native
LSB-first shifter (PIC), selecting `L` reverses the value, so the wire order
becomes MSB-first; the label describes the generic transformation, not the wire."
In PIC, say "leave the generic setting at `l` (no reversal)" rather than only
"do not select `L`."

### 9. [severity: minor] The PIC wiring page outsources physical pin numbers to an AVR programmer page

`docs/programmers/pic/wiring.md:37-40`

```text
- The physical header order is board-level: the v4 header numbering is in the
  table at the top of [the AVRISP-MKII wiring page](../avrisp/wiring.md); the
  v3 header labels the same signals.  This page names the signals, not the
  connector positions.
```

A PIC user should not have to open the AVRISP-MKII set to find the v4 header
order, and the v3 reader gets no pin numbers at all — only "the header labels
the same signals." The signal table is good; the physical step is the weak
link.

**Fix:** include the 12-pin v4 header table inline (it is short) and a v3 pin
note, or link to a board-level header page rather than another programmer's
set. If the page deliberately owns only signal names, say "see the board
silkscreen" for physical positions.

### 10. [severity: minor] "Assigning it to AUX" is ambiguous

`docs/programmers/pic/README.md:116-118`

```text
- **MCLR is not driven by the terminal bus operations.**  The terminal can
  drive `CS` after assigning it to AUX, and `PIC1` has `0x18`–`0x1F`; the
  clock/data operations never touch MCLR.
```

The phrase can be read two ways because AUX is both a pin and the command
family. It may mean "select CS as the target of `a`/`A` with `C`" (correct), but
it also reads as "assign CS to the AUX pin" (wrong). A reader can end up at `c`
and the AUX pin instead of `C` and the CS pin — which matters directly for a VPP
switch.

**Fix:** "after selecting the CS pin for `a`/`A` with `C` (the command family is
called AUX; the pin is CS)."

### 11. [severity: minor] The `v` row table reads as a six-column row

`docs/programmers/pic/wiring.md:128-131`

```text
| Board | Signal row (last four columns) | PGC | PGD |
|---|---|---|---|
| v4 | `-` `-` `PGC` `PGD` | CLK column | MOSI column |
| v3 | `PGC` `PGD` `-` `-` | CLK column | MOSI column |
```

The `PGC` and `PGD` columns are annotations, not part of the signal row, so the
table looks like a six-column row and the reader has to reconstruct which two
of the four are live. This is the one table in the set I had to read twice.

**Fix:** show the full row as a code block per board, with the PGC/PGD columns
marked underneath; or use a three-column table: board, full signal row, "PGC/PGD
live columns."

### 12. [severity: minor] The `0xA5` row is unreadable because it carries two protocols

`docs/programmers/pic/binary.md:116-118`

```text
| `0xA4` | write records | count, then 3 bytes per record (416) or 4 bytes (424) | `0x01` / `0x00` |
| `0xA5` | write and read bits | 416: count + command byte; 424: count only | 416: one byte per count; 424: six bytes per count; **no result byte** |
| `0xA7` | write and read script | write count, read count, then the script | `0x01`, plus any read bytes |
```

The `0xA5` cell packs two different host formats and two different return
shapes into one row, then adds "**no result byte**" at the end. A reader can
easily apply the 416 shape to 424 or vice versa.

**Fix:** split `0xA5` into two rows (416 and 424) or a small sub-table:
engine / host sends / device returns. Do the same for `0xA4` if the 3-byte vs
4-byte record split causes the same problem.

### 13. [severity: minor] The v3 caveat is repeated in nearly every file

`docs/programmers/pic/README.md:91-92`

```text
The mode number is the **default-image** position.  A custom build with a
different enabled-mode set shifts it; the menu is dynamic.  The v3 path is
**not verified on hardware**.
```

The same closing sentence appears at `entry-and-setup.md:115-116`,
`wiring.md:148-149`, `bus-operations.md:156-157`, `macros.md:83-84`,
`config-words.md:127-128`, `binary.md:160-161`, `picprog.md:114` and
`edge-cases.md:109-111`. That is nine occurrences of one status caveat, on top
of the separate README status line. It reads like an audit disclaimer and adds
noise to every page. A short caveat is allowed; nine is not useful.

**Fix:** one status line in the README and one v3 column in each availability
table. Delete the per-page closers; keep a single "not verified on v3" note
where it changes a procedure (e.g., the wiring page).

### 14. [severity: minor] Three unresolved naming forms

`docs/programmers/pic/picprog.md:31-38`

```text
`picprog` is the executable of the **PiratePICprog** project...
The banner prints `(Bus) Pirate PIC Programer v0.1`.
```

The README naming table maps `picprog` / `PiratePICprog` to `picprog`
(`README.md:30`), but the banner form "Pirate PIC Programer" (one `m`) is a
third form and is not in the table. If that is the real tool string, say so; if
it is a typo, fix it. The same table also calls `e1` an "on-board pull-up feed"
(`README.md:87`), but `e1` is External/None; and `wiring.md:145` uses the
undefined "v3a+":

```text
| on-board pull-up value | documented 2 kΩ (v3a+) | shared with other modes; see the I2C page |
```

**Fix:** add the banner spelling to the naming table (or correct it), relabel
the `e` row as "VPU source (`e`)" with `e1` external/none, and define "v3a and
later" or drop the board-revision shorthand.

### 15. [severity: minor] The quick start and limits are bullet chains that want to be tables

`docs/programmers/pic/README.md:55-76` has seven bullets in "The five-minute
version" (the template's skeleton uses six), and `README.md:105-122` has eight
bullets under "Limits and out of scope," several of them three lines long. For
example:

```text
- **No PIC algorithm in the terminal mode.**  No erase, program, verify,
  configuration-word or part-database logic is in the terminal mode.  The
  only built-in sequence is `(1)`, and it is 6b/14b only.
```

These are option/limit chains, not narrative; a table with "Limit | What it
means | Where" would scan faster and would cut the README length.

**Fix:** six-row quick-start table (wire, enter, program-mode, power/pull-up,
command/data, read/ID) and a six-row limits table. Keep prose only for the two
or three risks that need a sentence (no VPP, no algorithm, lock-out).

### 16. [severity: minor] `picprog` has a "trimmed example" and a duplicate worked sequence elsewhere

`docs/programmers/pic/picprog.md:77-79`

```text
The tool prints its own progress; a trimmed example:
```

"Trimmed" is capture provenance; the template says examples should simply be
examples. Separately, `config-words.md:48-93` and `sessions.md:40-77` both walk
the same `0x00` → `0x02` → payload → `0x08` → `0x04` → `r` sequence with
different values. The reader has to decide which one is canonical.

**Fix:** replace "a trimmed example" with "an example of the tool's progress
output." Keep one canonical write/read session (probably `sessions.md`) and have
`config-words.md` link to it, or make `config-words.md` the only worked example
and remove the duplicate.

### 17. [severity: minor] The 1.6 ms figure needs the framing clocks

`docs/programmers/pic/bus-operations.md:125-127`

```text
There is no speed menu.  The mode uses a fixed 100 µs pin delay: about 100 µs
per bit (roughly 10 kHz).  A 14-bit payload is about 1.6 ms of clocking, plus
the command delay.
```

14 bits at 100 µs is 1.4 ms; 1.6 ms is right only because the 6b/14b payload is
framed with a leading and trailing zero (16 clocks). The page says that earlier,
but the arithmetic in the timing section looks wrong until the reader remembers
the framing.

**Fix:** "A 14-bit payload is 16 clocks including the framing zeros, about
1.6 ms, plus the command delay."

## What to keep

- **The opening orientation.** "PIC mode is the Bus Pirate's **ICSP** bit
  engine ... The Bus Pirate is the bit engine; it is not a stand-alone
  programmer" (`README.md:6-14`) is the single best paragraph in the set. It
  tells me what I am holding and what I am not.
- **The naming table's intent.** Once found, the mapping of `PGC` ↔ `CLK`,
  `PGD` ↔ `MOSI`, `VPU` ↔ `Vpullup` ↔ `Vpu`, and `PIC` ↔ `PIC1` stopped me
  guessing. The table is misplaced, not wrong.
- **The width/family table and the `(1)` column.** `entry-and-setup.md:52-61`
  made the 6b/14b vs 4b/16b choice concrete, including the fact that `(1)` is
  unavailable on 4b/16b.
- **The missing-delay trap.** The exact capture
  `PIC(mod dly)=(4 0)` + `Syntax error at char 6` in `edge-cases.md:60-71` is
  the kind of trap docs usually miss; I would have hit it.
- **The pull-up explanation.** "a high is a released line and only the pull-up
  makes it high" (`README.md:96-98`) made open drain click. Keep this sentence
  verbatim.
- **The no-answer table.** All ones vs all zero, with the plain-read equivalents
  (`macros.md:66-79`), let me diagnose a missing target in one step. That is the
  most useful diagnostic in the set.
- **The `(1)` page.** The sequence, the 4b/16b limitation, the "does not drive
  MCLR" caveat and the "Please exit PIC programming mode" warning
  (`macros.md:16-47`) are together and unambiguous.
- **The `PIC1` command table and worked exchange.** `binary.md:35-98` is dense
  but directly implementable; the `0x00` exit vs `0x0F` terminal reset
  distinction is exactly the trap a host author needs.
- **The "Leaving" session.** `w` runs cleanup but does not leave the mode; a
  value typed after `w` still clocks out; `m1` is the way out
  (`sessions.md:122-138`). Real gotcha, well placed.
- **The cross-links.** The file map is accurate and every internal link I
  followed resolves. The set is easy to navigate once you are inside it.

## Structural opinion

The file-per-topic split is right for lookup, but the set is over-fragmented for
a first read. Concrete changes:

1. **Make the README a 95–105-line front door.** Put "what it is / what it is
   not / is it in my build / start here" in the first 20 lines. Move the naming
   table to `entry-and-setup.md` (or a one-page `naming.md`). Put the file map
   immediately after the intro. Cut the duplicate "Wiring headline" and turn
   the limits bullets into a six-row table. The current 135 lines are not
   catastrophic, but the first screen is reference material, not a start.

2. **Split `wiring.md`.** It currently owns signals, pull-ups, bring-up order,
   voltage limits, LVPP/HVPP, the adapter, the `v` row and the v3/v4 delta. The
   adapter is the one place a wiring mistake can destroy a part; give it its
   own `hvpp-adapter.md` with a block diagram (control inputs on one side,
   target pins on the other), a clear "never connect VPP to a Bus Pirate pin"
   rule, and a link to the adapter's source/schematic. Leave `wiring.md` for
   PGC/PGD/VDD/VSS/VPU and the pull-up.

3. **Merge `macros.md` into `bus-operations.md`.** It is 84 lines for one macro
   (`(1)`). The macro is part of the bus operations story; a section called
   "Device ID" in `bus-operations.md` would be easier to find and would remove a
   file from the map.

4. **Keep one canonical write/read session.** `config-words.md` and
   `sessions.md` currently duplicate the same opcode sequence. Keep the session
   in `sessions.md`, and let `config-words.md` own the opcode table and the
   lock-out risk with a link to the session. That also makes the wrong-table
   finding easier to fix in one place.

5. **Split `binary.md` into `pic1.md` and `rawwire.md` (or add a top-of-page
   contents).** It does two jobs: the byte-wise `PIC1` loop and the three
   raw-wire engines. The `0xA4`/`0xA5`/`0xA7` script section is the hardest read
   in the set and is marked unverified; it should be a clearly labelled
   advanced page, not the second half of the binary page a user reaches from
   the README.

6. **Give `picprog` a next step.** It is a host-tool card with a good ownership
   paragraph, a command line and an output example, but it ends on limits and
   "unknown." Add a source/download link and a route sentence (or a short "how
   to tell which interface it uses" procedure). Without that, the binary /
   `picprog` boundary is conceptually clear but practically unusable.

7. **Keep `edge-cases.md` as the central index.** It is the right shape
   (trigger → symptom → recovery). Just stop duplicating the same rows in the
   other pages; link to the entry instead. The v3 caveat can live in one place.

## Unknowns

- **Adapter polarity and sequencing.** I could not judge the published
  adapter's active-high/active-low control polarity, or whether its
  `VPP/MCLR` row is a control input or the switched VPP output. The set says
  this is unverified; that is honest, but it means I cannot wire the adapter
  safely from these docs alone.
- **`picprog`'s actual interface.** I could not tell whether it uses terminal
  PIC, `PIC1` or raw wire, or whether the v3 path is supported in practice.
  The set explicitly leaves this undocumented.
- **The intended family for the 6-bit opcode table.** The page calls it
  "classic," but the rows do not match the common PIC16F/PIC12F map. If the
  author had a specific family in mind, that family is not named, so I cannot
  tell whether the table is wrong or merely mislabelled.
- **Whether `e` before `W` is functionally required.** The code path suggests
  the source can be selected either before or after the rails, but the docs
  disagree; without hardware I cannot confirm which sequence the firmware
  expects in every mode.
- **Whether `(1)` alone enters programming mode.** The docs say this is
  part-specific and unverified. I could not test it; a reader still needs the
  part's programming specification for the entry condition.
- **The raw-wire `0xA4`/`0xA5`/`0xA7` script semantics.** The set marks them
  unverified; I could not validate the record sizes or the success-byte
  ordering against hardware.
- **`picprog`'s published part support.** I could not verify the 18F2550,
  18F24J50 or 24FJ64GA002 result claims without the tool and the parts; that is
  one reason the result prose should not be in a user page.
- **v3 behaviour generally.** I have no v3 hardware; every v3 claim in this
  review is a docs-consistency check, not a hardware observation.
