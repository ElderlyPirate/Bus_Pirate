# Developer documentation

**Status:** maintainer-facing — build, flash, bootloader, host tooling, test and
release material for the Bus Pirate v3/v4 community firmware.

This folder is **maintainer-facing**.  It names repository files, make targets,
scripts, bootloaders, host commands and symbols, which is its job.  The device
functions live in four groups under `docs/`: `protocol/`, `tools/`,
`capabilities/` and `programmers/`; their index is [docs/SPEC.md](../SPEC.md).
The firmware internals are [docs/core/](../core/README.md).  `docs/dev/` is not
part of that capability matrix — it covers how the image is built, written,
inspected and released.

Content is derived from the tracked sources where the sources can answer.
Where a statement depends on a board, adapter or tool the source cannot answer,
the page says `unverified`; [verification.md](verification.md) records the basis
of each user set and each developer page.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [building.md](building.md) | toolchain; `make` builds; feature sets and a custom build; MPLAB-X; artifacts; strings; build errors |
| [flashing.md](flashing.md) | bootloader entry; port; `pirate-loader`; Pickit2; verification; drivers |
| [bootloaders.md](bootloaders.md) | v3 ds30 and v4 USB; entry rules; host frames and a worked exchange; updates |
| [tooling.md](tooling.md) | generators; `bp` host frontend; removed legacy tools |
| [tests.md](tests.md) | native-test inventory for `Firmware/tests/` |
| [release.md](release.md) | release contents; firmware, bootloader and host frontend; artifacts and versions |
| [shared-machinery.md](shared-machinery.md) | descriptor accessors, parser and decimal helpers |
| [code-map.md](code-map.md) | maintainer code map (files + symbols) |
| [avrisp-design-notes.md](avrisp-design-notes.md) | AVRISP-MKII design notes, AVR053, custom-build omission |
| [verification.md](verification.md) | documentation basis and `unverified` areas |

## Version and name map

The firmware's displayed strings and the repository files use several
independent revisions.  They are not interchangeable.

| Item | Name / value | Where it appears |
|---|---|---|
| Application | **v9.0**, banner `ElderlyPirate Firmware v9.0` | `i`, banner |
| Bootloader | **5.0** on v3 and v4 | stored version word; v4 hello |
| v3 ds30 hello protocol | **1.0.2** | `pirate-loader` `Bootloader version: 1,02` |
| Host flasher | `pirate-loader` **1.0.7** | loader banner/help |
| Host frontend | package `bus-pirate-host`, command `bp`, version **0.1.0** | `scripts/bp/` |

The v3 stored loader version is **5.0**; the `1,02` that the host prints after
the device ID is the ds30 hello protocol, not the loader revision.  The v4
hello is `F1 05 00 4B`: device ID `F1` (241), loader version `05 00` (5.0), `K`.

## The five-minute version

- **Toolchain:** Microchip **XC16** from the auto-detected installer default
  or `PATH` (`XC16_DIR` overrides a non-default root); the tracked
  `Firmware/Makefile` builds both boards without MPLAB-X.  On Windows, put
  Git's `usr\bin` on `PATH` for the Makefile's shell tools.
- **Build v4:** `make -C Firmware BP_VERSION=V4` → `Firmware/bpv4.hex`.
- **Build v3:** `make -C Firmware BP_VERSION=V3` → `Firmware/bpv3.hex`.
- **Flash:** enter the bootloader (`$` then `y`; v3 also accepts the PGC/PGD
  jumper, v4 the NORMAL button), close the terminal, then run
  `pirate-loader` with `Firmware/bpv3.hex` or `Firmware/bpv4.hex`; the exact
  order and the v4 port rule are in [flashing.md](flashing.md).
- **Host work:** install the frontend from `scripts/bp/`, then `bp ports`,
  `bp version` and the mode subcommands; see [tooling.md](tooling.md).
- **Verify:** power-cycle, open the port **115200 8N1**, press Enter, type `i`.
  On v4 the firmware port may differ from the bootloader port.

## Shortest path: clean checkout to a flashed board

1. Install XC16 — the installer default is auto-detected, and a `PATH`
   installation also works (download from Microchip; see
   [building.md](building.md) §Prerequisites).  On Windows, put Git's
   `usr\bin` on `PATH` — see
   [building.md §Command-line build](building.md#command-line-build).
2. Build the image: `make -C Firmware BP_VERSION=V4` (or `V3`).
3. Build the loader (Unix; on Windows use the MinGW Makefiles or Visual
   Studio route — see
   [flashing.md §Building pirate-loader](flashing.md#building-pirate-loader)):

   ```
   cd Bootloaders/pirate-loader
   mkdir build && cd build
   cmake .. && cmake --build .
   ```

   Details and options:
   [flashing.md §Building pirate-loader](flashing.md#building-pirate-loader).
4. Put the board into its bootloader — see
   [flashing.md §Entering the bootloader](flashing.md#entering-the-bootloader).
5. Find the port: on v4 the bootloader enumerates as a new device, so use
   `pirate-loader --dev=<port> --hello` to confirm — see
   [flashing.md §Find the port after entry](flashing.md#find-the-port-after-entry).
   On v4 `--hello` sends the restart byte and leaves the bootloader; re-enter it
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
| bootloader source | `Bootloaders/v3/` | `Bootloaders/v4/` |
| bootloader entry `$` | requires the 5.0 loader | present in the 5.0 loader |
| serial link | FTDI cable (`0403:6001`) | USB CDC (`04D8:FB00`; bootloader `04D8:FAFF`) |
| all-features custom build | no — the SMPS guard stops the build | yes — LCD and SPI link together |
| host frontend `bp` | yes (host side) | yes (host side) |

## Limits & out of scope

- **Not a device-function reference.**  Per-mode behaviour is in the
  `protocol/`, `tools/`, `capabilities/` and `programmers/` sets; the
  capability matrix is [docs/SPEC.md](../SPEC.md).
- **Board design files** are in [`hardware/`](../../hardware/README.md)
  (Eagle schematics/boards and previews).  The LCD and SMPS pages cite them
  only where the board revision, connector or supply path matters.
- **Host driver installation** is not documented in this repository (see
  [flashing.md §Drivers and permissions](flashing.md#drivers-and-permissions)).
- **v3 hardware is unverified.**  v3 builds are supported; the v3 hardware
  behaviour in this set is source-derived and not verified on v3 hardware.
- **Third-party host programs** (avrdude, OpenOCD, picprog, sigrok/PulseView,
  flashrom) are covered with the function they serve, not here.

## Related docs

- [docs/generic/README.md](../generic/README.md) — using the terminal prompt.
- [docs/core/README.md](../core/README.md) — firmware architecture and the
  module map.
- [docs/SPEC.md](../SPEC.md) — function and capability index.
- [docs/programmers/avrisp/README.md](../programmers/avrisp/README.md) — the
  AVRISP-MKII personality.
- [docs/protocol/jtag/xsvf.md](../protocol/jtag/xsvf.md) — the firmware XSVF
  player; the host side is `bp xsvf` in [tooling.md](tooling.md).
