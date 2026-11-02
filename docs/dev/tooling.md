# Tooling and packaging

**Status:** draft.  Directory inventory confirmed against the repository tree;
no script behavior is asserted beyond its file name and language.

This page inventories the developer-side helpers that live in this repository
and how release artifacts are assembled.  Host *client* programs that drive a
specific Bus Pirate function (for example `avrdude`, OpenOCD, picprog, sigrok)
are mentioned in that function's page, not here.

## Scope

- **In:** the in-tree generators, tests, scripts and host tools, and the
  manual packaging steps for a release.
- **Out:** third-party tools, and the firmware/flash procedure
  ([building.md](building.md), [flashing.md](flashing.md)).

## Repository tooling

| Path | What it is |
|---|---|
| `tools/packstrings/packstrings.py` | Generates `Firmware/messages_v{3,4}.{s,h}` from the tab-separated string files beside it.  See [building.md](building.md) §Message strings. |
| `tools/packstrings/bus_pirate_{common,v3,v4}_strings.txt` | The authored terminal strings; the version files include the common file. |
| `tools/qt_gui/` | A Qt GUI for the Bus Pirate (project `BusPirateGUI.pro`, with a bundled `qextserialport`). |
| `Firmware/tests/test_native.py` | Runner for the hardware-free production-source tests (needs Python 3 + host GCC).  See [building.md](building.md) §Native tests. |
| `Firmware/tests/test_protocol_access.py`, `test_shared_format.py`, `test_messages.py` | The three harnesses `test_native.py` drives. |
| `Firmware/tests/test_avrisp*.py`, `avrisp_mock.h`, `avrisp_test.c` | AVRISP-specific native harnesses, run by their own gate scripts. |
| `Firmware/Doxyfile` | Doxygen configuration for the firmware sources. |
| `Bootloaders/pirate-loader/test.sh`, `test.bat` | Smoke scripts that run `pirate-loader --hello` against a default port to check the bootloader handshake. |
| `scripts/version.pl` | Perl host script that reads the firmware/hardware version banner from the port. |
| `scripts/BPbinmode.pl`, `scripts/BP_matrix.pl`, `scripts/selftest.pl`, `scripts/SPIeeprom.pl` | Perl host scripts (binary-mode, matrix, self-test, SPI EEPROM). |
| `scripts/P24qp.py` | Python 2 "Bus Pirate Programmer" host tool for PIC24F devices. |
| `scripts/I2CEEPROMWIN.c` | Windows host tool for I2C EEPROM work. |
| `scripts/pyBusPirateLite/` | A Python host library and sample clients for the Bus Pirate modes. |
| `scripts/BPXSVFPlayer/` | Cross-platform XSVF player (C, CMake/Makefile); `scripts/BP4_XSVFplayer_Win/` is the Windows variant. |
| `scripts/BusPirateRawCS/` | C# raw-wire client (Visual Studio solution). |
| `scripts/oscilloscope/` | A scope-style sample client. |
| `scripts/powertools/` | Sampler/utility clients: `SPISniffer/`, `LCDv2-tester/`, `self-test/`, `BP_MMA_Selftest/` (accelerometer self-test, Code::Blocks project). |
| `scripts/powertools/framework/` | Shared C client framework (`buspirate.c`, `serial.c`) used by the `powertools/` clients. |
| `scripts/HVPselftest/`, `scripts/TestFT232/` | Legacy hardware self-test programs. |

These are host-side programs; the repository does not package or install them.

## Packaging

There is **no packaging script** in this repository.  A release is assembled
by hand from clean *production* builds:

| Component | Where it comes from |
|---|---|
| v3 firmware | production build of the `BusPirate_v3` MPLAB-X configuration → `Firmware/busPirate.X/dist/BusPirate_v3/production/busPirate.X.production.hex` |
| v4 firmware | production build of `BusPirate_v4` → `…/dist/BusPirate_v4/production/busPirate.X.production.hex` |
| v3 bootloader | `Bootloaders/BPv3-bootloader/firmware-v4.5/` (ds30 loader) |
| v4 bootloader | `Bootloaders/BPv4-bootloader/firmware-v1/` |

- The IDE output paths follow the MPLAB-X / NetBeans convention (see
  [building.md](building.md) §MPLAB-X route); the command-line Makefile writes
  `Firmware/bpv3.hex` / `bpv4.hex` instead.
- Older release notes name the artifacts `bpv3-firmware.hex`,
  `bpv3-bootloader45.hex`, `bpv4-firmware.hex` and `bpv4-bootloader1.hex`;
  those names came from a manual clean-and-build-and-rename step, not a
  tracked script.

## Verification

- Not run for this page; see [verification.md](verification.md) for the
  maintainer record.

## Limits & out of scope

- Third-party client programs (avrdude, OpenOCD, picprog, sigrok/PulseView,
  flashrom) are covered with the function they serve.
- Windows installers, signing and distribution channels are out of scope.