# `bp` - Bus Pirate host frontend

`bp` is the console frontend for Bus Pirate v3/v4 host-side workflows. It
replaces the legacy `version.pl`, `I2CEEPROMWIN.c`, LCD adapter v2 test
utility, the C XSVF host players, and the GPLv3 `scripts/pyBusPirateLite/`
library with one typed, testable Python package. It talks to the Bus Pirate
binary protocol over a serial port and does not use a GUI.

All legacy host trees were removed after their functionality was ported; only
the `bp` package remains under `scripts/`.  SVF to XSVF conversion is not part
of this package; it remains an external Xilinx toolchain step.

## Scope

Implemented commands:

| Command | Purpose | Legacy equivalent |
|---|---|---|
| `bp version` | Read and print the firmware/hardware reset banner. | `scripts/version.pl` |
| `bp ports` | List serial ports with VID:PID, description, and likely Bus Pirate markers. | New convenience |
| `bp bbio` | Binary-bitbang entry, pins, self-test, ADC, PWM, and raw bit clocks. | `pyBusPirateLite.BitBang`, Qt GUI Power tab |
| `bp raw-wire` | Raw 2-wire/3-wire configuration, bytes, bits, CS, and clocks. | `pyBusPirateLite.RAW_WIRE`, `rawwire` |
| `bp i2c` | I2C scan, register get/set, commands, and block transfers. | `pyBusPirateLite.I2C`, `I2Chigh`, `i2c-test.py` |
| `bp spi` | SPI configuration, JEDEC chip ID, transfers, and flash-style read/write. | `pyBusPirateLite.SPI`, `spi_test.py`, Qt GUI SPI tab |
| `bp uart` | UART configuration, manual BRG, send, receive, and bridge setup. | `pyBusPirateLite.UART`, `noritake-bitmap-display.py` |
| `bp onewire` | 1-Wire reset, ROM search/read/match/skip, scratchpad, memory, and status. | `pyBusPirateLite._1WIRE` |
| `bp script` / `bp raw-script` | Send an ASCII-hex file through the binary command loop. | Qt GUI Raw Text tab, `BinMode::command` |
| `bp eeprom read` | Read an external I2C EEPROM into a text-hex file. | `scripts/I2CEEPROMWIN.c R/Rs` |
| `bp eeprom write` | Write a text-hex file to an external I2C EEPROM one byte at a time. | `scripts/I2CEEPROMWIN.c W/Ws` |
| `bp eeprom verify` | Compare an EEPROM span against a text-hex file. | New convenience |
| `bp lcd-test` | Run the LCD adapter v2 production LED/output test. | `scripts/powertools/LCDv2-tester` |
| `bp xsvf` | Stream an XSVF image, scan the JTAG chain, or reset the chain on a Bus Pirate v4. | `scripts/BPXSVFPlayer`, `scripts/BP4_XSVFplayer_Win` |

Legacy mode mapping: `R`/`W` use two-byte memory addressing
(`--address-width 2`); `Rs`/`Ws` use one-byte addressing
(`--address-width 1`).

The new mode groups cover the `pyBusPirateLite` modes and useful example
workflows. They expose the same wire operations as a library
(`bp.protocol.*`) and as one-shot CLI commands. No GUI and no OpenOCD wrapper
are included.

Not implemented in this package:

- SVF to XSVF conversion. Use Xilinx iMPACT or the external `svf2xsvf502`
  converter, then pass the resulting XSVF file to `bp xsvf`.
- GUI or keyboard polling through Windows `conio`.
- Automatic port selection. `bp ports` lists and marks candidate devices, but no
  command chooses or opens a port from that list.
- Page writes for EEPROM devices. Writes always use one byte per I2C
  transaction, matching the removed legacy tool, because EEPROM page sizes are
  not standardized.
- Binary-bitbang clock-speed set/read (`0x60`/`0x70`). The v9 firmware in this
  tree does not implement either command in binary-bitbang mode; use the
  per-mode `--bus-speed` options instead.
- SPI configuration read-back (`0x90`) and the SPI nibble commands
  (`0x20`/`0x30`). The v9 binary SPI handler accepts `0x80 | cfg` but does not
  implement those command paths.
- UART configuration read-back (`0xD0`). The v9 binary UART handler has no
  read-config command; `bp uart config` reports the values it just applied.
- A dedicated Microwire EEPROM workflow. The removed `MicroWire.py` example
  used raw-wire primitives; use `bp raw-wire` or `bp.protocol.raw_wire`
  directly.
- A host-side UART bridge that returns cleanly. `bp uart bridge` enters the
  firmware bridge, which only a hardware reset exits.

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

# List serial ports and mark likely Bus Pirate devices (no connection)
bp ports

# Enter binary bitbang, read pins, and reset back to the terminal
bp bbio enter --port COM6
bp bbio pins --port COM6
bp bbio reset --port COM6

# Raw 3-wire configuration and a byte transfer
bp raw-wire config --wires 3 --output normal --bus-speed 50k --port COM6
bp raw-wire write --wires 3 --data "9F 00 00 00" --port COM6

# Scan I2C addresses, then read/write register 0x10 on a device at 0x50
# (Click integer options are decimal: 80 = 0x50, 16 = 0x10, 165 = 0xA5)
bp i2c scan --port COM6
bp i2c get-byte --address 80 --register 16 --port COM6
bp i2c set-byte --address 80 --register 16 --value 165 --port COM6

# SPI JEDEC ID and a flash-style read (opcode 11 = 0x0B)
bp spi chip-id --port COM6
bp spi read --opcode 11 --address 0 --length 256 --file flash.bin --port COM6

# UART configuration and a short send
bp uart config --baud 115200 --port COM6
bp uart send --baud 115200 --data "48 49 0A" --port COM6

# 1-Wire discovery and scratchpad read
bp onewire scan --port COM6
bp onewire read-scratchpad --length 9 --port COM6

# Run an ASCII-hex file through the binary command loop
bp script --file script.txt --port COM6

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

# Scan and return the Bus Pirate to its terminal prompt afterwards
bp xsvf --port COM6 --scan --leave

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

Click integer options such as `--address`, `--register`, `--value`,
`--direction`, `--state`, `--opcode`, and `--length` are decimal. Hex is used
for `--data`/`--file` byte payloads, 1-Wire `--rom` values, and printed
protocol data. Convert a desired hex value to decimal in the shell or config
file (for example `0x50` -> `80`).

### `bp ports`

Lists the serial ports pyserial reports, without opening a port or selecting one
for another command. The command is useful before `bp version`, `bp eeprom`, or
`bp xsvf` when the device name is unknown.

```text
COM1  ----:----  Communications Port (COM1)
COM6  04D8:FB00  USB Serial Device (COM6)  [Bus Pirate v4 application]
COM9  1209:0002  USB Serial Device (COM9)
```

Each line contains the device name, `VID:PID` when pyserial reports USB IDs (or
`----:----` when it does not), the description, and a bracketed marker for known
Bus Pirate IDs:

