# Binary docs — round-1 re-audit

**Verdict: 17 of 17 in-set items filled in full.**

Scope: only the 17 findings from `feedback-binary-round1.md`, checked against the current six files in `docs/protocol/binary/`. No fresh full review. For the three cross-set items (1, 2, 3) I judged the binary page only; the linked i2c/smps pages are noted separately for the main session.

| # | Status | Evidence in the current docs | Remainder |
|---:|---|---|---|
| 1 | Filled in full | `edge-cases.md:237-238`: "an I2C write-then-read whose write or read count is larger than the 4096-byte terminal output buffer" and `:243-244`: "The limit is 4096 bytes on both v3 and v4.  This is the terminal output buffer, not the 128/256-byte command line." | The binary page is correct. Cross-set `i2c/binary.md:109` still says "4096 bytes on v4, 2048 on v3"; recorded for the main session. The edge case does not link the I2C page, but the README mode table does. |
| 2 | Filled in full | `sub-protocols.md:299-300`: "a start still consumes its operand byte so the following request stays aligned." New `edge-cases.md:178-180`: "the no-SMPS start path still reads its operand before it reports the refusal, so the following byte is the next command." | In-set full. Cross-set `capabilities/smps/binary.md:33-34` still says the operand is "**not** consumed"; recorded for the main session. |
| 3 | Filled in full | `entering-and-framing.md:188-190`: "On an SMPS-enabled build it also stops the SMPS feedback loop." `commands.md:209-210`: "supplies off, an SMPS feedback loop stopped." `sub-protocols.md:301-303`: "on v4 the terminal reset stops the loop." | In-set full. Cross-set `capabilities/smps/binary.md:22-23` still calls the terminal-reset stop "`unverified`"; recorded for the main session. |
| 4 | Filled in full | `README.md:30-33`: "Replies are minimal, but **`0x01` is not always success and `0x00` is not always failure**: a reply can be a result byte, a data word, a count, a pin level or a bus ACK. Use the command's reply shape." `commands.md:44-62` adds the reply-shapes table; `entering-and-framing.md:117-120` calls the old table "the usual case, not a universal rule." | None for the original issue. New minor N3: the table calls itself the full set but omits `0x70`'s result+level pair. |
| 5 | Filled in full | `README.md:22`: "worked BBIO1 and I2C exchanges, mode skeleton." `examples.md:176-208` adds the I2C exchange; `:189-192`: "`0x11 0xAA 0xBB` bulk write: n = 1, so two bytes / 0x01 command accepted / 0x00 per-byte ACK, not a failure / 0x00 per-byte ACK." | None. The I2C start/stop bytes (`0x02`/`0x03`) and the per-byte ACK shape match `i2c/binary.md:33-42`. |
| 6 | Filled in full | `commands.md:66-69`: "There is **no read-only five-pin command**.  Every state byte writes all five latches plus the supply and pull-up bits, then reports the levels; every direction byte rewrites all five directions, then reports the levels." `README.md:92-95` and `entering-and-framing.md:168-172` repeat the rule. | Original issue full. New minor N1: the README summary says a pin level costs "a rewrite of all five latches" for state **and** direction bytes; only state bytes rewrite latches. |
| 7 | Filled in full | `README.md:3-4`: "v3 is unverified (built for v3, not tested on v3 hardware)." Availability rows now read "present, unverified" (`README.md:74-79`, `commands.md:390-397`, `sub-protocols.md:336-337`); "build-only" is gone. | None. |
| 8 | Filled in full | `edge-cases.md:79-81`: "`0x0F` resets the pins, the supply and the pull-ups; send `0x13` first if a PWM timer must stop." `entering-and-framing.md:189-190`: "It is not a substitute for `0x13`: the PWM timer is left running." `commands.md:282-286` repeats it. | None. |
| 9 | Filled in full | `sub-protocols.md:304-307`: "presets the **AUX and CS latches high** (MOSI, CLK and MISO latch low)." | None. Matches `edge-cases.md:264-266`. |
| 10 | Filled in full | `README.md:65-68`: "The terminal's selection tokens are `A0`, `CS`, `A1` and `A2` (v3 prints `aux` for `A0`)." `commands.md:146-147` repeats the v3 form. | None. |
| 11 | Filled in full | `README.md:62` (`VREG`): "the on-board regulator that feeds the target supply; terminal `W` switches the same element." `entering-and-framing.md:96-103`: "the supply switches the on-board regulator, the same element as terminal `W`, which puts the 3.3 V and 5.0 V rails on their header pins" and "Unlike `W`, the binary supply bit runs no rail check and no bus setup." | None. "VCC pin" is gone. |
| 12 | Filled in full | `commands.md:312-319`: "returns 4 bytes… Above about 65.5 kHz the first count is larger, so the single prescaled gate is used and the result is a multiple of 256 Hz." `:321-324` adds the `0x16` → `0x00 0x00 0x27 0x10` (10000 Hz) exchange. | In-set full. Extra cross-set item: `capabilities/auxiliary/frequency.md:128-130` still says 65.28 kHz; recorded for the main session. |
| 13 | Filled in full | `README.md:40-45`: "Every entry prints an identifier except `0x08`/`0x09`, which have none and return to `BBIO1` after about five seconds of host silence." | None. |
| 14 | Filled in full | `examples.md:9-15`: "## Reading the exchanges" and "`x20` is this page's shorthand for twenty copies, not bytes to send." `:212`: "This is a template, not a literal exchange." | None. |
| 15 | Filled in full | Anchors are now used and resolve: `README.md:33` (`commands.md#reply-shapes`), `:45` (`sub-protocols.md#entry-bytes`), `:95` (`commands.md#reading-pins`), `:109` (generic `#leaving-the-cli--binary-and-sump-entry`); `commands.md:77`, `:213`, `:238`; `examples.md:174`, `:208`; `sub-protocols.md:79`, `:93`, `:238`. | None for the anchor issue. New minor N2: several inline links now push prose past 80 columns. |
| 16 | Filled in full | `sub-protocols.md:76-79`: "persists into BBIO1 and into every sub-protocol except binary I2C, which forces it back to `AUX`." The `0x6x` table (`:218-223`) no longer has a 1-Wire row. | None. |
| 17 | Filled in full | `examples.md:34-44`: "A host program runs the same sequence in five steps: 1. Open the serial port… 2. Write twenty `0x00` bytes. 3. Read exactly five bytes and compare them with `BBIO1`. 4. Send requests, reading exactly the reply length each command defines. 5. Send `0x0F`, read the `0x01`, then read text until the banner and the `HiZ>` prompt appear." | None required. It is a prose recipe rather than a code block; the sequence and lengths are clear. |

