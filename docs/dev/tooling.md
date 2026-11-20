# Tooling and host frontend

**Status:** maintainer-facing.  The directory inventory is taken from the
repository tree; behaviour statements are taken from each script's source.

This page covers the developer-side helpers that live in this repository and
the `bp` Python host frontend that replaced the old host tools.  Firmware
build and flash procedure are [building.md](building.md) and
[flashing.md](flashing.md); the native test inventory is [tests.md](tests.md);
release contents are [release.md](release.md).

## Scope

- **In:** the in-tree generators and support files; the `bp` host frontend,
  its commands, configuration and exit contract; the XSVF host path; the
  removed legacy host tools and their replacements.
- **Out:** third-party client programs (`avrdude`, OpenOCD, picprog, sigrok,
  flashrom), which are covered with the function they serve; firmware build
  and flash; the native C test suites (see [tests.md](tests.md)).

## Host frontend (`bp`)

`scripts/bp/` is the supported console host frontend.  Its distribution name
is **`bus-pirate-host`**, its console command is **`bp`**, and its version is
**0.1.0**.  It is an Apache-2.0 Python package that talks to the Bus Pirate
binary protocol over a serial port.  It has no GUI and no OpenOCD wrapper.

It replaces these removed host trees:

| Removed tool | Replacement |
|---|---|
| `scripts/version.pl` | `bp version` |
| `scripts/I2CEEPROMWIN.c` | `bp eeprom read` / `write` / `verify` |
| `scripts/powertools/LCDv2-tester/` | `bp lcd-test` |
| `scripts/BPXSVFPlayer/`, `scripts/BP4_XSVFplayer_Win/` | `bp xsvf` |
| `scripts/pyBusPirateLite/` | `bp bbio`, `bp raw-wire`, `bp i2c`, `bp spi`, `bp uart`, `bp onewire` |
| `tools/qt_gui/` (GUI and Raw Text tab) | the `bp` command groups; `bp script` / `bp raw-script` for the Raw Text tab |

The old trees remain available in Git history.  `scripts/bp/README.md` is the
authoritative command reference; this page gives the maintainer overview and
does not duplicate every option.

### Requirements and installation

- Python **3.12 or newer**.
- A Bus Pirate v3/v4 connected over USB serial.
- Runtime dependencies: `click`, `pydantic` V2, `pyserial`, `ruamel.yaml`.

From the package directory:

```bash
cd scripts/bp
python -m venv .venv
# Windows: .venv\Scripts\activate
# POSIX:   source .venv/bin/activate
python -m pip install -r requirements.txt
python -m pip install -e .
```

For the verification tools, install `requirements-dev.txt` instead; it
includes the runtime pins and adds `pytest`, `ruff` and `pyright`.

### Command groups

| Command | Purpose |
|---|---|
| `bp ports` | List serial ports with VID:PID, description and likely Bus Pirate markers; never opens a port |
| `bp version` | Read the reset banner and print hardware type, firmware version/build, DEVID/REVID and update URL |
| `bp bbio` | Binary-bitbang entry, pins, self-test, ADC, PWM and raw bit clocks |
| `bp raw-wire` | Raw 2-wire/3-wire configuration, bytes, bits, CS and clocks |
| `bp i2c` | I2C scan, register get/set, commands and block transfers |
| `bp spi` | SPI configuration, JEDEC chip ID, transfers and flash-style read/write |
| `bp uart` | UART configuration, manual BRG, send, receive and bridge setup |
| `bp onewire` | 1-Wire reset, ROM search/read/match/skip, scratchpad, memory and status |
| `bp script` / `bp raw-script` | Send an ASCII-hex file through the binary command loop |
| `bp eeprom read` / `write` / `verify` | External I2C EEPROM text-hex file transfer |
| `bp lcd-test` | LCD adapter v2 production LED/output test |
| `bp xsvf` | Stream an XSVF image, scan the JTAG chain, or reset it (v4) |

`--config` and `--verbose` are root options and must precede the command:

```bash
bp --config bp.yaml eeprom read --chip 0 --length 256 --file eeprom.txt
bp --verbose version --port COM6
```

`--verbose` enables stdlib `DEBUG` logging; the default level is `WARNING`.

A first contact session:

```
$ bp --help
Usage: bp [OPTIONS] COMMAND [ARGS]...

  Host frontend for Bus Pirate v3/v4 devices.

Options:
  --config FILE  Optional YAML configuration file with default 'port' and
                 'speed' values.
  --verbose      Enable diagnostic stdlib logging.
  --help         Show this message and exit.

Commands:
  bbio        Binary bitbang entry, pin control, self-test, ADC, PWM, and...
  eeprom      Read, write, or verify an external I2C EEPROM.
  i2c         Binary I2C scan, register, command, block-read, and...
  lcd-test    Run the LCD adapter v2 LED/output test.
  onewire     Binary 1-Wire reset, ROM search/read/match/skip,...
  ports       List serial ports and mark likely Bus Pirate devices...
  raw-script  Run an ASCII-hex file through the binary command loop.
  raw-wire    Binary raw 2-wire/3-wire configuration, byte, bit, and...
  script      Run an ASCII-hex file through the binary command loop.
  spi         Binary SPI configuration, chip-ID, transfer, and...
  uart        Binary UART configuration, send, receive, and bridge setup.
  version     Read the firmware and hardware version banner.
  xsvf        Play an XSVF file or run a JTAG chain scan/reset with a Bus...

$ bp ports
COM1  ----:----  Communications Port (COM1)
COM6  04D8:FB00  USB Serial Device (COM6)  [Bus Pirate v4 application]
COM9  1209:0002  USB Serial Device (COM9)
```

The `bp ports` markers are:

| VID:PID | Marker | Meaning |
|---|---|---|
| `04D8:FB00` | `[Bus Pirate v4 application]` | v4 running its application firmware |
| `04D8:FAFF` | `[Bus Pirate v4 bootloader]` | v4 in bootloader mode |
| `0403:6001` | `[FTDI (Bus Pirate v3 possible)]` | FTDI bridge; a v3 may use it, but the ID is not unique |

No ports prints `No serial ports found.` and exits 0.  `bp ports` never
connects, never resets the device, and never changes which port another
command uses.

### Configuration

The optional YAML configuration currently supports only `port` and `speed`.
Unknown keys are rejected.

```yaml
port: COM6
speed: 115200
```

Resolution order:

1. An explicit command option wins.
2. Otherwise the configuration value is used.
3. Otherwise the speed is 115200.
4. `bp version` falls back to `COM1` on Windows, `/dev/ttyACM0` on Linux,
   `/dev/cu.usbmodem00000001` on macOS, and `/dev/ttyUSB0` elsewhere.
5. `bp eeprom`, `bp lcd-test`, and `bp xsvf` do not guess a port; they
   require `--port` or a configured `port`.

The file must be a UTF-8 YAML mapping; empty files, invalid UTF-8 and
non-mapping documents fail.  There is no automatic config-file search.

### Hex and decimal input

Click integer options (`--address`, `--register`, `--value`, `--direction`,
`--state`, `--opcode`, `--length`) are **decimal**.  Hex is used for
`--data`/`--file` byte payloads, 1-Wire `--rom` values and printed protocol
data.  Convert a desired hex value in the shell or config file: `0x50` becomes
`80`.

Text-hex files are UTF-8 (an optional BOM is accepted), whitespace-separated
one- or two-digit tokens; `AA`, `aa`, `0xAA` and `0Xaa` are accepted.  Packed
strings such as `AABB`, truncated tokens and out-of-range values are rejected
before the serial port is opened.

### Exit codes

| Exit | Meaning |
|---|---|
| `0` | Command completed successfully, including an empty I2C/1-Wire scan, a zero-error self-test, `bp ports` with no ports, and a successful XSVF scan/reset |
| `1` | Runtime failure: bad port, timeout, protocol/NACK error, invalid hex, non-zero self-test, EEPROM verification mismatch, or a non-success XSVF player result |
| `2` | Click usage failure, or an XSVF host/transport/setup failure: bad or empty file, wrong `BBIO1`/`XSV1` banner, missing reply/result, incomplete scan reply, failed `--leave` sequence, or a missing required port |

The `bp xsvf` exit contract is stricter than the other commands: transport and
setup failures exit 2, a player result byte exits 1, and a successful
scan/reset/play exits 0.  See §XSVF host path.

### Host frontend tests

Run from `scripts/bp` with the local virtual environment active:

```bash
python -m pytest
ruff check .
ruff format --check .
pyright
```

The tests inject fake serial transports and protocol sessions; no hardware is
required.  They cover CLI wiring and exit codes, configuration validation,
port enumeration, transport failures, file formats, exact per-mode wire
sequences, protocol/rejection paths, EEPROM retry/verify, LCD cycles and the
XSVF chunk/result/leave handshake.  The test tree is `tests/` with
`test_protocol/` and `test_commands/` subdirectories.

### Host frontend limits

- **No GUI** and no Windows `conio` keyboard polling.
- **No automatic port selection.**  `bp ports` lists candidates; every other
  command needs an explicit or configured port.
- **No SVF parser or converter.**  Convert SVF to XSVF externally (Xilinx
  iMPACT or `svf2xsvf502`) before `bp xsvf`.
- **No EEPROM page writes.**  Every byte is a separate I2C transaction.
- **No binary-bitbang speed command.**  The v9 firmware has no `0x60`/`0x70`
  in BBIO; use the per-mode `--bus-speed` options.
- **No SPI configuration read-back and no UART configuration read-back.**  The
  CLI reports the values it applied.
- **UART bridge never returns**; only a hardware reset leaves it.
- **`bp xsvf` leaves the regulator on by default.**  Use `--leave` or press
  reset; see below.

## XSVF host path