| VID:PID | Marker | Meaning |
|---|---|---|
| `04D8:FB00` | `[Bus Pirate v4 application]` | Bus Pirate v4 running its application firmware. |
| `04D8:FAFF` | `[Bus Pirate v4 bootloader]` | Bus Pirate v4 in bootloader mode. |
| `0403:6001` | `[FTDI (Bus Pirate v3 possible)]` | FTDI USB-serial bridge; a v3 may use it, but the ID is not unique to Bus Pirate. |

No ports produces `No serial ports found.` and exits 0. Port enumeration happens
through `serial.tools.list_ports`; the external port objects are validated once
at that boundary (device and description must be strings, USB IDs must be
integers in `0..0xFFFF` and must appear as a pair). A malformed entry or an
enumeration failure reports the problem and exits 1. `bp ports` never connects,
never resets the device, and never changes which port another command uses.

Options: none.

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

### `bp bbio`

Binary-bitbang entry, pin control, self-test, one-shot ADC, PWM, and raw
bit-banged bytes. Shared options: `--port`, `--speed` (host serial speed).

| Command | Options | Behavior |
|---|---|---|
| `bp bbio enter` | connection | Send the binary entry sequence and leave the Bus Pirate in `BBIO1`. |
| `bp bbio reset` | connection | Enter bitbang, send `0x0F`, and require the `0x01` acknowledgement. Returns the terminal prompt. |
| `bp bbio version` | connection | Send `0x00` and require the `BBIO1` mode identifier. |
| `bp bbio pins` | `--direction 0..31`, `--state 0..127`, `--power`, `--pullups`, `--aux`, `--cs` | Optionally apply a direction mask (set bits are inputs) and state mask (set bits are high; power `0x40`, pull-ups `0x20`, AUX `0x10`, CS `0x01`), then read and print the pin state. |
| `bp bbio selftest` | `--long`, connection | Run the short or full firmware self-test, send `0xFF` to leave its loop, and print the error count. A non-zero count exits 1. |
| `bp bbio adc` | connection | Run one ADC sample (`0x14`) and print the raw 12-bit value. |
| `bp bbio pwm` | `--prescaler 0..3`, `--duty 0..65535`, `--period 0..65535`; or `--clear` | Configure or release the AUX PWM output. All three setup parameters are required. |
| `bp bbio write` | `--data HEX...` or `--file TEXT-HEX`, `--lsb-first`, `--power`, `--pullups` | Clock bytes out on MOSI with CLK pulses. |
| `bp bbio read` | `--count N`, `--lsb-first`, `--power`, `--pullups` | Clock bytes in from MISO and print them as hex. |

All commands except `enter` reset to the user terminal after success. `pins`,
`write`, and `read` drive only the Bus Pirate pins; no target is required.
Binary-bitbang speed set/read is not available in the v9 firmware. Use the
per-mode bus-speed options.

### `bp raw-wire`

Binary raw 2-wire and 3-wire mode. Shared options: `--output
normal|open-drain`, `--wires 2|3`, `--lsb-first`, `--bus-speed
5k|50k|100k|400k`, `--power`, `--pullups`, `--port`, `--speed`.

| Command | Additional options | Behavior |
|---|---|---|
| `bp raw-wire config` | shared | Enter raw-wire, apply output type, wire count, bit order, speed, and power/pull-ups, then reset. |
| `bp raw-wire write` | `--data`/`--file`, `--cs/--no-cs`, `--start`, `--stop` | Write bytes; default CS assertion is on for 3-wire and off for 2-wire. `--start`/`--stop` add 2-wire conditions. |
| `bp raw-wire read` | `--count N`, `--cs/--no-cs`, `--start`, `--stop` | Read bytes and print hex. |
| `bp raw-wire bit` | shared | Read one bit with clock (`0x07`). |
| `bp raw-wire peek` | shared | Read the input pin without clocking (`0x08`). |
| `bp raw-wire clock` | `--ticks 1..16` | Emit one through sixteen clock ticks. |

Two-wire writes use the firmware bulk byte command and require its per-byte
success status. Three-wire writes clock each bit explicitly with data/clock
commands because the firmware's three-wire bulk path returns no per-byte
status and re-clocks the received value; this is slower but deterministic.

### `bp i2c`

Binary I2C scan, register helpers, commands, and block transfers. Shared
options: `--bus-speed 5k|50k|100k|400k` (default `100k`), `--power/--no-power`
(default off), `--pullups/--no-pullups` (default on), `--port`, `--speed`.

| Command | Options | Behavior |
|---|---|---|
| `bp i2c scan` | `--start 0..127`, `--end 0..127` | Probe each 7-bit address for write and read acknowledgement and print every hit with its 8-bit write/read addresses. |
| `bp i2c get-byte` | `--address 0..127`, `--register 0..255` | Repeated-start register read; prints one byte. |
| `bp i2c set-byte` | `--address`, `--register`, `--value 0..255` | Start/address/register/data/stop write. |
| `bp i2c get-word` | `--address`, `--register` | Read a big-endian 16-bit register value. |
| `bp i2c set-word` | `--address`, `--register`, `--value 0..65535` | Write a big-endian 16-bit register value. |
| `bp i2c command` | `--address`, `--command 0..255` | Write one command byte to the device. |
| `bp i2c read` | `--address`, optional `--register`, `--length`, optional `--file` | Read sequential bytes; writes a text-hex file or prints hex. |
| `bp i2c write` | `--address`, optional `--register`, `--data`/`--file` | Write sequential bytes, splitting at the 16-byte bulk limit. |

A NACK on a register/read address or on any data byte raises
`I2cNackError` and exits 1 with the 7-bit address and operation. The flashed
BPv4 v9.0 firmware answers binary I2C start, stop, ACK, and NACK with `0x00`
where the protocol reference specifies `0x01`; the controller accepts either
reply for those four bus-control commands. Data-phase write status and
per-byte ACK bits remain strict.

### `bp spi`

Binary SPI configuration, JEDEC chip ID, raw transfers, and flash-style
read/write. Shared options: `--bus-speed` (default `1m`), `--clock-polarity
idle-low|idle-high`, `--clock-edge idle-to-active|active-to-idle` (default
active-to-idle), `--sample middle|end`, `--output normal|open-drain`,
`--power/--no-power` (default off), `--pullups/--no-pullups` (default off),
`--port`, `--speed`.

SPI speed codes in firmware order: `30k`, `125k`, `250k`, `1m`, `2m`, `2.6m`,
`4m`, `8m`, `50k`, `1.3m`, `3.2m`, `5.3m`.

| Command | Options | Behavior |
|---|---|---|
| `bp spi config` | shared | Enter SPI, apply speed/mode/power, then reset. |
| `bp spi chip-id` | shared | Assert CS, transfer `9F FF FF FF`, release CS, and print the four response bytes plus the three-byte JEDEC ID. |
| `bp spi transfer` | `--data`/`--file`, shared | Assert CS, transfer bytes, release CS, and print the received bytes. |
| `bp spi read` | `--opcode 0..255`, `--address 0..0xFFFFFFFF`, `--address-width 0..4` (default 3), `--dummy-bytes 0..16`, `--length`, `--file BINARY`, shared | Send opcode + big-endian address + dummy zero bytes, read `--length` bytes with `0xFF`, and write a binary file. |
| `bp spi write` | `--opcode` (optional), `--address`, `--address-width`, `--file BINARY`, shared | Send an optional opcode/address prefix followed by the file contents. |

