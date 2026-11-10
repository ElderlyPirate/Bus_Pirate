# PIC — the `picprog` host tool

Part of the PIC documentation set (`docs/pic/`).  This page is the supporting
section for the companion host program that drives PIC mode.  The mode itself
is documented in [bus-operations.md](bus-operations.md) and
[binary.md](binary.md).

`picprog` (Pirate PIC Prog) is a separate, external program — it is **not**
part of this firmware repository.  It runs on the host PC, opens the Bus
Pirate's serial port, enters the Bus Pirate's binary mode, and drives the PIC
programming algorithm over the mode's bit-shifting commands.  The Bus Pirate
supplies the ICSP clock/data engine; `picprog` supplies the PIC part database,
the entry sequence, the erase/program/verify algorithm, and the HEX/BIN file
handling.

Everything in this section is taken from the tool's own documentation and is
**unverified** against this firmware.

## Command line

```
picprog -p PROG -u PORT -s SPEED -c CHIP -t TYPE
        -w FILE | -r FILE [-E] [-V] [-I] [-h]
```

| Option | Argument | Meaning |
|---|---|---|
| `-p` | PROG | interface name (for example `buspirate`) |
| `-u` | PORT | serial port (`COM3`, `/dev/ttyUSB0`, …) |
| `-s` | SPEED | port speed in bps (for example `115200`) |
| `-c` | CHIP | chip type (for example `18F24J50`) |
| `-t` | TYPE | file format: `HEX` or `BIN` |
| `-w` | FILE | write FILE to the PIC |
| `-r` | FILE | read the PIC into FILE |
| `-E` | — | erase flash |
| `-V` | — | verify flash against the file |
| `-I` | — | read the chip ID |
| `-h` | — | print usage help |

## Example

Writing a HEX file, erasing first and verifying afterwards:

```
picprog -p buspirate -u COM3 -s 115200 -c 18F24J50 \
        -t HEX -w firmware.hex -E -V
```

The tool prints its banner, opens the interface, enters the Bus Pirate's
binary mode, checks the target's device ID, then reports its progress.  The
dialog lines below are the tool's own sample output, trimmed of status
banners:

```
(Bus) Pirate PIC Programer v0.1
buspirate
Initializing interface
115200
Entering binary mode
BP: Setup mode...
Setup peripherals...
Found '18F24J50' in programming database index = 1
Checking for 18F24J50 attached to programmer...
Erasing chip...
Writing page 0, 0000...
Writing page 1, 0100...
...
```

The database line reports the part name and its index in the tool's table;
the tool then checks the attached part's device ID and revision.  The page
addresses and count depend on the part and the file.

## Supported parts (as documented)

The tool's documentation lists an initial, deliberately limited part set:

| PIC family | Example part | VPP | Notes |
|---|---|---|---|
| 18Fxx5x | 18F2550 | high voltage (13 V) | needs the high-voltage adapter or an external VPP supply |
| 18F2/4xJxx | 18F24J50 | low voltage | ID/erase/write/read documented |
| 24FJXXGAXXX | 24FJ64GA002 | low voltage | read returns data but verification can fail (the tool's HEX parser does not understand the PIC24F format) |

Adding new families means adding a part entry and a family entry to the tool's
own tables and rebuilding it; that work is in the external tool, not in the
Bus Pirate firmware.

## What the tool needs from the Bus Pirate

- The Bus Pirate flashed with a build that includes PIC mode (both v3 and v4
  default images do — see [../SPEC.md](../../SPEC.md)).
- The ICSP wiring and a pull-up ([wiring.md](wiring.md)); a high-voltage VPP
  source for HVPP parts.
- A free serial port at a supported speed.

The split of responsibility is the key point for this mode: the Bus Pirate
bit-bangs PGC/PGD and nothing more, so any PIC algorithm is the host tool's
job.  A user building a different programmer drives the same `PIC1` binary
command set directly ([binary.md](binary.md)).