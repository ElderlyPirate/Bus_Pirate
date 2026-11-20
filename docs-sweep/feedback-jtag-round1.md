# JTAG docs — fresh-reader feedback (round 1)

Read as a v3/v4 owner who has never used the Bus Pirate's JTAG paths. Reading pass only: no hardware, no OpenOCD run, no XSVF run. I read the eight JTAG pages, the generic/binary pages they lean on, the XSVF and OpenOCD tool sets they link, plus `TEMPLATE.md` and `SPEC.md` as context. I did not read the keyboard set or the audit files.

## Verdict

**Yes with fixes.** The set explains the two-path model unusually well and the TAP reference is strong enough to use without a second browser tab. But the supply wiring never lands on an actual board pin, `AUX0` is used as if it were a terminal name, and the XSVF path is not honest enough about being 3.3 V push-pull only. Those are the kind of issues that stop a reader at the bench, and one of them can damage a target.

## Task log

### 1. First impression from the README alone

**Where:** `docs/protocol/jtag/README.md` only. **Time:** about two minutes. **Success:** yes.

The first table is the page's best asset: two paths, two entry bytes, two identifiers, and a "TAP owner" column. I immediately understood that there is no single mode called "JTAG" and that the difference is who walks the TAP. The "five-minute version" then gave me entry, wiring, pull-ups and clock in one screen.

What was unclear: line 3, `**Status:** v4; v3 is build-only (unverified).` I read that as "this set is for v4" and nearly stopped, because my board is a v3. The availability table two screens later says the OpenOCD path is present on v3, so the information is there — it is just ordered wrong for a v3 owner. The other front-door gap is the host: the hardware line says the OpenOCD path "needs a host that speaks `OCD1`", but not that OpenOCD's Bus Pirate driver is excluded from native Windows builds. I only found that on `tools/openocd/README.md`. For a v3 owner on Windows that is the difference between "there is a path" and "there is no path".

I also arrived with the phrase "JTAG TAP engine" in my head. The README never uses it. It says "TAP owner" and later `openocd.md` says "TAP shifter, not a JTAG engine". That is the right correction, but I had to assemble it myself. One sentence — "there is no standalone TAP-engine mode; the OpenOCD path is a raw shifter and the XSVF player owns the TAP" — would settle it instantly.

### 2. Enter the JTAG layer, find the entry table, exit cleanly

**Where:** README → `entry.md` → `openocd.md`. **Time:** about three minutes. **Success:** yes.

The 20-NUL entry, `BBIO1`, `0x06`/`0x18` sequence is unambiguous, and `entry.md`'s "What the link shows" block is the single clearest thing in the set. I could see the bytes, the identifiers and the two-step exit without reading any prose. The exit tables agree with the binary page: `0x00` leaves `OCD1` and prints `BBIO1`; the XSVF player leaves on `0x0F`, silently; a second `0x0F` returns to the terminal.

What was unclear: the README's file table says `entry.md` covers "settings", so I opened it expecting the command table. The full `OCD1` command table is in `openocd.md`. Not a dead end — `entry.md` links it at the top — but it cost a hop. The boundary between the TAP shifter and the players is clear from the README table; if I land directly on `entry.md` I have to infer it, because that page is mostly settings and player commands.

### 3. Wire TCK/TMS/TDI/TDO, TRST/SRST, pull-ups and supplies

**Where:** `wiring.md`, README five-minute version, `tools/xsvf/README.md`, `generic/commands.md`. **Time:** about five minutes. **Success:** partly.

The signal map is exactly what I wanted: one row per wire, direction, JTAG signal, role. The SRST/TRST mapping is explicit, the reset-line warning is explicit, and the pull-up matrix answers "do I need resistors?" for each path and mode. The Vpullup explanation matched the generic pages.

The failure was power. The page says the XSVF entry "feeds the VCC pin", but the signal map has no VCC row — only GND, MOSI, MISO, CLK, CS, AUX, AUX1/PGD. The board I know has `3.3V` and `5.0V` header pins. I could not tell which one the regulator energizes, which one to wire to the target's VCC, or whether both come up. For a 1.8 V target this moved from "unclear" to "I will not connect this until I know". The `AUX0` entry in the "Firmware line" column also stopped me: the terminal, the help and the generic docs call that pin `AUX`; `AUX0` is not a name I can type or find in the pin table.

