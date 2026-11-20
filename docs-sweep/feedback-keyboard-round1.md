# KEYB docs — first-read feedback (round 1)

**Reader:** fresh human reader, old Bus Pirate v3/v4 owner, wants to use the KEYB / PC-AT keyboard mode. Read the seven KEYB pages, the generic pages they lean on, and TEMPLATE/SPEC for context. Did not review the JTAG set or the `docs-sweep` artifacts.

## Verdict

**Yes with fixes.** The v4 path is followable: I could go from the README to `m8`, wire DATA/MOSI, CLOCK/CLK, VCC/5.0V, GND, run `W`, enable `e3`/`P`, start `(1)`, and decode `0x1C` as A. The protocol page is accurate and clear. The reasons this is not a "strong yes" are a pull-up diagnostic that contradicts the clock-ownership section, a wiring section that never states the board's 5 V tolerance or the external-supply path, and a missing power-on/BAT step that will surprise the first read.

## Task log

### 1. First impression from the README alone

- Looked: README, top to bottom.
- Speed: under 2 minutes.
- Success: yes. I knew it was the PC-AT/PS/2 keyboard mode, v4 default, v3 default excludes it, mode 8, no setup prompts, DATA/MOSI + CLOCK/CLK + 5.0V + GND, `W`, `(1)`, `r`/`r:N`, `(0)`.
- Unclear: the v3 feature rows are all dashes, so "module present, optional" reads as "not available"; `e3` is v4-only but the quick start does not say so; the five-minute bullets are not a numbered flow.

### 2. Enter KEYB, wire a keyboard, run `W`, use the monitor

- Looked: README five-minute version → entry-and-setup → wiring → usage §Before the first byte / §The live monitor.
- Speed: about 10 minutes, with one pass back to wiring.
- Success: yes on paper. `m8` → `KEYB>`; wire DATA→MOSI, CLOCK→CLK, VCC→5.0V, GND→GND; `W` → `POWER SUPPLIES ON` / `Clutch engaged!!!`; `e3` → `5V on-board pullup voltage enabled`; `P` → `Pull-up resistors ON`; `(1)` → `Input monitor, any key exits` / `Any key to exit`; press A → `0x1C`.
- Unclear: the pull-up check says both lines idle high, but the protocol page says CLOCK is held low; the external 5 V path; whether the first byte after `W` is the key or the keyboard's self-test; `e3` is not on v3.

### 3. Understand the frame

