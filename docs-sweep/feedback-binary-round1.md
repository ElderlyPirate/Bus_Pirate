# Binary layer — round-1 reader feedback

**Verdict: yes with fixes.** The set is usable enough to write a host, but three cross-page conflicts and one missing pin-I/O caveat are the kind of things that cause a silent desync or a powered-down target.

---

## Task log

| # | Where I looked | How fast | Success? | What was unclear |
|---|---|---|---|---|
| 1 | `protocol/binary/README.md` (all 119 lines), then `SPEC.md` §6 | ~3 min | mostly | "build-only" is never defined, so a v3 owner cannot tell whether the byte layer is usable on their board. The "0x01 = success, 0x00 = failure" summary is contradicted by several normal replies (finding 4). No host-side snippet for sending NULs, but the five-minute version says a terminal cannot do it (finding 17). |
| 2 | README five-minute version → `entering-and-framing.md` → `examples.md` → `commands.md` §Pin state/direction | ~5 min | yes for entry/leave; partial for pin reads | Entry, `BBIO1`, `0x01` ACK and `0x0F` are clear. I could not tell that there is **no read-only five-pin read**; every state/direction byte is a write. Polling with `0x80` would have turned my target supply and pull-ups off (finding 6). The supply bit is not mapped to terminal `W` or to the 3.3 V/5.0 V pins (finding 11). |
| 3 | `commands.md` §Self-test/PWM/ADC/Frequency, `examples.md`, `sub-protocols.md` §SMPS | ~8 min | partial | ADC one-shot and PWM have clean shapes and examples. Self-test has a state machine I had to read twice (after the count, the next non-`0xFF` byte is echoed, not executed). Frequency has a 4-byte shape in prose but no example (finding 12). The SMPS start/stop encoding is clear, but the operand-consumption and stop-on-exit behavior conflict with the linked SMPS page (findings 2 and 3). |
| 4 | `sub-protocols.md` §Entry/§0x0x–0x8x, `commands.md` §Same byte/different layer, `edge-cases.md` §0x0F inside a mode | ~6 min | partial | The entry bytes, identifiers and byte collisions are explained well; `0x0F` per mode is a strong table. But I could not run one byte from this set alone: the only "transfer" is a template with `(mode transfer bytes)`, and the shared `0x1x` shape (leading `0x01`, then per-byte replies) has no literal example (finding 5). The I2C per-byte ACK is inverted from the layer ACK, and I would have coded it wrong (finding 4). |
| 5 | `edge-cases.md` (all entries), `commands.md` §Reserved/PWM, `sub-protocols.md` §SMPS | ~7 min | yes for `0x40`/`0x51`/`0xF1`; partial for the buffer | The `0x40` power trap, the `0x51` direction trap and the `0xF1`-in-BBIO1 trap are all concrete and recoverable. The 4096-byte limit is stated three ways across the tree (finding 1). "0x0F resets the layer completely" is not true for a PWM timer setup (finding 8). |
| 6 | All six binary files; `docs/generic/` for naming | ~5 min | mostly | No TAB characters, no prose line over 80 columns, and every `host ->` comment starts in column 41. No dates, pass counts or bench provenance. The naming problems are "terminal buffer" vs the 128/256 B command buffer (finding 1), the missing v3 `aux` token (finding 10), and "target supply/VREG/VCC" (finding 11). The word "captures" in `examples.md` is the only bench-ish label (finding 14). |
| 7 | README link table, five-minute version, all section references | ~2 min | yes | One click to the command map, one to the examples, one to the edge cases; all link targets exist. Section pointers such as "§Entry bytes" are plain text, not anchors, so they land at the top of a long page (finding 15). The examples row overpromises a transfer (finding 5). No dead ends. |

---

## Findings

### 1. [major] The 4096-byte limit has two sizes and a name that collides with the command buffer

- `edge-cases.md:224-230` — "an I2C write-then-read whose write or read count is larger than the 4096-byte terminal buffer" … "The limit is the same 4096 bytes on v3 and v4."
- `i2c/binary.md:109-113` — "the binary payload buffer is **4096 bytes on v4, 2048 on v3** (not the small command line)" … "A zero write count, or either count over the limit, is answered `0x00` with **no payload consumed and no bus traffic**."
- `core/system.md:95` — "| terminal buffer | 4096 B | 4096 B |"

