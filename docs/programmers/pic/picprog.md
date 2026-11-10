# PIC — the `picprog` host tool (companion card)

Part of the PIC documentation set (`docs/programmers/pic/`).  This is the
companion card for the external host program that drives a PIC programming
algorithm through the Bus Pirate.  It is not part of this firmware.  The mode
itself is documented in [bus-operations.md](bus-operations.md) and
[binary.md](binary.md).

## What it is and where it lives

`picprog` (**Pirate PIC Prog** / PiratePICprog) is an older, separate
Dangerous Prototypes program.  It is **not distributed with this firmware**
and is not built or maintained here.  Its source is published in the
`DangerousPrototypes/PiratePICprog` repository; its part table, options and
behaviour are the tool's own, not this firmware's.

The split of responsibility is the key point for this mode: the Bus Pirate
bit-bangs PGC/PGD and nothing more, so the PIC entry sequence, part database,
erase/program/verify algorithm and HEX/BIN handling are all the host tool's
job.

## Which interface it drives

The original tool is external, so its exact binary interface is not documented
in this repository.  An in-repository C# companion drives the **raw-wire**
binary mode (`0x05`/`RAW1`) and its generic bit/CS/AUX commands, not
`PIC1`.  A host written against this firmware should choose one of the two
documented interfaces in [binary.md](binary.md).

## Part support (as documented by the tool)

The tool is in an initial, deliberately limited state:

| PIC family | Example part | VPP | Notes |
|---|---|---|---|
| 18Fxx5x | 18F2550 | high voltage (13 V) | **write/read not complete** — ID/erase only |
| 18F2/4xJxx | 18F24J50 | low voltage | ID/erase/write/read documented |
| 24FJXXGAXXX | 24FJ64GA002 | low voltage | read data is valid, but verify can fail (HEX format) |

The 18F2550 also needs the high-voltage adapter or an external VPP supply.
The 24FJ verify failure is the tool's HEX parser, which does not understand
the PIC24F firmware format.  Adding new families means adding a part entry and
a family entry to the tool's own tables and rebuilding it; that work is in the
external tool, not in the Bus Pirate firmware.

## Command line (from the tool's documentation)

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
picprog -p buspirate -u COM3 -s 115200 -c 18F24J50 \
        -t HEX -w firmware.hex -E -V
```

The tool's own example dialog (trimmed) looks like:

```
(Bus) Pirate PIC Programer v0.1
buspirate
Initializing interface
115200
Entering binary mode
...
Found '18F24J50' in programming database index = 1
Checking for 18F24J50 attached to programmer...
Erasing chip...
```

## What the tool needs from the Bus Pirate

- The Bus Pirate flashed with a build that includes PIC mode (both default
  images do — see [SPEC](../../SPEC.md)).
- The ICSP wiring and a pull-up ([wiring.md](wiring.md)); a high-voltage VPP
  source for HVPP parts.
- A free serial port at a supported speed.

A user building a different programmer drives one of the documented binary
interfaces directly ([binary.md](binary.md)).
