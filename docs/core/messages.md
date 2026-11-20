# Messages (`core_messages/`)

**Status:** current · maintainer-facing.  Covers the packed string catalogs,
the program-memory reader, the help block and the confirmation helper.  Most
user-visible text comes from this group; a few messages are literals in the
module that owns them (for example the v4 baud notice in `proc_menu.c`, the
version strings in `base.h`, and some mode-specific lines).  The wording itself
is documented by the user sets.  v3 is build-only here: its catalog is
source-derived and **not verified on v3 hardware**.

## Scope and files

| File | Responsibility |
|---|---|
| `messages.c` | packed-message reader, CRLF variant, `print_help()`, `agree()` |
| `messages.h` | selects the board catalog and declares the reader API |
| `messages_v3.s` / `messages_v4.s` | generated per-board message tables |
| `messages_v3.h` / `messages_v4.h` | generated declarations and statement macros |

Authoritative inputs and the generator live in `tools/packstrings/`
(`bus_pirate_common_strings.txt`, `bus_pirate_v3_strings.txt`,
`bus_pirate_v4_strings.txt`, `packstrings.py`).  The build consumes the
committed `.s`/`.h` files; it does not regenerate them.

The v4 catalog carries the shared set plus the AUX1/AUX2, VPU-source and XSVF
text; the v3 catalog carries the shared set plus its bootloader/version text.
Shared strings appear in both catalogs because each board links its own table.
Counts are build artifacts, not a contract: read the committed `.s` files or
the generator output.  The identifier mix is `BPMSG*` (numeric legacy),
`MSG_*` (symbolic) and `HLP1000`..`HLP1022` (the help block).

## Storage model

Each message is an assembler label in its own section:

```
.section .text.<identifier>, code
.global _<identifier>_str
_<identifier>_str:
    .pasciz "<payload>"
```

Consequences:

- `.pasciz` packs three payload bytes into each 24-bit program word (low word
  first, then the low byte of the high word).  The assembler appends one NUL
  terminator.
- One section per message lets the linker discard text for features that are
  compiled out.
- A message is addressed by an even PIC24 **table address**, not a RAM pointer.
  The generated header produces the address with
  `__builtin_tbladdress(<identifier>_str)`.
- The generator emits exactly one quoted literal per message, using fixed-width
  octal escapes for arbitrary non-NUL bytes, so an embedded escape cannot
  introduce an unintended early terminator.
- The generated `.s` is sorted by identifier, not by catalog order.  The help
  order is explicit in `messages.c`.

Every generated identifier has both a declaration and a statement macro:

```c
void MSG_RESET_MESSAGE_str(void);
#define MSG_RESET_MESSAGE \
    bp_message_write_line(__builtin_tbladdress(MSG_RESET_MESSAGE_str))
```

Callers write `MSG_RESET_MESSAGE;` (no parentheses).  The macro already selects
buffer output or buffer-plus-CRLF.

## Reader

```c
void bp_message_write_buffer(unsigned long strptr);
void bp_message_write_line(unsigned long strptr);
```

`bp_message_write_buffer()` walks the packed words:

1. Set `TBLPAG` from bits 23:16 of `strptr`.
2. Read the low byte of `TBLRDL` (byte 0), then the high byte (byte 1), then
   the low byte of `TBLRDH` (byte 2).
3. After byte 2, advance the table address by two and repeat.
4. Stop after transmitting a NUL; restore the caller's `TBLPAG`.

`bp_message_write_line()` runs the buffer writer, then emits one CRLF pair.
The reader restores the caller's `TBLPAG`, so a nested table read is safe.
Page boundaries are handled because `TBLPAG` is recomputed from the full
address on every word.

The API has **no length parameter**.  Callers must supply a valid, even,
NUL-terminated message address produced by the generated catalog.  It is not an
arbitrary flash-range reader, and there is no over-read guard.

## Naming

Three identifier families exist, all produced from the same catalog inputs:

| Family | Example | Use |
|---|---|---|
| numeric legacy | `BPMSG1086` | older call sites; kept stable |
| symbolic | `MSG_COMMAND_HAS_NO_EFFECT` | preferred for new code |
| help | `HLP1000`..`HLP1022` | the fixed `?` block, printed line by line |

The identifier is internal.  The displayed payload is the name authority for
user documentation; when the firmware shows two forms for one item, the user
docs state the mapping once.