**Problem.** I cannot tell what the v3 limit is. The page I am reviewing says 4096 on both; the page it links as the I2C command-set owner says 2048 on v3; the core page says 4096 on both. The name is also ambiguous: generic calls the 128 B/256 B line buffer the "terminal command buffer", so "terminal buffer" reads like the command line, not a 4096-byte payload buffer. Whichever page is right, a host cannot size a v3 buffer from the set: if the I2C page is right, this page's "4096 both" is wrong; if this page is right, the I2C page's 2048 is stale.

**Fix.** Pick one name — "binary payload buffer" or "terminal output buffer" — and define it once as distinct from the command buffer. Put the v3/v4 number in one place and link to it rather than restating it. Resolve the 2048/4096 conflict: this page is consistent with core, so either the I2C page has a v3-specific reason that must be stated, or it is stale and should be corrected.

### 2. [major] The SMPS start operand is consumed here and not consumed on the linked SMPS page

- `sub-protocols.md:297-298` — "On a build without SMPS support every request is refused with `0x00`, and a start still consumes its operand byte so the following request stays aligned."
- `smps/binary.md:31-35` — "A start request is also refused with `0x00`, but its operand byte is **not** consumed, so it is read as the next raw-wire request."

**Problem.** These are opposite instructions for the same byte sequence. If I follow the SMPS page on a build without SMPS, the next request is eaten as an operand; if I follow this page, I add the operand and stay aligned. This is exactly the recovery case a host needs. I checked the firmware's no-SMPS branch to settle it: it reads the operand, so this page's "consumed" is the behavior I would implement. The linked page is the one that would desync me.

**Fix.** Correct the SMPS page to "consumed" (or state a build difference if one exists), and add a one-line edge case here: "a start on a build without SMPS eats its operand; the following byte is the next request." Keep the binary page's sentence as the authority.

### 3. [major] `0x0F` stops the SMPS loop here, but the SMPS page calls that unverified

- `sub-protocols.md:299-301` — "Leaving raw wire with `0x00` does **not** stop the SMPS: stop it with `0xF1` while still in raw wire, or leave the binary layer with `0x0F`, whose v4 terminal reset runs the SMPS stop path."
- `edge-cases.md:167-169` — "leave the binary layer with `0x0F` (the v4 terminal reset runs the SMPS stop path)."
- `smps/binary.md:21-24` — "a hardware reset also stops it, but whether the terminal reset that returns from the binary layer stops it is `unverified`."

**Problem.** A stuck SMPS feedback loop is the one case where the reader needs a guaranteed exit. One page promises the terminal reset stops it; the other says maybe. I traced the v4 terminal reset through the board-state reset and it calls the SMPS stop, so the SMPS page's `unverified` looks stale — but as a reader I cannot know which page to trust from the pages themselves.

**Fix.** Decide the v4 behavior and state it in both places. If the board-state reset stops the loop, remove "unverified" from the SMPS page and keep the promise here. If it does not, remove the promise here and give `0xF1` in raw wire as the only recovery. Add the same sentence to the leaving table in `entering-and-framing.md` so the exit story is in one place.

### 4. [major] "0x01 success / 0x00 failure" is not true for several normal replies

- `README.md:29-30` — "Replies are minimal: **`0x01` success, `0x00` failure**, or the data bytes a read command returns."
- `sub-protocols.md:117-124` — "I2C | the slave ACK bit: `0x00` = ACK, `0x01` = NACK"
- `commands.md:186-189` — "The reply is a single byte: the **number of failed checks** (`0x00` = no errors)."
- `commands.md:36-37` — "the five pin levels (`0x00`–`0x1F`)"

**Problem.** The front-door rule reads as universal, but `0x00` is a normal reply for a clean self-test, for an all-low pin level, and for an I2C per-byte ACK — where `0x01` means NACK, the opposite of the layer's own result byte. The detail pages say this, but the README is what a host author codes first. I would have treated the first I2C ACK as a failure.

**Fix.** Reword the README bullet: "`0x01` is usually success and `0x00` usually failure, but a reply can be a count, a level or a bus bit; use the command's reply shape. In the `0x1x` I2C group the per-byte bit is `0x00` = ACK, `0x01` = NACK." Add a compact "reply shapes" table to `commands.md` — result byte, data word, count, level, stream, leading-`0x01`-then-per-byte — and link it from the README.

