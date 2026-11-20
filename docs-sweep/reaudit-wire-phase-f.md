# Wire — Phase F re-audit (2-Wire ATR / RA-2 / C5)

Re-checked the five Phase F findings against the current
`docs/protocol/wire/` pages and the final firmware in this worktree
(`Firmware/protocol_wire/raw2wire.c`, combined `9b179cb5`).  No files under
`docs/` were edited.

## Per-item verdicts

| # | Status | Evidence in the current docs | Remainder |
|---|---|---|---|
| 1 | Filled in full | `2wire/macros.md:53-58`: "drive CS low (assert reset) and hold about 255 µs; advance one clock tick on SCL (CLK); drive CS high (release reset).  It then reads the four values with CS high and **leaves CS high** after it finishes."  The banner is explicitly identified as "a fixed firmware caption and does not describe the current edge order".  `:153` adds "`(1)` leaves CS high (reset released); no release step is needed." | None. |
| 2 | Filled in full | `2wire/macros.md:60-64`: "`(1)` releases reset before sampling, so the four values are the card's reply when a card answers; with no card, no power, or no pull-up they are idle or floating data."  The old "not a valid ATR" warning and its Limits bullet are gone; no stale phrase remains anywhere in the wire set. | None. |
| 3 | Filled in full | `2wire/smartcard.md:28`: "driven low then released high by the ATR macro; left high after it."  `:57-58`: "pulses the card reset (reset on CS) low, releases it high, then reads four values and decodes them."  `:82-84`: "`(1)` reads the ATR after releasing reset; the four values are meaningful when the card answers ... parse still reads exactly four values at the default 8-bit width."  The old warning block and duplicate limit are removed. | None. |
| 4 | Filled in full | `2wire/smartcard.md:63-68` is now "Card reset after the macro": "`(1)` releases the card reset (CS high) before it samples and leaves CS high; no release step is needed.  `(2)` does not touch CS.  If you assert CS low yourself (`C` then `a`, or binary `0x04`), release it with `C` then `A` (or `0x05`), or leave and re-enter the mode, which makes CS an input." | None. |
| 5 | Filled in full | `2wire/edge-cases.md:32-33`: the old `(1)` "sampled with reset asserted / not a valid ATR" row is deleted; the no-answer row remains.  The CS row now has the trigger "`(1)` finished, then the card expected to run", the symptom "no release step is needed", and the behaviour "`(1)` leaves CS high (reset released).  `(2)` leaves CS unchanged; if you asserted CS low yourself ... release it with `C` then `A` ...". | None. |

**5 of 5 filled in full** — the set is clean for Phase F.

## Checks

- **Parse-only `(2)`:** unchanged and still distinct.  `2wire/macros.md:66-70`
  says it "reads four values and decodes them, without driving the reset";
  `smartcard.md:66` says "`(2)` does not touch CS."  The firmware still calls
  only `read_atr_header()` for the parse-only macro.
- **Firmware match:** `trigger_atr()` is CS low → ~255 µs → one clock tick →
  CS high; `raw2wire_run_macro()` then falls through to `read_atr_header()`.
  The corrected pages match that order, the final CS-high state, and the
  parse-only path.
- **Stale-wording sweep:** a repeat search over `docs/protocol/wire/` for
  `valid ATR`, `not a valid`, `reset asserted`, `while reset`, `leaves CS low`,
  `left low`, `sampled with reset`, and `reset still asserted` returns no
  matches.
- **Presentation:** 0 TAB characters in the 2-Wire set; changed prose is within
  ~80 columns (table rows excluded); tables are aligned; every relative link
  and anchor in the wire set resolves; no dates, bench narratives, pass counts,
  or tested/verified prose in the changed passages.
- **Cross-set:** the previous cross-set list is unchanged.  `wire/README.md`,
  `wire/binary.md`, `3wire/bus-operations.md`, `docs/SPEC.md`, and
  `docs/dev/verification.md` mention the ATR only generically; none repeats the
  old reset/CS claim.  No cross-set edits are needed.

## Note

The firmware banner still prints the fixed caption `RESET HIGH, CLOCK TICK,
RESET LOW`.  The docs now explicitly say it is a fixed caption and is not the
edge order, which is the correct user-facing handling; a caption change would
be a code-owner decision, not a docs defect.
