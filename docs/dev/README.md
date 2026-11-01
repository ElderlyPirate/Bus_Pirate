# Developer documentation

**Status:** stub — developer writing lives here (build, flash, bootloaders,
packaging, tooling).  Not part of the function spec.

- Source today: `Documentation/building-and-flashing-firmware.md`
  (migrate + fix: stale paths, v7.1 sample output, missing CLI build flow).
- Planned: `README.md` (build + flash), bootloader notes, tooling.
- See also [code-map.md](code-map.md) — maintainer code map for the user docs
  (file + symbol, no line numbers).
- SPEC: intentionally **not** listed in `docs/SPEC.md` (not device functions).

## Documentation verification

Maintainer-facing — **never part of the user docs** (user ruling 2026-11-01:
bench results are not documentation; user pages keep at most short
`unverified` caveats).

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
- Evidence logs from the bench live in the development workspace (outside
  this repository).  Per-topic `verification.md` in a user set remains
  removed (changelog-like, no user value) — summaries belong here.