# XSVF — Phase F delta audit

**Verdict: no.**  The XSVF tool pages still describe the pre-#227 host and the
old `XCOMMENT` trap.  A reader who builds the current source gets the wrong
exit codes, the wrong Windows early-error behaviour, and an unnecessary
reset-only recovery; the pages also imply the unchanged prebuilt EXEs have the
host fixes.

**Sources verified:** #227 `C:\Users\jschl\dev\bpv4-oocd` @ `992454d`
(firmware `96ccc14`); the local `Firmware/` was **not** used.  The host commits
`f7267f12`, `ed1abc32`, `b912fc67`, `992454d5` touch only `main.c` and the
test; the tracked EXEs (`BP4_Full_bitbang_XSVFplayer_Win.exe`,
`package/BPXSVFplayer[.exe]`) are unchanged legacy builds.  `package/out.log`
still shows the legacy `End of file reached.` output.

## Findings

### 1. [severity: blocker] Exit status and premature EOF are still the legacy mapping

**Location:** `players.md:46`:
> | Exit status | `0` after any run; `-1` for setup failure | `0`, `1`, `2` or `-1`; see below |

`players.md:222`:
> | Windows player | `0` after any run, whatever the result; `-1` for a setup failure |

`players.md:225-227`:
> - `0` is a clean result, **or** `End of file reached.` on the cross-platform
>   build.  The latter produced no result byte; treat it as a failure even
>   though the status is `0`.

also `players.md:231-232`, `players.md:206`, `limits-and-errors.md:114`,
`:130`, `workflow.md:201`, `:209`.

**Why:** the #227 sources in both `scripts/BP4_XSVFplayer_Win/main.c` and
`scripts/BPXSVFPlayer/main.c` return **0 = success, 1 = player result error,
2 = host/transport/setup failure**.  Setup failures return `2`, not `-1`.
A premature EOF prints `End of file reached without a completion result.` and
sets `status=2`; it is not status 0.  The current pages describe the legacy
prebuilt binaries, not the repaired players.

**Exact fix:** replace the exit table and bullets with: "Both current source
builds return `0` success; `1` player-result error; `2` host/transport/setup
failure (including no reply, incomplete chain scan, bad or empty file, and
`End of file reached without a completion result.`)."  Remove every `-1` exit
claim, update the EOF message and status everywhere it occurs, and add the
prebuilt-legacy note from finding 6.

### 2. [severity: major] Windows early-error path still says "one more chunk / No reply / 0"

**Location:** `players.md:24-26`:
> 7. Stops when the file is used up, or at a result on the cross-platform
>    build; the Windows build sends one more chunk after an early error and then
>    ends with `No reply.... Quitting.`

`players.md:204`:
> ... an early error stops the cross-platform build, but the Windows build sends one more chunk

`players.md:243-248`:
> The Windows build does not stop on an early error result: it prints the
> result, sends one more chunk, then ends with `No reply.... Quitting.` and
> still returns `0`.

also `players.md:211-212`, `workflow.md:164-169`,
`limits-and-errors.md:131`.

**Why:** in #227 the Windows host sets `timer_out=-1` on any non-`0xFF` result
and stops; it does **not** send another chunk.  It also sets `status=1` for the
player error, so it returns `1`, not `0`.  `No reply.... Quitting.` is no
longer the tail of an early-result run on either build.

**Exact fix:** remove the Windows-specific extra-chunk/`No reply`/`0` text.
State: "Both builds stop at an early result; a player error returns `1` and
sends no further chunk.  `No reply.... Quitting.` only occurs when the player
gives no result."  The failure transcript may use either build; delete the
sentence that says it uses the cross-platform build "for that reason."

### 3. [severity: major] `XCOMMENT` still described as a reset-only trap

**Location:** `README.md:98-99`:
> - **A stall inside an `XCOMMENT` string never returns.**  Only a board reset
>   exits it; see [limits-and-errors.md](limits-and-errors.md).

`limits-and-errors.md:74-77`:
> **The `XCOMMENT` exception.**  If the stall falls inside an `XCOMMENT`
> string, the player reads `0xFF` forever, never returns, and never sends a
> result.  `0x0F` is ignored.  **Reset the board** — that is the only exit.

`player.md:155-159`:
> **One exception: a stall inside an `XCOMMENT` string.**  The player reads the
> comment until a zero byte, and a stalled read returns `0xFF` forever, so the
> loop never ends and no result is sent. ... **reset the board** — that is the
> only exit.

also `limits-and-errors.md:79` ("after every other abort above").

