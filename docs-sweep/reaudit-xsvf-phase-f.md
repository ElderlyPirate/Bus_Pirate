# XSVF — Phase F re-audit

**Scope:** the 9 findings in `feedback-xsvf-phase-f.md`, re-read against the
current `docs/tools/xsvf/` pages and checked against #227
`C:\Users\jschl\dev\bpv4-oocd` @ `992454d` (firmware `96ccc14`).  The local
`Firmware/` was not used.  No fresh full review.

**Status key:** *Filled in full* = the documentation issue is closed and the
new text reads naturally.  *Partial* = part of the finding remains.
*Not addressed* = unchanged.  *Regressed* = the fix introduced a new defect.

## Per-item results

| # | Status | Current evidence (quote) | Remainder |
|---|---|---|---|
| 1 | **Filled in full** | `players.md:232`: "both source builds | `0` success; `1` player-result error; `2` host/transport/setup failure"; `players.md:234-239`: "`0` is an `XSVF_ERROR_NONE` result, or a stand-alone `-x`/`-r` operation ...; `2` ... no reply, incomplete chain scan, bad or empty file, `End of file reached without a completion result.`, or a serial or player-start failure."  `limits-and-errors.md:139` gives the new EOF message and "run returns `2`".  Every `-1`/old-EOF claim is gone. | The mapping is explicitly for source builds; the legacy note points to the prebuilt box. |
| 2 | **Filled in full** | `players.md:24-27`: "A result stops either source build; a player error returns `1`, and no further chunk is sent.  `No reply.... Quitting.` occurs only when the player gives no result."  `workflow.md:171-173`: "Both source builds stop at the first mismatch, send no further chunk, and return `1`."  `limits-and-errors.md:140` says the no-reply row means the player gave no result. | The old Windows extra-chunk/`No reply`/`0` text is gone. |
| 3 | **Filled in full** | `README.md:100-103`: "A stall inside an `XCOMMENT` string unwinds.  The comment loop exits on the stalled flag; the run returns `0x01` and the player loop stays reachable, so `0x0F` leaves."  `limits-and-errors.md:68` adds the XCOMMENT row "`0x01` at the next command boundary"; `:76-81` says "a board reset is not required" and "after every abort above".  `player.md:148/156-159` matches. | Reset-only wording is gone from all three pages. |
| 4 | **Filled in full** | `players.md:29-32`: "The scan is framed and drained before `0x03`, so `-x` with `-f` is safe in a source build ... The tracked legacy prebuilt EXEs still use the old unsized scan; with those, run `-x` alone, then reset."  `players.md:180-186` repeats the framed read and the legacy exception; `workflow.md:78-82` says the source builds frame/drain and `-x` with `-f` is safe.  The converter/400-bit text is unchanged. | The universal "do not combine" workaround is gone; only the legacy note remains. |
| 5 | **Filled in full** | `players.md:47`: "`-f` with `-x`/`-r` ... not required; `-x`/`-r` run alone and return `0`" (both columns).  `players.md:126-131`: "Both source builds run `-x`/`-r` without `-f` and return `0`; ... `No file specified...` and exit `2` occur only when none of `-f`, `-x` or `-r` is given"; the cross build prints `Thank you`, Windows stops.  `workflow.md:85-89` and `limits-and-errors.md:130` agree.  The Windows-stricter/cross-only probe wording is gone; `workflow.md:120-123` says both source builds support probe-only. | The table's behavior rows are source-build behavior; the separate legacy box makes that clear. |
| 6 | **Filled in full** | `players.md:45`: "Ready binary | legacy prebuilt; build source for current behavior"; `:56-67`: the tracked executables are the legacy builds and lack the framed scan, early-result stop, 0/1/2 mapping and clean bad-file exit; build the source.  `players.md:73`, `:270`, `:309`, `README.md:53-56` and `workflow.md:60-61` repeat the source/legacy distinction. | The old "package stops on an early error" claim is gone. |
| 7 | **Filled in full** | `players.md:193-198`: the table lists `Chain scan: incomplete reply (bounded timeout)` and `Chain scan: failed to send the command`; "Both current source builds give up after five attempts.  The tracked legacy prebuilt prints ` Got no reply for a Chain scan` instead."  `limits-and-errors.md:136-137` and `:151-152` match; the old `res`/`timeout_counter` paragraph is gone. | Legacy message is correctly scoped. |
| 8 | **Filled in full** | `limits-and-errors.md:131-132`: "`Error opening file`: fix the `-f` path; both source builds return `2` before the player start; reset or raw-serial `0x0F` twice" and the new `Error sizing nonempty input file` row with the same status.  `players.md:237-239` lists bad/empty file under `2`.  The "may terminate" wording is gone. | No null-handle/termination claim remains. |
| 9 | **Filled in full** | `players.md:294`: "End of operation reply: XSVF_ERROR_NONE (0x00)"; `players.md:218-219`: "Windows appends `(0xNN)` ... cross-platform ... name only"; `workflow.md:100` has `(0x00)` in the Windows tail and `:106` states the per-build suffix.  The cross-platform transcript remains without hex. | — |

**9 of 9 filled in full.**

## Cross-set

The recorded jtag navigation residue remains outside this set:
`docs/protocol/jtag/xsvf.md:159` points at the tools README instead of
`players.md#exit-status-and-the-next-run`.  It is not counted as an XSVF
finding here, per the task.

No behavioral jtag/openocd/binary edit is required for these 9 items; the
jtag `XCOMMENT` text and edge-case row already match the repaired firmware.

## Presentation

Touched XSVF files pass:

- **TABs:** 0 tab-containing lines across all seven files.
- **Prose width:** 0 non-table, non-code prose lines over 80 columns.
- **Tables/captures:** aligned; the Windows transcript, cross transcript and
  pySerial block are readable, and the new legacy/source box is a short
  section rather than a dense bullet chain.
- **Links/anchors:** all relative links resolve; the new
  `#sources-vs-prebuilt-binaries` anchors resolve.  The pre-existing
  `worked-command--windows` link follows the em-dash heading convention and is
  unchanged; the only known cross-set pointer is the separately recorded jtag
  link.
- **Provenance:** no dates, bench/rig prose, pass counts, "tested/captured on"
  claims or internal log names.  The only verification language is the
  allowed "code-derived and **unverified** on hardware" caveat.

**Set status: clean for the 9 Phase F findings.**
