# SPI docs — fresh-reader feedback (round 1)

## Verdict

**Yes with fixes.** I could enter SPI, wire it, do a byte exchange, sniff, and run a binary frame from these pages. The main obstacles to a clean first run are not missing content; they are one contradictory statement about the sniffer's exit state, a missing pull-up-voltage warning, and a bring-up order split across three pages.

## Task log

### 1. First impression from the README alone

- **Where I looked:** `docs/protocol/spi/README.md`, top to bottom.
- **How fast:** under a minute.
- **Success:** yes. I knew SPI was mode 5, that the Bus Pirate is the master, that terminal and binary paths exist, and that the first command is `m5` then `W`.
- **What was unclear:** "v3 is **build-only** and not verified" (`README.md:3`). As a v3 owner I could not tell whether SPI runs on my board or only compiles for it. The availability table says "terminal mode (`m5`) | yes | yes", so the answer is probably yes, but the status line contradicts that first impression. The quick start also tells me to run `W` without telling me when to run `e`/`P` for the pull-up, so "where do I start" is clear at the command level and fuzzy at the wiring level.

### 2. Enter SPI, choose speed/polarity/edge, wire, byte exchange

- **Where I looked:** README → `entry-and-setup.md` (menu capture, speed table, CPOL/CPHA table) → `wiring.md` (pins, pull-ups, supply) → `docs/generic/commands.md` (`e`/`P`) → `bus-operations.md` (worked session).
- **How fast:** 10–15 minutes, four pages.
- **Success:** yes on paper. The sequence `m5`, accept defaults, `W`, `[`, `0x9F`, `r`, `]` is unambiguous, and the `{` read-back example is concrete.
- **What was unclear:**
  - The order for on-board pull-ups. The README says `W`; `wiring.md` says `P` needs a voltage on Vpullup; `generic/commands.md` says "`e2`/`e3`, then `P`". I had to assemble "enter mode → `e2`/`e3` → `P` → `W`" myself.
  - The pull-up voltage is not tied to the target's logic level. `e3` is 5.0 V; the page never says "do not put 5 V on a 3.3 V target".
  - `m 5` vs `m5`: the table says "`m`, then the menu number" is the menu form, then the prose says `m` **alone** is the menu form. I read the table first and expected the wrong output.
  - The `{` flag's stickiness is only clear in `bus-operations.md`, not in the README bullet.
  - "Check the clock against the target's limit" is good advice but assumes a scope; the default 30 kHz is the practical answer and isn't stated as "start there".

### 3. Terminal macros and sniffer

- **Where I looked:** `macros.md` → `sniffer.md`.
- **How fast:** 2–3 minutes to find the macro list, the CS filter, and the stop rules.
- **Success:** yes. `(1)`/`(2)`, the empty 3–9 range, the 10–15 tweaks, the physical-CS filter, and the "stop only when the master is quiet" rule are all findable.
- **What was unclear:**
  - The sniffer "Usage" block is a format sample, not a walkthrough. It does not say "disconnect the target first", does not show the stop keypress, and does not show the post-exit state.
  - The macro page shows the `(0)` list but never shows `(11)` (or any tweak) actually printing a changed header.
  - "A closing marker can appear next to the following pair" (`sniffer.md:96`) is hard to parse. I do not know what "the loop's serial pass" means or how to tell a misplaced marker from a real one.
  - The binary stop rule says "send any byte", but the only concrete byte discussed is `0x00`, which exits to `BBIO1`. I wanted an example that stops and stays in `SPI1`.

### 4. Binary SPI layer