### 5. [major] The README promises a worked "entry → configure → transfer → exit", but the examples page has no transfer

- `README.md:21` — "| [examples.md](examples.md) | worked entry → configure → transfer → exit |"
- `examples.md:165-169` — "This is a template, not a literal capture. The five nibble-group modes (`0x01`–`0x05`) follow it… The canonical byte-by-byte SPI frame is in [the SPI binary page](../spi/binary.md) §Worked exchange."

**Problem.** The closest thing in this set is the "Mode skeleton" with `(shared configure bytes)` and `(mode transfer bytes)`. That is a reasonable ownership split — each mode page owns its command set — but it is not a worked transfer, and the shared `0x1x` group (leading `0x01`, then per-byte replies) has no literal example anywhere in this set. To run one byte I had to leave the set for the mode page. This is also where the I2C ACK inversion would bite, because the shape `0x01` + per-byte ACKs is not shown.

**Fix.** Either add one literal shared-group exchange (for example an I2C `0x11 0xAA 0xBB` → `0x01 0x00 0x00`, two bytes, both ACKed, labeled as an example) or change the README row to "BBIO1 worked exchanges plus a sub-protocol skeleton" and add a "pick your mode" table at the top of `examples.md`. While there, give the `0x3x` group a literal shape too: "replies `0x01` before and after" (`sub-protocols.md:142-146`) left me unsure whether I read one success byte or two.

### 6. [major] There is no read-only five-pin read, but the text reads as if there is

- `README.md:84-86` — "The pin levels themselves are reported by every state/direction byte."
- `commands.md:45-48` — "The lower seven bits drive the pins and peripherals; the reply is the five pin levels (`0x00`–`0x1F`) after the write."
- `entering-and-framing.md:159-161` — "There is **no read-back command**: nothing reports the current pin directions, speed, output type or wire count. The pin levels are reported by each state/direction byte"

**Problem.** "Reported by every state/direction byte" sounds like a read. It is not: a state byte writes all five latches and both the supply and pull-up bits before returning levels, and a direction byte rewrites all five directions. If I poll with `0x80` to watch a line, I turn the target supply and pull-ups off and drive every latch low. I would have done exactly that on my first session.

**Fix.** Add a short "Reading pins" paragraph to `commands.md` and a limit bullet to the README: "There is no read-only five-pin command. A state byte is a write; a direction byte rewrites all directions. For a non-destructive sample of the selected AUX pin, use `0x70 <edge> 0x00 0x00` (timeout 0), and make the pin an input first. Do not poll with `0x80`." If the firmware has a safe way to sample the other four pins, say it; if not, say they have none.

### 7. [minor] "build-only" is used everywhere and never defined in this set

- `README.md:3` — "**Status:** v4 documented; v3 is build-only and not verified."
- `README.md:68-76`, `commands.md:339-346`, `sub-protocols.md:333-340`, `entering-and-framing.md:194-201` — "present (build-only)"

**Problem.** A v3 owner's first question is "does this work on my board?" "Present (build-only)" can mean "compiled into the v3 build but untested" or "only a build artifact." The caveat is there, but the term is jargon and it repeats in every table.

**Fix.** Define it once in the README: "build-only = present in the v3 build, not tested on v3 hardware." Use "present, unverified on v3" in the tables, or keep "build-only" and link the definition. The repetition then becomes a short, consistent caveat instead of a new term to decode each time.

### 8. [minor] "0x0F resets the layer completely" is not true for a running PWM timer

- `edge-cases.md:78-79` — "send `0xC0` for the supply, or `0x5F` to return all lines to inputs. `0x0F` resets the layer completely."
- `commands.md:238-244` — "`0x0F` does not clear the PWM timer setup either; on v4 the terminal reset detaches the AUX output source… Send `0x13` before leaving if you want the module idle."

**Problem.** The recovery for the `0x40` trap says `0x0F` resets completely; a PWM timer setup survives the v4 terminal reset. A reader may rely on `0x0F` to leave the module idle and then find the timer still running.

**Fix.** Say "`0x0F` resets the pins, supply and pull-ups; send `0x13` first if a PWM timer must be stopped." Add the PWM caveat to the leaving table in `entering-and-framing.md`, not only to the PWM section.

