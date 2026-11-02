# Maintainer verification record

**Status:** maintainer material.  This is **never part of the user docs**
(user ruling: bench results are not documentation; user pages keep at most
short `unverified` caveats).  It records how the user-facing sets were
exercised, so a later reviewer can tell what rests on the bench and what is
source-only.

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

Evidence logs from the bench live in the development workspace (outside this
repository).  Per-topic `verification.md` in a user set remains removed
(changelog-like, no user value) — summaries belong here.

## Developer-tool pages

The [building.md](building.md), [flashing.md](flashing.md),
[bootloaders.md](bootloaders.md), [tooling.md](tooling.md) and
[shared-machinery.md](shared-machinery.md) pages are **source-derived**:
their claims come from the tracked Makefile, project files, bootloader
sources, loader sources and native test harnesses.  No build, flash,
bootloader write or test run was performed while writing them; each page links
here from its *Verification* section.

### Flash-error provenance

The old `Documentation/building-and-flashing-firmware.md` recorded that an
`Error updating firmware :(` message was still a successful v3 write when the
progress "passed page 41" ("if you see `Error updating firmware :(`, but it
progresses past page 41, this is actually a successful result").  That `41`
is a bench observation, not a constant in `pirate-loader.c`, so it is kept
here rather than in the how-to page; [flashing.md](flashing.md) carries only
the low-risk advice to power-cycle and check the version before retrying.