# AVRISP — Phase F repair record (RA-18 delta)

**Tree:** `docs/combined-sweep` @ `5eea3369`; the set was verified against the
worktree's own `Firmware/` (combined `9b179cb5`).  **Audit:** `feedback-avrisp-phase-f.md`.
**Scope:** all 16 in-set findings; the seven cross-set pages are recorded only.

## Verdict summary

| # | Decision | File | What changed |
|---|---|---|---|
| 1 | fixed | `entry.md` | deadline section split into startup budget + idle deadline |
| 2 | fixed | `entry.md` | USB enter bullet: two minutes to enumerate/configure/first request |
| 3 | fixed | `host-tools.md` | Windows first-use premise is now the 120 s budget |
| 4 | fixed | `host-tools.md` | closing paragraph: wait for `03EB:2104`, start then |
| 5 | fixed | `README.md` | front-page deadline split (USB 120 s / serial 5 s / idle 5 s) |
| 6 | fixed | `edge-cases.md` | "avrdude still starting" row no longer a 5 s race |
| 7 | fixed | `edge-cases.md` | Windows-binding row: two-minute budget, reset case |
| 8 | fixed | `docs/protocol/binary/commands.md` | AVRISP entry summary split |
| 9 | fixed | `docs/protocol/binary/edge-cases.md` | AVRISP mode-end behaviour split |
| 10 | fixed | `README.md`, `entry.md`, `host-tools.md` | "start immediately" → wait for the device |
| 11 | fixed | `host-tools.md` | residual five-second framing removed |
| 12 | fixed | `README.md`, `edge-cases.md`, `host-tools.md` | "may need more than one entry" → retry only on reset / >120 s |
| 13 | fixed | `sessions.md` | session intro: two minutes to first request |
| 14 | fixed | `isp.md` | "five-second deadline" → startup and idle deadlines |
| 15 | fixed | `command-model.md` | attaching exit = first request or the 120 s budget |
| 16 | fixed | `protocol.md` | empty OUT packet extends neither timer |

**Rejected: 0.**  All 16 audit fixes were applied, using the audit's exact
replacement wording where it gave one.

## Firmware evidence used

- `Firmware/programmer_avrisp/avrisp_mkii.h` — `AVRISP_USB_STARTUP_TICKS 12000`,
  `AVRISP_IDLE_TICKS 500`, `AVRISP_TIMEOUT_TICKS 100` (10 ms tick).
- `Firmware/programmer_avrisp/avrisp_mkii.c` — `_T3Interrupt` decrements the
  activity timer every 10 ms; `avrisp_usb_run()` arms
  `AVRISP_USB_STARTUP_TICKS` after the mkII attach, leaves the budget running
  across empty packets (`avrisp_request_length == 0` → `continue`, without
  rearming), and switches to `AVRISP_IDLE_TICKS` on the first non-empty
  request; `avrisp_response_end()` rearms both the command watchdog and the
  5 s idle; `avrisp_mkii_enter()` starts the serial route at
  `AVRISP_IDLE_TICKS` and `avrisp_serial_run()` re-arms it per received byte.
- `Firmware/programmer_avrisp/tests/avrisp_test.c` — RA-18 block (no/slow
  configuration, delayed first command, empty packets).

## Per-finding record

### 1 — central deadline section — **fixed**

- **File/quote (`entry.md`):** section renamed
  `## The startup budget and the idle deadline`; first bullets now read
  "**USB route:** … about **120 seconds** (two minutes) from attach until the
  first complete, non-empty request … nothing renews it: empty OUT packets and
  an open-but-idle handle do not extend it." / "**Serial route:** about
  **five seconds** from entry to the first frame." / "**After the first
  request, both routes:** … about **five seconds** without host traffic."
- **Kept:** only a completed non-empty request renews the USB idle window;
  any received byte renews the serial route; a request restarts the window
  when read and when the reply is sent.
- **Evidence:** `avrisp_mkii.h` tick constants; `avrisp_mkii.c`
  `avrisp_usb_run()` (startup arm, empty-packet continue, idle switch) and
  `avrisp_response_end()`.

### 2 — "enumerate and configure inside five seconds" — **fixed**

