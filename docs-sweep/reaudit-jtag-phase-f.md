# JTAG Phase F re-audit

Rechecked the three in-scope findings from `feedback-jtag-phase-f.md` against the current `docs/protocol/jtag/` working tree and #227 `bpv4-oocd@992454d`. No hardware run. The seven cross-set pages recorded in the fix record remain outside this set.

## Per-item results

| # | Status | Evidence in the current docs | Remainder |
|---:|---|---|---|
| 1 | Filled in full | `xsvf.md:100-102` — "A stall inside an `XCOMMENT` string exits the comment loop on the stalled flag.  The run returns `0x01` at the next command boundary, the player loop is reachable, and `0x0F` leaves the player." `xsvf.md:179-184` — "The player loop remains reachable after these aborts, including a stall inside an `XCOMMENT` string; the run returns `0x01` at the next command boundary, and `0x0F` leaves." `edge-cases.md:74` — "comment loop ends on the stall; run returns `0x01` at the next command boundary \| `0x0F` leaves the player; reset is not required". | None. The reset-only wording is gone from both pages. |
| 2 | Filled in full | `openocd.md:120-121` — "A **zero count** consumes no payload and returns no TDO byte on either build; the 3-byte echo is still sent." `edge-cases.md:42` — "Zero-bit shift (v3 and v4) \| 3-byte echo only; no payload read, no TDO byte \| do not send payload or wait for a TDO byte". The general caveats are scoped at `openocd.md:185-186` ("Other v3 behaviour…") and `edge-cases.md:96` ("Other v3 behaviour…"). | None. The v3 zero-count sentence no longer says unsupported or unverified. |
| 3 | Filled in full | `xsvf.md:155-160` — "The stock host tools close the port without sending `0x0F`; they can stop on a result or on an early error/EOF without one.  After a normal run the board is in the player loop; a raw-serial host can send `0x0F` twice only once that loop is back.  If the tool stopped mid-run, reset first.  The exit mapping is in [tools/xsvf](../../tools/xsvf/README.md)." | None. The page now defers the exit mapping to `tools/xsvf` instead of asserting one universal result path. |

**3 of 3 filled in full.**

## Presentation check

Checked the touched files (`xsvf.md`, `edge-cases.md`, `openocd.md`):

- No TAB characters.
- No non-table prose over 80 columns.
- Table rows have consistent column counts; the edited `edge-cases.md` rows keep the three-column shape.
- The byte-level captures in `xsvf.md` and `openocd.md` remain aligned at column 26.
- All relative links and anchors resolve, including the `tools/xsvf/README.md` link in `xsvf.md`.
- No dates, bench narratives, pass counts, provenance, or internal log names in the changed passages. The only “captured” occurrence is the technical phrase "captured TDO" in the XSVF result table, not provenance.

## Cross-set status

The JTAG set is clean for these three Phase F items. The seven cross-set pages recorded in `fixes-jtag-phase-f.md` — three in `docs/tools/xsvf/` for the XCOMMENT wording and four in `docs/tools/openocd/` for the v3 zero-count wording — still need their owning sets' Phase F repair; no in-set finding depends on editing them from here.
