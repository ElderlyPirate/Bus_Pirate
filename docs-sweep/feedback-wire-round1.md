# Wire docs — fresh-reader feedback (round 1)

## Verdict

**Yes with fixes.** The mode-selection table and the per-set topic files get a v3/v4 owner to a working session quickly, but a few exactness and bring-up defects — the ATR reset order, the `R2W` header capture, and the cluster's unconditional start steps — will waste a first-time user's time.

## Task log

### 1. First impression from the parent README

Looked at `docs/protocol/wire/README.md` only. About two minutes.

Which mode I need: clear. The first table maps target descriptions to 1-Wire / 2-Wire / 3-Wire and names the pins, mode numbers, headline extras and set links. That is the strongest page in the cluster.

Does it apply to my board: not clear enough. The status line says "present on v3 and v4, with v3 build-only here", and the availability table says `present (build-only)`. As a v3 owner I cannot tell whether the mode is in my firmware or only in the source tree.

Where to start: the four-step "Start here" sequence is there, but step 2 tells every reader to wire "data, clock and select", and step 3 tells every v4 reader to use `e2`/`e3`. Neither is true for every mode/output type. I had to go back to the table to correct both.

### 2. 1-Wire: enter, speed, wire, reset, byte exchange, overdrive

Looked at `1wire/README.md` → `entry-and-speed.md` → `wiring.md` → `bus-operations.md` / `timings.md`. About six minutes.

Success with careful reading. Entry (`m2` / `m2 1`), the speed menu, the mandatory pull-up, the `W` → `e2`/`e3` → `P` order, reset with `[` or `{`, and `r` / `0xNN` are all discoverable. The `BUS RESET  OK` / `*No device detected` / `*Short or no pull-up` distinction is the best part of the set.

What was unclear: the five-minute version says `O` switches standard ⇄ overdrive, which reads as if the host toggle is the whole job. It omits the `0x3C` Overdrive Skip ROM step, so following only the README gets silent garbage. There is also no plain `r` / `0xNN` capture with `WRITE:` and `READ:` lines; the worked session jumps straight to `(51)` and ROM macros.

### 3. 2-Wire: entry, settings header, START/write/read/STOP, clock/pull-up, ATR

Looked at `2wire/README.md` → `entry-and-setup.md` → `bus-operations.md` → `wiring.md` → `macros.md` / `smartcard.md`. About seven minutes.

Success on the main flow. The two entry prompts and defaults are clear, the header fields (`spd` / `hiz`, zero-based) are decoded properly, and `[` / `{` / `]` / `}` plus `r` / value are easy to follow. The clock table and the conditional pull-up rule are where I expected them.

What was unclear: the smart-card ATR sequence contradicts itself (see finding 1). The external pull-up rule says "a pull-up on the open-drain lines" but never says one resistor per line, so I was unsure whether one resistor on SDA was enough. The settings-header spacing is asserted as exact in one place and reproduced differently in `docs/generic/usage.md` (finding 2).

### 4. 3-Wire: entry, CS polarity, clocked exchange, no-macro error

Looked at `3wire/README.md` → `entry-and-setup.md` → `bus-operations.md` → `edge-cases.md`. About five minutes.

Success. The CS-polarity table is excellent: it names the menu choice, the `csl` value, the driven level and the printed string in one place. The no-macro case is documented with the exact `ERROR: command has no effect here`, blank line and `Syntax error at char 3`, and both the README and edge-cases repeat it consistently.

What was unclear: the clock-edge rules are understandable, but the `RAW1` specifics are buried at the bottom of a 296-line bus-operations page instead of a dedicated binary page like the other two sets. The `!` versus `r` sampling description also took a second read.

### 5. Shared `RAW1` binary layer

Looked at `wire/binary.md` → `2wire/binary.md` → `3wire/bus-operations.md` §Scripting. About five minutes.