- **Where I looked:** README → `binary.md` (entry, defaults, command table, worked frames) → `bus-operations.md` to compare the terminal wrapper.
- **How fast:** about five minutes.
- **Success:** yes. The one-frame sequence (`0x8A`, `0x62`, `0x04`, two counts, payload) is followable, and the `0x04` (CS managed) vs `0x05` (CS untouched) difference is clear.
- **What was unclear:**
  - The README's binary quick bullet (`README.md:41-43`) lists `0x04`/`0x05`, `0x1x`, `0x8x` but not `0x02`/`0x03` (CS) or `0x4x` (power/pull-ups). A first frame from the README alone is incomplete.
  - The `0x8x` "unset bits take their own defaults" trap is explained in prose but is easy to get wrong; a common-values table would help.
  - `0x06` means OpenOCD/JTAG in `BBIO1` and the AVR helper in `SPI1`; the entry note lists `0x08`, `0x09`, `0x10` and `0x18` collisions but not `0x06`.
  - The `0x06` version reply (`0x01 0x01 0x00 0x01`) is not explained, and a zero-length bulk read is not covered.

### 5. Documented edge cases: `0x4x` CS direction and `0x06` AVR helper

- **Where I looked:** `binary.md` §Edge cases → `edge-cases.md` §Binary.
- **How fast:** about two minutes.
- **Success:** yes, the recovery is actionable: send `0x02`, keep bit 0 clear, or re-run `0x6x`/`0x8x`. The `0x06` double-ack and the word-address/byte-length units are clear.
- **What was unclear:**
  - The `0x4x` "symptom" is "`0x04` does not drive CS low" — that is the cause, not what I see at the target. I would see the target ignore the transfer and read `0xFF`/noise.
  - `binary.md`'s "`0x03` leaves CS released (Hi-Z)" and `edge-cases.md`'s "the pin can stay an input" are different levels of detail; the second is the important one.
  - The `0x06` availability is contradictory: present in both builds in the v3/v4 table, but "a build without it answers `0x00`" in the text. I cannot tell whether my default build has it.

### 6. Rendering check

- **Where I looked:** all eight SPI files plus the `docs/generic/` pages; scanned for tab characters, long prose lines, dates, "capture"/"tested"/"verified" language, and naming variants.
- **How fast:** about ten minutes.
- **Success:** no TAB characters in any SPI or generic page. The speed menu, the `v` pin table, and the binary frame comments are carefully aligned. No dates, no pass counts, no "tested on" prose.
- **Problems found:** "capture" provenance in `entry-and-setup.md`; "terminal buffer" vs "output buffer" vs "terminal command buffer"; "The v3 column" in `macros.md` where there is no column; long edge-case table rows; "build-only" jargon; the same v3 caveat repeated. Details in the findings.

### 7. README navigation

- **Where I looked:** the README link table and every page's "Related docs".
- **How fast:** reference tables one click (`entry-and-setup.md`), worked sessions one click (`bus-operations.md`), binary frames one click (`binary.md`), edge cases one click from the README.
- **Success:** mostly. The map is complete and the file names are honest.
- **Dead ends:** no detail page's "Related docs" links to `edge-cases.md`. After hitting the `0x4x` trap in `binary.md`, I had to go back to the README to find the consolidated edge-case page. The same is true from `bus-operations.md`, `wiring.md` and `sniffer.md`.
- **Doubling back:** to do task 2 I went entry → wiring → generic → entry → bus-operations. A single linear "first transaction" path would remove that.

## Findings

Sorted by severity.

### 1. [severity: major] The sniffer's exit state is described two different ways

- **Location:** `docs/protocol/spi/entry-and-setup.md:284-285`, `docs/protocol/spi/sniffer.md:126-128`, `docs/protocol/spi/wiring.md:124-125`, `docs/protocol/spi/binary.md:336-337`.
- **Quote:**
  - `entry-and-setup.md:284-285`: "**The sniffer** configures its own pins as inputs and restores the master configuration on exit"
  - `sniffer.md:126-128`: "Normal exit restores the master pin configuration (CLK, MOSI and CS become outputs), so stopping on a live bus can contend with a master that is still driving those lines."