---

## Cross-set status (not counted against this set)

| Page | Current text | Status |
|---|---|---|
| `docs/protocol/i2c/binary.md:109` | "the binary payload buffer is **4096 bytes on v4, 2048 on v3**" | Still contradicts the binary page; recorded by the implementer, main-session item. |
| `docs/capabilities/smps/binary.md:33-34` | "its operand byte is **not** consumed, so it is read as the next raw-wire request" | Still contradicts finding 2; recorded, main-session item. |
| `docs/capabilities/smps/binary.md:22-23` | "whether the terminal reset that returns from the binary layer stops it is `unverified`" | Still contradicts finding 3; recorded, main-session item. |
| `docs/capabilities/auxiliary/frequency.md:128-130` | "255 or fewer edges.  Above about 65.28 kHz…" | Extra cross-set inconsistency with the corrected 65.5 kHz in `commands.md`; recorded as an extra item. |

The binary pages are correct on all four points; the linked pages are the reason these remain visible to a reader.

## Presentation check

- **TAB characters:** none in any of the six files.
- **Capture alignment:** every `host ->` / `host <-` comment starts in column 41 in `examples.md` and in the new `0x16` block in `commands.md:322-323`.
- **Menus / option lists:** the new reply-shapes and byte tables render as tables; no untabled option list was introduced.
- **~80-column wraps:** eight non-table prose lines are over 80 columns. The worst is `commands.md:26` at 100 columns (a long `edge-cases.md#…` link); `README.md:109`, `edge-cases.md:28`, `entering-and-framing.md:33` and `:227` are 89; `examples.md:179` is 81; `sub-protocols.md:93` and `:238` are 81.
- **Bench prose / dates / tested-result prose:** no dates, pass counts, capture provenance, evidence sections or internal log names. The only test-status wording is the v3 caveat "unverified (built for v3, not tested on v3 hardware)" and the table note "have not been tested on v3 hardware"; that is the allowed short caveat, not a bench result.
- **Naming:** "terminal output buffer" is now distinct from the "128/256-byte command line"; `VREG` is mapped to `W`; v3 `aux` is mapped to `A0`. No new mismatch found.

## New problems (minor, from the fixes)

N1. **[minor]** `README.md:92-94` — "every state or direction byte is a write, so a pin level costs a rewrite of all five latches (and, for a state byte, the supply and pull-up bits)." A direction byte rewrites all five **directions**, not latches; `commands.md:67-68` states it correctly. Fix: "…a rewrite of all five latches and the supply/pull-up bits (state byte), or of all five directions (direction byte)."

N2. **[minor]** Presentation wrap — `commands.md:26` is 100 columns and four other prose lines are 89 columns because the new anchors are inline. Fix: break the line after "See" or use reference-style links; no content change.

N3. **[minor]** `entering-and-framing.md:119-120` — "The full set of shapes is in [commands.md](commands.md#reply-shapes)." The table at `commands.md:50-58` does not include `0x70`'s two-byte result+level reply, which `commands.md:358-360` defines. Fix: add a "result byte + pin level" row, or change "full set" to "the common shapes."

---

**Verdict: 17 of 17 in-set items filled in full.** No major in-set issue remains; the open items are the recorded cross-set pages and the three minor nits above.
