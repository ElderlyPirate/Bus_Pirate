# `bp` - Bus Pirate host frontend

`bp` is the console frontend for Bus Pirate v3/v4 host-side workflows. It
replaces the legacy `version.pl`, `I2CEEPROMWIN.c`, and LCD adapter v2 test
utility with one typed, testable Python package. It talks to the Bus Pirate
binary protocol over a serial port and does not use a GUI.

The legacy tools remain in the repository; this package is the intended single
modern frontend. XSVF playback is explicitly out of scope for this phase.

## Scope

Implemented commands:

| Command | Purpose | Legacy equivalent |
|---|---|---|
| `bp version` | Read and print the firmware/hardware reset banner. | `scripts/version.pl` |
| `bp eeprom read` | Read an external I2C EEPROM into a text-hex file. | `scripts/I2CEEPROMWIN.c R/Rs` |
| `bp eeprom write` | Write a text-hex file to an external I2C EEPROM one byte at a time. | `scripts/I2CEEPROMWIN.c W/Ws` |
| `bp eeprom verify` | Compare an EEPROM span against a text-hex file. | New convenience |
| `bp lcd-test` | Run the LCD adapter v2 production LED/output test. | `scripts/powertools/LCDv2-tester` |

Legacy mode mapping: `R`/`W` use two-byte memory addressing
(`--address-width 2`); `Rs`/`Ws` use one-byte addressing
(`--address-width 1`).

Not implemented in this package:

- XSVF/JTAG playback.
- GUI or keyboard polling through Windows `conio`.
- Automatic discovery of Bus Pirate serial ports.
- Page writes for EEPROM devices. Writes always use one byte per I2C
  transaction, matching `I2CEEPROMWIN.c`, because EEPROM page sizes are not
  standardized.
- Changes to or removal of the legacy tools.

## Requirements and installation

- Python 3.12 or newer.
- A Bus Pirate v3/v4 connected over USB serial.

| Dependency | Role |
|---|---|
| `click` | CLI group, options, usage errors, and output. |
| `pydantic` V2 | Strict validation of the optional YAML config model. |
| `pyserial` | Serial port open/read/write/timeout boundary. |
| `ruamel.yaml` | Safe YAML loading at the configuration boundary. |

Install from the package directory:

```bash
cd scripts/bp
python -m venv .venv
# Windows: .venv\Scripts\activate
# POSIX:   source .venv/bin/activate
python -m pip install -r requirements.txt
python -m pip install -e .
```

For the verification tools, install `requirements-dev.txt` instead:

```bash
python -m pip install -r requirements-dev.txt
python -m pip install -e .
```

`requirements.txt` contains the runtime pins; `requirements-dev.txt` includes
it and adds the pinned `pytest`, `ruff`, and `pyright` tools.

## Quick start

```bash
# Firmware/hardware banner, from the platform default port
bp version

# Explicit port and 115200 baud
bp version --port COM6 --speed 115200

# Read 256 bytes of a 24FC64-class EEPROM (two address bytes)
bp eeprom read --port COM6 --chip 0 --address-width 2 --memory-address 0 --length 256 --file eeprom.txt

# Write the same text-hex file, then verify it
bp eeprom write --port COM6 --chip 0 --address-width 2 --memory-address 0 --file eeprom.txt
bp eeprom verify --port COM6 --chip 0 --address-width 2 --memory-address 0 --file eeprom.txt

# One LCD adapter v2 test cycle
bp lcd-test --port COM6

# Five cycles for a longer visible blink
bp lcd-test --port COM6 --repeat 5

# Production-style repeat until the operator types q
bp lcd-test --port COM6 --interactive
```

The examples are single-line commands so they work unchanged in POSIX shells
and Windows PowerShell.

`--config` and `--verbose` are root options and must precede the command:

```bash
bp --config bp.yaml eeprom read --chip 0 --length 256 --file eeprom.txt
bp --verbose version --port COM6
```

`--verbose` enables stdlib `DEBUG` logging; the default level is `WARNING`.

## Configuration

The optional YAML configuration currently supports only `port` and `speed`.
These are the only values the commands use. Unknown keys are rejected.

```yaml
port: COM6
speed: 115200
```

Resolution rules:

1. An explicit command option wins.
2. Otherwise the configuration value is used.
3. Otherwise the speed is 115200.
4. `bp version` falls back to `COM1` on Windows, `/dev/ttyACM0` on Linux,
   `/dev/cu.usbmodem00000001` on macOS, and `/dev/ttyUSB0` elsewhere.
5. `bp eeprom` and `bp lcd-test` do not guess a port; they require `--port` or
   a configured `port`.

The YAML file is validated strictly. `port` must be a non-empty string,
`speed` must be a positive integer, and extra keys fail rather than being
ignored. The file must be a UTF-8 YAML mapping; empty files, invalid UTF-8, and
non-mapping documents fail. There is no automatic config-file search.

## Command reference

### `bp version`

Reads the text banner produced by the Bus Pirate reset command, then prints:

```text
Hardware type:    Bus Pirate v4
Firmware version: ElderlyPirate Firmware v9.0 - goo.gl/gCzQnW - 14256fd5
Hardware version: DEVID:0x1019 REVID:0x0004 (24FJ256GB106 A6)
Update URL:       http://github.com/ElderlyPirate
```

The parser looks for the last `RESET` line, then selects the `Bus Pirate...`
line, the `...Firmware...` line, the `DEVID:...` line, and the `http://`/
`https://` URL. The protocol list and the optional v3 `Bootloader v...` line
therefore do not shift the fields. An incomplete banner fails with a message
asking for a device reset. This corrects the legacy tool's positional field
selection; see [Limitations and known issues](#limitations-and-known-issues).

The device must be at its user-terminal prompt. Reset it first if it is in a
binary mode, and allow the banner to start within the 1 s serial read timeout.

Options: `--port`, `--speed`.

### `bp eeprom`

All subcommands share:

| Option | Meaning |
|---|---|
| `--chip 0..7` | Three-bit EEPROM chip-select address. The write address is `0xA0 \| (chip << 1)`. |
| `--address-width 1\|2` | One or two memory-address bytes. Defaults to `2`, matching the old tool's `R`/`W` mode. |
| `--memory-address` | Starting memory address. Defaults to 0. Range is `0x0000..0x07FF` for one-byte addressing and `0x0000..0xFFFF` for two-byte addressing. |
| `--port` | Serial port. Required unless configured. |
| `--speed` | Serial speed. Defaults to 115200. |

`bp eeprom read` additionally requires `--length 1..65536` and `--file` (output
path). `write` and `verify` take an existing `--file`; their transfer length is
the number of bytes in the file.

`--memory-address` and `--length` accept decimal integers only; hexadecimal is
used only in messages. The legacy tool also parsed decimal values.

EEPROM transfers enable the Bus Pirate power supply and on-board pull-ups. Do
not use this command with an externally powered EEPROM board unless the supply
and pull-up configuration is safe for that board. The Bus Pirate hardware
configuration determines the supply voltage. I2C bus speed uses the firmware's
binary-mode default; this frontend adds no I2C speed option.

Text-hex format:

- UTF-8 text; an optional UTF-8 BOM is accepted. The content must be ASCII hex.
- Whitespace-separated tokens, each one or two hex digits.
- Accepted forms include `AA`, `aa`, `0xAA`, and `0Xaa`.
- No comma-separated or packed continuous strings such as `AABB`.
- Empty files, truncated tokens such as `0x`, and out-of-range values are
  rejected before the serial port is opened.
- Read output is uppercase, space-separated, two digits per byte, with a final
  newline: `AA BB CC\n`.

One-byte addressing encodes memory-address bits 8 through 10 in the I2C device
address, so the addressable span is 2 KiB. The implementation ORs those page
bits into the chip address consistently for the read and write paths. The
legacy C read path did not apply the same page bits as its write path; the new
read path corrects that mismatch.

Write behavior:

- Every byte is a separate start/address/data/stop transaction; page writes are
  never used.
- A NACKed byte is retried up to 100 times with a 5 ms delay between attempts.
  The legacy tool retried indefinitely; the bounded limit prevents a missing or
  damaged EEPROM from hanging a script forever.
- The retry count is reported after a successful write.

Read behavior:

- The memory pointer is set with a write transaction, then a separate read
  transaction reads the requested bytes. The final byte is NACKed directly;
  the legacy tool clocked one extra discarded byte before its NACK.
- The command writes the data to the requested file and prints a summary line
  rather than dumping the entire image to the console.
- Verification reads the expected span and reports the first mismatch as
  offset, expected value, and actual value.

### `bp lcd-test`

Runs the LCD adapter v2 production test over binary SPI. One cycle matches the
legacy test body:

1. `0x48`: power on, CS low.
2. `0x8A`: SPI 3.3 V output, clock idle low, active-to-idle edge, sample in the
   middle.
3. Write `0x00`, CS high, CS low.
4. Write `0xFF`, CS high, CS low.
5. Wait 1 ms.
6. `0x40`: power off.
7. `0x82`: SPI HiZ output, active-to-idle edge.

The Bus Pirate then leaves binary SPI and is reset after the requested cycles
complete successfully. If a cycle fails, the normal leave-mode/reset cleanup is
skipped so the original failure is not masked; reset the Bus Pirate before
reconnecting.

Options:

| Option | Meaning |
|---|---|
| `--repeat N` | Run N complete test cycles. Defaults to 1. |
| `--interactive` | Run cycles until the operator types `q`/`quit` at the prompt. Cannot be combined with `--repeat` greater than 1. |
| `--port`, `--speed` | Serial port and speed as above. |

A one-shot run emits one `0x00`/`0xFF` blink pair. Use `--repeat` when a longer
visible blink is needed. The test does not read a result from the adapter; it is
a manufacturing aid, so the operator must observe LED1 and the adapter outputs.
The test enables the Bus Pirate supply and 3.3 V SPI output; use it only with the
intended LCD adapter v2 hardware. SPI clock rate uses the firmware's binary SPI
default; this frontend adds no SPI speed option.

## Architecture and module boundaries

```text
scripts/bp/
  pyproject.toml        Packaging, console script `bp`
  requirements.txt      Runtime dependency pins
  requirements-dev.txt  Test/lint/type-check pins
  README.md             This document
  ruff.toml             Ruff lint/format policy
  pyrightconfig.json    Pyright policy
  pytest.ini            Pytest discovery and strictness
  .gitignore            Local virtualenv, caches, build output
  src/bp/
    __init__.py         Package marker
    __main__.py         `python -m bp`
    cli.py              Click group and BpError-to-ClickException mapping
    config.py           Pydantic YAML model, config loading, connection resolution
    errors.py           Domain exceptions and exit-worthy failures
    logging_config.py   stdlib logging setup for `--verbose`
    types.py            Frozen EEPROM/version/connection value objects
    py.typed            Inline typing marker
    transport.py        pyserial open/read/write/timeouts; no protocol logic
    protocol/
      __init__.py
      bbio.py           Binary-mode entry/framing, status checks, version banner
      i2c.py            Binary I2C primitives, EEPROM read/write/verify
      spi.py            Binary SPI primitives and LCD adapter v2 cycle
    commands/
      __init__.py       Shared Click-context accessor
      version.py        `version` command and transport-factory injection
      eeprom.py         `eeprom` group, text-hex files, EEPROM services
      lcd.py            `lcd-test` command and repeat/interactive loops
  tests/
    conftest.py         Scripted fake serial port and byte transport
    test_cli.py         CLI wiring, exit codes, configuration, failures
    test_config.py      YAML validation and connection precedence
    test_transport.py   Serial transport success/failure behavior
    test_types.py       Frozen EEPROM/connection value objects
    test_protocol/      Exact wire sequences and protocol rejections
    test_commands/      Text-hex services, version service, LCD behavior
```

Boundary rules:

- `transport.py` knows only bytes, serial timeouts, and pyserial failures. It
  has no Bus Pirate modes, commands, or output.
- `protocol/` knows Bus Pirate framing and wire sequences. It does not print,
  construct Click commands, or touch files.
- `commands/` owns file formats, user output, Click options, and orchestration.
  It never emits raw protocol bytes.
- `cli.py` only groups and wires commands plus shared error translation.
- Tests inject fake transports and fake protocol sessions; no hardware is
  required.

Extension contracts:

| Contract | Location | Operations |
|---|---|---|
| `Transport` | `src/bp/transport.py` | `write`, `read_exact`, `read_until_idle`, `reset_input`. Fake implementations must raise `TransportTimeout` when exact reads cannot be satisfied. |
| `EepromSession` | `src/bp/protocol/i2c.py` | `read_eeprom`, `write_eeprom`, `verify_eeprom`. The command services accept an injected session factory. |
| `LcdTestSession` | `src/bp/protocol/spi.py` | `run_cycle`. Tests replace the module-level `open_lcd_session` factory. |
| `open_eeprom_session`, `open_lcd_session` | `src/bp/protocol/` | Production factories that own `SerialTransport` plus the mode controller. |
| `VersionInfo`, `EepromLayout`, `ConnectionSettings` | `src/bp/types.py` | Frozen value objects validated at construction. |

These contracts exist because the commands are tested without hardware and
because the transport/protocol/command split is the package's extension point.

## Limitations and known issues

- **Hardware verification:** Partial. `bp version` was run against the BPv4.
  The EEPROM and LCD wire sequences were verified against the firmware sources,
  the legacy tools, and scripted fake transports, but no external EEPROM or LCD
  adapter was connected.
- **Version banner labels:** `bp version` matches the banner by labels and was
  hardware-verified on a BPv4 (`Bus Pirate v4`, v9.0 firmware, `DEVID:0x1019`).
  A firmware release that renames those labels or changes the URL scheme would
  require a parser update; the protocol list and optional v3 bootloader line are
  intentionally ignored.
- **One-shot LCD visibility:** A single cycle is a short 1 ms blink pair; use
  `--repeat` for a longer visible test.
- **No port discovery:** Ports must be passed explicitly or configured. The
  legacy `COM1`-style default exists only for `bp version`.
- **No EEPROM page writes:** Each byte uses an individual I2C transaction, so
  large writes are slower than page-programmed writers.
- **No XSVF or GUI:** Both are outside this phase.

## Failure behavior and exit codes

| Exit code | Meaning |
|---|---|
| `0` | Command completed successfully. |
| `1` | Runtime failure: bad port, timeout, protocol/NACK/status error, file I/O or malformed hex, verification mismatch, invalid configuration, or missing required port. |
| `2` | Click usage failure: missing required option, invalid range/choice, missing input file discovered by Click. |

Specific behavior:

- Opening a nonexistent or busy port reports `Could not open serial port ...`.
- A serial read that does not deliver the requested bytes reports the requested
  and received counts.
- Binary command status `0x00` and unexpected status bytes are rejected.
- EEPROM memory/read-address NACKs and write-attempt exhaustion are rejected
  with chip/memory-address context; writes also report the byte value.
- A verification mismatch exits 1 and reports the first differing offset.
- A failed operation does not attempt the normal "leave mode and reset" cleanup,
  so a transport or protocol error is not masked by a second failure. The bus,
  pull-ups, or adapter supply may remain in its current state; reset the Bus
  Pirate before reconnecting. A subsequent invocation sends the binary-mode
  entry sequence, but recovery from an arbitrary binary state is not guaranteed
  if the device was left mid-command.
- File contents are parsed and span-bounded before any serial session is
  opened. A read writes its output file only after the EEPROM transfer returns.

## Protocol notes

The binary protocol bytes follow the Bus Pirate v2.3+ binary I/O contract:

- Enter binary bitbang with twenty `0x00` bytes and require `BBIO1`.
- Enter I2C with `0x02` and require `I2C1`; configure power and pull-ups with
  `0x4C`.
- Enter SPI with `0x01` and require `SPI1`; CS low/high use `0x02`/`0x03`;
  single-byte transfer uses `0x10 <byte>`; SPI configuration uses `0x80 | cfg`.
- I2C start/stop/read/ACK/NACK use `0x02`/`0x03`/`0x04`/`0x06`/`0x07`; bulk
  writes use `0x10 | (count - 1)` followed by the bytes.
- Leave binary mode with `0x00` and require `BBIO1`; reset with `0x0F` and
  require status `0x01`.

## References

Authority and source material used by this package:

| Source | Purpose |
|---|---|
| [`scripts/version.pl`](../version.pl) | Legacy reset-banner behavior and platform default ports. |
| [`scripts/I2CEEPROMWIN.c`](../I2CEEPROMWIN.c) | Legacy EEPROM chip/addressing/file behavior and no-page writes. |
| [`scripts/powertools/LCDv2-tester/`](../powertools/LCDv2-tester/) | Legacy LCD adapter v2 test sequence. |
| [`Documentation/bitbang.md`](../../Documentation/bitbang.md) | Binary bitbang entry, identifiers, and reset. |
| [`Documentation/i2c.md`](../../Documentation/i2c.md) | I2C command semantics and ACK behavior. |
| [`Documentation/spi.md`](../../Documentation/spi.md) | SPI command semantics. |
| [`Firmware/binary_io.c`](../../Firmware/binary_io.c), [`Firmware/i2c.c`](../../Firmware/i2c.c), [`Firmware/spi.c`](../../Firmware/spi.c) | Exact status bytes, framing, and mode behavior used by the tests. |
| [`tests/`](tests/) | Executable contract for files, protocol sequences, and CLI exits. |

## Deviations and decisions

- **Standard-library logging:** This repository has no `simplelogger`/
  `get_component_logger` factory. The package uses stdlib `logging` and a
  `--verbose` root option for diagnostic logs; user-facing results still use
  Click, and expected failures use typed domain exceptions.
- **Requirements files:** Runtime pins live in `requirements.txt` and dev pins
  in `requirements-dev.txt`; `pyproject.toml` reads the runtime file through
  `[tool.setuptools.dynamic]`. No lockfile is added.
- **No pytest-asyncio:** The package has no asynchronous code, so `pytest.ini`
  omits `asyncio_mode` and the dev extra does not install `pytest-asyncio`.
- **Bounded EEPROM write retries:** The old tool retried forever. The new tool
  uses a fixed 100 attempts/5 ms policy for predictable failure, as described
  above. This is the intentional change to EEPROM write timing and retry
  behavior beyond the corrected one-byte read addressing.
- **Stricter text-hex input:** The legacy C parser used `sscanf("%X")` into a
  byte and could silently truncate malformed or oversized tokens. The new parser
  accepts only one or two hex digits, with an optional `0x` prefix, and rejects
  everything else before touching the port.
- **Last-byte NACK:** The legacy read path ACKed every byte and then clocked and
  discarded one extra byte so it could send a NACK. The new read path sends the
  NACK directly after the final requested byte.
- **Label-based version parsing:** The legacy Perl tool selected the last six
  CRLF-separated lines by position, which reports the protocol list/DEVID line
  as the hardware version on the v4 banner. The new parser selects the labelled
  hardware, firmware, DEVID and URL lines instead.
- **One-shot LCD scope:** One command cycle is one blink pair. It is not a
  continuously running loop like the Windows keyboard-driven utility; use
  `--repeat` or `--interactive` for repetition.
- **Quieter console output:** The legacy tools printed every read or written
  byte. The new commands print a final summary; the data itself is written to
  the requested text-hex file. Read output is newline-terminated rather than a
  bare trailing space. The legacy ANSI colors are not reproduced.
- **Clean-room implementation:** The package is CC0-1.0. The GPLv3
  `scripts/pyBusPirateLite/` tree was consulted only for behavioral reference;
  no source, comments, or structure were copied.

## Testing and verification

Run from `scripts/bp` with the local virtual environment active:

```bash
python -m pytest
ruff check .
ruff format --check .
pyright
```

Latest recorded results in this worktree:

```text
python -m pytest
146 passed

ruff check .
All checks passed!

ruff format --check .
28 files already formatted

pyright
0 errors, 0 warnings, 0 informations
```

The tests cover success and rejection paths for bad ports, timeouts, invalid
chip addresses, invalid address widths, truncated/empty hex, invalid UTF-8 in
config and hex files, file failures, protocol status/NACK errors, EEPROM retry
exhaustion, sequential address increments, verification mismatches, CLI exit
codes, configuration validation, and exact one-shot/repeat LCD command
sequences.

Hardware verification: **Partial.** `bp version --port COM6` was run against
the BPv4 and returned the expected hardware, firmware, DEVID and URL fields.
No external EEPROM or LCD adapter was connected for this work; the
EEPROM/LCD paths use scripted fake serial and transport implementations only.

Environment used for the recorded run:

- Python 3.13.7 (package requires 3.12 or newer; Pyright targets 3.12)
- click 8.5.1
- pydantic 2.14.4
- pyserial 3.5
- ruamel.yaml 0.19.1
- pytest 9.2.0
- ruff 0.17.6
- pyright 1.1.414

Python 3.12 itself was not executed in this worktree; compatibility is enforced
by `requires-python` and the Pyright target version, not by a 3.12 runtime run.

## License

The package is released under CC0-1.0, matching the Bus Pirate repository's
public-domain licensing for this tooling. See the repository `LICENSE` file for
the repository-wide note that some unrelated files have other licenses.
