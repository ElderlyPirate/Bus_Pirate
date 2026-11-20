# Core documentation — reader feedback, round 1

## Verdict

**Yes with fixes.**  The set gives a new maintainer a real architecture, real
contracts and unusually good failure lists; but three things would actively
mislead a reader (a wrong user-macro syntax, a wrong timer range in the linked
prompt page, and a "reset" heading that is not the board reset), and the two
largest navigation aids — the README module map and `system.md` — are harder to
use than the content deserves.

## Task log

| # | Task | Where I looked | How fast | Success? | What was unclear |
|---|---|---|---|---|---|
| 1 | First impression from README alone | `docs/core/README.md` only | One 133-line read | Mostly | Scope, boards and maintainer/user boundary are clear. No reading order: I guessed `system.md` because it is listed first. "Core" vs the `core_*` source groups is not explained until the tables. |
| 2 | Module map: dispatch / mode registration / board reset | README module map → `system.md`; then `base.c` via `dev/code-map.md` | (a) 2 hops + a 200-line scroll; (b) 2 hops; (c) 3 hops with one double-back | (a) yes, (b) yes, (c) eventually | `system.md` has no contents list, so "Command dispatcher" and "Mode registry" are found by scrolling. For "board reset" the obvious heading, `## Reset and exit paths`, is about `#`, `$`, `m1`, binary/SUMP returns — not `bp_reset_board_state()`. I doubled back to the mode-change lifecycle, then to the Scope table, then to `base.c`. |
| 3 | Follow the `system.md` lifecycle | Boot/main flow → terminal state → persistence → mode change → reset/exit → invariants | One full read; repeated scans for the reset order | Yes, with friction | Boot step 2 is a semicolon wall. The mode-change preamble ("share steps 1–3 and 5") reads awkwardly against the six steps. Safety rules are scattered across HiZ, the lifecycle, reset and invariants rather than collected. Failure modes are concrete and complete. |
| 4 | Two classic failure paths | USB CDC host-not-reading/lock; message catalog regeneration; sampled bitbang abort and EEPROM ACK/rollback | ~15 min for the pair | USB: partial; messages: partial; bitbang/EEPROM: yes | USB says what blocks but not how the user recovers, and says a producer "must discard" an invalidated span without saying how to detect invalidation. Messages gives generator contracts but not the concrete command, and "deterministic from any working directory" hides a cwd-sensitive output path. Bitbang never says who clears the sticky error in the shipped I2C mode. EEPROM's protected-write/readback and failed-rollback behaviour is clear. |
| 5 | Cross-set navigation, and back | Core → `generic/{README,navigation,commands,values,macros}`, `protocol/i2c/wiring`, `capabilities/selftest`, `tools/basic/entry-and-prompt`, `protocol/{binary,sump}`, `dev/{building,tooling,code-map}` | Mostly 1 click per target | Links land, facts do not always | Every link resolves. The landing pages are the right ones. But the linked `generic/commands.md` contradicts core on the timer range, `generic/macros.md` contradicts core on macro syntax, and the dev test pages point at a runner that is not in `Firmware/tests/`. No user page links back to core; that is defensible because user pages must stay code-free, but it leaves `dev/code-map.md` as the only return path and the landing pages do not mention it. |
| 6 | Rendering check | All six files: tabs, trailing whitespace, line lengths, table pipe counts, odd backticks, dates, bench/test words, naming pairs | Scripted scan + manual read of every table | Yes | No TABs, no trailing whitespace, no dates, no bench/rig/capture prose, no "tested/verified-result" claims; all prose is within ~80 columns and all tables have consistent pipe counts. Presentation defects are the README cell walls, one semantically wrong registry table, one split inline code span, the "pipe" label, and two names for the bitbang counter. All six files are LF-only with no final newline while neighbouring docs are CRLF; cosmetic, not a rendering failure. |
| 7 | README navigation test | README → module map (line 64) → cross-links (104/125) → failure modes | Module map and cross-links: seconds. Failure modes: dead end. | Partial | Module map and cross-links are reached in one scroll. There is no route to any failure-mode section: the word "failure" appears only in the eeprom row. "Native test coverage" is a section that points at other pages, not an index. A maintainer debugging a hang has to open each detail page and scroll to its last third. |

