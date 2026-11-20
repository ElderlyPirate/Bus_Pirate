# UART — round-1 reader feedback

## Verdict

**Yes with fixes.** The set is complete and technically strong, but the
first worked loopback cannot work as written, the v3 bridge/binary notes
mislead, and the repeated verification caveats make the v3 story harder to
trust than the content deserves.

## Task log

1. **README alone — what is UART, does it apply, where to start.** Read
   `README.md` 1–110. About one minute. Success: the five-minute version
   gives entry, wiring, send/read and errors in one screen. Unclear: line 3
   says v3 is "build-only (unverified)" while the availability table says
   "full duplex | yes" for v3; "build-only" is undefined, so I could not
   tell whether my v3 has a usable image. The opening paragraph is dense
   with HDUART/ART1 before either is explained, but the file table and
   five-minute version recover.

2. **Enter, choose speed, wire full duplex, read/write, live display.**
   README → `entry-and-setup.md` → `wiring.md` → `terminal-usage.md`.
   About 15 minutes. Success: speed presets, custom baud, auto-baud, `W`,
   `P`/`e`, read/write and the `[`/`{` display are all findable. Unclear:
   the worked loopback uses open drain with no pull-up (finding 1); the
   header vs one-line speed numbering is a real footgun (finding 10); target
   power is buried in the Vpullup section (finding 12).

3. **HDUART.** README → `hduart.md` → `wiring.md`. About five minutes.
   Success: selection (`Duplex` 2, or the last one-line field), forced open
   drain, idle-high, the no-self-echo window, and the bridge/monitor
   caveats are clear. Unclear: the pull-up is "to 3.3 V" and then "use the
   target's logic voltage" in the same breath (finding 11). There is no
   worked HDUART transcript, so I had to assemble the command sequence from
   the duplex prompt and one-line table.

4. **Receive errors.** `terminal-usage.md` → `errors.md` → `edge-cases.md`.
   About five minutes. Success: the 127-word ring, flag placement, failed-
   read artifact and silent-vs-LED overrun are all explained. Unclear: the
   counter is never readable and the MODE LED is never located (finding 4);
   the `-p` row is explained in detail and then marked unverified (finding
   7). The ring is introduced twice.

5. **Binary ART1.** README → `binary.md` → `edge-cases.md`. About ten
   minutes. Success: entry, preset speed, `0x07`, `0x8x`, bulk exchange,
   ACK counts and the `0x0F` trap are clear; the "same byte in BBIO1 and
   ART1" table is the best part of the page. Unclear: v3 bring-up tells me
   to send a v4-only command (finding 2); the `0x0F` trap is below the
   command table and absent from the README limits (finding 13).

6. **Rendering scan.** Read every UART file plus the generic pages; about
   20 minutes. No TAB characters, no stray control characters, consistent
   table pipe counts, and no misaligned captures. The speed menu and option
   tables render cleanly. Problems found: repeated verification footers (finding
   7), duplicated ring prose (finding 14), dense nested bullet chains
   (finding 15). No dates, bench/rig narratives, pass counts, capture
   provenance or internal log names.

7. **README navigation test.** README → files took about two minutes to
   understand and about one minute per reference table. Edge cases are one
   click away; the names and availability tables are good. But the README
   links files, not sections, so the speed table and one-line field table
   require scrolling a 450-line file. Worked sessions are scattered and not
   indexed. The "full default UART entry session" link in Related docs goes
   to a setup-only generic session, so I doubled back to `terminal-usage.md`
   for a real read/write session.

## Findings

1. **[severity: major] `terminal-usage.md:162-167` — the worked loopback is
   wired for failure.** The text says "A full-duplex loopback (TX tied to
   RX) at 9600 bps, 8/NONE", then enters with `m3 5 1 1 1 1 1` and shows
   `UART (spd brg dbp sb rxp hiz dup)=( 4 416 0 0 0 1 0 )`. Output `1` is
   open drain and no pull-up is fitted; with TX tied to RX there is no
   target to supply a high and no `P` command. The line never idles high,
   so there is no start bit and the shown `READ: 0x41` cannot happen. Fix:
   use `m3 5 1 1 1 2 1` (Normal) for a no-target loopback, or add "fit a
   1 kΩ pull-up from the TX/RX node to 3.3 V" and update the header to
   `hiz 0`.

2. **[severity: major] `edge-cases.md:98` and `binary.md:21-25,154-156` —
   v3 binary bring-up points at a v4-only command.** The edge-case says
   "send `0x51` then `0x4C` first", but `binary.md:213` says the pull-up
   feed is "— (answered `0x00`)" on v3, and the binary page itself only
   says "on v4 send the VPU feed". A v3 user follows the only fix offered,
   gets `0x00`, and still has no idle high. Fix: add the v3 branch — feed
   `Vpu` externally and send `0x4C`, or select normal output with the
   `0x8x`/`0x9x` bit-4 set (`0x90`) — and mark the `0x51` row v4-only.

