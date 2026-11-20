# XSVF docs — Phase F implementation record

Worktree: `C:\Users\jschl\dev\bpv4-docssweep`, branch `docs/combined-sweep`,
docs head `062b1e8`.  Verified against #227 `C:\Users\jschl\dev\bpv4-oocd`
@ `992454d` (firmware `96ccc14`); the local `Firmware/` was not used.  No
commit, no push, no firmware or other-worktree edit.  Edits are restricted to
`docs/tools/xsvf/**`.

Decision key: **fixed** = user docs changed; **rejected** = finding wrong or
already covered; **cross-set** = a navigation edit belongs to another set and
is recorded only.

| # | Decision | What changed (file + short quote) | Evidence |
|---|---|---|---|
| 1 | fixed | `players.md` exit table/bullets: "both source builds: `0` success; `1` player-result error; `2` host/transport/setup failure"; "`2` ... no reply, incomplete chain scan, bad or empty file, `End of file reached without a completion result.`".  `limits-and-errors.md` EOF row now uses the new message and "run returns `2`".  `workflow.md` §6 and its stopped-at table use the new message and status.  Every `-1` and "0 after any run" claim is gone. | #227 `scripts/BP4_XSVFplayer_Win/main.c` and `scripts/BPXSVFPlayer/main.c`: setup paths `return 2`; a non-`XSVF_ERROR_NONE` result sets `status=1`; no reply and the EOF branch set `status=2`.  `scripts/tests/test_xsvf_players.py` CASES: success rc0, early-error rc1, early-eof rc2, missing/empty/seek/tell/file-read rc2.  Test run at `992454d`: 58/58 pass. |
| 2 | fixed | `players.md` step 7: "A result stops either build; a player error returns `1`, and no further chunk is sent.  `No reply.... Quitting.` occurs only when the player gives no result."  The output row and exit section say the same.  `workflow.md` failure text: "Both builds stop at the first mismatch, send no further chunk, and return `1`."  `limits-and-errors.md` No-reply row: "the player gave no result". | Both #227 `main()` loops set `if (buffer[0] != XSVF_READY_FOR_DATA) timer_out=-1;` after a result; the fall-through chunk write is unreachable.  Tests: `early-error` rc1, 0 chunks; `error-after-chunk` rc1, 1 chunk; `no-result` rc2, 0 chunks.  The Windows-specific extra-chunk/`0` text is removed. |
| 3 | fixed | `README.md`: "A stall inside an `XCOMMENT` string unwinds ... the run returns `0x01` ... `0x0F` leaves.  A board reset is not required."  `limits-and-errors.md` and `player.md` replace the reset-only exception with the same behaviour and add an `XCOMMENT` row to the stall table; "every other abort" is now "every abort". | `Firmware/tool_xsvf/micro.c` `xsvfDoXCOMMENT()`: `} while ( ucText && !xsvf_stream_stalled );`; the next `xsvfRun()` returns `XSVF_ERROR_UNKNOWN` (`0x01`) at the command boundary.  `0x0F` is read by the player loop after the run. |
| 4 | fixed | `players.md` chain-scan section: "The current source builds read the count byte and exactly `count` payload bytes ... `-x` with `-f` is therefore safe in a source build."  The universal "do not combine / neither drains / run alone" warning is removed; a short "Legacy prebuilt" note says the tracked EXEs still use the old unsized scan, and tells the reader to run `-x` alone, reset, or rebuild.  `workflow.md` §4 now says `-x` with `-f` is safe in a source build.  The batch paragraph marks `-x` with `-f` safe in a source build.  `converter.md`/400-bit rule unchanged. | #227 both `main.c` add `read_serial_exact()`; `do_chain_scan()` reads one count byte then exactly `count` payload bytes and aborts on an incomplete reply before `0x03`.  Tests: `scan-fragmented`, `scan-zero`, `scan-max` rc0 with one chunk and one start; `scan-count-timeout`, `scan-payload-timeout`, `scan-write-fail` rc2 with no start. |
| 5 | fixed | `players.md` table/options: "not required; `-x`/`-r` run alone and return `0`"; "`-f` is required only to play a file.  Both builds run `-x`/`-r` without `-f` and return `0` ... `No file specified...` and exit `2` occur only when none of `-f`, `-x` or `-r` is given."  `workflow.md` §4 repeats the rule and says the probe-only flow is available from both source builds.  `limits-and-errors.md` No-file row: add `-f`, `-x` or `-r`; only none exits `2`.  The Windows-stricter and cross-platform-only probe wording is removed. | Both #227 `main.c` `else` branches close the port and return `status` (0) when `-x` or `-r` was requested; the `No file specified...` + `return 2` path is guarded by `!chainscan && !jtag_reset`.  Tests: `scan-only` rc0, `reset-only` rc0; `missing-args` rc2.  The cross-platform build prints `Thank you for playing!`; the Windows build stops after the scan/reset. |
| 6 | fixed | `players.md` new "Sources vs prebuilt binaries" section: "The tracked executables (`BP4_Full_bitbang_XSVFplayer_Win.exe`, `package/BPXSVFplayer`, `package/BPXSVFplayer.exe`) are the **legacy** builds.  They do not include ... the framed and drained chain scan, the early-result stop, the `0`/`1`/`2` exit mapping, or the clean bad-file exit."  "Ready binary" row changed to "legacy prebuilt; build source for current behavior"; the player row is marked `(legacy)`.  `workflow.md` commands state "The commands assume a source build; the tracked prebuilt executables are legacy."  `README.md` §Get the tools says the tracked player executables are legacy and points at the box.  The "package/BPXSVFplayer.exe stops on an early error" claim is removed. | #227 `f7267f12`, `ed1abc32`, `b912fc67`, `992454d5` touch only `main.c` and `scripts/tests/test_xsvf_players.py`; the tracked EXEs are unchanged.  `scripts/BPXSVFPlayer/package/out.log` still shows the legacy `End of file reached.` behavior.  The source-built test binaries show the current behavior. |
| 7 | fixed | `players.md` chain-scan table: `Chain scan: incomplete reply (bounded timeout)` and `Chain scan: failed to send the command`; "Both current source builds give up after five attempts.  The tracked legacy prebuilt prints ` Got no reply for a Chain scan` instead."  `limits-and-errors.md` uses the two current messages and adds the same legacy note.  The old `res`/`timeout_counter` paragraph is gone. | #227 both `do_chain_scan()` print exactly those two messages; `read_serial_exact()` bounds each read at five attempts (`CHAIN_SCAN_MAX_TRIES 5`).  Tests `scan-count-timeout`, `scan-payload-timeout`, `scan-write-fail` return rc2 and print `Chain scan`.  The legacy `package/out.log` shows the old `Waiting for a chain scan reply` / no-reply behavior. |
| 8 | fixed | `limits-and-errors.md`: "`Error opening file`: fix the `-f` path; both builds return `2` before the player start; reset or raw-serial `0x0F` twice"; new `Error sizing nonempty input file` row with the same status/exit advice.  `players.md` exit mapping lists "bad or empty file"; the "may terminate" Windows contrast is removed. | #227 both `main.c`: `fopen` NULL prints ` Error opening file` and `return 2`; `fseek`/`ftell`/`fseek` failure or size <= 0 prints ` Error sizing nonempty input file` and `return 2`; no `fseek` on a null handle.  Tests: `missing-file`, `empty-file`, `file-seek-fail`, `file-tell-fail`, `file-read-fail` rc2 with 0 starts; `NULL_FILE_SEEK` absent. |
| 9 | fixed | `players.md` Windows transcript: `End of operation reply: XSVF_ERROR_NONE (0x00)`; source-build note above the transcript.  `workflow.md` clean-run tail: same line.  Output table note: "Windows appends `(0xNN)` to the `End of operation reply` line; the cross-platform build prints the name only." | #227 Windows `main.c`: `printf(" End of operation reply: %s (0x%02X) \n", XSVF_ERROR[c], buffer[0]);`; cross-platform prints `"%s"` only.  The cross-platform transcript remains without the hex suffix. |