- **Problem:** "Restores" means "returns to what was there before". The docs tell me to enter the sniffer before `W`, in which case the previous state is inputs. But `sniffer.md` says the exit makes CS/CLK/MOSI outputs. I cannot tell whether exiting will start driving the bus, whether `W` is still required afterwards, or whether the SPI module itself is configured. This is the one place where a wrong guess can contend with a live external master. `wiring.md:123` makes it worse by saying "the master configuration **drives** CS, CLK and MOSI until the sniffer macro runs"; if `W` has not run, the pins are inputs, so "can drive" (as `sniffer.md:123` says) is the accurate wording.
- **Suggested fix:** Pick one authoritative sentence and use it in all four pages: "On exit the sniffer configures CLK/MOSI/CS as outputs (the master pin setup); it does not restore the previous input state. Run `W` after the sniffer if the SPI module is not already engaged." If the firmware actually restores inputs when `W` was never run, say that instead and delete "become outputs" from `sniffer.md`. Change `wiring.md`'s "drives" to "can drive".

### 2. [severity: major] Vpullup is never tied to the target's logic voltage

- **Location:** `docs/protocol/spi/wiring.md:57-61` and `78-81`.
- **Quote:** "Every high level on CS, CLK and MOSI comes from the pull-up, so fit **one pull-up per output line** — the on-board pull-ups switched by `P`, or external resistors of **2 kΩ – 10 kΩ** to the target's logic rail." / "`e2` routes the on-board **3.3 V** rail, and `e3` routes the on-board **5.0 V** rail."
- **Problem:** The "target's logic rail" phrase applies to external resistors only. The on-board `e3` route is 5.0 V, and the page never says that a 5 V pull-up on a 3.3 V target can damage it. A novice with a 3.3 V flash chip can reasonably pick `e3` because it is the Bus Pirate's 5 V rail, and put 5 V on CS/CLK/MOSI. The target-supply section separately says "connect the target's VCC to the rail it needs", but Vpullup is a different pin and the warning is not repeated.
- **Suggested fix:** Add a bold sentence in §The pull-up reference and an edge-case row: "Set Vpullup to the target's logic supply. A 5 V pull-up on a 3.3 V target can damage it. On v4 use `e2` for a 3.3 V target and `e3` only for a 5 V target, or feed `Vpu` externally at the target's voltage." Also state that `W`'s 3.3 V/5.0 V target rails are separate from the Vpullup reference.

### 3. [severity: major] The on-board pull-up bring-up order is split across three pages

- **Location:** `docs/protocol/spi/README.md:27-34`, `docs/protocol/spi/wiring.md:84-88`, `docs/generic/commands.md:35-40`.
- **Quote:** README: "Enter with `m5` ... then run `W`." / `wiring.md`: "`P` enables the on-board pull-ups. A voltage must be present on Vpullup for them to pull high" / `generic/commands.md`: "Enter a non-HiZ mode whose setup answers `Select output type:` with open drain ..., then issue `e2`/`e3`, then `P`."
- **Problem:** The first-time sequence is not in one place. I had to infer "enter mode → `e2`/`e3` (v4) → `P` → `W`". The README's five-minute version only mentions `W`; `wiring.md` mentions `P`/`e`; the generic page has the order. A reader can easily run `P` before `e` (gets `Warning: no voltage on Vpullup pin`) or wonder whether `W` should come before `e` (because `W` runs the pin setup).
- **Suggested fix:** Add a numbered "bring-up with on-board pull-ups (v4)" block to `wiring.md` or `entry-and-setup.md`, and link it from the README's `W` bullet. Example: 1) wire with the target unpowered; 2) `m5`, choose open drain; 3) `e2`/`e3` to match the target; 4) `P`; 5) `W`; 6) verify with `v`. If the real order is different, state the real order explicitly.

### 4. [severity: major] Generic `;N` advertises a 16-bit read that SPI does not have