- **File/quote (`entry.md`, USB enter bullet):** "The host has about two
  minutes to enumerate, configure and send the first request.  When the
  startup budget expires the firmware releases the target and restores the CDC
  device instead of waiting forever.  A binding that takes longer, or a host
  reset/deconfiguration that ends the mode, is retried by re-entering with
  `0x08`."
- **Kept:** the separate bullet "A host reset or an interface deconfiguration
  ends the personality; it does not resume after the interface is configured
  again."
- **Evidence:** `avrisp_usb_run()` sets the budget after `avrisp_usb_attach()`
  and then leaves through `restore_cdc:` → `avrisp_usb_switch_to_cdc()` when
  it expires; the loop condition also leaves when `usb_device_state` drops
  below configured.

### 3 — Windows first-use premise — **fixed**

- **File/quote (`host-tools.md`, §Windows USB first use):** "It is kept for
  about **120 seconds** from attach until the first complete request — long
  enough to enumerate, configure and finish a normal binding; empty packets do
  not extend it.  A binding that resets the device, or takes longer than two
  minutes, still needs another entry."
- **Recovery paths kept:** serial first, retry binding, pre-stage INF, Linux.
  Path 3 now ends "inside the 120-second budget" (see finding 11).
- **Evidence:** as finding 1; the budget is armed after attach and only the
  first non-empty request selects the 5 s idle.

### 4 — closing "deadline is already running" paragraph — **fixed**

- **File/quote (`host-tools.md`):** "enter with `0x08`, wait for `03EB:2104`
  to appear, then start `avrdude` (it does not wait for a device that is not
  yet enumerated). … The startup budget is not renewable — only the first
  complete request switches the mode to the five-second idle — but it is two
  minutes, so a normal binding has room."
- **Removed:** "do not wait for the device to appear" and "there is no
  keep-alive before the binding" (true only after the first request).
- **Evidence:** startup budget set in `avrisp_usb_run()`; idle switch at the
  first non-empty request (`avrisp_request_length != 0`).

### 5 — README front page — **fixed**

- **File/quote (`README.md`):** "**Deadline:** USB — about **two minutes
  (120 s)** from attach to the first request (covers enumeration and driver
  binding); serial — about **five seconds** from entry.  After the first
  request both routes end after about **five seconds** without host traffic;
  only a completed request renews it on USB, any byte on the serial route."
- **Also:** the file-table row now says "startup and idle deadlines".
- **Evidence:** findings 1–2.

### 6 — "mode ended while avrdude was still starting" — **fixed**

- **File/quote (`edge-cases.md`):** row recovery is now "start the tool once
  the device appears; the USB route holds the startup budget for about 120 s,
  so a slow start is covered unless a host reset/deconfiguration ended the
  mode".
- **Evidence:** startup budget vs. the `usb_device_state` leave condition in
  `avrisp_usb_run()`.

### 7 — Windows-binding failure row — **fixed**

- **File/quote (`edge-cases.md`):** trigger "First-time Windows driver
  binding does not finish inside the two-minute startup budget, or the
  binding resets the device"; recovery "re-enter with `0x08` and repeat the
  one-pass binding; a completed binding is normally kept …".
- **Evidence:** findings 1 and 2.

### 8 — binary-layer command map — **fixed** (in-set item)

- **File/quote (`docs/protocol/binary/commands.md`):** "… ends after about two
  minutes without a first request on the USB route (about five seconds on the
  serial route), then after about five seconds of idle once a request has
  arrived; a reset also ends it, then it prints `BBIO1` again."
- **Boundary:** only the AVRISP-specific sentence in that page was touched.
- **Evidence:** findings 1–2.

### 9 — binary-layer edge case — **fixed** (in-set item)

- **File/quote (`docs/protocol/binary/edge-cases.md`):** "on the USB route the
  personality waits about **two minutes** for the first complete request, then
  ends after about **five seconds** without host traffic; the serial route has
  about five seconds from entry to the first frame, then the same five-second
  idle."
- **Boundary:** only the `## AVRISP-MKII mode ends by itself` entry was
  touched.
- **Evidence:** findings 1–2.

### 10 — "start `avrdude` immediately" lines — **fixed**

