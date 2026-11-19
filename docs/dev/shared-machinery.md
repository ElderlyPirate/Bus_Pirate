# Shared helper contract (maintainer note)

**Status:** maintainer summary.  This covers the shipped helpers that the
terminal dispatcher, the mode registry, BASIC, the family setup parsers and
the numeric output code share.  It does **not** cover the shared module layer
— USB, bit-bang, messages and EEPROM are
[docs/core/](../core/README.md).

The helpers are small enough to be reached directly, but their boundaries are
deliberate: the descriptor table is private, setup prompts share one parser
entry, and decimal output has one engine.  A change should go through the
contract below rather than through a copied implementation.

## Shared implementations

| Helper | Where | Role |
|---|---|---|
| `bp_current_protocol()` | `core_system/core.c` | Returns a `const` pointer to the live mode descriptor, indexed by `bus_pirate_configuration.bus_mode`.  The protocol table stays private; callers do not mutate the descriptor. |
| `bp_protocol_by_index()` | `core_system/core.c` | Returns a `const` pointer to the same table by position.  Used to enumerate the mode menu; callers keep the index below `ENABLED_PROTOCOLS_COUNT`. |
| `getint_with_spaces()` | `core_system/proc_menu.c` | Consumes leading spaces and calls `getint()`.  It is the parser entry for the family setup prompts (SPI, I2C, UART, wire, AUX, LCD and PIC).  During scripted setup defaults, `consumewhitechars()` is a no-op. |
| `bp_write_dec_word()` | `core_system/base.c` | The out-of-line 16-bit decimal engine; it keeps the literal-divisor algorithm rather than a generic 32-bit division loop.  `bp_write_dec_byte()` delegates to it, and `bp_write_dec_dword()` uses `print_decimal()` for 32-bit values. |

Boundary rules:

- The mode-descriptor table is private and exposed only through the two
  accessors above.
- Optional callbacks and BASIC bus access use the accessors rather than the
  table directly.
- A setup parser must return through `getint_with_spaces()` so whitespace,
  cursor, error and buffer mutation match the other modes.  A caller that
  validates its input checks `mode_configuration.command_error` after the
  call.
- Numeric output goes through `bp_write_formatted_integer()` when the selected
  width and display format apply; that function masks the value to
  `mode_configuration.numbits` before dispatching to the decimal, hexadecimal,
  binary or raw writer.
- No heap allocation, C++ runtime or optional production feature is involved
  in any of these helpers.

Symbol locations are in [code-map.md](code-map.md) (rows "Value entry &
parsing", "Mode registry & callbacks" and "Numeric formatting & delays").
This page records the contract, not the line numbers.

## Native checks

The contract is pinned by the hardware-free harnesses in `Firmware/tests/`
(`test_shared_format.py`, `test_protocol_access.py`, `test_messages.py`; see
[tooling.md](tooling.md) §Test suites).  They:

- check the live descriptor selection and mode/index pointer identity;
- check all byte/word decimal values, plus parser result/cursor/error/buffer
  equivalence, against the production functions;
- check packed-message output across byte lanes, table-page crossings and page
  restoration on both address models.

Generated test files stay under `Firmware/build/` or a temporary directory and
are **not** linked into the shipping image.  They do not emulate USB
enumeration, hardware interrupts or MCU-cycle timing, and they do not claim
electrical protocol conformance.

## Verification

The helpers above are compiled from the production sources named in the table;
the per-set documentation basis is in [verification.md](verification.md).

## Limits & out of scope

- This is not an architecture deep dive for the whole firmware; the shared
  module layer is [docs/core/](../core/README.md).  This page covers only the
  helpers above.
- Symbol-level location lives in [code-map.md](code-map.md).
