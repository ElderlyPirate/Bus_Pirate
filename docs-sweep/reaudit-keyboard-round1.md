# KEYB docs — re-audit round 1

Scope: the 22 findings in `feedback-keyboard-round1.md`, checked against the current seven files in `docs/protocol/keyboard/` and the implementer record `fixes-keyboard-round1.md`. Read-only; no fresh review beyond the item list and the requested presentation check.

**Verdict: 19 of 22 items filled in full.** Two partial (3, 5), one rejected correctly (12), none untouched. The two partials contain Major new problems, so the process cap extends by one extra turn.

| # | Status | Evidence (current quote) | Remainder |
|---:|---|---|---|
| 1 | **Filled in full** | `edge-cases.md:8-10`: "then **DATA** (it must idle high through a pull-up; CLOCK is driven low by the mode between exchanges)"; `:29-31`: "an idle low CLK is normal, not a missing pull-up"; `wiring.md:163`: "DATA high through a pull-up; CLOCK driven low between exchanges". | The old "both lines idle high" wording is gone. |
| 2 | **Filled in full** | `wiring.md:149-153`: "The header I/O is **5 V class**: 5.5 V is the maximum on DATA or CLOCK. The keyboard's open-drain 5 V pull-up is within that." | Push-pull sources are now rejected for contention; no gap. |
| 3 | **Partial — new ordering error (Major)** | `wiring.md:121`: "feed `Vpullup`/`VPU` from the external supply (0–5 V), select `e1`, then `P`". | The order is backwards. `e1` refuses while Vpullup already has voltage: `generic/commands.md:19` says "Needs ... no voltage on the pin", and `protocol/spi/wiring.md:111` says "select `e1` before connecting the feed". Change to `e1` → feed → `P`; also update the `P`-warning recovery (`wiring.md:94`) and `edge-cases.md:120`. |
| 4 | **Filled in full** | `usage.md:27-41`: "first bytes after `W` can be the self-test result"; table `0xAA` BAT passed, `0xFC`/`0xFD` BAT failed, `0xFA` command ack; `scancodes.md:213-218`; `edge-cases.md:21`. | The `0xFA` row sits under "first bytes after power-up" although it usually follows a command; minor placement only. |
| 5 | **Partial — new BAT/Echo interaction (Major)** | `edge-cases.md:42-56`: "Use Echo as a presence test" ... `KEYB>r` / `READ: 0xEE` ... "If ... the read does not return `0xEE`, check power and wiring." `usage.md:38-41`: "The first `r` or `(1)` may return `0xAA` (or `0xFC`/`0xFD`); that is not a scancode." | A freshly powered keyboard can return `AA` before the echo. The test should say "read until `0xEE`; a BAT byte may come first", not fail on the first read. |
| 6 | **Filled in full** | `protocol.md:42-56`: "Two bit orders ... On the wire the order is fixed: least-significant bit first ... The prompt's `l`/`L` setting is a value-level reversal"; `entry-and-setup.md:139`: "prompt MSB (`l`); wire is LSB-first"; `usage.md:198-203`; `scancodes.md:227-232`. | No residual; the `0xED` → `0xB7` example is clear. |
| 7 | **Filled in full** | `usage.md:232-248`: "Sending several raw bytes (a string) ... the character's ASCII value, not text ... `a` is `0x61`, not the `A` key ... a keyboard may ignore them or `NACK`." | The capture still shows `abc`, but the text now prevents the text-injection reading. |
| 8 | **Filled in full** | `scancodes.md:41-105`: code-sorted single-byte and prefixed indexes; `:186-197`: full numeric-keypad table. | Minor residual: `:199-200` says "the keypad digits ... otherwise they send the navigation codes above"; keypad 5 has no navigation code in that table. Not the code-index request. |
| 9 | **Filled in full** | `usage.md:178-184`: `WRITE: 0xEENACK`; `:229-230`: "If the command byte reports `NACK` or `TIMEOUT`, do not send the data byte." | None. |
| 10 | **Filled in full** | `usage.md:91-102`: `KEYB>r:2` / `READ: 0x1C 0xF0`; third-read explanation; `:135`: "Both `NONE` words are failed reads; each `0x00` is that read's sentinel." | None. |
| 11 | **Filled in full** | `entry-and-setup.md:167-180`: full 12-name row "GND ... MOSI", "with the firmware's tabs shown as spaces"; "After `W` ... `CLK` ... output ... `MOSI` ... input ... `CLK` low." | The row is normalized to two spaces, not a literal tab-stop expansion; readable, not a misaligned multi-row table. |
| 12 | **Not a defect — rejection upheld** | `wiring.md:73-82`: `KEYB>W` / `POWER SUPPLIES ON` / `Clutch engaged!!!` (no blank line); `entry-and-setup.md:123-128`: `KEYB>w` / `Clutch disengaged!!!` / `POWER SUPPLIES OFF` (no blank line). | `MSG_POWER_SUPPLIES_ON`/`_OFF` are `bp_message_write_buffer` (no newline); `bpBR` is `bp_write_line("")`; `MSG_CLUTCH_*` are line writes. No blank line exists; the captures are correct. My round-1 item 12 was wrong. |
| 13 | **Filled in full** | `protocol.md:139`: "No retry after the command returns" and "the read window may re-sample after a frame error; issue `r` again for a fresh window". | None. |
| 14 | **Filled in full** | `usage.md:268`: `0xF3` "one byte: bits 0–4 rate, bits 5–6 delay"; `:274-275`: delay `00` 250 ms through `11` 1000 ms; `:272`: `0xFF` "usually `0xFA`, then `0xAA`". | None. |
| 15 | **Filled in full** | `scancodes.md` uses "Set 2" throughout; `usage.md:277-279`: "Two different acknowledgements are in play. The frame's ACK/NACK slot is reported on the `WRITE:` line. The keyboard's `0xFA` response is a data byte..." | None. |
| 16 | **Filled in full** | `edge-cases.md:143-155`: "What is not established" table; device-specific `unverified` tags removed; remaining caveats are host timing and the optional v3 build. | The v3 caveat still appears in README/entry/usage/wiring, but each is decision-relevant. |
| 17 | **Filled in full** | `README.md:10-25`: numbered 1–5 "First key in five minutes"; step 2 v3 `m8` selects PIC; step 4 v4 `e3`/`P`, v3 external. | None. |
| 18 | **Filled in full** | `entry-and-setup.md:45-47`: "After the mode number there are no questions ... the `(1)>` in the menu capture below is the shared mode menu's own prompt". | None. |
| 19 | **Filled in full** | `wiring.md:133-137`: "CLOCK stays driven low (it was set low by `W`)" in the `w` paragraph. | None. |
| 20 | **Filled in full** | `README.md:57-67`: v3 "optional (not in the default image)", "8 when enabled", monitor/writes "available in a custom build"; `:15-16`: "On a default v3 build `m8` selects PIC". | "8 when enabled" may shift in a custom build with LCD ordering; minor, not the original finding. |
| 21 | **Filled in full** | `protocol.md:28`: "stop — DATA released (pull-up high)" for both; `:37`: "DATA released (pulled high) is `NACK`"; `usage.md:167`: "the keyboard released DATA (pulled high) in the acknowledge slot". | None. |
| 22 | **Filled in full** | `wiring.md:104-105`: "With the keyboard driving the start bit low, a missing pull-up alone does not cause `*startbit error`; a floating DATA line can." | None. |

