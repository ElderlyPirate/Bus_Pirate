# Core messages - core starter Part 4

Owns `messages.c/h`, `messages_v3.h/s` and `messages_v4.h/s`: the per-board
packed-string catalogs, table reader, help dispatch and confirmation helper.
Canonical inputs and generator remain in `tools/packstrings/`. No protocol
engine, SPI implementation or core_eeprom audit is included.

## Accepted content

The six scoped files at `f73a79a` exactly match accepted combined `cc8492d`.
`e2246f1` carries `00fa5bc`'s UART duplex prompt declarations/packed text;
`f73a79a` carries `2cde9c3`'s UART settings-header label. Both preserve original
author identity/date and cite the source. No UART feature engine is imported.

The 1-Wire message additions are already carried by Part 2 `74c3791`/`8ec4b70`;
software-profile labels are already carried by Part 3 `521e344` (source
`95f91cd`/`bf47033`). Earlier label revisions and equivalent aliases are not
duplicated. No missing reader implementation existed in the accepted delta.

The accepted packed-reader fixture from `0653843` is also retained as
`tests/test_messages.py`, with only grouped source paths adapted. The new
catalog contracts are in `tests/test_message_catalog.py`; neither suite
replaces the other.

## Findings and dispositions

| Candidate / finding | Final disposition |
| --- | --- |
| B1, stale built v3 colour row | `905242e`: match the existing intended mixed-case canonical row from `9a81c15`. V3 source/target proof, not a physical v3 claim. |
| B2, generator inputs drift from artifacts | `ddc187e`: preserve accepted UART prompt/header, A6, URL and SPI-menu bytes in canonical inputs. Canonical generation reproduces the final corrected catalogs, with unchanged public identifiers and flags. |
| B3, allegedly dead overrun message | Retain it. It has two standalone UART call sites and linked V3/V4 sections. It is GC'd only in combined's different UART implementation; that is not global deadness. |
| B4, reversed UART LFCR sequences | `97efea5`: baud prefixes and board macro menus use the existing serial CRLF contract; inputs and artifacts agree. |
| B5, exact-value dedup TODO | No encoding/refactor. V3 has no exact duplicate group; V4 only RESET. Per-message sections/GC avoid a material linked cost. `0aefbf1` documents that ownership. |
| B6 / top-10 #6, alleged core bulk-writer zero-space spin | Rejected by the actual composite production functions: at full occupancy, `write_cdc(0)` publishes the full packet and resets length, so the next iteration progresses. All 65 fills and seven payload lengths pass. Intentional host backpressure is not a local spin. No base/CDC change. |
| Backspace/ESC mixed-operand claim | Not byte-equivalent in general: actual XC16 `.pasciz <8>, " ", <8>` inserts NUL after the quoted space. The generator now emits one quoted literal with fixed-width octal escapes; reader unchanged. Actual target objects, not source concatenation, are the arbiter. |
| Disabled BASIC filesystem strings | Keep feature-owned public identifiers; section GC handles disabled features. No BASIC feature change. |
| Per-board help/pin order/bridge/macros | Keep intended board differences. Documentation and callers agree; actual message macros and payloads are tested separately. |
| Documented 160kps / output-type wording | Retain those public strings; no unrelated wording sweep. |
| New: v4 legend contains SOH and omits pin-one separator/number | `043b331`: use tab plus literal 1, giving twelve correct columns. Exact before/after terminal bytes and native checks. |
| New: shared UART header has seven fields but standalone emits six | `227ccce`: append the truthful full-duplex value 0 to the standalone serializer. Combined retains its actual duplex-state serializer. No UART algorithm change. |
| New: v3 help advertises obsolete dot width syntax | `c5bf538`: show semicolon and `0x55;2`, matching the shared parser and v4. |
| New: generator silently accepts corrupt input | `14fd7df`: reject bad escapes, malformed fields/flags, duplicate/invalid IDs, embedded NUL and non-byte payloads before output. Source-relative includes and explicit UTF-8/LF make invocation deterministic; valid payloads/flags stay unchanged. |
| Accessor bounds/API documentation | `0aefbf1`: document valid even, NUL-terminated packed table addresses, not a RAM byte range. The reader implementation is unchanged. |

