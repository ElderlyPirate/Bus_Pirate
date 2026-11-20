# SUMP set — fresh-reader feedback (round 1)

I read this as an engineer with a v3 and a v4, no prior SUMP context, and no
client open yet. I read the README first, then followed its links. No hardware
was available, so this is a docs-and-arithmetic read, not a bench report.

## Verdict

**yes with fixes.** The set is well organized and the core capture path is
usable, but several protocol facts are wrong in ways that change the bytes a
reader sends (tier-cliff counts, the default divider, which channel bits are
dropped) and two recovery paths are incomplete (exit during a long command or a
capture; abort timing in the fast post-trigger window). Fix those and this is a
strong yes.

## Task log

| # | Path I took | Time | Result and friction |
|---|---|---|---|
| 1 | README, all 115 lines. Then `client.md` for the profile and `examples.md` for a capture. | ~2 min | **Mostly successful.** I could say what SUMP is, that v4 is current and v3 is build-only, that the client is sigrok/PulseView, and where the entry detail lives. Friction: no link to the client or a first-capture recipe, so "where do I start" ends at a software install I have to guess. A v3 owner also has to decide whether "build-only" means "usable" or "not supported". |
| 2 | README five-minute bullets 1–2, then `entry.md` entry contract, boundary rule, recovery table. | ~1 min | **Successful.** The 5–19 NUL + STX path, the 20-NUL `BBIO1` boundary, and the `0x00` then `0x0F` recovery are all findable and concrete. Friction: the README calls the 20-NUL case "the one mistake" but does not link its recovery; `entry.md:161` mixes "answered BBIO1" with "answered a sub-mode"; and `entry.md:134` says exit bytes work only while waiting for a command, which is not true during a capture. |
| 3 | `client.md` workflow, `rates.md` tables, `depth.md` tiers, `trigger.md` mask/position, `commands.md` encodings. | ~5 min | **Partial.** The round-rate table and the `(raw+1)×4` encodings got me a 1 MHz, 4096-sample, CS-trigger request without another reference. But an arbitrary rate has no worked divider (`300 kHz` gives the achieved rate but not `x=332`); the default divider row is a timer period, not a divider; the tier-cliff counts are not expressible; and "low channel bits are dropped" is backwards. |
| 4 | `readout.md`, then `depth.md` ring arithmetic and channel bits. | ~3 min | **Successful on the main path.** Reply length, newest-first order, padding-first, and the recorded/unrecorded/overwritten table are clear. The 8191 example is impossible as written. The "padding at the newest end" is surprising but stated three times, so I trusted it. I wanted one concrete byte example for "unrecorded bits vs padding" to be sure I would recognise it in a trace. |
| 5 | `edge-cases.md`, then `trigger.md` missing-trigger, `rates.md` rollover, `readout.md` aborts. | ~4 min | **Partial.** No-trigger, back-pressure, trigger-on-a-dropped-channel and the wrong layer are clear. But the rollover row is only in `rates.md`, the RX-overflow row has no consequence or recovery, the pre-fill blind window is much longer at slow rates than the text suggests, and the fast post-trigger abort timing contradicts itself across three pages. |
| 6 | Grep and line checks over all ten files; UTF-8 validation; relative-link and anchor check; raw capture-column check. | ~3 min | **No TABs, no trailing spaces, no broken fences, no dates or bench prose, all links and anchors resolve.** Capture comments are aligned at column 31 (`examples.md`) and 32 (`entry.md`, `readout.md`). No dense bullet chains or unreadable option lists. One naming mismatch (`ancillary version` vs `ancillary marker`); several repeated v3 caveats. A couple of prose lines are 81 characters (`client.md:66`, `rates.md:10`), not broken wraps. |
| 7 | README as a navigation test: entry, commands, rates, depth, trigger, readout. | <30 s total | **Fast.** Entry and commands are one or two clicks from the top; rates, depth, trigger and readout are each one link from the file table. No dead links. The only doubling back was to `commands.md` to reconcile the default divider and to `depth.md` to check the 8191 example. Recovery and client acquisition are the two dead ends. |

## Findings