- **Location:** `docs/generic/values.md:83-84` and `90`; `docs/protocol/spi/bus-operations.md:101-106`.
- **Quote:** `values.md`: "`;N` sets the read/write width in bits" and "| `r;16` | one 16-bit read |". `bus-operations.md`: "`;N` (the shared width setting) changes the **displayed** width and the window used by the bit-order setting; it does not make the transfer N bits wide."
- **Problem:** `bus-operations.md:5-8` tells me the shared grammar is owned by `docs/generic/`, and `values.md` then advertises `r;16` as a 16-bit read. In SPI, `r;16` clocks one byte and only displays wider. A reader who follows the link first will believe the opposite of what the SPI page says. This is a direct contradiction, not just a missing caveat.
- **Suggested fix:** Add the byte-oriented exception to `values.md` next to the `r;16` row: "In a byte-oriented mode (SPI), `;N` changes only the display and bit-reversal window; the mode page owns the transfer width. `r;16` in SPI is not a 16-bit read." Cross-link to `bus-operations.md` §Bit and word transfers.

### 5. [severity: major] The sniffer "Usage" section is not a walkthrough and puts the safety step last

- **Location:** `docs/protocol/spi/sniffer.md:10-24` and `122-128`.
- **Quote:** "SPI>(1) / Sniffer / Any key to exit / [0x30(0x00)0xFF(0x12)0xFF(0x50)][0x40(0x00)] / SPI>" and, 100 lines later, "Enter the sniffer **before** connecting the target."
- **Problem:** The first sniffer section a reader lands on shows `SPI>(1)` as step one. It does not say to disconnect the target first, does not show the keypress that stops the capture, and does not show what the prompt looks like after exit. The safety instruction is in §Safety, after §Output format, §CS-low vs all traffic and §CS capture path. A reader who follows §Usage can connect a live target before entering the sniffer, which is exactly the contention the docs warn about.
- **Suggested fix:** Turn §Usage into a five-step procedure: 1) target disconnected; 2) `(1)` or `(2)`; 3) connect the target; 4) observe the stream; 5) press any key while the external master is quiet, show the newline and `SPI>`, and state the exit pin state (see finding 1). Move the "before connecting" warning into step 1, not only §Safety.

### 6. [severity: minor] `m 5` vs `m5` is easy to read the wrong way

- **Location:** `docs/protocol/spi/entry-and-setup.md:19-27`.
- **Quote:** "| `m`, then the menu number | menu form; the setup prompts run |" and "what makes it the menu form is typing `m` **alone**, with no number on the same line."
- **Problem:** The table reads as if `m 5` on one line is the menu form. The next sentence says the opposite: `m 5` is the direct form (spaces allowed), and only `m` by itself is the menu form. I typed `m 5` from the table and got the direct-form output (`Ready` only) when I expected `Clutch disengaged!!!` and the finish-setup line.
- **Suggested fix:** Change the first row to "`m` [Enter], then the number at `(1)>`" and add a row "`m 5` or `m 5 <six values>` | direct form; spaces are allowed". Put the "m alone" rule next to the rows, not three lines below.

### 7. [severity: minor] The README's `{` bullet does not say the read-back flag is sticky

- **Location:** `docs/protocol/spi/README.md:35-37`; `docs/protocol/spi/bus-operations.md:29-31`.
- **Quote:** README: "`{` asserts and makes every following write also report the byte clocked in." `bus-operations.md`: "The flag set by `{` is **not** cleared by `]` or `}`; it stays set until `[` clears it or the mode is re-entered. `w`/`W` do not clear it."
- **Problem:** The README bullet reads like a per-transaction mode. The sticky behavior is a common surprise: after `{`, `]`, and a fresh write, I still get `READ:` and may think the mode is broken. The detail page has the rule, but the quick start does not.
- **Suggested fix:** Add "the flag is sticky: `]`/`}` and `w`/`W` do not clear it; use `[` for the next plain transaction" to the README bullet, or make the bullet a link to `bus-operations.md` §Chip select.

### 8. [severity: minor] The `0x4x` CS-direction trap's "symptom" is really the cause