`bp xsvf` is the current host player.  It targets the built-in v4 firmware
player: enter `BBIO1`, send `0x18`, wait for `XSV1`, then carry out reset,
scan and/or stream in that order.

| Option | Native flag | Meaning |
|---|---|---|
| `--port` | `-p` | Serial port; required unless the configuration supplies `port` |
| `--file` | `-f` | XSVF image to stream; optional when `--scan` or `--reset` is used |
| `--speed` | `-s` | Host serial speed, default 115200; does not change the firmware player's TCK |
| `--scan` | `-x` | Probe the JTAG chain and print the count byte and IDCODE bytes |
| `--reset` | `-r` | Reset the chain before scanning or streaming |
| `--leave` | none | After a successful run, send `0x0F` twice and return to the terminal |

Examples:

```
bp xsvf --port COM6 --reset
bp xsvf --port COM6 --scan
bp xsvf --port COM6 --scan --leave
bp xsvf --port COM6 --file design.xsvf --reset
```

Operation order:

1. Read the whole XSVF file; unreadable or empty files are rejected before the
   port is opened.
2. Enter binary bitbang (`BBIO1`), then the XSVF player (`XSV1`).
3. Send the chain reset if `--reset` was given.
4. Run the chain scan if `--scan` was given.
5. Stream the file if `--file` was given.
6. If `--leave` was given, return the board to the terminal.

File streaming:

- The image is sent in chunks of at most 4096 bytes, matching the firmware's
  chunk buffer.  The last chunk is the remainder; an exact multiple of 4096
  sends no zero-length chunk.
- The player asks for data with `0xFF`.  Each answer is a 16-bit big-endian
  length followed by exactly that many XSVF bytes.
- A missing complete result, or a request for more data after EOF, reports
  `End of file reached without a completion result` and exits 2.
- Result `0x00` prints `Success!` and exits 0; any other result prints the
  firmware's symbolic name and exits 1; an unknown result byte is reported as
  `XSVF_ERROR_UNKNOWN`.

Chain scan reply: one count byte equal to four times the number of devices,
then four IDCODE bytes per device, least-significant first and the device
nearest TDO first.  Example: `Chain Scan Result: 04 93 40 60 59` is one
device with IDCODE `0x59604093`.

Without `--leave`, a successful operation intentionally leaves the board in
the firmware player, as the old C players did; the regulator stays on.  With
`--leave`, the host sends `0x0F` once (return to `BBIO1`, no reply), sends
`0x0F` again (terminal reset), requires the `0x01` acknowledgement, and drains
the terminal banner.  A failed leave sequence exits 2.  A non-success result
or host error skips the leave sequence, because the player may still be busy.

Firmware limits: v4 only; longest single shift 400 bits; 4096-byte chunk;
63-device scan clamp; 3.3 V push-pull outputs only.  The firmware protocol and
result codes are in [docs/protocol/jtag/xsvf.md](../protocol/jtag/xsvf.md).
SVF conversion is an external prerequisite.

## Repository generators and support files

| Path | What it is |
|---|---|
| `tools/packstrings/packstrings.py` | Generates `Firmware/core_messages/messages_v{3,4}.{s,h}` from the tab-separated string files beside it.  See [building.md](building.md) §Message strings. |
| `tools/packstrings/bus_pirate_{common,v3,v4}_strings.txt` | The authored terminal strings; the version files include the common file. |
| `Firmware/Doxyfile` | Doxygen configuration for the firmware sources. |
| `Firmware/tests/` | The native hardware-free suites; see [tests.md](tests.md). |
| `Bootloaders/tests/` | Bootloader image/protocol tests and opt-in probes; see [bootloaders.md](bootloaders.md) §Build and test. |
| `contrib/98-buspirate.rules` | Linux udev rule for `/dev/buspirate`; see [flashing.md](flashing.md) §Drivers and permissions. |
| `hardware/` | Eagle schematics/boards, previews and 3D models; see [hardware/README.md](../../hardware/README.md). |
| `scripts/bp/` | The `bp` host frontend. |

Removed legacy samples (the C# raw-wire client, the oscilloscope example, the
old Perl examples, `P24qp`, `TestFT232`, the high-voltage self-test and the
original Qt GUI) remain available in Git history.  These are host-side
programs; the repository does not package or install them separately from
`bp`.

## Test suites

The authoritative firmware native-test table is [tests.md](tests.md).  The
bootloader tests are listed in [bootloaders.md](bootloaders.md) §Build and
test; the host frontend tests are the `pytest` suite in `scripts/bp/tests/`.

## Verification

The directory and command inventories above are taken from the tracked
files.  The per-set documentation basis is in [verification.md](verification.md).

## Limits & out of scope

- Third-party client programs (avrdude, OpenOCD, picprog, sigrok/PulseView,
  flashrom) are covered with the function they serve.
- Windows installers, signing and distribution channels are out of scope.
- `bp` is a host tool; it does not replace the firmware or the bootloader.