3. **[severity: major] `bridge.md:32-36` — the v3 bridge exit description is
   false.** The table says "**v3** | hardware reset only (no button
   escape)", then the next paragraph says "On exit a blank line is printed
   and the `UART>` prompt returns. The UART port stays configured, so a
   word received after the exit stays in the ring". A v3 reset reboots to
   `HiZ>`; it does not return to `UART>`, and the port settings and ring
   are gone. Fix: scope the second paragraph to v4 and add "On v3 the reset
   loses the UART configuration and any queued words."

4. **[severity: major] `errors.md:47-57` — the overrun counter is a dead
   end.** The page says "The counter counts **events** since the port was
   last enabled", then "any counter change turns the MODE LED **off** — the
   only visible signal." There is no command or status field that prints
   the count, and the page never says which physical LED is the MODE LED.
   A reader who wants to know whether data was lost has nothing to look at.
   Fix: add one sentence — "The count is internal; no command prints it.
   In bridge/monitor/`ART1` the only sign is the MODE LED turning off" —
   and identify the LED on the board.

5. **[severity: major] `terminal-usage.md:22-23` — the `;N` suffix is
   described backwards.** The text says "The `;N` suffix sets the width for
   the following value". The generic grammar it links to says the suffix
   follows the value (`0x55;4`) and sets the width for that value and later
   ones. Read literally, this teaches `;9 0x1FF`, which the parser will not
   accept as a width for the next value. Fix: "`;N` after a value sets that
   value's width and the width for later values until changed, for example
   `0x1FF;9`."

6. **[severity: major] `bridge.md:74-80` — the v3 host-side baud advice names
   the wrong side.** The closing sentence says "Adjust the computer-side
   rate first (`b`), then start the bridge at the target speed." `b` is a
   Bus Pirate command that changes the Bus Pirate's terminal UART, not the
   computer's port. If the host is at 9600 and the Bus Pirate at 115200,
   you cannot type `b` to fix it. Fix: "Set your terminal to the Bus
   Pirate's current link rate (115200 by default); use `b` to change the
   Bus Pirate side, then set the terminal to the new rate when prompted,
   then start the bridge."

