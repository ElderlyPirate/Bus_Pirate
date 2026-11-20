# `bp` - Bus Pirate host frontend

`bp` is the console frontend for Bus Pirate v3/v4 host-side workflows. It
replaces the legacy `version.pl`, `I2CEEPROMWIN.c`, LCD adapter v2 test
utility, and the C XSVF host players with one typed, testable Python package.
It talks to the Bus Pirate binary protocol over a serial port and does not use
a GUI.

The old EEPROM/LCD tools remain in the repository. The C XSVF host folders
were removed after `bp xsvf` replaced them. SVF to XSVF conversion is not part
of this package; it remains an external Xilinx toolchain step.

## Scope

Implemented commands:

| Command | Purpose | Legacy equivalent |
|---|---|---|
| `bp version` | Read and print the firmware/hardware reset banner. | `scripts/version.pl` |
| `bp eeprom read` | Read an external I2C EEPROM into a text-hex file. | `scripts/I2CEEPROMWIN.c R/Rs` |
| `bp eeprom write` | Write a text-hex file to an external I2C EEPROM one byte at a time. | `scripts/I2CEEPROMWIN.c W/Ws` |
| `bp eeprom verify` | Compare an EEPROM span against a text-hex file. | New convenience |
| `bp lcd-test` | Run the LCD adapter v2 production LED/output test. | `scripts/powertools/LCDv2-tester` |
| `bp xsvf` | Stream an XSVF image, scan the JTAG chain, or reset the chain on a Bus Pirate v4. | `scripts/BPXSVFPlayer`, `scripts/BP4_XSVFplayer_Win` |

Legacy mode mapping: `R`/`W` use two-byte memory addressing
(`--address-width 2`); `Rs`/`Ws` use one-byte addressing
(`--address-width 1`).

Not implemented in this package:

- SVF to XSVF conversion. Use Xilinx iMPACT or the external `svf2xsvf502`
  converter, then pass the resulting XSVF file to `bp xsvf`.
- GUI or keyboard polling through Windows `conio`.
- Automatic discovery of Bus Pirate serial ports.
- Page writes for EEPROM devices. Writes always use one byte per I2C
  transaction, matching `I2CEEPROMWIN.c`, because EEPROM page sizes are not
  standardized.
- Changes to or removal of the remaining legacy EEPROM/LCD tools.

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

# Reset or probe the JTAG chain on a Bus Pirate v4
bp xsvf --port COM6 --reset
bp xsvf --port COM6 --scan

# Stream an XSVF image after converting SVF to XSVF externally
bp xsvf --port COM6 --file design.xsvf --reset
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
5. `bp eeprom`, `bp lcd-test`, and `bp xsvf` do not guess a port; they
   require `--port` or a configured `port`.

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

### `bp xsvf`

Streams an XSVF 5.01 image to the Bus Pirate v4 firmware player, probes the
JTAG chain, or resets the chain. It replaces both C host players and keeps
their option names, operation order, result names, and `0`/`1`/`2` exit
contract. It does not convert SVF; produce the XSVF first with Xilinx iMPACT
or the external `svf2xsvf502` tool.

| Option | Native flag | Meaning |
|---|---|---|
| `--port` | `-p` | Serial port. Required unless the configuration file supplies `port`. |
| `--file` | `-f` | XSVF image to stream. Optional only when `--scan` or `--reset` is also present. |
| `--speed` | `-s` | Serial speed. Defaults to 115200. Host-side only; it does not change the firmware player's TCK rate. |
| `--scan` | `-x` | Probe the JTAG chain and print its count byte and IDCODE bytes. |
| `--reset` | `-r` | Send the chain-reset command before scanning or streaming. |

Operation order matches the C players:

1. Read the whole XSVF file and reject unreadable or empty files before the
   serial port is opened.
2. Enter binary bitbang mode (`BBIO1`), then the XSVF player (`XSV1`).
3. Send the chain reset if `--reset` was given.
4. Run the chain scan if `--scan` was given.
5. Stream the file if `--file` was given.

