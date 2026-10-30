# Shared Firmware Machinery

This is a standalone cross-mode consolidation on upstream/master 44574ce.
It does not import the combined bench image or its other feature branches.

## Scope

- `bp_current_protocol()` resolves the live mode descriptor for terminal and
  BASIC callbacks. `bp_protocol_by_index()` supports mode-menu enumeration.
  The descriptor table is private and exposed through const pointers, while
  retaining the existing compact initialized-data storage.
- `getint_with_spaces()` implements the existing consume-spaces/getint pair
  once for mode/tool setup. Parsing grammar, buffer position and errors are
  unchanged. Existing modes, callbacks, strings and feature flags are retained.

The changes adapt combined-pass commits d407820 and 54ae464. Context from
combined-only BASIC MODE/default setup, BASIC bit-order fixes and batched clock
pulses is deliberately NOT carried onto upstream. Those are separate changes;
the accessors work with the original upstream descriptor structure too.

## Decimal Sharing Dependency

The decimal hunk of original 54ae464 factors duplicate `print_decimal16`
implementations introduced by sibling PR #237 (`perf/base-number-math`,
e13e801). Upstream does not have that helper: its byte/word APIs already use
one generic `print_decimal` implementation. **Firmware/base.c is unchanged
in this standalone branch.** No #237 algorithm or other sibling fix is
silently pulled into this PR, and its additional 84-byte gain is not claimed.
Merely redirecting upstream byte output through its word API would run a
five-digit 32-bit division loop instead of the existing three-digit loop;
that is not used as a slower substitute for the missing #237 helper.

The adapted cherry-pick retains its original commit title/provenance, but
ports only the applicable token-sharing production changes and tests.
The descriptor/token changes can be reviewed directly against upstream now.
After #237 lands, the decimal-sharing delta can be a small follow-up. If the
entire original pair must be submitted together, explicitly stack this work
on #237 instead; do not fold this cross-mode scope into #237 itself.
When integrating separately added BASIC MODE or clock-batching callbacks,
migrate their added table accesses to these accessors as well. That is merge
integration work, not a reason to import those features into this branch.

## Building

The branch includes the existing canonical command-line Makefile and build
ignore rules from cb1fb95. Upstream lacked that Makefile; carrying these two
build-only files makes a fresh checkout self-contained, unlike relying on
an untracked local support file. No configuration or terminal-buffer changes
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

AVRISP sources, its native battery, v3 mkII UART simulator and mkII/CDC gate
are absent from this upstream-base branch and are not silently imported.
The PDI/TPI NVM-sharing commit is owned by the separate AVRISP-MKII PR.
These host tests do not emulate MCU timing or claim a new hardware bench of
upstream firmware. No board flashing or target operations are needed to run
this branch's native tests.
