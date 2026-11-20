# OpenOCD adapter — Phase F re-audit

**Scope:** the six numbered findings in
`docs-sweep/feedback-openocd-phase-f.md`, checked against the current
`docs/tools/openocd/` set.  Firmware checked against the final combined
`Firmware/tool_openocd/openocd.c`: the echo is written at line 223 before the
build split, the v3 transfer is inside `if (j)` at line 232, and the v4 loop is
`while (bit_sequences > 0)` at line 261.

**Verdict: 6 of 6 filled in full.**  The OpenOCD set is clean.  The separate
cross-set JTAG README item is still not edited; it remains a main-session
follow-up outside this set.

## Per-item table

| # | Status | Evidence (current docs) | Remainder |
|---:|---|---|---|
| 1 | **Filled in full** | `commands.md:194-199`: "A zero count is a guarded no-op on both builds: the 3-byte echo is sent; no payload is read and no TDO byte follows."  "OpenOCD never asks for zero bits" is kept as a host note. | None. |
| 2 | **Filled in full** | `limits-and-errors.md:25`: the zero-count row now says "no-op on both builds; 3-byte echo only" and "no payload read, no TDO byte; OpenOCD never sends one." | None. |
| 3 | **Filled in full** | `limits-and-errors.md:68-73`: "A zero count is a guarded no-op on both builds.  The 3-byte echo is sent; no payload is read and no TDO byte follows."  "No host needs a zero-bit shift" is kept as a host note. | None. |
| 4 | **Filled in full** | `limits-and-errors.md:128-145`: the edge-case table now goes from "full payload after a clamped count" to "a `0x07` confirmation at the wrong rate on v3"; the stale "a zero count on v3 ... no defined reply ... no zero-count guard" row is gone.  The zero-count section owns the defined behavior. | None. |
| 5 | **Filled in full** | `overview.md:101`: the `0x05` row now says "present; zero count guarded; other v3 behaviour not verified." | None. |
| 6 | **Filled in full** | `README.md:4-5`: "The path is compiled into both builds; its **other** v3 behaviour is not verified."  `README.md:65`: firmware row now says "present; other v3 behaviour not verified."  `README.md:105`: "**v3 is compiled in; other v3 behaviour is not verified on v3.**"  `overview.md:97`: same firmware-row wording.  `overview.md:108-109`: "The v3 build includes the path; apart from the guarded zero-count case, its hardware behaviour is **not verified on v3**."  `limits-and-errors.md:27-28`: "The v3 path is compiled into the build; other v3 behaviour is not verified on v3." | None. |

## Cross-set follow-up

Not part of the six in-set findings; the fixes record explicitly leaves it to
the JTAG set.  The three passages are unchanged in
`docs/protocol/jtag/README.md`:

- `:3-4`: "OpenOCD path on v3 (build-only, unverified) and v4 ... v3 behaviour
  in this set is unverified."
- `:81`: "| OpenOCD path (`0x06`) | present, unverified | present |"
- `:88`: "The v3 column is **not verified on v3**."

The JTAG set's detailed pages already have the correct zero-count statement:
`protocol/jtag/openocd.md:120-121`, `protocol/jtag/edge-cases.md:42`,
`protocol/jtag/edge-cases.md:96` ("Other v3 behaviour..."), and
`protocol/jtag/openocd.md:185-186` ("Other v3 behaviour...").

## Presentation check

- Zero TAB characters across the nine OpenOCD files.
- All Markdown tables have a consistent column count; no ragged rows.
- No non-table prose line outside code exceeds 78 columns.
- All relative links resolve; no broken anchors (`#leaving-and-recovery`
  exists).
- No dates, bench/rig/capture prose, pass counts or positive
  tested/verified-result claims were added.  The only verification language is
  the allowed "not verified / unverified" caveat.
