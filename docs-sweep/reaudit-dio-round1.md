# DIO — re-audit, round 1

**Verdict: 18 of 18 items filled in full.**

Numbers match `feedback-dio-round1.md`. I re-read the current DIO pages and
the two generic pages touched by the fixes. The firmware code-owner candidate
is still open, but the DIO pages now describe its actual effect.

| # | Status | Evidence in current docs | Remainder |
|---:|---|---|---|
| 1 — safe park | Filled in full | `park-and-reset.md:52–53`: "`w` releases only the five signal pins; `#` releases every I/O pin". `:131–135`: "After `#` (or a mode change): every I/O pin must show `I` … including `AUX1` / `AUX2`"; after `w`, "only the five signal pins are released" and "`AUX1` / `AUX2` are unchanged". `README.md:55–58` uses the same split and says "use `#` if `AUX1` / `AUX2` may have been driven". | Code-owner candidate remains: `dio_cleanup()` still does not release `AUX1`/`AUX2`. The docs now state that behavior and route to `#`; no DIO text says `w` releases them. If the firmware changes, update `README.md`, `park-and-reset.md`, `wiring.md` and `edge-cases.md` together. |
| 2 — all-five outputs | Filled in full | `pin-control.md:104–105`: "with nothing connected … make only `CLK` an output". `:121–123`: "`0x1B` … Only `CLK` is driven; the other four stay inputs." `:127`: "`r` need not read `0x04` … because the four input bits float." `read-back.md:93–94`: "`0x1F` makes all five inputs, so no pin drives the target"; `:104–105`: "The other four inputs float". | Only `0x00` remains in the direction reference, with "Only use `0x00` when all five nets are safe to drive" (`pin-control.md:55–56`). No live-target all-output recipe remains. |
| 3 — `0x40` / `0x20` | Filled in full | `pin-control.md:41–45`: "`0x40` or `0x20` is a direction write, not 'supply on' or 'pull-ups on'.  Both make all five pins outputs and leave the supply and pull-ups at their previous settings." `edge-cases.md:43`: the matching row gives `0xC0` / `0xE0` / `0xA0` recovery. | None. |
| 4 — `r` and AUX1/AUX2 | Filled in full | `aux-pin.md:63–67`: "For `AUX` and `CS`, … `r` and `v` both show the result; for `AUX1` and `AUX2`, only `v` shows it, because `r` reads the five signal pins." `:127–130`: "A numeric value cannot reach `AUX1` or `AUX2`." | None. |
| 5 — naming | Filled in full | `aux-pin.md:28`: "`A0` on v4, `aux` on v3". `:33–36`: the prose repeats the v4/v3 split. `README.md:88–89`: "`A0` / `CS` / `A1` / `A2` on v4, and `aux` / `CS` on v3". `wiring.md:21`: "`A0` (v4) / `aux` (v3)". | No `AUX0` remains anywhere in `docs/protocol/dio/`. |
| 6 — selected target | Filled in full | `aux-pin.md:43–48`: "Run `i`.  The status block prints `a/A/@ controls <pin> pin` … `v` shows the pin states, not the selection." `README.md:51–52`: "Run `i` to see the selected target; `v` does not show it." | None. |
| 7 — `W` setup | Filled in full | `entry-and-setup.md:99–102`: "`W` starts the rails and runs the mode's setup hook, which is empty in direct I/O: it does not change directions, levels, latches or pull-ups." `park-and-reset.md:84–86`: "runs the mode's setup hook, which is empty in direct I/O". `generic/navigation.md:213–215`: "In DIO the setup hook is empty, so `W` only switches the supplies." | Cross-set edit present. `generic/README.md:57` and `generic/usage.md:165` still use the general "bus setup" phrase, but they are not DIO-specific and the DIO page is explicit. |
| 8 — target supply | Filled in full | `README.md:42–45`: "`GND`, the signal pins you use, and at most one supply rail … For a self-powered target, leave both rails unconnected." | None. |
| 9 — what survives | Filled in full | `park-and-reset.md:55–68`: the table is now `w` / Mode change / `#` with rows for supply, five pins, `AUX1`/`AUX2`, pull-ups, VPU source, latches, width, bit order, AUX target, display, prompt and macros. `:70–75`: "`w` leaves three settings alone: the bit order, the AUX target and the display format"; "A mode change … keeps the display format and the macros". | None. |
| 10 — levels vs latches | Filled in full | `edge-cases.md:87`: "the pins stay inputs and the latches stay low (the pin levels float)". | None. |
| 11 — `{` error | Filled in full | `read-back.md:113–115`: "`ERROR: command has no effect here`, then the dispatcher's `Syntax error at char 1`; nothing is enabled." `edge-cases.md:108–113`: same two-line reply, plus the `(n)` blank-line note. | None. |
| 12 — `v` capture | Filled in full | `read-back.md:69–73`: "The device emits tabs between columns; the spacing above is a fixed-column transcription … `Pinstates:` is the title … The first row … is the probe-colour legend; it is omitted above." The capture at `:63–65` now has all 12 columns. | None. |
| 13 — value-pin order | Filled in full | `README.md:69`: "pins a value can address … `AUX` (4), `MOSI` (3), `CLK` (2), `MISO` (1), `CS` (0)". `wiring.md:34–40` gives the same bit order in the cross-reference table. | None. |
| 14 — orphan wraps | Filled in full | `README.md:9–10` now ends "… or any pin no bus mode exposes."; `entry-and-setup.md:93–95` ends "empty setup hook."; `park-and-reset.md:114–117` ends "`w`, even after the supply goes off." | The three cited paragraphs no longer end in a single word. One different one-word tail remains at `aux-pin.md:86` (below). |
| 15 — edge-case layout | Filled in full | `edge-cases.md:8–35` is a "Symptom index"; `:39–48`, `:72–87` and `:101–104` are four-column Trigger / Symptom / Behaviour / Recovery tables. The five deep cases retain headings (`:50`, `:61`, `:89`, `:106`, `:120`). | The tables are wide in raw Markdown but valid and readable when rendered. |
| 16 — v3 caveat | Filled in full | Kept at `README.md:75` ("The v3 column is **not verified** on v3 hardware") and `entry-and-setup.md:40–41`; `read-back.md:129–130` keeps the specific "the v3 table is unverified" note. | No per-page footer remains in `wiring.md`, `pin-control.md`, `aux-pin.md`, `park-and-reset.md` or `macros.md`. |
| 17 — build check | Filled in full | `README.md:38–41`: "On a custom build, run `i` and look for `DIO` in the bracketed mode list, or use the number the `m` menu prints; the numbers above are the default image." | None. |
| 18 — width / cleanup | Filled in full | `generic/values.md:96–99`: "until a mode change, reset, or a mode cleanup restores 8 bits (DIO's `w` restores 8)". `park-and-reset.md:63` gives width 8 bits for `w`; `:70–71` says `w` resets it. | Cross-set edit present. |

