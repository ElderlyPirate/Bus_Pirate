# PIC — worked sessions

Part of the PIC documentation set (`docs/programmers/pic/`).  These are
terminal sessions with the wiring from [wiring.md](wiring.md) in place.
Target values are examples.

## Bring-up and device ID

Wiring: PGC, PGD and ground to the target, a pull-up on both lines, and the
target powered (its own supply or `W`).  The target is in programming mode:
VPP applied for HVPP, or MCLR tied to VDD for LVPP.  `(1)` does not drive
MCLR.

```
HiZ>m9
Commandmode?
1. 6b/14b
2. 4b/16b
(1)>1
Delay?
(1)>1
Ready
PIC>e2
3.3V on-board pullup voltage enabled
PIC>W
POWER SUPPLIES ON
Clutch engaged!!!
PIC>P
Pull-up resistors ON
PIC>(1)
DevID = 0x0012 Rev = 0x1F
Please exit PIC programming mode
PIC>
```

`e2` selects the 3.3 V feed for VPU; `W` is only needed if the target is
powered from the Bus Pirate or the on-board pull-up feed is used; `P`
connects the pull-ups.  The ID is an example; the no-answer value is in
[macros.md](macros.md).

## Write and read one word

Wiring: the same.  The mode is 6b/14b, the target is in programming mode, and
the address is the current one.  The opcodes are the six-bit example values
from [config-words.md](config-words.md).

```
PIC>[
CMD6
PIC>0x02
WRITE: 0x02
PIC>]
DTA
PIC>0x1234
WRITE: 0x1234;14
PIC>[
CMD6
PIC>0x08
WRITE: 0x08
```

`0x02` loads the word `0x1234`; `0x08` begins the programming cycle.  Wait
the part's programming time, then read the same address:

```
PIC>0x04
WRITE: 0x04
PIC>]
DTA
PIC>r
READ: 0x1234;14
PIC>
```

`0x04` is the read opcode; the read itself is the `r` in data state.  The
read-back value is an example: a protected, erased or failed part returns a
different word.  A full part program is a loop of load, begin, wait, and
address increment — the host tool's job.

## 4b/16b entry and read

Wiring: the same.  The part is a 4b/16b (PIC18-class) target.

```
HiZ>m 9 2 1
PIC(mod dly)=(4 1)
Ready
PIC>[
CMD4
PIC>0x0
WRITE: 0x00
PIC>]
DTA
PIC>r
READ: 0x1234
PIC>(1)
Not implemented (yet)
Please exit PIC programming mode
PIC>
```

The `;16` suffix is absent because 16 bits is the default word display in
4b/16b.  The `(1)` macro is 6b/14b only; use [`picprog`](picprog.md) for this part.

## No target

Wiring: the pull-up enabled (`P`), VPU fed, and no target connected.

```
PIC>]
DTA
PIC>r
READ: 0x3FFF;14
PIC>(1)
DevID = 0x01FF Rev = 0x1F
Please exit PIC programming mode
PIC>
```

All ones is the no-answer signature.  In 4b/16b the plain read is
`READ: 0xFFFF`.  Without the pull-up the read is undefined, not all ones.

## Leaving

`w` powers the rails down and runs the mode cleanup, but it does **not**
leave the mode:

```
PIC>w
Clutch disengaged!!!
POWER SUPPLIES OFF
PIC>m1
Ready
HiZ>
```

`m1` returns to HiZ and releases the pins.  A value typed after `w` but
before `m1` is still clocked out on PGC/PGD, so leave the mode when the
programming session is over.
