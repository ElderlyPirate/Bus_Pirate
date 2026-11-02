# PIC — macros

Part of the PIC documentation set (`docs/pic/`).  PIC has a small macro set:
`(0)` lists it, and `(1)` reads the device.  The generic macro grammar
(`(0)` list, `(n)` run, error wording) is owned by `docs/generic/` — see
[commands.md](../generic/commands.md) §Macros.

## The macro list — `(0)`

```
PIC>(0)
(1) get devID
PIC>
```

The only mode macro is the device-ID read.  An unknown macro number prints the
generic unknown-macro error.

## `(1)` — get devID

`(1)` performs the low-level ICSP entry handshake on PGD/PGC, reads the
14-bit device/revision word back, and prints it split into a device ID and a
5-bit revision:

```
PIC>(1)
DevID = 0x0012 Rev = 0x1F
Please exit PIC programming mode
PIC>
```

| Field | Printed as | Meaning |
|---|---|---|
| `DevID` | 4 hex digits | the device identification, the 14-bit word shifted down by 5 (so up to 9 bits) |
| `Rev` | 2 hex digits | the revision, the low 5 bits of the same word |

The macro manages the Command/Data state itself and suppresses the `CMD`/`DTA`
banners while it runs.  It always ends by printing
`Please exit PIC programming mode` — a reminder that after the ID read the
mode is still in a programming session and should be left (`m` to change
mode) before using the Bus Pirate for anything else.

### 6b/14b only

`(1)` is implemented for the **6b/14b** width only.  In **4b/16b** mode it
prints:

```
PIC>(1)
Not implemented (yet)
Please exit PIC programming mode
```

## Reading a device with no reply

If nothing answers on the bus, `(1)` returns an all-zero word:

```
PIC>(1)
DevID = 0x0000 Rev = 0x00
Please exit PIC programming mode
```

An all-zero result means the target is not responding — no power, no pull-up,
no common ground, or the part is not in programming mode.  See
[edge-cases.md](edge-cases.md).

## v3 / v4

The `(0)` list and the `(1)` device-ID macro are the same in both builds.
The 4b/16b limitation is the same.  The v3 path is **not verified**.