### 4. Drive the TAP — reset/idle, IR/DR shifts, IDCODE, BYPASS

**Where:** `tap.md`, `openocd.md`. **Time:** about six minutes. **Success:** yes for the model, no for a copy-paste IDCODE read.

The 16-state table plus the full TMS transition table plus the "reaching a shift" table is enough. I did not need a separate JTAG state diagram. The reset rules (five TCK with TMS high, one low clock to idle), LSB-first shifting, the IR/DR length rule and the IDCODE field table all landed. The BYPASS explanation is short and useful, and the chain-probe section tells me exactly what the firmware does and where it lies to me (stuck-low TDO reporting 63 devices).

What was unclear: the worked walk gives the header and a flat list of TMS bytes, but not the 3-byte echo and not the grouped TDI/TMS payload. `5F 00 00 00 00 03` is not a byte string I can send as-is: the real payload is three groups of four bytes with the TMS bytes interleaved with zero TDI bytes. I had to go back to `openocd.md` §TAP shift and reconstruct it. The table's "Capture-IR | parallel-load the IR" also contradicts line 108, which correctly says Capture-IR loads device status bits.

### 5. Use each player once — one OpenOCD command and one XSVF run

**Where:** `openocd.md` + `tools/openocd/`, `xsvf.md` + `tools/xsvf/`. **Time:** about eight minutes. **Success:** yes; the ownership boundary is clear.

The best page for ownership is `tap.md`'s "How each path drives the TAP" table: bit source, state tracking, IR data, DR length, reset, chain discovery, clock. `openocd.md` then says "TAP shifter, not a JTAG engine", and `xsvf.md` says the player walks the TAP itself. I could answer "who owns the TAP?" without ambiguity. The tool pages provide a complete reference config and a complete player command line, and both tell me how to get out.

What was unclear: the names change when I cross the link. The JTAG set calls the firmware sub-mode the **XSVF player** and the PC program the **XSVF host player**. The tool set calls the same two things the **firmware player** and the **host tools** / **host player**. Both definitions are internally consistent, but there is no cross-map, and I re-read the first tool page to be sure "firmware player" was not a third thing. The XSVF tool page's wiring bullet also reverses the arrow convention used by every table, which made me stop and check the pinout again.

### 6. Rendering check

**Where:** all eight JTAG files, plus `tools/xsvf/` and `tools/openocd/`. **Method:** grep/script for tabs, line lengths, table column counts, link/anchor resolution, and capture alignment.

What is clean: no TAB characters anywhere; no prose line over 80 columns; every Markdown table has a consistent column count; every relative link and anchor in the set resolves; the three byte-level captures are aligned at column 26; no dates, bench narratives, pass counts or "tested on" claims in the JTAG set; the only "verified" lines are the short "not verified" caveats.

What is not clean: the XSVF tool pages carry a captured-run narrative and an internal `run_log.bat` cleanup note; the JTAG set itself uses `AUX0` for a pin the terminal calls `AUX`; "8192 bit sequences" is used where the reader needs "8192 bits"; the converter page contradicts itself on `-r`; and one tool page says "chain order" where the JTAG page says "nearest TDO first". Details in the findings.

### 7. README navigation test

**Where:** README link table, then each detail page. **Time:** under a minute to reach every core page.

Entry, wiring, TAP, clock, both player pages and edge cases are all one click from the README. The tool sets are one more click. That is the right shape.

Dead-ends and doubling back: `tap.md`, `entry.md` and `clock.md` have no footer or "Related" block. At the end of `tap.md` I had no link to `entry.md` or `edge-cases.md`; I used the browser back button to the README and started again. `tap.md` also has only three outgoing links (openocd, wiring, xsvf), so a reader who lands there directly is isolated from the entry and failure pages. The pages that do have a Related block (`wiring.md`, `openocd.md`, `xsvf.md`, `edge-cases.md`) show how easy the fix is.

## Findings

### 1. [blocker] The supply wiring never lands on a board pin

