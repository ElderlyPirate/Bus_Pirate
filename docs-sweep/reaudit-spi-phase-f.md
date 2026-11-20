# re-audit — SPI Phase F (RA-3 / C4)

Same auditor, findings from `feedback-spi-phase-f.md`. Re-read the current
`docs/protocol/spi/` pages and checked the load-bearing claims against the
final worktree firmware (`Firmware/protocol_spi/spi.c` `spi_binary_set_cs()`,
`SPI_BASE_COMMAND_CS_LOW/HIGH`, `spi_read_write_io()`;
`Firmware/protocol_binary/binary_io.c` `bp_binary_io_peripherals_set()`).
Docs head `0ef82aaa`. No unrelated content reviewed.

## Per-item table

| # | Status | Evidence in the current docs | Remainder |
|---|---|---|---|
| 1 (major) "cannot recover" workaround | **Filled in full** | `binary.md` §Configure peripherals: "`0x02`, `0x03` and a non-empty `0x04` set the direction themselves: `0x02` drives CS low and makes it an output, so it recovers a CS left as an input by `0x4x`; `0x03` releases CS (input/Hi-Z in open drain, driven high in Normal); `0x04` asserts through the same helper at the start and releases at the end. … A second `0x4x` is not needed for recovery." The old "only write the CS latch … cannot recover" text is gone. | — |
| 2 (major) command rows / CS paragraph | **Filled in full** | Command table: `0x02` = "CS low (assert; enables the output)"; `0x03` = "CS high (release: input/Hi-Z in open drain, driven high in Normal)". Paragraph: "`0x02` writes the CS latch low and makes CS an output, so it asserts. `0x03` writes the latch high and releases CS … Both set the direction". | — |
| 3 (major) auto-CS `0x04` after `0x4x` | **Filled in full** | §Write-then-read: "`0x04` uses the same CS helper: it drives CS low and makes it an output at the start, then releases it (input/Hi-Z in open drain, driven high in Normal) at the end." Edge table: "`0x4x` released CS as input … `0x02` or a non-empty `0x04` asserts". `edge-cases.md`: "not a failure: `0x04` drives CS low and makes it an output for the transfer, then releases it … no recovery needed". The zero/zero paragraph still says CS is unchanged. | — |
| 4 (minor) "while CS is an output" preconditions | **Filled in full** | `binary.md` bulk: "use `0x02` (it asserts and enables the output…)". `0x06`: "Assert CS with `0x02` before the read (it drives low as an output) and release it with `0x03` afterwards…". Both `0x06` edge rows: "first assert CS with `0x02` (it enables the output)". `edge-cases.md` `0x1x` row: "assert it with `0x02` (it enables the output)". README: "`0x02` asserts CS and enables the output; `0x03` releases it…". No "output only" or "while CS is an output" precondition remains. | — |
| 5 (minor) `0x03` release edge row | **Filled in full** | `binary.md`: "`0x4x` bit 0 set (open drain) … CS released as an input … expected; `0x02` asserts, `0x03` releases". `edge-cases.md`: "`0x03` after a `0x4x` released CS … CS released (input in open drain, driven high in Normal) … expected; use `0x02` to assert". The "latch only"/"pin stays an input" failure framing is gone. | — |

## Verification against the final tree

- `spi_binary_set_cs(LOW)`: `IOLAT &= ~CS` and `IODIR &= ~CS` → `0x02` drives
  low as an output, independent of the previous direction.
- `spi_binary_set_cs(HIGH)`: with `high_impedance == ON` it sets `IODIR |= CS`
  and `IOLAT |= CS` (input/Hi-Z); otherwise it sets the latch high and
  `IODIR &= ~CS` (driven high) → `0x03` release behaviour matches.
- `spi_read_write_io()` calls the same helper for `0x04` only after the
  zero-length early return → the "non-empty `0x04`" qualifier is correct and
  the zero/zero path still leaves CS unchanged.
- `0x4x` bit 0 still sets CS directly (`bp_binary_io_peripherals_set()`), so
  the trigger for the interaction is real; the recovery is now correctly
  `0x02`/non-empty `0x04`, not a second `0x4x`.

## Presentation

- No TAB characters in the set.
- The edited table rows are pipe-aligned; the changed prose stays in the set's
  usual ~80-column style (longest changed prose 83 columns).
- All relative links and anchors in the set resolve, including
  `wiring.md#the-pull-up-reference-vpullup`,
  `wiring.md#bring-up-order-with-the-on-board-pull-ups-v4` and the
  `§Configure peripherals` references.
- No dates, bench prose, pass counts or tested/verified-result claims were
  introduced; the only "verified" forms are the existing v3 "not verified"
  caveats.
- No pre-fix wording remains: a scan for "while CS is an output", "output only",
  "already an output", "does not change the pin direction", "writes the latch
  only", "cannot recover", "`0x4x` trap", "leaves the pin an input" and
  "transfer ignored" finds only the approved post-fix sentences (plus the
  unrelated `wiring.md` "open-drain output only" pull-up cell).

## Cross-set

No cross-set edit is needed. `docs/protocol/binary/sub-protocols.md:171-173`
still describes `0x4x` bit 0 correctly (set releases input/Hi-Z in open drain,
driven high in Normal; clear drives low) and its "check the mode page" pointer
remains valid. `docs/protocol/binary/edge-cases.md:268` and
`sub-protocols.md:313` "CS latched high" are the BBIO1 state-byte effect, not
the SPI `0x02` command. No other set repeats the pre-fix wording.

**5 of 5 filled in full**
