# JTAG docs — Phase F fixes

Scope: `docs/protocol/jtag/**` only.  No firmware, host-tool or WORKLOG
edits.  No commit or push.

Verified against #227 `bpv4-oocd@992454d` (firmware byte-identical to
`96ccc14`), not the local `Firmware/` copy.

**Counts:** 3 fixed in-set; 0 rejected; 7 cross-set pages recorded for their
owning sets.

## Per-finding record

| # | Decision | What changed (file + short quote) | Evidence |
|---:|---|---|---|
| 1 | fixed | `xsvf.md` stall paragraph: "A stall inside an `XCOMMENT` string exits the comment loop on the stalled flag.  The run returns `0x01` at the next command boundary, the player loop is reachable, and `0x0F` leaves the player."  Limits bullet: "The player loop remains reachable after these aborts, including a stall inside an `XCOMMENT` string; the run returns `0x01` at the next command boundary, and `0x0F` leaves."  `edge-cases.md` row: "comment loop ends on the stall; run returns `0x01` at the next command boundary"; recovery "`0x0F` leaves the player; reset is not required". | #227 `Firmware/tool_xsvf/micro.c` `xsvfDoXCOMMENT()` loops `while (ucText && !xsvf_stream_stalled)`; `xsvfRun()` returns `XSVF_ERROR_UNKNOWN` (`0x01`) when `xsvf_stream_stalled` is set; the player's `0x0F` handler remains reachable. |
| 2 | fixed | `openocd.md`: "A **zero count** consumes no payload and returns no TDO byte on either build; the 3-byte echo is still sent."  `edge-cases.md` row: "Zero-bit shift (v3 and v4) \| 3-byte echo only; no payload read, no TDO byte \| do not send payload or wait for a TDO byte".  Caveats scoped: "Other v3 behaviour is build-only and not verified." and "Other v3 behaviour in this set is build-only — **not verified**." | #227 `Firmware/tool_openocd/openocd.c` writes the 3-byte `0x05` echo with `bp_write_buffer(buf, 3)` and then guards the v3 transfer with `if (j) { ... binOpenOCDTapShiftFast(...) }`; the v4 path enters `while (bit_sequences > 0)`.  A zero count skips payload and TDO on both builds. |
| 3 | fixed | `xsvf.md` Host workflow: "The stock host tools close the port without sending `0x0F`; they can stop on a result or on an early error/EOF without one.  After a normal run the board is in the player loop; a raw-serial host can send `0x0F` twice only once that loop is back.  If the tool stopped mid-run, reset first.  The exit mapping is in [tools/xsvf](../../tools/xsvf/README.md)." | #227 `scripts/BPXSVFPlayer/main.c` cleanup closes the serial port without sending `0x0F`; the run loop can stop on a result byte, on `End of file reached without a completion result.`, on a send failure, or on `No reply.... Quitting.`.  The owning tool pages already carry the result/exit mapping. |

## Cross-set changes for the owning sets

Do not edit these from the JTAG set.

| Owning set | Page | Stale claim | Required repair |
|---|---|---|---|
| xsvf | `docs/tools/xsvf/README.md:98-99` | "A stall inside an `XCOMMENT` string never returns.  Only a board reset exits it" | The comment loop exits on the stalled flag; the run returns `0x01` at the next command boundary; `0x0F` leaves; reset is not required. |
| xsvf | `docs/tools/xsvf/limits-and-errors.md:74-76` | "`0x0F` is ignored.  **Reset the board** — that is the only exit." | Same as above; keep the `-xprintsrc` note. |
| xsvf | `docs/tools/xsvf/player.md:155-159` | "the loop never ends and no result is sent ... **reset the board** — that is the only exit." | Same as above. |
| openocd | `docs/tools/openocd/commands.md:194-198` | "**v3:** a zero count is not supported; the v3 path has no zero-count guard. ... the v3 outcome is unverified." | A zero count is a guarded no-op on v3 and v4: 3-byte echo only, no payload, no TDO byte. |
| openocd | `docs/tools/openocd/limits-and-errors.md:25` | "zero count \| v4 no-op; v3 unsupported \| do not send one" | Zero count is a no-op on both builds; the echo is still sent. |
| openocd | `docs/tools/openocd/limits-and-errors.md:68-73` | "**v3:** a zero count is not supported.  The v3 path has no zero-count guard ... unverified." | Same as above; remove the v3 exception. |
| openocd | `docs/tools/openocd/limits-and-errors.md:139` | "a zero count on v3 \| no defined reply; the shift does not complete normally \| the v3 path has no zero-count guard" | Same as above; remove the row or change it to the no-op behaviour. |

`docs/protocol/binary/edge-cases.md:274-286` already matches #227
(`0x01` at the next boundary, `0x0F` recovery); no binary edit is needed.
`docs/tools/xsvf/players.md` and `workflow.md` already carry the correct
host-exit mapping; the JTAG page was the stale side of that cross-link.

## Remaining questions

- The seven cross-set pages still describe the pre-#227 behavior until their
  owning sets run the same Phase F repair.  This set only links to them.
- The v3 zero-count guard is code-verified in #227, not hardware-tested; the
  paragraph deliberately carries no "unverified" qualifier, while the rest
  of the v3 path keeps its build-only caveat.
- The host-exit paragraph now defers to `tools/xsvf`; if that set changes its
  exit mapping again, the JTAG link should follow it.

## Phase F follow-up — openocd cross-set

Scoped the three `README.md` caveats that still implied every v3 behaviour
was unverified.  Verified against the final combined `Firmware/`: the 3-byte
`0x05` echo is written before the build split, the v3 transfer is inside
`if (j)`, and the v4 path uses `while (bit_sequences > 0)`, so a zero count
skips payload and TDO on both builds.

| # | Decision | Change | Evidence |
|---:|---|---|---|
| 1 | fixed | Status line: "OpenOCD path on v3 (build-only; **other** v3 behaviour unverified) and v4; the XSVF player is v4 only.  Other v3 behaviour in this set is unverified." | `Firmware/tool_openocd/openocd.c` echo before the split; `if (j)` on v3, `while (bit_sequences > 0)` on v4. |
| 2 | fixed | Availability row: "present; zero count guarded; other v3 behaviour unverified". | Same guard; zero count sends only the 3-byte echo. |
| 3 | fixed | Closing caveat: "The v3 column is **not verified on v3**, except the guarded zero-count case." | Same guard; the rest of the v3 path remains unverified. |

