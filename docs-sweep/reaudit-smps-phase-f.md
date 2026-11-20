# SMPS — Phase F re-audit

**Scope:** the 12 findings in `feedback-smps-phase-f.md`, checked against the
current six files in `docs/capabilities/smps/` and the final
`Firmware/` tree (`9b179cb5`).  No fresh full review.

**Verdict: 12 of 12 filled in full.**  No partial, not-addressed or regressed
items.  The SMPS set is clean; the five cross-set pages listed below still need
their ODC-clear additions.

## Per-item results

| # | Status | Evidence quote (current docs) | Remainder |
|---|---|---|---|
| 1 | Filled in full | `feedback-loop.md:84-87` — "A start clears the stored sample to zero.  Before the first new conversion, `0xF0` returns `0x00 0x00`; after a conversion it reports the new sample.  There is no validity flag: a real zero sample also reads zero, so a zero first read is not proof that a conversion has happened."  The old “does not clear” / discard / poll bullets are gone. | None. Steps 1 and 4 of “What a start configures” also say the start clears the sample and the `AUX1` ODC bit. |
| 2 | Filled in full | `binary.md:86` — "`0xF0` after a new start \| `0x00 0x00` until the first conversion; then the new sample \| ... the start clears the stored sample; no validity flag, so a real zero also reads zero."  `:118-120` — "returns `0x00 0x00` if no conversion has completed, otherwise the new sample.  It never returns the previous run's sample."  `:199-200` — "immediate `0xF0` read is `0x00 0x00` until the first new conversion; it cannot return the previous sample."  `:243-244` — "After a start, the first read is `0x00 0x00` until the first conversion; there is no validity flag."  Comments at `:105-108` and `:180` say “first read before a conversion” / “zero until the first conversion”. | None. The terminal-reset row correctly keeps the last stored sample; the reset does not clear it. |
| 3 | Filled in full | `README.md:76-80` — "A start clears the stored sample: before the first new conversion `0xF0` reads `0x00 0x00`.  There is no validity flag, so a real zero sample also reads zero."  `:101` — "read before the first conversion (zero)".  `:135-137` — "A start clears the stored sample.  The first `0xF0` read is `0x00 0x00` until the first conversion; ... a real zero sample also reads zero." | None. |
| 4 | Filled in full | `edge-cases.md:193-200` — "A `0xF0` read immediately after the new start reads `0x00 0x00` until the first new conversion"; the re-start "clears the stored sample to zero, clears the `AUX1` open-drain bit"; "there is no validity flag, so a zero sample can also be a real reading." | None. Recovery is only `0xF1`; no discard/poll. |
| 5 | Filled in full | `edge-cases.md:61-63` — "stop turns the converter off and leaves the stored sample unchanged.  A new start clears it; a power cycle or hardware reset also reinitializes it." | None. The “reading after stop repeats the last value” warning is retained. |
| 6 | Filled in full | `binary.md:124-134` — the example now shows `start`, then `host -> 0xF0` / `host <- 0x01 0xD3` “read after a conversion”, then `stop`, then the stopped read of `0x01 0xD3`. | None. The added read makes the nonzero stopped sample valid. |
| 7 | Filled in full | `README.md:45-49` — "The SMPS start clears the `AUX1` open-drain bit, and a v4 terminal or board reset (`#`, `0x0F`, or a mode change) also clears it.  If a binary path drives `AUX1` before the start without a terminal reset, a leftover open-drain high can still need an add-on pull-up." | None. The power-cycle/hardware-reset-only claim is gone; the pre-start binary pull-up warning remains. |
| 8 | Filled in full | `wiring.md:69-78` — "The SMPS start clears the `AUX1` open-drain bit; a pull-up is only needed if a binary path drives `AUX1` before the start while it is still open drain."  "The SMPS start clears the `AUX1` open-drain bit and drives the PWM push-pull.  A v4 terminal or board reset (`#`, `0x0F`, or a mode change) also clears it; no power cycle is needed."  `:111-115` — "the SMPS start clears the open-drain bit, so the PWM is push-pull after a start." | None. The pull-up warning is retained for the pre-start binary path. |
| 9 | Filled in full | `edge-cases.md:129-141` — entry retitled "`AUX1` is open drain before a binary SMPS start"; the start "clears the `AUX1` open-drain bit and drives the pin push-pull"; `#`/`0x0F`/`m` also clear it; "No power cycle is required." | None. Recovery is start or `#`/`0x0F`/`m`; pull-up only for a pre-start binary path. |
| 10 | Filled in full | `exit.md:35-36` — stop table adds "`AUX1` ODC \| unchanged"; `:113-121` — paths table has an `AUX1 ODC` column: `0x0F`, `#`, `m` and hardware reset/power cycle = cleared on v4; `0xF1`, `0x00`, `0xF1` in `BBIO1` = unchanged; `:81-85`, `:93-100`, `:104-109` name the ODC clear in the reset descriptions. | None. |
| 11 | Filled in full | `feedback-loop.md:21-30` — step 1 ends "and clears the stored sample to zero"; step 4 starts "Clears the `AUX1` open-drain bit, drives `AUX1` low, and routes the output-compare channel ...". | None. |
| 12 | Filled in full | `feedback-loop.md:35-36` — "Ownership lasts until `0xF1` or a v4 terminal/board reset (including a mode change); it is not tied to being in `RAW1`." | None. |

## Cross-set status

The fixes-record cross-set list is still pending in the main session; these
pages do not currently mention the V4 `AUX1` ODC clear:

- `docs/protocol/binary/commands.md:211-215` — v4 terminal-reset paragraph.
- `docs/protocol/binary/entering-and-framing.md:186-190` — v4 terminal-reset bullet.
- `docs/capabilities/auxiliary/aux-pin.md:158-161` — reset/persistence.
- `docs/capabilities/auxiliary/wiring.md:140-145` — leaving the board.
- `docs/capabilities/auxiliary/pwm-servo.md:129-134` — reset table (optional
  consistency).

`docs/protocol/binary/sub-protocols.md:290-296` is already correct for C6:
“a start clears it … a read after the new start is `0x00 0x00` until the first
conversion completes; a stop leaves the last sample unchanged.”  No change.

## Presentation

- No TAB characters in the six SMPS files.
- All `host ->` / `host <-` capture comments align at column 26.
- No prose line exceeds 80 columns; two link/reference lines are 81 and 110
  columns (`exit.md:147`, `wiring.md:191`), not prose wraps.
- All relative links and anchors resolve; the `binary.md#a-host-sequence` and
  `feedback-loop.md#ownership` anchors are present.
- No dates, bench prose, pass counts, “captured on” provenance or internal log
  names in the changed pages; the only qualifiers are the existing
  `unverified` / “not verified on v3” caveats.

**Finding count:** 12 of 12 filled in full; no new problems in the SMPS set.