The read workflow matches the old `spi_test.py` `-r`/`-i` shape: CS asserted
for the whole opcode/address/data sequence, one instruction prefix, then
dummy-byte reads. `--address-width 0` sends no address; `--dummy-bytes` covers
fast-read commands that need one or more post-address dummy clocks. The old
`-w` write example was a stub and `-e` erase was a `pass`; `bp spi write`
therefore implements the useful generic write path rather than replicating
those stubs.

### `bp uart`

Binary UART configuration, send, receive, and firmware bridge setup. Shared
options: `--baud` (default `9600`), `--data-bits-parity 8n|8e|8o|9n`,
`--stop-bits 1|2`, `--polarity normal|inverted`, `--output
normal|open-drain`, `--power/--no-power` (default off),
`--pullups/--no-pullups` (default off), `--port`, `--speed`.

Firmware baud codes: `300`, `1200`, `2400`, `4800`, `9600`, `19200`, `38400`,
`57600`, `115200`, `31250`.

| Command | Options | Behavior |
|---|---|---|
| `bp uart config` | shared | Enter UART, apply baud/frame/output, then reset. |
| `bp uart manual-speed` | `--brg 0..65535`, frame/output options | Enter UART and program a raw 16-bit BRG, then reset. The old helper formula was `BRG = (FOSC / (4 * baud)) - 1` with `FOSC = 16 MHz`. |
| `bp uart send` | `--data`/`--file`, shared | Send hex bytes in 16-byte chunks; each byte is acknowledged by the firmware. |
| `bp uart receive` | `--length N`, optional `--file TEXT-HEX`, `--timeout SECONDS` | Enable the firmware receive echo, read `N` bytes, disable echo, and print/write them. The default serial read timeout is 5 s. |
| `bp uart bridge` | `--yes`, shared | Enter the firmware transparent bridge. It only exits on a hardware reset; `--yes` is required. The command prints a warning and leaves the bridge running. |

Receive echo is the firmware `0x02`/`0x03` mode. If a read times out, the
controller does not attempt the disable-echo command, so the session reports
the timeout directly; reset the Bus Pirate before the next attempt.

### `bp onewire`

Binary 1-Wire reset, ROM search/read/match/skip, scratchpad, memory, and status
operations. Shared options: `--bus-speed standard|overdrive` (default
standard), `--power/--no-power` (default off), `--pullups/--no-pullups`
(default on), `--port`, `--speed`.

ROM arguments accept eight bytes as 16 hex digits, with optional `:`, `-`, or
space separators. Omit `--rom` to use SKIP ROM for single-device buses.

| Command | Options | Behavior |
|---|---|---|
| `bp onewire reset` | shared | 1-Wire bus reset. |
| `bp onewire scan` | shared | ROM search; prints each ROM, family code, and CRC status. |
| `bp onewire alarm-scan` | shared | Alarm-search macro. |
| `bp onewire read-rom` | shared | Read ROM from a single device. |
| `bp onewire read-scratchpad` | `--rom`, `--length` (default 9), optional `--file` | Reset, match/skip, send `0xBE`, read the scratchpad. |
| `bp onewire write-scratchpad` | `--rom`, `--data`/`--file` | Reset, match/skip, send `0x4E` plus data. |
| `bp onewire copy-scratchpad` | `--rom` | Reset, match/skip, send `0x48`. |
| `bp onewire read-memory` | `--rom`, `--address 0..65535`, `--length`, optional `--file` | Reset, match/skip, send `0xF0` plus a big-endian address, read bytes (DS2431-style). |
| `bp onewire read-status` | `--rom`, `--address` | Reset, match/skip, send `0xAA` plus address, read one status byte. |
| `bp onewire send` | `--rom` or `--skip-rom`, `--data`/`--file` | Reset, select a device, and send arbitrary command bytes. |

The search parser reads eight-byte ROM groups until the firmware sends eight
`0xFF` bytes. Search is limited to 64 devices per call; a missing terminator
raises a protocol error. ROM objects expose family, serial, CRC, and a
Dallas/Maxim CRC-8 validity result.

### `bp script` / `bp raw-script`

Send a whitespace-separated ASCII-hex file through the binary command loop,
matching the Qt GUI Raw Text tab. Each token is one or two hex digits with an
optional `0x` prefix. The command enters binary bitbang first, then sends each
byte as one command and prints the reply bytes before the per-byte serial
timeout elapses.

| Option | Meaning |
|---|---|
| `--file` | ASCII-hex file. Required. |
| `--reply-timeout` | Per-command reply wait in seconds. Default 0.1. |
| `--max-reply-bytes` | Maximum reply bytes printed for one command. Default 256. |
| `--reset` | Send the terminal reset after the script. Use only when the script left the Bus Pirate in binary bitbang. |
| `--port`, `--speed` | Serial connection. |

Example output:

```text
001 | raw bytes text: 00 tx:   0 rx: 42 42 49 4F 31
002 | raw bytes text: 0A tx:  10 rx: (none)
Exec: ASCII Script...Success! (2 bytes)
```

The command does not reset by default because script bytes can enter any binary
submode; an unconditional `0x0F` would be interpreted by that mode. Add
`--reset` only when the script stays in BBIO.

### `bp xsvf`

Streams an XSVF 5.01 image to the Bus Pirate v4 firmware player, probes the
JTAG chain, or resets the chain. It replaces both C host players and keeps
their option names, reset/scan/play order, result names, and `0`/`1`/`2` exit
contract. It does not convert SVF; produce the XSVF first with Xilinx iMPACT
or the external `svf2xsvf502` tool.

| Option | Native flag | Meaning |
|---|---|---|
| `--port` | `-p` | Serial port. Required unless the configuration file supplies `port`. |
| `--file` | `-f` | XSVF image to stream. Optional only when `--scan` or `--reset` is also present. |
| `--speed` | `-s` | Serial speed. Defaults to 115200. Host-side only; it does not change the firmware player's TCK rate. |
| `--scan` | `-x` | Probe the JTAG chain and print its count byte and IDCODE bytes. |
| `--reset` | `-r` | Send the chain-reset command before scanning or streaming. |
| `--leave` | none | After a successful operation, return the board to the user terminal by sending `0x0F` twice. Off by default. |

Operation order for the reset, scan, and stream steps matches the C players:

1. Read the whole XSVF file and reject unreadable or empty files before the
   serial port is opened.
2. Enter binary bitbang mode (`BBIO1`), then the XSVF player (`XSV1`).
3. Send the chain reset if `--reset` was given.
4. Run the chain scan if `--scan` was given.
5. Stream the file if `--file` was given.
6. If `--leave` was given, return the board to the terminal.