**Where:** `wiring.md:47-48` — "**The XSVF entry powers the target.**  Sending `0x18` enables the on-board regulator, which feeds the VCC pin." The signal map at `wiring.md:12-20` has rows for MOSI, MISO, CLK, CS, AUX, AUX1/PGD and GND — no VCC, no 3.3 V, no 5.0 V. `wiring.md:55-56` adds "The regulator voltage is not selectable from either JTAG path."

**Problem:** The JTAG signal map has no `VCC` row. The board's supply pins in the terminal `v` table are `3.3V` and `5.0V`. The page tells me the regulator feeds "VCC" but never tells me which header pin to connect to the target's VCC, whether both rails are energized, or what happens if I connect the target to the wrong one. The XSVF path turns the regulator on automatically, so a reader who guesses can back-drive a 1.8 V or 2.5 V target. "Not selectable from the JTAG path" is also misleading: software cannot select it, but the wiring does.

**Fix:** Add `3.3V` and `5.0V` rows to the signal map (role: "target supply, enabled by VREG"), and say plainly: "VREG energizes both header rails. Wire the target's VCC to the rail it needs — never both. Do not use the XSVF player on a target whose VCC rail is below 3.3 V." Use the same sentence in `xsvf.md` §Entry and `tools/xsvf/workflow.md` §2. If the hardware only brings up one rail, say which one; if that is unknown, say `Requires verification` instead of implying a fixed VCC.

### 2. [major] `AUX0` is a firmware symbol used as a terminal pin name

**Where:** `wiring.md:29-31` — the table header "Firmware line" and the row "| SRST | `AUX` | AUX0 |"; `tools/openocd/README.md:31` — "SRST on AUX0, TRST on AUX1 (v4) or PGD (v3)"; the same page's wiring table at line 66 — "| AUX0 | SRST (optional) |"; `tools/openocd/workflow.md:23` — "| AUX0 | SRST | Bus Pirate → target (optional) |".

**Problem:** The terminal, the `?` help and the generic AUX mapping call that pin `AUX` and its selection token `A0`. `AUX0` is the firmware's internal symbol (`BP_AUX0`), not a string the user can type or find in the pin table. `wiring.md` even says "this set uses the printed pin names" at line 36, then prints `AUX0` in the same table. The OpenOCD pages use `AUX0` without any mapping at all, so a reader who follows the tool page cannot find the pin.

**Fix:** Use `AUX` everywhere in user text, optionally with the selection token: `AUX` (`A0`). Drop the "Firmware line" column or replace it with "Terminal pin". If the internal name is useful, keep it in `docs/dev/`, not in the wiring table.

### 3. [major] The XSVF path is not honest enough about being 3.3 V push-pull only

**Where:** `wiring.md:44-46` — "A lower-voltage target should be driven through the OpenOCD **open-drain** pin mode with pull-ups to the target's own rail.  Do **not** drive a 3.3 V push-pull output into a 1.8 V chain." `edge-cases.md:24` — "| Push-pull into a lower-voltage chain | contention, damage risk | use open-drain and pull-ups |". `xsvf.md` §Entry has no lower-voltage warning; it only says the player drives push-pull.

**Problem:** The generic edge-case row says "use open-drain and pull-ups", but the XSVF player has no open-drain mode. Its entry row in `entry.md:123-130` fixes the pins as "TDI, TCK and TMS push-pull outputs". So a 1.8 V or 2.5 V target cannot use the XSVF player at all — and the player also switches on the regulator. The reader has to infer this by combining three pages. A user who reads only the XSVF tool page's "logic level is 3.3 V, interface is 5 V safe" may think "5 V safe" means 1.8 V is fine.

**Fix:** Add a row to `xsvf.md` Limits and to `tools/xsvf/README.md` Limits: "Lower-voltage target: not supported. The player is 3.3 V push-pull and powers VCC; use the OpenOCD path with open-drain and external pull-ups." Change `edge-cases.md:24`'s recovery to "OpenOCD path only: open-drain and pull-ups; the XSVF player cannot do this."

### 4. [major] The README hides that the OpenOCD host is POSIX-only

**Where:** `README.md:83-85` — "Hardware: a Bus Pirate (v4 for the XSVF player), a USB cable, the target's JTAG header and a common ground.  The OpenOCD path also needs a host that speaks `OCD1`; the XSVF player needs a v4 and an XSVF file." Compare `tools/openocd/README.md:27-29` — "Native Windows (MinGW) builds exclude it; a Cygwin/POSIX layer may work, otherwise use Linux, macOS or BSD."