`--reset` and `--scan` can run without a file. Giving none of `--file`,
`--scan`, or `--reset` is a usage error and exits 2. Click validates a missing
`--file` path before the command runs and exits 2.

File streaming:

- The image is sent in chunks of at most 4096 bytes, matching the firmware's
  chunk buffer. The last chunk is the remainder; an exact multiple of 4096
  sends no zero-length chunk.
- The player requests data with `0xFF`. Each answer is a 16-bit big-endian
  length followed by exactly that many XSVF bytes.
- After the final byte the host waits for the result byte. If the player asks
  for more data after the file is exhausted, the command reports
  `End of file reached without a completion result` and exits 2.
- Result `0x00` (`XSVF_ERROR_NONE`) prints `Success!` and exits 0. Any other
  result prints the firmware's symbolic name and exits 1. An unknown result
  byte is reported as `XSVF_ERROR_UNKNOWN`, matching the source builds.

Player result codes:

| Byte | Name | Meaning |
|---|---|---|
| `0x00` | `XSVF_ERROR_NONE` | Stream completed. |
| `0x01` | `XSVF_ERROR_UNKNOWN` | Host stopped feeding, sent data during a wait, or the player hit an unknown condition. |
| `0x02` | `XSVF_ERROR_TDOMISMATCH` | Captured TDO did not match the expected TDO. |
| `0x03` | `XSVF_ERROR_MAXRETRIES` | TDO mismatched after the maximum retries. |
| `0x04` | `XSVF_ERROR_ILLEGALCMD` | Illegal or unsupported XSVF command. |
| `0x05` | `XSVF_ERROR_ILLEGALSTATE` | Illegal TAP state in the stream. |
| `0x06` | `XSVF_ERROR_DATAOVERFLOW` | A shift exceeds the player's 400-bit buffer. |
| `0x07` | `XSVF_ERROR_LAST` | Internal last-error marker; not expected in normal runs. |

Chain scan reply:

- One count byte equal to four times the number of devices, then four IDCODE
  bytes per device. Bytes are least-significant first and the device nearest
  TDO is first.
- Example: `Chain Scan Result: 04 93 40 60 59` is one device; the IDCODE is
  `0x59604093`.
- No target can still produce a valid reply: `00` with no payload (TDO stuck
  high) or `FC` followed by 252 zero bytes (TDO stuck low and the 63-device
  scan clamp). Both exit 0; they are wiring observations, not stream errors.
  A missing count or payload exits 2.

Exit state after a successful operation:

- `bp xsvf` intentionally leaves the Bus Pirate in the firmware player, as
  the C players did. The regulator stays on, so the target rails remain
  powered. The serial port is closed, but the terminal prompt does not return.
- Press reset before the next run, or send `0x0F` twice from a raw serial host
  once the player command loop is reachable. The first `0x0F` resets the chain
  and returns to `BBIO1` silently; the second runs the terminal reset and
  replies `0x01`.
- Do not send `0x0F` while a run is still in progress; it can be consumed as
  stream data. A command that stopped at `End of file reached without a
  completion result` should be left for the firmware's roughly five-second
  stall before the `0x0F` pair is attempted. A command that timed out waiting
  for the player may have left it still running; wait, check the target, then
  press reset.

Firmware requirements and limits:

- Bus Pirate v4 only. On v3 the `0x18` byte is answered with `0x00`; the
  command reports that XSVF requires a v4 with player firmware and exits 2.
- Longest single DR or IR shift is 400 bits (50 bytes). Larger streams return
  `XSVF_ERROR_DATAOVERFLOW`.
- The chunk limit is 4096 bytes; the chain scan is clamped to 63 devices.
- The player drives 3.3 V push-pull outputs, has no open-drain mode, and
  switches the on-board regulator on at entry. A 1.8 V or 2.5 V target cannot
  use this path. Connect the target's VCC to one rail only, never both, and
  disconnect any external supply first.
- There is no host TCK setting.

