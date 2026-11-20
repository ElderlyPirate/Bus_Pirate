# I2C — round 1 re-audit

**Verdict: 23 of 23 items filled in full.** No original item is partial,
unaddressed or regressed. Three new minor problems are listed at the end; none
is a Major, so no extra audit round is required.

Scope: the 23 findings in `feedback-i2c-round1.md`, re-checked against the
current `docs/protocol/i2c/` files and the two cross-set files. This is not a
fresh full review.

## Cross-set checks

**`docs/SPEC.md` — v3 hardware engine.** `SPEC.md:59` now reads
`| · hardware engine | off | x | 100k/400k/1M | ✓ |`, and `SPEC.md:10-12`
defines `off` as “optional feature, not in the default build”. This matches
`README.md:10-11` (“standard on v4, optional on v3 and not in the default v3
build”), the README table at line 68 (“optional build, not the default”) and
`entry-and-setup.md:42-43` (“On v4, and on a v3 build that compiles the hardware
engine in (not the default v3 build)”). No open conflict.

**`docs/generic/commands.md` — `e` procedure.** The page now says “Enter a non-HiZ
open-drain mode (most bus modes default to it; some, like I2C, set it without an
output-type prompt), then issue `e2`/`e3`, switch the on-board rails on with `W`,
then `P`.” This agrees with the I2C rule at `entry-and-setup.md:167`:
“I2C has **no `Select output type:` prompt**”. The old requirement to answer an
output-type prompt is gone, and no I2C edit was needed.

## Per-item results

| # | Status | Current evidence | Remainder |
|---:|---|---|---|
| 1 | Filled in full | `README.md:76-79`: “Both boards have switched on-board pull-ups … v3 needs an external 0–5 V feed on Vpullup; an external pull-up is optional.” `wiring.md:53-54`, `:69-70` and `:150` now say the same. | None. |
| 2 | Filled in full | `README.md:10-11`: “standard on v4, optional on v3 and not in the default v3 build”; `README.md:68`: “optional build, not the default”; `entry-and-setup.md:42-43`: “on a v3 build that compiles the hardware engine in (not the default v3 build)”; `SPEC.md:59`: `off`. | Cross-set cell now consistent. |
| 3 | Filled in full | `binary.md:295-296`: “host -> 0x60            speed = software profile 0” followed by “host <- 0x01”. | None. |
| 4 | Filled in full | `entry-and-setup.md:183`: “park and remove the target rails”; `entry-and-setup.md:190-191`: “`w` removes the target rails only: it leaves `P` and the VPU source as they were.” | Original issue fixed; new v4-only `e1` qualifier noted at N1. |
| 5 | Filled in full | `docs/generic/commands.md:37-39`: “Enter a non-HiZ open-drain mode (most bus modes default to it; some, like I2C, set it without an output-type prompt)”; `entry-and-setup.md:167`: “I2C has **no `Select output type:` prompt**”. | Cross-set page now consistent; no I2C edit needed. |
| 6 | Filled in full | `README.md:41-42`: “`W` powers the target rails; skip it for a self-powered target, where `e`/`P` still work.” | Original issue fixed; new v4-only `e` qualifier noted at N1. |
| 7 | Filled in full | `entry-and-setup.md:160-163`: “Typing the header as the arguments to `m4` fails … the prompts then run. Typing it at the `I2C>` prompt is a syntax error.” `edge-cases.md:32` matches. | None. |
| 8 | Filled in full | `entry-and-setup.md:25-26`: “Only `m` with no number on the same line (or `m0`, which counts as no number) opens the menu.” | None. |
| 9 | Filled in full | `bus-operations.md:286-288`: “Set the pointer again before each read transaction; bytes within one transaction auto-increment. A new read address does not continue the previous write's pointer.” | None. |
| 10 | Filled in full | `clock-stretching.md:31-40`: “## Two outcomes” and “After a timeout: … It does not retry the abandoned command”; `clock-stretching.md:78-80`: “starts from the current line state; it does not retry the abandoned command”. `edge-cases.md:70` matches. | None. |
| 11 | Filled in full | `binary.md:232-235`: “`0x01` (the command was accepted) … The first byte is not the sub-command's success flag; it is `0x01` even when the sub-command is unknown.” `edge-cases.md:140` matches. | None. |
| 12 | Filled in full | `eeprom.md:154-158`: “`0x4C` (supply, pull-ups and AUX low) … `0x4C` drives AUX low and the tool never sends `0x09 0x02`, so leave AUX unjumpered when using it; a jumpered AUX clamps SDA/SCL.” | None. |
| 13 | Filled in full | `eeprom.md:186-187`: “The write retry loop has no limit; a missing, unpowered or miswired chip leaves the tool retrying until it is interrupted.” | Docs fixed; the host tool's unlimited retry remains a code-owner item, not a doc gap. |
| 14 | Filled in full | `wiring.md:82-84`: “The `v` table labels that column `VPU` and the reference card marks the pin `Vpu`; this set uses Vpullup in prose and quotes the other forms where they appear.” `README.md:89` keeps the same mapping. | None. |
| 15 | Filled in full | No `## Unverified and caveats` heading remains; `edge-cases.md` ends at line 151 with the Binary table. `clock-stretching.md:27` now says “On v4”; `speeds.md:44` says “at the default build”. | Short inline `not verified`/`unverified` caveats remain (README status and limits, per-table v3 footers, wiring/eeprom); they are inline caveats, not an evidence section. |
| 16 | Filled in full | `README.md:46-51`: “**Start/stop:** … **Write:** … **Read:** … **Repeated start:** …” — the old six-fact Traffic bullet is gone. | Original issue fixed; the section is now nine bullets, noted at N2. |
| 17 | Filled in full | `edge-cases.md:78`: “the driver is re-initialised; the pending bit stays set”; `clock-stretching.md:107-108`: “`W` re-runs the driver setup but leaves the pending bit set; resolve the read with `]` first.” | None. |
| 18 | Filled in full | `edge-cases.md:52`: “send multiple bytes; change the width only for a bit-level transfer”; `:53`: “leave the width at 8 in I2C”. | None. |
| 19 | Filled in full | `README.md:52-55`: “`(3)` and `(4)` drive the on-board EEPROM (v4); run `(3)` before `(4)`. Both require **2 Hardware**; from Software `(3)` prints … and `(4)` is silent.” | None. |
| 20 | Filled in full | `bus-operations.md:179-180`: “with a start, the address byte, an ACK read, and a stop”; `:209-210`: “Every probe ends with a stop, hit or no hit, so the bus is idle between addresses.” | None. |
| 21 | Filled in full | `README.md:106-108`: “**No generic device layer** … The only chip-specific support is the on-board EEPROM macros 3/4 (v4) and the external EEPROM tool.” | None. |
| 22 | Filled in full | `README.md:44-45`: “`P` is enough at the slow profiles; for 400 kHz/1 MHz add a 1.5–2.2 kΩ external pull-up on SCL.” | Original issue fixed; v3 qualification noted at N1. |
| 23 | Filled in full | `bus-operations.md:59-63`: the repeated-start example now ends “I2C>r / READ: 0xAA / I2C>] / NACK / I2C STOP BIT”. | None. |