**Problem:** As a v3 owner on native Windows, I have neither path: the XSVF player is v4-only, and the only v3 path (OpenOCD) has no native Windows driver. The README's availability table and hardware line do not say this. A reader can spend time planning an OpenOCD session before discovering the constraint on the tool page. "Needs a host that speaks OCD1" is technically true but does not warn about the hard OS constraint.

**Fix:** Add a `Host` row to the availability table: v3 "POSIX only (OpenOCD driver; native Windows excluded)", v4 "POSIX for OpenOCD; Windows for the XSVF host player". Add one sentence to the Hardware line. If Cygwin is a real option, say "may work" as the tool page does; do not promise it.

### 5. [major] The worked TAP walk cannot be executed as written

**Where:** `tap.md:219-222` — "A single-device chain on the OpenOCD path.  The sequence is 43 bit sequences in one `0x05` block, header `0x05 0x00 0x2B`; the TMS bytes, in transmission order, are `5F 00 00 00 00 03` (each byte least-significant bit first) and TDI is all zero."

**Problem:** The `0x05` payload is not six TMS bytes. It is three groups of four bytes, with the TMS bytes interleaved with zero TDI bytes and the high TDI/TMS bytes only for groups over 8 bits. A reader who sends `5F 00 00 00 00 03` as the payload shifts the wrong TDI/TMS pairs and gets garbage. The walk also never says to read the 3-byte echo before sending the payload, which is the first thing that will desync a hand-written host.

**Fix:** Show the exchange, not just the bit table:

```
host -> 0x05 0x00 0x2B         43 bit sequences
host <- 0x05 0x00 0x2B         3-byte echo
host -> 00 5F 00 00            TDI low, TMS low, TDI high, TMS high
host -> 00 00 00 00
host -> 00 00 00 03
host <- (6 TDO bytes)          low byte first, one or two per group
```

Or add a "group" column to the bit table. State "wait for the echo" explicitly. The bit-level table itself is correct; it is the byte-level instruction that is missing.

### 6. [major] The same firmware sub-mode and host program have two names

**Where:** `README.md:21-23` — "The **XSVF player** is the firmware sub-mode; the PC program that streams a file is the **XSVF host player**." `tools/xsvf/README.md:14-16` — "the firmware sub-mode is the **firmware player** (binary `0x18`, answers `XSV1`); the PC programs are the **host tools**, two builds of the **host player**". `tools/xsvf/limits-and-errors.md:9-10` repeats the "firmware player" / "host tools" pair.

**Problem:** Each set is internally consistent, but the cross-link changes both names. A reader who goes from `xsvf.md` to the tool set sees "firmware player" and "host player" and has to re-derive that these are the same "XSVF player" and "XSVF host player". For a reader trying to keep the TAP-owner boundary straight, that is unnecessary friction.

**Fix:** Pick one pair and use it everywhere: **XSVF player** = firmware sub-mode (`0x18`, `XSV1`); **XSVF host player** = PC program. If the tool set wants "firmware player" and "host tools", add a one-line cross-map at the top of both READMEs: "Elsewhere this is called the XSVF player / XSVF host player."

### 7. [minor] The README's status line reads as v4-only

**Where:** `README.md:3` — "**Status:** v4; v3 is build-only (unverified).  The XSVF player is v4 only."

**Problem:** The first clause makes the set look v4-only. The availability table later says the OpenOCD path is present on v3, so the detail is there, but a v3 owner may stop at the status line. The sentence also lumps "v3 is build-only" and "XSVF is v4 only" into one status, which are two different facts.

**Fix:** "**Status:** OpenOCD path on v3 (build-only, unverified) and v4; XSVF player on v4 only.  v3 hardware behaviour throughout this set is unverified."

### 8. [minor] Capture-IR is described as loading the instruction

**Where:** `tap.md:42` — "| Capture-IR | IR | parallel-load the IR |". The same page says at line 108: "Capture-IR loads device status bits; their meaning is device-specific."

**Problem:** Capture-IR does not load the current instruction into the IR; it loads a device-specific status pattern into the IR shift path. The table's one-line purpose is the first thing a reader sees and contradicts the later, correct sentence.

