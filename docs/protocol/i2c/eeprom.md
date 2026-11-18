# I2C — on-board EEPROM and the Windows client

Part of the I2C documentation set (`docs/protocol/i2c/`). This page owns the
on-board EEPROM's geometry, address sequence and write protect, and the Windows
`I2CEEPROMWIN` client for **external** EEPROMs. The macro list and semantics are
in [macros.md](macros.md); the binary command set is in [binary.md](binary.md).

## On-board EEPROM (v4)

The Bus Pirate v4 carries a **24xx64** on its own internal I2C bus, separate
from the external SDA/SCL pins:

| Property | Value |
|---|---|
| device | 24xx64 |
| size | 8K × 8 (8192 bytes) |
| memory address | two bytes, `0x0000`–`0x1FFF` |
| page size | 32 bytes |
| 7-bit address | `0x50` (write `0xA0`, read `0xA1`) |
| supply | 3.3 V, from the board |
| bus pull-ups | on-board, on the internal bus |
| write protect | its own pin, asserted at reset and on a mode change |

Nothing external is needed to use it: no header wiring, no Vpullup feed, no `W`,
no pull-up resistors. Macro `(3)` switches the bus and requires the **Hardware**
driver; macro `(4)` clears the write protect. Both are described in
[macros.md](macros.md).

### Address sequence

A memory access starts with the device address and then the two-byte memory
address, high byte first:

```
write: 0xA0 addr_hi addr_lo data ...
read:  0xA0 addr_hi addr_lo   (repeated START)
       0xA1 read ...
```

Writes commit at **STOP**, then the chip runs an internal write cycle. Until it
finishes it NACKs its address; poll `0xA0` (start, address, stop) until it ACKs
before the next access. Keep a burst within one 32-byte page: the chip's page
write wraps at the page boundary.

### Example — write and read back

```
I2C>(3)
Now using on-board EEPROM I2C interface
I2C>(4)
On-board EEPROM write protect disabled
I2C>[
I2C START BIT
I2C>0xA0
WRITE: 0xA0 ACK
I2C>0x00
WRITE: 0x00 ACK
I2C>0x20
WRITE: 0x20 ACK
I2C>0x5A
WRITE: 0x5A ACK
I2C>]
I2C STOP BIT
I2C>
```

Then poll the address until it ACKs, set the pointer again with a repeated
start, and read:

```
I2C>[
I2C START BIT
I2C>0xA0
WRITE: 0xA0 NACK
I2C>]
I2C STOP BIT
```