- **Location:** `docs/protocol/spi/binary.md:424-425`; `docs/protocol/spi/edge-cases.md:111-112`.
- **Quote:** "| A `0x4x` released CS as an input | `0x04` does not drive CS low | send `0x02` first, or keep bit 0 clear |" / "| `0x04` is used after a `0x4x` that released CS as an input | CS is not driven low | ..."
- **Problem:** "`0x04` does not drive CS low" is the mechanism, not what I observe. At the target, the transfer is ignored and reads return `0xFF`/noise; I might blame the target or the wiring. The two tables also differ in detail: `binary.md:425` says "`0x03` leaves CS released (Hi-Z)", while `edge-cases.md:112` says "the latch is high but the pin can stay an input; in open drain it is Hi-Z either way." The second is the important part of the trap; the first can be read as normal behavior.
- **Suggested fix:** In both tables, write the observable symptom: "the target ignores the transfer; MISO reads `0xFF`/noise; a scope or `v` shows CS never goes low." Add: "`0x03` alone only writes the latch; it does not restore the output. Use `0x02`, or `0x6x`/`0x8x` to re-run the setup."

### 9. [severity: minor] The `0x06` helper needs its context and availability pinned down

- **Location:** `docs/protocol/spi/binary.md:22-27`, `252-296`, `439`.
- **Quote:** "inside `SPI1`, `0x08` and `0x09` are unknown actions, `0x10` is a one-byte bulk transfer and `0x18` is a nine-byte bulk transfer — not the XSVF entry." / "This command exists because the build enables the extended AVR option; a build without it answers `0x00` to `0x06`." / "| extended AVR command (`0x06`) | present | present |"
- **Problem:** Three gaps. (a) `0x06` is the `BBIO1` OpenOCD/JTAG command; inside `SPI1` it is the AVR helper. The page lists other layer collisions but not this one. (b) The text says a build without the extended AVR option answers `0x00`, while the v3/v4 table says "present | present"; I cannot tell whether my default build has it. (c) The version sub-command's `0x00 0x01` reply is not explained, and a zero-length bulk read is not documented.
- **Suggested fix:** Add `0x06` to the collision note ("inside `SPI1`, `0x06` is the extended AVR helper, not the `BBIO1` JTAG command"). Change the availability sentence to "present in the default v3/v4 builds; a custom build without the extended AVR option answers `0x00`." Explain the version bytes (for example "`0x0001`, version 1") and state what a zero length returns.

### 10. [severity: minor] "build-only" is undefined and the v3 caveat is repeated three times in the README

- **Location:** `docs/protocol/spi/README.md:3`, `52`, `63`, `91`.
- **Quote:** "**Status:** v4 documented; v3 is build-only and not verified." / "| build | present, build-only | present, default image |" / "The v3 column is **not verified**; v3 is build-only here." / "All v3 behaviour in this set is build-only and **not verified**."
- **Problem:** As a v3 owner, "build-only" does not tell me whether SPI runs on my board. The availability table says "terminal mode (`m5`) | yes | yes", which suggests it does, but the status line suggests only a build exists. "Default image" is also jargon. The same caveat appears three times in one page, which makes the set feel defensive rather than clear.
- **Suggested fix:** "**Status:** v3 and v4 documented; the v3 hardware behaviour is not verified." In the table: "build | v3 build present (not hardware-verified) | default v4 firmware". Keep one caveat in the status and one in the v3/v4 table; drop the third.

### 11. [severity: minor] "Terminal buffer", "output buffer" and "terminal command buffer" name different things