## Presentation verification

- **TAB characters:** none in the ten I2C files, `SPEC.md` or
  `generic/commands.md` (mechanical scan).
- **Column alignment:** the new repeated-start capture (`bus-operations.md:59-63`)
  and the binary worked exchange (`binary.md:284-338`) are aligned. The
  intentional double space in `READ:  ACK` and the glued
  `WRITE: 0xA0I2C bus error` line remain and are still explained; they are not
  misalignment.
- **Option lists:** driver and speed menus are tables (`entry-and-setup.md:61-66`);
  the `(0)` list is a firmware capture, not a bullet list. Edge and binary
  references are tables.
- **Wraps:** no prose line over 82 columns outside tables and code blocks.
- **Bench/date/verification prose:** no four-digit years, no “tested”, no “captured
  on”, no pass counts, no evidence section, no internal log names. The dedicated
  “Unverified and caveats” section is gone. The remaining `not verified` /
  `unverified` text is a short inline caveat in the README, the per-table v3
  footers and the wiring/eeprom pages; it is not a bench narrative or an evidence
  section.

## New problems caused by the fixes

1. **[minor] New v4-only `e`/`e1` and `P` advice is not board-qualified.**
   `README.md:41-42`: “skip it for a self-powered target, where `e`/`P` still
   work.” `README.md:44-45`: “`P` is enough at the slow profiles; for 400 kHz/1
   MHz add a 1.5–2.2 kΩ external pull-up on SCL.” `entry-and-setup.md:190-191`:
   “Use `p` and `e1` to switch them off before rewiring.” On v3 `e`/`e1` do not
   exist (`entry-and-setup.md:245`: “v3 has no `e`”), and the v3 on-board pull-up
   value is “not covered (**unverified**)” (`wiring.md:69`). The original
   findings 4, 6 and 22 are fixed, but the new sentences read as if the v4 path
   applies everywhere.
   **Fix:** “where `P` works (and `e` on v4)”; “On v4, `P` is enough …”; “Use
   `p` and, on v4, `e1` …”.

2. **[minor] The five-minute version is now nine bullets.**
   `README.md:33-59` contains Enter, Wiring, Bring-up, Start/stop, Write, Read,
   Repeated start, Macros and Limits. The split fixed the six-fact Traffic
   bullet, but it grew the section from six to nine items and 27 lines; it now
   reads as a reference page rather than a five-minute scan. The front page is
   still short overall, but this section is no longer as concise as its heading.
   **Fix:** merge start/stop + repeated start and write/read, or move the traffic
   commands into a small table.

3. **[minor] `README.md:27` reads “sniffer in sniffer”.**
   The row says “scanner format in bus-operations, sniffer in sniffer.” The
   second “sniffer” is the file name, but it is the only occurrence without a
   path or code span, so it reads as a typo.
   **Fix:** “scanner format in `bus-operations.md`, sniffer format in
   `sniffer.md`.”

## Qualifications

No original finding is open. Two items remain outside the doc fix: the optional
v3 hardware-engine build is still source-supported but untested (the docs and
SPEC now consistently call it optional/`off`), and the host tool still has the
unlimited write retry and no AUX release (the docs now describe that behaviour).
The structural suggestions from round 1 were not applied and were not part of
the 23 findings.