`print_help()` is a fixed list of `HLP1000`..`HLP1022` (23 lines).  It does not
enumerate the catalog and does not depend on the active mode.

`agree()` prints `BPMSG1135` (`Are you sure? `), reads one byte, echoes it,
prints CRLF, and returns true for `y` or `Y` (the comparison masks bit 5).  Any
other byte means "no".  It performs no retry and does not validate the input.

## Generation and catalog contracts

`tools/packstrings/packstrings.py <input> <output-prefix> <v3|v4>` produces the
`.s` and `.h` pair.  The input format carries an identifier, a flag (`0` =
plain buffer output, `1` = append one CRLF) and a payload.

The generator rejects, before writing anything:

- malformed rows or flags;
- unknown escapes and embedded NULs;
- invalid or duplicate identifiers;
- payloads that are not byte strings.

Includes (`#include` lines) resolve relative to the input file, not the
process working directory.  Output is UTF-8 without BOM with LF line endings,
and entries are written in identifier order, so the contents are deterministic.
The output prefix is relative to the process working directory; run the
generator from `tools/packstrings/` so the checked-in paths resolve:

```
cd tools/packstrings
python packstrings.py bus_pirate_v3_strings.txt ../../Firmware/core_messages/messages_v3 v3
python packstrings.py bus_pirate_v4_strings.txt ../../Firmware/core_messages/messages_v4 v4
```

The third argument is the header include-guard tag (`v3` ->
`BP_MESSAGES_V3_H`).  A wrong prefix writes a pair the Makefile never compiles.

Regeneration is checked by comparing normalized text, so a CRLF checkout does
not create a false mismatch; payload bytes, flags and identifiers are checked
independently.

## Failure modes and edge cases

| Trigger | Symptom | Behaviour | Recovery |
|---|---|---|---|
| Bad pointer | unrelated program memory is printed until a zero byte | no bounds check; an odd address or missing terminator is not detected | pass only a generated, even, NUL-terminated table address |
| Page crossing | none | `TBLPAG` is recomputed per word | no split needed |
| Embedded NUL | output truncates at the NUL | the generator refuses NUL; a hand-written `.pasciz` string can contain one | remove the NUL from the payload |
| Missing string for a compiled-out feature | link error | the per-message section lets the linker drop it; a call site that stays compiled in must not reference it | guard the call site or include the string |
| Confirmation input | the LF of a CRLF remains for the next read | `agree()` consumes exactly one byte, echoes it, and returns true for `y`/`Y` only | type `y` or `n` alone |
| Feature text on the wrong board | link error, not a blank line | a message compiled out of a board is absent | compile it for that board or guard the caller |

## Invariants

- Catalog addresses are even table addresses; `TBLPAG` is caller state and is
  always restored.
- The reader transmits payload bytes only; it never pads, wraps or line-breaks
  on its own.  `bp_message_write_line()` adds exactly one CRLF.
- The generated header and assembly are a pair; edit the catalog inputs and
  regenerate, never patch one artifact alone.
- Message identifiers are stable within the catalog; the displayed text may
  change without renaming call sites.
- `print_help()` ordering is explicit in `messages.c`, not derived from the
  catalog order.
- The generator sorts by identifier; a diff that only reorders the inputs does
  not change the generated files.

## Native test coverage

`tests/test_messages.py` compiles the production reader and runs it on both
address models with mocked table reads, including page crossings, `TBLPAG`
restoration and every `agree()` input byte.  Coverage includes messages up to
1 KiB and table addresses near the top of a page.

`tests/test_message_catalog.py` checks the generated artifacts against the
canonical inputs: identifiers, flags, per-message payloads and lengths, every
real macro at three table addresses plus lengths 0..512, all three NUL lanes,
upper bytes, page carry, no read past the final program word, nested `TBLPAG`
restoration, and malformed-input rejection.

Run either directly from the repository root:

```
python Firmware/tests/test_messages.py
python Firmware/tests/test_message_catalog.py
```

Both are host-side tests with deterministic mocks; they do not execute the
assembler's `.pasciz` encoding on target.

## Related docs

- [system.md](system.md) — where the help block, confirmation and status text
  are used.
- [navigation](../generic/navigation.md) — the user-visible navigation and
  help text.
- [macros](../generic/macros.md) — the user macro layer that prints through
  these catalogs.
- [building](../dev/building.md) and [tooling](../dev/tooling.md) — the message
  generator workflow and the broader test inventory.