- **Files/quotes:**
  - `README.md`: "then start `avrdude` once `03EB:2104` appears."
  - `entry.md`: "Then start `avrdude` once `03EB:2104` appears (see the
    deadlines below)."; USB bullet "Start `avrdude` once `03EB:2104` appears.
    … the USB route keeps the identity for about two minutes, so a slow start
    is covered."
  - `host-tools.md`: covered by finding 4.
- **Evidence:** the device identity exists for the whole 120 s budget; starting
  before it appears is the failure `avrdude` cannot recover from.

### 11 — residual five-second framing in the Windows section — **fixed**

- **Files/quotes (`host-tools.md`):**
  - "the CDC install is not time-limited";
  - path 3 "inside the 120-second budget";
  - failure-map trigger "the host reset/deconfigured the interface, or the
    binding exceeded the 120-second budget".
- **Evidence:** findings 1–2; the serial route has no USB re-enumeration.

### 12 — "binding may need more than one entry" — **fixed**

- **Files/quotes:**
  - `README.md`: "A first-time Windows libusb/WinUSB binding is retried by
    re-entering if it resets the device or takes longer than the two-minute
    startup budget".
  - `edge-cases.md`: "if the binding resets the device or takes longer than the
    two-minute startup budget, re-enter and retry".
  - `host-tools.md` driver table: "if the first binding resets the device or
    exceeds the two-minute startup budget, re-enter and retry".
- **Evidence:** retry is still valid, but only for a reset/deconfiguration or a
  >120 s install; a 5 s timeout no longer exists on the USB route.

### 13 — worked-session intro — **fixed**

- **File/quote (`sessions.md`):** "start the host tool once `03EB:2104`
  appears — the mode keeps the USB identity for about two minutes until the
  first request, then ends after about five seconds of idle."
- **Evidence:** findings 1–2.

### 14 — ISP page pointer — **fixed**

- **File/quote (`isp.md`):** "The host-side snippet, the startup and idle
  deadlines (USB: about two minutes to the first request; five seconds of idle
  afterwards) and the way back out are owned by [entry.md](entry.md)."
- **Removed:** "do not enter the mode and then look up a command" (no longer
  necessary inside the 120 s budget).
- **Evidence:** finding 1.

### 15 — state model attaching exit — **fixed**

- **File/quote (`command-model.md`):** attaching row left-by is now "host
  configures the interface and the first request arrives, or the 120-second
  startup budget restores CDC"; the ready row now says the pre-first-request
  exit is the startup budget and the post-request exit is the five-second idle;
  the ending row and the failure table say "startup budget or idle deadline".
- **Evidence:** `avrisp_usb_run()` two-phase loop (budget → first non-empty
  request → idle), `avrisp_response_end()`.

### 16 — USB transport empty-packet rule — **fixed**

- **File/quote (`protocol.md`):** "An empty OUT packet does not start a request
  and does not extend the 120-second startup budget or renew the five-second
  idle deadline."
- **Evidence:** in `avrisp_usb_run()`, an empty request returns before
  `avrisp_activity_ticks = AVRISP_IDLE_TICKS`, so it renews neither the
  startup budget nor the idle window.

## Residual consistency edits (same rule, no separate audit finding)

| File | Quote | Why |
|---|---|---|
| `host-tools.md` | failure-map rows "start the host tool once the device appears"; "on Windows, re-enter and retry if the binding resets the device or exceeds the two-minute startup budget"; "started before the device appeared, or the mode ended (host reset/deconfiguration, or the startup budget expired)" | last stale "at once"/"more than one entry"/5 s-timing lines |
| `entry.md` | "1. the startup budget or the idle deadline expires,"; page intro "startup and idle deadlines" | the exit cause is now two deadlines |
| `command-model.md` | "Only the mode-level events (the startup budget or idle, USB stall/reset, board reset) end the personality." | keep the failure summary consistent with the state table |
| `host-tools.md`, `protocol.md` | related-doc labels "the startup and idle deadlines" | link descriptors matched the new section |

## Cross-set list — recorded, not edited (7)

These pages were outside this repair's edit boundary.  All still need the
same split in their owning set.

