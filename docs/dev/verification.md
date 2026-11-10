# Maintainer verification record

**Status:** maintainer material.  This is **never part of the user docs**
(user ruling: bench results are not documentation; user pages keep at most
short `unverified` caveats).  It records the basis of each documentation set,
so a later reviewer can tell what rests on hardware and what is source-only.

## Scope and labels

- **bench** — the set's behaviour was exercised on v4 hardware during the
  module round or a later follow-up; the note names what was covered.
- **source-only** — the claims come from the grouped source and the native
  suites; no hardware exercise is recorded for that set.
- **unverified** — the hardware path itself is unavailable (LCD adapter, SMPS
  board), so the set is code-derived and says so.

Two standing facts: the **v3 column of every set is build-only** (no v3
hardware was used), and a `bench` note covers only what it lists — the rest of
that set is still source-derived.  Sets not marked `bench` here were not
necessarily never exercised; this record simply does not claim it.

## Per-set basis

| Set | Basis | Note |
|---|---|---|
| [generic](../generic/README.md) | bench | v4 prompt layer (reset, binary/SUMP entry, macros, supplies); v3 source-only |
| [core](../core/README.md) | source-only | internals plus the native suites; architecture claims are structural |
| [wire/1wire](../protocol/wire/1wire/README.md) | bench | READ ROM round trip, overdrive toggle, reset/bit widths; macros, roster and binary source-only |
| [wire/2wire](../protocol/wire/2wire/README.md) | bench | smart-card ATR trigger/parse, `m6 2` partial form; binary path source-only |
| [wire/3wire](../protocol/wire/3wire/README.md) | bench | CS polarity, bulk-byte reply, clock idle; VPU/wiring values unverified |
| [spi](../protocol/spi/README.md) | source-only | terminal, sniffer and binary contracts from source |
| [i2c](../protocol/i2c/README.md) | bench | terminal legs, sniffer, tester matrix, onboard EEPROM path; scanner, hardware rates and binary source-only |
| [uart](../protocol/uart/README.md) | bench | HDUART, bridge, loopback, error flags; 115200 rig-limited, binary presets source-only |
| [jtag](../protocol/jtag/README.md) | bench | XSVF player live run; OpenOCD adapter source-only |
| [dio](../protocol/dio/README.md) | bench | read-back (`r` bits 4–0) and the `w` state; v3 unverified |
| [keyboard](../protocol/keyboard/README.md) | source-only | frame/parity/ACK and the timeout string from source; keyboard draw unverified |
| [binary](../protocol/binary/README.md) | source-only | byte layer and sub-protocol entry; v3 unverified |
| [sump](../protocol/sump/README.md) | source-only | entry, exit bytes, tiers and capacities; v3 large captures unverified |
| [tools/basic](../tools/basic/README.md) | source-only | interpreter limits and the unavailable commands from source |
| [tools/xsvf](../tools/xsvf/README.md) | bench | player run; 400-bit shifts and 4096-byte chunking unverified |
| [tools/openocd](../tools/openocd/README.md) | source-only | version matrix and shift contract from source; v3 build-only |
| [capabilities/adc](../capabilities/adc/README.md) | source-only | probe/continuous contracts; v3 divider unverified |
| [capabilities/auxiliary](../capabilities/auxiliary/README.md) | source-only | PWM/servo/frequency/edge-wait contracts; external wiring unverified |
| [capabilities/selftest](../capabilities/selftest/README.md) | source-only | check rows and the `0x10` column from source |
| [capabilities/smps](../capabilities/smps/README.md) | unverified | no SMPS board available; every electrical value is code-derived |
| [capabilities/lcd](../capabilities/lcd/README.md) | unverified | no adapter available; pin-out, glyphs and the SPI link are code-derived |
| [programmers/avrisp](../programmers/avrisp/README.md) | source-only | ISP/PDI/TPI and USB personality from source; TPI not verified |
| [programmers/pic](../programmers/pic/README.md) | source-only | ICSP bit engine pinned by `test_pic.py`; v3 unverified |

## Bench notes

- Generic layer: v4 build `e4b2239`, bench runs 2026-11-01; v3 source-only.
- 1-Wire: v4 + emulated slave (2026-10-27) — READ ROM round trip, `O` toggle
  17/17, bit/reset widths; macros (0)/85/204/236/240, roster, `1W01` binary
  and the failure strings source-only.
- I2C: v4 + emulated EEPROM-style slave (2026-10-26/27) — tester matrix
  32/32, terminal legs, sniffer, on-board EEPROM via the Windows tool;
  address scanner, hardware rates, binary `0x08` success path source-only.
- UART: v4 + peer rig (2026-10-31/11-01) — HDUART five runs 19/19
  (9600–57600 + 64-byte bulk), bridge 17/17, loopback, error flags; 115200
  rig-limited; binary presets source-only.
- 2-Wire: v4 + smart-card rig — ATR trigger and parse (4 bytes + 32 edges);
  `m6 2` partial form (interactive prompts, `Ready`/`2WIRE>`, clean exit);
  binary path source-only.
- 3-Wire: v4 — CS polarity, bulk-byte reply (`bitbang_read_with_write` plus
  transmit) and clock idle at mode entry; v3 unverified.
- DIO: v4 — `r` read-back bits 4–0, before/after state transcripts.
- JTAG / XSVF: v4 — a live player run reached `XSVF_ERROR_NONE`; the 400-bit
  shift and 4096-byte chunk limits are source-only.

## Developer-tool pages

The [building.md](building.md), [flashing.md](flashing.md),
[bootloaders.md](bootloaders.md), [tooling.md](tooling.md),
[shared-machinery.md](shared-machinery.md) and [code-map.md](code-map.md)
pages are source-derived (see the [README](README.md) for what that means).
No build, flash, bootloader write or test run was performed while writing
them; each page links here from its *Verification* section.

### Flash-error provenance

The earlier flashing note recorded that an
`Error updating firmware :(` message was still a successful v3 write when the
progress "passed page 41" ("if you see `Error updating firmware :(`, but it
progresses past page 41, this is actually a successful result").  That `41`
is a bench observation, not a constant in `pirate-loader.c`, so it is kept
here rather than in the how-to page; [flashing.md](flashing.md) carries only
the low-risk advice to power-cycle and check the version before retrying.

## Evidence

The bench logs are **not shipped** with this repository and are not required
to use it: they are retained by the maintainer in the development workspace,
outside the repository.  Every statement in this file can be re-derived from
the grouped source and the native suites in `Firmware/tests/`.
