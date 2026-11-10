# Developer documentation

**Status:** maintainer-facing — build, flash, bootloader and tooling notes for the Bus
Pirate v3/v4 community firmware.

This folder is **maintainer-facing**: build, flash, bootloader, tooling and
code-map material for people changing the firmware.  It names repository
files, make targets, scripts and bootloaders, which is its job.  Everything
here is **derived from the source** unless the verification record
([verification.md](verification.md)) says a set was exercised on hardware;
none of the commands here were run to write these pages.

The device functions live in four groups under `docs/`: `protocol/` (SPI, I2C,
UART, the three wire modes, JTAG, DIO, keyboard, binary, SUMP), `tools/`
(BASIC, XSVF, OpenOCD), `capabilities/` (ADC, AUX, self-test, SMPS, LCD) and
`programmers/` (AVRISP-MKII, PIC).  Their index is [docs/SPEC.md](../SPEC.md);
the firmware internals are [docs/core/](../core/README.md).  `docs/dev/` is
deliberately not part of that capability matrix — the SPEC mentions it only as
the location of the build/flash tooling, not as device functions.

The page structure and presentation rules every set follows are in
[docs/TEMPLATE.md](../TEMPLATE.md).

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [building.md](building.md) | toolchain prerequisites; the command-line `make` build for v3/v4; feature sets and a worked custom build; the MPLAB-X route; produced artifacts; message-string generation; native tests; common build errors |
| [flashing.md](flashing.md) | entering the bootloader on v3/v4; finding the port; building and using `pirate-loader`; its options and the v3 speed trap; the Pickit2 route; post-flash verification; drivers/permissions |
| [bootloaders.md](bootloaders.md) | what the bootloader folders contain; the v3/v4 revisions and entry rules; the host protocol status bytes; when and how to update a bootloader |
| [tooling.md](tooling.md) | the repository scripts and host tools; the authoritative native-test suite table; release assembly |
| [shared-machinery.md](shared-machinery.md) | maintainer note on the shared mode-descriptor / decimal / parser helpers |
| [code-map.md](code-map.md) | maintainer cross-group code map (files + symbols; user pages carry none) |
| [avrisp-design-notes.md](avrisp-design-notes.md) | implementation design notes for the AVRISP-MKII personality |
| [verification.md](verification.md) | maintainer verification record (per-set basis and bench notes) |

## The five-minute version

- **Toolchain:** Microchip **XC16** on `PATH` (or selected with `XC16_DIR`);
  the tracked `Firmware/Makefile` builds both boards without MPLAB-X.
- **Build v4:** `make -C Firmware BP_VERSION=V4` → `Firmware/bpv4.hex`.
- **Build v3:** `make -C Firmware BP_VERSION=V3` → `Firmware/bpv3.hex`.
- **Flash:** enter the bootloader (`$` then `y`; v3 also accepts the PGC/PGD
  jumper, v4 the NORMAL button), close the terminal, then
  `pirate-loader --dev=<bootloader port> --hex=Firmware/bpv4.hex`.
- **Verify:** power-cycle, open the port **115200 8N1**, press Enter, type `i`.

## Shortest path: clean checkout to a flashed board

1. Install XC16 and put it on `PATH` (download from Microchip; see
   [building.md](building.md) §Prerequisites).
2. Build the image: `make -C Firmware BP_VERSION=V4` (or `V3`).
3. Build the loader (Unix; on Windows run `cmake ..` and open the generated
   `pirate-loader.sln`):

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
   [flashing.md §Find the port](flashing.md#find-the-port-after-entry).
6. `pirate-loader --dev=<port> --hex=Firmware/bpv4.hex`.
7. Power-cycle and confirm with `i`.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| CLI `make` build | yes → `bpv3.hex` | yes → `bpv4.hex` |
| MPLAB-X project config | `BusPirate_v3` | `BusPirate_v4` |
| bootloader source | `Bootloaders/BPv3-bootloader/firmware-v4.5/` | `Bootloaders/BPv4-bootloader/firmware-v1/` |
| bootloader entry `$` | requires the v4.5 loader (the `$` flag) | present in the current loader |
| serial link | FTDI cable (`0403:6001`) | USB CDC (`04d8:fb00`) |

## Limits & out of scope

- **Not a device-function reference.**  Per-mode behaviour is in the
  `protocol/`, `tools/`, `capabilities/` and `programmers/` sets; the
  capability matrix is [docs/SPEC.md](../SPEC.md).
- **Hardware schematics** live in the separate hardware repository, not here.
- **Host driver installation** is not documented in this repository (see
  [flashing.md §Drivers and permissions](flashing.md#drivers-and-permissions)).

## Related docs

- [docs/generic/README.md](../generic/README.md) — using the terminal prompt.
- [docs/core/README.md](../core/README.md) — firmware architecture and the
  module map.
- [docs/SPEC.md](../SPEC.md) — function and capability index.
- [docs/programmers/avrisp/README.md](../programmers/avrisp/README.md) — the
  AVRISP-MKII personality.
