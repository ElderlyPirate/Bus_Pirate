# Tooling, tests and release assembly

**Status:** maintainer-facing.  The directory inventory is taken from the
repository tree; behaviour statements are taken from each script's source.

This page inventories the developer-side helpers that live in this repository,
the native test suites and how release artifacts are assembled.  Host *client*
programs that drive a specific Bus Pirate function (for example `avrdude`,
OpenOCD, picprog, sigrok) are mentioned in that function's page, not here.

## Scope

- **In:** the in-tree generators, scripts and host tools; the authoritative
  native-test suite table; the XSVF host-tool caveats; the manual
  release-assembly steps.
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

### XSVF host players

The two players are the same tool in two builds.  Their options and transcripts
are in [docs/tools/xsvf/players.md](../tools/xsvf/players.md); the following
points matter when maintaining or debugging them:

| Point | Behaviour |
|---|---|
| build files | Windows: Dev-C++ `.dev` + `Makefile.win`; cross-platform: plain `Makefile` + Code::Blocks `.cbp` |
| `-x` with `-f` | unsafe in one invocation: the chain-scan read is unsized, and leftover reply bytes are read as the run result |
| Windows exit status | `0` after any run, whatever the result; `-1` for a setup failure |
| cross-platform exit status | `0`, `1`, `2` or `-1`; `End of file reached.` still returns `0` without a result byte |
| host exit | the host closes the port without sending `0x0F`, so the firmware player keeps the regulator on until reset |
| `-s` | host port speed only; it does not change the Bus Pirate's rate |

- Run a chain scan in its own invocation, reset the board, then run the file
  with `-r` only.  A custom host that combines them must read the count byte
  and exactly `count` more bytes before `0x03`.
- Do not treat the Windows exit status as a result; read
  `End of operation reply:` and `Success!`.
- The player is v4-only; on v3 the `0x18` byte is answered with `0x00`.
- Failure and recovery detail:
  [docs/tools/xsvf/limits-and-errors.md](../tools/xsvf/limits-and-errors.md).

## Test suites

The inventory below is authoritative for `Firmware/tests/`.  Every one of the
**32 `test_*.py`** files is a standalone script run from the repository root;
there is **no aggregate runner**, so a pass in one says nothing about the
others.  Generated files stay under `Firmware/build/` or a temporary
directory and are not linked into the image.  None of them emulates PIC24
timing, USB enumeration or electrical behaviour.

### Shared machinery and core

| Suite | Covers | Runner / needs |
|---|---|---|
| `test_protocol_access.py` | live mode-descriptor accessors: `bp_current_protocol()`, `bp_protocol_by_index()`, mode/index pointer identity | direct; `gcc` |
| `test_shared_format.py` | decimal formatter plus parser result/cursor/error/buffer equivalence | direct; `gcc` |
| `test_messages.py` | packed-message reader: both address models, byte lanes, page crossings | direct; `gcc` |
| `test_message_catalog.py` | catalog inventory, both-board regeneration, colour rows/CRLF, generator rejects malformed input, help, UART settings fields, CDC zero, nested messages, keyboard timeout text | direct, `--case`; `gcc` |
| `test_core_system.py` | production `core_system`, `capability_aux` and `protocol_dio` functions: formatting, command-line ring, `getint`/`getrepeat`/`getnumbits`/`getnumber`/`getlong`, baud confirmation, flash read, escape/prefix, macro/history, UART interrupts, VPU/AUX latches, DIO setup | direct, `--case`; `gcc` |
| `test_core_adc.py` | production manual ADC transaction with a pending old-channel completion | direct; `gcc` |
| `test_core_adc_ownership.py` | core ADC refusal/ownership policy, SMPS owner fixture, resumed foreground probe | direct; `gcc` |
| `test_core_cleanup.py` | retired core declarations and stable USB encodings | direct; `gcc` |
| `test_core_config_words.py` | flash-config address/read contracts and board definitions against the linker scripts | direct; `gcc` |
| `test_core_dio_cleanup.py` | core DIO dispatch/park, weak fallback and strong hook selection | direct; `gcc` |
| `test_core_eeprom.py` | onboard EEPROM driver: preserve, fault injection, bounds, ready, bus clear, init | direct, `--case`; `gcc` |
| `test_core_lcd_cleanup.py` | core LCD dispatch/park, disabled/enabled fallback and strong hook | direct; `gcc` |
| `test_core_reset.py` | production reset and SMPS-stop bodies, state and order | direct; `gcc` |
| `test_core_sump_return.py` | core SUMP/BBIO return output and state | direct; `gcc` |
| `test_usb.py` | USB stack: line coding/baud, endpoints, bounds, halt, setup, CDC, ROM table, config/control, hardware, ready | direct, `--case`; `gcc` |
| `test_bitbang.py` | setup/profile selection, waiting-clock bounds, raw and waiting paths, profile-at-entry order, MPLAB paths | direct, `--case`; `gcc` |

