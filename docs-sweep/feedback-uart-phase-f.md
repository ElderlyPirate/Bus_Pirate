# Phase F delta audit — UART (`docs/protocol/uart/`)

Scope: only the statements changed by the Phase F repairs (RA-4 auto-baud
message, RA-1 v3 raw-BRG range/prompt, RA-5 `w`-disable ring/flags).  Verified
against the docs worktree's own `Firmware/` (combined `9b179cb5`), not the PR
worktrees.  No full re-review; no edits made.

**Verdict: yes with fixes.** The set is otherwise sound and the RA-1/RA-5
positive statements are mostly present; five stale auto-baud-message passages
and two stale disabled-`r` passages still contradict the final firmware.

---

## Findings

### 1. [major] Auto-baud overflow message is stale in five places — firmware says `Baud>150k`, not `Baud>16m`

**Locations / quotes:**
- `docs/protocol/uart/entry-and-setup.md:252`
  > `| measured rate above about 150000 bps | `** Baud>16m: The BP cannot measure above 16000000, Done.`; a reading up to the 256000 table end still maps to 128000 or 256000, above it falls back to 115200 |`
- `docs/protocol/uart/entry-and-setup.md:262`
  > "The printed 16,000,000 is the message text, not the check."
- `docs/protocol/uart/errors.md:111`
  > `| `** Baud>16m: The BP cannot measure above 16000000, Done.` | the measured rate exceeded the 150 kbaud check |`
- `docs/protocol/uart/macros.md:94-96`
  > "Above about 150000 bps neither line is printed; the firmware prints
  > `** Baud>16m: The BP cannot measure above 16000000, Done.`  The printed
  > 16,000,000 is the message text; the check is at 150 kbaud."
- `docs/protocol/uart/edge-cases.md:80`
  > `| `(4)` above about 150 kbaud | `** Baud>16m: The BP cannot measure above 16000000, Done.` | the printed ceiling is the message text; the check is at 150 kbaud |`

**Why:** both final message tables now contain
`** Baud>150k: The BP cannot measure above 150000, Done.`
(`Firmware/core_messages/messages_v3.s:1559`,
`messages_v4.s:1655`).  The check is `detected > 150000`
(`Firmware/protocol_uart/uart.c:702-703`).  The `16m`/`16,000,000` text and the
"printed … is the message text, not the check" caveats are stale and now
self-contradictory.

**Exact fix:** replace every `** Baud>16m: The BP cannot measure above
16000000, Done.` with `** Baud>150k: The BP cannot measure above 150000,
Done.`; delete the three caveats (entry-and-setup line 262, macros lines 95-96,
edge-cases recovery cell).  Keep the verified boundary sentence in
entry-and-setup (a reading up to the 256000 table end maps to 128000 or 256000;
above it falls back to 115200).  In `edge-cases.md:80` the recovery cell
becomes "no measurement is reported".

### 2. [major] The docs still claim a disabled `r` can return a stale word; the `w` disable clears the ring and last-word flags

**Locations / quotes:**
- `docs/protocol/uart/entry-and-setup.md:402-404`
  > "A word left in the ring by an earlier session can still be returned by `r` until `W` clears it."
- `docs/protocol/uart/edge-cases.md:37`
  > `| `r` before `W` (after entry or after `w`) | a stale word from an earlier session can still be returned | the port is disabled but the ring is not cleared until the next enable; `W` clears it |`
- `docs/protocol/uart/edge-cases.md:36` (implies the ring survived `w`)
  > `| `w` was used | the port is off and the rails are down | `W` re-engages with the stored settings and clears the ring |`

**Why:** `uart2_disable()` sets `uart2_rx_head = 0`, `uart2_rx_tail = 0` and
`uart2_rx_last_flags = 0` after `IEC1bits.U2RXIE = OFF`, `UARTEN = OFF` and
`UTXEN = OFF` (`Firmware/protocol_uart/uart2.c:204-232`).  `w` calls
`uart_cleanup()` → `uart2_disable()` (`uart.c:353`,
`Firmware/core_system/proc_menu.c:2317`).  A disabled `r` sees an empty ring
and `uart_read()` prints `FAILED, NO DATA` (`uart.c:109-137`).  The overrun
count is **not** cleared by the disable; `uart2_enable()` clears it
(`uart2.c:187`).  So the "stale word" claim is wrong, and after `W` the
observable is `READ: FAILED, NO DATA0x00`.

**Exact fix:** replace the sentence in `entry-and-setup.md:402-404` with:
"The disable that ended the previous session cleared the queued receive words
and last-word flags, so `r` before `W` reports `READ: FAILED, NO DATA0x00`;
the overrun counter survives until the next `W`."  Rewrite the
`edge-cases.md:37` row to that same symptom/recovery, and make `:36` say `W`
re-engages with the stored settings (the `w` cleanup already cleared the
ring).

