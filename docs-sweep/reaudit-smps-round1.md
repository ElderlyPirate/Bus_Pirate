# SMPS docs — re-audit of round 1 findings

**Scope:** the 17 findings in `feedback-smps-round1.md`, checked against the
current six files in `docs/capabilities/smps/` and the cross-set lines named in
`fixes-smps-round1.md`.  This is an item-by-item re-audit, not a fresh review.

**Verdict: 17 of 17 items filled in full.**

No item is partial, not addressed, or regressed.  Two firmware owner candidates
remain by design, and one cross-set item is still open outside the SMPS pages.

## Per-item results

| # | Status | Evidence quote (current docs) | Remainder |
|---|---|---|---|
| 1 | Filled in full | `binary.md:137` — "replies `0x00`, consumes the operand, and **stays in `RAW1`**"; `binary.md:147` — "The operand is consumed even when it is `0x00`; a zero operand is not an exit request on this path." `edge-cases.md:15-17` repeats "this includes `0x00`: `0xF2 0x00` replies `0x00`, consumes the zero, and does not leave raw wire." | None. The old `BBIO1` example is gone. |
| 2 | Filled in full | `wiring.md:17` — "The add-on named in this set is the legacy Dangerous Prototypes **Bus Pirate SMPS** board." The 2x5 pin table (`:22-33`), `JP1` mapping (`:35-38`) and "The schematic and its part list give no input or output voltage, current or power rating" (`:44-45`) are present; `README.md:52-57` links the section. | The supply rating remains undocumented by design; the page now says so, gives the `VTARGET` power-stage pin, and points to the archive/measure path. |
| 3 | Filled in full | `README.md:130` — "The firmware's cutoff can act only for requests `0x200`-`0x526`; whether the add-on can reach the target is unknown, and no request is a promise of a regulated output." `feedback-loop.md:110` — "This table describes the firmware comparison, not the add-on's regulated output range." | None. `binary.md:236-238` carries the same limit. |
| 4 | Filled in full | `README.md:86` — "## Shortest byte sequence (not a safety claim)"; `:90` — "Keep the load disconnected and verify the add-on output with your own measurement before connecting anything." The first start is now `0xF2 0x00` for `0x200` (`:98`). | None. |
| 5 | Filled in full | `binary.md:118` — "There is no freshness flag; treat that first read as stale and use a later read." `feedback-loop.md:84-86` — "Discard the first read after a start.  If you need evidence of a new conversion, poll until the value changes; a stable output can legitimately repeat, so that is a heuristic, not a guarantee." | CO-1 (firmware has no freshness flag) remains; the documented workaround is complete. |
| 6 | Filled in full | `README.md:45` — "**No pull-up is required** ... in the normal push-pull case." `wiring.md:74-75` — "the SMPS start does not clear the open-drain bit.  A v4 terminal reset (`#` or `0x0F`) does not clear it either; a power cycle or hardware reset does." `edge-cases.md:129-142` adds the OpenOCD scenario. | CO-2 (firmware does not clear `AUX1` open drain at start) remains; the documented recovery is complete. |
| 7 | Filled in full | `feedback-loop.md:140` — "refusal count `0x01`; no check runs; the self-test then sits in its idle echo loop - send `0xFF` to leave (`BBIO1` is not re-printed)." `edge-cases.md:170-174` gives the echo example and recovery. | None. |
| 8 | Filled in full | `feedback-loop.md:53-55` — "`target` is a raw comparison threshold on the same numeric scale as the 10-bit sample, but it is a 16-bit value.  It ranges from 397 ... to 3177 ...; a 10-bit sample can never reach a target above 1023." Table header is "Target (threshold)". | None. |
| 9 | Filled in full | `edge-cases.md:73-76` — "The duty follows the floating reading: it may sit at the computed duty or be cut to zero, and the output is uncontrolled.  A zero feedback can also mean a real 0 V output; the two cases look the same." Recovery adds "Remove the load and do not touch the output until the feedback path is verified" (`:77-79`). | None. `wiring.md:136-139` and `binary.md:243-246` repeat the result. |
| 10 | Filled in full | `README.md:68` — "**Check the build:** once in `RAW1`, send `0xF1`.  `0x01` means SMPS is built in; `0x00` means it is not.  It does not start the supply; if a loop is already running, it stops it." The shortest-session table lists the optional check (`:97`) and explains the `0x00` branch (`:105-107`). | None. |
| 11 | Filled in full | `README.md:38` — "Read in this order: wiring -> binary -> feedback-loop -> exit -> edge-cases.  Wire the Bus Pirate side before the first start." The wiring headline now precedes the five-minute version (`:41` vs `:59`). | None. |
| 12 | Filled in full | `README.md:64` — "The NULs need a host program or script; a terminal cannot send them (the Python example is in [binary.md](binary.md#a-host-sequence))." | None. |
| 13 | Filled in full | `README.md:22` — "No `k` selection is needed for SMPS: the start routes the PWM to `AUX1` regardless of the selected AUX target." `wiring.md:101-103` repeats it. | None. |
| 14 | Filled in full | `feedback-loop.md:142` — the `W` row says "refused with the same `ADC busy` line and syntax error; `w` still works"; the note at `:146-148` explains the rail check and says the binary `0x4x` bit is not blocked. `wiring.md:82-84` repeats it. | The row's detail-owner link is `[generic commands]`; the refusal detail is in `generic/edge-cases.md:20`. The row is self-contained, so this is a link nit, not a content gap. |
| 15 | Filled in full | `exit.md:80` — "On v4 the return runs the **terminal reset** (the firmware's board reset)." The table now has `0x0F` and `#` both "yes on v4 (terminal reset)" (`:112-113`); `m` is separately "board reset" (`:114`), explained by "the same board reset before the new mode's setup" (`:95`). | The original `0x0F`/`#` mismatch is gone. The `m` row's "board reset" is a deliberate subset; the page defines the terms. |
| 16 | Filled in full | `binary.md:40` — "read the last feedback sample (raw counts)"; `:45` — "The sample is the ADC-node count, not a calibrated output voltage." `README.md:75` uses "feedback sample". | None in the SMPS set; `protocol/binary/sub-protocols.md:289` now also says "last feedback sample (ADC-node count, not a calibrated voltage)". |
| 17 | Filled in full | `binary.md:76-87` — "## Reply shapes" is now a four-column table: Request, Reply, Shape, Notes; it includes the stale/zero and disabled-build rows. | None. |

