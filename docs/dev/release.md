# Release assembly

**Status:** maintainer-facing.  There is **no packaging script for the
firmware or bootloaders**; a release is assembled by hand from clean
production builds and the tracked host frontend.

This page answers what ships, where each artifact comes from, and how the
independent versions relate.  Building the firmware is
[building.md](building.md); flashing it is [flashing.md](flashing.md).

## Scope

- **In:** the build products that make up a release; firmware, bootloader,
  host-frontend and hardware components; artifact names; version
  relationships.
- **Out:** signing, installers, distribution channels and a bill of materials
  for third-party tools.

## Version map

| Component | Version | Source of the version |
|---|---|---|
| Application firmware | **v9.0** | `BP_FIRMWARE_STRING` in `Firmware/core_system/base.h` |
| Bootloader | **5.0** | v3 `0xABFA`; v4 `0x1FF6` and hello |
| `pirate-loader` host flasher | **1.0.7** | `PIRATE_LOADER_VERSION` in `pirate-loader.c` |
| `bp` host frontend | **0.1.0** | `[project] version` in `scripts/bp/pyproject.toml` |

These are independent.  The application revision is v9.0 on both boards, the
bootloader revision is 5.0 on both boards, and the host tools have their own
versions.  The v3 `1,02` printed by `pirate-loader` is the ds30 hello protocol,
not the loader revision; see [README.md](README.md) §Version and name map.

## Release contents

| Component | Source | Build product |
|---|---|---|
| v3 firmware (make) | `Firmware/` with `BP_VERSION=V3` | `Firmware/bpv3.hex` |
| v4 firmware (make) | `Firmware/` with `BP_VERSION=V4` | `Firmware/bpv4.hex` |
| v3/v4 firmware (MPLAB-X) | `BusPirate_v3` / `BusPirate_v4` production build | `dist/<config>/production/…` (IDE path; see [building.md](building.md)) |
| v3 bootloader | `Bootloaders/v3/` | `Bootloaders/v3/build/bpv3-bootloader.hex` |
| v4 bootloader | `Bootloaders/v4/` | `Bootloaders/v4/build/bpv4-bootloader.hex` |
| Host frontend | `scripts/bp/` | source tree, or a wheel/sdist built from `pyproject.toml` |
| Hardware design files | `hardware/` | Eagle `.sch`/`.brd`, PNG previews, 3D models |

The firmware is an Intel HEX image; the bootloader images are separate Intel
HEX images.  They are not interchangeable: `pirate-loader` writes only the
application, while a bootloader write needs a hardware programmer.  See
[bootloaders.md](bootloaders.md) §Updating the bootloader.

## Firmware artifacts

The command-line build writes into `Firmware/`:

| Artifact | v4 | v3 |
|---|---|---|
| Intel HEX (ship this) | `Firmware/bpv4.hex` | `Firmware/bpv3.hex` |
| ELF | `Firmware/bpv4.elf` | `Firmware/bpv3.elf` |
| Linker map | `Firmware/bpv4.map` | `Firmware/bpv3.map` |
| Objects | `Firmware/build/bpv4/` | `Firmware/build/bpv3/` |

The MPLAB-X route writes
`Firmware/busPirate.X/dist/<config>/production/busPirate.X.production.hex`.
That directory name is the IDE convention and is **unverified** as a tracked
path; the command-line Makefile is the reproducible route.  See
[building.md](building.md) §MPLAB-X route.

Older release notes name the artifacts `bpv3-firmware.hex`,
`bpv3-bootloader.hex`, `bpv4-firmware.hex` and `bpv4-bootloader.hex`.  Those
names came from a manual clean-build-and-rename step, not a tracked script.
The current names are the Makefile outputs above.

## Host frontend

`scripts/bp/` is a separately versioned Python package:

```bash
cd scripts/bp
python -m pip install -e .
bp --help
```

A wheel can be built with the usual PEP 517 frontend, for example:

```bash
cd scripts/bp
python -m pip wheel . --no-deps -w dist
```

The package requires Python 3.12 or newer.  Its console entry point is `bp`;
installing from source is enough for a release that ships the source tree.
The host frontend version is independent of the firmware and bootloader
versions.  Its `README.md` records the command surface and the known
firmware-side quirks; check that file when pairing a newer frontend with an
older image.

A smoke check on a machine with a board:

```
bp ports
bp version --port <firmware port>
```

`bp ports` needs no device; `bp version` needs the firmware port at its
terminal prompt.  Neither is a substitute for the flashing check in
[flashing.md](flashing.md).

## Bootloader artifacts

```
make -C Bootloaders XC16=/path/to/xc16/bin/xc16-
```

Images and maps are `Bootloaders/v3/build/bpv3-bootloader.{hex,map}` and
`Bootloaders/v4/build/bpv4-bootloader.{hex,map}`.  The build treats compiler
warnings as errors.  No prebuilt loader binary is tracked; build from source.
The image tests check the reset/version/config words and the loader /
application windows.  See [bootloaders.md](bootloaders.md) §Build and test.

## Release procedure

1. Start from a clean checkout at the release commit.
2. Build the v3 and v4 firmware with the command-line Makefile (or a production
   MPLAB-X build) and keep the HEX, ELF and map.
3. Build both bootloaders with `make -C Bootloaders` and keep the HEX and map.
4. Run the native suites in [tests.md](tests.md) and the bootloader test tree.
5. Install the host frontend from `scripts/bp/` in a clean virtual
   environment; run `bp --help` and, if a board is present, `bp ports` and
   `bp version`.
6. Record the hashes of the shipped files if the release channel needs them.
   There is no in-tree signing or checksum manifest.
7. Assemble the release notes from the artifact names and the version map
   above.  Do not rename a HEX after the fact: the loader and the documented
   commands expect the Makefile names.

## Verification

The artifact names and paths are taken from the Makefile, the bootloader
Makefiles and the package metadata.  The MPLAB-X output directory is IDE
generated and is marked unverified.  The per-set documentation basis is in
[verification.md](verification.md).

## Limits & out of scope

- No signed images, installers or distribution channel are defined here.
- No scripted release assembly for the firmware or bootloaders; the steps above
  are manual on purpose.
- A release that ships only the HEX files omits the host frontend and the
  hardware sources; state the intended contents explicitly.
- Bootloader updates are not part of a routine application release; see
  [bootloaders.md](bootloaders.md) §Updating the bootloader.