**Fix:** Change the purpose to "load status bits into the IR shift path" or "parallel-load the IR shift register with status".

### 9. [minor] "8192 bit sequences" is the wrong unit for a reader

**Where:** `README.md:92-93` — "One `0x05` shift is clamped to 8192 bit sequences; the echo reports the requested count." `openocd.md:105-106` — "The count is a **16-bit big-endian** number of bit sequences.  It is clamped to `0x2000` (8192) bits per command." `tap.md:219-220` — "The sequence is 43 bit sequences in one `0x05` block".

**Problem:** "Bit sequence" is the firmware's loop unit, but a reader reasonably reads "8192 bit sequences" as 8192 groups of something. The parenthetical "8192 bits" on the OpenOCD page fixes it there, but the README's first mention does not. The same word appears in the worked walk's first sentence.

**Fix:** Use "bits" in user-facing text. If the firmware field is genuinely a "bit-sequence count", define it once: "one bit sequence = one TDI/TMS bit pair clocked by TCK", then use "bits" elsewhere.

### 10. [minor] The XSVF wiring bullet reverses the table's arrow convention

**Where:** `tools/xsvf/README.md:38` — "Wire TDI→MOSI, TDO→MISO, TCK→CLK, TMS→CS, plus ground." The table on the same page at lines 67-73 is "Bus Pirate | JTAG" with `MOSI | TDI`, `MISO | TDO`, `CLK | TCK`, `CS | TMS`.

**Problem:** The connections are electrically the same, but every other table in the set puts the Bus Pirate pin on the left. This bullet puts the JTAG signal on the left, so a quick reader can read "TDI→MOSI" as "the Bus Pirate's TDI goes to MOSI" (which is not a pin) or wire the direction backwards. It is a needless second convention in a five-minute quick start.

**Fix:** "Wire MOSI→TDI, MISO←TDO, CLK→TCK, CS→TMS, plus ground." Keep the arrow direction consistent with the table.

### 11. [minor] The converter says `-r` is XC9500-only, then uses it everywhere

**Where:** `tools/xsvf/converter.md:52` — "| `-r N` | maximum retry count, N times (XC9500/XL/XV only) |". The per-family recipes at lines 74-80 use `-r 0` for CoolRunner/II, XC18V00/XCF00S, XCF00P, Spartan-II/Virtex, XC4000+, and System ACE.

**Problem:** A reader cannot tell whether `-r` is valid outside XC9500. If it is valid, the option table is wrong; if it is not, the recipes are wrong. Either way, I would not know which command to run for a CoolRunner/II file.

**Fix:** Remove the parenthetical from the option table, or explain that the bundled ReadMe describes `-r` for XC9500 but the family recipes also pass `-r 0`. State what `-r 0` means (no retries) in the recipes.

### 12. [minor] The XSVF tool pages read like a captured run and carry a stale internal script name

**Where:** `tools/xsvf/workflow.md:82-83` — "A clean Windows run of the shipped sample looks like this (banner and chunk lines elided; the full transcript is in [players.md](players.md)):". `tools/xsvf/players.md:69-72` — "The Windows folder also carries `run_log.bat`, which is stale and unsupported: it calls `BP4_XSVF_Win` (not shipped; the executable is `BP4_Full_bitbang_XSVFplayer_Win.exe`) and uses `com13`."

**Problem:** "A clean Windows run of the shipped sample" and "the full transcript" tell me this is a capture from a particular run, not an example. I do not know whether the sample is the one in my download, and the phrase ties the page to a particular run. The `run_log.bat` paragraph is repository housekeeping about a stale script; it is noise in a user page and exposes an internal log name.

**Fix:** "An example Windows session:" and "the same example, with more lines, is in players.md". Drop the `run_log.bat` paragraph or move it to a maintainer note. Keep the `run.bat`/`run2.bat` advice if it helps, but do not list a stale unsupported file.

### 13. [minor] Three detail pages have no footer or way back

**Where:** `tap.md` ends at line 240 with "expect the nearest-TDO device first." and has only three outgoing links (openocd, wiring, xsvf). `entry.md` ends at line 154 with "board reset." `clock.md` ends at line 94 with "rate is too fast." None has a `## Related` block, while `wiring.md`, `openocd.md`, `xsvf.md` and `edge-cases.md` do.

