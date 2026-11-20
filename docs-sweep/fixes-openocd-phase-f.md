# OpenOCD adapter — Phase F fixes record

**Worktree:** `C:\Users\jschl\dev\bpv4-docssweep`, branch `docs/combined-sweep`
(docs head `c26f9cdc`).  **Firmware checked:** the worktree's final combined
`Firmware/` (`9b179cb5`).  **Set:** `docs/tools/openocd/`.  **Source checked:**
`Firmware/tool_openocd/openocd.c`, `case CMD_TAP_SHIFT`.  No firmware, other
worktree, or WORKLOG file was changed; nothing was committed or pushed.

## Decisions

| # | Decision | File + quote | Evidence |
|---|---|---|---|
| 1 | fixed | `commands.md`: “A zero count is a guarded no-op on both builds: the 3-byte echo is sent; no payload is read and no TDO byte follows.”  The “OpenOCD never asks for zero bits” line is kept. | In the final tree the three echo bytes are written before the build split.  The v3 branch wraps the transfer in `if (j) { … binOpenOCDTapShiftFast(…); }`; the v4 branch uses `while (bit_sequences > 0)`.  A zero count skips both transfers, so only the echo is sent. |
| 2 | fixed | `limits-and-errors.md`: the firmware-limits row now says “no-op on both builds; 3-byte echo only” and “no payload read, no TDO byte; OpenOCD never sends one”. | Same guard in `openocd.c`: echo first, then `if (j)` on v3 and `while (bit_sequences > 0)` on v4. |
| 3 | fixed | `limits-and-errors.md`: “A zero count is a guarded no-op on both builds.  The 3-byte echo is sent; no payload is read and no TDO byte follows.”  The non-caveat host note “No host needs a zero-bit shift” is kept. | Same guard; there is no v3-only failure path. |
| 4 | fixed | `limits-and-errors.md`: the “a zero count on v3 … no defined reply …” edge-case row was deleted. | The defined reply is the 3-byte echo, and the shift is a deliberate no-op; the zero-count section on the same page owns the behavior. |
| 5 | fixed | `overview.md`: the `0x05` row now reads “present; zero count guarded; other v3 behaviour not verified”. | The zero-count path is guarded in firmware; the rest of the v3 `0x05` path keeps its caveat. |
| 6 | fixed | `README.md`: “its **other** v3 behaviour is not verified”; firmware row “present; other v3 behaviour not verified”; limit bullet “**v3 is compiled in; other v3 behaviour is not verified on v3.**”  `overview.md`: firmware row “present; other v3 behaviour not verified”; closing note “apart from the guarded zero-count case, its hardware behaviour is **not verified on v3**.”  `limits-and-errors.md`: “other v3 behaviour is not verified on v3”. | These caveats now exclude the guarded zero-count path; the specifically guarded row and section carry the verified zero-count behavior. |

## Cross-set list

`docs/protocol/jtag/README.md` was **not** edited; it belongs to the accepted
JTAG set and needs a follow-up.  The three caveat-scoping passages are:

1. `docs/protocol/jtag/README.md:3-4` — scope the status line:

   > OpenOCD path on v3 (build-only; **other** v3 behaviour unverified) and
   > v4; the XSVF player is v4 only.  Other v3 behaviour in this set is
   > unverified.

2. `docs/protocol/jtag/README.md:81` — the availability row:

   > | OpenOCD path (`0x06`) | present; zero count guarded; other v3
   > behaviour unverified | present |

3. `docs/protocol/jtag/README.md:88` — the closing caveat:

   > The v3 column is **not verified on v3**, except the guarded zero-count
   > case.

Already correct in the JTAG set; no cross-set edit needed:

- `docs/protocol/jtag/openocd.md:120-121` — zero count consumes no payload
  and returns no TDO byte on either build; the echo is still sent.
- `docs/protocol/jtag/edge-cases.md:42` — zero-bit shift (v3 and v4):
  3-byte echo only; no payload read, no TDO byte.
- `docs/protocol/jtag/edge-cases.md:96` — “Other v3 behaviour in this set is
  build-only — **not verified**.”
- `docs/protocol/jtag/openocd.md:185-186` — “Other v3 behaviour is build-only
  and not verified.”

## Verification

- All relative links in `docs/tools/openocd/` resolve.
- Zero TAB characters; prose lines are at or below 82 columns; no dates,
  bench prose, pass counts, or tested/verified-result claims were added.
- No zero-count failure wording remains in the set: `commands.md` and
  `limits-and-errors.md` now describe the echo-only no-op on both builds.
- Counts: **6 fixed, 0 rejected, 1 cross-set item** (three passages in
  `docs/protocol/jtag/README.md`).
