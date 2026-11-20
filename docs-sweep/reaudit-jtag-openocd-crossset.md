# JTAG README — openocd cross-set follow-up re-check

Focused check of the three blanket-v3 caveat passages in `docs/protocol/jtag/README.md`. Verified the guard against the final local `Firmware/` (`2a4da4b`): `tool_openocd/openocd.c` writes the 3-byte `0x05` echo before the build split, guards the v3 transfer with `if (j)`, and enters the v4 path with `while (bit_sequences > 0)`, so zero count skips payload and TDO on both builds. No other pages reviewed.

| # | Item | Status | Evidence quote |
|---:|---|---|---|
| 1 | Status lines 3-4: v3 caveat scoped to “other v3 behaviour” | Filled in full | `README.md:3-5` — "**Status:** OpenOCD path on v3 (build-only; **other** v3 behaviour unverified) and v4; the XSVF player is v4 only.  Other v3 behaviour in this set is unverified." |
| 2 | `OpenOCD path (0x06)` availability row | Filled in full | `README.md:82` — "\| OpenOCD path (`0x06`) \| present; zero count guarded; other v3 behaviour unverified \| present \|" |
| 3 | Closing caveat | Filled in full | `README.md:89` — "The v3 column is **not verified on v3**, except the guarded zero-count case." |

**3 of 3 filled in full.**

Presentation: no TAB characters; all Markdown tables retain consistent column counts; no non-table prose line exceeds 80 columns; every relative link and anchor resolves. No dates, bench narratives, pass counts, provenance, or internal log names.