7. **[severity: major] `README.md:3`, `entry-and-setup.md:441`,
   `edge-cases.md:102-112` — the v3 status is ambiguous and repeated into
   noise.** The first line a reader sees is "documented for v4; v3 is
   build-only (unverified)", then every page ends with "The v3 column is
   **not verified**", and the edge-case file adds a dedicated "Unverified
   paths" section. "Build-only" is never defined; the availability table
   simultaneously says v3 has full duplex, HDUART and the bridge. A v3
   owner cannot tell which instructions are safe to follow. Fix: put one
   short v3 status paragraph in the README ("v3 firmware is present and
   documented from the build; its hardware behaviour is not confirmed"),
   delete the per-page footers, and keep only the specific caveat where it
   changes a step (for example the `-p` rendering).

8. **[severity: minor] `README.md:45` and `macros.md:15` — the same macro has
   two names.** The firmware menu says `Live monitor`; the README calls
   `(2)` "the raw monitor", and `terminal-usage.md:142` calls it "the raw
   live monitor". A reader looking for "raw monitor" in the `(0)` list sees
   "Live monitor" and has to infer the mapping. The same applies to
   `Bridge with flow control` vs "flow-control bridge". Fix: use the
   firmware's `Live monitor` and `Bridge with flow control` in prose, or
   add both to the README names table.

9. **[severity: minor] `entry-and-setup.md:359-363` — "If any field is
   missing" contradicts the previous sentence.** The page says "The fields
   through `output` are required; `duplex` may be omitted", then "If any
   field is missing or out of range, the firmware falls back to the full
   prompt chain". A reader cannot tell whether omitting `duplex` is safe.
   Fix: "If any required field (through `output`) is missing or out of
   range…".

10. **[severity: minor] `entry-and-setup.md:326` and `:258` — header `spd 9`
    is described only as "custom", but auto-baud also writes it.** The
    header table says "`9` is custom"; the auto-baud section says a
    detected rate is "shown as `spd 9` with the divider in `brg`". Fix:
    "`9` = custom baud or an auto-baud result".

11. **[severity: minor] `wiring.md:121-124` and `hduart.md:46` — the pull-up
    sentence argues with itself and talks to a reviewer.** It says "**A
    pull-up resistor to 3.3 V is required — stated explicitly.**", then two
    sentences later "use the target's logic voltage when it is not 3.3 V".
    For a 5 V or 1.8 V target the first sentence is wrong. "Stated
    explicitly" is not information for the reader. Fix: "A pull-up to the
    target's logic voltage (3.3 V typical) is required. 1 kΩ–10 kΩ is
    suitable."

12. **[severity: minor] `wiring.md:32-34` — "the three required wires" omits
    target power.** The page says "The three required wires are **TX, RX
    and GND**", then much later "`W` enables the on-board 3.3 V and 5.0 V
    rails on their header pins to power the target; Vpullup only sets the
    pull-up reference". A user with a non-self-powered target can wire the
    three data wires and wonder why nothing runs. Fix: add a note to "Pick
    your wiring": "Non-self-powered target: also connect 3.3 V or 5.0 V
    and GND; `W` switches that rail on. Vpullup is separate."

13. **[severity: minor] `README.md:14-24` and `:96-97` — the front door has
    no section links and hides the one dangerous binary command.** The file
    table only links files, so finding the speed table or one-line field
    table means scrolling `entry-and-setup.md` (450 lines); worked sessions
    are not indexed at all. The binary bullet in Limits says only "The
    binary layer is **full duplex only** and its bulk writes are 8-bit",
    with no mention of the `0x0F` one-way bridge. Fix: add a short
    "Reference index" (deep links to the speed menu, one-line form, flags,
    failed reads, overrun, binary commands) and a "Worked sessions" table;
    add a Limits bullet:
    "`0x0F` is a terminal return only in BBIO1; inside `ART1` it is a
    one-way bridge (v4 NORMAL button, v3 reset)."

14. **[severity: minor] `terminal-usage.md:127-130`, `errors.md:37-39`,
    `hduart.md:123-127` — the receive-ring paragraph is copied three
    times.** The same "four-word hardware receive FIFO … 127-usable-word
    software ring … ninth data bit and parity/framing flags" text appears
    in three files. Reading front-to-back, I read the same paragraph three
    times; three copies can drift. Fix: keep the full description in
    `terminal-usage.md` and link to it from `errors.md` and `hduart.md`.

15. **[severity: minor] `hduart.md:63-82`, `entry-and-setup.md:246-264`,
    `terminal-usage.md:102-119` — key behaviour is buried in nested bullet
    chains.** The HDUART "before / during / after" window and the
    auto-baud caveats are exactly the things a table makes scannable. Fix:
    convert each to a two-column "condition → what you see / what to do"
    table; keep prose only for the explanation.

## What to keep

- The README five-minute version: entry, wiring, send/read and errors in
  one screen.
- The speed-menu table plus the explicit v3 raw-divider vs v4 bps split.
- "Pick your wiring" and the per-board `v` pin-order table; the
  `TxD`/`RxD` = MOSI/MISO mapping is unambiguous.
- The HDUART no-self-echo window: it explains a behaviour I could never
  guess from the wire.
- The `READ: FAILED, NO DATA0x00` explanation; without it I would have
  thought the firmware was broken.
- The binary "same byte in BBIO1 and ART1" table and the worked exchange
  with exact ACK counts.
- The edge-case tables: trigger → symptom → recovery is the right shape,
  and the paste-the-header, comma-separated and v3-bps-vs-divider rows are
  the things I would actually hit.
- The README names table; mapping firmware strings to prose is rare and
  valuable.

## Structural opinion

The file split works for this function: entry/setup, wiring, terminal
usage, HDUART, macros, bridge, binary, errors, edge cases is the right
order. The problem is that the README maps files, not tasks.

Concrete proposals:

1. Add a **Reference index** at the top of the README with deep links:
   speed menu, custom baud, one-line form, frame settings, flags, failed
   reads, overrun, binary commands, `0x0F` trap. One click to the table is
   better than one click plus 300 lines of scrolling.
2. Add a **Worked sessions** table: menu entry, loopback read/write, live
   display, HDUART, bridge prompt, binary exchange. HDUART currently has no
   transcript at all; a ten-line session showing the no-self-echo result
   would close that gap.
3. Give each fact one owner: ring in `terminal-usage.md`, overrun/LED in
   `errors.md`, pull-up in `wiring.md`, auto-baud in `macros.md`, binary
   bring-up in `binary.md`. Other pages link instead of restating.
4. Consolidate the v3 status into one README paragraph and drop the
   repeated "v3 column is not verified" footers; keep only page-specific
   caveats.
5. Add a small **speed-number conversion** table next to the settings
   header: menu number (1–11) → one-line value → header `spd`. That removes
   the most dangerous footgun in the set.
6. Put the `0x0F` trap in a warning block at the very top of `binary.md`,
   not only in the command table, and add a symptom index to the top of
   `edge-cases.md` ("no data", "garbled", "LED off", "bridge won't exit").

## Unknowns

- Whether the v3 FTDI/baud advice is correct; the page conflates the
  computer-side rate and the Bus Pirate-side rate, so I cannot tell what a
  v3 user should actually type.
- Whether the `-p` parity rendering and the 16 MHz / 125 kbaud peer claim
  are real; the docs themselves mark the parity case unverified.
- Which physical LED is the MODE LED; the set never says, and "the only
  visible signal" is not actionable without that.
- Whether a v3 binary user is expected to use normal output by default;
  there is no v3 worked exchange and the only bring-up instructions are
  v4-only.
- The 5.5 V limit and the "UART pins are 5 V logic" wording; the page says
  the board ratings are unverified, and "5 V logic" reads as 5 V output
  even though Normal output is 3.3 V.
