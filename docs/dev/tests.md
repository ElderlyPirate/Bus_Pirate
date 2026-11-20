# Native test inventory

**Status:** maintainer-facing.  This is the authoritative inventory of
`Firmware/tests/`.  Every suite compiles real production C against small
mocks; none emulates PIC24 timing, USB enumeration or electrical behaviour.

## Scope

- **In:** the hardware-free native suites under `Firmware/tests/`, their
  coverage, and their runner requirements.
- **Out:** the bootloader test tree (see
  [bootloaders.md](bootloaders.md) §Build and test), the `bp` host-frontend
  tests (see [tooling.md](tooling.md) §Host frontend), and any hardware
  qualification.

## Running the suites

There are **37 `test_*.py` files**.  Each is a standalone script run from the
repository root; there is **no aggregate runner**, so a pass in one says
nothing about the others.

```
python Firmware/tests/test_core_system.py
python Firmware/tests/test_spi.py --case manual
python Firmware/tests/test_avrisp_board_gate.py "C:/Program Files (x86)/Microchip/MPLABXC16/v2.10/bin/xc16-gcc.exe"
```

- Most suites need Python 3 and `gcc` on `PATH`.
- The two XC16 gates (`test_avrisp_board_gate.py`, `test_avrisp_cdc_gate.py`)
  need `xc16-gcc`; they default to the MPLAB-X installer path and accept the
  compiler as the first argument.
- Several suites accept `--case <name>` to run one case.
- Many suites accept an optional `--revision <commit>` to read an older source
  revision with `git show`; the default is the current working tree.
  `test_bitbang.py` without `--case` also compares against its recorded
  reference revision, so run it from a git checkout that contains that
  history.
- `test_avrisp.py` is the large one; the rest are much smaller.
- Generated test files stay under `Firmware/build/` or a temporary directory
  and are never linked into the shipping image.

Support files next to the suites are `avrisp_mock.h`, `avrisp_test.c` (the
AVRISP mock harness), `eeprom_diagnostic_selftest.c` and `BASIC-REVIEW.md`.

## Shared machinery and core

| Suite | Covers | Runner / needs |
|---|---|---|
| `test_protocol_access.py` | live mode-descriptor accessors: `bp_current_protocol()`, `bp_protocol_by_index()`, mode/index pointer identity | direct; `gcc` |
| `test_shared_format.py` | decimal formatter plus parser result/cursor/error/buffer equivalence | direct; `gcc` |
| `test_messages.py` | packed-message reader: both address models, byte lanes, page crossings | direct; `gcc` |
| `test_message_catalog.py` | catalog inventory, both-board regeneration, colour rows/CRLF, generator rejects malformed input, help, UART settings fields, CDC zero, nested messages, keyboard timeout text | direct, `--case`; `gcc` |
| `test_core_system.py` | production `core_system`, `capability_aux` and `protocol_dio` functions: formatting, command-line ring, `getint`/`getrepeat`/`getnumbits`/`getnumber`/`getlong`, baud confirmation, flash read, escape/prefix, macro/history, UART interrupts, VPU/AUX latches, DIO setup | direct, `--case`; `gcc` |
| `test_core_adc.py` | production manual ADC transaction with a pending old-channel completion | direct; `gcc` |
| `test_core_adc_ownership.py` | core ADC refusal/ownership policy, SMPS owner fixture, resumed foreground probe | direct; `gcc` |
| `test_core_baud.py` | terminal raw-BRG policy and the real confirmation/rollback control flow | direct; `gcc` |
| `test_core_cleanup.py` | retired core declarations and stable USB encodings | direct; `gcc` |
| `test_core_config_words.py` | flash-config address/read contracts and board definitions against the linker scripts | direct; `gcc` |
| `test_core_dio_cleanup.py` | core DIO dispatch/park, weak fallback and strong hook selection | direct; `gcc` |
| `test_core_eeprom.py` | onboard EEPROM driver: preserve, fault injection, bounds, ready, bus clear, init | direct, `--case`; `gcc` |
| `test_core_lcd_cleanup.py` | core LCD dispatch/park, disabled/enabled fallback and strong hook | direct; `gcc` |
| `test_core_reset.py` | production reset and SMPS-stop bodies, state and order | direct; `gcc` |
| `test_core_sump_return.py` | core SUMP/BBIO return output and state | direct; `gcc` |
| `test_usb.py` | USB stack: line coding/baud, endpoints, bounds, halt, setup, CDC, ROM table, config/control, hardware, ready | direct, `--case`; `gcc` |
| `test_bitbang.py` | setup/profile selection, waiting-clock bounds, raw and waiting paths, profile-at-entry order, MPLAB paths | direct, `--case`; `gcc`; the no-case comparison reads the recorded reference revision `237d175` from Git history |