`--reset` and `--scan` can run without a file. Giving none of `--file`,
`--scan`, or `--reset` is a usage error and exits 2. Click validates a missing
`--file` path before the command runs and exits 2.

`--leave` is opt-in because the default matches the native C host behavior:
successful runs leave the firmware player active. With `--leave`, the host sends
`0x0F` once to leave the player for `BBIO1` (no reply), sends `0x0F` again to run
the terminal reset, requires the `0x01` acknowledgement, then drains the terminal
banner. The banner text is not parsed or shown and is not an error if it is
partial or empty. Leaving happens only after every requested reset, scan, and
stream operation succeeds; a player result error (exit 1) or an earlier host
failure (exit 2) skips it. If either leave command, the acknowledgement, or the
banner drain fails, the command exits 2 with `Leaving XSVF player: ...`.

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

- By default, `bp xsvf` intentionally leaves the Bus Pirate in the firmware
  player, as the C players did. The regulator stays on, so the target rails
  remain powered. The serial port is closed, but the terminal prompt does not
  return.
- With `--leave`, a successful reset/scan/play run sends the `0x0F` pair and
  returns the terminal prompt; the trailing banner is drained and discarded.
  The output adds `Returned to the Bus Pirate terminal.`
- A run that ends with a non-success player result or a host error does not
  attempt the leave sequence, because the player may still be busy. Recover as
  described below.
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
  LICENSE               Apache License 2.0
  NOTICE                Copyright/attribution notice
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
    hextext.py          Whitespace-separated ASCII-hex parse/format helpers
    ports.py            pyserial port enumeration, boundary validation, Bus Pirate ID hints
    protocol/
      __init__.py
      bbio.py           Entry/framing, status checks, bitbang pins/ADC/PWM/self-test
      raw_wire.py       Raw 2-wire/3-wire configuration, bits, bytes, clocks
      i2c.py            Binary I2C, scanning, register helpers, EEPROM transfer
      spi.py            Binary SPI, chip-ID/bulk transfer, LCD adapter v2 cycle
      uart.py           Binary UART configuration, send, receive, bridge
      onewire.py        Binary 1-Wire ROM/search/scratchpad/memory/status
      xsvf.py           XSVF entry, chain reset/scan, chunk handshake, result mapping
    commands/
      __init__.py       Shared Click-context accessor
      inputs.py         Shared hex/binary file and preview helpers
      version.py        `version` command and transport-factory injection
      ports.py          `ports` command and likely-device output markers
      bbio.py           `bbio` group and bitbang services
      raw_wire.py       `raw-wire` group and raw-wire services
      i2c.py            `i2c` group and register/scan services
      spi.py            `spi` group and chip-ID/read/write services
      uart.py           `uart` group and send/receive/bridge services
      onewire.py        `onewire` group and ROM/scratchpad/memory services
      script.py         `script`/`raw-script` binary command loop
      eeprom.py         `eeprom` group, text-hex files, EEPROM services
      lcd.py            `lcd-test` command and repeat/interactive loops
      xsvf.py           `xsvf` command, file reads, orchestration, output
  tests/
    conftest.py         Scripted fake serial port and byte transport
    test_cli.py         CLI wiring, exit codes, configuration, failures
    test_config.py      YAML validation and connection precedence
    test_ports.py       Port enumeration, boundary validation, marker classification
    test_transport.py   Serial transport success/failure behavior
    test_types.py       Frozen EEPROM/connection/mode value objects
    test_protocol/      Exact wire sequences and protocol rejections per mode
    test_commands/      Command services, file formats, CLI exits, script loop
