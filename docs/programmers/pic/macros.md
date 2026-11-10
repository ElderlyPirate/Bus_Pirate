# PIC — macros

Part of the PIC documentation set (`docs/programmers/pic/`).  PIC has a small
macro set: `(0)` lists it, and `(1)` reads the device.  The generic macro
grammar (`(0)` list, `(n)` run, error wording) is owned by the generic layer —
see [generic commands](../../generic/commands.md).

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

The values above are an example, not a specific part's ID.

| Field | Printed as | Meaning |
|---|---|---|
| `DevID` | 4 hex digits | the device identification, the 14-bit word shifted down by 5 (so up to 9 bits) |
| `Rev` | 2 hex digits | the revision, the low 5 bits of the same word |

In **6b/14b** the macro manages the Command/Data state itself (it switches to
Command, issues the entry sequence, then reads in Data) and suppresses the
`CMD`/`DTA` banners while it runs.  It always ends by printing
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

The 4b/16b path leaves the Command/Data state unchanged (it does not run the
sequence).  Use the host tool for a 4b/16b part — see
[entry-and-setup.md](entry-and-setup.md) for the width ↔ family table.

## No answer on the bus

There is no error reporting from this mode; the value you read is the only
signal, so compare it with the part's expected ID.

- **Pull-up fitted, target absent:** PGD is released and idles high, so every
  sampled bit is `1` and `(1)` returns all ones:

  ```
  PIC>(1)
  DevID = 0x01FF Rev = 0x1F
  Please exit PIC programming mode
  ```

- **No pull-up:** the read value is undefined (the line floats).
- **All zero (`DevID = 0x0000 Rev = 0x00`):** PGD is being held low — check
  that the target is powered and that PGD/PGC are not shorted or swapped; the
  part may also be holding the line during entry.

An all-zero word is not the "no answer" signature; all ones with the pull-up
fitted is.  See [edge-cases.md](edge-cases.md).

## v3 / v4

The `(0)` list and the `(1)` device-ID macro are the same in both builds.
The 4b/16b limitation is the same.  The v3 path is **not verified**.