## Findings

### Major

1. **[major] `docs/core/system.md:252` — the user-macro syntax shown here is wrong.**
   Quote: ``| `<n>=text>` | define a user macro | `<0>` lists, `<n>` recalls; recall replays through the editor |``
   The actual grammar is `<n=body>`: the first `>` ends the body, as the user
   page states (`docs/generic/macros.md:25`, ``| `<n=body>` | store `body` in slot n; the first `>` ends the body |``).
   A maintainer copying the core table will type `<1>=abc>` and get a syntax
   error.  This is also the clearest evidence that duplicating user grammar in
   the dispatcher table drifts.
   **Fix:** change the key to `<n=text>` (no trailing `>`), state that the first
   `>` terminates the body, and link to `generic/macros.md` for the full rules.

2. **[major] `docs/core/system.md:233` vs `docs/generic/commands.md:97` — the `t` timer range is stated two ways.**
   Core: ``| `t` / `T` | `bp_timer_print()` / `bp_timer_reset()` | 31-bit microsecond timestamp |``
   and `system.md:116`: "returns a 31-bit microsecond count that wraps about every 35 minutes".
   Linked user page: "`t` reads a 32-bit microsecond count, so it wraps after about 71 minutes."
   The core page is right: `bp_timer_microseconds()` returns
   `((overflows << 16) | ticks) >> 1`, so the top bit is always zero.  A
   maintainer who trusts the linked page may "correct" core to 32-bit and
   introduce an off-by-2× timeout analysis.  The page is reachable from the
   core `Related docs` list.
   **Fix:** correct `generic/commands.md` to 31-bit / ~35.8 minutes; in core,
   add the one-line reason (the right shift discards the top bit) so the next
   reader does not have to re-derive it.

3. **[major] `docs/core/system.md:479` — `## Reset and exit paths` is not where the board reset lives.**
   Quote: "`reset_terminal_state()` is the shared soft-reset helper, used by the v4 `#` path, the SUMP exit and the v4 binary return."
   The section covers terminal resets and exit commands.  The actual
   `bp_reset_board_state()` sequence is buried in the mode-change lifecycle at
   `system.md:394` ("`bp_reset_board_state()`: bus pins and AUX released ..."),
   and the EEPROM and SMPS parts are only linked from `eeprom.md` and
   `smps`.  A reader asked to "find where the board reset lives" follows the
   obvious heading and lands in the wrong section; I doubled back to the
   lifecycle, then to the Scope table.
   **Fix:** retitle the section "Terminal reset and exit paths"; add a
   `### Board-state reset (bp_reset_board_state())` subsection with the ordered
   sequence, its callers (mode change, v4 `#`, SUMP/binary return,
   `bp_change_mode`), and links to `eeprom.md` and `smps`.  Keep the exit table
   below it.

4. **[major] `docs/core/system.md:1–13` — 617 lines and no contents list.**
   Quote: "**Status:** current · maintainer-facing.  Covers startup, the terminal dispatcher, the mode registry, shared state, menus and the reset/exit paths."
   The first heading is at line 1 and the next navigation aid is the `Scope and
   files` table; a reader then meets `Command dispatcher` at 218, `Mode
   registry` at 301, `Adding a mode` at 420, `Reset and exit paths` at 479,
   `Failure modes` at 501 and `Invariants` at 542.  Task 2 and task 7 both
   required repeated long scrolls.  `system.md` is doing tutorial, reference and
   procedure in one file.
   **Fix:** add a short contents block with anchors at the top.  Better: split
   it into `system.md` (boot, serial, terminal state), `dispatch.md` (editor,
   dispatcher, parsing) and `modes.md` (registry, lifecycle, adding, menus), or
   at least move `Adding a mode` to its own page.

