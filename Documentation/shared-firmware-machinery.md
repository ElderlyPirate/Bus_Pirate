# Shared Firmware Machinery

**Requires #237 (perf/base-number-math) to be merged first.**

This cross-mode consolidation is explicitly chained on that numeric-math
prerequisite. The prerequisite is present in the branch history so both
supported board builds and native tests can be verified before publication.
After #237 merges, rebase onto the updated upstream before reviewing the
remaining changes against the main branch.

## Shared Implementations

- `bp_current_protocol()` resolves the live mode descriptor for terminal and
  BASIC callbacks. `bp_protocol_by_index()` supports mode-menu enumeration.
  The table is private, exposed through const pointers, and retains compact
  initialized-data storage instead of more expensive PSV storage.
- `getint_with_spaces()` implements the existing consume-spaces/getint pair
  once for mode/tool setup. Grammar, cursor/error behavior, buffer mutations
  and existing feature flags are unchanged.
- `bp_write_dec_word()` is the one out-of-line 16-bit decimal engine; byte
  output delegates to it. This shares #237's literal-division algorithm
  rather than duplicating its inline body. It does not replace the fast
  16-bit algorithm with a generic 32-bit division loop.

Existing descriptor contents, protocol callbacks, pin/clock loops, modes,
strings and numeric output are retained. Optional callbacks introduced by
other changes must use the same accessors rather than expose the private
table. No C++ runtime, heap allocation or feature removal is required.

## Building

The tracked command-line Makefile mirrors the XC16 project settings and
keeps board objects separate. XC16 v2.10 is required, either on PATH or
selected with `XC16_DIR`.

```sh
make -C Firmware BP_VERSION=V4
make -C Firmware BP_VERSION=V3
python Firmware/tests/test_native.py
```

Windows installations can select the shell and compiler explicitly:

```powershell
make -C Firmware BP_VERSION=V4 'SHELL=C:/Program Files/Git/usr/bin/sh.exe' 'XC16_DIR=C:/Program Files (x86)/Microchip/MPLABXC16/v2.10'
make -C Firmware BP_VERSION=V3 'SHELL=C:/Program Files/Git/usr/bin/sh.exe' 'XC16_DIR=C:/Program Files (x86)/Microchip/MPLABXC16/v2.10'
```

Use the same optimization level and feature flags for size comparisons.
The decimal-sharing commit must be compared with the #237 prerequisite
already applied, not with a different numeric implementation.

## Native Checks

Python 3 and host GCC compile actual production functions with small mocks.
The battery checks live descriptor selection, all byte/word decimal values,
parser result/cursor/error/buffer equivalence, and packed-message output,
byte lanes, table-page crossings and page restoration.

The formatter fixture supports both the prerequisite's inline helper and
the shared implementation, allowing exact before/after output checks.
Scripted setup defaults are tested only when that feature exists; tests
do not introduce optional production features to satisfy a fixture.

Generated tests stay under `Firmware/build` and are not linked into the
shipping image. They do not emulate USB enumeration, hardware interrupts
or MCU-cycle timing, and do not claim electrical protocol conformance.