**Problem:** At the end of `tap.md` I wanted the entry page and the failure page; neither was linked. I used the browser back button to the README and started over. The same happens at the end of `entry.md` and `clock.md`. It is a small cost, but it is exactly the kind of doubling back a map page is supposed to prevent.

**Fix:** Add the same short Related block to `entry.md`, `tap.md` and `clock.md`: README, entry, wiring, TAP, clock, both player pages, edge cases. A breadcrumb at the top would also work, but the footer is the consistent pattern in this set.

### 14. [minor] The JTAG page gives one XSVF stall result; the tool page gives three

**Where:** `xsvf.md:89-92` — "If the host sends nothing for about **5 seconds** while the player waits for a chunk, the forced `0xFF` is read as the next command: the run aborts with result `0x04` (illegal command) and the player loop stays reachable." `tools/xsvf/limits-and-errors.md:48-49` — "A chunk-wait stall normally ends with `0x04`; if it lands inside a command the result can be `0x01` (or `0x06` if a length was read as `0xFF`)".

**Problem:** A reader who only reads the JTAG page will see `0x01` or `0x06` after a host stall and think the diagnosis is wrong. The tool page is more precise, but the JTAG page is the one that owns the byte protocol.

**Fix:** Add one sentence to `xsvf.md`: "Normally `0x04`; if the substituted `0xFF` lands inside a multi-byte command it can be `0x01`, or `0x06` if it is read as a length. See the XSVF tool set's error table." Keep the detailed table in the tool set.

### 15. [minor] "Chain order" does not say which end

**Where:** `tools/xsvf/players.md:112` — "the IDCODEs follow, four bytes per device in chain order." `xsvf.md:54` — "the device nearest TDO first."

**Problem:** "Chain order" could mean the first device after the Bus Pirate's TDI (the far end) or the first device to drive TDO. The JTAG page is explicit; the tool page is not. A reader comparing a scan result to a board layout needs the direction.

**Fix:** Use "nearest-TDO device first" in the tool page too, or say "in the order the chain shifts out (nearest TDO first)".

### 16. [minor] The terminal power check is suggested while the player owns the port

**Where:** `tools/xsvf/workflow.md:153-155` — "check the VCC pin, ground and the regulator (the terminal power-supply check reports `VREG too low, is there a short?`)."

**Problem:** The terminal `W` check is not available while the XSVF player is in its command loop. The sentence reads as if I can run it during a failed run. I would have to reset first, then run `W`, which is a different recovery step.

**Fix:** "After resetting to the terminal, `W` reports `VREG too low, is there a short?` if the rail is not coming up."

### 17. [minor] "An earlier AUX write" is too broad

**Where:** `wiring.md:90-92` — "If AUX (SRST) or AUX1 (TRST) was left driving low by an earlier AUX write, release it or reset the board before entering the player." Compare `../binary/entering-and-framing.md:50-54`: binary entry leaves AUX an input.

**Problem:** A terminal `a0` before the 20 NULs does not leave AUX low; binary entry resets it. The only way AUX is still driven low when `0x18` is sent is an AUX write inside BBIO1, because the XSVF entry does not run the binary reset. As written, the warning makes me worry about a terminal action that cannot cause the problem.

**Fix:** "If AUX (SRST) or AUX1 (TRST) was left driving low by a pin write inside `BBIO1`, release it or reset the board before entering the player." Add the same qualifier to the XSVF entry row.

### 18. [minor] The OpenOCD worked exchange enables VREG without a local warning

**Where:** `openocd.md:156-157` — "host -> `0x01 0x01` pin mode = JTAG (push-pull)" then "host -> `0x02 0x02 0x01` feature VREG = on". The warning about VREG is above, at lines 92-93.

**Problem:** The block is labelled a format illustration, but it is also the only full command sequence on the page. A reader can copy it onto a self-powered target and enable the regulator. The warning exists, but it is not in the example.

**Fix:** Use the mode LED in the example (`0x02 0x01 0x01`) or add a comment on the VREG line: "VREG on only if the board powers the target."

## What to keep