| # | Page | Current text | Required fix |
|---|---|---|---|
| 17 | `docs/protocol/binary/commands.md` | "ends after about five seconds without host traffic or on a reset, then prints `BBIO1` again" | **covered here** by finding 8 (AVRISP-specific sentence only) |
| 18 | `docs/protocol/binary/edge-cases.md` | "ends after about **five seconds** without host traffic" | **covered here** by finding 9 |
| 19 | `docs/protocol/binary/README.md:42-44` | "return to `BBIO1` after about five seconds of host silence" | "after about 120 s without a first request on USB (5 s on serial), then 5 s idle" |
| 20 | `docs/protocol/binary/sub-protocols.md:35-40` | "ends after about **five seconds** without host traffic or on a reset" | same split as finding 8 |
| 21 | `docs/protocol/binary/entering-and-framing.md:182` | "idle timeout (about 5 s) or reset; `BBIO1` printed" | "startup budget (about 120 s USB / 5 s serial) or idle (5 s) or reset" |
| 22 | `docs/protocol/binary/examples.md:239` | "they end by idle timeout and then print `BBIO1`" | "they end by the startup budget (USB) or idle timeout, then print `BBIO1`" |
| 23 | `docs/dev/avrisp-design-notes.md:39-42` | "**Idle exit** is about five seconds without host activity …" | add a startup-budget bullet: 120 s after attach for the first complete non-empty request, then 5 s idle; serial has the 5 s entry window; empty packets extend neither |

**Main-session action:** apply items 19–23 in the owning sets (`binary` and
`dev`).  Items 17–18 are already fixed here because the audit numbered them
as in-set findings 8–9; no further edit is needed for those two.  In
`docs/dev/avrisp-design-notes.md` the related-docs table also labels
`entry.md` "entry bytes, idle deadline and exit" — update that label with the
same startup-budget wording.

## Presentation / checks

- No TABs in any edited file; prose (non-table) lines wrap at ≤80 columns (the
  only >80 prose line in the set is the pre-existing AVR053 URL in
  `rescue-clock-osccal.md`).
- Relative links and anchors checked across the edited pages; all resolve.
- User pages carry no code references or bench/date/pass-count prose; firmware
  file/symbol references appear only in this local record.
- Not run: a hardware check or a host-tool run — this is a documentation
  repair against the source and the RA-18 test description.

## Files changed

- `docs/programmers/avrisp/README.md`
- `docs/programmers/avrisp/entry.md`
- `docs/programmers/avrisp/host-tools.md`
- `docs/programmers/avrisp/edge-cases.md`
- `docs/programmers/avrisp/sessions.md`
- `docs/programmers/avrisp/isp.md`
- `docs/programmers/avrisp/command-model.md`
- `docs/programmers/avrisp/protocol.md`
- `docs/protocol/binary/commands.md` (AVRISP sentence; in-set finding 8)
- `docs/protocol/binary/edge-cases.md` (AVRISP entry; in-set finding 9)

## Re-audit residual — item 10 (minor) — **fixed**

- **Finding:** the post-entry "start `avrdude`" instruction in `README.md` and
  `entry.md` said "once `03EB:2104` appears" in bullets that cover both entry
  routes.  On the serial route `03EB:2104` never appears: the CDC port stays
  enumerated.
- **Decision:** fixed by splitting the instruction per route; the surrounding
  bullets and links are unchanged.
- **`README.md` (new wording):**

  ```
    terminal emulator normally cannot send NULs — use the host snippet in
    [entry.md](entry.md), then start `avrdude` once `03EB:2104` appears (USB)
    or once the entry script has released the port (serial).
  ```

- **`entry.md` (new wording):**

  ```
  Then start `avrdude` once `03EB:2104` appears (USB) or once the entry script
  has released the port (serial) — see the deadlines below.  On the USB route
  the CDC port disappears when the mkII device enumerates; on the serial route
  the same port stays enumerated and must be free for `avrdude`.
  ```

- **Evidence:** the USB route switches to the mkII descriptors, so the host
  binds and opens `03EB:2104`; the serial route (`avrisp_serial_run()`) never
  re-enumerates and reads the same CDC port, which the entry script must
  close/release first (also stated in `serial-route.md` §Entry step 3).