### 1. [severity: major] The tier-cliff table asks for read counts the protocol cannot express

**Where:** `docs/protocol/sump/depth.md:143` and `:153`, `docs/protocol/sump/readout.md:72`, `docs/protocol/sump/edge-cases.md:45`.

**Quote:** `depth.md:143` — `| 8191 | 4096 | 4095 | all |`; `readout.md:72` — `8191, for example, captures 4096 samples and pads the other 4095 bytes.`; `edge-cases.md:45` — `| a 8191 count at 1 MHz | 4096 real samples and 4095 padding bytes | the tier is chosen from the request; use 8192 to reach the packed tier |`

**Why it hurts:** `commands.md:68-70` says the read count is `(raw + 1) × 4` and that "Both are therefore always multiples of four". 8191, 4095, 4097, 16 383, 32 767 and 65 535 are not multiples of four. A client that types 8191 will reject it or round it — down to 8188 gives a 4096 capture with 4092 padding, up to 8192 gives the packed tier instead — so the described 4096/4095 split is not what happens either way. The one table that exists to make the tier cliff exact is wrong at every cliff, and the 4095/4097 rows cannot be produced at all.

**Fix:** Replace the table with expressible counts: 4092/4096, 8188/8192, 16 380/16 384, 32 764/32 768, 65 532/262 144. Add one sentence: "A client UI may round a typed count to a multiple of four; check the count it actually sends." Update the 8191 examples in `readout.md` and `edge-cases.md` to 8188.

### 2. [severity: major] The default "divider" is a timer period; sending it as a divider gives ~62 kHz

**Where:** `docs/protocol/sump/rates.md:98-99`, `docs/protocol/sump/commands.md:157`, `docs/protocol/sump/edge-cases.md:35`.

**Quote:** `rates.md:98-99` — `After entry, 0x00 or any completed capture, the divider is the 1601-tick default: about 10 kHz.`; `commands.md:157` — `| divider | 1601 timer ticks, about 10 kHz |`; `edge-cases.md:35` — `every reset restores the 1601-tick default; send 0x80 before every capture`.

**Why it hurts:** 1601 is the timer period (`PR + 1`), not the divider. If a reader sends `0x80` with `x=1601`, the page's own formula gives `floor(4×1601÷25)+1 = 257` ticks and `16 MHz ÷ 257 = 62.26 kHz`, not 10 kHz. The default is a timer period of 1601 ticks (about 9.994 kHz); the equivalent divider is about 10000. This is a 6× rate error if the row is followed literally.

**Fix:** Say "no divider is sent; the default timer period is 1601 ticks (about 9.994 kHz)". Keep `x=9999` (1600 ticks, 10 kHz) and add `x=10000` (1601 ticks, 9.994 kHz) to the rate table. Rename the `divider` row to `timer period / divider (not set)`.

### 3. [severity: major] "Low channel bits are dropped" is backwards

**Where:** `docs/protocol/sump/client.md:110-111`, `docs/protocol/sump/trigger.md:130-132`; the correct table is `docs/protocol/sump/depth.md:55`.

**Quote:** `client.md:110-111` — `The reverse also happens at 8192 samples and above, where low channel bits are dropped to gain depth.`; `trigger.md:130-132` — `A capture at 8192 samples and above drops low channel bits ([depth.md](depth.md#depth-tiers)), but a trigger on a dropped channel still fires.`; `depth.md:55` — `"Bits kept" are the low channel bits of the table above.`

**Why it hurts:** The depth table keeps bits 0–3 at 8192, 0–1 at 16 384 and bit 0 at 32 768. The dropped bits are the **high** bits. On v4 at 8192, AUX2 (bit 0), MOSI, CLK and MISO are kept; CS (bit 4), AUX (bit 5) and AUX1 (bit 7) are dropped. The two sentences say the opposite and will make a reader choose the wrong trigger or the wrong depth for a channel they care about.

**Fix:** Change both to "high channel bits are dropped", and add the concrete v4 example: "at 8192 v4 keeps bits 0–3 (AUX2/MOSI/CLK/MISO); CS is bit 4 and is dropped."