The full common and board-specific tables, generated macros/declarations,
call sites and serializer boundaries were checked. The docs cross-check is
read-only: generic navigation, 1-Wire entry/speed, UART macros and bridge text.
The docs workstream may update its transcriptions separately.

## Packing and manual proof

`.pasciz` stores three payload bytes per 24-bit program word and appends NUL.
The table address advances by two per word. The reader uses TBLRDL low/high
bytes, then the low byte of TBLRDH; it carries the full table address across
page boundaries and restores the caller's TBLPAG. All three NUL positions,
empty messages, high-byte payloads and nested calls are covered. No length or
arbitrary flash-range promise is added to this ABI.

- DS70000157G 2.10 (p.21): TBLPAG supplies the upper eight address bits.
- DS70000157G TBLRDH/TBLRDL (pp.440-443): low word versus program bits 23:16,
  phantom high byte, even/odd byte semantics, table addressing and cycles.
- DS39897C 4.3.2: program-memory table access and its separate low/high paths.
- XC16 assembler `.pasciz` behavior is verified in the actual V3/V4 objects:
  all 288/308 sections have the expected payload, terminator and rounded word
  count. This is target-object proof, not an assumption from source spelling.
- DS50002106 5.3.5: each quoted `.pasciz` operand gets a trailing zero.
  One quoted operand with fixed-width octal escapes avoids embedded NULs;
  target-assembled synthetic empty/backspace/all-byte fixtures cover emission.
- CRLF, field counts and UI syntax follow the actual serial/parser contracts;
  a peripheral manual does not define those application strings.

Instruction inspection retains the accepted reader. No speculative caching,
new serializer, string merging, artificial length cap or wait/retry is needed.
Grouping and the packed object sections are compared independently of fixes.

## Regression gates

`python Firmware/tests/test_message_catalog.py` passes 15 checks, including:

- all 596 per-board identifiers/declarations/macros and their line flags;
- exact regeneration of four committed artifacts from canonical inputs;
- both-board colours/help/CRLF and UART label/value counts;
- 2814 packed output cases: every real macro at three table addresses plus
  lengths 0..512 on each board, all NUL lanes, upper bytes, page carry and
  no read beyond the message's final program word;
- nested writer TBLPAG restoration and all 256 confirmation input bytes;
- 455 core bulk-writer cases using actual CDC span/commit/flush functions,
  including a full initial buffer/zero-space commit;
- malformed generator inputs and source-relative includes.

The C fixtures execute extracted production functions with deterministic
table/serial/CDC mocks, not a PIC24/SIE simulator. Actual assembled payload
checks and clean XC16 V4/V3 builds provide the target-side complement.

`core_battery.py --messages` integrates the durable terminal leg: help, colour
columns, A6/URL, UART prompts/header/macro menu, negative confirmation/CRLF,
software-profile labels, both O messages and numeric prefix serialization.
It never enters SPI, accepts a bridge, generates bus signals, accesses XM64,
or reinstalls the removed AUX-CLK bridge. Core and CDC regressions run on both
final exact-banner images; combined includes the retained SUMP/BASIC callers.

The final tips, totals and log names are indexed by the local
`messages/acceptance-final.json` and the single comprehensive PR #209 body.
V3 message behavior is source/native/target-object/build proof, not a physical
V3 session. No physical malformed-pointer or USB-wedge experiment is used.

## Grouping

All six scoped files live in `Firmware/core_messages/`. Board headers remain
relative to `messages.h`. MPLAB item paths, exported include directories and
CLI source/include paths follow the move; existing clients retain ordinary
header spelling, so SPI needs no edit. Generator documentation and tests use
the grouped output paths. The pre-existing local family Makefile remains
untracked; the combined's tracked Makefile is updated.

`messages-group-proof.txt` records identical V4/V3 map summary rows, reader
instructions/relocations and packed-section contents across the move. Source
and assembled-byte proofs retain the exact accepted reader/confirmation code.
No WORKLOG edit, other-family rebase, SPI implementation/rig change or new PR
comment is part of this pass. Final bench state is HiZ, power off, AUX input.
Stop before core_eeprom.