## Item 12 — rejected item

The rejection is correct. `MSG_POWER_SUPPLIES_ON` and `MSG_POWER_SUPPLIES_OFF` are `bp_message_write_buffer` macros, so they do not terminate the line; `bpBR` is `bp_write_line("")` and supplies the newline. `switch_psu_on()` therefore prints `POWER SUPPLIES ON` + newline + `Clutch engaged!!!`, not a blank line. The current captures in `wiring.md:73-82` and `entry-and-setup.md:97-102` / `:123-128` match. No doc change is needed; my round-1 item 12 was based on treating those two messages as line writes.

## New problems (max 5)

1. **[major] External-Vpullup order is backwards.** `wiring.md:121` says "feed `Vpullup`/`VPU` from the external supply (0–5 V), select `e1`, then `P`". `e1` refuses while the pin already has voltage (`generic/commands.md:19`), and the SPI/UART pages say "select `e1` before connecting the feed" (`protocol/spi/wiring.md:111`, `protocol/uart/wiring.md:107`). This makes the new external-Vpullup path fail. Fix: `e1` → feed Vpullup → `P`; update `wiring.md:94` and `edge-cases.md:120` to match.
2. **[major] Echo presence test can false-fail on BAT.** `edge-cases.md:42-56` expects one `r` to return `0xEE`; `usage.md:38-41` says the first `r` may return `0xAA`/`0xFC`/`0xFD`. A freshly powered keyboard can put `AA` ahead of the echo, so the test's "does not return `0xEE`, check power and wiring" is a false alarm. Fix: "read until `0xEE`; a BAT byte may come first."
3. **[minor] Keypad-5 navigation overgeneralization.** `scancodes.md:199-200` says the keypad digits "otherwise send the navigation codes above"; keypad 5 has no navigation equivalent in the table. Fix: restrict the statement to keypad 1–4/6–9 and note keypad 5 is keyboard-dependent/undefined.

## Presentation check

- **TAB characters:** 0 bytes in all seven files.
- **Prose wraps:** no non-table, non-code line over 80 columns.
- **Tables / fences / headings:** every table has a consistent column count; code fences are balanced; no heading-level jumps; every relative link and anchor resolves.
- **Menus / option lists:** the `m` menu has a table (`entry-and-setup.md:19-31`) plus the exact capture; the `(0)` macro list has a table (`usage.md:295-298`) plus the exact capture; no unreadable option list.
- **Naming:** "Set 2" is consistent; `ACK` (the frame slot) and `0xFA` (a response byte) are explicitly separated; no duplicate name form remains.
- **Captures:** the `W`/`w` captures have no blank line, which is correct per item 12. The `v` row is complete (12 names) and readable, though it is a normalized two-space list rather than a literal tab-stop expansion; no multi-row capture is ragged.
- **No bench prose / dates / tested-result prose:** no dates, bench/rig/battery narratives, pass counts, "captured on" provenance, or internal log names. The only `unverified` / "What is not established" text is the allowed caveat for host-write timing and the optional v3 build.

## Counts

- Filled in full: **19**
- Partial: **2** (items 3, 5)
- Not addressed: **0**
- Regressed: **0**
- Rejected correctly: **1** (item 12)
- Total: **22**

Two Major findings remain (items 3 and 5), so the process cap extends by one extra turn.
