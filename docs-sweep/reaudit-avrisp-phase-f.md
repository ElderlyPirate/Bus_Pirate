# AVRISP — Phase F re-audit (RA-18)

Re-audit of the 18 in-set items from `docs-sweep/feedback-avrisp-phase-f.md`
against the implementer's working tree (`5eea3369` + uncommitted edits;
`docs-sweep/fixes-avrisp-phase-f.md` read).  Verified against the worktree's
own `Firmware/` (`9b179cb5`): `AVRISP_USB_STARTUP_TICKS 12000` = 120 s at the
10 ms tick, armed after attach; the first complete non-empty request switches
to `AVRISP_IDLE_TICKS 500` = 5 s; empty packets rearm neither; the serial
route stays at 5 s; `avrisp_response_end()` rearms the 5 s idle.

Items 17–18 are the cross-set rows for the same two binary pages already
counted as items 8–9; they are listed separately below to match the audit
numbering but carry the same evidence.

## Verdict

**18 of 18 filled in full.**  Item 10 was the only partial at the first
re-check: the two generic entry paragraphs told the serial route to wait for
`03EB:2104`.  The follow-up edit splits the instruction by route
(`README.md:48-50`, `entry.md:71-74`) and is correct; everything else matches
the firmware.  **The AVRISP set is clean.**

## Per-item verdicts

| # | Item | Current text | Verdict |
|---|---|---|---|
| 1 | Central deadline section | "**USB route:** the mkII identity is held for about **120 seconds** (two minutes) from attach until the first complete, non-empty request … nothing renews it: empty OUT packets and an open-but-idle handle do not extend it." / "**Serial route:** about **five seconds** from entry to the first frame." / "**After the first request, both routes:** … about **five seconds** without host traffic." | **full** |
| 2 | USB enter bullet | "The host has about two minutes to enumerate, configure and send the first request.  When the startup budget expires the firmware releases the target and restores the CDC device instead of waiting forever." | **full** |
| 3 | Windows first-use premise | "It is kept for about **120 seconds** from attach until the first complete request — long enough to enumerate, configure and finish a normal binding; empty packets do not extend it.  A binding that resets the device, or takes longer than two minutes, still needs another entry." | **full** |
| 4 | Closing "deadline is already running" paragraph | "enter with `0x08`, wait for `03EB:2104` to appear, then start `avrdude` … The startup budget is not renewable — only the first complete request switches the mode to the five-second idle — but it is two minutes, so a normal binding has room." | **full** |
| 5 | README front page | "**Deadline:** USB — about **two minutes (120 s)** from attach to the first request (covers enumeration and driver binding); serial — about **five seconds** from entry.  After the first request both routes end after about **five seconds** without host traffic…" | **full** |
| 6 | "avrdude still starting" row | "start the tool once the device appears; the USB route holds the startup budget for about 120 s, so a slow start is covered unless a host reset/deconfiguration ended the mode" | **full** |
| 7 | Windows-binding failure row | "First-time Windows driver binding does not finish inside the two-minute startup budget, or the binding resets the device" / "re-enter with `0x08` and repeat the one-pass binding; a completed binding is normally kept…" | **full** |
| 8 | Binary command map (AVRISP sentence) | "ends after about two minutes without a first request on the USB route (about five seconds on the serial route), then after about five seconds of idle once a request has arrived; a reset also ends it" | **full** |
| 9 | Binary edge case (AVRISP entry) | "on the USB route the personality waits about **two minutes** for the first complete request, then ends after about **five seconds** without host traffic; the serial route has about five seconds from entry to the first frame, then the same five-second idle." | **full** |
| 10 | "start `avrdude` immediately" lines | README: "then start `avrdude` once `03EB:2104` appears (USB) or once the entry script has released the port (serial)." — entry: "Then start `avrdude` once `03EB:2104` appears (USB) or once the entry script has released the port (serial) — see the deadlines below." | **full** |
| 11 | Residual five-second framing in the Windows section | "the CDC install is not time-limited"; "inside the 120-second budget"; "the binding exceeded the 120-second budget" | **full** |
| 12 | "binding may need more than one entry" | "if the binding resets the device or takes longer than the two-minute startup budget, re-enter and retry" (all three pages) | **full** |
| 13 | Worked-session intro | "start the host tool once `03EB:2104` appears — the mode keeps the USB identity for about two minutes until the first request, then ends after about five seconds of idle." | **full** |
| 14 | ISP page pointer | "the startup and idle deadlines (USB: about two minutes to the first request; five seconds of idle afterwards)" | **full** |
| 15 | State model attaching exit | "host configures the interface and the first request arrives, or the 120-second startup budget restores CDC" (and the matching ready/ending/failure rows) | **full** |
| 16 | USB transport empty-packet rule | "An empty OUT packet does not start a request and does not extend the 120-second startup budget or renew the five-second idle deadline." | **full** |
| 17 | Binary command map (cross-set row) | same page as item 8 — "… ends after about two minutes without a first request on the USB route (about five seconds on the serial route) …" | **full** |
| 18 | Binary edge case (cross-set row) | same page as item 9 — "… waits about **two minutes** for the first complete request, then ends after about **five seconds** …" | **full** |

## Item 10 follow-up (resolved)

**Item 10 — filled in full after the follow-up edit.**
`README.md:48-50` — "use the host snippet in [entry.md](entry.md), then start
`avrdude` once `03EB:2104` appears (USB) or once the entry script has
released the port (serial)."
`entry.md:71-74` — "Then start `avrdude` once `03EB:2104` appears (USB) or
once the entry script has released the port (serial) — see the deadlines
below.  On the USB route the CDC port disappears when the mkII device
enumerates; on the serial route the same port stays enumerated and must be
free for `avrdude`."

Both sentences now sit in route-neutral paragraphs and split correctly:
USB waits for the re-enumerated `03EB:2104`; serial waits only for the entry
script to close the port, and the following sentence explains that the CDC
port stays enumerated.  `03EB:2104` is no longer asked for on the serial
path.  The surrounding bullets (Enter, Deadline, USB route, Serial route) and
the `[entry.md](entry.md)` / `[host-tools.md](host-tools.md)` links are
unchanged from the first re-check.

## Checks

- The 120 s / 5 s / 5 s model is stated consistently everywhere it appears;
  no page still says the five-second clock starts at entry for USB, and no
  page still puts enumeration inside a five-second window.
- Empty packets reset neither timer: `entry.md` (startup bullet and idle
  bullet), `protocol.md`, `host-tools.md`.  The remaining "only a completed
  request renews the idle window" lines are correct for the post-request
  window and are not a residual.
- Serial entry is 5 s in `entry.md`, `README.md`, the binary pages and
  `command-model.md`; `serial-route.md:39` ("five-second idle deadline")
  remains correct for that route.
- Remaining "five seconds" hits are all correct contexts: `edge-cases.md:13`
  (mid-session), `sessions.md:185` (end of a session with requests),
  `rescue-clock-osccal.md:98` (after the ISP session), `serial-route.md:39`
  (serial route).
- Presentation of the changed passages: no TABs; prose wraps at ≤80 columns
  (only the pre-existing AVR053 URL exceeds it); tables are pipe tables in
  the file's existing style; all relative links resolve; the changed links
  point at existing sections (`§Windows USB first use`, `entry.md`); no
  dates, bench prose or pass counts in any changed passage.
- Not reviewed, per instruction: cross-set items 19–23 (binary README,
  sub-protocols, entering-and-framing, examples, dev design notes).

**The AVRISP set is clean: 18 of 18 filled in full, no not-addressed items,
no regressions.**