### 9. [minor] The `0xF1` latch description is looser than the edge case's own decode

- `sub-protocols.md:302-304` — "a stop byte sent after leaving raw wire switches the target supply and pull-ups on and presets the pin latches high"
- `edge-cases.md:250-253` — "`0xF1` is a pin state byte: target supply on, pull-ups on, AUX and CS latched high, MOSI/CLK/MISO latched low."

**Problem.** "Presets the pin latches high" reads as all five latches; `0xF1` sets AUX and CS high and the other three low. The detailed edge case is right, but the shared-group page is the one a raw-wire reader hits first.

**Fix.** Use the exact decode in both: "presets the AUX and CS latches high (MOSI/CLK/MISO low)," or link to the edge case instead of restating it.

### 10. [minor] The v3 AUX token `aux` is missing from the mapping

- `README.md:59-60` — "The terminal's selection tokens are `A0` `CS` `A1` `A2`: `A0` is the pin printed `AUX`, `A1` is `AUX1`, `A2` is `AUX2` (v4 for the last two)."
- `generic/commands.md:125-135` — "| `A0` (`aux` on v3) |"

**Problem.** On v3 the help and selection use `aux/CS`; `A0` is the v4 form. A v3 reader looking for `A0` in the help screen will not find it, and the binary README's mapping omits the v3 form even though it explains the v4-only AUX1/AUX2.

**Fix.** Write "`A0` (`aux` on v3), `CS`, and `A1`/`A2` (v4)" and keep the pin-name mapping that follows.

### 11. [minor] "Target supply (VREG)" is not mapped to terminal `W`, and "the VCC pin" is not a header name

- `entering-and-framing.md:95-98` — "The pull-up resistors and the target supply are different circuits. The resistors tie the I/O lines to Vpullup; the supply puts the regulated rail on the VCC pin."
- `generic/README.md:149-150` — "`W` enables the on-board regulator and puts the 3.3 V and 5.0 V rails on their header pins"

**Problem.** I could not tell whether the binary supply bit is the same element as terminal `W`, whether it brings up both header rails or one, and what "the VCC pin" names. The `v` table has `5.0V` and `3.3V` columns, not `VCC`. The binary bit also differs from `W` — no rail check, no clutch setup — but the supply row of the wiring table does not say which rails come up.

**Fix.** Add a names-table row: "target supply (VREG) — the on-board regulator, the same element as terminal `W`; state which header rails it brings up (the `v` table shows 5.0V and 3.3V, not VCC); the binary bit does no rail check and runs no bus setup." Replace "the VCC pin" with "the 3.3 V and 5.0 V header pins" (or "the target's VCC") once you know which one the sentence means.

### 12. [minor] The 4-byte frequency reply has no worked example

- `commands.md:266-273` — "returns 4 bytes, most significant byte first, in **Hz**… Allow about one second above about 65.28 kHz and about two seconds when the rerun runs."

**Problem.** The shape is prose only; I had to trust the byte order and could not see a 4-byte value. The AUX page adds that the binary reading is a multiple of 256 Hz above ~65.28 kHz; that is absent here, so a host author may expect 1 Hz resolution at high frequencies.

**Fix.** Add a 3-line exchange, for example `0x16` → `0x00 0x00 0x27 0x10` = 10,000 Hz, and one sentence: "above about 65.28 kHz the result is a multiple of 256 Hz."

### 13. [minor] The README entry-byte bullet does not warn that `0x08`/`0x09` have no identifier

- `README.md:37-40` — "`0x08`/`0x09` AVRISP-MKII (v4), `0x18` XSVF (v4). Identifiers and owners are in [sub-protocols.md](sub-protocols.md) §Entry bytes."
- `sub-protocols.md:25-26`, `34-39` — "AVRISP-MKII … | none" … "**no command loop and no identifier**"

**Problem.** Every other entry prints an identifier; a host that sends `0x08` and waits for one hangs until the ~5 s timeout, then sees `BBIO1`. That is a bad first experience with a listed entry byte. The detail is one click away, but the bullet is what a reader scans.

**Fix.** Add "(no identifier; hands the link to the personality, returns to `BBIO1` after about 5 s)" to the bullet or a footnote.

### 14. [minor] "captures" and `x20` are notation labels that can be mistaken for protocol or bench terms