## Code-owner and cross-set checks

- **Finding 1 / safe park:** `park-and-reset.md:114–117` says `w` does not
  release `AUX1`/`AUX2`; `README.md:55–58`, `wiring.md:140–142` and
  `edge-cases.md:86` all route the reader to `#` or a mode change. This is
  consistent with current `dio_cleanup()`. The code-owner candidate is a
  firmware decision, not a remaining docs contradiction. If `dio_cleanup()`
  is changed to release `AUX1`/`AUX2`, the four pages named in the fixes
  record must change together.
- **Finding 18:** `docs/generic/values.md` now includes the mode-cleanup case;
  the DIO page and the generic page agree.
- **Finding 7:** `docs/generic/navigation.md` now says the DIO hook is empty;
  the DIO pages say the same. Other generic pages use the general "bus setup"
  wording, but do not make a DIO-specific claim.

## Presentation check

- **TAB characters:** none in `docs/protocol/dio/` or `docs/generic/`.
- **Aligned captures:** pass. The `read-back.md` `v` capture starts its 12
  columns at 8-column intervals; the entry/menu and macro captures are
  single-column and readable.
- **Menus / option lists:** pass. The README task and file lists, the wiring
  and park tables, and the edge-case index are tables. The mode menu remains
  an exact terminal capture, which is appropriate.
- **~80-column wraps:** mostly pass. The three paragraphs from finding 14 are
  fixed. One single-word tail remains at `aux-pin.md:86`; short tails at
  `entry-and-setup.md:95` and `park-and-reset.md:125` are not single words but
  could be joined.
- **Bench prose / dates / tested-result claims:** pass. No bench, rig, date,
  pass-count, "captured on", evidence or log-name prose. The only verification
  language is the allowed negative "not verified" / "unverified" caveat.
- The new four-column edge-case tables are wide (up to 399 characters per
  row), but they are valid tables and the symptom index keeps them navigable.

## New / remaining problems

1. **[minor] `wiring.md:31` is false for v4.** It says: "The header order, the
   value-bit order and the `v`-table order are three different sequences."
   For v4 the header order and the `v`-table order are identical; only the
   value-bit order differs. The mapping table at `:34–40` actually shows this.
   Fix: "On v3 these are three different sequences; on v4 the header and
   `v`-table orders match, while the value-bit order differs."
2. **[minor] One single-word wrap remains at `aux-pin.md:86`.** The paragraph
   ends "… leaves the pin as an / input:". The three paragraphs cited in
   finding 14 are fixed, but this is the same presentation problem. Rewrap it;
   consider also joining the 2–3-word tails at `entry-and-setup.md:95`
   ("empty setup hook.") and `park-and-reset.md:125` ("was driven.").
3. **[minor] `read-back.md:57–58` and `:84–85` count "four" floating inputs.**
   "the four inputs float" and "the four floating input levels are examples
   too" refer to the four free signal inputs, but `AUX1` / `AUX2` are also
   inputs in the same capture and their `L` / `L` entries are equally
   examples. A reader may take them as driven low. Say "the floating input
   levels (including `AUX1` / `AUX2`) are examples too."

## Still open

- Firmware code-owner candidate: `dio_cleanup()` leaves a `k`/`K` + `A`
  driven `AUX1`/`AUX2` asserted after `w`. The docs describe this correctly;
  the fix, if any, belongs to the code owner.
- The three minor documentation nits above. No blocker or major item remains.