### 4. [severity: major] Exit bytes mid-command are not "ignored", and they do work during a capture

**Where:** `docs/protocol/sump/entry.md:134-137`.

**Quote:** `The exit bytes are commands only when the analyzer is waiting for a command. In the middle of a five-byte command they are parameter data; the command then completes and is ignored, or the one-second gap described in [commands.md](commands.md#parser-rules) heals the parser.`

**Why it hurts:** For a known long command, the exit byte becomes a parameter and the command **takes effect**. Sending `80 0F ...`, for example, sets the divider low byte to `0x0F`; it is not ignored. Also, while a capture is running, an exit byte does abort it and is then handled as a command — `readout.md:103-110` and `trigger.md:119-126` say so. The absolute "only when the analyzer is waiting for a command" is false during a capture and will make a reader think `#` is ignored when it is the fastest way out.

**Fix:** Rewrite as two cases: "While a capture is running, any byte aborts it and is then handled as a command; `#` stops the wait and leaves SUMP. Inside a five-byte command, an exit byte is a parameter: the command completes and **takes effect** (unless its opcode is unknown). Use the one-second gap or five `0x00` resets to discard a partial command."

### 5. [severity: major] The trigger pre-fill blind window is much longer than "16 samples" suggests

**Where:** `docs/protocol/sump/trigger.md:85-87` and `:121`; the burst rule is `docs/protocol/sump/rates.md:109-110`.

**Quote:** `trigger.md:85-87` — `The fill runs in whole bursts of about 4 ms worth of samples, so even a zero-length pre-trigger part costs one burst (16 samples at the slowest rates): an edge during that first burst is ignored and the capture waits for the next one.`; `trigger.md:121` — `| the edge arrives before the pre-fill | ignored; the next edge is accepted | choose a signal that keeps producing edges |`

**Why it hurts:** The burst is `max(4 ms, 16 samples)`. At 1 MHz that is 4000 samples (4 ms), but at 1 kHz it is 16 samples = **16 ms**, at 100 Hz 160 ms, and at 5.96 Hz about **2.7 s**. A reader capturing a slow one-shot event can lose the event even though the trigger is armed. "Choose a signal that keeps producing edges" is not a recovery for a one-shot edge; it asks the user to change the signal.

**Fix:** Add a small table: rate, pre-fill samples, time before arming (1 MHz / 4000 / 4 ms; 100 kHz / 400 / 4 ms; 10 kHz / 40 / 4 ms; 1 kHz / 16 / 16 ms; 100 Hz / 16 / 160 ms; 5.96 Hz / 16 / 2.7 s). Say plainly: an edge inside the blind window is lost; arm the trigger after that time, use a free-run capture, or accept the loss.

### 6. [severity: major] A byte in the fast post-trigger window is handled after the readout, not "after it"

**Where:** `docs/protocol/sump/trigger.md:108-110`, `docs/protocol/sump/edge-cases.md:90`; the correct statement is `docs/protocol/sump/readout.md:105`.

**Quote:** `trigger.md:109-110` — `A byte sent during the wait aborts the capture; a byte sent during the short post-trigger window is handled after it.`; `edge-cases.md:90` — `| a byte sent during the fast post-trigger window | the window completes, then the byte is handled | the window is at most about 1 ms |`; `readout.md:105` — `| during the short fast post-trigger window | not polled; the window completes and the byte is handled after the readout |`

**Why it hurts:** "After it" and "then" hide the readout. The window is at most ~1 ms on v4, but **2.05 ms on v3 at 2 MHz** (4096 samples; `rates.md:87`), and the readout can add about 0.36 s for 4096 bytes at 115200. A user sending `#` to abort during that window will wait for the whole readout, not for the window. The three pages disagree on the delay.

**Fix:** Use `readout.md`'s wording in all three: "not polled until the window and readout finish"; add the v3 2.05 ms window and the readout time. Change `edge-cases.md` "at most about 1 ms" to "at most ~1 ms on v4, ~2.05 ms on v3 2 MHz, then the readout".

### 7. [severity: minor] "Round rates land exactly" and "nearest engine" are both false at the edges

