# PIC — the `picprog` host companion

Part of the PIC documentation set (`docs/programmers/pic/`).  This is the
companion card for the external host program that drives a PIC programming
algorithm through the Bus Pirate.  It is not part of this firmware.  The mode
itself is documented in [bus-operations.md](bus-operations.md) and
[binary.md](binary.md).

## The boundary

The firmware is a bit engine: it clocks opcodes and payloads on PGC/PGD and
nothing else.  `picprog` is the programmer: it owns the part table, the
programming-mode entry, the erase/program/verify algorithm, the configuration
words and the HEX/BIN file handling.  If the tool and the firmware disagree
about a part, the tool's part entry is the one to change.

## The route

`picprog` uses the **raw-wire** path, not the terminal PIC mode and not
`PIC1`:

1. Enter the binary layer and wait for `BBIO1`.
2. Send `0x05` for raw wire and wait for `RAW1`.
3. Use the raw-wire peripheral command (`0x4x`) for power, AUX and CS.
4. Select a PIC engine with `0xA0`: `1` = 416, `2` = 424, `3` = 614.
5. Use the `0xAx` PIC group, chiefly the `0xA7` write/read script, for the
   programming sequence.

The raw-wire PIC group is not conditioned on the terminal PIC build option,
so a build without terminal `PIC` still answers this path.  The tool's source
is [the PiratePICprog repository][picprog-source].

Other host routes:

| Route | What it drives | Where |
|---|---|---|
| terminal PIC mode | `[`/`]`, values, `r`, `(1)` | [bus-operations.md](bus-operations.md), [macros.md](macros.md) |
| `PIC1` (`0x07`) | the byte-wise PIC loop | [binary.md](binary.md) |
| raw wire (`0x05`) + `0xAx` | the script-driven PIC engines | [binary.md](binary.md) |
| `picprog` | raw wire + the `0xA7` script | this page |
| minimal host example | raw-wire generic bit/CS/AUX commands | [the Python raw-wire client](../../../scripts/pyBusPirateLite/pyBusPirateLite/RAW_WIRE.py) |

## What it is

`picprog` is the executable of the **PiratePICprog** project, an older
Dangerous Prototypes host program.  It is **not distributed with this
firmware** and is not built or maintained here.  Its source is published in
[the PiratePICprog repository][picprog-source]; its part table, options and
behaviour are the tool's own.

The banner prints `(Bus) Pirate Pic Programmer` and the build's version; the
current source defaults to `v0.3`.  Current availability and maintenance
status of the external project are **unknown** here.

## Part support

The tool ships a small part table.  That table is the authority for which
parts it supports and which need a high-voltage adapter; this set does not
re-verify it.  A part not in the table is not supported by the tool.  For an
HVPP part, use the adapter or an external VPP supply.

## Command line

```
picprog -p PROG -u PORT -s SPEED -c CHIP -t TYPE
        -w FILE | -r FILE [-E] [-V] [-I] [-h]
```

| Option | Meaning |
|---|---|
| `-p` / `-u` / `-s` | interface, serial port, port speed |
| `-c` / `-t` | chip type, file format (`HEX`/`BIN`) |
| `-w` / `-r` | write / read a file |
| `-E` / `-V` / `-I` | erase, verify, read the chip ID |
| `-h` | usage help |

Example (write, erase first, then verify):

```
picprog -p buspirate -u COM3 -s 115200 -c 18F24J50 -t HEX -w app.hex -E -V
```

`COM3` is a Windows example; use the port the host lists on other systems.

## What a run looks like

The tool prints its own progress; an example:

```
(Bus) Pirate Pic Programmer v0.3

Found '18F24J50' in programming database :) index = 1
Initializing interface
115200
Entering binary mode
BP: Setup mode...
Setup peripherals...
(OK)
Checking for 18F24J50 attached to programmer...
Found 18F24J50 (0x4C02, ID: 0x260 REV: 0x2)
Erasing chip... OK :)
Writing page 0, 0000...
Writing page 1, 0100...
```

## What the tool needs

- A Bus Pirate that answers `BBIO1` and `RAW1`.  The raw-wire PIC group is in
  both default images and is not gated on the terminal PIC mode.
- The ICSP wiring and a pull-up ([wiring.md](wiring.md)); a high-voltage VPP
  source for HVPP parts.
- A free serial port at a supported speed.
- The target in the tool's part table.

A user building a different programmer drives one of the documented binary
interfaces ([binary.md](binary.md)) or the terminal mode
([bus-operations.md](bus-operations.md)).

## Limits

- The tool's part table, masks and timings are outside this firmware; the
  tool's own table is the authority.
- `picprog` drives the raw-wire PIC group; it does not use the terminal PIC
  mode or `PIC1`.
- The tool is external and its current availability is **unknown** here.

[picprog-source]: https://github.com/DangerousPrototypes/PiratePICprog
