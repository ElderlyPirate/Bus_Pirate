# Shared helper contract (maintainer note)

**Status:** maintainer summary.  This consolidates the durable helper contract
from the earlier maintainer page.  It covers four helpers, not the shared
module layer — USB, bit-bang, messages and EEPROM are
[docs/core/](../core/README.md).

This page describes the small set of helpers shared by the terminal
dispatcher, the mode registry, BASIC and the numeric output code, so a future
change touches them through their accessors rather than the private state.

## Shared implementations

| Helper | Where | Role |
|---|---|---|
| `bp_current_protocol()` | `core_system/core.c` | Resolves the live mode descriptor for terminal and BASIC callbacks.  The protocol table stays private; callers get a `const` descriptor pointer. |
| `bp_protocol_by_index()` | `core_system/core.c` | Same table, indexed — used for mode-menu enumeration. |
| `getint_with_spaces()` | `core_system/proc_menu.c` | The single consume-spaces/getint pair used by mode and tool setup prompts, so grammar, cursor/error behavior and buffer mutation are identical everywhere. |
| `bp_write_dec_word()` | `core_system/base.c` | The one out-of-line 16-bit decimal engine; byte output delegates to it (`bp_write_dec_byte()`).  It keeps the fast literal-division algorithm rather than a generic 32-bit division loop. |

Boundary rules:

- The mode-descriptor table is private and exposed only through the two
  accessors above.
- Optional callbacks introduced by later features must use the same accessors
  rather than reach for the table directly.
- Existing descriptor contents, protocol callbacks, pin/clock loops, modes,
  strings and numeric output are unchanged by this sharing; no C++ runtime,
  heap allocation or feature removal is involved.

Symbol locations are in [code-map.md](code-map.md) (rows "Value entry &
parsing", "Mode registry & callbacks" and "Numeric formatting & delays").
This page records the contract, not the line numbers.

## Native checks

The sharing is pinned by the hardware-free harnesses in `Firmware/tests/`
(`test_shared_format.py`, `test_protocol_access.py`, `test_messages.py`; see
[building.md](building.md) §Native tests and the suite table in
[tooling.md](tooling.md)).  They:

- check the live descriptor selection and mode/index pointer identity;
- check all byte/word decimal values, plus parser result/cursor/error/buffer
  equivalence, against the production functions;
- check packed-message output across byte lanes, table-page crossings and page
  restoration on both address models.

The formatter fixture supports the inline helper and the shared
implementation, so before/after output can be compared.  Scripted setup
defaults are tested only when that feature is present — tests do not introduce
optional production features just to satisfy a fixture.

Generated test files stay under `Firmware/build/` and are **not** linked into
the shipping image.  They do not emulate USB enumeration, hardware interrupts
or MCU-cycle timing, and they do not claim electrical protocol conformance.

## Verification

- Not run for this page; see [verification.md](verification.md) for the
  maintainer record.

## Limits & out of scope

- This is not an architecture deep dive for the whole firmware; the shared
  module layer is [docs/core/](../core/README.md).  This page covers only the
  helpers above.
- Symbol-level location lives in [code-map.md](code-map.md).