**Where:** `docs/protocol/sump/rates.md:40`, `:45`, `:20-22`.

**Quote:** `rates.md:40` — `Round client rates land exactly because the timer period is an integer:`; `rates.md:45` — `| 8.33 MHz | 11 | 8 MHz | 8 MHz | — |`; `rates.md:20-22` — `the device then produces the nearest period or engine it can`.

**Why it hurts:** 8.33 MHz is not exact; it selects the 8 MHz engine. "Nearest" is also wrong at band edges: divider 93 (nominal 1.064 MHz) selects 2 MHz, not the nearer 1 MHz; divider 43 (nominal 2.27 MHz) selects 4 MHz, not the nearer 2 MHz. A reader doing timing maths may expect a nearer value than the firmware gives.

**Fix:** Change the heading to "Common requests and their achieved rates"; change "nearest" to "the engine selected by the divider band". Keep 8.33 MHz as a nominal request/8 MHz achieved row.

### 8. [severity: minor] `enh` is called an "ancillary version" and an "ancillary marker"

**Where:** `docs/protocol/sump/client.md:28` and `docs/protocol/sump/commands.md:94`.

**Quote:** `client.md:28` — "`enh` ancillary marker that matches the enhanced client profiles"; `commands.md:94` — "`0x03` | string | ancillary version, `enh`".

**Why it hurts:** The same metadata key is named two ways. A reader cannot tell whether `enh` is a version string, a marker, or both, and a client author may look for the wrong key semantics.

**Fix:** Use one form: "ancillary version string `enh` (the enhanced-profile marker)". The command table treats it as a version; the client table treats it as a marker. Pick "version" as the name and keep "marker" only as the role.

### 9. [severity: minor] The entry recovery row mixes "answered BBIO1" with "answered a sub-mode"

**Where:** `docs/protocol/sump/entry.md:161`.

**Quote:** `| the board answered BBIO1 | it is in the binary layer; return with 0x0F, or 0x00 then 0x0F from a sub-mode, and retry with five to nineteen NULs |`

**Why it hurts:** If the board answered `BBIO1`, it is already in BBIO1 and `0x0F` is the return. `0x00` then `0x0F` is for a sub-mode identifier (`I2C1`, `SPI1`, …). The row conflates two symptoms; a user who sees `BBIO1` may send an unnecessary `0x00` (which just re-prints `BBIO1`) and then `0x0F`, which works but is confusing.

**Fix:** Split into two rows: "answered `BBIO1` → send `0x0F`"; "answered `I2C1`/`SPI1`/… → send `0x00`, read `BBIO1`, then `0x0F`". Link `protocol/binary/edge-cases.md#0x0f-inside-a-mode-does-not-return-to-the-terminal`.

### 10. [severity: minor] Counter rollover has no row in the failure-mode page

**Where:** `docs/protocol/sump/rates.md:131-132`; `docs/protocol/sump/edge-cases.md` has no rollover entry.

**Quote:** `rates.md:131-132` — `The rollover handling is invisible to the host: the reply length, the newest-first order and the pre-trigger split are unchanged by a long wait.`

**Why it hurts:** `edge-cases.md` is the page a reader opens for "no trigger, overrun, rollover, wrong layer". Rollover is not there. The symptom is "none", but the reader has to know to look in `rates.md`; the README table does mention "timer and counter rollover" under `rates.md`, so this is a navigation gap rather than a missing fact.

**Fix:** Add one row to `edge-cases.md`: "trigger wait longer than the 32-bit sample counter | no visible symptom; reply length/order/split unchanged | counter is rebased; no action" and link `rates.md#counter-rollover`.

### 11. [severity: minor] "A long burst can overflow it" has no consequence or recovery

**Where:** `docs/protocol/sump/readout.md:106`, `docs/protocol/sump/edge-cases.md:91`.

**Quote:** `readout.md:106` — `| during the readout | handled after the last sample; one byte waits in the receiver, a long burst can overflow it |`; `edge-cases.md:91` — `| a byte sent during the readout | it is handled after the last sample | one byte waits in the receiver; a long burst can overflow it |`