- **Location:** `docs/protocol/spi/README.md:61`; `docs/protocol/spi/sniffer.md:137-140`; `docs/generic/README.md:138`.
- **Quote:** "| terminal buffer | 4096 B | 4096 B |" / "Sniffer output is queued in the terminal buffer ... the CS marker ring, the SPI input FIFOs and the output buffer share one abort handling." / "| terminal command buffer | 128 B | 256 B |"
- **Problem:** "Terminal buffer" (4096 B output) and "terminal command buffer" (128/256 B input) are different things; `sniffer.md` then says "output buffer" for the same 4096 B buffer. A reader can think the docs contradict themselves, or that the sniffer is limited to the 128/256 B command buffer.
- **Suggested fix:** Define "terminal output buffer (4096 B)" once and use it everywhere; use "command buffer (128/256 B)" for the input line. In `sniffer.md`, replace "output buffer" with "terminal output buffer" or say "the same output buffer".

### 12. [severity: minor] `macros.md` refers to a "v3 column" that does not exist, and never shows a tweak in action

- **Location:** `docs/protocol/spi/macros.md:50-62`, `97-100`.
- **Quote:** "The macro list, the sniff macros and the clock tweaks 10–15 are the same in both builds. The v3 column is **not verified** (v3 is build-only here)."
- **Problem:** There is no v3 column in that section; the sentence is a copy-paste artifact. Also, the clock tweaks are described in a table but never shown: `(0)` has a capture, `(11)` does not. I do not know what the header looks like after a tweak, which is the one thing the macro does.
- **Suggested fix:** Change to "The v3 behaviour is not verified." Add a three-line capture of one tweak (for example `SPI>(11)` → `SPI (spd ckp ske smp csl hiz)=( 4 1 1 0 1 0 )` → `SPI>`), or say explicitly that the header prints with the new field.

### 13. [severity: minor] "Menu-form capture" and "The capture below" read like capture provenance

- **Location:** `docs/protocol/spi/entry-and-setup.md:40-42`.
- **Quote:** "### Menu-form capture" / "The capture below is the default v4 build; the default v3 build omits `KEYB` ..."
- **Problem:** The section reads like a bench transcript, and "capture" is the word the rest of the docs use for the sniffer's live bus capture. The same page otherwise presents examples as examples ("The `0x9F` / `0xEF` values are example values"). The heading makes a reference section sound like evidence.
- **Suggested fix:** "### Menu-form example" and "The default v4 menu is:". If the point is that the spacing is exact, put that in a note: "Transcribed with the firmware's spacing; the blank line before each prompt is printed by the prompt reader."

### 14. [severity: minor] The README map comes before the five-minute version, and the binary quick bullet is incomplete

- **Location:** `docs/protocol/spi/README.md:15-25`, `41-43`, `92-94`.
- **Quote:** the seven-row file table immediately after the intro; "Binary: 20 NUL bytes enter `BBIO1`, then `0x01` enters `SPI1`. `0x04`/`0x05` are write-then-read, `0x1x` is bulk, `0x8x` sets clock/output options."; "Shared prompt grammar (`r`, `:N`, `;N`, `W`/`w`, `P`/`p`, `e`, `v`, `(n)`, wrappers, `#`, `$`) ..."
- **Problem:** The first screen is a dense map; the quick start is below it. The binary bullet lists the data commands but not `0x02`/`0x03` (CS) or `0x4x` (power/pull-ups), so it cannot produce a first frame without opening `binary.md`. The shared-grammar list omits `o` and `l`/`L`, which the SPI pages use.
- **Suggested fix:** Put "The five-minute version" above the file table, or add a one-line "Start here" link at the top. Add "`0x02`/`0x03` assert/release CS; `0x4x` sets power/pull-ups" to the binary bullet. Add `o` and `l`/`L` to the shared-grammar list.

### 15. [severity: minor] The `0x44`/`0x4C` examples also drive AUX low, but the comments omit it

- **Location:** `docs/protocol/spi/binary.md:175-177`, `382`.
- **Quote:** "`0x44` is pull-ups on with CS low." / "`0x44` pull-ups on, CS low (no target supply)"
- **Problem:** `0x44` = `0100 0100`; bit 1 is clear, so it drives AUX low. The comment names only pull-ups and CS. If something is connected to AUX, or AUX is assigned to CS, this is an unexpected side effect. The `0x4x` bit table does define bit 1, but the worked frame's comment is the part a reader copies.
- **Suggested fix:** "`0x44` = pull-ups on, AUX low, CS low" (and the same for `0x4C`/`0x4D`), or mask the AUX bit in examples that do not intend to change it.

