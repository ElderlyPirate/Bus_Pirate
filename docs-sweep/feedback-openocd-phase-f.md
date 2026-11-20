# OpenOCD adapter — Phase F delta audit

**Scope:** the zero-count repair only.  I read the nine `docs/tools/openocd/`
pages and the two cross-linked JTAG pages that the tool set defers to.  I did
not review `tools/xsvf/`.

**Source check.**  PR `bpv4-oocd@992454d`, `Firmware/tool_openocd/openocd.c`:
the 3-byte echo is written at lines 219-222 before the v3 branch; the v3
`binOpenOCDTapShiftFast()` call (line 248) is inside `if (j)` at lines 224-231.
The v4 branch uses `while (bit_sequences > 0)` at line 260.  A zero count is
therefore echo-only on both builds: no payload read, no TDO byte.

**Verdict: no.**  The repair is not reflected.  `commands.md` and
`limits-and-errors.md` still say the v3 zero count is unsupported, unguarded
and unverified; the availability tables still blanket-caveat the whole v3
path, and the cross-set JTAG README does the same.  No other OpenOCD behavior
was re-opened.

## Findings

1. **[minor] `commands.md:194-198` still says the v3 zero count is unsupported.**
   Quote: “**v3:** a zero count is not supported; the v3 path has no
   zero-count guard.  Use at least one bit; the v3 outcome is unverified.”
   Why: the PR adds `if (j)` around the v3 `binOpenOCDTapShiftFast()` call
   after the echo; zero is a guarded no-op on both builds.
   Fix: replace the two bullets with:
   “A zero count is a guarded no-op on both builds: the 3-byte echo is sent;
   no payload is read and no TDO byte follows.”
   The existing “OpenOCD never asks for zero bits” can stay.

2. **[minor] `limits-and-errors.md:25` still tabulates “v4 no-op; v3
   unsupported”.**
   Quote: “| zero count | v4 no-op; v3 unsupported | do not send one;
   OpenOCD never does |”
   Why: v3 is the same guarded echo-only no-op; only the code path differs.
   Fix: “| zero count | no-op on both builds; 3-byte echo only | no payload
   read, no TDO byte; OpenOCD never sends one |”.

3. **[minor] `limits-and-errors.md:68-73` repeats the stale v3 section.**
   Quote: “**v3:** a zero count is not supported.  The v3 path has no
   zero-count guard; use at least one bit.  The v3 outcome is unverified.”
   Why: same source fact as finding 1; there is no v3 failure path.
   Fix: replace the section body with:
   “A zero count is a guarded no-op on both builds.  The 3-byte echo is sent;
   no payload is read and no TDO byte follows.”
   Keep “No host needs a zero-bit shift” only if you want the host note; it is
   not a caveat.

4. **[minor] `limits-and-errors.md:139` still lists zero count as a v3 edge
   failure.**
   Quote: “| a zero count on v3 | no defined reply; the shift does not complete
   normally | the v3 path has no zero-count guard | send at least one bit |”
   Why: there is a defined reply (the 3-byte echo), and the shift is a
   deliberate no-op; this row will send a custom host looking for a failure.
   Fix: delete the row.  The zero-count section in the same page owns the
   defined behavior.

5. **[minor] `overview.md:101` still blanket-caveats the whole `0x05` row.**
   Quote: “| `0x05` TAP shift | present, not verified on v3 | present |”
   Why: the zero-count path is not an unverified v3 failure; it is guarded in
   firmware.  The rest of v3 `0x05` can remain caveated.
   Fix: “| `0x05` TAP shift | present; zero count guarded; other v3 behaviour
   not verified | present |”.

6. **[minor] Blanket v3 caveats still re-caveat the zero-count path.**
   - `README.md:4-5`: “The path is compiled into both builds; its v3
     behaviour is not verified.”  Fix: “... its **other** v3 behaviour is not
     verified.”
   - `README.md:65`: “| firmware path (`0x06`) | present, not verified on v3 |
     present |”  Fix: “present; other v3 behaviour not verified.”
   - `README.md:105`: “**v3 is compiled in but not verified on v3.**”  Fix:
     “**v3 is compiled in; other v3 behaviour is not verified on v3.**”
   - `overview.md:97`: same firmware-path row as `README.md:65`; use
     “present; other v3 behaviour not verified.”
   - `overview.md:108-109`: “The v3 build includes the path, but its hardware
     behaviour is **not verified on v3**.”  Fix: “The v3 build includes the
     path; apart from the guarded zero-count case, its hardware behaviour is
     **not verified on v3**.”
   - `limits-and-errors.md:27-28`: “The v3 path is compiled into the build;
     its hardware behaviour is not verified on v3.”  Fix: “The v3 path is
     compiled into the build; other v3 behaviour is not verified on v3.”

## Cross-set list

These need the owning JTAG set, not this set:

- `docs/protocol/jtag/README.md:3-4`: “OpenOCD path on v3 (build-only,
  unverified) and v4 ... v3 behaviour in this set is unverified.”  Scope it:
  “OpenOCD path on v3 (build-only; **other** v3 behaviour unverified) ...”
  and “Other v3 behaviour in this set is unverified.”  The zero-count guard
  is already stated correctly in `openocd.md:120-121`.
- `docs/protocol/jtag/README.md:81`: “| OpenOCD path (`0x06`) | present,
  unverified | present |”  Use “present; zero count guarded; other v3
  behaviour unverified.”
- `docs/protocol/jtag/README.md:88`: “The v3 column is **not verified on
  v3**.”  Use “The v3 column is **not verified on v3**, except the guarded
  zero-count case.”

Already correct; no cross-set edit needed:

- `docs/protocol/jtag/openocd.md:120-121` — “A **zero count** consumes no
  payload and returns no TDO byte on either build; the 3-byte echo is still
  sent.”
- `docs/protocol/jtag/edge-cases.md:42` — “Zero-bit shift (v3 and v4) |
  3-byte echo only; no payload read, no TDO byte ...”
- `docs/protocol/jtag/edge-cases.md:96` — “Other v3 behaviour in this set is
  build-only — **not verified**.”
- `docs/protocol/jtag/openocd.md:185-186` — “Other v3 behaviour is build-only
  and not verified.”

## Coverage and presentation

- `entry-and-exit.md` and `exchanges.md` contain no zero-count claim.  The
  only mention in `exchanges.md` is the Related link “zero counts and
  recovery”; no wording change is needed.
- The targeted zero-count prose lines are 71–79 columns; no TAB characters,
  dates, bench prose, pass counts or tested/verified-result claims were found
  in the set.  The only long targeted passage is the `limits-and-errors.md:139`
  table row, which finding 4 removes.
