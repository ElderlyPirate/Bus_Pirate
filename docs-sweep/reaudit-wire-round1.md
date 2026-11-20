# Wire docs — round 1 re-audit

**Scope:** the 14 findings in `feedback-wire-round1.md`, checked against the current
`docs/protocol/wire/`, `docs/generic/`, and the changed `docs/protocol/binary/`
passages.

## Verdict

**13 of 14 items filled in full**

Open: **#1 (major)** — the ATR reset/read window is still not physically followable.

## Per-item results

| # | Status | Evidence (current docs) | Remainder |
|---|---|---|---|
| 1 | **Partial (major; code-owner)** | `2wire/macros.md:61-62`: "then read the reply.  The read happens after the final CS low, so the card's reset is asserted while the four values are sampled." `2wire/smartcard.md:56-57`: "reads the four values with reset still asserted". | The read window is now stated, but the stated window is unusable: a card held in reset cannot return an ATR. The fix record records a firmware candidate, but the user page gives no warning or workaround. Either release reset for the read (or read before the final low), or explicitly mark `(1)` as not ATR-compliant and likely to return garbage until the firmware is fixed. |
| 2 | Filled in full | `generic/usage.md:211`: `R2W (spd hiz)=( 1 1  )`. All other `R2W` captures checked carry the double space (`2wire/entry-and-setup.md:89`, `:110-112`; `wire/README.md:75`). | None. |
| 3 | Filled in full | `wire/README.md:99-103`: "2-Wire — SDA + SCL + ground (CS only for the ATR).  3-Wire — MOSI + MISO + CLK + CS + ground.  Fit the pull-up the mode needs: 1-Wire always; 2-Wire and 3-Wire only in open drain. ... in normal output skip both." | None. |
| 4 | Filled in full | `1wire/README.md:38-40`: "`O` switches standard ⇄ overdrive in place and prints the new speed; it changes only the host timing, so send the Overdrive Skip ROM byte `0x3C` at standard speed first". | None. |
| 5 | Filled in full | `wire/README.md:32` lists `3wire/binary.md`; `3wire/binary.md:1` exists; `3wire/bus-operations.md:205-206` links it; `docs/protocol/binary/sub-protocols.md:40` links it. | None. |
| 6 | Filled in full | `2wire/wiring.md:46-49`: "Open-drain 2-Wire needs one pull-up per line: SDA and SCL each need their own resistor. ... 2 kΩ – 10 kΩ from each of SDA and SCL to the pull-up voltage." `wire/README.md:41` and `2wire/edge-cases.md:11` repeat the rule. | None. |
| 7 | Filled in full | `wire/README.md:80-82`: "The lower-case forms `raw 2-wire` and `raw 3-wire` name the library, and `2 wires` / `3 wires` is the binary wire-count setting; neither is the mode name." Binary-page mode references now use `2-Wire` / `3-Wire`; remaining lower-case forms are the mapped library names. | None. |
| 8 | Filled in full | `generic/README.md:3-6`: "Examples use v4 output; v3 differences are noted inline and in the delta table ... The v3 build is included; its hardware behaviour is not verified." `wire/README.md` no longer contains "taken from the build" or "the per-mode sets repeat that caveat". | None. |
| 9 | Filled in full | `wire/README.md:3-4` states the v3 status once at cluster level; `wire/README.md:117-118` carries the availability note. Per-page generic footers are gone: `## v3 / v4` headings remain only in the three wiring pages and `wire/binary.md:304`. Remaining "unverified" passages are feature-specific (`1wire/edge-cases.md:75-80`, `2wire/edge-cases.md:56-58`, `3wire/edge-cases.md:54-56`). | None. The cluster page repeats the v3 caveat in its status and availability note; that is mild duplication, not the old every-page footer. |
| 10 | Filled in full | `1wire/bus-operations.md:94-110`: "### A plain byte exchange" with `{`, `BUS RESET  OK`, `0x33`, `WRITE: 0x33`, `r`, `READ: 0x28`, and "`]` and `}` are refused". | None. |
| 11 | Filled in full | `wire/binary.md:257-266`: `0x02` start before the bulk write and `0x03` stop after the read; `:271-272`: "START and STOP are optional for raw clocking". | None. |
| 12 | Filled in full | `2wire/binary.md:23-31`: a `Request / Meaning / Host sends / Reply / Terminal equivalent` table; `3wire/binary.md:26-34` uses the same shape. | None. |
| 13 | Filled in full | `1wire/timings.md:22-24`: "A 70 µs slot is about 14.3 kbps and a 12 µs slot about 83 kbps, so the label rates are names, not the reciprocal of the slot period." | None. |
| 14 | Filled in full | `wire/README.md:3-4`: "present in the v3 and v4 builds.  The v3 hardware behaviour is not verified." Availability cells now say `present`; `:117-118` defines the `—` cell. No `build-only` remains in the wire set. | None. |

## Presentation checks

- **TAB characters:** none. Scanned every `.md` under `docs/`; zero hits.
- **Aligned captures:** the `v` pin-table rows in `generic/usage.md` all start at
  the same columns (0, 9, 18, …, 99). The binary exchange comments are readable
  and start at column 21; the longer `(sampled byte)` reply shifts its comment
  once, but it is a command/reply block, not a column table.
- **Menus / option lists:** the entry pages still use tables. The new
  `3wire/binary.md` and the rewritten `2wire/binary.md` use command tables, not
  bullet chains.
- **~80-column wraps:** no non-table, non-code line in `docs/protocol/wire/` or
  `docs/generic/` exceeds 82 characters. The only longer lines are terminal
  captures and table rows.
- **No bench prose / dates / tested-result prose:** no `20xx` dates, bench/rig/
  battery/capture narratives, pass counts, evidence sections, provenance lines,
  or internal log names. The remaining "unverified" lines are short feature
  caveats (search/roster, `1W01`, `RAW1`, ATR timing, on-board pull-up value),
  which the documentation rules permit.
- **Naming:** the lower-case `raw 2-wire` / `raw 3-wire` forms are now explicitly
  defined as library names in `wire/README.md:80-82`; the mode prose is
  normalized.

## New problems found while re-reading (max 5)

1. **[severity: minor] The new 3-Wire binary page maps `0x04`/`0x05` to the
   terminal CS wrappers without qualifying the CS polarity.**
   `3wire/binary.md:32`: "`0x04` / `0x05` | CS low / high | — | `0x01` | `[` / `{`
   and `]` / `}`"; `:52-54`: "These are the binary counterparts of the terminal
   `[`/`{` and `]`/`}`, and they do not carry the 3-Wire read-back flag."
   With the `CS` entry choice (active-high, `csl` = 0), terminal `[` drives CS
   **high**, while `0x04` drives it **low**. The mapping is only true for the
   default `/CS`. Fix: qualify "with `/CS`" or show both polarities in the table.
2. **[severity: minor] `2wire/smartcard.md:35` still speaks of "a pull-up ... on
   the data line" while the corrected external rule is one pull-up on each of
   SDA and SCL.** The next sentence says the on-board group covers both lines,
   so this is not a contradiction for the on-board case, but an external
   smart-card build can still read it as a single-resistor instruction. Fix:
   say "one on SDA and one on SCL" here too, or point at the explicit rule in
   `wiring.md`.

## Loop note

Finding #1 remains **major**. Under the stated process cap, this round qualifies
for the one extra fix → re-audit turn. If the firmware candidate is not fixed,
the docs should at minimum carry an explicit ATR-unusable warning before the set
is called complete.