```

Boundary rules:

- `transport.py` knows only bytes, serial timeouts, and pyserial failures. It
  has no Bus Pirate modes, commands, or output.
- `ports.py` knows only pyserial enumeration and the external port-info
  boundary. It does not open a port, speak a protocol, or choose a port.
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
| `PortLister`, `list_serial_ports` | `src/bp/ports.py` | `PortLister` supplies external port objects (default `serial.tools.list_ports.comports`); `list_serial_ports` returns validated `SerialPortInfo` tuples. |
| `SerialPortInfo`, `BusPiratePortKind` | `src/bp/ports.py` | Frozen validated port value object with `vid_pid` and `bus_pirate_kind`; likely-device enum for `04D8:FB00`, `04D8:FAFF`, and `0403:6001`. |
| `BitbangSession`, `BitbangSessionFactory` | `src/bp/protocol/bbio.py` | `configure_directions`, `set_pins`, `read_pins`, power/pull-up/AUX/CS, bit/byte clocks, self-test, ADC, PWM. |
| `RawWireSession`, `RawWireSessionFactory` | `src/bp/protocol/raw_wire.py` | `configure`, `set_speed`, CS/start/stop/clock/data, bit/byte read/write, bulk clock. |
| `I2cControlSession`, `I2cDeviceSession`, `EepromSession` | `src/bp/protocol/i2c.py` | `scan`, `configure_speed`, `device`, register byte/word helpers, block transfer, EEPROM read/write/verify. |
| `SpiSession`, `LcdTestSession` | `src/bp/protocol/spi.py` | `configure_speed`, `configure_spi`, CS, transfer/bulk/read/write, `run_cycle`. |
| `UartSession`, `UartSessionFactory` | `src/bp/protocol/uart.py` | `configure_speed`, `configure_manual_speed`, `configure`, send, receive, bridge. |
| `OneWireSession`, `OneWireSessionFactory` | `src/bp/protocol/onewire.py` | Reset, read/write, search/alarm search, ROM read/match/skip, scratchpad, memory, status. |
| `XsvfSession` | `src/bp/protocol/xsvf.py` | `reset_chain`, `scan_chain`, `play`, `leave_to_terminal`. The command service accepts an injected session factory. |
| `open_bitbang_session`, `open_raw_wire_session`, `open_i2c_session`, `open_spi_session`, `open_uart_session`, `open_onewire_session`, `open_eeprom_session`, `open_lcd_session`, `open_xsvf_session` | `src/bp/protocol/` | Production factories that own `SerialTransport` plus the mode controller. |
| `VersionInfo`, `EepromLayout`, `ConnectionSettings` | `src/bp/types.py` | Frozen value objects validated at construction. |
| `SelfTestReport`, `RawWireConfig`, `I2cScanHit`, `SpiConfig`, `UartConfig`, `OneWireRom` | `src/bp/protocol/` | Frozen protocol value objects with validated masks, ranges, and CRC helpers. |
| `ChainScan`, `XsvfPlayReport` | `src/bp/protocol/xsvf.py` | Frozen XSVF scan/result reports, including raw count+IDCODE bytes and decoded IDCODEs. |

These contracts exist because the commands are tested without hardware and
because the transport/protocol/command split is the package's extension point.

## Limitations and known issues

- **Hardware verification:** Partial. On the BPv4 at COM6 (v9.0 firmware), the
  following no-target paths were exercised: `bp ports`, `bp version`, `bp bbio
  enter/reset/version/pins`, `bp raw-wire config`, `bp spi config`, `bp uart
  config`, `bp onewire reset/scan`, a full `bp i2c scan`, `bp script --reset`
  with byte `00`, `bp xsvf --reset`, and `bp xsvf --scan --leave` followed by
  `bp version`. The EEPROM, LCD, UART send/receive/bridge, 1-Wire device, SPI
  chip-ID/read/write, and I2C register/block operations were verified against
  firmware sources and fake transports but **not** against a connected target.
  No XSVF file was played and target programming was not attempted.
- **I2C bus-control replies on the flashed v9.0 firmware:** Start, stop, ACK,
  and NACK return `0x00`, while the protocol reference says `0x01`. The I2C
  controller accepts both values for those four commands and still enforces
  bulk-write success and per-byte ACK bits. The full scan path was exercised on
  hardware; register/block reads and writes were not target-tested.
- **Binary-bitbang speed is not available:** The v9 firmware implements no
  `0x60`/`0x70` speed command in BBIO mode. `bp bbio speed` was not shipped.
  Use `--bus-speed` on `bp raw-wire`, `bp i2c`, `bp spi`, and `bp uart`.
- **SPI configuration read-back is unavailable:** The firmware accepts
  `0x80 | cfg` but has no configuration-read command. The CLI reports the
  settings it applied rather than reading them back.
- **UART configuration read-back is unavailable:** The old `UART.read_cfg`
  sent `0xD0`; the v9 binary UART handler does not implement it. Only write
  configuration is ported.
- **Three-wire raw bulk writes:** The firmware's three-wire bulk-byte path
  does not return per-byte status and re-clocks the received value. The
  raw-wire controller clocks three-wire bytes bit by bit instead. Two-wire
  writes use the bulk command.
- **UART receive requires echo:** Binary UART has no read command. `bp uart
  receive` enables firmware receive echo, reads the requested byte count, then
  disables echo. A timeout leaves echo enabled until the next reset.
- **UART bridge never returns:** `bp uart bridge` enters the firmware bridge
  and exits after printing the warning. Only a hardware reset leaves bridge
  mode; Ctrl-C at the host cannot recover the firmware.
- **1-Wire search limit:** Search collects at most 64 devices per call and
  requires the firmware's eight-`0xFF` terminator. A real ROM equal to
  `FF FF FF FF FF FF FF FF` would be ambiguous with the terminator.
- **1-Wire device helpers are generic:** Scratchpad and memory commands are
  raw Dallas/Maxim command helpers (`0xBE`, `0x4E`, `0x48`, `0xF0`, `0xAA`).
  Device-specific lengths, EEPROM write timing, and strong pull-up
  requirements are the caller's responsibility.
- **Raw script side effects:** `bp script` executes arbitrary binary commands.
  It does not reset by default because the script may enter any submode. Use
  `--reset` only when the script leaves the device in BBIO.
- **Version banner labels:** `bp version` matches the banner by labels and was
  hardware-verified on a BPv4 (`Bus Pirate v4`, v9.0 firmware, `DEVID:0x1019`).
  A firmware release that renames those labels or changes the URL scheme would
  require a parser update; the protocol list and optional v3 bootloader line are
  intentionally ignored.
- **One-shot LCD visibility:** A single cycle is a short 1 ms blink pair; use
  `--repeat` for a longer visible test.
- **Port listing only:** `bp ports` enumerates and marks candidates, but no
  command selects or opens a port automatically. Ports must still be passed
  explicitly or configured. The legacy `COM1`-style default exists only for `bp
  version`.
- **No EEPROM page writes:** Each byte uses an individual I2C transaction, so
  large writes are slower than page-programmed writers.
- **No GUI:** GUI and `conio` keyboard polling are outside this package.
- **XSVF target may stay powered:** Without `--leave`, `bp xsvf` intentionally
  leaves the firmware player running, and the player leaves the regulator on.
  Press reset before reconnecting to the terminal, or use `--leave` on the next
  successful run. See the `bp xsvf` section for the exact exit state.
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
| `0` | Command completed successfully, including an XSVF scan/reset, an `XSVF_ERROR_NONE` stream result, an empty I2C/1-Wire scan, a zero-error self-test, or `bp ports` with no ports. |
| `1` | Runtime failure for `version`, `ports`, the mode groups, `script`, `eeprom`, or `lcd-test`; an XSVF player returned any non-success result byte. Port enumeration/entry validation failures, data-phase NACKs, self-test errors, invalid hex, ROM parse errors, and read/write timeouts also exit 1. |
| `2` | Click usage failure, or an XSVF host/transport/setup failure: bad or empty file, bad port, invalid configuration, wrong `BBIO1`/`XSV1` banner, missing reply/result, incomplete scan reply, failed `--leave` reset/acknowledgement/banner drain, or missing required port. Bridge mode without `--yes` also exits 2. |

Specific behavior:

- Opening a nonexistent or busy port reports `Could not open serial port ...`.
  For `bp xsvf`, transport and setup failures exit 2; for the other commands
  they retain exit 1.
- `bp ports` does not open a port. An enumeration error or an external entry
  without a string device/description or valid integer VID/PID pair exits 1 and
  reports the boundary violation.
- A serial read that does not deliver the requested bytes reports the requested
  and received counts.
- Binary command status `0x00` and unexpected status bytes are rejected, except
  for the four I2C bus-control replies documented above.
- I2C register/read-address NACKs and data-byte NACKs raise `I2cNackError` with
  the 7-bit address and operation.
- EEPROM memory/read-address NACKs and write-attempt exhaustion are rejected
  with chip/memory-address context; writes also report the byte value.
- A self-test with a non-zero error count exits 1 after leaving the self-test
  loop and resetting.
- `bp script` rejects empty, undecodable, and malformed text-hex files before
  opening the port. A per-byte reply timeout is not an error; it prints
  `(none)`.
- `bp onewire` rejects non-16-digit or non-hex ROM arguments before opening the
  port.
- `bp uart receive` reports a serial timeout directly; it does not send the
  disable-echo command after a failed read.
- `bp uart bridge` requires `--yes`; without it Click reports a usage error and
  exits 2.
- `bp spi read`/`write` reject addresses that do not fit the selected
  `--address-width` before opening the port.
- A failed mode-group operation does not attempt the normal "leave mode and
  reset" cleanup, so a transport or protocol error is not masked.
- A verification mismatch exits 1 and reports the first differing offset.
- `bp xsvf` rejects a missing or empty XSVF file before opening the port. It
  exits 2 for a wrong banner, v3/no-player response, scan timeout, missing data
  request, missing result, EOF before completion, or a failed `--leave` sequence.
  A non-success player result exits 1 and prints its `XSVF_ERROR_*` name; the
  leave sequence is not attempted after that result.
- A failed EEPROM/LCD operation does not attempt the normal "leave mode and
  reset" cleanup, so a transport or protocol error is not masked by a second
  failure. The bus, pull-ups, or adapter supply may remain in its current
  state; reset the Bus Pirate before reconnecting.
- `bp xsvf` performs no leave-mode cleanup by default, even on success. The
  board remains in the firmware player; recovery requires reset or the raw
  `0x0F` pair described in the command section. A new invocation cannot re-enter
  binary mode until the board is out of the player. Pass `--leave` to request
  the return-to-terminal sequence after a successful run.
- File contents are parsed and span-bounded before any serial session is
  opened. A read writes its output file only after the EEPROM transfer returns.

## Protocol notes

The binary protocol bytes follow the Bus Pirate v2.3+ binary I/O contract:

- Enter binary bitbang with a leading `0x00` (exit any active submode) plus
  the documented twenty `0x00` bytes, then require `BBIO1`. The package sends
  them as one 21-byte write.
- Binary-bitbang directions use `0x40 | mask` (set bits are inputs; five bits
  used) and pin/power/pull-up state uses `0x80 | mask`; both return one pin
  state byte. Short/full self-test use `0x10`/`0x11`, report an error byte,
  and leave with `0xFF`; ADC uses `0x14` and returns two big-endian bytes; PWM
  setup/clear use `0x12`/`0x13`.
- Enter raw-wire with `0x05` and require `RAW1`; configuration uses
  `0x80 | cfg` (bit 3 high-impedance, bit 2 three-wire, bit 1 LSB-first),
  speed `0x60 | code`, CS `0x04`/`0x05`, start/stop `0x02`/`0x03`, read
  byte/bit/peek `0x06`/`0x07`/`0x08`, clock tick/low/high `0x09`/`0x0A`/`0x0B`,
  data low/high `0x0C`/`0x0D`, bulk bytes `0x10 | (count - 1)`, and bulk clock
  `0x20 | (count - 1)`.
- Enter I2C with `0x02` and require `I2C1`; configure power and pull-ups with
  `0x40 | flags` (`0x4C` for EEPROM sessions, `0x44` for the generic CLI
  default); speed uses `0x60 | code` (0-3).
- Enter SPI with `0x01` and require `SPI1`; CS low/high use `0x02`/`0x03`;
  single-byte transfer uses `0x10 <byte>`; configuration uses `0x80 | cfg`;
  speed uses `0x60 | code`; bulk read/write uses `0x10 | (count - 1)` plus
  that many bytes and returns that many received bytes after a success status.
- Enter UART with `0x03` and require `ART1`; speed `0x60 | code`, manual BRG
  `0x07` plus high/low bytes, frame config `0x80 | cfg`, echo on/off
  `0x02`/`0x03`, bridge `0x0F`, bulk send `0x10 | (count - 1)`.
- Enter 1-Wire with `0x04` and require `1W01`; reset/read/search/alarm
  `0x02`/`0x04`/`0x08`/`0x09`, speed `0x20 | code`, bulk write
  `0x10 | (count - 1)` with one status per byte.
- I2C start/stop/read/ACK/NACK use `0x02`/`0x03`/`0x04`/`0x06`/`0x07`; bulk
  writes use `0x10 | (count - 1)` followed by the bytes and return one ACK bit
  per byte.
- Enter XSVF with `0x18` and require `XSV1`; v4 firmware only.
- XSVF chain reset uses `0x01` and has no reply. Chain scan uses `0x02` and
  returns a one-byte count followed by that many IDCODE bytes.
- XSVF playback starts with `0x03`. The player sends `0xFF` whenever it needs
  a chunk; the host answers with a 16-bit big-endian length and that many
  bytes. Chunks are at most 4096 bytes. The run ends with one result byte.
- Leaving the XSVF player uses `0x0F` (chain reset, silent return to `BBIO1`);
  a second `0x0F` returns to the terminal and replies `0x01`. `bp xsvf` sends
  this pair only with `--leave`, after all requested operations succeed; it
  then drains and discards the terminal banner.
- Leave binary mode with `0x00` and require `BBIO1`; reset with `0x0F` and
  require status `0x01`.

## References

Authority and source material used by this package:

| Source | Purpose |
|---|---|
| `git history:scripts/version.pl` | Legacy reset-banner behavior and platform default ports; removed. |
| `git history:scripts/I2CEEPROMWIN.c` | Legacy EEPROM chip/addressing/file behavior and no-page writes; removed. |
| `git history:scripts/powertools/LCDv2-tester/` | Legacy LCD adapter v2 test sequence; removed. |
| `git history:scripts/pyBusPirateLite/` | Behavioral reference only for `BitBang`, `RAW_WIRE`, `I2C`, `I2Chigh`, `SPI`, `UART`, and `_1WIRE`; removed after the port. No source, comments, or structure were copied. |
| `git history:scripts/pyBusPirateLite/spi_test.py` | SPI chip-ID and flash-style read workflow reference. |
| pyserial 3.5 `serial.tools.list_ports` | Port enumeration and the external `ListPortInfo` objects narrowed by `bp/ports.py`. |
| `git history:scripts/pyBusPirateLite/i2c-test.py`, `thermometer.py` | I2C register/word helper usage reference. |
| `git history:scripts/pyBusPirateLite/MicroWire.py`, `shtxx.py` | Raw-wire 3-wire and 2-wire usage reference. |
| [`tools/qt_gui/GUI/BinMode.cpp`](../../tools/qt_gui/GUI/BinMode.cpp), [`tools/qt_gui/GUI/Interface_rawtext.cpp`](../../tools/qt_gui/GUI/Interface_rawtext.cpp) | BBIO settings and Raw Text ASCII-hex behavior reference. |
| [`Documentation/bitbang.md`](../../Documentation/bitbang.md) | Binary bitbang terminal behavior and reset. |
| [`Documentation/i2c.md`](../../Documentation/i2c.md), [`Documentation/raw_2-wire.md`](../../Documentation/raw_2-wire.md), [`Documentation/raw_3-wire.md`](../../Documentation/raw_3-wire.md) | I2C and raw-wire terminal semantics and ACK behavior. |
| [`Documentation/spi.md`](../../Documentation/spi.md), [`Documentation/uart.md`](../../Documentation/uart.md), [`Documentation/1-wire.md`](../../Documentation/1-wire.md) | SPI, UART, and 1-Wire terminal semantics. |
| [`Firmware/binary_io.c`](../../Firmware/binary_io.c), [`Firmware/bitbang.c`](../../Firmware/bitbang.c), [`Firmware/i2c.c`](../../Firmware/i2c.c), [`Firmware/spi.c`](../../Firmware/spi.c), [`Firmware/uart.c`](../../Firmware/uart.c), [`Firmware/1wire.c`](../../Firmware/1wire.c) | Exact binary status bytes, framing, command maps, limits, and mode behavior used by the tests. |
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
- **Clean-room mode port:** The GPLv3 `scripts/pyBusPirateLite/` library was
  used only to identify behavior and workflows. The new code is an independent
  Apache-2.0 implementation with different types, module boundaries, CLI naming,
  validation, and test doubles. The old tree was deleted in the same change;
  it remains available in Git history for reference.
- **No BBIO speed command:** The old Qt GUI and `BitBang.py` sent `0x60`/`0x70`
  in binary-bitbang mode. The v9 firmware does not implement them in BBIO. The
  package omits the command rather than shipping a failing path; bus speed is
  set by each mode's own handler.
- **I2C bus-control reply variance:** The flashed v9.0 firmware returns `0x00`
  for start/stop/ACK/NACK. The new controller accepts `0x00` or `0x01` for
  those four commands, based on hardware evidence, while retaining strict
  data-phase checks. This differs from the protocol reference and is
  documented as a firmware quirk.
- **Leading NUL on binary entry:** One leading `0x00` exits any active binary
  submode before the twenty-NUL BBIO entry sequence. This prevents stale
  submode bytes from shifting the `BBIO1` banner when a previous command
  failed before cleanup. The bytes are sent in one 21-byte write.
- **Three-wire writes clock bits explicitly:** The firmware's three-wire bulk
  path is not used because it returns no per-byte status and re-clocks the
  received value. This is slower than the old example's bulk call but avoids
  corrupting the bus stream.
- **UART receive uses echo:** The old library exposed `begin_input`/`end_input`
  but no receive helper. `bp uart receive` composes those commands with a
  bounded serial read and a selectable timeout.
- **UART bridge is setup-only:** The old library only entered bridge mode.
  `bp uart bridge` does the same and requires `--yes`; it cannot bridge host
  stdin/stdout because firmware bridge mode never returns to the command loop.
- **SPI workflow:** The old `spi_test.py` read/id paths are ported as `bp spi
  read`/`bp spi chip-id`. Its write path was a stub and erase was `pass`, so
  `bp spi write` provides a real generic write instead of reproducing them.
- **1-Wire helpers are explicit device commands:** The old `_1WIRE` module
  exposed reset, read, and search only. The new module adds ROM read/match/skip,
  scratchpad, memory, and status helpers because the task requires the useful
  scratchpad/memory/status operations, while keeping them device-generic.
- **Raw script is hex-only:** The Qt Raw Text tab parsed tokens with
  `toUShort(base 0)`, which also accepts decimal and octal. `bp script` accepts
  the documented ASCII-hex forms only, reusing the package's strict hex parser.
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
- **Port discovery is list-only:** `bp ports` uses pyserial's
  `serial.tools.list_ports` and narrows its external port objects into
  `SerialPortInfo` at the boundary. It prints device, VID:PID, description, and
  markers for `04D8:FB00`, `04D8:FAFF`, and `0403:6001`, but never connects,
  scans the bus, or substitutes for an explicit `--port`. The FTDI ID is marked
  as a possible v3 rather than a confirmed Bus Pirate because other devices use
  it.
- **XSVF C tools removed:** `scripts/BPXSVFPlayer/` and
  `scripts/BP4_XSVFplayer_Win/` were deleted after `bp xsvf` passed its tests.
  The bundled Xilinx `SVF2XSVF` binaries lived inside those folders and were
  not ported; conversion is now documented as an external prerequisite.
- **Built-in player, not the old alternate firmware:** the removed C host
  players were written for the standalone XSVF-player firmware and sent `0x03`
  without a binary-mode entry. `bp xsvf` targets the current built-in v4
  player: `BBIO1` then `0x18`/`XSV1`. The standalone alternate firmware was
  already dropped by #233.
- **XSVF native exit contract:** `bp xsvf` returns 0 for success/scan/reset,
  1 for a non-success player result, and 2 for host, transport, or setup
  failures. The other commands keep the original 0/1/2 behavior described in
  their sections.
- **XSVF leave is opt-in:** The C players closed the port without sending
  `0x0F`, leaving the board in the player. `bp xsvf` preserves that default
  because a `0x0F` sent during a run is consumed as data. The new `--leave` flag
  sends the pair only after every requested operation succeeds, requires the
  `0x01` terminal-reset acknowledgement, and drains the banner without treating
  its content as a result. A leave failure exits 2. The README documents both
  the opt-in path and the raw recovery path.
- **XSVF file validation:** The legacy C player did not fail cleanly on a
  missing or empty file. `bp xsvf` rejects both before opening the serial port
  and exits 2, matching the source-build behavior documented for the C tools.
- **XSVF output:** The Python command prints the file size, reset and scan
  summaries, one final `Sent ... bytes in ... chunk(s)` line, the
  `End of operation reply`, and `Success!` instead of one progress line per
  chunk. It keeps the native result names and the scan byte display.
- **Clean-room implementation:** The package is Apache-2.0. The GPLv3
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
491 passed

ruff check .
All checks passed!

ruff format --check .
58 files already formatted

pyright
0 errors, 0 warnings, 0 informations
```

