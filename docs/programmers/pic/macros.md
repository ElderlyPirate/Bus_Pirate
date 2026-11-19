# PIC — macros

Part of the PIC documentation set (`docs/programmers/pic/`).  PIC has one
macro: `(0)` lists it, `(1)` reads the device ID.  The generic macro grammar
is in [generic commands](../../generic/commands.md).

## The macro list — `(0)`

| Macro | Text | Action |
|---|---|---|
| `(0)` | `(1) get devID` | list |
| `(1)` | — | read the device ID (6b/14b only) |

An unknown macro prints `Unknown macro, try ? or (0) for help`.

## `(1)` — get devID

`(1)` sends the classic 6-bit ICSP sequence on PGD/PGC, reads one 14-bit
word and splits it into a device ID and a 5-bit revision:

```
PIC>(1)
DevID = 0x0012 Rev = 0x1F
Please exit PIC programming mode
PIC>
```

The values are an example, not a specific part's ID.

| Field | Printed | Meaning |
|---|---|---|
| `DevID` | 4 hex digits | the word shifted right by 5 (up to 9 bits) |
| `Rev` | 2 hex digits | the low 5 bits of the same word |

The sequence is: load configuration space at address 0 (`0x00` command and
a zero payload), six address increments (`0x06`), then a read (`0x04`).  On
the PIC16F midrange parts the macro targets, the device ID sits six addresses
into configuration space, which is why the step count is fixed; other 6-bit
parts can use a different ID address.  The macro manages the Command/Data
state itself and suppresses the banners while it runs.  It does not drive
MCLR: apply VPP for an HVPP part or hold MCLR at VDD for an LVPP part before
running it.  Whether that sequence alone enters programming mode is
part-specific (**unverified**).

The macro reads and decodes the word itself, so the generic `l`/`L` setting
does not change its sequence or its output.  It ends in Data state and always
prints `Please exit PIC programming mode`; leave the mode (`m`/`m1`) before
using the Bus Pirate for anything else.

### 6b/14b only

In 4b/16b the macro prints:

```
PIC>(1)
Not implemented (yet)
Please exit PIC programming mode
```

It does not run the sequence and leaves the state unchanged.  Use the host
tool for a 4b/16b part.

## No answer on the bus

The mode has no error reporting; the read value is the only signal.

| Condition | `(1)` result | Meaning |
|---|---|---|
| pull-up fitted, no target | `DevID = 0x01FF Rev = 0x1F` | every sampled bit is 1 |
| no pull-up | undefined | the line floats |
| PGD held low | `DevID = 0x0000 Rev = 0x00` | a driven-low line or a fault |
| wrong part | a non-zero ID that is not the part's | the mode cannot detect it |
| read-protected part | a plausible or masked word | the mode cannot detect it |

All ones with the pull-up fitted is the **no-answer** signature.  All zero is
not: it means PGD is being held low.  See [edge-cases.md](edge-cases.md).

A plain read shows the same signature: with the pull-up fitted and no target,
`r` in 6b/14b data state returns `READ: 0x3FFF;14`, and in 4b/16b it returns
`READ: 0xFFFF`.

## v3 / v4

The macro list, the `(1)` sequence and the 4b/16b limitation are the same in
both builds.
