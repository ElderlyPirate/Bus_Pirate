# I2C — bus operations

Part of the I2C documentation set (`docs/i2c/`).  This page owns the on-bus
commands: start/stop, byte read/write, the ACK/NACK model, and the address
scanner.  Shared prompt grammar — value entry, repetition `:N`, width `;N`,
the wrappers, `(n)` macros — is owned by `docs/generic/`; only the I2C effect
is described here.

## Start and stop

| Input | Action | Firmware output |
|---|---|---|
| `[` | start condition | `I2C START BIT` |
| `{` | start-with-read form; same I2C start | `I2C START BIT` |
| `]` | stop condition | `I2C STOP BIT` |
| `}` | stop-from-read form; same I2C stop | `I2C STOP BIT` |

A start or stop that resolves a still-pending read ACK first prints `NACK` and
sends a NACK bit (see
[Reading a byte](#reading-a-byte--the-acknack-model) below).

Starting while a line is stuck low prints `Warning: *Short or no pull-up` —
the same pull-up check as the scanner ([wiring.md](wiring.md)).

## Writing a byte

Any number written in the mode is an I2C byte and the slave's ACK is checked
immediately:

| Input | Meaning |
|---|---|
| `0xNN` | hex byte (`0xA0`, `0xA` — partial widths allowed) |
| `0b…` | binary byte (partial widths allowed) |
| `NNN` | decimal byte (no prefix) |

The reply echoes the value and the slave's response:

```
WRITE: 0xA0 ACK
WRITE: 0x60 NACK
```

A write also flushes any pending read ACK first (it prints `ACK` then the
`WRITE:` line).

## Reading a byte — the ACK/NACK model

**Quick tell:** if a `READ:` line shows **two spaces** after the colon, the
`ACK`/`NACK` that follows is the deferred ninth bit of the **previous** byte;
the value printed after it is this byte.  (A write does not use this form — a
pending bit flushed by a write prints as its own `ACK` line first, see
Writing a byte above.)

`r` reads one byte; `r:N` reads N bytes.  The I2C read byte's **ninth bit is
not sent when the byte is read**.  Instead the Bus Pirate remembers it as
*pending* and sends it with the **next** command:

- if the next command is a **start or stop**, the pending bit is sent as
  **NACK** (this ends the read);
- for any other next command it is sent as **ACK**.

This is why reading two bytes in a row prints an `ACK` before the second
value:

```
I2C>r
READ: 0xAA
I2C>r
READ:  ACK 0xBB
I2C>]
NACK
I2C STOP BIT
```

The two spaces after `READ:` in the second read are the header followed by the
deferred `ACK`; the final `]` resolves the second byte's pending bit as `NACK`
and then sends the stop.

Notes:

- A bulk read `r:N` prints one `READ: ` header and then the N bytes, each
  preceded by the deferred `ACK` of the byte before it.  The last byte's bit
  stays pending until the next command.
- A single write (`0xNN` / `0b…` / decimal) does not defer anything: the
  slave's ACK is read during the write itself.
- The current firmware prints **no explicit "pending" warning** when a read is
  left unresolved — the state is shown only by whether the next line starts
  with `ACK`/`NACK`.  (Older published I2C text showed a `*(N)ACK PENDING`
  banner and a `BULK READ n BYTES:` banner; neither string exists in the
  current firmware.)
- A partial read leaves the ninth bit hanging until a following command; if
  you break a transaction early, send a stop (`]`) or a start (`[`) to resolve
  it as NACK rather than leaving it pending.

## Address scanner — macro `(1)`

The scanner brute-forces the 7-bit address space.  It checks every 8-bit
address (`0x00`–`0xFF`) with a start, the address byte, and an ACK-read.

Each hit is the raw 8-bit address the slave acknowledged, then the 7-bit
address in parentheses with a `W` (write address, R/W bit 0) or `R` (read
address, R/W bit 1) marker.  All hits are printed on one line.  Datasheets
usually list the 7-bit address; the 8-bit form is what a logic analyzer or
sniffer shows.

**Hit example** — format illustration:

```
I2C>(1)
Searching I2C address space. Found devices at:
0xA0(0x50 W) 0xA1(0x50 R)
I2C>
```

**Empty bus** (no device responds): the header prints and then nothing else,
no addresses, one blank line, then the prompt.

```
I2C>(1)
Searching I2C address space. Found devices at:

I2C>
```

Behaviour details:

- Before scanning, both lines are driven high through the pull-up.  The
  pre-check trips **only when a line still reads low** (a short, or a target
  holding it): it prints `Warning: *Short or no pull-up` and returns
  **without scanning**.  If instead a floating line reads high — the usual
  no-pull-up case — the check passes, the scan runs, and every 8-bit address
  ACKs, so the scanner reports a device at every address.  That is exactly the
  no-pull-up symptom described in [wiring.md](wiring.md); the two outcomes
  differ only by whether the floating line happens to read low or high.
- For a **read** address that ACKs, the scanner reads one byte and NACKs it,
  then sends a stop.  This is deliberate: a slave that ACKs its read address
  starts driving data, and a stop sent immediately can be missed; reading and
  NACKing ends the transaction cleanly and avoids "ghost" addresses.

The strings and format above are **not verified**.

## Worked session

In this example the addressed device is an EEPROM-style slave at address
`0x50` (write `0xA0`, read `0xA1`).  The bring-up is in
[entry-and-setup.md](entry-and-setup.md); this is the on-bus flow:

```
I2C>[
I2C START BIT
I2C>0xA0
WRITE: 0xA0 ACK
I2C>0x20
WRITE: 0x20 ACK
I2C>0xAA
WRITE: 0xAA ACK
I2C>0xBB
WRITE: 0xBB ACK
I2C>[
I2C START BIT
I2C>0xA0
WRITE: 0xA0 ACK
I2C>0x20
WRITE: 0x20 ACK
I2C>[
I2C START BIT
I2C>0xA1
WRITE: 0xA1 ACK
I2C>r
READ: 0xAA
I2C>r
READ:  ACK 0xBB
I2C>]
NACK
I2C STOP BIT
I2C>
```

In this example the EEPROM-style device treats the first data byte after the
address as a register pointer, so the sequence writes `0xAA 0xBB` at register
`0x20` and reads them back.  A binary-mode exchange of the same transaction is
in [binary.md](binary.md).

## v3 / v4

Start/stop, read/write and the ACK/NACK rules are the same in both builds.
The scanner's per-bit timing depends on the software driver's speed profile,
which differs per board only in menu wording.  The v3 path is **not verified**.