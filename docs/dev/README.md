# Developer documentation

**Status:** maintainer-facing — build, flash, bootloader, tooling, code-map and
verification material for the Bus Pirate v3/v4 community firmware.

This folder is **maintainer-facing**.  It names repository files, make targets,
scripts, bootloaders and symbols, which is its job.  The device functions live
in four groups under `docs/`: `protocol/`, `tools/`, `capabilities/` and
`programmers/`; their index is [docs/SPEC.md](../SPEC.md).  The firmware
internals are [docs/core/](../core/README.md).  `docs/dev/` is not part of that
capability matrix — it covers how the image is built, written, inspected and
mapped.

Everything here is derived from the tracked sources and scripts.  Where a
statement depends on a board or adapter that the source cannot answer, the
page says `unverified`; [verification.md](verification.md) records the basis of
each user set.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [building.md](building.md) | toolchain prerequisites; the command-line `make` build for v3/v4; feature sets and a worked custom build; the v4 all-features link result; the MPLAB-X route; produced artifacts; message-string generation; native tests; common build errors |
| [flashing.md](flashing.md) | entering the bootloader on v3/v4; finding the port; building and using `pirate-loader`; its options and the v3 speed trap; the Pickit2 route; post-flash verification; drivers and permissions |
| [bootloaders.md](bootloaders.md) | what the bootloader folders contain; the v3 ds30 and v4 USB revisions; the reset-time entry rules; the host frames and status bytes; when and how to update a bootloader |
| [tooling.md](tooling.md) | the repository scripts and host tools; the authoritative native-test suite table; the XSVF host-tool caveats; release assembly |
| [shared-machinery.md](shared-machinery.md) | maintainer contract for the shipped descriptor accessors, parser helper and decimal output helpers |
| [code-map.md](code-map.md) | maintainer cross-group code map (files + symbols; user pages carry none) |
| [avrisp-design-notes.md](avrisp-design-notes.md) | implementation design notes for the AVRISP-MKII personality, including AVR053 and the custom-build omission |
| [verification.md](verification.md) | per-set documentation basis and the areas that remain `unverified` |

## The five-minute version

- **Toolchain:** Microchip **XC16** on `PATH` (or selected with `XC16_DIR`);
  the tracked `Firmware/Makefile` builds both boards without MPLAB-X.
- **Build v4:** `make -C Firmware BP_VERSION=V4` → `Firmware/bpv4.hex`.
- **Build v3:** `make -C Firmware BP_VERSION=V3` → `Firmware/bpv3.hex`.
- **Flash:** enter the bootloader (`$` then `y`; v3 also accepts the PGC/PGD
  jumper, v4 the NORMAL button), close the terminal, then run
  `pirate-loader` with `Firmware/bpv3.hex` or `Firmware/bpv4.hex`; the exact
  order and the v4 port rule are in [flashing.md](flashing.md).
- **Verify:** power-cycle, open the port **115200 8N1**, press Enter, type `i`.
  On v4 the firmware port may differ from the bootloader port.

## Shortest path: clean checkout to a flashed board

1. Install XC16 and put it on `PATH` (download from Microchip; see
   [building.md](building.md) §Prerequisites).  On Windows, put Git's
   `usr\bin` on `PATH` and pass `XC16_DIR` explicitly — see
   [building.md §Command-line build](building.md#command-line-build).
2. Build the image: `make -C Firmware BP_VERSION=V4` (or `V3`).
3. Build the loader (Unix; on Windows the Visual Studio route is unverified —
   see [flashing.md §Building pirate-loader](flashing.md#building-pirate-loader)):

   ```
   cd Bootloaders/pirate-loader
   mkdir build && cd build
   cmake .. && make
   ```

   Details and options:
   [flashing.md §Building pirate-loader](flashing.md#building-pirate-loader).
4. Put the board into its bootloader — see
   [flashing.md §Entering the bootloader](flashing.md#entering-the-bootloader).
5. Find the port: on v4 the bootloader enumerates as a new device, so use
   `pirate-loader --dev=<port> --hello` to confirm — see
   [flashing.md §Find the port](flashing.md#find-the-port-after-entry).  On
   v4 `--hello` sends the restart byte and leaves the bootloader; re-enter it
   before step 6 if you used `--hello`.
6. Flash: `pirate-loader --dev=<port> --hex=Firmware/bpv4.hex` (use
   `Firmware/bpv3.hex` on v3).  Run it directly after entering the bootloader;
   do not put `--hello` between the entry and this step.
7. Power-cycle and confirm with `i`.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| CLI `make` build | yes → `bpv3.hex` | yes → `bpv4.hex` |
| MPLAB-X project configuration | `BusPirate_v3` | `BusPirate_v4` |
| bootloader source | `Bootloaders/BPv3-bootloader/firmware-v4.5/` | `Bootloaders/BPv4-bootloader/firmware-v1/` |
| bootloader entry `$` | requires the v4.5 loader (the `$` flag) | present in the current loader |
| serial link | FTDI cable (`0403:6001`) | USB CDC (`04d8:fb00`; bootloader `04d8:FAFF`) |
| all-features custom build | no — the SMPS guard stops the build | yes — LCD and SPI link together |

## Limits & out of scope

- **Not a device-function reference.**  Per-mode behaviour is in the
  `protocol/`, `tools/`, `capabilities/` and `programmers/` sets; the
  capability matrix is [docs/SPEC.md](../SPEC.md).
- **Hardware schematics** live in the separate hardware repository, not here.
  The LCD and SMPS pages cite the legacy design files only where the board
  revision, connector or supply path matters.
- **Host driver installation** is not documented in this repository (see
  [flashing.md §Drivers and permissions](flashing.md#drivers-and-permissions)).
- **v3 hardware is unverified.**  v3 builds are supported; the v3 hardware
  behaviour in this set is source-derived and not verified on hardware.

## Related docs

- [docs/generic/README.md](../generic/README.md) — using the terminal prompt.
- [docs/core/README.md](../core/README.md) — firmware architecture and the
  module map.
- [docs/SPEC.md](../SPEC.md) — function and capability index.
- [docs/programmers/avrisp/README.md](../programmers/avrisp/README.md) — the
  AVRISP-MKII personality.
- [docs/tools/xsvf/README.md](../tools/xsvf/README.md) — the XSVF host player
  set.