## Counts

- Findings processed: 9.
- **Fixed**: 9 (1–9).
- **Rejected**: 0.
- **Cross-set navigation item**: 1 (jtag; recorded below, not edited here).
- **Legacy-prebuilt note**: included in finding 6 and repeated below.

## Legacy prebuilt binaries

The tracked executables are **not** current:

- `scripts/BP4_XSVFplayer_Win/BP4_Full_bitbang_XSVFplayer_Win.exe`
- `scripts/BPXSVFPlayer/package/BPXSVFplayer`
- `scripts/BPXSVFPlayer/package/BPXSVFplayer.exe`

The #227 host commits change only `main.c` and
`scripts/tests/test_xsvf_players.py`; the tracked EXEs are unchanged legacy
builds.  `scripts/BPXSVFPlayer/package/out.log` still shows the old
`End of file reached.` output.  The current behavior in this set is the
**source build**:

- Windows: build from `scripts/BP4_XSVFplayer_Win/` (Dev-C++ project or
  Windows makefile).
- Cross-platform: build from `scripts/BPXSVFPlayer/` (`Makefile` or
  Code::Blocks project).

The user pages now say this in `players.md` §Sources vs prebuilt binaries,
the `Ready binary` and player rows, the worked-transcript notes, the
`workflow.md` command note, the `limits-and-errors.md` host-message
introduction, and `README.md` §Get the tools.  A reader with a tracked EXE
is told to rebuild before relying on the `0`/`1`/`2` mapping, the early-error
stop, the framed scan, or the clean bad-file exit.

