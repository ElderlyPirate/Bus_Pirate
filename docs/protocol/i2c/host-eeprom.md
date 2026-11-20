# I2C — external EEPROM host access

Part of the I2C documentation set (`docs/protocol/i2c/`). This page owns the
`bp eeprom` frontend: how a host reads, writes or verifies an **external** I2C
EEPROM through the binary `I2C1` layer. The on-board v4 EEPROM is different and
is reached with the terminal macros — see [onboard-eeprom.md](onboard-eeprom.md).
The terminal byte sequence is in
[bus-operations.md](bus-operations.md#worked-session-eeprom-write-and-read-back);
the binary layer itself is in [binary.md](binary.md).

## What it is

`bp eeprom` is the `bp` package's external-EEPROM workflow. It replaces the
legacy Windows program `I2CEEPROMWIN`; that program is no longer shipped, and
its `R`/`W`/`Rs`/`Ws` modes map to `--address-width 2`/`1`.

The command drives the **external** SDA/SCL bus through binary `I2C1`. It
cannot select the v4 on-board EEPROM bus, which is only reachable from the
terminal with macros `(3)` and `(4)`.

## Installation

`bp` is a Python console package; Python 3.12 or newer is required. From the
repository's `scripts/bp` directory:

```
cd scripts/bp
python -m venv .venv
# Windows: .venv\Scripts\activate
# POSIX:   source .venv/bin/activate
python -m pip install -r requirements.txt
python -m pip install -e .
```

The package needs a serial port for the Bus Pirate; it does not need a GUI.

## Wiring and power

| Item | Requirement |
|---|---|
| SDA / SCL | SDA on MOSI, SCL on CLK, common ground with the EEPROM |
| pull-up | 2 kΩ–10 kΩ from each line to the logic rail; required |
| Vpullup | if you rely on the Bus Pirate pull-ups, feed this pin externally; binary entry leaves the on-board sources off |
| target supply | switched on; do not parallel it with an externally powered board |
| AUX | the binary peripheral command drives AUX low; leave AUX unjumpered from SDA/SCL |
| serial port | close the terminal first — the COM port is exclusive |

`bp eeprom` sends the binary peripheral command with power and pull-ups on. It
does not send the v4 on-board pull-up-feed command, and binary entry turns the
on-board sources off. A board whose Vpullup pin has no external feed therefore
has no bus idle high even though `P` is on, unless the target provides its own
pull-ups to its VCC. Jumper Vpullup to the target rail, or feed it from a supply
at the target's logic level. The examples assume such a feed.

On Windows, if a two-digit port is not found, use the device form:
`--port \\.\COM10`. `bp ports` lists serial devices and marks likely Bus
Pirate IDs without opening a port.

## Commands

| Command | What it does |
|---|---|
| `bp eeprom read` | read a span into a text-hex file |
| `bp eeprom write` | write a text-hex file one byte per transaction |
| `bp eeprom verify` | read the span back and compare it with the file |

`read` requires `--length` and `--file`. `write` and `verify` take the transfer
length from the number of bytes in the file.

Shared options:

| Option | Meaning |
|---|---|
| `--chip 0..7` | chip-select bits (required); `0` is 7-bit `0x50`, write `0xA0`, read `0xA1` |
| `--address-width 1\|2` | one or two memory-address bytes; default `2` |
| `--memory-address` | starting memory address; default 0; decimal only |
| `--file` | output file for `read`, input file for `write`/`verify` |
| `--length 1..65536` | bytes to read; `read` only |
| `--port` | serial port; required unless a configuration file supplies it |
| `--speed` | **host serial** speed; default 115200; not the I2C clock |

Click integer options are decimal. Write `--memory-address 32`, not `0x20`;
`0x20` is a usage error. `--speed` changes the link to the Bus Pirate, not the
I2C bus rate; the binary entry default (`Maximum speed`, software driver) is
used.

## A read, a write and a verify

```
bp eeprom read --port COM6 --chip 0 --address-width 2 --memory-address 0 --length 256 --file eeprom.txt
Read 256 bytes from 0x0000 to eeprom.txt.
bp eeprom write --port COM6 --chip 0 --address-width 2 --memory-address 0 --file eeprom.txt
Wrote 256 bytes at 0x0000 with 0 write retries.
bp eeprom verify --port COM6 --chip 0 --address-width 2 --memory-address 0 --file eeprom.txt
Verified 256 bytes at 0x0000 against eeprom.txt.
```

The command lines are single-line on purpose; copy them whole, or set `port`
in a `bp.yaml` and omit `--port`. `read` prints a summary and writes the image;
it does not dump 256 bytes to the console. `write` reports the number of bytes
and the total retried attempts. `verify` compares the read-back span and reports
the first mismatch.

For a 24LC08-style part with one address byte, use `--address-width 1` and the
same commands:

```
bp eeprom read --port COM6 --chip 0 --address-width 1 --memory-address 0 --length 256 --file eeprom8.txt
```

## Text-hex file format

A write/verify file is UTF-8 text (an optional UTF-8 BOM is accepted) with
whitespace-separated tokens:

- one or two hex digits per token, optionally prefixed `0x` or `0X`;
  `AA`, `aa`, `0xAA` and `0Xaa` are all accepted;
- any whitespace separates tokens;
- packed strings such as `AABB`, tokens longer than two digits, `0x` with no
  digits, and empty files are rejected before the serial port is opened.

```
A5 5A C3 3C 01 FE
```

A read file uses uppercase, two-digit tokens separated by one space and ends
with a newline:

```
AA BB CC
```

Decimal is not accepted: `255` is a three-digit token and is rejected, not read
as decimal 255.

## Addressing

| `--chip` | 7-bit address | write byte | read byte |
|---:|---|---|---|
| 0 | `0x50` | `0xA0` | `0xA1` |
| 1 | `0x51` | `0xA2` | `0xA3` |
| 2 | `0x52` | `0xA4` | `0xA5` |
| 3 | `0x53` | `0xA6` | `0xA7` |
| 4 | `0x54` | `0xA8` | `0xA9` |
| 5 | `0x55` | `0xAA` | `0xAB` |
| 6 | `0x56` | `0xAC` | `0xAD` |
| 7 | `0x57` | `0xAE` | `0xAF` |

`--address-width 2` covers memory addresses `0x0000`–`0xFFFF` (64 KiB).
`--address-width 1` covers `0x0000`–`0x07FF` (2 KiB) and encodes address bits
8–10 in the device address, so one-byte-address parts that use the upper bits
as page select still work.

The tool does not know the part's real size. A 24xx64 is 8 KiB, but
`--address-width 2` will accept addresses up to `0xFFFF`; staying inside the
part's range is the caller's responsibility. A transfer that runs past the
selected address space is rejected before the bus is touched.

## Behaviour

| Aspect | Behaviour | Value / limit | On failure |
|---|---|---|---|
| write transaction | start, device address, memory address, data, stop | one byte per transaction | a NACK retries the whole byte frame |
| page writes | not used | page sizes vary by part, so single-byte writes stay chip-agnostic | a burst never crosses a page boundary |
| write retry | each byte is retried after a NACK | 100 attempts, 5 ms apart (about 0.5 s) | error after 100; no option to raise it |
| read | pointer write, stop, then a separate read | all but the last byte ACKed | a NACK on the pointer or read address is an error |
| serial link | 115200 8N1 by default | 1 s read and write timeout | a timeout is an error; no retry loop |
| file output | full transfer, then file write | uppercase hex, final newline | a failed read does not write the file |
| verify | read the expected span and compare | first mismatch reported | exits non-zero at the first difference |
| cleanup | return to BBIO1 and reset the board | after a successful command | a failed command skips the leave/reset step |

**Why one byte per transaction.** EEPROM page sizes are not standardized. The
frontend writes one byte per I2C transaction so the same command works on any
part, at the cost of one transaction and one write-cycle wait per byte.

**A failed write is partial.** Earlier bytes remain programmed. There is no
rollback and no page buffer to discard. Reset the board before the next run:
a failed command may leave it in binary I2C mode. Then fix the cause and read
the file back with `verify`, or re-run the write.

## Failure output and exit codes

| Exit | Meaning |
|---|---|
| 0 | the read, write or verify completed |
| 1 | runtime failure: port, timeout, NACK, invalid hex, span, or verify mismatch |
| 2 | Click usage failure: missing or invalid option, missing input file |

Typical messages:

```
Error: No serial port specified; pass --port or set 'port' in the configuration file
Error: Could not open serial port COM6: ...
Error: EEPROM at chip address 0 did not acknowledge memory address 0x0020
Error: EEPROM at chip address 0 did not acknowledge read address 0xA1
Error: EEPROM write at address 0x0020 (value 0xAA) was not acknowledged after 100 attempts
Error: EEPROM verification failed at offset 0x0005: expected 0xAA, read 0xFF
Error: Invalid hex token 'AABB' at position 1
```

The verify offset is relative to the start of the transfer, not the absolute
EEPROM address. For a transfer starting at `0x0100`, `offset 0x0005` is memory
address `0x0105`.

A failed command does not attempt the normal leave-mode cleanup, so the Bus
Pirate can remain in binary I2C mode. Reset the board before the next run.

## Limits

- External bus only; the on-board v4 EEPROM is not reachable.
- No page writes: a large image is slow by design (one transaction per byte).
- No I2C bus-speed choice in the command; it uses the binary entry default.
- No pull-up-voltage selection; feed Vpullup yourself.
- No rollback on a failed write.
- No automatic port selection; `--port` or a configuration `port` is required.
- The full `bp i2c` group (scan, register and block operations) is a separate
  host path and is outside this set.

## Related docs

- [onboard-eeprom.md](onboard-eeprom.md) — the v4 on-board chip and macros 3/4.
- [binary.md](binary.md) — the `I2C1` commands the frontend uses.
- [wiring.md](wiring.md) — pull-ups, Vpullup and the voltage limit.
- [edge-cases.md](edge-cases.md#host-eeprom-tool) — invocation and failure
  cases.
