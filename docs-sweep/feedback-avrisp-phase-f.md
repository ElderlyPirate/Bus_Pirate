# AVRISP — Phase F delta audit (RA-18)

Read-only delta audit of `docs/programmers/avrisp/` against the RA-18
firmware repair in the docs worktree (`5eea3369`, combined `9b179cb5`).
The standing auditor prompt's human-lens shape, the Phase F row for `avrisp`
(`#240`, "USB startup 120 s until first complete request, then 5 s idle;
serial entry 5 s") and worklog §2F were read first.

**Firmware evidence used:** `programmer_avrisp/avrisp_mkii.h`
(`AVRISP_USB_STARTUP_TICKS 12000`, `AVRISP_IDLE_TICKS 500`,
`AVRISP_TIMEOUT_TICKS 100`), `avrisp_mkii.c` (`_T3Interrupt` 10 ms tick;
`avrisp_usb_run()` sets the 120 s budget after attach, switches to the 5 s
idle after the first non-empty request; empty packets do not reset it;
`avrisp_mkii_enter()` leaves the serial route at 5 s; `avrisp_response_end()`
resets to 5 s), and `tests/avrisp_test.c` (RA-18 block: no/slow
configuration, delayed first command, empty packets).  So: **120 s from
attach to the first complete, non-empty request; 5 s idle afterwards on both
routes; 5 s entry window on the serial route.**

## Verdict

**Yes with fixes (delta outstanding).**  The set is otherwise the accepted
22/22 content, but the RA-18 change has not reached the docs: **no page
carries the 120 s startup exception**.  This audit records **23 findings**
(16 in the set: 9 major, 7 minor; plus 7 cross-set pages), all still teaching
the pre-fix single five-second startup rule.  One of them (the Windows
first-use procedure) is built entirely on the old rule, and three
troubleshooting rows document failure modes that no longer exist.

## Findings

### Major

**1. [major] The central deadline section still says 5 s from entry covers
USB enumeration.**
`entry.md:110-113` — "The personality ends after about **five seconds**
without host traffic.  The clock starts **when the mode is entered**, so USB
enumeration and `avrdude` start-up are inside the same window.  Do not enter
the mode and then look up a command."
Why: the USB route now runs on a **120 s** startup budget from attach until
the first complete, non-empty request; 5 s only starts after that request.
The paragraph also makes the 5 s rule apply to the serial route without
saying so.
Fix: split the section into "The startup budget and the idle deadline":
USB = about 120 s (two minutes) from attach to the first complete, non-empty
request (covers enumeration, configuration and a delayed first command;
nothing renews it; empty OUT packets and an idle handle do not extend it);
serial = about 5 s from entry to the first frame; after the first request
both routes = about 5 s without host traffic.  Keep the existing sentence
that a request restarts the idle window when read and when the reply is sent,
and keep "a host reset or deconfiguration ends the personality" (it applies
inside the 120 s too).

**2. [major] "The host must enumerate and configure … inside the same
five-second window" is the old rule and the reason given for the Windows
workaround.**
`entry.md:128-133` — "The host must enumerate and configure the new
interface inside the same five-second window.  A slow first-time driver
binding can exceed it; the firmware then releases the target and restores
the CDC device instead of waiting forever.  A binding that did not finish
is retried by re-entering with `0x08`…"
Why: enumeration, configuration and driver binding have the 120 s budget;
a slow first-time binding is exactly what RA-18 fixed.  What can still end
the mode early is a host reset or a deconfiguration *after* the interface is
configured (the loop leaves on state < configured) — keep that, but do not
attribute it to 5 s.
Fix: "The host has about two minutes to enumerate, configure and send the
first request.  A binding that takes longer, or a host reset/deconfiguration
that ends the mode, is retried by re-entering with `0x08`."

**3. [major] The Windows first-use section is built on the five-second
startup.**
`host-tools.md:109-112` — "the personality ends after about five seconds
without a completed request.  A first-time Windows driver installation can
take longer than that, so a single enter-then-install attempt may not
finish."
Why: the install has about 120 s; the premise of the section (and of its
"may need more than one entry" advice) is the old timer.  A single
enter-then-install now normally finishes; only a binding longer than two
minutes or one that resets/deconfigures the interface fails.
Fix: rewrite the opening to "the mkII identity is kept for about
**120 seconds** from attach until the first complete request — long enough
to enumerate, configure and finish a normal binding; empty packets do not
extend it.  A binding that resets the device, or takes longer than two
minutes, still needs another entry."  Keep the four recovery paths; reword
path 3's "the first enumeration still has to finish inside the window" to
"inside the 120-second budget".

**4. [major] The closing paragraph says the deadline is "already running"
and that only a completed request renews "the five-second window".**
`host-tools.md:135-141` — "After the binding is in place, enter with `0x08`
and start `avrdude` immediately; do not wait for the device to appear in a
device list first, because the deadline is already running. … There is no
keep-alive before the binding: only a completed request renews the
five-second window, and an unbound host cannot send one."
Why: before the first request the governing timer is the 120 s startup
budget, not the 5 s idle; waiting for the device to appear is now safe and
is the correct advice (`avrdude` does not wait for a device itself).  The
"no keep-alive" claim is true only after the first request.
Fix: "enter with `0x08`, wait for `03EB:2104` to appear, then start
`avrdude` (it does not wait for a device that is not yet enumerated).  The
startup budget is not renewable — only the first complete request switches
the mode to the five-second idle — but it is two minutes, so a normal
binding has room."

**5. [major] The front page states the 5 s deadline from entry, with
enumeration inside it.**
`README.md:50-51` — "**Deadline:** about **five seconds** without host
traffic ends the mode, counted from entry (USB enumeration is inside that
window).  Only a completed request renews it on the USB route…"
Why: wrong for USB startup (120 s); correct only after the first request.
Fix: "**Deadline:** USB — about **two minutes (120 s)** from attach to the
first request (covers enumeration and driver binding); serial — about
**five seconds** from entry.  After the first request both routes end after
about **five seconds** without host traffic; only a completed request renews
it on USB, any byte on the serial route."

**6. [major] The "avrdude was still starting" row teaches a 5 s race that
no longer exists on USB.**
`edge-cases.md:15` — "| The mode ended while `avrdude` was still starting |
… | prepare the `avrdude` command first; the five-second clock starts at
entry, so enter the mode only just before running it |"
Why: with the 120 s startup budget the command can be prepared after entry;
the real remaining cause is a host reset/deconfiguration or a >120 s wait.
Fix: "| The mode ended while `avrdude` was still starting | … | start the
tool once the device appears; the USB route holds the startup budget for
about 120 s, so a slow start is covered unless a host reset/deconfiguration
ended the mode |".

**7. [major] The Windows-binding failure row names a five-second window.**
`edge-cases.md:16` — "| First-time Windows driver binding does not finish
inside the five-second window | the mkII device disappears and the CDC port
returns; the binding is incomplete | re-enter with `0x08` and repeat… |"
Why: the budget is 120 s; a binding "not finishing" in 5 s is not a failure.
Fix: "| First-time Windows driver binding does not finish inside the
two-minute startup budget, or the binding resets the device | … | re-enter
with `0x08` and repeat the binding; a completed binding is normally kept … |".

**8. [major] The binary-layer command map states the single 5 s rule.**
`docs/protocol/binary/commands.md:205-207` — "they hand the link to the
personality, which ends after about five seconds without host traffic or on
a reset, then prints `BBIO1` again."
Why: on the USB route an entered personality with no request now lasts about
120 s; serial 5 s.  Cross-set summary, but it is the binary layer's own
description of the two entries.
Fix: "which ends after about two minutes without a first request on the USB
route (about five seconds on the serial route), then after about five
seconds of idle once a request has arrived; a reset also ends it, then it
prints `BBIO1` again."

**9. [major] The binary-layer edge-case page repeats the single 5 s rule.**
`docs/protocol/binary/edge-cases.md:293-296` — "**Behaviour:** the
personality ends after about **five seconds** without host traffic, and a
hardware reset also leaves it."
Why and fix: same as finding 8 — split USB startup (120 s) from the idle
(5 s) and the serial entry (5 s).

### Minor

**10. [minor] Three "start `avrdude` immediately" lines are now
over-strict.**
`entry.md:70` — "Then start `avrdude` immediately (see the deadline below)";
`entry.md:134-136` — "Start `avrdude` as soon as the entry script returns";
`README.md:49` — "then start `avrdude` immediately"; `host-tools.md:136` —
"do not wait for the device to appear in a device list first".
Why: the 120 s budget makes "immediately" unnecessary, and starting before
the device appears is the failure `avrdude` cannot recover from.
Fix: "start `avrdude` once `03EB:2104` appears; the USB route keeps it for
about two minutes."

**11. [minor] Residual five-second framing in the Windows section.**
`host-tools.md:116` — "that install has no five-second deadline";
`:131` — "the first enumeration still has to finish inside the window";
`:190` — "host sees the device only briefly | the first driver binding did
not finish inside the five-second window".
Why: the first two are true but read against the old rule; the failure row
is now wrong (a brief appearance is a reset/deconfiguration, not a 5 s
timeout).
Fix: `:116` "the CDC install is not time-limited"; `:131` "inside the
120-second startup budget"; `:190` trigger "the host reset/deconfigured the
interface, or the binding exceeded the 120-second budget".

**12. [minor] Three "the first binding may need more than one entry" lines
are premised on the old timer.**
`README.md:57`; `edge-cases.md:28`; `host-tools.md:92`.
Why: retrying is still valid, but only for a binding reset or a >120 s
install, not for a 5 s timeout.
Fix: "if the binding resets the device or takes longer than the two-minute
startup budget, re-enter and retry".

**13. [minor] The worked-session intro says 5 s without a request.**
`sessions.md:20-21` — "Enter with `0x08` … and start the host tool at once —
the mode ends after about five seconds without a request."
Why: the first request has 120 s.
Fix: "the mode keeps the USB identity for about two minutes until the first
request, then ends after about five seconds of idle."

**14. [minor] The ISP page points at "the five-second deadline" as the
whole story.**
`isp.md:19-21` — "The host-side snippet, the five-second deadline and the
way back out are owned by [entry.md](entry.md) — do not enter the mode and
then look up a command."
Why: only the post-request idle is five seconds.
Fix: "the startup and idle deadlines (USB: about 120 s to the first
request; 5 s idle afterwards) …".