- **The two-path table at the top of the README.** Entry byte, identifier and TAP owner in one row. It is the whole mental model in four lines.
- **"How each path drives the TAP" in `tap.md`.** The best ownership table in the set: every job, both paths, no ambiguity.
- **The 16-state table, the full TMS transition table and the "reaching a shift" table.** I did not need a separate JTAG reference. That is rare and worth protecting.
- **The signal map and the pull-up matrix.** One row per wire; "no pull-up needed" or "2–10 kΩ" stated per mode. This is exactly the shape a wiring page should have.
- **The byte-level exchanges.** `entry.md`'s "What the link shows", `openocd.md`'s worked exchange and `xsvf.md`'s worked exchange made the protocols concrete. The aligned columns and the "`0x01` is a raw byte, not the text '1'" note are the kind of detail that prevents silly mistakes.
- **The exit tables.** `0x00` vs `0x0F`, the silent XSVF return, the second `0x0F`, and the "XSVF ignores `0x00`" rule are all in one place and consistent.
- **The chain-probe honesty.** "Stuck-low TDO reports 63 devices of garbage" and "stuck-high reports zero devices" are the sort of failure signatures a reader remembers at the bench.
- **"TAP shifter, not a JTAG engine."** It corrects the most likely wrong expectation in one sentence.

## Structural opinion

The layout works for this function. A map README, then one page per concern (entry, wiring, TAP, clock, each player, edge cases) is the right shape; the protocol/tool split is also right, because the byte protocol and the host workflow really are different jobs. I would not merge `openocd.md` into `tools/openocd/` or `xsvf.md` into `tools/xsvf/`; the cross-links are clean.

Concrete proposals:

1. **Add a "Two paths at a glance" table to the README** with columns: path, entry, identifier, TAP owner, host program, board support, host OS. This would answer the first three questions of a new reader in one table and would expose the POSIX-only constraint without a separate paragraph.
2. **Add a "Power" table to `wiring.md`**, next to the signal map, with rows `3.3V`, `5.0V`, `VCC` (target), `GND`, and the rule "VREG energizes both rails; connect the target to one, never both." This belongs with wiring, not buried in a bullet list called "Levels and voltage".
3. **Add a one-sentence "no standalone TAP engine" definition to the README.** "There is no separate TAP-engine mode: the OpenOCD path is a raw shifter and the XSVF player owns the TAP." It would settle the phrase a reader is most likely to arrive with.
4. **Give `entry.md`, `tap.md` and `clock.md` the same footer as the other pages.** A consistent Related block is cheaper than a breadcrumb and matches the rest of the set.
5. **Move the worked TAP walk's byte-level detail into the walk itself.** A full payload block or a group table, plus "wait for the echo", would turn a correct bit table into something I can actually type.
6. **Put the XSVF player's electrical limits in its own Limits section.** "3.3 V push-pull only; no open-drain; powers VCC; not for 1.8 V/2.5 V targets" is a single row that currently has to be assembled from three pages.
7. **Standardize the player names across the two sets.** One pair, or a one-line mapping at the top of each README. The current split is small but it lands exactly where the reader is trying to keep the TAP-owner boundary straight.
8. **Keep the examples, lose the capture language.** "An example Windows session" and "a run ends with `XSVF_ERROR_NONE`" are enough; "clean run of the shipped sample" and "full transcript" make the page feel like a test report.

## Unknowns

- I did not run any of this on hardware. I cannot judge whether the 400-bit shift limit, the 4096-byte chunk limit, the 8192-bit clamp or the stall timing are correct; I can only judge whether the explanation is followable. The pages label the code-derived limits as unverified, which is the right posture.
- I cannot judge v3 behaviour. The set says it is build-only and unverified; I have no v3 to compare.
- I cannot judge the electrical details of the on-board regulator, the pull-up value, or the 5 V tolerance without a schematic. I can judge that the docs never map "VCC" to a header pin and never say which rail the regulator brings up.
- I cannot judge whether OpenOCD's Bus Pirate driver actually works through Cygwin on Windows; the tool page says "may work", and I did not test it.
- I cannot judge whether the exact OpenOCD option names, version matrix, result-message wording or host-player banners match the installed tools. I read them as documentation, not as a test report.
- I cannot judge whether `AUX0` is ever printed by some firmware build. I can say it is not in the terminal/help mapping used by the generic docs, and the JTAG set itself says the printed pin name is `AUX`.