## Cross-set navigation item

`docs/protocol/jtag/xsvf.md` (already committed at `062b1e8`) says in §Host
workflow:

> "The exit mapping is in
> [tools/xsvf](../../tools/xsvf/README.md)."

The README is a map; the mapping is owned by
`players.md#exit-status-and-the-next-run`.  The jtag page should ultimately
point at:

> `[tools/xsvf/players.md#exit-status-and-the-next-run](../../tools/xsvf/players.md#exit-status-and-the-next-run)`

No jtag edit was made here.  The XSVF README already links to
`players.md`, which owns the mapping.

## Verification

- Source inspection: #227 `scripts/BP4_XSVFplayer_Win/main.c`,
  `scripts/BPXSVFPlayer/main.c`,
  `scripts/tests/test_xsvf_players.py`; firmware
  `Firmware/tool_xsvf/micro.c` `xsvfDoXCOMMENT()`.
- `python scripts/tests/test_xsvf_players.py` at `992454d`: **58/58 pass**.
  The cases cover success (0), player errors (1), early EOF and no-reply
  (2), missing/empty/unreadable file (2), setup/entry failures (2), framed
  and fragmented scans (0), scan-only/reset-only (0), and missing arguments
  (2).
- Docs checks over all seven `docs/tools/xsvf/*.md`: 0 TABs, 0 non-table
  prose lines over 80 columns, aligned tables/fences, and every relative
  link/anchor resolves.
- No hardware was used; this is source/test and documentation verification.

## Remaining questions

- **Refresh the tracked EXEs?**  The owner may choose to replace the legacy
  binaries with source builds; the docs already label them legacy and tell
  the reader to rebuild.  This is a release/artifact decision, not a docs
  change.
- **jtag navigation** — the one cross-set link above remains for the jtag
  owner; the jtag page was already committed.
- **Legacy scan workaround** — the docs keep the short note for readers who
  run a tracked prebuilt: run `-x` alone, reset, then play the file, or
  rebuild.  The current source players do not need it.
