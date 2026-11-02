# Developer documentation

**Status:** draft — build, flash, bootloader and tooling notes for the Bus
Pirate v3/v4 community firmware.

This folder is developer writing, not a device-function reference: it names
repository files, make targets, scripts and bootloaders, which is its job.
The device functions themselves live under `docs/<function>/`; the capability
matrix is `docs/SPEC.md`.  `docs/dev/` is deliberately not part of that
capability matrix — `docs/SPEC.md` mentions it only as the location of the
build/flash tooling, not as device functions.

The governing documentation contract is CodeGuidelines
`project_guidelines/documentation_guidelines.md`; the repo shape is
`docs/TEMPLATE.md`.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [building.md](building.md) | toolchain prerequisites; the command-line `make` build for v3/v4; the MPLAB-X route; produced artifacts; message-string generation; native tests; common build errors |
| [flashing.md](flashing.md) | entering the bootloader on v3/v4; building and using `pirate-loader`; its options; the Pickit2 route; post-flash verification; drivers/permissions |
| [bootloaders.md](bootloaders.md) | what the bootloader folders contain; the v3/v4 revisions; when a bootloader update is needed instead of a firmware flash |
| [tooling.md](tooling.md) | the repository scripts and helpers (message strings, native tests, host tools); packaging steps |
| [shared-machinery.md](shared-machinery.md) | maintainer note on the shared mode-descriptor / decimal / parser helpers |
| [code-map.md](code-map.md) | maintainer code map for the user docs (file + symbol, no line numbers) |
| [avrisp-design-notes.md](avrisp-design-notes.md) | implementation design notes for the AVRISP-MKII personality |
| [verification.md](verification.md) | maintainer verification record (verification summaries per module) |

## The five-minute version

- **Toolchain:** Microchip **XC16** on `PATH` (or selected with `XC16_DIR`);
  the tracked `Firmware/Makefile` builds both boards without MPLAB-X.
- **Build v4:** `make -C Firmware BP_VERSION=V4` → `Firmware/bpv4.hex`.
- **Build v3:** `make -C Firmware BP_VERSION=V3` → `Firmware/bpv3.hex`.
- **Flash:** enter the bootloader (`$` then `y`, or the PGC/PGD jumper), close
  the terminal, then
  `pirate-loader --dev=COMx --hex=Firmware/bpv4.hex`.
- **Verify:** power-cycle, open the port **115200 8N1**, press Enter, type `i`.

## Shortest path: clean checkout to a flashed board

1. Install XC16 and put it on `PATH` (download from Microchip; see
   [building.md](building.md) §Prerequisites).
2. Build the image: `make -C Firmware BP_VERSION=V4` (or `V3`).
3. Build the loader: CMake in `Bootloaders/pirate-loader/`.
4. Put the board into its bootloader ([flashing.md](flashing.md) §Entering).
5. `pirate-loader --dev=<port> --hex=Firmware/bpv4.hex`.
6. Power-cycle and confirm with `i`.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| CLI `make` build | yes → `bpv3.hex` | yes → `bpv4.hex` |
| MPLAB-X project config | `BusPirate_v3` | `BusPirate_v4` |
| bootloader source | `Bootloaders/BPv3-bootloader/firmware-v4.5/` | `Bootloaders/BPv4-bootloader/firmware-v1/` |
| bootloader entry `$` | requires the v4.5 loader (the `$` flag) | present in the current loader |
| serial link | FTDI cable (`0403:6001`) | USB CDC (`04d8:fb00`) |

## Limits & out of scope

- **Not a device-function reference.**  Per-mode behaviour is in
  `docs/<function>/`; the capability matrix is `docs/SPEC.md`.
- **Hardware schematics** live in the separate hardware repository, not here.
- **Host driver installation** is not documented in this repository (see
  [flashing.md](flashing.md) §Drivers and permissions).
- The old `Documentation/building-and-flashing-firmware.md` and
  `Documentation/shared-firmware-machinery.md` pages are superseded by this
  folder.

## Related docs

- `docs/generic/README.md` — using the terminal prompt.
- `docs/avrisp/README.md` — the AVRISP-MKII personality.