- `examples.md:9-14` — "## Reading the captures" … "`x20` means twenty copies."
- `examples.md:165` — "This is a template, not a literal capture."

**Problem.** The page's own disclaimer says the blocks are format illustrations, which is exactly right, but the labels fight it: "captures" suggests bench traces, and `x20` is not a byte sequence. A reader skimming `host -> 0x00 x20` could send the ASCII `x20`.

**Fix.** Rename to "Reading the exchanges"; say "`x20` is this page's shorthand for twenty copies, not bytes to send." Keep the "format illustration / read values are examples" disclaimer — it is the right tone.

### 15. [minor] Section references are not anchors, and one section name is shortened

- `README.md:39-40` — "[sub-protocols.md](sub-protocols.md) §Entry bytes"
- `README.md:100-101` — "[commands.md](../../generic/commands.md) §Leaving the CLI"
- `entering-and-framing.md:32-33`, `edge-cases.md:27-28` — same short form

**Problem.** The link table is fast, but a section pointer lands at the top of a 349-line page and the reader has to scroll. "§Leaving the CLI" is also not the actual heading ("Leaving the CLI — binary and SUMP entry"), so I was not sure I was in the right section.

**Fix.** Link the anchors (`sub-protocols.md#entry-bytes`, `generic/commands.md#leaving-the-cli--binary-and-sump-entry`) and use the exact heading text. This is a small change that removes most of the doubling back.

### 16. [minor] Two cross-mode table details are less precise than the rest

- `sub-protocols.md:74-78` — "The AUX target selection … persists into BBIO1 and into most sub-protocols"
- `sub-protocols.md:216-222` — the `0x6x` table includes "| 1-Wire | `0x2x` low two bits | 0 standard, 1 overdrive |"

**Problem.** "Most" makes me check each mode; `commands.md` says binary I2C is the one that resets the AUX target. The `0x6x` table also has a 1-Wire row for a group 1-Wire does not have, which made me re-read the group matrix.

**Fix.** Say "persists into every mode except binary I2C" (if that is true), and move the 1-Wire speed note to the `0x2x` table or mark the row "not a `0x6x` command — see `0x2x`."

### 17. [minor] No minimal host recipe for the entry sequence

- `README.md:26-28` — "Enter from the terminal prompt with **20 consecutive `0x00`** bytes; the layer replies `BBIO1` (five bytes, no line ending). A host program sends them — a terminal cannot type NUL."
- `entering-and-framing.md:23-27` — "Send **20 consecutive `0x00`** bytes. The Bus Pirate replies `BBIO1`… Wait for the five identifier bytes before sending the first request."

**Problem.** The page says a host program sends the NULs, but it does not show one. As a reader with a board and a serial port, I know the bytes but not the first ten lines of code: open the port, write 20 zero bytes, read exactly five bytes, compare to `BBIO1`, then send `0x0F` to get out. The examples are byte tables, not host code. That is the "hello world" gap for a byte layer.

**Fix.** Add a short language-neutral pseudo-code block (or a Python/pyserial snippet) to `examples.md` §Enter, identify, leave. Keep it to the entry, the five-byte compare and the exit; no library. A reader can adapt it to any serial API.

---

## What to keep

- **The opening and the names table.** "Byte-oriented control layer… single request bytes and raw bytes instead of terminal commands" is the right one-paragraph answer to "what is BBIO?" The `Vpullup` (`Vpu`, `VPU`) and `AUX`/`A0` mappings are the kind of explicit mapping most docs omit.
- **The five-minute version.** Entry via 20 NULs, the `BBIO1` reply, the `1xxxxxxx`/`010xxxxx` class split, the entry-byte list and the wiring headline are all in one screen. I did not need to open a detail page to know where to start.
- **"Same byte, different layer or mode."** This is the clearest part of the set. The `0x40` "all outputs", `0x51` direction, `0xA0` PIC, `0xF0` SMPS and `0x0F` per-mode collisions are all in one place. The repetition of the `0x40`/`0x51` traps in `edge-cases.md` is justified.
- **The pin-control examples.** `0x5B` = CLK output, `0x84` = CLK high, `0x80` = all latches low, with the "set means input" warning and the "actual levels, not latches" caveat. I understood the direction convention on the first read.
- **The exchange formatting.** Comments at a fixed column (41) in every `host ->` block, `host ->`/`host <-` consistently defined, and "read values are examples; the reply shape is the contract." This is exactly what a host author needs.
- **The terminal-vs-binary table** in `sub-protocols.md`. "Same bus, different command set" and "each mode applies its own defaults" is the distinction I was missing before I read it.
- **The `0x0F` inside-a-mode table** and the "send `0x00` to get back to `BBIO1`, then `0x0F`" recovery. I would have started with `0x0F` and been in the I2C sniffer.
- **The edge-case format.** Trigger → symptom → behaviour → recovery, especially the `0x40` power trap, the `0xF1`-in-BBIO1 trap and the "wait frame with invalid selector still consumes the timeout" entry. Those are concrete and recoverable.