**Why it hurts:** "Can overflow it" does not say what is lost or how to recover. If the receiver overflows, the next command may be lost or the parser may be out of step; the parser rules (`commands.md#parser-rules`) have the answer (five `0x00` resets or one-second gap), but neither row links them. A user who sends a burst during a long readout may think the board is stuck.

**Fix:** Add: "bytes beyond the receiver are lost; the next command may be ignored. After the readout, send five `0x00` resets or wait one second before the next command." Link `commands.md#parser-rules`.

### 12. [severity: minor] A v3 user has no default baud or wrong-baud symptom

**Where:** `docs/protocol/sump/client.md:48`.

**Quote:** `| v3 | UART | the terminal's baud rate, 8 data bits, no parity, one stop bit |`

**Why it hurts:** "The terminal's baud rate" is correct but not actionable if the user changed `b` earlier. The default is 115200 (generic README/navigation), and a wrong baud looks like no `1ALS` or garbage. The SUMP page should be self-contained for a v3 owner.

**Fix:** Add: "Default 115200. If `b` was used, set the client to the same rate; a mismatch gives no or garbled `1ALS`." Link `generic/navigation.md#terminal-baud--b`.

### 13. [severity: minor] The README names a client but gives no way to get one or start a first capture

**Where:** `docs/protocol/sump/README.md:60-62` and `:103`.

**Quote:** `README.md:60-62` — `The client owns the port.  Choose the SUMP/OLS profile in sigrok or PulseView and select the Bus Pirate's serial port; close any terminal on that port first.`; `README.md:103` — `No host program ships with the firmware.`

**Why it hurts:** For a fresh reader this is the largest first-impression gap. The set says what to choose but not where to get it, and "No host program ships" is a dead end. The scope excludes client UI, but a link and a five-line first-capture recipe are not UI documentation.

**Fix:** Add links to sigrok/PulseView and a minimal first capture: "Open PulseView, choose the OLS/SUMP profile, 1 MHz, 4096 samples, no trigger, run; connect a known square wave." If the exact client invocation is unverified, mark it as such. At minimum, replace "No host program ships" with "No SUMP client ships; install PulseView/sigrok from …".

### 14. [severity: minor] The command page says an exit prints the banner but omits the `HiZ>` prompt

**Where:** `docs/protocol/sump/commands.md:20-22`.

**Quote:** `an exit additionally prints the terminal banner.  There are no line endings, length prefixes or checksums in the protocol replies.`

**Why it hurts:** `entry.md:128-132` and `readout.md:122` say the exit prints the banner **and** the `HiZ>` prompt. A client that parses the exit will see an extra five bytes it did not expect from this sentence. The prompt is not a protocol reply, but it is on the wire.

**Fix:** Change to "an exit additionally prints the terminal banner and the `HiZ>` prompt." Or explicitly say the prompt is terminal output, not part of the SUMP reply.

### 15. [severity: minor] "The rate is never an error" is too broad for the slow end

**Where:** `docs/protocol/sump/client.md:63`.

**Quote:** `A request above the advertised 16 MHz still selects the 16 MHz engine; the rate is never an error.`

**Why it hurts:** A request slower than about 6 Hz cannot be expressed: the 24-bit divider wraps and the capture runs at a different rate (`rates.md:143-146`). The sentence is true for the fast end, but the unqualified "never" contradicts the slow-end clamp.

**Fix:** Say "a request above 16 MHz is not an error; it selects the 16 MHz engine. The slow end wraps, so clamp it before computing the divider."

### 16. [severity: minor] "A shorter request fills only that many slots" conflicts with the tier rule

**Where:** `docs/protocol/sump/depth.md:97`.

**Quote:** `A shorter request fills only that many slots.  The readout walks backwards from the newest sample, which is why the stream is newest-first.`

**Why it hurts:** A request of 4097 is "shorter" than the 8192 tier size, but the capture is 4096 (the tier size), not 4097. The sentence is only true below the tier size; inside a tier, the request is padded. A reader may expect the ring to fill 4097 slots.

**Fix:** "A request below the tier size fills only the requested number of slots; a request inside a tier fills the tier size and pads the rest."