- Looked: protocol.md; wiring.md §Direction and levels; generic/values §Bit order.
- Speed: about 8 minutes.
- Success: yes for both directions. I could follow a device-to-host 11-slot frame (start low, 8 data LSB-first, odd parity, stop high) and a host-to-device 12-slot frame (same plus the keyboard's ACK slot; DATA low = ACK, high = NACK; stop sampled then discarded). Clock ownership and the 1.3 ms read window are clear.
- Unclear: "DATA high" vs "DATA released" in the slot table; the `l` MSB default vs the wire's LSB-first; a low stop bit on a write is not reported; the "both lines idle high" sentence contradicts "CLOCK is held low between exchanges."

### 4. Use the terminal

- Looked: usage.md §Reading, §Writing, §Two-byte, §Sending several, §Macros, §Worked examples; scancodes.md.
- Speed: about 10 minutes.
- Success: mostly. `r` → `READ: 0x1C`; `r:2` repeats; `0xEE` → `WRITE: 0xEEACK`; `0xED` + `0x04` for Caps Lock; `"abc"` → `WRITE: "aACKbACKcACK"`; `(0)` lists the two macros; `(1)` monitors; `0x1C` is A in the roster.
- Unclear: no `NACK` capture; no successful `r:2` capture; `"abc"` looks like text injection but sends raw ASCII bytes; the roster is keyed by key, not code, so decoding a stream takes several scans; keypad 1–4/6–9 are missing.

### 5. Edge cases

- Looked: edge-cases.md; wiring.md §Pull-up / §Supply.
- Speed: about 8 minutes.
- Success: the symptom tables are comprehensive (unpowered, absent, no pull-up, frame errors, write faults, false ACK, shared settings). I could map most failures to a word and a next step.
- Unclear: the recovery for `TIMEOUT`/parity/stopbit says both lines must idle high, which is wrong for CLOCK; `NONE` is the same for idle, absent, and unpowered-but-pulled-high; no power-on/BAT step; the external supply path.

### 6. Rendering check

- Scanned all seven files for TAB bytes, prose line length, table shape, fences, headings, links/anchors, dates, bench prose, "tested/verified" claims, and naming variants.
- Result: no TAB bytes (0 in every file), no prose line over 80 columns, no dates, no bench/rig/battery/capture narratives, no pass counts, no "captured on" provenance, no internal log names, no "tested" or positive "verified" claims. All relative links and anchors resolve; every table has a consistent column count; fences and heading levels are balanced.
- Found: the `v` excerpt is a partial row; the `W`/`w` captures omit the blank line the firmware prints; `Set 2`/`Set-2` and `ACK`/`0xFA` are inconsistent; the unverified caveats are repeated across the set. Details below.

### 7. README navigation test

- One click from the README link table to every detail page. No dead ends, no broken anchors.
- Fastest path is the five-minute version; the link table is one click for protocol, wiring, scancodes, edge cases.
- Doubling back: wiring → usage for the monitor; edge-cases → wiring → entry-and-setup → generic/usage for `v`; protocol → entry-and-setup → usage → scancodes for the LSB/MSB reconciliation. The README's map is good; the quick start should be above the link table, and the `v`/LSB boxes would remove the two worst detours.

## Findings

### Major

1. [severity: major] **The pull-up check contradicts the clock-ownership section.**

   - `docs/protocol/keyboard/edge-cases.md:8` — "then **pull-up** (both lines idle high between exchanges)"
   - `docs/protocol/keyboard/edge-cases.md:49-50` — "`TIMEOUT`, `*parity error`, `*stopbit error` — both lines must idle high / between exchanges"
   - `docs/protocol/keyboard/protocol.md:53` — "Once `W` has engaged the mode, CLOCK is held low between exchanges.  That is the host inhibit"
   - `docs/protocol/keyboard/wiring.md:149` — "| idle high | external or keyboard pull-up on DATA and CLOCK |"

   Problem: After `W`, the Bus Pirate drives CLOCK low. A meter or logic probe shows `L` on CLK, which is normal, not a missing pull-up. The recovery step sends the reader to check a state that never occurs. `v` after `W` shows CLK as an output low; this is the expected reading.

   Fix: "DATA must idle high (MOSI input, pulled up). CLOCK is held low by the mode between exchanges — that is the inhibit, not a fault. Check CLOCK's high phase during a frame, or release the mode before measuring. Change the pin-summary row from 'idle high' to 'high when released'."

2. [severity: major] **The wiring page never states the board's 5 V input tolerance.**

   - `docs/protocol/keyboard/wiring.md:137-139` — "The interface is a 5 V interface; keep DATA and CLOCK within the 5 V rail, and do not apply an external voltage to a line that is already driven."
   - Compare `docs/protocol/dio/wiring.md:67` — "The I/O pins are **5 V class**" and `docs/protocol/spi/wiring.md:133` — "The SPI interface is **5 V safe**; do not exceed **5.5 V** on any signal pin."

   Problem: KEYB is the only 5 V protocol page that does not say whether the header pins tolerate the 5 V pull-up. A cautious reader with a 3.3 V-looking board may not connect the keyboard; a bold reader has no maximum. The generic auxiliary page's "5 V push-pull input tolerance is unverified" makes it worse.

   Fix: add the sibling-page line: "The header I/O is 5 V class; 5.5 V maximum. The keyboard's open-drain 5 V pull-up is within that. A 5 V push-pull source is not the same and needs a level shifter."

3. [severity: major] **The external 5 V supply path is underspecified, and the warning implies a dangerous case is okay.**

   - `docs/protocol/keyboard/wiring.md:113-116` — "An external 5 V supply is fine if it shares ground with the Bus Pirate... Do not connect an external supply to the `5.0V` pin while the Bus Pirate's own rail is on."
   - `docs/protocol/keyboard/wiring.md:61` — "| the Bus Pirate | `W`, then `e3`, then `P` | the 5 V rail is the pull-up source |"
   - `docs/protocol/keyboard/README.md:46-47` — "Send it before any read or write; the keyboard cannot answer until the interface is engaged."

   Problem: (a) The qualifier "while the Bus Pirate's own rail is on" implies connecting an external supply to the 5.0V pin with the rail off is acceptable; it back-feeds the rail. (b) With an external supply and the BP rail off, `e3` has no 5 V to route and `P` has no reference; the docs do not say to fit external pull-ups or feed Vpullup from the external supply. (c) The README's absolute "cannot answer" is misleading for an externally powered keyboard: it is already powered, so `W`'s remaining jobs are the idle inhibit and the pull-up reference, not power.

   Fix: "Never connect an external supply to the `5.0V` pin; power the keyboard directly from the external supply and share ground. With an external supply you have three pull-up options: fit external 1–10 kΩ resistors to it; feed the Vpullup pin from it (0–5 V) and use `P`; or run `W`, `e3`, `P` and let the Bus Pirate's own 5 V rail feed only Vpullup, not the keyboard. `e3` only routes the Bus Pirate's own 5 V rail. `W` is still recommended to engage the idle inhibit, but an externally powered keyboard can clock without it."

4. [severity: major] **The first bytes after `W` are missing: BAT `0xAA` and the first-read surprise.**

   - `docs/protocol/keyboard/usage.md:122-125` — "A key pressed while idle leaves a scancode in the keyboard's output buffer; the next `r` releases CLOCK and the keyboard clocks it promptly."
   - `docs/protocol/keyboard/usage.md:224-226` — "A keyboard may first send `0xFA` (command acknowledged) as a data byte..."
   - `docs/protocol/keyboard/scancodes.md:129` — "| `FA` | a command-acknowledge byte, if the keyboard sends one as data |"

   Problem: A standard PS/2 keyboard's power-on self-test commonly sends `0xAA` (BAT OK) or `0xFC`/`0xFD` (error), and after `0xFF` it sends `0xFA` then `0xAA`. After `W`, the first `r` or `(1)` can show `0xAA`, not the key. The roster has no `AA`, so the reader may think it is a key or a fault. The docs also do not say to wait for BAT before expecting key codes; the first read can be `NONE` while the keyboard boots.

   Fix: add a "first bytes after power-up" note in usage and a row in scancodes: `AA` = BAT passed (not a key); `FC`/`FD` = BAT error; after `0xFF`, expect `FA` then `AA`; give the keyboard a moment after `W` before pressing keys.

5. [severity: major] **`NONE` is ambiguous, and there is no positive "is a keyboard there?" test.**

   - `docs/protocol/keyboard/edge-cases.md:18-22` — "keyboard unpowered but the lines are pulled high | `NONE`"; "keyboard powered and idle | `NONE`"; "no keyboard attached, pull-ups on | `NONE`"
   - `docs/protocol/keyboard/edge-cases.md:28-29` — "`NONE` — normal at idle.  Press a key."

   Problem: Three different conditions produce the same word, and pressing a key does not distinguish an absent keyboard. A reader with no keyboard sees `NONE` and believes it is idle. The `0xEE` Echo row (`usage.md:214` — "the keyboard returns `0xEE`") is the obvious presence test, but it is not framed as one.

   Fix: add a "is a keyboard there?" test: `0xEE` → `WRITE: 0xEEACK`, then `r` → `READ: 0xEE`; if `TIMEOUT`/`NACK`, check power and wiring. This also catches a stuck-low DATA line that ACKs every write.

6. [severity: major] **LSB-first on the wire vs `l`/`L` in the terminal is scattered across four pages.**

   - `docs/protocol/keyboard/protocol.md:31` — "The data bits are least-significant bit first."
   - `docs/protocol/keyboard/entry-and-setup.md:135` — "| bit order | MSB first (`l`) |"
   - `docs/protocol/keyboard/usage.md:160-163` — "With the default MSB order... leave them at 8 bits and MSB (`l`) for host commands."
   - `docs/protocol/keyboard/scancodes.md:146` — "`l` (MSB, default) leaves values alone; `L` reverses reads and writes"

   Problem: The wire is always LSB-first; `l`/`L` is a value-level reversal in the base layer, not a wire-order change. A reader who sets `L` sends a reversed byte (`0xED` → `0xB7`), which is not the Set LEDs command. The reconciliation exists at `scancodes.md:138` ("The keyboard frame is always one 8-bit byte with LSB-first data"), but it is on the last page, after the reader has already met three "MSB" statements.

   Fix: add a short "two bit orders" box in protocol.md and near the top of scancodes.md: "On the wire: always LSB-first. In the terminal: `l` (default) prints the byte as received; `L` reverses the 8-bit value before send/after read. Leave `l` for host commands and roster lookups." Also change `entry-and-setup.md:142` ("the printed value can change") to "the printed value, and with `L` the byte sent, can change."

7. [severity: major] **The string send reads like text injection, but sends raw ASCII bytes.**

   - `docs/protocol/keyboard/usage.md:191-202` — 'A quoted string sends one keyboard byte per character: / KEYB>"abc" / WRITE: "aACKbACKcACK" ... Each character is echoed and its ACK follows...'
   - `docs/protocol/keyboard/README.md:8` — "It does not translate scancodes into characters."

   Problem: A reader may think `"abc"` types text on the host. It sends `0x61 0x62 0x63` as host-to-device command bytes, which are not text and may be ignored or NACKed. The example's ACKs look like successful text injection, reinforcing the mistake.

   Fix: label it "raw bytes, not text; `a` is 0x61, not the A key; use a string only for a sequence of raw host bytes." Or use a meaningful multi-byte command sequence instead of `"abc"`.

8. [severity: major] **The scancode roster is keyed by key name, not by code, and is incomplete.**

   - `docs/protocol/keyboard/scancodes.md:3-4` — "lookup aid for the bytes the mode prints"
   - `docs/protocol/keyboard/scancodes.md:36` — "## Set 2 roster"
   - `docs/protocol/keyboard/scancodes.md:113-115` — "The numeric keypad has its own codes: for example `0` is `70`, `5` is `73`, `*` is `7C`..."

   Problem: The primary use case is decoding a stream like `1C F0 1C`; the page is organized by key name, so `0x5A` (Enter) or `0x83` (F7) takes three to five table scans. The "standard Set-2 roster" omits keypad 1–4 and 6–9 and only gives keypad examples in prose.

   Fix: add a single code-sorted table (code → key) at the top, or sort the existing tables by code; add the missing keypad digits. Keep the key-name tables for "what code does A send?"

### Minor

9. [severity: minor] **No `NACK` capture, and no failure path for two-byte/string sends.**

   - `docs/protocol/keyboard/usage.md:142-146` shows only `WRITE: 0xEEACK`; `usage.md:153-157` shows `TIMEOUT`; the table at `usage.md:134-138` lists `NACK`.

   Problem: The exact `WRITE: 0xEENACK` line is never shown, and the section does not say what to do if the command byte NACKs before a data byte. A reader may send the mask anyway.

   Fix: add `KEYB>0xEE` / `WRITE: 0xEENACK` and "if the command byte is NACK/TIMEOUT, do not send the data byte; retry the command."

10. [severity: minor] **The only `r:N` capture is a failure, and it looks like a glitch.**

    - `docs/protocol/keyboard/usage.md:100-105` — "KEYB>r:2 / READ:  NONE / 0x00  NONE / 0x00"

    Problem: A reader sees `0x00  NONE` on one line and may think it is a value plus an error, not two failed reads. The explanation is there, but there is no successful `r:2` capture.

    Fix: add `KEYB>r:2` / `READ: 0x1C 0xF0` and annotate the failure capture.

11. [severity: minor] **The `v` excerpt is a four-name fragment, not the table.**

    - `docs/protocol/keyboard/entry-and-setup.md:164-165` — "CS      MISO    CLK     MOSI"

    Problem: The section says "`v` prints the rail/pin table", but the block shows only the last four signal names at column 0, with no directions or levels. It looks like a broken table; the reader has to go to generic/usage for the real layout.

    Fix: show a full 12-column row, or label it "last four signal-name columns only (space-aligned from tabs)."

12. [severity: minor] **The `W`/`w` captures omit the blank line the firmware prints.**

    - `docs/protocol/keyboard/entry-and-setup.md:96-99`, `105-109`, `119-123`

    Problem: The firmware prints `POWER SUPPLIES ON`, then a blank line, then `Clutch engaged!!!`; the captures show no blank line. Same for `w` and the refusal.

    Fix: add the blank line, or state "blank lines omitted from captures."

13. [severity: minor] **"No retry" is too absolute.**

    - `docs/protocol/keyboard/protocol.md:122` — "| No retry | a bad frame is reported once; issue `r` again |"

    Problem: The read window re-enters the frame reader after a non-timeout error (startbit/parity/stopbit), so a transient error can be re-sampled and not reported. "Reported once" is not exactly the behavior.

    Fix: "No retry after the command returns; the read window may re-sample after a frame error."

14. [severity: minor] **Two command-table rows are underspecified.**

    - `docs/protocol/keyboard/usage.md:217` — "| `0xF3` | Set typematic rate/delay | one byte |"
    - `docs/protocol/keyboard/usage.md:221` — "| `0xFF` | Reset | self-test; a result byte follows |"

    Problem: `0xF3`'s byte layout (bits 0–4 rate, bits 5–6 delay) is not given, so the row is unusable; `0xFF`'s usual response is `FA` then `AA`, not "a result byte."

    Fix: add the `0xF3` bit layout and the `FA`/`AA` sequence.

15. [severity: minor] **Naming: `Set 2` vs `Set-2`, and `ACK` vs `0xFA`.**

    - `docs/protocol/keyboard/scancodes.md:17` — "**Set 2**"; `scancodes.md:38` — "standard Set-2 **make** codes"
    - `docs/protocol/keyboard/usage.md:224-226` — "A keyboard may first send `0xFA` (command acknowledged) as a data byte" vs the frame's `ACK`.

    Problem: Hyphenation varies; "acknowledge" names two different things. A reader can conflate the frame's ACK slot with the `0xFA` response byte.

    Fix: pick "Set 2" (or map "Set-2" once); add "Two acknowledgements: the frame's ACK/NACK slot and the keyboard's `0xFA` response byte (read with `r`)."

16. [severity: minor] **The unverified caveats are repeated across the set.**

    - `docs/protocol/keyboard/README.md:3-4,67`; `entry-and-setup.md:192-196`; `wiring.md:8,118,159`; `usage.md:299-302`; `edge-cases.md:118-126`

    Problem: The set hedges so often that it is hard to tell what is actually unknown from what is merely untested on v3. The "Unverified paths" section reads like an audit note next to the user instructions.

    Fix: one short "What is not established" table in edge-cases; keep one-line caveats only where they change a decision.

17. [severity: minor] **The README quick start is not a numbered flow, and `e3` is v4-only.**

    - `docs/protocol/keyboard/README.md:40-53`; `README.md:44` — "run `W`, then `e3`, then `P`"

    Problem: The bullets put the pull-up step inside the wiring bullet, before the `W` bullet, so a reader may run `W` twice or enable `P` before `W`. A custom v3 build has no `e`, so `e3` is not an option there.

    Fix: make it a five-step list: wire → `m8` → `W` → `e3`/`P` if needed (v4; external pull-ups on v3) → `(1)`.

18. [severity: minor] **"There is no `(1)>` question to answer" is ambiguous next to the `(1)>8` capture.**

    - `docs/protocol/keyboard/entry-and-setup.md:45`

    Problem: The menu capture at line 64 shows `(1)>8`; a reader may think the sentence contradicts the capture.

    Fix: "There are no setup questions after the mode number; the `(1)>` in the capture is the mode menu's own prompt."

19. [severity: minor] **The `w` paragraph says "after `W`".**

    - `docs/protocol/keyboard/wiring.md:122-125` — "`w` switches the rail off but leaves the interface as it was: CLOCK stays driven low after `W`..."

    Problem: The paragraph is about `w`; "after `W`" is ambiguous.

    Fix: "CLOCK stays driven low after `w` (it was set low by `W`)."

20. [severity: minor] **The README's v3 column reads as "not available," and `m8` is a v3 trap.**

    - `docs/protocol/keyboard/README.md:57-64`; `README.md:40` — "`m` then `8`, or `m8`"; `edge-cases.md:109` — "`m8` on a default v3 build | selects PIC"

    Problem: The v3 feature rows are all dashes, while the text says "module present, optional"; a v3 owner may stop. The five-minute version does not mark `m8` as v4, so a v3 reader can select PIC by following it.

    Fix: use "optional build" in the v3 column for monitor/writes, or add a "custom v3 build" note; add "on a default v3 build `m8` selects PIC" to the quick start.

21. [severity: minor] **The frame table's "DATA high" vs "DATA released" implies two electrical states.**

    - `docs/protocol/keyboard/protocol.md:25-29` — "| 11 | stop — DATA high | stop — DATA released |"

    Problem: The bus is open drain; neither side drives high. "DATA high" and "DATA released" are the same state, and the table's wording suggests otherwise.

    Fix: use "released (pull-up high)" for both; reserve "driven high" for push-pull.

22. [severity: minor] **"A missing pull-up does not by itself produce `*startbit error`" is too absolute.**

    - `docs/protocol/keyboard/wiring.md:104-105`

    Problem: A missing pull-up can leave DATA floating high; when CLOCK goes low, the start-bit check fails. The edge-cases table itself lists `*startbit error` for floating lines. The intended qualifier "with the keyboard driving the start bit" is missing.

    Fix: "With the keyboard driving the start bit low, a missing pull-up alone does not cause a startbit error; a floating DATA line can."

## What to keep

- The README's map/link table. It answered "what is this / is it on my board / where do I start" in under two minutes, and every link/anchor resolves.
- The protocol page's 11/12-slot table. It is the clearest explanation of one device-to-host and one host-to-device frame in the set; the odd-parity and ACK/NACK rules are correct, and the 1.3 ms read window is concrete.
- The edge-cases read/write split, especially "DATA low at the acknowledge slot (stuck or shorted) → `ACK` for every write, even nonsense." That is the kind of failure a human would not guess.
- The scancode roster's accuracy. I spot-checked the letters, number row, punctuation, modifiers, function keys, navigation keys, Print Screen, Pause, and keypad examples; they are correct, including the `E0` prefixes and the `F0` break rule.
- The spacing notes. `READ:  NONE`, `WRITE: 0xEEACK`, `WRITE: 0xEE TIMEOUT`, and the monitor's trailing space are weird enough that a reader would otherwise think the docs were broken; the notes make the captures readable.
- The exact firmware strings. The quoted messages match the firmware's own strings (`POWER SUPPLIES ON`, `Clutch engaged!!!`, `5V on-board pullup voltage enabled`, `NONE`, `*parity error`, `Unknown macro, try ? or (0) for help`).
- The clean presentation: no tabs, no dates, no bench/rig prose, no capture provenance, no code references. The tables are well-formed and the prose wraps.

## Structural opinion

The layout fits the function: a short map, then one file each for entry, wiring, protocol, usage, scancodes, and edge cases. I would not merge or split the set. Four concrete changes:

1. Put a numbered "first key in five minutes" sequence at the top of the README, above the link table: wire → `m8` → `W` → `e3`/`P` if needed → `(1)` → press a key → terminal key to exit. Keep the link table below it. This removes the only real doubling back in the first-run path.
2. Move a compact `v`/`i` diagnostic capture into `wiring.md` or the top of `edge-cases.md`. The current `v` section in `entry-and-setup.md` is a four-name fragment; the reader who needs it is debugging, and should not have to go to `generic/usage.md`.
3. Add a code-sorted scancode index at the top of `scancodes.md`. Keep the key-name tables for the reverse question; put the stream-decoding path first.
4. Add a "two bit orders" box to `protocol.md` and link it from `entry-and-setup.md`, `usage.md`, and `scancodes.md`. The facts are present, but they are spread across four pages; one box would stop the `L`-sends-a-reversed-command mistake.

A smaller change: consolidate the nine "unverified"/"not verified" caveats into one short table in `edge-cases.md`, and keep only the inline caveats that change a decision (host-write timing, keyboard-specific pull-ups/current/BAT). The set reads more confidently when it says "unknown" once, precisely, than when it says "unverified" on every page.

## Unknowns

- Whether a real keyboard ACKs or NACKs the host-write timing. The docs flag this as unverified; I did not test hardware.
- Whether a given keyboard has internal DATA/CLOCK pull-ups, its 5 V current draw, its BAT behavior, or which scancode set it sends. The docs flag these; I could not judge them from the text.
- Whether the `*startbit error` "keyboard's own clock-low window before its frame" explanation is true for a specific keyboard. It is plausible, but I did not verify it.
- The Bus Pirate v4's exact input tolerance. I inferred "5 V class / 5.5 V maximum" from the DIO, I2C, SPI, and PIC pages; the KEYB page should state it, and the exact limit may need a hardware check.
- The optional v3 build's behavior. The docs flag it as not verified; I did not judge it.
- Whether `P` must be re-issued after `W`/`e3`. The generic edge-cases says the pull-ups stay enabled; the KEYB wiring page says "then `P` again." I could not tell whether the re-issue is required or just a check.
- Whether the read window's internal re-sampling after a frame error is intentional. The source shows it; the docs' "No retry" is ambiguous.
- How the `v` table actually renders on a given terminal. The docs say the firmware emits tabs; I read the space-aligned transcription, not a live terminal.