**Why:** #227 `Firmware/tool_xsvf/micro.c` `xsvfDoXCOMMENT()` now exits on
`xsvf_stream_stalled` (`while (ucText && !xsvf_stream_stalled)`); the next
`xsvfRun()` returns `XSVF_ERROR_UNKNOWN` (`0x01`) at the command boundary, and
`0x0F` leaves the player.  The jtag protocol page already says this; the tool
pages do not.  Reset is no longer required.

**Exact fix:** replace the "exception" wording in all three pages with: "A
stall inside `XCOMMENT` exits the comment loop on the stalled flag.  The run
returns `0x01` at the next command boundary; the player loop is reachable and
`0x0F` leaves.  A board reset is not required."  Add an `XCOMMENT` row to the
stall table and change "every other abort above" to "every abort above."

### 4. [severity: major] Chain scan is still described as unsized; the separate `-x` workaround is obsolete for the rebuilt players

**Location:** `players.md:28-30`:
> Points 5 and 6 share one port.  A host that sends both must read the scan
> reply in full before `0x03`; the shipped players do not, so run `-x` alone.

`players.md:159-171`:
> **Do not combine `-x` with `-f` in one invocation.**  Neither build uses the
> count byte to size the read, and neither drains the port before sending
> `0x03`. ... Run `-x` in its own invocation, press reset ... then run the file
> with `-r -f`.