5. **[major] `docs/core/README.md:68–72` and `:108` — the maps are cell walls.**
   The module-map rows run up to 284 characters; the public-entry cell in the
   `core_system` and `core_usb` rows is the bulk of it, for example
   ``| `core_system/` | `main.c`, `core.c/h`, ... | `main`, `serviceuser`, ... | [system.md](system.md) |``.
   The layer table is worse: line 108 is a 477-character row with twelve links
   in a single cell, all under "`protocol_*`".  This is the primary lookup table
   of the set; depending on the renderer it wraps into a dense block or forces
   horizontal scrolling, and a reader cannot scan "where does `protocol_i2c`
   live?" without parsing a sentence.
   **Fix:** one row per source group for the module map, with the entry-point
   list moved into the detail page (it is already there); make "Where the other
   layers live" a one-row-per-set table or a bullet list.  This would also bring
   the 133-line README back toward its intended ~120 lines.

6. **[major] `docs/core/README.md:98–102` — the README has no route to the failure modes, and its test section is a dead end.**
   The README claims to be the map, but the word "failure" occurs only in the
   eeprom description; there is no index from "terminal hung", "mode will not
   engage", "I2C timeout", "string missing" to the owning page's
   `#failure-modes-and-edge-cases` section.  "Native test coverage" says "Each
   page names its standalone host suite; run it directly.  There is no
   aggregate runner", which is true but sends the reader back into the pages.
   A maintainer debugging a failure starts at the map and dead-ends.
   **Fix:** add a compact failure index (symptom → page + anchor) and either
   list the suites in a 5-row table or move the test paragraph to
   `dev/verification.md` and link it.

### Minor

7. **[minor] `docs/core/system.md:308–327` — the mode-registry table contradicts its own paragraph on LCD builds.**
   The table lists ``| 9 | `BP_PICPROG` | `PIC` | both |``, ``| 10 | `BP_DIO` | `DIO` | both |``, then a
   row ``| — (9 when built) | `BP_HD44780` | `LCD` | optional both; off in the default image |``.  The paragraph below says:
   "when LCD is compiled in it takes menu 9 and `PIC`/`DIO` move to 10/11."
   A reader scanning the table sees menu 9 twice and may configure the wrong
   mode; this is exactly the renumbering trap the "Adding a mode" section warns
   about.
   **Fix:** keep this table to the default v4 build only, and add a separate
   build matrix (rows: KEYB on/off × LCD on/off; columns: menu numbers for
   PIC/DIO/LCD).  Remove the annotation row.

8. **[minor] `docs/core/system.md:379–385` — the null-callback contract is ambiguous for a new mode author.**
   Quote: "Callbacks that should do nothing are assigned the null defaults in `core.c` ... Each null callback prints `ERROR: command has no effect here` and sets `command_error`; DIO's silent setup callback is the one exception.  `setup_prepare`/`setup_execute`/`cleanup` use `reset_mode_to_8_bits()` where no protocol work is needed".
   "Each null callback" and the setup exception are in the same breath; a reader
   cannot tell which descriptor fields are required, which are optional, and
   what an omitted field does.  `cleanup` is safety-relevant, so guessing here
   is expensive.
   **Fix:** add a field → required/optional → default if omitted → observable
   behaviour table; separate the `null_*` operation callbacks from the
   `reset_mode_to_8_bits` setup/cleanup default.

