# SPI docs — round 1 re-audit

**Scope:** the 17 findings in `feedback-spi-round1.md`, checked against the current `docs/protocol/spi/`. Finding 4 was the cross-set item in `docs/generic/`; it is judged on the SPI page only, as instructed. The generic follow-ups recorded in `fixes-spi-round1.md` are noted but not counted against the SPI set.

## Per-item results

| # | Status | Evidence (current docs) | Remainder |
|---:|---|---|---|
| 1 | **Filled in full** | `entry-and-setup.md:287-290`: "**The sniffer** configures its own pins as inputs and, on exit, runs the master pin/module setup: CLK, MOSI and CS become outputs. It does not restore the input state..." Same statement in `sniffer.md:38-40`, `wiring.md:150-152` and `binary.md:346-348`; `wiring.md:148-149` now says the master configuration **can** drive before the macro. | — |
| 2 | **Filled in full** | `wiring.md:85-89`: "**Set Vpullup to the target's logic supply.** A 5 V pull-up on a 3.3 V target can damage it: on v4 use `e2` for a 3.3 V target and `e3` only for a 5 V target..." The row is repeated at `wiring.md:161` and `edge-cases.md:26`. | The binary page's `0x5x` section still lacks this warning (see new problem 2). |
| 3 | **Filled in full** | `wiring.md:97-113` is the numbered order: wire, open drain, `e2`/`e3`, `W`, `P`, `v`. `README.md:32-35` and `entry-and-setup.md:254-256` state "`e2`/`e3` before `W`, then `P`" and link the block. | SPI pages are consistent; `generic/commands.md:39` still says "then issue `e2`/`e3`, then `P`" (recorded cross-set #3, outside this set). |
| 4 | **Filled in full (SPI side)** | `bus-operations.md:101-106`: "`;N` ... changes the **displayed** width and the window used by the bit-order setting; it does not make the transfer N bits wide." This matches the documented rule in `fixes-spi-round1.md`. | No SPI change was needed. `generic/values.md:83-90` still advertises `r;16` as "one 16-bit read"; recorded for the main session, outside this set. |
| 5 | **Filled in full** | `sniffer.md:12-20` is a five-step procedure: 1 disconnect target, 2 run `(1)`/`(2)`, 3 connect target, 4 read stream, 5 press any key while the master is quiet. `sniffer.md:38-40` states the exit pin state. The example shows the blank line and `SPI>`. | — |
| 6 | **Filled in full** | `entry-and-setup.md:21-27`: the table separates "`m` [Enter], then the number at `(1)>`" from "`m5` or `m 5`", and the prose says "Only `m` **alone**, with no number on the same line, opens the menu." | — |
| 7 | **Filled in full** | `README.md:40-43`: "The flag is sticky: `]`/`}` and `w`/`W` do not clear it; `[` does." | — |
| 8 | **Partial — new inconsistency** | The trap itself is fixed: `binary.md:185-192` says "A bare `0x02` writes the latch low but leaves the pin an input, so it cannot recover the connection"; `binary.md:436-437` and `edge-cases.md:113-114` use the observable symptom "the transfer is ignored; CS never goes low". | The corrected `0x02` caveat is not applied elsewhere. `binary.md:138` still says "assert CS with `0x02` first"; `binary.md:299` says "Assert CS with `0x02` before the read"; `binary.md:432`, `edge-cases.md:115` and `edge-cases.md:124` repeat it; `README.md:47-48` says "`0x02`/`0x03` assert/release CS"; `binary.md:86` still calls `0x02` "CS low (assert)". A reader who has just done a `0x4x` with bit 0 set is told both that `0x02` cannot recover and that `0x02` is the way to assert CS. |
| 9 | **Filled in full** | `binary.md:25-28`: "`0x06` is the `BBIO1` OpenOCD/JTAG entry but the extended AVR helper inside `SPI1`." `binary.md:273-276`: a zero length returns the two acks and no data; the version is "the big-endian version `0x0001` (version 1)". `binary.md:305-306`: "present in the default v3 and v4 builds; a custom build without the extended AVR option answers `0x00`". | — |
| 10 | **Filled in full** | `README.md:3`: "**Status:** v3 and v4 documented; the v3 hardware behaviour is not verified." `README.md:60`: "included in the default build"; `README.md:71`: "The v3 column is **not verified**." No "build-only" remains in the SPI set, and the third README caveat is gone. | — |
| 11 | **Filled in full (SPI)** | `README.md:87-89`: "**Buffers.** The *terminal output buffer* is the 4096 B queue... The *command buffer* is the input line buffer, 128 B on v3 and 256 B on v4. They are separate buffers." `sniffer.md:148-151` and the availability tables use "terminal output buffer". | `generic/README.md:138` still says "terminal command buffer"; recorded cross-set #5, outside this set. |
| 12 | **Filled in full** | `macros.md:71-77` adds the `(11)` example with the changed header `( 4 1 1 0 1 0 )`; `macros.md:108` says "The v3 behaviour is **not verified**" (no invented column). | — |
| 13 | **Filled in full** | `entry-and-setup.md:40-42`: heading "### Menu-form example"; "The default v4 menu is shown"; the word "capture" is gone from the section. `entry-and-setup.md:107-108` keeps the spacing note without provenance language. | — |
| 14 | **Filled in full** | `README.md:13-15` adds "**Start here:** [The five-minute version]"; `README.md:47-51` now lists `0x02`/`0x03`, `0x4x` and `0x6x`; `README.md:103-105` adds `o` and `l`/`L` to the shared grammar. | — |
| 15 | **Filled in full** | `binary.md:180-183`: "Every `0x4x` request also drives AUX as an output; bit 1 sets its level. For example `0x4C` = power on + pull-ups on + AUX low + CS low... `0x44` is pull-ups on + AUX low + CS low." The worked frame comment at `binary.md:394` is updated. | — |
| 16 | **Filled in full** | `sniffer.md:105-108`: "At a boundary the closing `]` can be delayed until the next `[` is pending, so the two markers can print together before the next byte pair; the byte pairs themselves stay in bus order." `sniffer.md:162-164`: "send `0x01` to stop and immediately get `SPI1` back; send `0x00` to stop and exit to `BBIO1`." `edge-cases.md:98-99` repeats both. | — |
| 17 | **Partial — one row regressed** | The bus-operations row is shorter (126 chars at `bus-operations.md:153`). The edge-cases row is shorter than the original 200 chars but still 174 chars at `edge-cases.md:114`: "the latch is high but the pin stays an input". | `binary.md:436` is now 148 chars (was 108) and `binary.md:437` is 169 chars (was 135); the binary CS-direction rows grew, not shrank. `edge-cases.md:15` (195 chars) and `sniffer.md:204` (197 chars) remain the widest in the set. The class of problem is still present. |

**15 of 17 items filled in full.**

Open items: **#8** (major: the `0x02` caveat is not propagated) and **#17** (minor: wide rows remain, and the binary rows grew).

## Presentation check

- **TAB characters:** none in the eight SPI files.
- **Aligned captures:** the new `(11)` header capture (`macros.md:74-76`) and the five-step sniffer transcript (`sniffer.md:22-29`) are clean; the binary frame comments are still aligned at their comment columns.
- **Menus / option lists as tables:** the setup prompt chain, speed menu, `(0)` macro list, binary command table and edge-case tables are tables. The new bring-up order and sniffer usage are numbered procedures, which is the right form for a sequence.
- **~80-column wraps:** one prose line is 84 characters (`bus-operations.md:19`); everything else is 82 or less. This is within the "~80" target. The wide table rows are the only real presentation issue (see #17).
- **No bench prose, dates, tested/verified-result prose:** no dates, no pass counts, no "captured on", no internal log names. The only "capture" uses are the sniffer's live-bus capture. The "not verified" caveats are the allowed inline form; "The text and spacing of the option lines are the firmware's" is a transcription-fidelity note, not a test result.

## New problems

1. **[severity: major] The corrected `0x02` caveat is not propagated.** `binary.md:191-192` says a bare `0x02` "cannot recover the connection", but `binary.md:138`, `binary.md:299`, `binary.md:432`, `edge-cases.md:115`, `edge-cases.md:124` and `README.md:47-48` still tell the reader to "assert CS with `0x02`", and the command table at `binary.md:86` still calls `0x02` "CS low (assert)". This is the same trap the fix was meant to clarify. Fix: qualify every `0x02` instruction with "when CS is an output; after a `0x4x` that released it, use another `0x4x` with bit 0 clear or `0x6x`/`0x8x`", and change the command-table entry to "CS latch low (assert only when the pin is an output)".
2. **[severity: major] The binary `0x5x` pull-up feed still lacks the target-voltage warning.** `binary.md:61-62` offers `0x51` = 3.3 V and `0x52` = 5.0 V, and `binary.md:199-205` lists the same choices, but neither says the source must match the target's logic supply or that a 5 V pull-up can damage a 3.3 V target. The warning exists in `wiring.md:85-89` and `edge-cases.md:26`, but the binary page does not link `wiring.md` in that section (its Related docs link entry, sniffer, bus-operations, edge-cases and the binary pages, not wiring). Fix: add "use `0x51` for a 3.3 V target and `0x52` only for a 5 V target; a 5 V pull-up can damage a 3.3 V target" to §Pull-up feed and link `wiring.md` §The pull-up reference.

## Cross-set follow-ups (outside this set)

Recorded in `fixes-spi-round1.md` for the main session; not counted in the 15/17:

- `docs/generic/values.md` §Width — still says "`;N` sets the read/write width in bits" and "`r;16` | one 16-bit read" (finding 4).
- `docs/generic/commands.md` §Feeding the Vpullup pin — still says "`e2`/`e3`, then `P`" (finding 3).
- `docs/generic/README.md` availability — still says "terminal command buffer" rather than mapping to the SPI set's "command buffer" / "terminal output buffer" (finding 11).
- `docs/generic/navigation.md` §Mode menu — still has the `m`-alone / `m <N>` wording (finding 6 consistency).