## Protocols, capabilities and tools

| Suite | Covers | Runner / needs |
|---|---|---|
| `test_binary_bulk_transfer.py` | production raw-wire bulk handler: 2/3 wires, both orders, ACK/sample framing, no extra clocks | direct; `gcc` |
| `test_binary_capabilities.py` | capability functions with host register mocks (SMPS, self-test, AUX and related binary paths) | direct, `--case`; `gcc` |
| `test_capability_inline_parse.py` | actual AUX/LCD consumers plus the core parser: whitespace, wrap, fallback and defaults | direct; `gcc` |
| `test_dio.py` | production DIO source contracts: disabled build, routing and cleanup | direct, `--case`; `gcc` |
| `test_dio_pins.py` | the three production pin helpers with independent `PORT`/`LAT` storage | direct; `gcc` |
| `test_i2c_binary_aux.py` | extracted I2C `0x09` replies and the unchanged terminal AUX helpers | direct; `gcc` |
| `test_raw2wire_atr.py` | production 2-Wire ATR reset/read ordering and decode contracts | direct; `gcc`; optional revision argument |
| `test_spi.py` | production binary SPI dispatch and CS paths with register and host-I/O mocks | direct, `--case manual|auto|preserve`; `gcc` |
| `test_sump.py` | production SUMP C with PIC-width integers: capacity, readout, count, parser, trigger, timer, LED, rollover and assembly fixture | direct, `--case`; `gcc` |
| `test_uart.py` | production UART setup/dispatch regressions: setup, binary, bridge, common paths, restore, detection, baud | direct, `--case`; `gcc` |
| `test_uart_backpressure.py` | actual UART streaming loops and CDC writers under bounded host stalls | direct; `gcc` |
| `test_uart_bulk.py` | production binary UART plus the RX ring/ISR under a dense loopback schedule | direct; `gcc` |
| `test_uart_rx.py` | production UART2 driver against a four-deep FIFO mock: register ordering, ring ownership, delayed interrupts | direct, `--case`; `gcc` |
| `test_basic.py` | production BASIC tokenizer/interpreter with deterministic hardware stubs | direct; `gcc` |

## Programmers and feature gates

| Suite | Covers | Runner / needs |
|---|---|---|
| `test_avrisp.py` | AVRISP C sources against mocks: V2 engine, ISP, PDI/TPI, XPROG | direct; `gcc` |
| `test_avrisp_board_gate.py` | XC16 feature gate: AVRISP present on v4, absent on v3, forced v3 error | XC16; optional path argument |
| `test_avrisp_cdc_gate.py` | XC16 preprocessed `core_usb/cdc.c` keeps the mkII dispatch hook and the normal CDC fallback | XC16; optional path argument |
| `test_jtag_xsvf_contracts.py` | JTAG/XSVF source contracts: XCOMMENT stall, v3 zero-count guard, result handling | direct; `gcc` |
| `test_pic.py` | PIC ICSP bit engine: `read_bits` / `write_bits` order and width | direct; `gcc` |
| `test_pic_setup.py` | PIC setup parser: inline, invalid and default cases; parser call order and state | direct; `gcc` |

## What the suites do not cover

- PIC24 instruction timing, interrupt latency and code size; those are checked
  by the XC16 build and the map.
- USB enumeration, host driver binding and real USB/SIE behaviour.
- Electrical levels, pull-up values, target loads and protocol conformance on
  a physical bus.
- The v3 hardware path.  v3 suites compile and execute host code; no v3 board
  is used.

A new suite belongs in `Firmware/tests/`, must be added to this table with its
coverage and runner, and must not require a board.  A suite that changes a
gate or fixture also updates the page that documents that gate.

## Verification

The inventory above is taken from the tracked `Firmware/tests/test_*.py` set.
The per-set documentation basis is in [verification.md](verification.md).

## Related docs

- [building.md](building.md) — build and message generation.
- [bootloaders.md](bootloaders.md) — bootloader test tree.
- [tooling.md](tooling.md) — host frontend tests.