Repeat that poll until `0xA0` is ACKed, then:

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
READ: 0x5A
I2C>]
NACK
I2C STOP BIT
I2C>
```

The write protect stays cleared for the session. A reset or a mode change
re-asserts it, so `(3)` and `(4)` must be repeated in a new session.

### What follows the redirect

| Operation | After `(3)` |
|---|---|
| start, stop, repeated start | internal bus (hardware engine) |
| byte write and read | internal bus (hardware engine) |
| address scanner `(1)` | internal bus |
| sniffer `(2)` | still the external SDA/SCL pins |
| leaving or re-entering the mode | back to the external bus |

The on-board chip's address `0x50` is also a common external EEPROM address. A
scan after `(3)` finds `0x50` even with nothing on the external pins; a scan
before `(3)` does not see the on-board chip.

The macro strings, the on-board part and the write-protect path are **not
verified**; they need a v4 with a populated on-board EEPROM.

## The Windows I2C EEPROM client

`I2CEEPROMWIN` is a Windows command-line program that reads or writes an
**external** I2C EEPROM through the Bus Pirate in binary I2C mode. It cannot
reach the on-board chip; use macros 3/4 for that. The program's own usage line
spells the executable `i2ceepromwin`.

### Invocation

The usage line is the program's own:

```
i2ceepromwin [com?] [chip addr 0-7] [R/W/Rs/Ws] [file] (read len) (mem addr)
```

| Argument | Meaning |
|---|---|
| `com?` | the serial port, e.g. `COM6` |
| `chip addr 0-7` | device select bits A0–A2 (0 = base `0x50`) |
| `R` / `W` | read/write a device with a **two-byte** memory address |
| `Rs` / `Ws` | read/write a device with a **one-byte** address |
| `file` | output file (read) or input file (write) |
| `read len` | bytes to read (read mode; required) |
| `mem addr` | starting memory address (default 0) |

`R`/`W` match 24xx64-class parts with two address bytes; `Rs`/`Ws` match
one-byte-address parts such as the 24LC08. The choice must match the chip, or
the tool addresses the wrong location.

### Behaviour

- The port is opened at **115200 8N1**.
- The tool sends 20 NUL bytes, then `0x02` (enter I2C), `0x51` (on-board 3.3 V
  pull-up feed, v4) and `0x4C` (supply, pull-ups and AUX low). It checks for
  `BBIO1I2C1` in the reply. On a v3 the `0x51` reply is `0x00` and is harmless.
  `0x4C` drives AUX low and the tool never sends `0x09 0x02`, so leave AUX
  unjumpered when using it; a jumpered AUX clamps SDA/SCL.
- Writes are **one byte per transaction**: start, device address, memory
  address, data, stop. Page writes are not used, so a write burst cannot cross a
  page boundary. A byte is retried until the chip acknowledges it; the summary
  line reports the byte count and the retry count.
- Reads set the memory pointer with start, device address, address bytes, stop,
  then send start and the read address and read the requested length. All but
  the last byte are ACKed; the last is NACKed before the stop.
- The write file is whitespace-separated hex tokens; spaces or newlines, up to
  four characters per token, with or without `0x`:

```
A5 5A C3 3C 01 FE
```

- Read output is printed to the terminal as hex bytes and written to the file as
  space-separated hex tokens.
- The data-phase command sequence is `0x02` start, `0x03` stop, `0x1x`
  bulk-write, `0x04` read and `0x06`/`0x07` ACK/NACK; setup uses `0x51` and
  `0x4C`; exit uses `0x00` and `0x0F`. These are the commands in
  [binary.md](binary.md).

### Limits

- It drives the external bus only. It cannot select the on-board EEPROM bus.
- It always uses the binary `I2C1` path, which is the software driver at the
  maximum profile. There is no speed or VPU choice in the tool.
- It does not use page writes, so a large file is slow by design.
- The write retry loop has no limit; a missing, unpowered or miswired chip
  leaves the tool retrying until it is interrupted.
- It has no verify pass beyond the per-byte ACK retry; read the file back to
  confirm the contents.

## Edge cases

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `(4)` before `(3)` | nothing | the redirect is a precondition |
| `(3)` in Software | `Onboard EEPROM requires hardware I2C` | re-enter with `m4 2 <speed>` |
| A write is issued, then read immediately | the address NACKs, or old data is read | poll `0xA0` until ACK, then set the pointer again |
| A burst crosses a 32-byte boundary | the data wraps inside the page | split the burst at the page boundary |
| The mode is re-entered after `(4)` | the chip is protected again | run `(3)` and `(4)` again |
| `(2)` is run after `(3)` expecting EEPROM traffic | no traffic is shown | the sniffer watches the external pins only |
| The tool is pointed at the on-board part | nothing responds | the tool is external-bus only |
| `R` is used on a one-byte-address part | wrong data or no ACK | use `Rs`/`Ws` for that chip |
| The write file has a token above `FF` | the tool reports the bad token and stops | use byte values only |
| The write file has extra separators | tokens are still parsed | spaces and newlines are accepted |

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| on-board EEPROM and macros 3/4 | no | yes |
| internal bus | no | I2C1, on-board pull-ups |
| `I2CEEPROMWIN` | yes, external bus | yes, external bus |
| tool's `0x51` step | answered `0x00`, harmless | applied |

The v3 column is **not verified**.

## Related docs

- [macros.md](macros.md) — the macro list and the redirect rules.
- [bus-operations.md](bus-operations.md) — the terminal transaction shape.
- [binary.md](binary.md) — the `I2C1` commands the tool uses.
- [wiring.md](wiring.md) — the external bus and Vpullup.