The tests cover success and rejection paths for bad ports, timeouts, invalid
chip addresses, invalid address widths, truncated/empty hex, invalid UTF-8 in
config and hex files, file failures, protocol status/NACK errors, EEPROM retry
exhaustion, sequential address increments, verification mismatches, CLI exit
codes, configuration validation, and exact one-shot/repeat LCD command
sequences. `test_ports.py` additionally covers fake external port objects,
missing and mistyped fields, VID/PID range and pairing, enumeration failures,
all three likely-Bus-Pirate IDs, unknown IDs, and the no-port case. The XSVF
tests cover the exact `0x0F`/`0x0F` leave pair, the required `0x01`
acknowledgement, failed and unexpected acknowledgements, missing
acknowledgements, write failures, banner-drain failure, skip-after-failure
behavior, and CLI wiring for `--leave`.

Mode coverage includes:

- BBIO: exact direction/state masks, power/pull-up/AUX/CS accumulation, 5-bit
  direction and 7-bit state validation, MSB/LSB bit clocks, ADC range checks,
  PWM frames/ranges, self-test entry/exit and non-zero error handling, banner
  rejection, and reset-on-exit behavior.
- Raw wire: configuration-byte encoding, 2-wire bulk writes with per-byte
  status, 3-wire bit-by-bit writes, CS/start/stop framing, bit/peek validation,
  clock bounds, and mode cleanup.
