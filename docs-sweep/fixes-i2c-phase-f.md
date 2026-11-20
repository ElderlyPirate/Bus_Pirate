# I2C docs — Phase F delta repair

Scope: `docs/protocol/i2c/**` only. No firmware, host-tool, generic-set,
binary-set, `docs/dev/` or WORKLOG edits. No commit or push.

**Result:** 3 findings: 3 fixed, 0 rejected, 0 cross-set edits required.

## Per-finding record

| # | Decision | Change (file + short quote) | Evidence |
|---:|---|---|---|
| 1 | fixed | `binary.md:72` command table: “`0x01`, then the sub-result (always exactly two bytes)”. Removed “optional text for drive/release”. | `Firmware/protocol_i2c/i2c.c`, `binary_io_enter_i2c_mode()` case `9`: `REPORT_IO_SUCCESS()` sends `0x01`, then `user_serial_transmit_character(fr)` sends the sub-result. The direct pad writes are used with the comment “Do not call the terminal helpers here: their messages would corrupt the reply.” |
| 2 | fixed | `binary.md` AUX section now reads “The reply is always exactly two bytes: `0x01` (the command was accepted), then the sub-result. The first byte is not the sub-command's success flag; it is `0x01` even when the sub-command is unknown.” Deleted the “Text before the sub-result” table and the “host must consume” sentence. Both release notes now say only `0x09 0x02`; the worked exchange keeps `host <- 0x01 command accepted` and `host <- 0x01 sub-result`, and the `"AUX INPUT/HI-Z"` line is gone. | Same `i2c.c` case `9`: the `0x00`, `0x01`, `0x02`, `0x03`, `0x10`, `0x20` and default branches all write exactly two reply bytes; none calls a terminal text helper. The terminal `a`/`A`/`@` messages live in `proc_menu.c` and are not on the binary path. |
| 3 | fixed | `eeprom.md` write bullet: “A byte is retried for up to 100 attempts, with 1 ms between NACKed attempts. If it still NACKs, the tool prints `Write failed at memory address 0xNNNN after 100 attempts (no ACK)` and exits 7; bytes already written remain.” Limits bullet documents the write-mode 1000 ms serial timeouts, `Serial I/O failed writing memory address 0xNNNN`, exit 7, best-effort cleanup/no rollback, and the unchanged read mode. Added the two audit edge-case rows: “A byte NACKs 100 times” and “A write-mode serial transfer fails or is short”. | `scripts/I2CEEPROMWIN.c`: `WRITE_MAX_ATTEMPTS 100`; the per-byte loop increments `attempts`, checks the ACK bytes, prints `Write failed at memory address 0x%X after %d attempts (no ACK)` at 100, writes `\x03\x00\x0F`, closes the port/file and returns 7; `Sleep(1)` runs between NACKed attempts. Write mode sets `ReadTotalTimeoutConstant` and `WriteTotalTimeoutConstant` to 1000; a failed or short `WriteFile`/`ReadFile` prints `Serial I/O failed writing memory address 0x%X` and returns 7. The cleanup writes `\x03\x00\x0F` and closes before returning; no rollback. |

## Cross-set note

No cross-set changes are required.

- `docs/generic/commands.md:88`, `:139-140` — the terminal `a` / `A` / `@`
  messages (`AUX LOW`, `AUX HIGH`, `AUX INPUT/HI-Z, READ: ...`) are correct
  and were left untouched. They are not the binary `0x09` reply.
- `docs/protocol/binary/commands.md:159` — only states that the I2C
  extended-AUX command selects its own AUX/CS target; it does not describe a
  reply shape. No change needed.
- `docs/dev/tooling.md:31` — only names `scripts/I2CEEPROMWIN.c`; it has no
  behavior text. No change needed.
- Host-tool ownership: the I2C set already owns the tool page
  (`docs/protocol/i2c/eeprom.md`); there is no separate host-tool page and no
  documentation gap to record.

## Verification

- Checked each finding against the final tree in this worktree: `Firmware/`
  combined `9b179cb5` (firmware `2d82dc0`) and `scripts/I2CEEPROMWIN.c`
  (host tail `c3e4c436`).
- Re-read `binary.md` and `eeprom.md`; no `optional text`, `not always two
  bytes`, `Text before the sub-result`, `must consume`, `AUX INPUT/HI-Z`, or
  unbounded-retry text remains in `docs/protocol/i2c/`.
- Checked local links and heading anchors in `docs/protocol/i2c/`; all
  resolve. No TAB characters and no prose over 82 columns outside tables and
  code blocks.
- No commit or push; no firmware, host-tool or WORKLOG edits.

## Files changed

- `docs/protocol/i2c/binary.md`
- `docs/protocol/i2c/eeprom.md`
- `docs-sweep/fixes-i2c-phase-f.md` (this record)
