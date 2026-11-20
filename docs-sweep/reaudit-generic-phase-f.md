# Generic prompt layer — focused presentation re-check

Scope: the two presentation items only. No fresh full review. Firmware tree
checked as before (`Firmware/`, `9b179cb5`).

**Verdict: 2 of 2 filled in full.**

## Per-item table

| # | Status | Current passage | Remainder |
|---|---|---|---|
| 1 | **Filled in full** | `docs/generic/usage.md:346-348`: "The board-state reset also stops Timer2, so any AUX PWM/servo output ends.  On v4 it releases `AUX1`/`AUX2`, clears their output routing and the `AUX1` open-drain bit, so every header I/O pin is an input afterwards." | None. It uses "output routing" and "open-drain bit" rather than `RPOUT`/`ODC`, and limits the open-drain clear to `AUX1`; no AUX2 ODC claim. |
| 2 | **Filled in full** | `docs/generic/navigation.md:247-250`: "A valid / `b10 <value>` (1–32767) skips the prompt; a missing, zero, malformed, or / over 32767 value reopens `Enter raw value for BRG` at the same `(34)>` prompt. / The menu default is 9 (115200), the rate the firmware boots at." | None. No line-leading `>`; "over 32767" keeps the re-prompt meaning intact. |

## Other changed generic passages

No other changed generic passage has a line-leading `>` or a code-identifier
leak. A scan of `docs/generic/*.md` found no line beginning with `>` and no
`RPOUT`, `ODC`, `BP_AUX`, `T2CON`, `U1BRG`, `MSG_`, `proc_menu`, `base.c`,
`core_system`, or `BP_HIZ` text. The two changed files are `usage.md` and
`navigation.md`; their diff is limited to the two passages above.

## Presentation

- No TAB characters in either touched file.
- No prose line exceeds 80 columns.
- The existing links and anchors resolve.
- No dates, bench/pass-count prose, or positive tested/verified claims were
  added; the existing `not verified` caveats remain the only such strings.