- I2C: exact repeated-start register sequences, byte/word helpers, block
  read/write splitting at 16 bytes, address scan with write/read ACK
  combinations, NACK paths for every operation, bus-control reply variance,
  invalid ranges, and EEPROM sequencing.
- SPI: configuration bits, all speed codes, bulk transfer lengths, CS framing,
  chunked read/write, chip-ID response, address-width/address-fit rejection,
  dummy-byte rejection, and binary file handling.
- UART: frame configuration bits, table and manual BRG programming, 16-byte
  send chunking, echo receive, timeout behavior without `end_input`, bridge
  cleanup suppression, and invalid ranges.
- 1-Wire: CRC-8 reference vector, ROM field/CRC validation, search/alarm
  terminator parsing and 64-device limit, read/match/skip, scratchpad,
  memory/status command frames, address/length rejection, and mode cleanup.
- Script: binary entry, per-byte reply collection, reset option, timeout
  propagation, both command aliases, invalid/empty/missing hex files, and
  reply-limit validation.

XSVF coverage includes exact `BBIO1`/`XSV1` entry, reset and scan command
sequences, scan payload reads and truncation, invalid count bytes, chunk
boundaries at 3, 4096, 4097, and 8195 bytes, data-request resend, EOF before a
result, every result byte including unknown values, success, missing
data/result replies, wrong banners, v3/no-player responses, empty/missing
files, transport write failures, the `--leave` `0x0F` pair and `0x01`
acknowledgement, failed/unexpected/missing acknowledgements, terminal-banner
drain failure, no-leave-after-failure behavior, and CLI exit codes 0, 1, and 2.

