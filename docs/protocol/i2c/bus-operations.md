# I2C — bus operations

Part of the I2C documentation set (`docs/protocol/i2c/`).  This page owns the
on-bus commands: start/stop, byte read/write, the ACK/NACK model, and the
address scanner.  Shared prompt grammar — value entry, repetition `:N`,
width `;N`, wrappers, `(n)` macros — is owned by
[docs/generic/](../../generic/README.md); only the I2C effect is described here.

## Start and stop

| Input | Action | Firmware output |
|---|---|---|
| `[` | start condition | `I2C START BIT` |
| `{` | start-with-read form; same I2C start | `I2C START BIT` |
| `]` | stop condition | `I2C STOP BIT` |
| `}` | stop-from-read form; same I2C stop | `I2C STOP BIT` |

A start or stop that resolves a still-pending read ACK first prints `NACK` and
sends a NACK bit (see [Reading a byte](#reading-a-byte--the-acknack-model)).

If the bus cannot complete the condition — for example a slave holds SCL low
past the bound — the command reports `I2C bus error`, releases both pins, and
is abandoned; see [clock-stretching.md](clock-stretching.md).

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

**A write also resolves a pending read bit first.**  The line prints the
`WRITE: ` header and the value, then the deferred ACK of the previous read,
then the slave's response to this write.  Both ACKs look the same, so read
them in order:

```
I2C>r
READ: 0xAA
I2C>0xA0
WRITE: 0xA0 ACK ACK
```

The first `ACK` belongs to the previous read byte (sent before the write);
the second is the slave's ACK of `0xA0`.  A NACK in the slave position reads:

```
I2C>r
READ: 0xAA
I2C>0x60
WRITE: 0x60 ACK NACK
```

## Reading a byte — the ACK/NACK model

**Quick tell:** if a `READ:` line shows **two spaces** after the colon, the
`ACK`/`NACK` that follows is the deferred ninth bit of the **previous** byte;
the value printed after it is this byte.

`r` reads one byte; `r:N` reads N bytes.  The read byte's **ninth bit is not
sent when the byte is read**.  The Bus Pirate remembers it as *pending* and
sends it with the next command:

- the next **read or write** sends the pending bit as **ACK**;
- a **start or stop** sends it as **NACK** (ending the read);
- a command that does not access the bus (`v`, `o`, `P`, `t`, `(0)`, …)
  leaves it pending.

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

The two spaces after `READ:` are the header followed by the deferred `ACK`;
the final `]` resolves the second byte's pending bit as `NACK` and then sends
the stop.

Notes:

- A bulk read `r:N` prints one `READ: ` header; the **first** byte prints
  directly after it.  Each later byte is preceded by the deferred ACK of the
  previous one.  The last byte's bit stays pending until the next command.
- A single write (`0xNN` / `0b…` / decimal) does not defer anything: the
  slave's ACK is read during the write itself.
- No explicit "pending" warning is printed when a read is left unresolved;
  the state shows only as the `ACK`/`NACK` prefix or as the ACK inside the
  next write line.
- If you break a transaction early, send a stop (`]`) or a start (`[`) to
  resolve the pending bit as NACK rather than leaving it pending.

## Address scanner — macro `(1)`

The scanner checks every 8-bit address (`0x00`–`0xFF`) with a start, the
address byte, and an ACK read.  Each hit prints the raw 8-bit address, then
the 7-bit address in parentheses with `W` (R/W bit 0) or `R` (R/W bit 1).
All hits print on one line.

**Hit example** — default HEX display format:

```
I2C>(1)
Searching I2C address space. Found devices at:
0xA0(0x50 W) 0xA1(0x50 R)
I2C>
```

The printed values follow the current display format (`o`); with decimal
display the same hit prints `160(80 W) 161(80 R)`.

**Empty bus** (no device responds): the header prints, then one blank line,
then the prompt.

```
I2C>(1)
Searching I2C address space. Found devices at:

I2C>
```

Behaviour:

- Before scanning, both lines are driven high through the pull-up.  The
  pre-check trips **only when a line still reads low** (a short, or a target
  holding it): it prints `Warning: *Short or no pull-up` and returns
  **without scanning**.  A floating line that reads high passes the check,
  the scan runs, and every address ACKs — the no-pull-up symptom in
  [wiring.md](wiring.md).
- For a **read** address that ACKs, the scanner reads one byte, NACKs it, and
  sends a stop.  This ends the transaction cleanly and avoids "ghost"
  addresses from a slave that started driving data.

The strings and format above are **not verified**.

## Worked session

The device is a 24xx64-style EEPROM at 7-bit `0x50` (write `0xA0`, read
`0xA1`) with a two-byte memory address.  The bring-up is in
[entry-and-setup.md](entry-and-setup.md); this is the on-bus flow.

Write `0xAA 0xBB` at address `0x0020`, commit it, then read it back:

```
I2C>[
I2C START BIT
I2C>0xA0
WRITE: 0xA0 ACK
I2C>0x00
WRITE: 0x00 ACK
I2C>0x20
WRITE: 0x20 ACK
I2C>0xAA
WRITE: 0xAA ACK
I2C>0xBB
WRITE: 0xBB ACK
I2C>]
I2C STOP BIT
```

The EEPROM starts its internal write cycle at the **STOP**; until it
finishes, it answers its address with NACK.  Poll it, then stop:

```
I2C>[
I2C START BIT
I2C>0xA0
WRITE: 0xA0 ACK
I2C>]
I2C STOP BIT
```

Repeat that start/address/stop poll until `0xA0` is ACKed.  Then set the
pointer and read:

```
I2C>[
I2C START BIT
I2C>0xA0
WRITE: 0xA0 ACK
I2C>0x00
WRITE: 0x00 ACK
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

The first data bytes after the address are the memory address, not stored
data: the write cycle commits only at STOP, and the pointer must be set again
before a read.  The same transaction in binary form is in
[binary.md](binary.md).

## v3 / v4

Start/stop, read/write and the ACK/NACK rules are the same in both builds.
Scanner timing follows the software speed profile.  The v3 path is **not
verified**.