Success. Entry/exit (`20 × 0x00`, `0x05`, `0x00`, `0x0F`), the `0x1x` bulk exchange, and the terminal-vs-binary table are clear. The per-wire reply difference is the thing I most needed, and it is stated twice: `0x01` per byte in 2-wire versus a sampled MISO byte in 3-wire.

What was unclear: the 3-wire side is harder to find and has no dedicated page. The shared 2-wire worked exchange never issues `0x02` / `0x03`, so it demonstrates raw clocking rather than the START/write/read/STOP transaction I expected. The entry "maximum profile but records profile 1" quirk is explained, but only after the table that says "speed setting: profile 1".

### 6. Rendering check

Scanned every file in `docs/protocol/wire/` and `docs/generic/`.

No TAB characters. Prose wraps at about 80 columns. The `v` pin-table captures are space-aligned and columns line up. Captures that exceed 80 columns are only terminal output, which is unavoidable.

Problems found: the `R2W` header spacing mismatch (finding 2); capture/build provenance in user pages (finding 8); the same v3 caveat repeated on almost every page (finding 9); lowercase `2-wire` / `3-wire` prose that contradicts the declared prose forms (finding 7); a dense bullet command reference in `2wire/binary.md` (finding 12); and the 1-Wire menu kbps labels not reconciling with the stated slot periods (finding 13). The run-together `Data units: 2Data unit length (bits): 128` line is ugly but is explained immediately below it; I would keep that note, perhaps add a visible "no newline" marker in the capture.

### 7. README navigation test

Parent → per-protocol set: one click. Set → a topic page: two clicks. Set → worked session: two clicks, but no file-map row says "worked session"; I had to open `bus-operations.md` in each set on the assumption it would be there. Binary worked exchange: `wire/binary.md#worked-exchange` is easy; the 3-wire equivalent is at the end of `bus-operations.md` and is not in the parent file map as a binary page.

No dead ends. The only doubling back was the parent "Start here" sequence sending me to the wrong wiring for 1-Wire/2-Wire and to `e2`/`e3` for normal-output modes, then back to the mode table.

## Findings

### 1. [severity: major] ATR reset sequence re-asserts reset before reading

`docs/protocol/wire/2wire/macros.md:58-61`:

> 4. drive CS low — the banner's `RESET LOW`,
>
> then read the reply.  The macro **leaves CS low** (the card's reset asserted)
> after it finishes.

`docs/protocol/wire/2wire/smartcard.md:65-66`:

> `(1)` ends with CS **low**, so the card's reset is still asserted.

Problem: as written, step 4 drives CS low (asserted reset) and only then reads the reply. A card held in reset cannot send an ATR. The later "release it with `A`" note confirms the authors mean CS low = reset asserted, so the sequence as printed is not physically followable. This is the headline smart-card feature, and a user hand-driving the sequence would fail.

Suggested fix: state the read window explicitly. Either read the ATR after CS goes high and before the final CS low, or say that the macro reads while reset is released and only then re-asserts CS. Reconcile the step order with the "leaves CS low" sentence, and check the firmware sequence before publishing.

### 2. [severity: major] The same `m6 2 1` command prints two different `R2W` headers

`docs/generic/usage.md:210-211`:

> `HiZ>m6 2 1`
> `R2W (spd hiz)=( 1 1 )`

`docs/protocol/wire/2wire/entry-and-setup.md:96-98`:

> **The double space before `)` is correct**: each value is followed by a
> space, and the closing token itself begins with a space.

`docs/protocol/wire/README.md:81-82`:

> The 2-Wire header has **two spaces** before `)`; the 3-Wire header has one.
> Both are exact; the entry pages explain the fields.

Problem: the generic worked session drops one of the two spaces. This is not a cosmetic nit: the wire pages make exact transcription a feature, and a host or user comparing the expected header will see a mismatch. The same command cannot produce both strings.

Suggested fix: change `docs/generic/usage.md:211` to `R2W (spd hiz)=( 1 1  )` and add a spacing check to whatever renders or validates captures. If the firmware really emits one space in some build, say which build; do not leave two unqualified examples.

### 3. [severity: major] "Start here" over-wires 1-Wire/2-Wire and tells normal-output users to run `e2`/`e3`

`docs/protocol/wire/README.md:97-101`:

> 1. Pick the mode from the table above.
> 2. Wire data, clock and select plus ground; then fit the pull-up the mode
>    needs.
> 3. Send `W`; on v4 use `e2`/`e3`, then `P` if you use the on-board pull-up.

Problem: step 2 is only true for 3-Wire. 1-Wire needs data + ground; 2-Wire needs SDA + SCL + ground, with CS only for the ATR macro. A first-time 1-Wire reader will look for clock and select pins that do not exist. Step 3 is also unconditional: `e2`/`e3` are refused in normal output, so a 2-Wire or 3-Wire user who chose the second output type gets a syntax error at the first bring-up command.

Suggested fix: branch the steps, for example: "1-Wire: data + GND. 2-Wire: SDA + SCL + GND (CS only for ATR). 3-Wire: MOSI + MISO + CLK + CS + GND. Fit the pull-up only for open drain. On v4 in open drain, `e2`/`e3` then `P`; in normal output skip both." Keep it to the same four steps.

### 4. [severity: major] 1-Wire quick start makes `O` look like the whole overdrive procedure

`docs/protocol/wire/1wire/README.md:37-38`:

> `O` switches standard ⇄ overdrive in place and prints the new speed; `v`
> prints the pin table (the data pin is shown as `OWD`).

`docs/protocol/wire/1wire/entry-and-speed.md:124-127`:

> A common 1-Wire sequence sends the **Overdrive Skip ROM** command (`0x3C`) at
> standard speed.  The parts switch to overdrive timing immediately; send `O`
> before the next reset so the host follows them.

Problem: the quick start says `O` switches speed, which is true only for the host. The device stays at standard until it receives `0x3C`, and a host/device mismatch produces garbled reads, not an error. A reader following only the README will toggle `O` and wonder why the bus fails.

Suggested fix: add one clause to the quick-start bullet: "host timing only; send `0x3C` at standard speed first (see entry-and-speed)." Or move the overdrive bullet below the reset/byte bullet and state the two-sided requirement.

### 5. [severity: minor] 3-Wire binary content has no dedicated page

`docs/protocol/wire/README.md:31`:

> | [3wire/README.md](3wire/README.md) | the 3-Wire set: entry, wiring, CS polarity, clock edges, `RAW1` specifics |

`docs/protocol/wire/3wire/README.md:25`:

> | [bus-operations.md](bus-operations.md) | CS control, clock edges, byte read/write, the bitwise controls, no macros, the shared `RAW1` binary path |

Problem: 1-Wire and 2-Wire each have a `binary.md`; 3-Wire's binary material is a 56-line section at the end of a 296-line bus-operations page. The parent table points at the set README, not at the binary content, and the "file map" has no row for it. This is the main navigation friction in task 5.

Suggested fix: create `3wire/binary.md` with the existing §Scripting text and worked exchange, add it to the 3-Wire and parent file maps, and link it from `wire/binary.md` as the 3-wire detail page. Keep bus-operations focused on the terminal bus.

### 6. [severity: minor] 2-Wire external pull-up rule does not say one resistor per line

`docs/protocol/wire/2wire/wiring.md:46-48`:

> **Open-drain 2-Wire needs a pull-up on the open-drain lines.**  In open drain
> the Bus Pirate can only pull a line low; the high level comes solely from the
> pull-up resistor.  Value guidance: **2 kΩ – 10 kΩ** to the pull-up voltage.

`docs/protocol/wire/README.md:40`:

> | 2-Wire | **conditional** — open-drain output only | 2 kΩ–10 kΩ to the pull-up voltage |

Problem: both SDA and SCL need a pull-up. The page says "lines" but never states that an external build needs two resistors, one per line. A fresh reader can fit one resistor on SDA and still have no clock high.

Suggested fix: write "one 2 kΩ–10 kΩ resistor from each of SDA and SCL to the pull-up voltage" in the value table and in §The pull-up. The smart-card page already says the on-board group covers both lines; make the wiring page equally explicit.

### 7. [severity: minor] Lowercase `2-wire` / `3-wire` prose contradicts the declared forms

`docs/protocol/wire/README.md:80-81`:

> `1WIRE`, `2WIRE` and `3WIRE` are the firmware tokens; `1-Wire`, `2-Wire` and
> `3-Wire` are prose.

`docs/protocol/wire/binary.md:82`:

> In 2-wire they correspond to the terminal `[` and `]`.

`docs/protocol/wire/2wire/binary.md:21`:

> ## 2-wire specifics

Problem: the cluster declares `2-Wire` / `3-Wire` as the prose forms, then uses `2-wire` / `3-wire` throughout the binary pages. A reader cannot tell whether the lowercase form means the mode, the library, or a generic wire count. The same issue appears in `3wire/bus-operations.md` (for example line 220, "handler starts in 2-wire").

Suggested fix: normalize the mode name to `2-Wire` / `3-Wire` everywhere, or add one sentence to the naming table: "lowercase `2-wire` / `3-wire` refers to the wire count or the library implementation, not the mode." If the latter, use it consistently.

### 8. [severity: minor] Capture and build provenance in user pages

`docs/generic/README.md:4-6`:

> the v3 build is covered but not verified on v3 hardware.  Captures in
> this set are v4; inline notes and the delta table in
> [navigation.md](navigation.md#v3--v4-delta) list the differences.

`docs/protocol/wire/README.md:59-60`:

> **v3 is build-only** in this documentation: the v3 behaviour is taken from
> the build, not from hardware.  The per-mode sets repeat that caveat.

Problem: "Captures in this set are v4" is capture provenance, and "taken from the build, not from hardware" is build provenance. The second sentence is also meta-commentary about the docs ("the per-mode sets repeat that caveat") rather than information a user needs. Both violate the user-doc presentation rule even though the underlying caveat is allowed.

Suggested fix: replace with plain statements: "Examples use v4 output; v3 differences are noted inline." For v3: "v3: included in the build; hardware behaviour not verified." Put the evidence trail in `docs/dev/`, not here.

### 9. [severity: minor] The same v3 caveat is repeated at the foot of almost every page

`docs/protocol/wire/1wire/bus-operations.md:173`:

> The v3 path is **not verified**.

`docs/protocol/wire/3wire/entry-and-setup.md:204-205`:

> The two builds ask the same three questions with the same wording and the
> same defaults.  The v3 column is **not verified** (v3 is build-only here).

Problem: the phrase appears about 39 times across the wire set. Repeating it on every page footer dilutes the caveats that actually matter per feature — the 1-Wire search/roster macros, the `1W01` path, and the `RAW1` path. A human stops reading the caveat after the third page.

Suggested fix: state the v3 status once in the set README availability table and once in the cluster availability table. Keep a page-level caveat only where a specific feature is unverified (search/roster, binary paths), not as a generic footer.

### 10. [severity: minor] 1-Wire has no plain byte-exchange capture

`docs/protocol/wire/1wire/bus-operations.md:127-129`:

> Full round trip: enter, engage, source VPU, enable the pull-up, detect, read
> the ROM, put the device into overdrive, toggle the host speed with `O`, read
> again, toggle back, and reset to standard.  The ROM values are examples.

Problem: the worked session is macro-centric. It shows `(51)`, `{0x3C` and `O`, but no simple `r` / `0xNN` exchange with `WRITE:` and `READ:` lines. Task 2 asks for a reset and a byte exchange; a reader has to synthesize the exact output from the command table, unlike the 2-Wire and 3-Wire pages, which both have a plain session.

Suggested fix: add a short block before the ROM session: `1WIRE>{0xCC` → `BUS RESET  OK` / `WRITE: 0xCC`, then `1WIRE>r` → `READ: 0xNN`. Note that `]` is refused in this mode, so the reset is the transaction boundary.

### 11. [severity: minor] The `RAW1` 2-wire worked exchange never issues START or STOP

`docs/protocol/wire/binary.md:241-242`:

> A minimal 2-wire byte write and byte read, open drain with a pull-up.  The
> device-specific bytes are illustrative.

The block then runs `0x10` / data / `0x06` with no `0x02` or `0x03`.

Problem: the shared example is presented as a 2-wire transaction but only clocks bytes. Raw clocking without START/STOP is legal, but a reader copying it to a real 2-wire target may get no response and conclude the binary path is broken. The page's own wrapper table maps `[` / `]` to `0x02` / `0x03`, so the omission is conspicuous.

Suggested fix: add `0x02` before the bulk write and `0x03` after the read, or add one sentence: "START/STOP are optional for raw clocking; add `0x02` / `0x03` when the target expects a framed transaction."

### 12. [severity: minor] 2-Wire binary specifics are a dense bullet chain, not a command table

`docs/protocol/wire/2wire/binary.md:25-30`:

> - **Bulk byte (`0x1x`)** — the low nibble is `n`; the host sends `n + 1` data
>   bytes.  The per-byte sequence is:
>
>   1. host → `0x1n` (the request),
>   2. host ← `0x01` (the command is accepted),
>   3. for each of the `n + 1` bytes: host → data byte, host ← `0x01`.

Problem: the section is a command reference, but each command is a bold bullet containing prose, sub-steps and caveats. The 3-Wire page has the same shape in its §Scripting bullets. Scanning for "what is the reply to `0x8x`" is slower than it should be.

Suggested fix: make a table with columns `Request | Meaning | Host sends | Reply | Terminal equivalent`, and keep only the genuinely nuanced notes below it. The same treatment would improve `3wire/bus-operations.md:219-231`.

### 13. [severity: minor] 1-Wire menu kbps labels do not reconcile with the stated slot periods

`docs/protocol/wire/1wire/timings.md:15-18`:

> | standard | `1` | `Standard (~16.3kbps)` | ≈ 70 µs |
> | overdrive | `2` | `Overdrive (~160kps)` | ≈ 12 µs |

Problem: one 70 µs slot is about 14.3 kbps, not 16.3 kbps; one 12 µs slot is about 83 kbps, not 160 kbps. The page says the labels are nominal, but it never explains the gap, so a reader cannot tell whether the Bus Pirate is slower than the menu claims or the slot figure is wrong. The 2-Wire and 3-Wire clock tables do derive their real rates; this one does not.

Suggested fix: add a sentence: "The menu labels are the firmware's nominal names; the stated slot periods imply roughly 14 kbps standard and 83 kbps overdrive." Or add a derived-rate column, as the 2-Wire/3-Wire tables do.

### 14. [severity: minor] "build-only" is undefined for a v3 owner

`docs/protocol/wire/README.md:3-4`:

> **Status:** three separate raw bit-bus modes; present on v3 and v4, with v3
> build-only here (not verified on v3 hardware).

`docs/protocol/wire/README.md:107-111`:

> | 1-Wire (mode 2) | present (build-only) | present |

Problem: "build-only" is not defined. A v3 owner cannot tell whether the mode is compiled into their firmware, whether only the source builds it, or whether the docs simply have no hardware evidence. "Present" and "build-only" read as contradictory.

Suggested fix: define it once: "v3: included in the v3 firmware build; not tested on v3 hardware." Then use "in v3 build" in the tables. Drop "build-only" unless it means something different from "not hardware-verified."

## What to keep

- The parent mode-selection table. It answers "which mode" in one screen and is the reason the cluster is usable at all.
- The CS-polarity table in `3wire/bus-operations.md`. Menu choice, `csl` value, driven level and printed string in one place is exactly the right shape.
- The trigger → symptom → behaviour/recovery tables in all three edge-cases pages. They are concrete and avoid vague "check your wiring" advice.
- The exact firmware strings and the notes about intentional defects: `BUS RESET  OK`, `~160kps`, the run-together `Data units` line, the `R2W` double space, the `(0)` blank line. This is what makes the docs trustworthy — except where one capture disagrees (finding 2).
- The binary "Terminal wrappers vs binary commands" table and the per-wire bulk reply distinction. That is the clearest part of task 5.
- The worked sessions with real prompts (`1WIRE>`, `2WIRE>`, `3WIRE>`), and the 2-Wire ATR field decode. They let me compare my own terminal output line by line.
- The "Not I2C" / "Not SPI" / "no macros by design" statements. They prevent the most likely wrong expectation.
- The presentation basics: no tabs, ~80-column prose, aligned pin-table captures. The set is pleasant to read on a normal terminal-width editor.

## Structural opinion

The three-level layout — cluster front door → per-protocol set README → parallel topic files — is the right structure for this function. The per-set file maps are accurate and the topic split (entry, wiring, bus operations, macros, binary, edge cases) matches how I actually looked things up. I would not reorganize the cluster.

Concrete changes:

1. **Give 3-Wire a `binary.md`.** It is the only set that breaks the parallel-file pattern. Move §Scripting and the worked exchange out of `bus-operations.md`, add the row to both file maps, and link it from `wire/binary.md`.
2. **Add a worked-session index to the cluster page.** One small table: 1-Wire → `1wire/bus-operations.md#worked-session`, 2-Wire → `2wire/bus-operations.md#worked-session`, 3-Wire → `3wire/bus-operations.md#worked-session`, binary → `binary.md#worked-exchange`. That removes the "open bus-operations and hope" step.
3. **Make the cluster front door shorter, not broader.** It currently repeats the per-mode pull-up table, naming mapping, binary matrix and availability summary that the sets also carry. Keep selection, start, and links; move the shared electrical rules to one "shared" page (or lean on `generic/commands.md`). That also gives the v3 caveat one home.
4. **Add a mode-entry-forms table.** `m2 1` is a valid one-field form, `m6 2` is not, `m7 2` is not. This is a recurring trap across the three sets and belongs in `generic/navigation.md` or the cluster page, not repeated as a paragraph in each entry page.
5. **Turn the five-minute sections into bring-up checklists with branches.** Enter → `W` → (`e2`/`e3` only if open drain) → `P` only if open drain → reset/START → exchange. The current bullets are close, but the 1-Wire overdrive step and the normal-output branch are missing.

## Unknowns

- Whether the ATR reset order in `2wire/macros.md` reflects the firmware or is a transcription error. I cannot reconcile "CS low = reset asserted" with "then read the reply"; someone with the firmware or a card needs to confirm the real order.
- Whether a smart card will answer at the Bus Pirate's ~6–111 kHz clock. The smart-card page calls 6 kHz "slow but safe" but never states ISO 7816's nominal clock range or the ATR timeout. I could not judge this without a card.
- Whether the v4 on-board pull-up is actually 2 kΩ. The docs mark it unverified; that affects the parasitic-power advice and the "2 kΩ or less" rule.
- Whether `R2W` really has two spaces on every build. The docs assert it, one capture disagrees, and I cannot run the firmware.
- Whether the 1-Wire search/roster macros and the `1W01` path work on v4. The docs mark them unverified; I have no hardware to test.
- Whether the 3-Wire sample point (after the clock-high delay) matches the targets users will wire. The page gives one edge scheme but no scope capture; I cannot judge the timing against a real device.
- Whether the v3 build actually includes these modes as described. The docs say "build-only"; I cannot test v3 hardware.