Hardware verification: **Partial.** On the BPv4 at COM6 (firmware v9.0):

- `bp ports` returned `COM1  ----:----  Communications Port (COM1)`,
  `COM6  04D8:FB00  USB Serial Device (COM6)  [Bus Pirate v4 application]`, and
  the unrelated `COM9  1209:0002  USB Serial Device (COM9)`. It did not open a
  port.
- `bp version --port COM6` returned `Bus Pirate v4`,
  `ElderlyPirate Firmware v9.0 - goo.gl/gCzQnW - 14256fd5`,
  `DEVID:0x1019 REVID:0x0004 (24FJ256GB106 A6)`, and the ElderlyPirate URL.
- `bp bbio enter`, `bp bbio version`, and `bp bbio pins` succeeded; `bp bbio
  reset` restored the terminal prompt. `bp bbio version` returned `BBIO1` and
  `bp bbio pins` returned `0x00` with no target connected.
- `bp raw-wire config --wires 3 --output normal --bus-speed 50k`, `bp spi
  config`, and `bp uart config --baud 9600` entered their modes, applied the
  requested settings, and reset successfully.
- `bp raw-wire write --wires 3 --data 00` exercised the three-wire bit-clock
  write path. `bp bbio write --data 00` and `bp bbio read --count 1` exercised
  the MOSI/CLK bit-bang paths and returned `00` from the floating bus. No
  target was connected.
- `bp onewire reset` succeeded and `bp onewire scan` returned
  `No 1-Wire devices found.` with no target, exercising the search terminator
  path.
- `bp i2c scan --start 0 --end 127` completed and returned
  `No I2C devices acknowledged addresses 0x00 through 0x7F.` No target was
  connected. This run exposed the firmware's `0x00` bus-control replies; after
  the documented adaptation the full scan path completed.
- `bp script --file ... --reset` with the single byte `00` printed
  `rx: 42 42 49 4F 31` (`BBIO1`), reset the board, and restored the terminal.
- `bp xsvf --reset --port COM6` exited 0 and printed
  `Performing Reset.. Done`.
- `bp xsvf --scan --port COM6` exited 0 and returned
  `Chain Scan Result: FC` followed by 252 zero bytes. That is the documented
  TDO-stuck-low/no-target result, including the 63-device scan clamp; no target
  or CPLD was connected.
- `bp xsvf --scan --leave --port COM6` exited 0, printed the same scan result
  and `Returned to the Bus Pirate terminal.`, and restored the user terminal.
  An immediate `bp version --port COM6` then returned the full banner, proving
  the `0x0F` pair and acknowledgement path on hardware. No XSVF file was played.
- After each safe command the board was reset or recovered to the terminal
  prompt. `bp version` was rerun after the mode checks and returned the banner.

No XSVF file was played: that would attempt to program an absent target. No
target programming was claimed or attempted. No I2C device, EEPROM, SPI flash,
UART peer, or 1-Wire device was connected. Register/block I2C operations, SPI
chip-ID/flash read/write, UART send/receive/bridge, and 1-Wire device commands
therefore have fake-transport and source-level verification only. The EEPROM
and LCD paths also use scripted fake serial and transport implementations; no
external EEPROM or LCD adapter was connected.

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
| `docs/tools/xsvf/README.md` | Replace the host-player file map and "Get the tools" folder links with `bp xsvf`; state that SVF conversion remains external and document the default player-exit state plus `--leave`. |
| `docs/tools/xsvf/players.md` | Rewrite the Windows/cross-platform build comparison, prebuilt-binary section, options (including `--leave`), output, and exit status around `bp xsvf`; remove references to `scripts/BPXSVFPlayer/`, `scripts/BP4_XSVFplayer_Win/`, and `scripts/tests/test_xsvf_players.py`. |
| `docs/tools/xsvf/workflow.md` | Replace the C command examples and file paths with `bp xsvf`; keep the wiring, firmware-player, and recovery material, and show `bp xsvf --scan --leave` as the terminal-return path. |
| `docs/tools/xsvf/limits-and-errors.md` | Replace C host messages and "legacy prebuilt" notes with the Python messages and exit contract, including failed `--leave` sequences as exit 2; keep the firmware limits and result actions. |
| `docs/protocol/jtag/xsvf.md` | Point the host workflow at `bp xsvf`; the in-mode protocol, result codes, and firmware limits are unchanged. Note that `--leave` reuses the existing `0x0F` pair and `0x01` reset acknowledgement. |
| `docs/tools/xsvf/converter.md` | Keep the external Xilinx conversion material, but remove folder paths that pointed inside the deleted C trees. |
| `docs/tools/xsvf/player.md` | Firmware-side content remains valid; check any host-tool cross-references. |
| Any `docs/` page that lists `scripts/pyBusPirateLite/` or its examples | Replace the host-library reference with the relevant `bp bbio`, `bp raw-wire`, `bp i2c`, `bp spi`, `bp uart`, `bp onewire`, or `bp script` section; state that the GPLv3 tree was removed. Exact page paths are on the docs branch and were not inspected on this branch; this is `Requires verification`. |
| Any `docs/` page that documents host port discovery or port selection | Add `bp ports` as the list-only helper; state that it prints device, VID:PID, description, and likely-device markers but never connects or chooses a port. Include the `04D8:FB00`, `04D8:FAFF`, and `0403:6001` meanings. |
| Any `docs/` command index, installation page, or quick-start page | Add `bp ports` and the `bp xsvf --leave` terminal-return example; keep the default XSVF exit-state warning. |
| `docs/` protocol pages that document binary host framing | Add the 21-byte binary entry sequence, the new mode command bytes, and the v9 firmware I2C `0x00` bus-control reply variance. |
| `docs/` tooling pages that mention the old Qt Raw Text tab | Point the ASCII-hex workflow at `bp script` / `bp raw-script`; note the package accepts strict ASCII hex rather than the GUI's base-0 parser. |
| `docs/` pages describing BBIO speed set/read | Remove or mark unsupported for the v9 firmware; speed is per-mode. |

The current-branch `Documentation/jtag.md` also marks the XSVF player as a
deprecated TODO. That note is stale for the v4 firmware player and is outside
this package's edit scope.

## License

The Python package is released under the **Apache License 2.0** (2026,
Bus Pirate contributors): a software license with an explicit patent grant,
attribution requirements and GPLv3 compatibility. See [`LICENSE`](LICENSE) and
[`NOTICE`](NOTICE); every Python file carries the SPDX identifiers.

The repository firmware/hardware remain under their existing CC0/public-domain
terms. The GPLv3 `pyBusPirateLite` tree was removed; this package is a
clean-room implementation and does not copy its code.
