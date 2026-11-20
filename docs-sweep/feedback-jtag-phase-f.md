# JTAG Phase F delta audit

Verified against #227 `C:\Users\jschl\dev\bpv4-oocd` at `992454d` (firmware byte-identical to `96ccc14`). Read-only; no hardware run. Scope is the Phase F delta only: XCOMMENT stall, v3 `0x05` zero count, and the host-tool exit deferral in `protocol/jtag/xsvf.md`.

**Verdict: No — the two #227 repairs are still absent from the JTAG set.** `xsvf.md` and `edge-cases.md` still describe an XCOMMENT stall as reset-only, and `openocd.md` / `edge-cases.md` still describe the v3 zero-count shift as unsupported or unverified. The XSVF host-exit paragraph also overstates the tool behavior and does not defer cleanly to `tools/xsvf`.

## Findings

### 1. [major] The XCOMMENT stall is still documented as reset-only

**Where:**
- `docs/protocol/jtag/xsvf.md:100-102` — "The player loop is reachable after these aborts, except when the stall falls inside an `XCOMMENT` string: that loop keeps reading `0xFF` and does not end until the board is reset."
- `docs/protocol/jtag/xsvf.md:180-181` — "The player loop remains reachable after these aborts, except for a stall inside an `XCOMMENT` string; reset the board then."
- `docs/protocol/jtag/edge-cases.md:74` — "| Host stops inside an `XCOMMENT` string | player does not return; `0x0F` is ignored | reset the board |"

**Why it is wrong:** In #227, `xsvfDoXCOMMENT()` now loops on `while (ucText && !xsvf_stream_stalled)`. A stalled chunk read exits the comment loop; the next `xsvfRun()` sees `xsvf_stream_stalled` and returns `XSVF_ERROR_UNKNOWN` (`0x01`). The player loop is reachable again and `0x0F` leaves. The current text also contradicts its own `0x01` result row (`xsvf.md:115`) and the already-repaired binary C2 section (`docs/protocol/binary/edge-cases.md:274-286`), which says the stall aborts with `0x01` and `0x0F` is the recovery.

**Exact fix:**
- Replace `xsvf.md:100-102` with: "A stall inside an `XCOMMENT` string exits the comment loop on the stalled flag. The run returns `0x01` at the next command boundary, the player loop is reachable, and `0x0F` leaves the player."
- Replace `xsvf.md:180-181` with: "The player loop remains reachable after these aborts, including a stall inside an `XCOMMENT` string; the run returns `0x01` at the next command boundary, and `0x0F` leaves."
- Replace the `edge-cases.md:74` row with: "| Host stops inside an `XCOMMENT` string | comment loop ends on the stall; run returns `0x01` at the next command boundary | `0x0F` leaves the player; reset is not required |"

### 2. [major] The v3 zero-count TAP shift is still called unsupported/unverified

**Where:**
- `docs/protocol/jtag/openocd.md:120-121` — "On v4 a **zero count** consumes no payload and returns no TDO byte. On v3 the zero-count path is unverified."
- `docs/protocol/jtag/edge-cases.md:42` — "| Zero-bit shift on v4 | no payload, no TDO byte | do not wait for a reply; v3 is unverified |"

**Why it is wrong:** In #227, the v3 `CMD_TAP_SHIFT` sends the 3-byte echo first, then calls `binOpenOCDTapShiftFast()` only under `if (j)`. A zero count is a guarded no-op on v3 exactly as on v4: no payload is read and no TDO byte is returned. The v3 path does have a zero-count guard now; the "unsupported/unverified" wording describes the pre-fix build. The general v3 caveat at `openocd.md:183-186` and `edge-cases.md:94-99` should not re-caveat this specific behavior.

**Exact fix:**
- Replace `openocd.md:120-121` with: "A **zero count** consumes no payload and returns no TDO byte on either build; the 3-byte echo is still sent."
- Replace the `edge-cases.md:42` row with: "| Zero-bit shift (v3 and v4) | 3-byte echo only; no payload read, no TDO byte | do not send payload or wait for a TDO byte |"
- Scope the blanket caveats: `openocd.md:185-186` "The v3 behaviours are not verified" becomes "Other v3 behaviour is build-only and not verified"; `edge-cases.md:96` "All v3 behaviour in this set is build-only — not verified" becomes "Other v3 behaviour in this set is build-only — not verified." The zero-count paragraph itself should carry no "unverified" qualifier.

### 3. [minor] The host exit paragraph says every tool closes after a result and offers `0x0F` too broadly

**Where:** `docs/protocol/jtag/xsvf.md:155-158` — "The stock host tools close the port after the result byte and do not leave the XSVF player, so the board stays in the player loop with the regulator on until it is reset, or a raw-serial host sends `0x0F` twice."

**Why it is wrong:** The #227 host tools do not always produce a result byte. `docs/tools/xsvf/players.md:250-255` says the players close the port without sending `0x0F`; the board is in the player loop after a normal run, but a raw-serial `0x0F` pair works only once the player is back at its command loop (a result was printed, or the ~5 s stall expired). `docs/tools/xsvf/players.md:225-227` and `limits-and-errors.md:114-115` also cover the no-result `End of file reached.` case, and `workflow.md:207-210` says a `No reply.... Quitting.` stop may mean the player is still running, so reset first. The jtag page turns those cases into "after the result byte" and an unconditional `0x0F` alternative.

**Exact fix:** Replace `xsvf.md:155-158` with: "The stock host tools close the port without sending `0x0F`; they can stop on a result or on an early error/EOF without one. After a normal run the board is in the player loop; a raw-serial host can send `0x0F` twice only once that loop is back. If the tool stopped mid-run, reset first. The exit mapping is in [tools/xsvf](../../tools/xsvf/README.md)." This keeps the host-workflow detail in its owning set instead of duplicating it in the protocol page.

## Cross-set list

These pages still describe the pre-#227 behavior and need their owning set's audit; they are not edits for the JTAG set.

| Source set | Page | Stale claim |
|---|---|---|
| xsvf | `docs/tools/xsvf/README.md:98-99` | "A stall inside an `XCOMMENT` string never returns. Only a board reset exits it." |
| xsvf | `docs/tools/xsvf/limits-and-errors.md:74-76` | "`0x0F` is ignored. **Reset the board** — that is the only exit." |
| xsvf | `docs/tools/xsvf/player.md:155-159` | "the loop never ends and no result is sent ... reset the board — that is the only exit." |
| openocd | `docs/tools/openocd/commands.md:194-198` | "**v3:** a zero count is not supported; the v3 path has no zero-count guard. ... the v3 outcome is unverified." |
| openocd | `docs/tools/openocd/limits-and-errors.md:25` | "zero count \| v4 no-op; v3 unsupported \| do not send one" |
| openocd | `docs/tools/openocd/limits-and-errors.md:68-73` | "**v3:** a zero count is not supported. The v3 path has no zero-count guard ... unverified." |
| openocd | `docs/tools/openocd/limits-and-errors.md:139` | "a zero count on v3 \| no defined reply; the shift does not complete normally \| the v3 path has no zero-count guard" |

`docs/protocol/binary/edge-cases.md:274-286` already matches #227 (`0x01` at the next boundary, `0x0F` recovery); no binary edit is needed. `docs/tools/xsvf/players.md` and `workflow.md` already carry the correct host exit mapping; the JTAG page is the stale side of that cross-link.

## Presentation

Checked only the passages above. No TAB characters; prose lines are within 80 columns; the `edge-cases.md` rows are consistent three-column tables; no dates, bench narratives, pass counts, or provenance in these passages. The replacements above should keep the same table shape and line budget.
