# SPI docs — extra-turn re-audit (round 2)

**Scope:** only the three items in `reaudit-spi-round1.md`: #8 propagation, the new binary `0x5x` safety issue, and the wide-row minor. No fresh full review.

## Per-item results

| Item | Status | Evidence (current docs) | Remainder |
|---|---|---|---|
| #8 — `0x02` propagation | **Filled in full** | `README.md:47-48`: "`0x02`/`0x03` write the CS latch low/high (assert/release while CS is an output)." `binary.md:90`: "CS latch low (asserts only while CS is an output)"; `binary.md:142`: "use `0x02` while CS is an output (see §Configure peripherals for the `0x4x` trap)"; `binary.md:308-310`: "Assert CS with `0x02` (output only) ... both act only while CS is an output"; `binary.md:442` and `edge-cases.md:115`, `edge-cases.md:124`: "while CS is an output". The recovery is consistent at `binary.md:193-196`, `binary.md:446-447` and `edge-cases.md:113-114`: another `0x4x` with bit 0 clear, or `0x6x`/`0x8x`; "A bare `0x02` writes the latch low but leaves the pin an input, so it cannot recover the connection." | — |
| New Major — binary `0x5x` safety | **Filled in full** | `binary.md:67-69`: "**Match the Vpullup source to the target's logic supply:** use `0x51` for a 3.3 V target and `0x52` only for a 5 V target. A 5 V pull-up can damage a 3.3 V target. See [wiring.md](wiring.md#the-pull-up-reference-vpullup)." The same warning is repeated at `binary.md:212-214` in §Pull-up feed, and `binary.md:476-477` adds the wiring page to Related docs. | — |
| Wide rows (#17) | **Partial — minor** | The cited CS-direction rows are now short: `binary.md:446` 108 chars, `binary.md:447` 122; `edge-cases.md:113` 114, `edge-cases.md:114` 109; `bus-operations.md:153` 126; `sniffer.md:204` 116. No new over-wide rows were introduced. | `edge-cases.md:15` (the "`W` succeeds with the target unpowered" row) is still 154 chars, not below the 130 the fix record claims: "`W` checks its own rails; power the target separately." The pre-existing `edge-cases.md:25` open-drain row remains 195 chars, the widest in the set. Both are presentation-only. |

**Verdict: 2 of 3 extra items filled in full; no remaining Major. The module is clean of Major findings.** #17 remains a minor partial.

## Presentation check (touched files: README, binary, edge-cases, sniffer, bus-operations)

- **No TAB characters; no trailing whitespace.**
- **Links resolve:** the new file targets exist, and the anchors `#the-pull-up-reference-vpullup` and `#bring-up-order-with-the-on-board-pull-ups-v4` match their headings.
- **No bench prose, dates, tested/verified-result prose, pass counts or internal log names.** The "not verified" caveats are the allowed inline form.
- **Prose wraps:** one 84-character line at `bus-operations.md:19`, within the "~80" target; no other prose over 82.
- **Table rows:** no new over-wide rows; the two remaining wide rows are pre-existing (`edge-cases.md:15` at 154 and `edge-cases.md:25` at 195).