SVF to XSVF conversion is an external prerequisite. The removed C folders
also carried Xilinx's platform-specific `svf2xsvf502` and `mergexsvf`
utilities; this package does not port or replace them.

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
    cli.py              Click group and BpError-to-ClickException mapping with exit codes
    config.py           Pydantic YAML model, config loading, connection resolution
    errors.py           Domain exceptions, XSVF error classes, exit-code contract
    logging_config.py   stdlib logging setup for `--verbose`
    types.py            Frozen EEPROM/version/connection/mode value objects
    py.typed            Inline typing marker
    transport.py        pyserial open/read/write/timeouts; no protocol logic
    protocol/
      __init__.py
      bbio.py           Binary-mode entry/framing, status checks, version banner
      i2c.py            Binary I2C primitives, EEPROM read/write/verify
      spi.py            Binary SPI primitives and LCD adapter v2 cycle
      xsvf.py           XSVF entry, chain reset/scan, chunk handshake, result mapping
    commands/
      __init__.py       Shared Click-context accessor
      version.py        `version` command and transport-factory injection
      eeprom.py         `eeprom` group, text-hex files, EEPROM services
      lcd.py            `lcd-test` command and repeat/interactive loops
      xsvf.py           `xsvf` command, file reads, orchestration, output
  tests/
    conftest.py         Scripted fake serial port and byte transport
    test_cli.py         CLI wiring, exit codes, configuration, failures
    test_config.py      YAML validation and connection precedence
    test_transport.py   Serial transport success/failure behavior
    test_types.py       Frozen EEPROM/connection/mode value objects
    test_protocol/      Exact wire sequences, XSVF chunks/results, protocol rejections
    test_commands/      Text-hex services, version service, LCD/XSVF behavior
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
| `XsvfSession` | `src/bp/protocol/xsvf.py` | `reset_chain`, `scan_chain`, `play`. The command service accepts an injected session factory. |
| `open_eeprom_session`, `open_lcd_session`, `open_xsvf_session` | `src/bp/protocol/` | Production factories that own `SerialTransport` plus the mode controller. |
| `VersionInfo`, `EepromLayout`, `ConnectionSettings` | `src/bp/types.py` | Frozen value objects validated at construction. |
| `ChainScan`, `XsvfPlayReport` | `src/bp/protocol/xsvf.py` | Frozen XSVF scan/result reports, including raw count+IDCODE bytes and decoded IDCODEs. |

These contracts exist because the commands are tested without hardware and
because the transport/protocol/command split is the package's extension point.

## Limitations and known issues

- **Hardware verification:** Partial. `bp version`, `bp xsvf --reset`, and
  `bp xsvf --scan` were run against the BPv4 on COM6. The EEPROM and LCD wire
  sequences were verified against the firmware sources, the legacy tools, and
  scripted fake transports, but no external EEPROM or LCD adapter was
  connected. No XSVF target/CPLD was connected, so no XSVF file was played and
  target programming was not attempted.
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
- **No GUI:** GUI and `conio` keyboard polling are outside this package.
- **XSVF target may stay powered:** `bp xsvf` intentionally leaves the
  firmware player running, and the player leaves the regulator on. Press reset
  before reconnecting to the terminal. See the `bp xsvf` section for the exact
  exit state.
- **XSVF device limits:** The firmware player supports a longest single shift
  of 400 bits, a 4096-byte chunk, a 63-device scan clamp, 3.3 V push-pull
  signalling, and no host TCK setting. Streams outside those limits fail with
  the documented result or are unusable. These limits are code-derived; only
  the no-target scan and reset paths were exercised on hardware.
- **SVF conversion is external:** No SVF parser or Xilinx converter is
  implemented. A `.svf` file cannot be passed to `bp xsvf`.

## Failure behavior and exit codes

| Exit code | Meaning |
|---|---|
| `0` | Command completed successfully, including an XSVF scan/reset or an `XSVF_ERROR_NONE` stream result. |
| `1` | Runtime failure for `version`/`eeprom`/`lcd-test`, or the XSVF player returned any non-success result byte. |
| `2` | Click usage failure, or an XSVF host/transport/setup failure: bad or empty file, bad port, invalid configuration, wrong `BBIO1`/`XSV1` banner, missing reply/result, incomplete scan reply, or missing required port. |