### 17. [severity: minor] The v3 warning in the 8192 example does not say the same mask byte means CS

**Where:** `docs/protocol/sump/examples.md:89`.

**Quote:** `On v3 the same count keeps CS, MISO, CLK and MOSI; an AUX2 trigger is not available there.`

**Why it hurts:** The example block sends `c0 01 00 00 00` and is described as triggering on AUX2 (v4). On v3, bit 0 is CS, so the same bytes trigger on CS, not "no AUX2". The note warns that AUX2 is unavailable but does not say the byte is still valid and now selects CS. A v3 reader can unknowingly trigger on the chip select.

**Fix:** Add "this block is v4; on v3 `c0 01` selects CS — use the v3 mask table and change the trigger intentionally." Or add a short v3 variant of the block.

### 18. [severity: minor] "1ALS on entry and again for every 0x02" reads as two replies on entry

**Where:** `docs/protocol/sump/client.md:16-17`.

**Quote:** `The device sends the four bytes 1ALS when it enters SUMP and again for every 0x02 identification request.`

**Why it hurts:** The entry sequence's own `0x02` is the identification request; the device sends one `1ALS` for entry, not one for "entry" plus one for the `0x02`. The examples and `entry.md` show one. A literal reader may wait for a second reply and time out.

**Fix:** "The entry `0x02` returns `1ALS`; a later `0x02` request returns it again."

### 19. [severity: minor] "Build-only" and the repeated v3 caveats read like an audit, not a manual

**Where:** `docs/protocol/sump/README.md:78` and `:86`, `docs/protocol/sump/depth.md:172`, `docs/protocol/sump/edge-cases.md:99`, `docs/protocol/sump/trigger.md:144`, `docs/protocol/sump/rates.md:160`.

**Quote:** `README.md:78` — `| build | present, build-only | present, default image |`; `README.md:86` — `The v3 column is build-only and **not verified on v3 hardware**.`

**Why it hurts:** "Build-only" is not plain: it can mean "present in the build" or "only a build, not a working port." The same "not verified on v3 hardware" caveat appears in most files. The fact is honest, but the repetition makes the set feel like a status report and dilutes the one place where it matters (v3 fast capacities, 4 MHz jitter, readout timing). One short availability note would be enough; six copies is more than a reader needs.

**Fix:** Say once in the README: "v3: present in the build; not verified on hardware. Fast capacities and 4 MHz jitter are the v3-specific caveats." Keep short inline caveats only on the v3 fast/ jitter/readout pages. Replace "build-only" with "present in the build".

## What to keep

- The **5–19 NUL vs 20-NUL boundary** table and the `0x00` → `0x0F` recovery example. This is the clearest part of the set; I could reproduce the wrong-layer recovery without another reference.
- The **exact byte examples** with comments aligned at a single column. I could recompute the read count and delay from `81 ff 03 ff 00` and confirm the 4096/1024 split. The entry/metadata block is also easy to follow because it groups one key per line.
- The **newest-first** warning repeated in `client.md`, `readout.md` and `examples.md`. I did not flip the trace because the warning is impossible to miss.
- The **padding-first / padding-at-newest-end** callout. It is counterintuitive and would have confused me without the explicit "a newest-first client shows the padding at the newest end."
- The **per-board channel and trigger mask tables**, plus the AUX token mapping (`A0`/`AUX`, `A1`/`AUX1`, `A2`/`AUX2`). These are the tables I would tape to the bench.
- The **"trigger is an edge, not a level/pattern/sequence"** callout and the **"mask is independent of the recorded set"** warning. Both are easy to get wrong and are stated before the tables.
- The **"configuration resets after every capture"** and **"there is no read-back"** warnings. They explain why a client must resend the setup and why a host cannot query it.
- The **fast buffer / tier / ring arithmetic** explanation. The concept is clear even where the example counts are wrong; the byte/packed distinction and the 2048-word view are the right level of detail.
- The **edge-cases tables** themselves. The trigger → symptom → behaviour/recovery format is exactly what I want when a capture fails; most rows are concrete.
- The **wiring headline** ("no pull-up, no supply required; every probe pin is an input; 5 V safe"). I did not have to hunt for it.

