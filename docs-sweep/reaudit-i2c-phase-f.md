# I2C — Phase F re-audit

**Verdict: 3 of 3 filled in full.** No original finding is partial, not
addressed or regressed. The set is clean for the Phase F delta.

Scope: the 3 findings in `feedback-i2c-phase-f.md`, checked against the current
`docs/protocol/i2c/` and the final `Firmware/` / `scripts/I2CEEPROMWIN.c` tree.
No fresh full review.

## Per-item results

| # | Status | Current evidence | Remainder |
|---:|---|---|---|
| 1 | Filled in full | `binary.md:72`: “`0x01`, then the sub-result (always exactly two bytes)”. The old “optional text for drive/release” phrase is gone. | None. |
| 2 | Filled in full | `binary.md:232-235`: “The reply is always exactly two bytes: `0x01` (the command was accepted), then the sub-result.” The text-before-sub-result table and the “must consume” sentence are gone. `binary.md:255-256` now says only “release it (`0x09 0x02`, high-Z) before bus traffic.” The worked exchange is exactly `host <- 0x01` command accepted and `host <- 0x01` sub-result (`binary.md:292-294`); the `AUX INPUT/HI-Z` line is gone. | None. |
| 3 | Filled in full | `eeprom.md:161-165`: “A byte is retried for up to 100 attempts, with 1 ms between NACKed attempts. If it still NACKs, the tool prints `Write failed at memory address 0xNNNN after 100 attempts (no ACK)` and exits 7; bytes already written remain.” `eeprom.md:189-195` documents the write-mode 1000 ms serial timeouts, `Serial I/O failed writing memory address 0xNNNN`, exit 7, best-effort stop/exit/reset, no rollback, partial-write caveat and the unchanged read mode. The two audit edge rows are present at `eeprom.md:213-214`. | None. |

## Verification

- **Firmware:** the final `0x09` path sends exactly `0x01` plus the sub-result;
  `0x00`, `0x01`, `0x02`, `0x03`, `0x10`, `0x20` and unknown all take the same
  two-byte reply shape. The extracted regression test asserts `length==2` for
  every sub-command. Terminal `a` / `A` / `@` still print
  `AUX LOW` / `AUX HIGH` / `AUX INPUT/HI-Z`; those are separate and unchanged.
- **Host tool:** `WRITE_MAX_ATTEMPTS 100`, `Sleep(1)` between NACKed attempts,
  the `Write failed ... after 100 attempts (no ACK)` path returns 7, write mode
  sets 1000 ms read/write timeouts, short/failed serial transfers return 7, and
  both failure paths make the best-effort `0x03 0x00 0x0F` cleanup and close the
  port and file. The page matches the source, including the partial-write
  caveat and the read-mode scope.
- **Presentation (touched files):** no TAB characters; no prose over 82 columns
  outside tables and code blocks; the new command, AUX and EEPROM tables are
  aligned; captures are aligned; all local links and heading anchors resolve;
  no dates, bench prose, pass counts or internal log names.
- **Cross-set:** no stale `0x09` or host-tool text outside the I2C set. Terminal
  `a` / `A` / `@` text in `docs/generic/commands.md` remains correct and
  untouched.

No open items.