Specific behavior:

- Opening a nonexistent or busy port reports `Could not open serial port ...`.
  For `bp xsvf`, transport and setup failures exit 2; for the other commands
  they retain exit 1.
- A serial read that does not deliver the requested bytes reports the requested
  and received counts.
- Binary command status `0x00` and unexpected status bytes are rejected.
- EEPROM memory/read-address NACKs and write-attempt exhaustion are rejected
  with chip/memory-address context; writes also report the byte value.
- A verification mismatch exits 1 and reports the first differing offset.
- `bp xsvf` rejects a missing or empty XSVF file before opening the port. It
  exits 2 for a wrong banner, v3/no-player response, scan timeout, missing data
  request, missing result, or EOF before completion. A non-success player
  result exits 1 and prints its `XSVF_ERROR_*` name.
- A failed EEPROM/LCD operation does not attempt the normal "leave mode and
  reset" cleanup, so a transport or protocol error is not masked by a second
  failure. The bus, pull-ups, or adapter supply may remain in its current
  state; reset the Bus Pirate before reconnecting.
- `bp xsvf` intentionally performs no leave-mode cleanup, even on success. The
  board remains in the firmware player; recovery requires reset or the raw
  `0x0F` pair described in the command section. A new invocation cannot re-enter
  binary mode until the board is out of the player.
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
- Enter XSVF with `0x18` and require `XSV1`; v4 firmware only.
- XSVF chain reset uses `0x01` and has no reply. Chain scan uses `0x02` and
  returns a one-byte count followed by that many IDCODE bytes.
- XSVF playback starts with `0x03`. The player sends `0xFF` whenever it needs
  a chunk; the host answers with a 16-bit big-endian length and that many
  bytes. Chunks are at most 4096 bytes. The run ends with one result byte.
- Leaving the XSVF player uses `0x0F` (chain reset, silent return to `BBIO1`);
  a second `0x0F` returns to the terminal and replies `0x01`. `bp xsvf` does
  not send this pair because native host behavior leaves the player.
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
| [`Firmware/jtag.c`](../../Firmware/jtag.c), [`Firmware/jtag/ports.c`](../../Firmware/jtag/ports.c), [`Firmware/jtag/micro.c`](../../Firmware/jtag/micro.c) | XSVF player commands, chunk buffer, XSVF 5.01 command set, and result bytes. |
| `docs/combined-sweep:docs/protocol/jtag/xsvf.md`, `docs/combined-sweep:docs/tools/xsvf/*` | Authoritative XSVF protocol, host options, native output, limits, and exit semantics used to build the replacement. These pages live on the docs branch, are not present here, and were not edited by this change. |
| [`tests/`](tests/) | Executable contract for files, protocol sequences, XSVF chunk/result handling, and CLI exits. |

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
- **XSVF C tools removed:** `scripts/BPXSVFPlayer/` and
  `scripts/BP4_XSVFplayer_Win/` were deleted after `bp xsvf` passed its tests.
  The bundled Xilinx `SVF2XSVF` binaries lived inside those folders and were
  not ported; conversion is now documented as an external prerequisite.
- **XSVF native exit contract:** `bp xsvf` returns 0 for success/scan/reset,
  1 for a non-success player result, and 2 for host, transport, or setup
  failures. The other commands keep the original 0/1/2 behavior described in
  their sections.
- **No XSVF leave-mode cleanup:** The C players closed the port without
  sending `0x0F`, leaving the board in the player. `bp xsvf` preserves that
  behavior because the player exit is not available on all v4 firmware and a
  `0x0F` sent during a run is consumed as data. The README documents the reset
  and raw `0x0F` recovery paths.
- **XSVF file validation:** The legacy C player did not fail cleanly on a
  missing or empty file. `bp xsvf` rejects both before opening the serial port
  and exits 2, matching the source-build behavior documented for the C tools.