## Structural opinion

The layout works for this function. One page per topic (entry, client, commands, rates, depth, trigger, readout, examples, edge-cases) is better than one long manual, and the README is a genuine map: the file table plus the five-minute version got me to every page in under a minute. The split matches the mental model — enter, configure, capture, read, recover — even though the file table is in a more reference-like order.

Concrete proposals:

1. **Add a "Start here" path table at the top of the README.** Columns: step, page, why. Steps: enter (`entry.md`), choose client (`client.md`), rate/depth/trigger (`rates.md`, `depth.md`, `trigger.md`), run/read (`readout.md`), recover (`entry.md#recovery`, `edge-cases.md`). This fixes the two dead ends (recovery and client acquisition) without moving files.
2. **Re-order the file table to the reader path, or add a "use when" column.** `commands.md` is a reference, not step 3; a reader configuring a capture wants `rates` → `depth` → `trigger` → `readout` first. Keep `commands` after `readout` and before `examples`, or mark it "reference".
3. **Add a `quickstart` section or file with one real first capture.** "PulseView, OLS profile, 1 MHz, 4096 samples, no trigger, one known square wave; then try a CS trigger." Link `examples.md` for the byte-level version. This is the missing bridge between "what is SUMP" and "I have a trace".
4. **Make the rate page self-contained.** Add the inverse `x = round(100 MHz ÷ rate) − 1`, a `300 kHz → x=332` worked row, and a separate "default timer period (not a divider)" note. This is the one place where the tables do not let a script author compute the request without another reference.
5. **Add two cross-page tables that are currently prose-only:** the trigger pre-fill blind window (rate / samples / time) and the abort timing (timer capture, fast trigger wait, fast post-trigger window, readout). Both are recovery facts a user needs at the bench, and both are currently split across three pages.
6. **Pick one authority for duplicated material.** `client.md` "Options that do nothing" repeats `commands.md` "Ignored commands"; `client.md` sample-order and `readout.md` sample-order repeat each other. Keep the command table as the authority and have the client page link to it, and keep the sample-order/padding explanation once (probably `readout.md`) with links from `client.md`.
7. **Consolidate the v3 caveat.** One availability note in the README, plus short inline caveats only where the v3 behavior differs numerically (fast capacities, 4 MHz jitter, readout timing). This will make the set read like a manual rather than a verification report.

## Unknowns

- **Client behavior and label.** I could not verify whether a given sigrok/PulseView release labels the profile "Openbench Logic Sniffer & SUMP compatibles", whether it sends exactly five reset bytes, or whether it rounds a typed sample count to a multiple of four. The docs mark these unverified; I agree.
- **v3 hardware.** No v3 board was available. The v3 channel map, fast capacities, 4 MHz jitter and readout timing are source/document claims, not bench results; the docs say so.
- **5.5 V absolute maximum.** The SUMP pages state "5 V safe, 5.5 V maximum" as fact, while some sibling pages call the board-level limit "unverified" (`protocol/spi/edge-cases.md:18`). I could not resolve which is authoritative. A reader wiring a target should not rely on the number without a board-level source.
- **Non-OLS SUMP clients.** The `1ALS` signature is deliberately reversed; the docs say OLS/sigrok accept it. I did not test a generic SUMP client that expects `SLA1`.
- **Readout overflow consequence.** The docs say a long burst "can overflow" the receiver but not what is lost. I could not test whether it drops only the extra bytes, the next command, or corrupts the parser.
- **Fast post-trigger abort on hardware.** `readout.md` says the byte is handled after the readout; I did not measure the delay on a board, especially the v3 2 MHz 4096-sample case.
- **The exact default timer period.** The rates table's own 10 kHz row uses x=9999 with 1600 ticks; the default says 1601 ticks, which the same arithmetic makes about 9.994 kHz. I did not run a capture to confirm the achieved rate.
- **Whether the examples work byte-for-byte on hardware.** I did not have a board; the examples are internally consistent except for the counts called out above.
