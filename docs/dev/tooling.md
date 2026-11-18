# Tooling, tests and release assembly

**Status:** maintainer-facing.  Directory inventory taken from the repository tree;
behaviour statements are taken from each script's source.  No script or test
was run for this page.

This page inventories the developer-side helpers that live in this repository,
the native test suites and how release artifacts are assembled.  Host *client*
programs that drive a specific Bus Pirate function (for example `avrdude`,
OpenOCD, picprog, sigrok) are mentioned in that function's page, not here.

## Scope

- **In:** the in-tree generators, scripts and host tools; the authoritative
  native-test suite table; the manual release-assembly steps.
- **Out:** third-party tools, and the firmware/flash procedure
  ([building.md](building.md), [flashing.md](flashing.md)).

## Repository scripts and host tools

| Path | What it is |
|---|---|
| `tools/packstrings/packstrings.py` | Generates `Firmware/core_messages/messages_v{3,4}.{s,h}` from the tab-separated string files beside it.  See [building.md](building.md) §Message strings. |
| `tools/packstrings/bus_pirate_{common,v3,v4}_strings.txt` | The authored terminal strings; the version files include the common file. |
| `tools/qt_gui/` | A Qt GUI for the Bus Pirate (project `BusPirateGUI.pro`, with a bundled `qextserialport`). |
| `Firmware/Doxyfile` | Doxygen configuration for the firmware sources. |
| `Bootloaders/pirate-loader/test.sh`, `test.bat` | Smoke scripts that run `pirate-loader --hello` against a default port to check the bootloader handshake. |
| `scripts/version.pl` | Perl host script that reads the firmware/hardware version banner from the port. |
| `scripts/BPbinmode.pl`, `scripts/BP_matrix.pl`, `scripts/selftest.pl`, `scripts/SPIeeprom.pl` | Perl host scripts (binary-mode, matrix, self-test, SPI EEPROM). |
| `scripts/P24qp.py` | Python 2 "Bus Pirate Programmer" host tool for PIC24F devices. |
| `scripts/I2CEEPROMWIN.c` | Windows host tool for I2C EEPROM work. |
| `scripts/pyBusPirateLite/` | A Python host library and sample clients for the Bus Pirate modes. |
| `scripts/BPXSVFPlayer/` | Cross-platform XSVF player (C; Code::Blocks `BPXSVFplayer.cbp` + plain `Makefile`). |
| `scripts/BP4_XSVFplayer_Win/` | Windows XSVF-player variant (Dev-C++ `.dev` + `Makefile.win`). |
| `scripts/BusPirateRawCS/` | C# raw-wire client (Visual Studio solution). |
| `scripts/oscilloscope/` | A scope-style sample client. |
| `scripts/powertools/` | Sampler/utility clients: `SPISniffer/`, `LCDv2-tester/`, `self-test/`, `BP_MMA_Selftest/` (accelerometer self-test, Code::Blocks project). |
| `scripts/powertools/framework/` | Shared C client framework (`buspirate.c`, `serial.c`) used by the `powertools/` clients. |
| `scripts/HVPselftest/`, `scripts/TestFT232/` | Legacy hardware self-test programs. |

These are host-side programs; the repository does not package or install them.
The two XSVF players are plain Makefile/IDE projects — neither has a
`CMakeLists.txt`.

## Test suites

The inventory below is authoritative for `Firmware/tests/`.  Every suite is a
standalone script run from the repository root; generated files stay under
`Firmware/build/` and are not linked into the image.  None of them emulates
PIC24 timing, USB enumeration or electrical behaviour.