- **XSVF output:** The Python command prints the file size, reset and scan
  summaries, one final `Sent ... bytes in ... chunk(s)` line, the
  `End of operation reply`, and `Success!` instead of one progress line per
  chunk. It keeps the native result names and the scan byte display.
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
208 passed

ruff check .
All checks passed!

ruff format --check .
32 files already formatted

pyright
0 errors, 0 warnings, 0 informations
```

The tests cover success and rejection paths for bad ports, timeouts, invalid
chip addresses, invalid address widths, truncated/empty hex, invalid UTF-8 in
config and hex files, file failures, protocol status/NACK errors, EEPROM retry
exhaustion, sequential address increments, verification mismatches, CLI exit
codes, configuration validation, and exact one-shot/repeat LCD command
sequences.

XSVF coverage includes exact `BBIO1`/`XSV1` entry, reset and scan command
sequences, scan payload reads and truncation, invalid count bytes, chunk
boundaries at 3, 4096, 4097, and 8195 bytes, data-request resend, EOF before a
result, every result byte including unknown values, success, missing
data/result replies, wrong banners, v3/no-player responses, empty/missing
files, transport write failures, and CLI exit codes 0, 1, and 2.

Hardware verification: **Partial.** On the BPv4 at COM6 (firmware v9.0):

- `bp version --port COM6` returned `Bus Pirate v4`,
  `ElderlyPirate Firmware v9.0 - goo.gl/gCzQnW - 14256fd5`,
  `DEVID:0x1019 REVID:0x0004 (24FJ256GB106 A6)`, and the ElderlyPirate URL.
- `bp xsvf --reset --port COM6` exited 0 and printed
  `Performing Reset.. Done`.
- `bp xsvf --scan --port COM6` exited 0 and returned
  `Chain Scan Result: FC` followed by 252 zero bytes. That is the documented
  TDO-stuck-low/no-target result, including the 63-device scan clamp; no target
  or CPLD was connected.
- After each safe command the board remained in the firmware player, as
  documented. A raw `0x0F`, `0x0F` pair returned `0x01` and the terminal
  banner, restoring the prompt.

No XSVF file was played: that would attempt to program an absent target. No
target programming was claimed or attempted. The EEPROM and LCD paths use
scripted fake serial and transport implementations only; no external EEPROM or
LCD adapter was connected.

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

## Documentation impact

The XSVF documentation on the `docs/combined-sweep` branch still describes the
removed C host tools. This branch intentionally does not edit `docs/`, so the
following updates are required when the branches are combined:

| Document | Required change |
|---|---|
| `docs/tools/xsvf/README.md` | Replace the host-player file map and "Get the tools" folder links with `bp xsvf`; state that SVF conversion remains external. |
| `docs/tools/xsvf/players.md` | Rewrite the Windows/cross-platform build comparison, prebuilt-binary section, options, output, and exit status around `bp xsvf`; remove references to `scripts/BPXSVFPlayer/`, `scripts/BP4_XSVFplayer_Win/`, and `scripts/tests/test_xsvf_players.py`. |
| `docs/tools/xsvf/workflow.md` | Replace the C command examples and file paths with `bp xsvf`; keep the wiring, firmware-player, and recovery material. |
| `docs/tools/xsvf/limits-and-errors.md` | Replace C host messages and "legacy prebuilt" notes with the Python messages and exit contract; keep the firmware limits and result actions. |
| `docs/protocol/jtag/xsvf.md` | Point the host workflow at `bp xsvf`; the in-mode protocol, result codes, and firmware limits are unchanged. |
| `docs/tools/xsvf/converter.md` | Keep the external Xilinx conversion material, but remove folder paths that pointed inside the deleted C trees. |
| `docs/tools/xsvf/player.md` | Firmware-side content remains valid; check any host-tool cross-references. |

The current-branch `Documentation/jtag.md` also marks the XSVF player as a
deprecated TODO. That note is stale for the v4 firmware player and is outside
this package's edit scope.

## License

The package is released under CC0-1.0, matching the Bus Pirate repository's
public-domain licensing for this tooling. See the repository `LICENSE` file for
the repository-wide note that some unrelated files have other licenses.