### 16. [severity: minor] Two sniffer edge-case explanations are too terse to use

- **Location:** `docs/protocol/spi/sniffer.md:94-98` and `147-151`; `docs/protocol/spi/edge-cases.md:97`.
- **Quote:** "The exact split between a boundary marker and the adjacent pair is limited by the loop's serial pass, so a closing marker can appear next to the following pair." / "send any byte to leave the aborted sniffer, and that byte becomes the next command."
- **Problem:** The boundary sentence is opaque: I do not know what "the loop's serial pass" is, or how to tell a misplaced `]` from a genuine one. The binary stop rule says "send any byte" but gives no example that stays in `SPI1`; the only concrete byte discussed is `0x00`, which exits to `BBIO1`.
- **Suggested fix:** Replace the boundary sentence with a concrete example ("a `]` can print after the next `[`; the byte pairs are still in order"). Add: "For example, send `0x01` to stop and immediately get `SPI1` back; send `0x00` to exit to `BBIO1`."

### 17. [severity: minor] The longest edge-case table rows are unreadable in raw Markdown

- **Location:** `docs/protocol/spi/edge-cases.md:112` (200 characters), `docs/protocol/spi/binary.md:425` (135), `docs/protocol/spi/bus-operations.md:153` (164).
- **Quote:** "| `0x03` is used after a `0x4x` that released CS | the latch is high but the pin can stay an input; in open drain it is Hi-Z either way | use `0x02` to drive it low; `0x6x`/`0x8x` restore the output |"
- **Problem:** In a text editor or an 80-column terminal, these rows wrap into a block and the trigger/symptom/recovery boundaries disappear. The content is good; the presentation is hard to scan. This is the one place where the table format works against the reader.
- **Suggested fix:** For the longest entries, use a two-column layout (trigger | symptom and recovery) or a short definition list; keep the three-column table for the short rows. Alternatively, split the symptom and recovery with a sentence break so the wrapped text still has visible structure.

## What to keep

- **The CPOL/CPHA table in `entry-and-setup.md`.** It is the single best thing in the set: I could map a target datasheet's mode 0/3 to two prompt answers without guessing. The note that the input sample phase is *not* CPHA is the kind of precision that saves an afternoon.
- **The settings-header table and the warning that the header is what the firmware prints, while the one-line form takes menu numbers.** The `m5 4 1 2 1 2 2` worked example made the mapping concrete.
- **The wiring direction table and the open-drain/Normal trade-off.** "CS, CLK and MOSI are outputs; MISO is an input; ground is common" plus "a pull-up is required unless Normal" is the right headline.
- **The bus-operations worked session.** `W` → `[` → `0x9F` → `r` → `]` is exactly the first transaction I wanted, and the `{` example shows the read-back form without a second page.
- **The sniffer's CS-filter explanation.** "The markers follow the physical CS level, not the mode's CS-polarity setting" and "use `(2)` for an active-high bus" is a subtle point handled well.
- **The `0x4x` CS-direction paragraph in `binary.md`.** "Setting bit 0 releases CS as an **input** ... `0x04` and `0x03` only write the CS latch; they do not change the pin direction" is the clearest explanation of a nasty trap I have read in a hobbyist doc.
- **The `0x06` double-ack table and the word-address/byte-length units.** Once I read it, the frame made sense.
- **The trigger → symptom → behaviour/recovery format in `edge-cases.md`.** It is the right shape for a failure reference; my complaints are about duplication and row width, not the format.
- **The honesty about the sniffer's unknown maximum rate.** "Do not assume a 10 MHz bus can be captured" is better than a fake number.

## Structural opinion

