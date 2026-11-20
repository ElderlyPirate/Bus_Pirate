# XSVF docs — round 1 feedback

**Reader:** engineer with a v3/v4 Bus Pirate, an XSVF/SVF file, and no prior
context on this doc set.

**Verdict: yes with fixes (one blocker).** The set is unusually clear about the
hard parts — firmware player vs host player, VREG, 3.3 V push-pull, the
400-bit/4096-byte limits and the `XCOMMENT` trap — but one documented main-path
combination (`-x` plus `-f`) can desync the following run and print
`Success!` without configuring anything, and the exit-status story is not the
shipped tool's actual behaviour. Fix the top items and this is a strong set.

## Task log

### 1. README first impression

- **Where:** `docs/tools/xsvf/README.md`, then `workflow.md` and
  `converter.md`.
- **How fast:** under a minute to answer "what is this and does my board
  support it."
- **Success:** partial. I knew it was an XSVF file player, v4 only, Windows or
  Linux host, and that I should start at `workflow.md` (or the converter if I
  only had an SVF).
- **What was unclear:** where the host players and converter actually come
  from. The set names the folders only in `players.md`, and "what ships" is
  there too; nothing says "in the firmware repository under `scripts/`, and the
  repo does not install them." A v3 owner also gets "v4 only" but no one-line
  "use OpenOCD's `svf` instead" in the README.

### 2. End-to-end workflow and the four worked sessions

- **Where:** `workflow.md` steps 1–7; `converter.md`; `players.md` worked
  commands; the probe-only and failure sessions.
- **How fast:** about 10 minutes to read the whole path; the four sessions are
  followable.
- **Success:** yes for the run/probe/failure transcripts. The conversion step
  is not in the same session: a reader has to go workflow → converter →
  workflow. That is one extra hop, not a dead end, but it weakens the
  "end-to-end" promise.
