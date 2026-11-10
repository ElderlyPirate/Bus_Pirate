# I2C — on-board EEPROM and the Windows client

Part of the I2C documentation set (`docs/protocol/i2c/`).  This page owns the
on-board EEPROM's geometry and address sequence, the write protect, and the
Windows I2C EEPROM tool.  The macro list and semantics are in
[macros.md](macros.md); the binary command set in [binary.md](binary.md).

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

Macro `(3)` switches the active bus to it and requires the **Hardware**
driver; macro `(4)` clears its write protect.  Both are described in
[macros.md](macros.md).

### Address sequence

A memory access starts with the device address and then the two-byte memory
address, high byte first:

```
write: 0xA0 addr_hi addr_lo data ...
read:  0xA0 addr_hi addr_lo   (repeated START)
       0xA1 read ...
```

A read is usually one combined sequence: write the pointer, repeated start,
read address, then read.  The terminal form is shown in
[bus-operations.md](bus-operations.md) §Worked session; the binary form in
[binary.md](binary.md) §Worked exchange.

Writes commit at **STOP**, then the chip runs an internal write cycle.  Until
it finishes it NACKs its address; poll `0xA0` (start, address, stop) until it
ACKs before the next access.  Keep a burst within one 32-byte page.

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

The macro strings and the on-board chip are **not verified**; they require a
v4 with a populated on-board EEPROM.

## The Windows I2C EEPROM client

`I2CEEPROMWIN` is a Windows command-line program that reads or writes an
**external** I2C EEPROM through the Bus Pirate in binary I2C mode.

### Invocation

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
| `mem addr` | starting memory address |

The `R`/`Rs` (and `W`/`Ws`) choice must match the external chip: `R`/`W` use
two address bytes (24FC64/24xx64 class), `Rs`/`Ws` use one (24LC08 class).

- The port is opened at **115200 8N1**.  The tool enters binary I2C and, on
  v4, requests the on-board 3.3 V pull-up feed before switching on the power
  supplies and pull-ups.
- The write file is whitespace-separated hex tokens, e.g. `A5 5A C3 3C 01 FE`
  (spaces or newlines; each token may be up to four characters and may carry
  `0x`).
- Write mode retries a byte until the EEPROM acknowledges it, then reports
  `N bytes written with M retries`.
- The data-phase command sequence is `0x02` start, `0x03` stop, `0x1x`
  bulk-write, `0x04` read and `0x06`/`0x07` ACK/NACK; setup uses `0x51` and
  `0x4C`; exit uses `0x00` and `0x0F`.  These are documented in
  [binary.md](binary.md).

## Related

- Macro list and semantics: [macros.md](macros.md).
- Binary command set: [binary.md](binary.md).
- Pull-ups and VPU: [wiring.md](wiring.md).