The eight-file split is right for this function: README as map, then entry, wiring, bus operations, macros, sniffer, binary, edge cases. I would not collapse it. Three changes would make it work better:

1. **Pick one home for edge cases.** Right now every page has an "Edge cases" table *and* `edge-cases.md` repeats the same rows. The duplication has already drifted: the sniffer exit is "restores" in three pages and "become outputs" in one; the `0x03` trap is "leaves CS released" in `binary.md` and "the pin can stay an input" in `edge-cases.md`. Keep the per-page tables for local context, but make `edge-cases.md` a short index of "symptom → page/anchor" instead of a second copy, or move all detail to `edge-cases.md` and leave one-line pointers. Do not keep both full copies.
2. **Split the binary page, or give it a table of contents.** `binary.md` is 453 lines and covers entry, electrical setup, the command table, `0x04`/`0x05`, bulk, faults, `0x4x`, `0x5x`, `0x6x`, `0x8x`, `0x06`, the binary sniffer, three worked frames and edge cases. The `0x06` helper and the binary sniffer are sub-functions with their own state; they deserve `binary-avr.md` and `binary-sniffer.md` (or at least an in-page TOC at the top). The task-4 reader needs entry + one frame + the CS/power commands; they should not scroll past the AVR helper to find them.
3. **Add a linear "first transaction" page.** The README is a map and the detail pages are modular, so a first-time reader bounces: entry → wiring → generic → entry → bus-operations. A single `quickstart.md` (or a "First transaction" section in the README) with one path — wire (including Vpullup matching), enter, choose Normal or open drain, `W`, `[`, write, `r`, `]`, `w` — would remove the doubling back. The five-minute version is close; it needs the electrical order and the exact `e`/`P`/`W` sequence.

Two smaller proposals:

- **Add common-value tables** for `0x4x` and `0x8x`: `0x44` (pull-ups, CS low), `0x4C` (power + pull-ups, CS low), `0x4D` (same, CS released); `0x80`/`0x82`/`0x88`/`0x8A` for the four common clock/output combinations. The prose is correct, but the bit math is where I slowed down.
- **Add one side-by-side terminal-vs-binary table** for the same `0x9F`/`0xEF` exchange: terminal `[`, `0x9F`, `r`, `]` vs binary `0x04` with counts and payload. That would answer "is the difference clear?" faster than two separate worked examples.

Finally, make "Related docs" bidirectional. Add `edge-cases.md` to every page's related list, and add `binary.md` ↔ `bus-operations.md` links where the two paths do the same thing. The consolidated edge-case page is currently reachable only from the README, which is a dead end once you are deep in `binary.md`.

## Unknowns

- I could not judge whether the binary entry defaults (speed index 1 = 125 kHz), the speed-index mapping, or the `0x06` presence match the actual firmware; I only checked that the pages agree with each other.
- I could not judge the v3 hardware claims (polled CS, 10-column `v` table, 7-entry history, 4096-byte buffer). The docs say v3 is not verified; I treated those as claims, not facts.
- I could not judge whether `w` leaves the on-board pull-ups enabled. The `w` capture shows only `Clutch disengaged!!!` and `POWER SUPPLIES OFF`; no page says what happens to `P`. A reader may assume `w` is a full safe state.
- I could not judge whether exiting the sniffer configures the SPI module or only the pins. The docs contradict each other on the pin state, and neither page says whether `W` is still required.
- I could not judge 1.8 V target support. The wiring page says "the target's logic rail" and the interface is "5 V safe", but the input high threshold is not specified; a 1.8 V target may not read as high.
- I could not judge the real sniffer throughput or the 32-entry marker ring; the docs are honest that the maximum rate is unspecified.
- I could not judge whether `m 5` (with a space) is parsed as the direct form on hardware. The entry page says it is; I did not run a board.
- I could not judge what `0x06 0x02` with a zero length returns, or what the version bytes `0x00 0x01` mean beyond "version".