### 3. [minor] The positive `w`-clears-the-ring / counter-survives statement is missing from the bring-up, buffer and counter pages

**Locations / quotes:**
- `docs/protocol/uart/entry-and-setup.md:420-421`
  > "`w` does the reverse: it runs the mode cleanup, switches the rails off and disables the port.  The stored settings remain; `W` re-engages with them."
- `docs/protocol/uart/terminal-usage.md:151-153`
  > "The ring is reset when the port is enabled: after `W`, after a binary speed/frame change, and when `(4)` re-runs the setup.  Words queued before one of those steps are lost."
- `docs/protocol/uart/errors.md:46-50`
  > "The counter resets whenever the port is enabled — after `W`, after a binary speed/frame change, and when `(4)` re-runs the setup."
- `docs/protocol/uart/README.md:63` (one-liner, optional)
  > "`w` turns the supplies off and disables the port."

**Why:** RA-5 makes the disable clear the queued words/flags, while the
overrun counter survives until the next enable.  The current pages describe
only the enable-side reset, so a reader cannot tell what `w` does to the ring
or the count.

**Exact fix:** add to the `w` bullet in `entry-and-setup.md`: "The cleanup
also clears the queued receive words and last-word flags; the overrun counter
survives until the next `W`."  In `terminal-usage.md` say the ring is also
cleared when the port is disabled (`w`, a mode change, a reset).  In
`errors.md` add "A `w` disable leaves the counter intact."  Optionally extend
the README `w` line to "…disables the port and clears the queued receive
words."

### 4. [minor] The v3 custom-BRG paragraph does not say what an invalid entry does

**Location / quote:** `docs/protocol/uart/entry-and-setup.md:174-176`
> "On **v3** the value is the divider itself.  The relationship is `rate = 4,000,000 / (BRG + 1)`, so `127` is 31250 bps (MIDI), `34` is about 115200 bps, and `416` is about 9592 bps."

**Why:** the v3 prompt is `getnumber(34, 1, 32767, 0)`
(`Firmware/protocol_uart/uart.c:270`).  Zero, above 32767, or an over-long
entry prints `Invalid choice, try again` and re-asks at `(34)>`; an empty line
takes the default 34; non-digit characters are ignored (bell).  The range
1–32767 is already in the table (line 167) and the v3/v4 table (line 439), but
the retry behaviour is absent.  (The generic `b` menu's separate
`Enter raw value for BRG` path is already documented at
`docs/generic/navigation.md:244-249`; this is the UART mode's own
`Raw value for BRG (MIDI=127)` prompt.  Note: the task summary's
"missing/malformed reopen" is accurate for the `b` command's initial failure,
not for this prompt's empty/non-numeric line.)

**Exact fix:** add after the v3 relationship sentence: "The prompt re-asks at
`(34)>` with `Invalid choice, try again` when the entry is zero, above 32767,
or too long; an empty line takes the default 34."

---

## Cross-set list (for the main session)

1. **`docs/SPEC.md` UART row** — still shows v3/v4 as plain `x` with no
   "v3 build-only" note (line 50).  SPEC pass.
2. **`docs/generic/navigation.md` §Terminal baud** — already documents the
   `b10` raw-BRG retry (`Enter raw value for BRG`, 1–32767, `(34)>`,
   missing/zero/malformed/>32767 reopens).  Verified correct; no change.
   The UART mode's `Raw value for BRG (MIDI=127)` prompt is a separate path
   (finding 4).
3. **`docs/protocol/binary/sub-protocols.md#shared-command-groups`** — UART
   `binary.md` links this anchor; keep it stable in the binary pass.
4. No old `Baud>16m`/`16,000,000` text remains in `docs/generic/` or
   `docs/SPEC.md` (checked); the fix is UART-set-only.

---

## Verified correct (no change)

- v3 one-line `brg` is a **raw divider** 1–32767; v4 is bps 62–65535
  (`entry-and-setup.md:194-207`; `uart.c:174-187`).
- Auto-baud boundary: `> 150000` prints the overflow message; an entry-time
  reading up to 256000 maps to 128000/256000; above 256000 falls back to
  115200 (`uart.c:542-560`, `:700-712`).  The docs' boundary sentence is
  accurate once the message text is fixed.
- `Calculated:` can report 31250 (the common-rate table includes it,
  `uart.c:51-53`).
- Binary `0x6x` operands `0xA`–`0xF` are answered `0x00` and do not
  reconfigure (`uart.c:897-910`).
- The `i`/`v` settings-header homes and the `0x0F` `ART1` bridge trap are
  unchanged and correct for this delta.

## Presentation (changed passages)

No TABs; the edited passages are table rows/bullets that stay aligned; no
prose over ~82 columns outside tables; no dates, bench prose, pass counts or
tested/verified claims.  All relative links in the set resolve.