## Required cross-checks

| Check | Status | Evidence |
|---|---|---|
| Exact disabled-build frame | Full | `binary.md:137-149` and `edge-cases.md:12-20`: one failure byte, operand consumed, stays in `RAW1`, including `0x00`. |
| Add-on limits | Full | The board is identified and pinned out; the page explicitly says the schematic has no input/output rating and the supply/output/feedback scaling are `unverified` (`wiring.md:40-47`). |
| Freshness rule | Full | No freshness flag; discard the first read or poll (`binary.md:118-120`, `feedback-loop.md:82-86`, `README.md:75-78`). |
| Ownership paths | Full | `d`/`D`, `0x14`/`0x15`, `g`/`S`, `0x12`/`0x13`, `0x10`/`0x11`, `P`, `W`, `0x5x` are in the ownership table (`feedback-loop.md:136-143`), with the self-test echo loop and `W`/`w` distinction stated. |

## Cross-set status

| Cross-set item | Status | Evidence |
|---|---|---|
| Binary sub-protocols wording | Applied | `protocol/binary/sub-protocols.md:289` — "last feedback sample (ADC-node count, not a calibrated voltage)"; `:295-297` — "the range where the firmware cutoff can act ... whether the add-on actually reaches a requested target is unknown." |
| Generic `W` refusal | Applied | `generic/edge-cases.md:20` now lists `W` and says "`w` and `p` are not blocked." |
| Dev verification record | **Open** | `dev/verification.md:45` still says "no SMPS board available; every electrical value is code-derived." The fixes record asks for "connector/pinout and jumper from the legacy schematic; input/output ratings still undocumented; firmware behaviour code-derived." This is outside the SMPS pages; main session to apply. |
| Optional selftest example | Not applied (optional) | `selftest/binary.md:143` and `checks.md:248` still use `0xF2 0x58` as an SMPS-specific start. The fixes record calls this optional consistency only, not a defect. |

## Code-owner candidates

- **CO-1 — stale `0xF0` sample with no freshness flag.** Accepted. The docs now
  state the limitation and the discard/poll workaround (`feedback-loop.md:84-86`,
  `binary.md:118-120`). Firmware is unchanged.
- **CO-2 — `AUX1` open-drain not cleared at SMPS start.** Accepted. The docs now
  name the OpenOCD open-drain path, say the SMPS start and a v4 terminal reset
  do not clear it, and give the power-cycle/hardware-reset recovery
  (`wiring.md:72-77`, `edge-cases.md:129-142`). Firmware is unchanged.

## Presentation check

- **TAB characters:** none in the six SMPS files.
- **Captures:** every `host ->` / `host <-` line in `binary.md` and `exit.md`
  aligns its comment at column 26; no misaligned capture columns.
- **Menus / option lists:** no menus in this set; pinout, reply-shape,
  ownership and exit-path lists are tables. The five-minute version is a short
  start list, not an option menu.
- **~80-column wraps:** no prose line over 80. Two link lines exceed:
  `exit.md:142` at 81 columns and `wiring.md:43` at 114 columns (a bare archive
  URL). Neither is a broken prose wrap; use a reference-style link if strict
  column count matters.
- **Bench prose / dates / tested claims:** none in the SMPS user pages. The only
  status qualifiers are the inline `unverified` / "not verified on v3" caveats.
  No run dates, pass counts, evidence sections, "captured on" provenance or
  internal log names. The add-on section cites the legacy schematic and a public
  archive link to identify the board and pinout; that is a source reference, not
  a bench/test provenance claim.

## New problems

1. **[minor] `feedback-loop.md:142` — the `W` detail-owner link points to
   `generic/commands.md`, while the actual SMPS refusal row is in
   `generic/edge-cases.md:20`.** The row and the note already state the
   behaviour, so this is only a navigation nit. Change the link to
   `../../generic/edge-cases.md` or add the refusal note to `generic/commands.md`.
2. **[minor] `wiring.md:43` — the archive URL is a 114-column line.** If the
   ~80-column rule is applied to links as well as prose, convert it to a
   reference-style link. The URL itself is not broken.

## Still-open items

- Cross-set: `docs/dev/verification.md:45` still says "no SMPS board
  available; every electrical value is code-derived." Main session to apply the
  fixes-record wording.
- Optional: the selftest pages still use `0xF2 0x58` as an SMPS-specific start
  example. The fixes record classifies this as consistency-only, not a defect.
- Minor link nit: the `W` ownership row's detail-owner link (see New problems
  #1).