**15. [minor] The state model calls the attaching exit "the idle
deadline".**
`command-model.md:34` — "| attaching (USB route) | … | `0x08` | host
configures the interface, or the idle deadline restores CDC |".
Why: before the first request it is the 120 s startup budget, not the idle
deadline.
Fix: "host configures the interface and the first request arrives, or the
120-second startup budget restores CDC".

**16. [minor] The USB transport list mentions only the idle deadline for an
empty packet.**
`protocol.md:62-63` — "An empty OUT packet does not start a request and does
not renew the idle deadline."
Why: not false after the first request, but the more useful contract is that
empty packets also do not extend the 120 s startup budget (the worklog makes
this explicit).  A host author reading only this page will miss the startup
rule.
Fix: "An empty OUT packet does not start a request and does not extend the
120-second startup budget or renew the five-second idle deadline."

## Cross-set list

These pages are outside the AVRISP folder and are not edited by this audit;
each carries the same pre-RA-18 summary.

| # | Page | Quote | Fix |
|---|---|---|---|
| 17 | `docs/protocol/binary/commands.md:205-207` | "ends after about five seconds without host traffic or on a reset, then prints `BBIO1` again" | see finding 8 |
| 18 | `docs/protocol/binary/edge-cases.md:293-296` | "ends after about **five seconds** without host traffic" | see finding 9 |
| 19 | `docs/protocol/binary/README.md:42-44` | "`0x08`/`0x09` … return to `BBIO1` after about five seconds of host silence" | "after about 120 s without a first request on USB (5 s on serial), then 5 s idle" |
| 20 | `docs/protocol/binary/sub-protocols.md:35-40` | "ends after about **five seconds** without host traffic or on a reset" | same split as finding 8 |
| 21 | `docs/protocol/binary/entering-and-framing.md:182` | "AVRISP-MKII (`0x08` / `0x09`) … idle timeout (about 5 s) or reset" | "startup budget (about 120 s USB / 5 s serial) or idle (5 s) or reset" |
| 22 | `docs/protocol/binary/examples.md:239` | "they end by idle timeout and then print `BBIO1`" | "they end by the startup budget (USB) or idle timeout, then print `BBIO1`" |
| 23 | `docs/dev/avrisp-design-notes.md:39-42` | "**Idle exit** is about five seconds without host activity.  Empty OUT packets and an idle handle do **not** renew the session" | add "**Startup budget:** the USB route waits about 120 s after attach for the first complete non-empty request (covers enumeration and driver binding); the first request switches to the 5 s idle.  The serial route has only the 5 s entry window.  Empty OUT packets extend neither." |

## What must stay

- The post-request rule: "On the USB route only a completed, non-empty
  request renews the window" — still true and important; do not replace it
  with "any traffic".
- "A request restarts the window when it is read and again when its reply is
  sent" — matches `avrisp_response_end()`.
- "A host reset or an interface deconfiguration ends the personality; it
  does not resume after the interface is configured again" — still true,
  including inside the 120 s; the repair must not over-promise.
- The serial-route pointer "the five-second idle deadline" in
  `serial-route.md:39` — correct as written for that route; leave it.
- The Windows recovery paths themselves (serial first, retry binding,
  pre-stage INF, Linux host) — still valid; only their 5 s premise changes.
- The 1 s command watchdog (`AVRISP_TIMEOUT_TICKS`), the 256-byte request
  cap and the "empty packet does not start a request" contract — unchanged
  by RA-18.

## Presentation

The passages to change carry no TABs, wrap at ≤80 columns, keep their tables
aligned, and contain no dates, bench prose or pass counts.  Only findings 1,
2, 3, 4 and 5 need paragraph rewrites; the rest are one-clause edits.
