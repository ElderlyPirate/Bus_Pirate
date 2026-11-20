# I2C — Phase F delta audit

**Verdict: no.** The two required deltas are still stale in the current set:
`0x09` is documented as emitting terminal text, and `I2CEEPROMWIN` is still
documented as retrying without a limit. The terminal `a`/`A`/`@` text is
correct and separate; do not remove it.

Scope: only the statements changed by #202 / RA-28. Checked against
`Firmware/` at combined `9b179cb5` and `scripts/I2CEEPROMWIN.c` at the current
host tail. No docs edits.

## Findings

### 1. [blocker] `docs/protocol/i2c/binary.md:72` — the command table still promises optional text for `0x09`

> “`0x01`, optional text for drive/release, then the sub-result”

**Why it is wrong:** the final firmware's `0x09` path writes only the leading
`0x01` and then the sub-result byte. The extracted regression test asserts
exactly two reply bytes for every sub-command (`length==2`), including the
drive/release selectors. No terminal text is emitted. A host that follows this
table waits for a text line that never arrives.

**Exact fix:** change the reply cell to:

> “`0x01`, then the sub-result (always exactly two bytes)”

### 2. [blocker] `docs/protocol/i2c/binary.md:125-127`, `:232-245`, `:265-266`, `:302-305` — the AUX section, the consume notes and the worked example still teach the old text protocol

> “It is **not always two bytes**: the drive/release sub-commands print a
> terminal text line on the same wire between the two:”

The table that follows still lists:

> | `0x00` | `AUX LOW` then CR LF |  
> | `0x01` | `AUX HIGH` then CR LF |  
> | `0x02` | `AUX INPUT/HI-Z` (no line break) |

and the paragraph after it still says:

> “A host must consume that text before reading the sub-result.”

The same stale instruction survives in two more places:

> “release AUX with `0x09 0x02` (and consume the `AUX INPUT/HI-Z` text) before
> any bus traffic.” — `binary.md:125-127`

> “release it (`0x09 0x02`, high-Z; consume the `AUX INPUT/HI-Z` text) before
> bus traffic.” — `binary.md:265-266`

The worked exchange still has the text line:

> `host <- "AUX INPUT/HI-Z"` — `binary.md:304`

**Why it is wrong:** the final `0x09` reply is exactly `0x01` plus the
sub-result for `0x00`, `0x01`, `0x02`, `0x03`, `0x10`, `0x20` and an unknown
sub-command. The terminal helpers still print `AUX LOW` / `AUX HIGH` /
`AUX INPUT/HI-Z` for the terminal `a` / `A` / `@` commands only; the binary
command uses direct pad writes and is silent. Any host that consumes an
`AUX INPUT/HI-Z` string after `0x09 0x02` will block or read the next reply
byte as text.

**Exact fix:** replace the “not always two bytes” paragraph with:

> “After `0x09` the host sends one sub-command byte. The reply is always
> exactly two bytes: `0x01` (the command was accepted), then the sub-result.
> The first byte is not the sub-command's success flag; it is `0x01` even when
> the sub-command is unknown.”

Delete the “Text before the sub-result” table and the “A host must consume that
text” sentence. Change the two notes to “release AUX with `0x09 0x02` before
any bus traffic” and “release it (`0x09 0x02`, high-Z) before bus traffic.”
Delete `host <- "AUX INPUT/HI-Z"` from the worked exchange; keep:

> `host <- 0x01            command accepted`  
> `host <- 0x01            sub-result`

Keep the terminal `a` / `A` / `@` messages in
`docs/generic/commands.md:88` and `:139-140` untouched.

### 3. [major] `docs/protocol/i2c/eeprom.md:159-162`, `:186-187` — `I2CEEPROMWIN` still says the write retry is unbounded

> “A byte is retried until the chip acknowledges it; the summary line reports
> the byte count and the retry count.”

> “The write retry loop has no limit; a missing, unpowered or miswired chip
> leaves the tool retrying until it is interrupted.”

**Why it is wrong:** the current host tool limits each byte to
`WRITE_MAX_ATTEMPTS 100`, sleeps 1 ms between NACKed attempts, prints
`Write failed at memory address 0xNNNN after 100 attempts (no ACK)` and exits
7. In write mode it sets 1000 ms serial read/write timeouts; a failed or short
`ReadFile`/`WriteFile` prints `Serial I/O failed writing memory address 0xNNNN`
and exits 7. Both paths make a best-effort stop/exit/reset and close the port
and file, but do not roll back bytes already written. The docs say the opposite
in both the behaviour and limits sections.

**Exact fix:** change the write bullet to:

> “Writes are **one byte per transaction**: start, device address, memory
> address, data, stop. Page writes are not used, so a write burst cannot cross
> a page boundary. A byte is retried for up to 100 attempts, with 1 ms between
> NACKed attempts. If it still NACKs, the tool prints “Write failed at memory
> address 0xNNNN after 100 attempts (no ACK)” and exits 7; bytes already
> written remain. On success the summary reports the byte count and the retry
> count.”

Replace the no-limit limits bullet with:

> “In write mode the tool sets 1000 ms serial read and write timeouts. A failed
> or short serial transfer prints “Serial I/O failed writing memory address
> 0xNNNN” and exits 7. On either failure the tool attempts a stop/exit/reset
> and closes the port and file; there is no rollback, so a failed write can
> leave the EEPROM partially programmed. Read mode keeps the previous serial
> timeout behaviour; the 1000 ms limits are write-mode only.”

Add these rows to the Edge cases table:

> “A byte NACKs 100 times | “Write failed at memory address 0xNNNN after 100
> attempts (no ACK)” | earlier bytes remain; verify or re-run”  
> “A write-mode serial transfer fails or is short | “Serial I/O failed writing
> memory address 0xNNNN” | exits 7; cleanup is best effort; the EEPROM may be
> partially written”

## Cross-set list

- **`docs/generic/commands.md:88`, `:139-140`** — terminal `a` / `A` / `@`
  still print `AUX LOW`, `AUX HIGH` and `AUX INPUT/HI-Z, READ: ...`. This is
  correct; keep it. It is not the binary `0x09` reply.
- **`docs/protocol/binary/commands.md:159`** — only says the I2C extended-AUX
  command selects its own AUX/CS target; it does not state a reply shape. No
  change needed.
- **`docs/dev/tooling.md:31`** — only names `scripts/I2CEEPROMWIN.c`. No
  behaviour text to update.
- **Host-tool ownership:** the I2C set already owns the tool page
  (`docs/protocol/i2c/eeprom.md`). There is no separate host-tool page and no
  cross-set documentation gap to record.
- No stale `0x09` text or host-tool retry claim was found outside the I2C set.

## Presentation check (changed passages only)

The stale passages are presentation-clean: no TAB characters, no prose over
82 columns outside tables and code blocks, tables aligned, and no dates, bench
narrative, pass counts or internal log names. The AUX text table is aligned;
its problem is content, not layout. No separate presentation finding.
