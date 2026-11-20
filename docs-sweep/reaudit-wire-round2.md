# Wire docs — round 2 verification

**Scope:** the remaining Major (#1) and the two re-audit minors (#15, #16), plus
presentation checks on the touched files.

## Per-item results

| Item | Status | Evidence quote | Remainder |
|---|---|---|---|
| #1 — ATR caution (code-owner) | **Filled in full (docs)** | `2wire/macros.md:66-69`: "**The four values are not a valid ATR.**  `(1)` samples them while the card's reset is asserted, so a card cannot answer; this is a known firmware limitation.  Do not use the decoded protocol, read-type or data-unit fields as card data." `2wire/smartcard.md:63-65` carries the same warning; `2wire/edge-cases.md:32` adds the trigger/symptom/recovery row. | None for the docs. The macro is now explicitly marked unusable as an ATR source; the remaining action is the recorded firmware candidate (release reset for the read, or read before the final low). |
| #15 — 3-Wire `0x04`/`0x05` polarity mapping | **Filled in full** | `3wire/binary.md:32`: "`0x04` / `0x05` | CS low / high | — | `0x01` | `[` / `{` and `]` / `}` (with `/CS`)"; `:52-55`: "with the `CS` choice the wrappers drive the opposite levels, so `0x05` matches `[`/`{` and `0x04` matches `]`/`}`." | None. The default `/CS` mapping and the active-high swap are both correct. |
| #16 — Smart-card pull-up count | **Filled in full** | `2wire/smartcard.md:34-39`: "**one pull-up (2 kΩ–10 kΩ) on each of SDA and SCL** ... An external build therefore needs two resistors, one per line." | None. |

## Presentation checks (touched files)

- **TAB characters:** none in `2wire/macros.md`, `2wire/smartcard.md`,
  `2wire/edge-cases.md` or `3wire/binary.md`.
- **Wraps:** no non-table, non-code line exceeds 82 characters in the touched
  files. The only longer lines are table rows.
- **Tables:** the ATR edge-case and the 3-Wire binary command reference remain
  tables; no new bullet-chain option list was introduced.
- **No bench/date/tested-result prose:** no dates, bench/rig/capture narratives,
  pass counts, evidence sections or log names. "Known firmware limitation" is a
  behavior caveat, not test reporting.
- **Links:** every relative file link in the touched files resolves; the
  `entry-and-setup.md#clock-speeds` anchor exists.

## Verdict

**3 of 3 items filled in full — no remaining Major in the documentation.**

The ATR feature is still a code-owner firmware candidate, but the user pages now
state the limitation and the practical consequence. No further documentation
round is needed for this module.