also `players.md:114-119`, `:166-167` ("Other leftovers ... the Windows
build can also terminate" — legacy-only once the scan is drained), `:185`,
`workflow.md:75-79`, `limits-and-errors.md:135`, and the batch warning at
`players.md:69-76`.

**Why:** #227 adds `read_serial_exact()` and `do_chain_scan()` reads one count
byte then exactly `count` payload bytes in both hosts; the scan is framed and
drained before `0x03`.  `-x` with `-f` is now safe in the rebuilt source
players.  The old prebuilt EXEs still have the unsized scan, but the pages
currently state the old behavior for "both builds."

**Exact fix:** for the rebuilt source players, remove the universal "do not
combine / neither drains / run alone" warning and say "the scan is framed and
drained; `-x` with `-f` is safe."  Keep a short legacy note: "The tracked
prebuilt EXEs still use the old unsized scan; with those, run `-x` alone or
rebuild from source."  Keep the converter/400-bit rule unchanged.

### 5. [severity: major] `-x`/`-r` alone and the `-f` rule still describe the old Windows build

**Location:** `players.md:45`:
> | `-f` with `-x`/`-r` | required to continue; the tool stops after the reset/scan | optional; the tool performs them alone |

`players.md:105-110`:
> The Windows player runs them and then stops with ` No file specified. Need an
> input xsvf file`; the help text's "`-f` is optional" is true only for the
> cross-platform build.

`players.md:173-174`:
> The Windows build performs the scan and then stops with ` No file specified.
> Need an input xsvf file`; that is expected when `-x` is used alone...

`players.md:233-234`:
> On the Windows build, `-x`/`-r` without `-f` prints ` No file specified. Need
> an input xsvf file` and still exits `0`.

also `players.md:49-51`, `:333-335` (probe-only shown as cross-platform
only), `workflow.md:82-85`, `:114`, `limits-and-errors.md:123`.

**Why:** in #227 both hosts treat `-x` and `-r` as standalone operations.  With
`-x` or `-r` and no `-f`, they close the port and return **0**; the Windows
build does **not** print `No file specified` (the cross-platform build prints
`Thank you for playing!`).  Only a run with none of `-f`/`-x`/`-r` prints
`No file specified...` and returns **2**.  Both help texts say `-f` is
optional for `-x`/`-r`; the old "Windows is stricter" distinction is gone.

**Exact fix:** update the table and text: "Both builds run `-x`/`-r` without
`-f`, returning 0; `-f` is required only to play a file.  `No file
specified...` and exit 2 occur only when none of `-f`/`-x`/`-r` is given."
Remove the cross-platform-only probe wording and the "Windows stricter `-f`"
claim; the probe-only flow is available from both rebuilt players.

### 6. [severity: major] Prebuilt EXEs are presented as current

**Location:** `players.md:43`:
> | Ready binary | yes, in the folder | yes, under `package/` |

`players.md:57`:
> | player | `BP4_Full_bitbang_XSVFplayer_Win.exe` | `package/BPXSVFplayer`, `package/BPXSVFplayer.exe` |

`players.md:80-82`:
> **Windows:** either build runs a clean file.  The Windows player is the
> ready executable ... The cross-platform `package/BPXSVFplayer.exe` stops on
> an early error and returns a status.

`README.md:50-54`:
> ... run the executable from its folder, or build it with the included project
> or `Makefile`.

**Why:** #227 changes only `main.c` and the test.  The tracked EXEs are the
unchanged 2026-10-24 legacy builds; `package/out.log` still shows the old
`End of file reached.` behavior.  The rebuilt fixes (sized scan, early-error
stop, exit 0/1/2, clean bad-file exit) are in the source, not those binaries.
The pages currently tell readers the ready/package binaries have the fixes.

**Exact fix:** add a "Sources vs prebuilt binaries" box: "The tracked EXEs
(`BP4_Full_bitbang_XSVFplayer_Win.exe`, `package/BPXSVFplayer[.exe]`) are the
legacy builds and do not include the host-tail fixes.  Build
`scripts/BPXSVFPlayer/main.c` with the Makefile/Dev-C++/Code::Blocks to get the
behavior on these pages; a source build is the current player."  Change the
"Ready binary" row to "legacy prebuilt; build source for current behavior" and
remove the `package/BPXSVFplayer.exe stops on an early error` claim.  The
workflow commands should say whether they use a source build or the legacy
prebuilt path.

### 7. [severity: minor] Chain-scan messages and retry details are stale

**Location:** `players.md:184`:
> | ` Got no reply for a Chain scan` | the scan did not answer within the wait |

`players.md:187-189`:
> The Windows build appends `res` and `timeout_counter` to the no-reply line;
> the cross-platform build appends `(bounded timeout)`.  Both give up after five
> reads.

also `limits-and-errors.md:128`.

**Why:** the #227 rebuilds both print `Chain scan: incomplete reply (bounded
timeout)` or `Chain scan: failed to send the command`; neither appends
`res`/`timeout_counter`.  Both still bound the wait at five attempts.  The old
`Got no reply...` line is legacy-prebuilt behavior.

**Exact fix:** document the current messages and five-attempt bound; if the
legacy EXE is kept in the docs, label its `Got no reply...` line as legacy.

### 8. [severity: minor] Bad input file still says Windows "may terminate"

**Location:** `limits-and-errors.md:124`:
> | `Error opening file` | fix the `-f` path; the cross-platform build exits, the Windows build can terminate after the message |

`players.md:236-238`:
> On the Windows build, a bad `-f` path is not reported this way: it prints
> `Error opening file` and may terminate.

**Why:** #227 `RA-26` makes both hosts print `Error opening file` (or
`Error sizing nonempty input file`) and return 2 before `0x03`; the old Windows
null-handle dereference is gone.  The board is already in the player loop, but
no run starts.

**Exact fix:** "Both builds report `Error opening file` or `Error sizing
nonempty input file` and return 2 before the player start; they do not
terminate.  Reset or use raw-serial `0x0F` to leave the player."  Remove the
"may terminate" contrast.

### 9. [severity: minor] Windows transcript omits the new `(0xNN)` suffix

**Location:** `players.md:293`:
> End of operation reply: XSVF_ERROR_NONE

and `workflow.md:96`:
> End of operation reply: XSVF_ERROR_NONE

**Why:** the #227 Windows host prints `End of operation reply: XSVF_ERROR_NONE
(0x00)`; the cross-platform host prints without the hex.  The worked Windows
transcript and its tail are stale.

**Exact fix:** add `(0x00)` to the Windows transcript (and the workflow tail
that cites it), and add a row note: "Windows appends `(0xNN)`; the
cross-platform build does not."

## Cross-set list

- **jtag, navigation only:** `docs/protocol/jtag/xsvf.md:159` says "The exit
  mapping is in [tools/xsvf](../../tools/xsvf/README.md)"; the README is a map,
  not the mapping.  Point it at
  `tools/xsvf/players.md#exit-status-and-the-next-run` after the host page is
  updated.
- **No behavioral jtag/openocd/binary edits are required for this delta.**
  `docs/protocol/jtag/xsvf.md:100-102` and
  `docs/protocol/jtag/edge-cases.md:74` already state the new `XCOMMENT`
  unwind; `docs/protocol/binary/edge-cases.md:274-286` and
  `docs/tools/openocd/limits-and-errors.md:150` do not carry the old
  reset-only claim.
- **Tools set:** all findings above are in `docs/tools/xsvf/**` and need the
  implementer, not another set.

## Presentation

No XSVF page has a Phase F edit yet, so there is no changed passage to render.
The current pages still scan clean: 0 TAB-containing lines, 0 non-table prose
lines over 80 columns, aligned tables/captures, and no dates, bench prose,
pass counts or provenance.  The stale text above is content, not formatting;
the fixes should preserve the existing table/short-section style.
