# KEYB docs — re-audit round 2

Scope: the three extra-turn items from `reaudit-keyboard-round1.md` (items 3 and 5, plus the keypad-5 minor), checked against the current `docs/protocol/keyboard/` files and the extra-turn section of `fixes-keyboard-round1.md`. Read-only; no fresh full review.

| Item | Status | Evidence (current quote) | Remainder |
|---|---|---|---|
| 3 — external-Vpullup order | **Filled in full** | `wiring.md:122`: "select `e1`, feed `Vpullup`/`VPU` from the external supply (0–5 V), then `P`". `wiring.md:94`: "select `e1`, connect the external feed, then `P`". `edge-cases.md:124`: "select `e1`, connect the external feed, then `P`". `wiring.md:126`: "`e1` refuses while Vpullup already has voltage, so select it before connecting an external feed." | None; the table and both recovery notes agree on `e1` → feed → `P`. |
| 5 — Echo/BAT presence test | **Filled in full** | `edge-cases.md:45-60`: test capture `READ: 0xAA` then a second `r` with `READ: 0xEE`; "Read again; a BAT byte means the keyboard is alive, but it is not the echo. The returned `0xEE` is the positive evidence." Failure only on `NACK`/`TIMEOUT`, or reads that "never return `0xEE`". | None; `0xEE` is the positive result and `0xAA`/`FC`/`FD` no longer causes a false failure. |
| Keypad-5 minor | **Filled in full** | `scancodes.md:199-201`: "With Num Lock off, keypad 1–4 and 6–9 send the navigation codes above; keypad 5 has no navigation assignment, and its Num Lock-off behavior is keyboard-dependent." | None. |

## Presentation check — touched files

- `wiring.md`, `edge-cases.md`, `scancodes.md`: **0 TAB bytes**.
- Prose wraps: no non-table, non-code line over 80 columns.
- Links and anchors: all resolve; table column counts consistent; code fences balanced.
- No bench prose, dates, pass counts, capture provenance, internal log names, or "tested" claims. The only evidence-like phrase is the natural "positive evidence" in the Echo test; it is not bench prose.
- `wiring.md:94` and `edge-cases.md:124` agree with the supply table; no stale "feed before `e1`" wording remains.

**Verdict: 3 of 3 extra-turn items filled in full. Overall: 21 of 22 findings filled in full, 1 rejected correctly (item 12), no remaining Major — module clean.**
