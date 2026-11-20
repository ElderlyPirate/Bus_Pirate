# Phase F re-audit — UART (`docs/protocol/uart/`)

Continuation of the Phase F delta audit.  Inputs: `feedback-uart-phase-f.md`
(4 findings: 2 major + 2 minor), `fixes-uart-phase-f.md`, and the current UART
set.  Verified against the docs worktree's own `Firmware/` (combined
`9b179cb5`).  No fresh full review; no edits made.

**Verdict line: 4 of 4 filled in full.**

No partials, no "not addressed", no regressions.  The set is clean for this
delta; the only open items are the cross-set notes below (not UART defects).

---

## Per-item results

| # | Status | Evidence (current doc quote + location) | Remainder |
|---|---|---|---|
| **1** (major) auto-baud overflow message | **Filled in full** | `entry-and-setup.md:255` — `| measured rate above about 150000 bps | `** Baud>150k: The BP cannot measure above 150000, Done.`; a reading up to the 256000 table end still maps to 128000 or 256000, above it falls back to 115200 |`.  `entry-and-setup.md:262-265` now says only "A measured rate above 150000 prints the overflow message instead of the `Estimated:`/`Calculated:` pair; …" — the old "printed 16,000,000 is the message text" caveat is gone.  `errors.md:112` — `| `** Baud>150k: The BP cannot measure above 150000, Done.` | the measured rate exceeded the 150 kbaud check |`.  `macros.md:94-95` — "Above about 150000 bps neither line is printed; the firmware prints `** Baud>150k: The BP cannot measure above 150000, Done.`".  `edge-cases.md:80` — "`(4)` above about 150 kbaud | `** Baud>150k: The BP cannot measure above 150000, Done.` | no measurement is reported".  Grep for `Baud>16m`, `16,000,000`, `16000000`, `message text`, `printed ceiling`: **0 matches**.  Firmware: both `messages_v3.s:1559` and `messages_v4.s:1655` store exactly the new string; the check is `else if (detected > 150000)` (`uart.c:702`). | none |
| **2** (major) disabled `r` / stale word | **Filled in full** | `entry-and-setup.md:404-407` — "The disable that ended the previous session cleared the queued receive words and last-word flags, so `r` before `W` reports `READ: FAILED, NO DATA0x00`; the overrun counter survives until the next `W`."  `edge-cases.md:36` — "`W` re-engages with the stored settings; the `w` cleanup already cleared the receive ring"; `:37` — "`r` before `W` (after entry or after `w`) | `READ: FAILED, NO DATA0x00`; the disable cleared the ring | run `W`; the overrun counter survives until then".  The old "stale word … can still be returned" / "ring is not cleared until the next enable" text is gone (grep: 0).  Code: `uart2_disable()` turns the RX interrupt and `UARTEN`/`UTXEN` off, then sets `uart2_rx_head = 0`, `uart2_rx_tail = 0`, `uart2_rx_last_flags = 0` (`uart2.c:204-220`); it leaves `uart2_rx_overruns` untouched (`uart2_enable()` resets it at `:187`).  `uart_read()` takes the no-data branch when `head == tail` and returns 0 (`uart.c:109-137`). | none |
| **3** (minor) positive `w`-clears / counter-survives statement | **Filled in full** | `entry-and-setup.md:423-425` — "`w` … disables the port.  The cleanup also clears the queued receive words and last-word flags; the overrun counter survives until the next `W`."  `terminal-usage.md:151-155` — "The ring is reset when the port is enabled … and it is also cleared when the port is disabled (`w`, a mode change, a reset). … The overrun counter is different: a disable leaves it intact, and the next enable resets it."  `errors.md:50` — "A `w` disable leaves it intact."  `README.md:63-65` — "`w` turns the supplies off, disables the port and clears the queued receive words; the overrun counter is kept until the next `W`."  Matches `uart2.c:181-220`. | none |
| **4** (minor) v3 custom-BRG retry | **Filled in full** | `entry-and-setup.md:176-179` — "An empty line takes the default 34.  Zero, a value above 32767, or an over-long entry prints `Invalid choice, try again` and re-asks at `(34)>`; non-digit characters are ignored."  The 1–32767 range and default 34 remain in the custom-baud table (`:167`) and the v3/v4 table (`:444`).  Code: `brg = getnumber(34, 1, 32767, 0)` (`uart.c:270`); `getnumber()` returns the default on an empty buffer, re-prompts with `BPMSG1211` on `too_long` or an out-of-range magnitude, and ignores non-digits (`proc_menu.c:1251-1351`).  Prompt text is the UART mode's `Raw value for BRG (MIDI=127)` (`messages_v3.s:1679`), not the separate `b`-menu prompt. | none |

---

## Presentation / verification

- **TABs:** 0 in the set.  **Prose width:** no line >82 columns outside
  tables/captures.  **Tables/captures:** aligned; the changed passages are
  table rows/bullets, no capture edits.
- **Dates / bench / pass counts / tested-verified prose:** none (only the
  short `unverified`/`not verified` caveats already present).
- **Links:** all relative links resolve; all fragments resolve (GitHub-style
  slug check, 0 issues) — including
  `generic/navigation.md#terminal-baud--b`,
  `hduart.md#no-self-echo--what-is-sent-vs-what-comes-back`,
  `bridge.md#1--transparent-bridge`, and
  `../binary/sub-protocols.md#shared-command-groups`.
- **Firmware authority:** `Baud>150k` / `150000` in both message tables;
  `detected > 150000`; `getnumber(34, 1, 32767, 0)`; disable/enable ring and
  counter behaviour as documented.

## Cross-set notes (unchanged, for the main session)

1. **`docs/SPEC.md` UART row** — still lists v3/v4 as plain `x` with no
   "v3 build-only" note.  SPEC pass.
2. **`docs/protocol/binary/sub-protocols.md#shared-command-groups`** — still
   linked by `binary.md`; keep the anchor stable.
3. **`docs/generic/navigation.md` §Terminal baud** — its separate `b10`
   raw-BRG retry is already correct; no change.