---

## Structural opinion

The README-as-map plus five parallel pages is the right shape for a byte layer. The split matches how I used it: enter (`entering-and-framing`), look up a byte (`commands`), see the shared groups (`sub-protocols`), try an exchange (`examples`), debug (`edge-cases`). I never had to read a 600-line README, and the set is small enough to scan.

The one structural weakness is the **shared-group material is all table, no exchange**. `sub-protocols.md` owns the `0x0x`–`0xFx` semantics, but the only literal blocks are BBIO1-only. That makes the set feel like a reference manual for a protocol I already know, rather than a guide for a first host. One literal shared-group exchange — I2C is the best choice because it shows the leading `0x01` and the inverted ACK bit — would fix the biggest usability gap without stepping on the per-mode command sets.

The **`0x0F`/leaving story is split three ways**: `commands.md` §Reset, `entering-and-framing.md` §Leaving, and `edge-cases.md` §`0x0F` inside a mode. The happy path belongs in one place (the leaving table), with the PWM and SMPS caveats attached; the per-mode trap table can stay in edge cases and link back. Right now I had to read all three to know whether `0x0F` stops the SMPS or the PWM.

The **availability caveat repeats in every file**. Define "build-only" once and let the tables carry a short "v3 unverified" note; the repetition is noise without adding confidence. The same goes for "no read-back" — one section in `commands.md` with the "reading pins" caveat would be stronger than the same sentence in three places.

Concrete proposals:

1. Add a **"reply shapes" table** near the top of `commands.md`: result byte, data word, count, level, stream, leading-`0x01`-then-per-byte, and the I2C inverted ACK. Link it from the README.
2. Add a **literal shared-group exchange** to `examples.md` (one I2C write of two bytes, with `0x01 0x00 0x00` and a note on which byte is which).
3. Add a **"Reading pins"** subsection to `commands.md` and a limit bullet in the README.
4. **Consolidate the leaving semantics** in `entering-and-framing.md` §Leaving, including PWM and SMPS; have `commands.md` and `edge-cases.md` link to it.
5. **Use anchors** for the section pointers in the README and the "Related docs" lines.
6. Keep the edge-case bullets; they are readable. Add a one-line **trigger index** at the top of `edge-cases.md` for scanning.

---

## Unknowns

- **No hardware.** I did not verify v3 behavior, the 20-NUL timing, the `0x70` wait on an output pin, or the SMPS electrical behavior. The docs themselves say v3 is unverified; I cannot improve on that from a reader's chair.
- **The v3 2048/4096 question.** I saw the conflict and checked `core/system.md`, but I could not resolve it from the user pages alone. The I2C page's 2048 may have a v3-specific reason that is not stated.
- **SPI-specific claims.** Per the brief I did not audit `docs/protocol/spi/`. I checked that the link resolves, but I did not compare its `0x0F` refusal, `0x8x` bits or `0x1x` frame against this set. If the two disagree, I would not have caught it.
- **AVRISP re-enumeration.** I cannot tell what a host actually sees on the USB route before the `BBIO1` re-announcement; the docs say "re-enumerates first," but the host-side sequence is not something I can judge without the board.
- **The SMPS unit and useful range.** The docs say the unit is undefined and the electrical behavior is unverified; I have no way to judge the 0x200–0x526 recommendation or the 256 Hz frequency multiple.
- **The terminal rendering of `v`.** I only checked the transcribed table; I did not see the actual tab-separated output, so I cannot judge whether the columns line up on a real terminal.
- **The v3 `aux` token.** I relied on `generic/commands.md`'s note that v3 prints `aux/CS`; I did not see a v3 help screen.