9. **[minor] `docs/core/usb.md:211–215` and `:193–198` — the CDC failure modes say what blocks but not how to recover or detect invalidation.**
   Quote: "**Host not reading.**  The IN path blocks for BD ownership instead of dropping data.  A producer in the SOF handler does not block; it leaves the data queued for the next flush."
   There is no symptom ("prompt stops after N bytes"), no recovery ("drain or
   reopen the host port; the firmware cannot drop"), and no detection for the
   earlier rule: "Bus reset/reconfiguration invalidates any span previously
   returned by `getinbuf_cdc()`/`getoutbuf_cdc()`; a producer must not retain or
   commit one across a reconnect."  A maintainer cannot implement "discard"
   without knowing what signals invalidation.
   **Fix:** add symptom/recovery and a detection sentence ("treat a drop below
   `CONFIGURED_STATE` or a new `SET_CONFIGURATION` as invalidation; re-fetch the
   span").  Also clarify the lock: `:178–179` says "nesting is safe but not a
   reference count" while `:244` says "the lock is a flag, not a counter"; state
   explicitly that callers must not call `setLock()` twice and `freeLock()` once.

10. **[minor] `docs/core/messages.md:136–137` — "deterministic from any working directory" hides the cwd-sensitive output path.**
    Quote: "Output is UTF-8 without BOM with LF line endings, and entries are written in identifier order, so generation is deterministic from any working directory."
    The *contents* are deterministic, but the output prefix is relative to the
    process cwd; `docs/dev/building.md:237–241` says "Run the generator **from
    `tools/packstrings/`** because the output prefixes above are relative to
    that directory."  A maintainer following the core page can write
    `messages_v3.s/h` into the repository root.  The concrete two commands are
    also only on the dev page.
    **Fix:** inline the two invocations and the cwd note in "Generation and
    catalog contracts"; reword to "contents are deterministic; the output prefix
    is relative to the cwd."

11. **[minor] `docs/core/bitbang.md:110–113` and `:180–182` — the sticky-error recovery does not say who clears it.**
    Quote: "the caller must clear it before a new operation."
    In the shipped I2C mode the terminal wrappers clear the error at the start
    of start/read/write, so a user recovers by reissuing the transaction; a raw
    caller must call `bitbang_i2c_clear_error()` explicitly.  The page never
    says this, so a maintainer can add a redundant clear or assume the next
    command auto-recovers.
    **Fix:** add one sentence: "The I2C terminal wrappers clear it at the start
    of each transaction; raw callers must call `bitbang_i2c_clear_error()`;
    `bitbang_setup()` does not clear it."

12. **[minor] `docs/core/bitbang.md:20` vs `:93` — the stretch counter is named two ways without a mapping.**
    Data model: ``| `bitbang_clk_stretch_timeouts` | saturating count ... |``
    API group: ``| diagnostics | `bitbang_clock_stretch_timeouts` | saturating count ... |``
    One is the internal static variable, the other is the accessor function.
    A reader cannot tell whether `bitbang_clk_stretch_timeouts` is callable.
    **Fix:** use `bitbang_clock_stretch_timeouts()` in both places and say once
    that it reads the internal saturating counter; drop the bare variable name
    from the API list.

13. **[minor] `docs/core/system.md:242` — the `|` key is written as "pipe".**
    Quote: ``| `=` / pipe | `convert_value(NO / YES) | ... |``
    Every other row uses the literal key; "pipe" makes the reader translate.
    The user page writes the escaped form (`docs/generic/values.md:203`,
    ``| `\|X` | bit-reverse X ... |``).
    **Fix:** write the key as `` `\|` `` (escaped in the table) or "`|` (pipe)".

14. **[minor] `docs/core/system.md:335–336` — an exact firmware message is split inside an inline code span.**
    Quote: "technology is `1-Wire`; the firmware's status messages print `1-WIRE speed:" / "standard/overdrive`.  Use `1WIRE` for the mode everywhere in this set."
    The message is one string; the line break lands in the middle of the code
    span.  Some renderers show the backticks literally, and a reader cannot copy
    the string as one unit.
    **Fix:** keep `1-WIRE speed: standard/overdrive` on one line, or put it in a
    fenced code block.

15. **[minor] `docs/core/system.md:97–99` — "flash configuration words" is the wrong term for the device ID/revision source.**
    Quote: "`bus_pirate_configuration.device_type` and `.device_revision` are read from the flash configuration words at boot."
    The values come from the device ID/revision words in the config-memory region
    (`0xFF0000`/`0xFF0002`), not the user `CFG0`..`CFG2` words that the status
    block prints.  A maintainer chasing a board-ID problem can inspect the wrong
    words.
    **Fix:** say "device ID/revision words in the config-memory region
    (`0xFF0000`/`0xFF0002`); the user `CFG0`..`CFG2` words are separate."

16. **[minor] `docs/core/README.md:100–101` vs `docs/dev/building.md:257` — the linked test instructions point at a missing runner.**
    Core: "Each page names its standalone host suite; run it directly.  There is no aggregate runner."
    Dev: "python Firmware/tests/test_native.py", and `docs/dev/tooling.md:54`
    lists `test_native.py` as a runner.  There is no
    `Firmware/tests/test_native.py` in the tree (I listed the directory; only
    the per-suite `test_*.py` files are present).  Core is right; the dev pages
    are the dead end.
    **Fix:** remove `test_native.py` from the dev pages or add the runner.  Add
    one line to core: "the only runner is the per-suite script named on each
    page."

17. **[minor] `docs/core/system.md:408–410` vs `docs/generic/navigation.md:202` — the direct `m<N>` form is described differently.**
    Core: "The direct `m<n>` form skips both announcements and prints only `Ready`."
    User page: "| `m<N>` (for example `m6`) | runs the same setup prompts and prints `Ready`; the `Clutch` line is printed only by the menu form |"
    Core is correct (source skips both `Clutch disengaged!!!` and the
    finish-setup prompt); the user page omits the second omission.  A reader
    arriving from core expects one fewer line than the firmware prints.
    **Fix:** add "and the finish-setup prompt" to the `m<N>` row in
    `generic/navigation.md`.

18. **[minor] `docs/core/system.md:501–540` — the failure modes are a 13-bullet chain that should be a table.**
    The section mixes trigger, symptom, state and recovery in bold-lead
    paragraphs ("**Power-up failure.** ...", "**Pull-up without source.** ...").
    The longest entries hide the recovery clause; cross-referencing "does `$`
    recover from X?" means reading all thirteen.  The same pattern appears in
    `usb.md`, `bitbang.md`, `eeprom.md` and `messages.md`.
    **Fix:** a four-column table per page (trigger → symptom → behaviour →
    recovery), keeping the bold lead as the trigger.

19. **[minor] `docs/core/messages.md:25–30` and `docs/core/eeprom.md:188` — drift-prone inventories are stated as facts.**
    "Current catalog size: v3 288 / v4 308" (the generated labels do currently
    count 288/308) and "512 self-test values/WP-entry states".  Every string or
    test edit must update prose that no build reads.  These are inventory, not
    an architectural contract.
    **Fix:** replace the counts with the relationship ("the v4 catalog is the
    shared set plus AUX/VPU/XSVF text") and keep exact counts in
    `dev/tooling.md`; if kept, label them "at the time of writing".

20. **[minor] `docs/core/README.md:3–16` — no "start here".**
    Quote: "This README is a map, not the manual.  The details live in the files below."
    The five files are listed as peers; a new maintainer has to guess whether
    `system.md` or the module page comes first.
    **Fix:** add one sentence: "New to the core: read `system.md` first, then the
    module you are changing; each page ends with its suite and related pages."

## What to keep

- The **boundary statement** in the first paragraph: "This set describes the
  firmware core ... It is not a user guide."  That single line told me what not
  to expect and where the user material lives.
- The **layering table** (`owns` / `depends on` / `used by`).  It is the fastest
  way to see why `core_bitbang` is reachable from PIC and why `core_usb` is not
  a protocol set.
- The **terminal-state and serial-abstraction tables** in `system.md`.  The v3
  row / v4 row format makes the transport differences obvious without prose.
- The **contract-first failure lists**: `bitbang.md`'s "true on success vs
  `bitbang_i2c_start` true on error" warning, the waiting-I2C open-drain and
  iteration-bound rules, `usb.md`'s BD-ownership/ZLP/lock invariants,
  `eeprom.md`'s protected-write/readback and failed-rollback behaviour, and
  `messages.md`'s "generated header and assembly are a pair" rule.  These are
  the parts I would hand to someone changing the code.
- The **naming mappings** for `1WIRE`/`1-Wire`/`1-WIRE speed:` and
  `Vpullup`/`VPU` are explicit and used consistently in the core pages.
- The **v3 caveats** are honest and short ("build-only; not verified on v3
  hardware") without turning into bench prose.
- The **rendering hygiene**: no tabs, no trailing whitespace, no dates, no
  bench/rig/capture narratives, no pass counts or "tested" claims.  The core
  set is clean here, which is rare.

## Structural opinion

The module-per-page layout matches the source tree and works for "I am changing
`core_usb`"; it does not match the cross-module tasks a maintainer actually
starts with.  Concrete proposals:

1. **Add a task index to the README**, above the module map: "add a mode" →
   `system.md#adding-a-mode`; "add a command key" →
   `system.md#command-dispatcher` + `core.c`; "change reset order" → the
   board-state reset subsection proposed in finding 3; "add a string" →
   `messages.md#generation-and-catalog-contracts`; "terminal hangs" →
   `usb.md#failure-modes-and-edge-cases`; "I2C clock timeout" →
   `bitbang.md#failure-modes-and-edge-cases`.  This is the missing link between
   the module map and a maintainer's first question.
2. **Split `system.md`** (617 lines) into boot/terminal, dispatch/parsing, and
   modes/reset/menus, or at minimum add a contents block and move "Adding a
   mode" to its own file.  The current file mixes tutorial, reference and
   procedure; a reader cannot tell which mode of reading is expected at each
   heading.
3. **Replace the single registry table with a default-build table plus a build
   matrix.**  The matrix should answer "what is menu 9?" for the four KEYB/LCD
   combinations without prose.  This prevents the duplicate-9 problem and makes
   the "Adding a mode" renumbering warning concrete.
4. **Move the test inventories to `dev/`** and keep one line per core page:
   "Suite: `tests/test_x.py` (host model; not an SIE/electrical simulator)."
   The current per-page coverage paragraphs duplicate `dev/tooling.md` and
   `dev/verification.md` and add drift.
5. **Use failure-mode tables** on every detail page.  The content is already
   trigger/symptom/recovery-shaped; the bullet format is the only reason it
   reads as a wall.
6. **Keep the one-way core → user links.**  The user pages should stay
   code-free; a backlink from `generic/` to `core/system.md` would violate that
   boundary.  Instead, make `dev/code-map.md` the explicit return path in the
   core README's "Where the other layers live" section.
7. **Trim the README to four jobs**: what the layer is, the module map (one row
   per group), the task/failure index, and the availability/limits.  The
   layer-link table and the entry-point lists belong in the detail pages or
   `dev/code-map.md`.

## Unknowns

- I did not run the native suites or build the firmware; the coverage claims
  (including "all seven `core_system` files") are unverified.  I did count the
  generated message labels: 288 for v3 and 308 for v4, matching the text.
- I spot-checked the source for the timer width, macro grammar, reset order,
  registry enum order, and signature/flag addresses.  I did not audit every
  USB descriptor byte, EEPROM timing value or HLP line against the source.
- I cannot judge v3 hardware behaviour; the docs themselves say it is
  source-derived and not verified.  That caveat is appropriate and I did not
  treat it as a defect.
- I do not know whether `test_native.py` is intentionally absent from this
  snapshot or missing from the repository; either way the core and dev pages
  disagree.  I did not try to reconstruct or run it.
- I did not verify the "512 self-test values" count or the MPLAB-X path-list
  assertion; those are test-inventory claims, not reader-facing contracts.
- The USB host-not-reading and span-invalidation recovery advice in finding 9
  is inferred from the documented contract; I did not reproduce a hung host on
  hardware.
