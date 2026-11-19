# Maintainer verification record

**Status:** maintainer material.  This is **never part of the user docs** (user
pages keep at most short `unverified` caveats).  It records the basis of each
documentation set so a later reviewer can tell what rests on a hardware
history, what is source-derived, and what is explicitly unverified.

## Scope and labels

| Label | Meaning |
|---|---|
| `source-only` | The claims come from the grouped source and the native fixtures; no hardware basis is recorded for the set. |
| `source + v4 history` | The v4 behaviour had a hardware basis at some point; the rest of the set is source-derived.  No dates or pass counts are recorded here. |
| `source + partial history` | Only part of the set (named in the note) has a hardware basis; the remainder is source-derived. |
| `unverified` | The hardware path itself is unavailable or not covered, so the set is code-derived and says so. |

Two standing facts:

- **v3 hardware is unverified throughout.**  No v3 hardware basis is claimed
  by any set.
- A label covers only what its note lists; the rest of that set is still
  source-derived.  A set not marked with a history label is not thereby
  claimed never to have been used — this record simply does not claim it.

This is a documentation-basis record, not an electrical-conformance report and
not a test report.  Native fixtures are named in [tooling.md](tooling.md)
§Test suites.

## Per-set basis

| Set | Basis | Note |
|---|---|---|
| [generic](../generic/README.md) | source + v4 history | v4 prompt layer (reset, binary/SUMP entry, macros, supplies); v3 build-only |
| [core](../core/README.md) | source-only | internals plus the native fixtures; architecture claims are structural |
| [wire/1wire](../protocol/wire/1wire/README.md) | source + v4 history | READ ROM round trip, overdrive toggle, reset/bit widths; macros, roster and binary source-only |
| [wire/2wire](../protocol/wire/2wire/README.md) | source + v4 history | smart-card ATR trigger/parse, `m6 2` partial form; binary path source-only |
| [wire/3wire](../protocol/wire/3wire/README.md) | source + v4 history | CS polarity, bulk-byte reply, clock idle; VPU/wiring values unverified |
| [spi](../protocol/spi/README.md) | source-only | terminal, sniffer and binary contracts from source |
| [i2c](../protocol/i2c/README.md) | source + v4 history | terminal legs, sniffer, tester matrix, onboard EEPROM path; scanner, hardware rates and binary source-only |
| [uart](../protocol/uart/README.md) | source + v4 history | HDUART, bridge, loopback, error flags; the receive path is a **127-usable-word ring** (128 slots, one held free) and the terminal output buffer is **4096 bytes** on both builds; binary presets source-only |
| [jtag](../protocol/jtag/README.md) | source + v4 history | XSVF player history; OpenOCD adapter source-only; host-tool caveats in the XSVF set |
| [dio](../protocol/dio/README.md) | source + v4 history | read-back (`r` bits 4–0); the safe-park contract covers five pins on v3 and seven on v4 (`AUX1`/`AUX2` included); v3 unverified |
| [keyboard](../protocol/keyboard/README.md) | source-only | frame/parity/ACK from source; `TIMEOUT` is a line message; host-write timing and pull-ups unverified |
| [binary](../protocol/binary/README.md) | source-only | byte layer and sub-protocol entry; v3 unverified |
| [sump](../protocol/sump/README.md) | source-only | entry, exit bytes, tiers and capacities; the v3 sample buffer is **4096 bytes**; v3 large captures unverified |
| [tools/basic](../tools/basic/README.md) | source-only | interpreter limits from source; `FREQ`/`DUTY` are usable, `DEBUG` is off in the default image and a build option, the EEPROM filesystem is disabled |
| [tools/xsvf](../tools/xsvf/README.md) | source + v4 history | firmware player history; the 400-bit shift, 4096-byte chunk limits, bounded chain scan and standalone `-x`/`-r` behaviour are source-only; the exit-status difference and host-tool caveats (`-x`/`-f`, player exit) are in that set |
| [tools/openocd](../tools/openocd/README.md) | source-only | version matrix and shift contract from source; v3 build-only |
| [capabilities/adc](../capabilities/adc/README.md) | source-only | probe/continuous contracts; SMPS ownership refusal is the gate; v3 build-only |
| [capabilities/auxiliary](../capabilities/auxiliary/README.md) | source-only | PWM/servo/frequency/edge-wait contracts; external wiring unverified |
| [capabilities/selftest](../capabilities/selftest/README.md) | source-only | check rows and the `0x10` column from source |
| [capabilities/smps](../capabilities/smps/README.md) | unverified | board unavailable for an electrical check; connector/pinout and jumper from the legacy schematic/BOM; input/output ratings undocumented; firmware behaviour code-derived |
| [capabilities/lcd](../capabilities/lcd/README.md) | unverified | no adapter available for an electrical check; board revisions (v2/v2b/v3/v3a, v4/v4b), connector and supply from the legacy design files in `hardware-other`; on-glass glyphs and timing code-derived |
| [programmers/avrisp](../programmers/avrisp/README.md) | source + partial history | ISP/PDI had a hardware basis; TPI and the remainder are source-only; see [avrisp-design-notes.md](avrisp-design-notes.md) |
| [programmers/pic](../programmers/pic/README.md) | source-only | ICSP bit engine pinned by the native ICSP fixture; the CO-8a state reset and CO-8b display-width checks are source/native; v3 unverified |

## Developer pages

| Page | Basis |
|---|---|
| [building.md](building.md) | Makefile, `configuration.h`, MPLAB-X project, `tools/packstrings/` and `Firmware/tests/`; the v4 all-features link result is recorded as a build result |
| [flashing.md](flashing.md) | `proc_menu.c` `$` path, the two bootloader sources and `pirate-loader`; the `pk2cmd` switch set is unverified |
| [bootloaders.md](bootloaders.md) | the ds30 and USB bootloader sources; the upgrader route is source-derived and `unverified` |
| [tooling.md](tooling.md) | directory inventory and the current `Firmware/tests/test_*.py` set |
| [shared-machinery.md](shared-machinery.md) | the shipped core helpers named in its table |
| [code-map.md](code-map.md) | file + symbol locations; no line numbers |
| [avrisp-design-notes.md](avrisp-design-notes.md) | the AVRISP sources and the user set; AVR053 and the custom-build omission are recorded there |

## Limits

- The record does not replace an electrical check, a target-specific protocol
  conformance test, or the host-tool caveats owned by the user sets.
- No pass counts, run dates, logs or capture provenance are kept here.
  The source, the native fixtures and the user sets' `unverified` caveats are
  the reproducible basis.
- `unverified` is a statement about coverage, not a defect report.
