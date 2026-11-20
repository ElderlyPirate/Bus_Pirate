# Fixes — UART Phase F delta repair

Scope: the Phase F delta audit in `feedback-uart-phase-f.md` (2 major,
2 minor).  Worktree `C:\Users\jschl\dev\bpv4-docssweep`, branch
`docs/combined-sweep`, docs head `d23d145d`; evidence is the worktree's own
`Firmware/` (combined `9b179cb5`).  No edits outside `docs/protocol/uart/`.

**Result: 4 / 4 fixed (2 major, 2 minor); 0 rejected.**  No commit, no push.

---

## 1 → fixed — auto-baud overflow message is `Baud>150k`

**Files:** `entry-and-setup.md`, `errors.md`, `macros.md`, `edge-cases.md`

**Changed:** every old message replaced with
`** Baud>150k: The BP cannot measure above 150000, Done.`

- `entry-and-setup.md` auto-baud condition row:
  > measured rate above about 150000 bps | `** Baud>150k: The BP cannot
  > measure above 150000, Done.`; a reading up to the 256000 table end still
  > maps to 128000 or 256000, above it falls back to 115200
- `entry-and-setup.md` following paragraph: the stale caveat
  "The printed 16,000,000 is the message text, not the check." deleted.
- `errors.md` message table:
  > `** Baud>150k: The BP cannot measure above 150000, Done.`
- `macros.md`:
  > Above about 150000 bps neither line is printed; the firmware prints
  > `** Baud>150k: The BP cannot measure above 150000, Done.`
  (the "printed 16,000,000 … check is at 150 kbaud" caveat deleted).
- `edge-cases.md`:
  > `(4)` above about 150 kbaud | `** Baud>150k: The BP cannot measure above
  > 150000, Done.` | no measurement is reported

**Evidence:** `Firmware/core_messages/messages_v3.s:1559` and
`Firmware/core_messages/messages_v4.s:1655` both store
`"** Baud>150k: The BP cannot measure above 150000, Done."`; the check is
`else if (detected > 150000)` in `Firmware/protocol_uart/uart.c:702`.  A
final grep for `Baud>16m`, `16,000,000`, `16000000`, `message text`,
`printed ceiling` in `docs/protocol/uart/`: 0 matches.

---

## 2 → fixed — a disabled `r` returns `FAILED, NO DATA`, not a stale word

**Files:** `entry-and-setup.md`, `edge-cases.md`

**Changed:** the stale "a word left in the ring … can still be returned by
`r` until `W` clears it" / "ring is not cleared until the next enable"
claims removed.

- `entry-and-setup.md` bring-up bullet:
  > The disable that ended the previous session cleared the queued receive
  > words and last-word flags, so `r` before `W` reports
  > `READ: FAILED, NO DATA0x00`; the overrun counter survives until the next
  > `W`.
- `entry-and-setup.md` `W` bullet:
  > `W` re-runs the setup and enables the port.  Enabling resets the receive
  > ring and the overrun counter.
- `edge-cases.md`:
  > `w` was used | the port is off and the rails are down | `W` re-engages
  > with the stored settings; the `w` cleanup already cleared the receive
  > ring
  >
  > `r` before `W` (after entry or after `w`) | `READ: FAILED, NO DATA0x00`;
  > the disable cleared the ring | run `W`; the overrun counter survives
  > until then

**Evidence:** `Firmware/protocol_uart/uart2.c:204-232`
(`uart2_disable()` turns off the RX interrupt and `UARTEN`/`UTXEN`, then sets
`uart2_rx_head = 0`, `uart2_rx_tail = 0`, `uart2_rx_last_flags = 0`; it does
**not** touch `uart2_rx_overruns`).  `uart2_enable()` clears
`uart2_rx_overruns` at `uart2.c:187`.  `uart_cleanup()` is
`uart2_disable()` (`uart.c:353`), and `w` calls
`bp_current_protocol()->cleanup()` through `switch_psu_off()`
(`Firmware/core_system/proc_menu.c:2311-2322`, `case 'w'`).  With
`head == tail`, `uart_read()` takes the no-data branch and returns 0
(`uart.c:109-137`), which the prompt renders as
`READ: FAILED, NO DATA0x00`.

---

## 3 → fixed — positive `w`-clears / counter-survives statement added

**Files:** `entry-and-setup.md`, `terminal-usage.md`, `errors.md`,
`README.md`

**Changed:**

- `entry-and-setup.md` `w` bullet:
  > The cleanup also clears the queued receive words and last-word flags;
  > the overrun counter survives until the next `W`.
- `terminal-usage.md`:
  > … and it is also cleared when the port is disabled (`w`, a mode change,
  > a reset). … The overrun counter is different: a disable leaves it
  > intact, and the next enable resets it.
- `errors.md`:
  > A `w` disable leaves it intact.
- `README.md`:
  > `w` turns the supplies off, disables the port and clears the queued
  > receive words; the overrun counter is kept until the next `W`.

**Evidence:** same as #2 — disable clears the queue/flags and preserves the
counter; enable clears the counter (`uart2.c:181-188`, `:204-232`).

---

## 4 → fixed — v3 custom-BRG retry documented

**File:** `entry-and-setup.md`

**Changed:** after the v3 divider relationship sentence:
> An empty line takes the default 34.  Zero, a value above 32767, or an
> over-long entry prints `Invalid choice, try again` and re-asks at
> `(34)>`; non-digit characters are ignored.

The 1–32767 range and default 34 were already in the custom-baud table and
the v3/v4 table.

**Evidence:** `Firmware/protocol_uart/uart.c:268-270`
(`MSG_UART_RAW_BRG_PROMPT; brg = getnumber(34, 1, 32767, 0);`).
`getnumber()` in `Firmware/core_system/proc_menu.c:1251-1351`:
empty buffer returns `def` (34); `too_long`, or a magnitude outside
`min`/`max`, prints `BPMSG1211` and `goto again` (re-prompts at `(34)>`);
a non-digit rings the bell and is ignored.  `BPMSG1211` is
`"\r\nInvalid choice, try again"` in both
`Firmware/core_messages/messages_v3.s:575` and `messages_v4.s:599`.

---

## Cross-set notes (for the main session; not edited here)

1. **`docs/SPEC.md` UART row** — still lists v3/v4 as plain `x` with no
   "v3 build-only" note.  Left for the SPEC pass; not edited.
2. **`docs/protocol/binary/sub-protocols.md#shared-command-groups`** — left
   stable; `binary.md` still links it (two references), no change made.
3. **`docs/generic/navigation.md` §Terminal baud** — its separate `b10`
   raw-BRG retry was already correct; no change.

## Presentation / verification

- No TABs; edited prose wraps at ~82 columns or less; captures/tables keep
  their alignment.
- All relative links in `docs/protocol/uart/` resolve.
- No dates, bench prose, pass counts, or tested/verified-result claims added;
  no code references (firmware strings only).
- No files outside `docs/protocol/uart/` were modified by this repair.