- **What was unclear:** how to satisfy the 400-bit gate when iMPACT made the
  XSVF directly (the docs point at the converter's `-a` listing); why the
  Windows worked command combines `-x` with `-f` when the scan is a single
  unsized read; what the number in `Sending N Bytes (D029)` actually counts.

### 3. Enter/exit the firmware player and read the result bytes, XCOMMENT

- **Where:** `player.md` entry/exit/result tables; README limits; `limits-and-errors.md`;
  `protocol/jtag/xsvf.md` for the byte detail.
- **How fast:** entry and result interpretation were immediate. Exit was the
  slow part.
- **Success:** entry (`BBIO1`, `0x18`, `XSV1`) and the result-byte table are
  clear. I found the `XCOMMENT` trap in one hop from the README and the action
  ("reset the board") is unambiguous. The `0x0F` exit is not something a human
  can perform after the host player closes the port; the only practical exit is
  the reset button. The README's "press reset, then `W`" is also wrong because
  `W` is refused in `HiZ>`.
- **What was unclear:** how to send `0x0F` from a normal terminal; what to do
  for `XSVF_ERROR_UNKNOWN` beyond "send only chunks" (that is host-protocol
  advice, not a user action).

### 4. Host players: `-x`, `-r`, `-f`, chain scan, exit status

- **Where:** `players.md` options/serial/output/exit tables; the two worked
  commands; the chain-scan section; `players.md` "What a host player does."
- **How fast:** options and platform differences were fast. Exit status and
  chain scan took reading the shipped source to trust.
- **Success:** mostly. The platform difference is clear: Windows requires `-f`
  even with `-x`/`-r`, cross-platform runs them alone; the Windows build prints
  `ok`, cross-platform does not print `ok` after the entry lines; Windows exits
  0 after any run, cross-platform claims 0/1/2.
- **What was unclear:** the chain scan is one unsized read, and the docs never
  say what happens to the unread bytes before the file run; the exit-status
  table omits setup failures and the early-EOF case; the `ok` row reads
  absolute even though the cross-platform transcript shows `ok` later.

### 5. Wiring and limits

- **Where:** `wiring.md`, `workflow.md` step 2, `protocol/jtag/wiring.md`,
  `limits-and-errors.md`, `protocol/jtag/xsvf.md`.
- **How fast:** fast. Five wires, no pull-up, one rail, 3.3 V push-pull only,
  400 bits, 4096 bytes, 63 devices, no TCK setting.
- **Success:** yes, I would wire from this. The rails/VREG/back-drive warning
  and the 5 V-target caveat are the right level of scary.
- **What was unclear:** whether the on-board pull-ups survive binary entry
  (`wiring.md` and `protocol/jtag/edge-cases.md` disagree); the exact terminal
  steps to release a leftover AUX/AUX1 reset (the table says "drive it high"
  and "release it at the terminal," not `c`/`k` + `A`); how to see the
  `VREG too low` message from inside the player (you have to reset first).

### 6. Rendering scan

- **Where:** all seven files in `docs/tools/xsvf/`, plus their captures.
- **How fast:** fast.
- **Success:** **no TAB characters, no dates, no bench/rig/battery/capture
  narratives, no pass counts, no "tested"/"captured on" provenance, and no
  internal log names.** The only "log" reference is `run_log.bat`, a shipped
  batch file, not a log. The only verification language is the short
  "unverified"/"not verified" caveat. Captures are space-aligned; the option
  lists are tables, not bullet chains. This is clean.
- **What was unclear / what did not render:** three prose lines exceed ~80
  columns; parenthetical notes sit inside code fences and look like output; the
  Linux worked command uses a `>` prompt; the chain-scan example is not a valid
  IDCODE; the host banner has the same "XSVF Player" name as the firmware mode;
  one "What ships" cell is 139 characters wide.

### 7. README navigation test

- **Where:** README table and "Start here" links → each page.
- **How fast:** one click to workflow, one to player, one to host players, one
  to wiring, one to limits. All relative links and anchors I followed resolve.
- **Success:** yes. The README is a real map, not a manual, and the file table
  tells me which page owns which question.
- **Dead ends / doubling back:** acquiring the tools (no README block); the
  `0x0F` exit (no raw-serial instruction); the shift check for iMPACT-generated
  XSVF (only the converter has `-a`); the v3 alternative is only in
  `limits-and-errors.md`/protocol, not the README. The four worked sessions are
  split between `players.md` and `workflow.md`, so the "worked sessions" page is
  not the one place to see them.

## Findings

### 1. [severity: blocker] `-x` plus `-f` can desync into a false `Success!`

**Location:** `docs/tools/xsvf/players.md:139-141`:
> The host prints whatever one read returned, so a long line can be truncated; do not use it as a chain inventory.

and `docs/tools/xsvf/workflow.md:70-71`:
> Add `-x` to print a chain scan before the run.  It is informational; the file is what configures the target.

The Windows worked command at `players.md:206` combines them:
> `BP4_Full_bitbang_XSVFplayer_Win.exe -p COM5 -s 115200 -f xc95test.xsvf -x -r`

**Problem:** the scan is a single `serial_read`; the host does not use the
count byte and does not drain the port before sending `0x03`. If the scan line
is short (the docs themselves say it can be), the unread count/IDCODE bytes
remain in the port and the run loop reads them as the player's reply. A
leftover `0x00` prints `End of operation reply: XSVF_ERROR_NONE` and
`Success!` even though the file was never streamed. Other leftovers print a
spurious error. The docs call the scan "informational" but never warn that a
short scan poisons the following run.

**Fix:** add a warning in `players.md` and `workflow.md`: "Run `-x` alone, or
verify the line contains `1 + 4 × devices` bytes before combining it with
`-f`; a short scan leaves bytes that the run reads as its result." Better:
fix the host to use the count byte and flush before `0x03`. At minimum, change
the worked command to `-r` only and show `-x` as a separate invocation.

### 2. [severity: major] README says `W` after reset, but `W` is refused in HiZ

**Location:** `docs/tools/xsvf/README.md:42-44`:
> The host player does not leave the firmware player.  **Press reset** to get
> the terminal back, then `W` if you want the terminal to power the rails
> again.

`workflow.md:178-180` has the correct version:
> While the player owns the port, `W` cannot be reached.  Reset to the
> terminal, then `W` if you want the terminal to power the rails again — but
> enter a mode first, because `W` is refused in `HiZ>`.

**Problem:** reset returns to `HiZ>`, where `W` prints `Command not used in
this mode`. A reader following the README's first-run exit hits a refusal.

**Fix:** mirror the workflow wording in the README, or remove the `W`
suggestion: "Press reset; the rails are off. To re-power them, enter a mode
(`m`) and then `W` (which runs that mode's setup), or reconnect the target's
own supply."

### 3. [severity: major] "Send `0x0F` twice" is not a human exit

**Location:** `workflow.md:161-167`:
> 2. Or send `0x0F` twice — only after the player is back at its command loop
> (a result was printed, or the ~5 s stall has expired):
> - the first `0x0F` resets the chain and returns to `BBIO1` silently;
> - the second `0x0F` replies `0x01` and runs the terminal reset...

Also `players.md:186-190`: "The host players close the port without sending
`0x0F`... The `0x0F` escape works only once the player is back at its command
loop"; `README.md:81-82`; `limits-and-errors.md:125` ("send `0x0F` or press
reset").

**Problem:** the host players close the port. A normal terminal cannot send a
raw `0x0F`; the docs never say to reopen the port with a raw-serial terminal or
script. A user may type `0x0F` or press Ctrl-O. The reset button is the only
exit that is actually described end to end.

**Fix:** label the `0x0F` path as "custom host/script only" and either give a
concrete raw-serial recipe (open the port, write `\x0f`, wait, write `\x0f`) or
say "press reset; the stock players do not need the `0x0F` path." A small
"human exit vs host byte" table would settle it.

### 4. [severity: major] Cross-platform exit status is not the tool's actual contract

**Location:** `players.md:40`:
> | Exit status | `0` after a run; non-zero for a setup failure | `0` clean, `1` firmware error, `2` host error |

and `players.md:184`:
> | cross-platform player | `0` clean; `1` the firmware returned an error; `2` a host or transport failure |

**Problem:** the shipped cross-platform tool returns `-1` (255) for setup
failures (no port, no `BBIO1`, no `XSV1`, file open), and it leaves the status
at `0` for `End of file reached.` even though no result byte was read. The
table says `0` is clean and `2` covers host failures. A script that trusts `$?`
will accept an incomplete file (0) and miss a missing port (255). This is
exactly the "meaningful exit status" the page promises.

**Fix:** document the actual mapping: `0` = clean result **or** `End of file
reached.` (no result; check the text), `1` = firmware error, `2` = transport/no
reply, `255` = setup failure. Or fix the tool to return `2` on early EOF and
setup. Add the sentence: "Do not trust `0` alone; the success check is
`End of operation reply: XSVF_ERROR_NONE`."

### 5. [severity: major] The 400-bit gate has no check on the preferred iMPACT path

**Location:** `workflow.md:12-19`:
> Preferred: generate the XSVF directly with iMPACT...
> **Shift gate:** the largest `XSDRSIZE` ... must be **400 bits or less** ...
> The `-a` text listing shows the values ([converter.md](converter.md#checking-the-shift-size)).

`converter.md:91-104`:
> Use the `-a` text listing to check it:
> 1. Convert with `-a`.

**Problem:** `-a` is an option of `svf2xsvf502`, which needs an SVF input. If
iMPACT produced the XSVF directly, there is no text listing and no documented
way to find the largest `XSDRSIZE`. The mandatory gate becomes a dead end for
the preferred path.

**Fix:** add: "iMPACT writes no `-a` listing; either keep the SVF and convert
with `-a` for the check, inspect the XSVF with a parser, or treat a
`DATAOVERFLOW` result as the check." If the gate really matters, recommend the
SVF path when the source is available.

### 6. [severity: major] Pull-up statement contradicts the JTAG edge-cases page

**Location:** `wiring.md:28-30`:
> Entry does not enable the on-board pull-ups; binary entry switches them off.
> If a custom host turned them on inside `BBIO1` with a pin write, they stay on
> until the next board reset.

`protocol/jtag/edge-cases.md:15`:
> | Entering binary from a mode | the header pins other than AUX1, VREG and the pull-ups reset | ...

**Problem:** one page says binary entry switches the pull-ups off; the other
says the pull-ups are among the things that are **not** reset. A reader cannot
know whether a pull-up left on at the terminal survives into `0x18`, which is
exactly the kind of thing that makes a loaded bus look like a bad target.

**Fix:** verify against the firmware and state one behaviour in both pages. If
binary entry clears them, correct the JTAG row; if not, change the XSVF line
and tell the reader to reset before `0x18`. The "no pull-up needed" instruction
is fine either way; the state transition is the missing fact.

### 7. [severity: major] No "get the tools" line; `scripts/...` is not anchored

**Location:** `README.md:54-56`:
> Hardware: a Bus Pirate v4, a USB cable, the target's JTAG header and a common
> ground, plus a Windows or Linux host.  The firmware player is not built into
> the v3 image.

`players.md:36`:
> | Folder | `scripts/BP4_XSVFplayer_Win/` | `scripts/BPXSVFPlayer/` |

**Problem:** a fresh reader with only the docs learns the executable names and
folder names, but not that they live in the firmware repository under
`scripts/`, or that the repository does not install them. The first task
("where do I start?") stops at "do I have the exe?"

**Fix:** add a short "Get the tools" block to the README: "In the firmware
repository: `scripts/BP4_XSVFplayer_Win/` and `scripts/BPXSVFPlayer/`; the
repository does not install them — run from the folder or build with the
Makefile/project." A link to the repository (not the dev docs) is enough.

### 8. [severity: minor] `-r N` says XC9500/XL/XV only, but other recipes use `-r 0`

**Location:** `converter.md:52`:
> | `-r N` | maximum retry count, N times (XC9500/XL/XV only) |

`converter.md:73-75` and `:79`:
> | CoolRunner/II | `-r 0 -extensions -xwait` |
> | XC18V00/XCF00S | `-r 0` |
> | XCF00P | `-r 0 -extensions` |
> | System ACE MPM/SC | `-r 0 -xwait` |

**Problem:** a reader following CoolRunner/II or XC18V00 sees an option the
table says is not for that family. Is the recipe wrong, or is the table
describing only the retry feature? The docs do not say.

**Fix:** explain that the non-XC9500 recipes pass `-r 0` to disable retries,
and that the parenthetical means retry counts are meaningful only for
XC9500/XL/XV; or drop the parenthetical.

### 9. [severity: minor] `Sending N Bytes (offset)` is not an offset

**Location:** `players.md:166`:
> | `Sending N Bytes (offset)...` | one chunk; the offset is hex |

`players.md:235-237`:
> The chunk offsets are hex, so `1000` is 4096 bytes and the final `D029` is
> 53289, the whole file.  The elided lines repeat `Sending 4096 Bytes (…)...ok`
> up to offset `D000`.

**Problem:** 53289 is `0xD029`; the final 41-byte chunk starts at `0xD000`, not
`0xD029`. The number in parentheses is the running total after the chunk, not
the chunk's starting offset. The docs' own "up to offset `D000`" then "final
`D029`" cannot both be offsets.

**Fix:** call it "the running total after the chunk, hex: the last full chunk
totals `D000`; the final 41 bytes total `D029`." Update the output-table row.

### 10. [severity: minor] Host summary says it stops only when the file is used up

**Location:** `players.md:24`:
> 7. Stops when the file is used up and prints the reply it read.

`workflow.md:143-148` shows the opposite:
> (the player aborts at the first mismatch; the count stops there)
> End of operation reply: XSVF_ERROR_TDOMISMATCH

**Problem:** the host stops on any result byte; an error ends the run before
the file is used up. The summary makes the failure transcript look impossible.

**Fix:** "Stops when the player returns a result byte (an error can end the run
early) or the file is used up; prints the result."

### 11. [severity: minor] `entry ok` table says "prints no `ok`" but the transcript has `ok`

**Location:** `players.md:41`:
> | Entry `ok` text | prints `ok` after each entry step | prints no `ok` |

`players.md:260-261`:
> Waiting for first data request...ok
> Sending 4096 Bytes (1000)...ok

**Problem:** the row reads absolute; a reader comparing the cross-platform
transcript sees `ok` and thinks the table is wrong. The intended scope is only
the two entry lines.

**Fix:** "prints no `ok` after the two entry lines; the run's `ok` suffixes
still appear."

### 12. [severity: minor] Chain-scan example is not a valid IDCODE; `FC` conflates two cases

**Location:** `players.md:138`:
> That is one device: count `04`, then the four IDCODE bytes `30 06 09 00`.

and `players.md:146`:
> | count `FC` (`4 × 63`), garbage IDCODEs | TDO stuck low, or a chain over 63 devices (clamped) |

also `workflow.md:117`/`:122-123` and `limits-and-errors.md:92`.

**Problem (a):** least-significant-byte-first `30 06 09 00` is `0x00090630`;
bit 0 is 0, but an IEEE 1149.1 IDCODE has bit 0 = 1. A JTAG-aware reader sees
an invalid IDCODE labelled as one. **Problem (b):** a real chain over 63
devices reports the first 63 **valid** IDCODEs, not "garbage IDCODEs"; only TDO
stuck low produces garbage. The table conflates a wiring failure with a clamped
real chain.

**Fix:** use a valid example (for instance `93 40 60 59` for `0x59604093`) or
label the bytes "illustrative DR bytes, not a real IDCODE." Split `FC` into
"`FC` + garbage → TDO stuck low" and "`FC` + plausible first 63 → chain > 63,
truncated."

### 13. [severity: minor] Presentation: ragged wraps, notes inside captures, Linux prompt

**Locations and quotes:**

- `players.md:44` (82 columns): "`ok` words and a stricter `-f` rule; the cross-platform build reports a meaningful"
- `players.md:187` (92 columns): "the board stays in the firmware player loop with the regulator (VREG) on: **press reset** to"
- `wiring.md:5` (81 columns): "JTAG paths is [protocol/jtag/wiring.md](../../protocol/jtag/wiring.md); this page"
- `players.md:227`: " (chunk lines elided)" inside the transcript.
- `workflow.md:145`: " (the player aborts at the first mismatch; the count stops there)" inside the failure transcript.
- `players.md:245`: "> BPXSVFplayer -p /dev/ttyACM0 -s 115200 -f xc95test.xsvf -r" (a Linux command shown with a Windows `>` prompt; `workflow.md` uses `$`).
- `players.md:53`: the converter cell is 139 characters wide and packs five paths plus a brace expansion into one cell.

**Problem:** the three prose lines wrap visibly short on the next line; the
parentheticals look like tool output, especially the full sentence in the
failure transcript; the Linux `>` is inconsistent and copyable; the
"What ships" converter row is hard to scan.

**Fix:** rewrap the three lines; move the notes outside the code fences or mark
them as comments; use `$` for the Linux command; split the converter row into
Windows and Unix rows (or a two-line list).

### 14. [severity: minor] Host banner reuses the firmware mode's name

**Location:** `README.md:11-16`:
> The firmware sub-mode is the **firmware player** (`0x18`, answers `XSV1`);
> the JTAG set calls the same thing the **XSVF player**.  The PC programs are
> the **host players**...

`players.md:35`:
> | Banner | `BusPirate XSVF Player V.02` | `BusPirate XSVF Player V.01` |

**Problem:** the PC program's banner is also "XSVF Player"; a reader who sees
`BusPirate XSVF Player V.02` may think it is the firmware mode. The naming
paragraph does not mention the banner string, so the explicit mapping stops one
step short.

**Fix:** add: "The host programs' banner also reads `BusPirate XSVF Player`;
that is the PC tool. The firmware mode only answers `XSV1`." Use "host player"
for the PC program consistently.

### 15. [severity: minor] Windows `Error opening file` contrast is misleading

**Location:** `limits-and-errors.md:123`:
> | `Error opening file` | fix the `-f` path; the cross-platform build exits here |

**Problem:** the cross-platform build exits cleanly; the Windows build prints
the message and then continues into `fseek` on the null handle, so it can
terminate abruptly instead of giving a clean error. The table's "the
cross-platform build exits here" implies Windows handles it.

**Fix:** "The cross-platform build exits; the Windows build may terminate after
the message — check the `-f` path." Or fix the tool to return before the file
operations.

### 16. [severity: minor] Failure-mode fixes lack the terminal steps; the VREG check is unreachable inside the player

**Location:** `workflow.md:191-193`:
> ... check the wired rail, the ground, and the regulator (the terminal power
> check reports `VREG too low, is there a short?`).

`wiring.md:77-78`:
> | AUX (SRST) left low by a `BBIO1` pin write | target held in system reset |
> drive it high before `0x18`, or reset the board |
> | AUX1 (TRST, v4) left low by a terminal `A1` write | test logic in reset;
> all-zero reads | release it at the terminal before `0x18`, or reset |

**Problem:** a user seeing `TDOMISMATCH` is inside the player; the terminal
power check requires a reset (and `W` requires a mode). The SRST/TRST fixes say
"drive it high" but not `c`/`k` + `A`; `A1` is a pin/selection token, not a
terminal command. A user can only take the "or reset" branch, and the docs do
not say that reset clears both.

**Fix:** add the exact recovery: "Press reset; enter a mode; `W`; if it prints
`VREG too low, is there a short?`, check the rail." For TRST: "press reset
(clears both), or at the terminal select `A1` with `k`, then `A`." For AUX:
`c` then `A`.

### 17. [severity: minor] README does not redirect v3 owners

**Location:** `README.md:80`:
> - **v4 only.**  On v3 the `0x18` byte is answered with `0x00`.

`README.md:99`:
> | [../openocd/README.md](../openocd/README.md) | the host-driven JTAG alternative for larger devices |

**Problem:** a v3 owner gets "no" but not "use OpenOCD's `svf` through `0x06`";
that alternative is only in `limits-and-errors.md` and the protocol set. The
related-doc label says "larger devices," not "v3."

**Fix:** add to Availability or Limits: "v3: no XSVF player; use the OpenOCD
path (`0x06`) with OpenOCD's `svf` command; see `../openocd/`." Change the
related-doc label to "the v3/larger-device alternative."

## What to keep

- The naming paragraph in the README is the single most useful thing: it
  separates the **firmware player** (`0x18`, `XSV1`) from the **host players**
  and the **host tools**. Once read, the rest of the set is unambiguous.
- The result-byte table with byte, printed name, and meaning, plus the `0x07`
  (`XSVF_ERROR_LAST`) and `0xFF` (`XSVF_READY_FOR_DATA`) notes. That is exactly
  the table a first run needs.
- The `XCOMMENT` trap is stated plainly, with the only exit ("reset the
  board") and the cause (`-xprintsrc` comments). I found it in one hop from the
  README.
- "The host player finishing is not the firmware player finishing" is a
  genuinely useful warning; it explains a half-configured target that otherwise
  looks like a tool bug.
- The wiring/levels page: five wires, no pull-up, one rail only, 3.3 V
  push-pull only, 5 V-threshold caveat, VREG fault note. I would wire from it.
- The limits table (400 bits / 4096 bytes / 5 s / 63 devices / result 0x01,
  0x04, 0x05, 0x06) is compact and scannable; the device-class fit table makes
  the "this is for XC9500-class parts" point without burying it.
- The platform comparison table and "What ships" are honest about the two
  builds, the extra `ok` words, the `-f` difference, and the stale batch file.
- The converter's "Naming" section (501/502/`svf2xsvf`) and the `-a` shift
  check are useful; the family recipes are copy-paste ready.
- The README is a real map: one click to workflow, player, host tools, wiring,
  limits; all links and anchors I followed resolve.

## Structural opinion

The overall shape works for this function: map → workflow → firmware side →
host side → converter → wiring → limits. It matches the order I actually read
in, and the README does not try to be the manual.

Concrete changes:

1. **Move all four worked sessions into `workflow.md`** (or a `sessions.md`
   linked from both). Today `workflow.md` says it owns "worked sessions," but
   the two full run transcripts are in `players.md`. One canonical page would
   remove the duplication and make the end-to-end promise true; `players.md`
   can keep the output-line table and platform deltas.
2. **Add a "Get the tools" block to the README** (repository `scripts/` paths,
   "not installed by the repo") and a one-line v3 → OpenOCD `svf` redirect.
   These are the two front-door gaps.
3. **Add a "human exit" table**: "press reset (always) | `0x0F` from a raw
   host (only at the command loop)" with either a pySerial one-liner or an
   explicit "script only" label. This would resolve the `0x0F`/`W`/reset
   confusion in one place.
4. **Treat the chain scan as a trap, not just a diagnostic.** Put the
   false-success warning next to the `-x` option and next to the worked
   command, and either fix the host to size the read or show `-x` as a
   separate invocation.
5. **Consider splitting `limits-and-errors.md`** into "firmware limits/result
   actions" and "host messages/recovery," or at least put "Result actions"
   before the long host-message table and add a "read this table first" line.
   The page is 168 lines and currently mixes limits, result actions, host
   messages, and v3.
6. Keep `wiring.md` as a short repeat of `protocol/jtag/wiring.md`; the
   cross-link is right. Keep `player.md` as the tool-side view of the firmware;
   the overlap with `protocol/jtag/xsvf.md` is justified by audience. The
   structural problem is only the split sessions and the missing front-door
   facts, not the page count.

## Unknowns

- I could not test the 400-bit, 4096-byte or 5 s stall limits on hardware; the
  docs say they are unverified, and I only confirmed the code path.
- I could not verify that `04 30 06 09 00` is a real capture. It does not decode
  as a valid IDCODE (bit 0 = 0), so either the example is illustrative or the
  label is wrong.
- I could not reproduce the short-scan false success on a board; the host code
  shows the unread-byte path, but USB CDC read sizes may make it rare.
- I could not determine whether binary entry clears the on-board pull-ups on
  every v4 build; the XSVF wiring page and the JTAG edge-cases page disagree.
- I could not test the Windows `Error opening file` path; the source suggests a
  null-handle dereference after the message.
- I could not test whether the cross-platform source builds on macOS; the docs
  say Windows or Linux, and "cross-platform" is the tool's name.
- I could not test a 5 V target's input threshold; that is target-specific.
- I could not test the converter's family recipes end to end (no SVF/toolchain),
  though the recipes match the bundled ReadMe.