| Suite | Covers | Runner / needs |
|---|---|---|
| `test_protocol_access.py` | live mode-descriptor accessors: `bp_current_protocol()`, `bp_protocol_by_index()`, mode/index pointer identity | direct; `gcc` |
| `test_shared_format.py` | decimal formatter plus parser result/cursor/error/buffer equivalence | direct; `gcc` |
| `test_messages.py` | packed-message reader: both address models, byte lanes, page crossings | direct; `gcc` |
| `test_core_system.py` | production `core_system`, `capability_aux` and `protocol_dio` functions: formatting, command-line ring, `getint`/`getrepeat`/`getnumbits`/`getnumber`/`getlong`, baud confirmation, flash read, escape/prefix, macro/history, UART interrupts, VPU/AUX latches, DIO setup (25 cases) | direct, `--case` selects one |
| `test_usb.py` | USB stack: line coding/baud, endpoint state, bounds, halt, setup, CDC, ROM table, config/control, hardware, ready | direct; `gcc` |
| `test_bitbang.py` | setup/profile selection, waiting-clock bounds, raw and waiting paths, profile-at-entry order in the three clients, MPLAB paths | direct; `gcc` |
| `test_message_catalog.py` | catalog inventory, both-board regeneration, colour rows/CRLF, generator rejects malformed input, help, UART settings fields, CDC zero, nested messages | direct; `gcc` |
| `test_core_eeprom.py` | onboard EEPROM driver: preserve, fault injection, bounds, ready, bus clear, init, MPLAB paths | direct; `gcc` |
| `test_pic.py` | PIC ICSP bit engine: `read_bits` / `write_bits` order and width | direct; `gcc` |
| `test_avrisp.py` | the AVRISP C sources against mocks (V2 engine, ISP, PDI/TPI, XPROG) | direct; `gcc` |
| `test_avrisp_board_gate.py` | XC16 feature gate: AVRISP present on v4, absent on v3 | direct; optional `xc16-gcc` path |
| `test_avrisp_cdc_gate.py` | XC16 preprocessed `core_usb/cdc.c` keeps the mkII dispatch hook | direct; optional `xc16-gcc` path |

Support files next to the suites: `avrisp_mock.h`, `avrisp_test.c` (the AVRISP
mock harness) and `eeprom_diagnostic_selftest.c`.  The two XC16 gate scripts
default to the MPLAB-X installer path and accept the compiler as the first
argument.

There is no aggregate runner: each suite is run directly, so a pass in one says
nothing about the others.

## Release assembly

There is **no packaging script** in this repository.  A release is assembled
by hand from clean *production* builds:

| Component | Where it comes from |
|---|---|
| v3 firmware | production build of the `BusPirate_v3` MPLAB-X configuration → `Firmware/busPirate.X/dist/BusPirate_v3/production/busPirate.X.production.hex` |
| v4 firmware | production build of `BusPirate_v4` → `…/dist/BusPirate_v4/production/busPirate.X.production.hex` |
| v3 bootloader | `Bootloaders/BPv3-bootloader/firmware-v4.5/` (ds30 loader), built with MPLAB-X/XC16 |
| v4 bootloader | `Bootloaders/BPv4-bootloader/firmware-v1/`, built with MPLAB-X/XC16 |

- **Not verified:** the `dist/<config>/production/` directory is the MPLAB-X /
  NetBeans convention; the project files do not override it, but the exact
  generated path is IDE output, not a tracked artifact.  See
  [building.md](building.md) §MPLAB-X route.  The command-line Makefile writes
  `Firmware/bpv3.hex` / `bpv4.hex` instead.
- Older release notes name the artifacts `bpv3-firmware.hex`,
  `bpv3-bootloader45.hex`, `bpv4-firmware.hex` and `bpv4-bootloader1.hex`;
  those names came from a manual clean-and-build-and-rename step, not a
  tracked script.
- Bootloader images are not prebuilt in the tree; the v3 one is built from
  `firmware-v4.5/` (see [bootloaders.md](bootloaders.md) §v3 update route).

## Verification

- Not run for this page; see [verification.md](verification.md) for the
  maintainer record.

## Limits & out of scope

- Third-party client programs (avrdude, OpenOCD, picprog, sigrok/PulseView,
  flashrom) are covered with the function they serve.
- Windows installers, signing and distribution channels are out of scope.
