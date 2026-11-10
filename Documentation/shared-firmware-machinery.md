# Shared Firmware Machinery

The cross-mode consolidation of the live protocol descriptors, the
spaced command-token parse, and the 16-bit decimal output. It is stacked on
the **core starter** (#209, base `e94e0f0`); the accessors, the shared token
wrapper and the decimal engine it builds on are carried by that core starter,
so this note and the remaining `getint_with_spaces()` caller cleanup ride the
`perf` family fold on top of it.

## Scope

- `bp_current_protocol()` resolves the live mode descriptor for terminal and
  BASIC callbacks. `bp_protocol_by_index()` supports mode-menu enumeration.
  The descriptor table is private and exposed through const pointers, while
  retaining the existing compact initialized-data storage.
- `getint_with_spaces()` implements the existing consume-spaces/getint pair
  once for mode/tool setup. Parsing grammar, buffer position and errors are
  unchanged. Existing modes, callbacks, strings and feature flags are retained.
- The byte/word decimal displays share one 16-bit formatter
  (`bp_write_dec_word()`; `bp_write_dec_byte()` delegates) whose literal
  `/10` and `%10` fold to multiply+shift on the PIC24 (no hardware divide,
  DS70000157G). Only the rare 32-bit paths keep the runtime division helper.

The descriptor and decimal bodies, plus the `getint_with_spaces()` wrapper,
are absorbed by the core starter (#209). What remains here is the caller
cleanup that replaces the ten mode modules' `consumewhitechars(); getint();`
pairs with the shared wrapper. Context from combined-only BASIC MODE/default
setup, BASIC bit-order fixes and batched clock pulses is deliberately NOT
carried.

## Building

The core starter does not track the command-line Makefile; that build-only
file is the `#203` CLI-Makefile work and is kept as an untracked local support
file here (the house pattern for a family worktree), adapted to the core
source layout (`core_system/`, `core_bitbang/`, `core_messages/`,
`core_eeprom/`, `core_usb/`). No configuration or terminal-buffer changes
from the later build branch are included.

```sh
make -C Firmware BP_VERSION=V4
make -C Firmware BP_VERSION=V3
python Firmware/tests/test_native.py
```

XC16 v2.10 must be installed or selected with `XC16_DIR`. On this Windows
host the canonical equivalent is:

```powershell
make -C Firmware BP_VERSION=V4 'SHELL=C:/Program Files/Git/usr/bin/sh.exe' 'XC16_DIR=C:/Program Files (x86)/Microchip/MPLABXC16/v2.10'
make -C Firmware BP_VERSION=V3 'SHELL=C:/Program Files/Git/usr/bin/sh.exe' 'XC16_DIR=C:/Program Files (x86)/Microchip/MPLABXC16/v2.10'
```

Keep the optimization level and feature flags unchanged for size comparisons.

## Tests And Limits

The native battery compiles actual production functions with host GCC and
small mocks; generated files stay in Firmware/build. It checks live descriptor
selection, decimal output exhaustively, parser result/cursor/error/buffer
equivalence, and packed-message output/page handling. It detects whether
scripted setup defaults actually exist rather than importing that feature
just to satisfy a combined-tree fixture.
It requires Python 3 and a host GCC in addition to XC16 for the ROM builds.

These host tests do not emulate MCU timing or claim a new hardware bench of
upstream firmware. No board flashing or target operations are needed to run
this branch's native tests.