### Protocol and tool families

| Suite | Covers | Runner / needs |
|---|---|---|
| `test_binary_bulk_transfer.py` | production raw-wire bulk handler: 2/3 wires, both orders, ACK/sample framing, no extra clocks | direct; `gcc` |
| `test_binary_capabilities.py` | capability functions with host register mocks (SMPS, self-test, AUX and related binary paths) | direct, `--case`; `gcc` |
| `test_capability_inline_parse.py` | actual AUX/LCD consumers plus the core parser: whitespace, wrap, fallback and defaults | direct; `gcc` |
| `test_dio.py` | production DIO source contracts: disabled build, routing and cleanup | direct, `--case`; `gcc` |
| `test_dio_pins.py` | the three production pin helpers with independent `PORT`/`LAT` storage | direct; `gcc` |
| `test_sump.py` | production SUMP C with PIC-width integers: capacity, readout, count, parser, trigger, timer, LED, rollover and assembly fixture | direct, `--case`; `gcc` |
| `test_uart.py` | production UART setup/dispatch regressions: setup, binary, bridge, common paths, restore, detection, baud | direct, `--case`; `gcc` |
| `test_uart_backpressure.py` | actual UART streaming loops and CDC writers under bounded host stalls | direct; `gcc` |
| `test_uart_bulk.py` | production binary UART plus the RX ring/ISR under a dense loopback schedule | direct; `gcc` |
| `test_uart_rx.py` | production UART2 driver against a four-deep FIFO mock: register ordering, ring ownership, delayed interrupts | direct, `--case`; `gcc` |
| `test_basic.py` | production BASIC tokenizer/interpreter with deterministic hardware stubs | direct; `gcc` |

### Programmer and feature gates

| Suite | Covers | Runner / needs |
|---|---|---|
| `test_avrisp.py` | AVRISP C sources against mocks: V2 engine, ISP, PDI/TPI, XPROG | direct; `gcc` |
| `test_avrisp_board_gate.py` | XC16 feature gate: AVRISP present on v4, absent on v3, forced v3 error | direct; optional `xc16-gcc` path |
| `test_avrisp_cdc_gate.py` | XC16 preprocessed `core_usb/cdc.c` keeps the mkII dispatch hook and the normal CDC fallback | direct; optional `xc16-gcc` path |
| `test_pic.py` | PIC ICSP bit engine: `read_bits` / `write_bits` order and width | direct; `gcc` |
| `test_pic_setup.py` | PIC setup parser: inline, invalid and default cases; parser call order and state | direct; `gcc` |

Support files next to the suites: `avrisp_mock.h`, `avrisp_test.c` (the AVRISP
mock harness) and `eeprom_diagnostic_selftest.c`.  Several fixtures read a
historical revision with `git show`; run them from a git checkout.  The two
XC16 gate scripts default to the MPLAB-X installer path and accept the
compiler as the first argument.

## Release assembly

There is **no packaging script** in this repository.  A release is assembled
by hand from clean *production* builds:

| Component | Where it comes from |
|---|---|
| v3 firmware | production build of the `BusPirate_v3` MPLAB-X configuration → `Firmware/busPirate.X/dist/BusPirate_v3/production/busPirate.X.production.hex` |
| v4 firmware | production build of `BusPirate_v4` → `…/dist/BusPirate_v4/production/busPirate.X.production.hex` |
| v3 bootloader | `Bootloaders/BPv3-bootloader/firmware-v4.5/` (ds30 loader), built with MPLAB-X/XC16 |
| v4 bootloader | `Bootloaders/BPv4-bootloader/firmware-v1/`, built with MPLAB-X/XC16 |

- **Unverified:** the `dist/<config>/production/` directory is the MPLAB-X /
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

The suite and tool inventories above are taken from the tracked files.  The
per-set documentation basis is in [verification.md](verification.md).

## Limits & out of scope

- Third-party client programs (avrdude, OpenOCD, picprog, sigrok/PulseView,
  flashrom) are covered with the function they serve.
- Windows installers, signing and distribution channels are out of scope.
- The test table names the current `Firmware/tests/test_*.py` set; a new suite
  must be added here with its coverage and runner.
