# I2C — EEPROM work and the Windows client

Part of the I2C documentation set (`docs/i2c/`).  This page covers the two
EEPROM stories in I2C mode: the Bus Pirate v4's **on-board** EEPROM macros, and
the **Windows I2C EEPROM tool** used to read/write an external I2C EEPROM
through the Bus Pirate.

## On-board EEPROM (v4) — macros 3 and 4

The Bus Pirate v4 carries a small EEPROM-style I2C flash chip (about **8 kB**)
on the board, wired to its **own internal I2C bus** — separate pins from the
external SDA/SCL.  Macros 3 and 4 talk to it.  The macro semantics are in
[macros.md](macros.md); in short:

| Macro | Action |
|---|---|
| `(3)` | switch the active interface to the on-board EEPROM, `Now using on-board EEPROM I2C interface` |
| `(4)` | clear the on-board EEPROM write protect, `On-board EEPROM write protect disabled` (only after `(3)`) |

After `(3)`, start/stop/read/write drive the internal bus.  The EEPROM answers
at 7-bit address **`0x50`** (write `0xA0`, read `0xA1`), like a 24LCxx part.
The on-board EEPROM is **write-protected until `(4)` is run**, and a reset
re-asserts the protect.  The internal bus is driven by the hardware I2C
engine, so these macros belong with the Hardware driver.

These macros are **not verified**.  They require a v4 with a populated on-board
EEPROM.

## The Windows I2C EEPROM client

`I2CEEPROMWIN` is a Windows command-line program that reads or writes an
**external** I2C EEPROM through the Bus Pirate.  It drives the Bus Pirate in
binary I2C mode (see [binary.md](binary.md)); it is a host-side client, not a
mode.

### Invocation

```
i2ceepromwin [com?] [chip addr 0-7] [R/W/Rs/Ws] [file] (read len) (mem addr)
```

| Argument | Meaning |
|---|---|
| `com?` | the Bus Pirate serial port, e.g. `COM6` |
| `chip addr 0-7` | the device select bits A0–A2 (0 = base `0x50`) |
| `R` / `W` | read / write a device with a **two-byte** memory address (e.g. 24FC64) |
| `Rs` / `Ws` | read / write a device with a **one-byte** address (e.g. 24LC08) |
| `file` | output file (read) or input file (write) |
| `read len` | number of bytes to read (read mode; required) |
| `mem addr` | starting memory address |

- The port is opened at **115200 8N1**.  The tool enters binary I2C, enables
  the power supplies and the pull-ups, and on v4 applies the on-board 3.3 V
  pull-up feed first (so a removed VPU jumper is handled automatically).
- The write file is space-separated hex with no line breaks, e.g.
  `A5 5A C3 3C 01 FE`.
- Write mode retries a byte until the EEPROM acknowledges it, then reports
  `N bytes written with M retries`.
- The client's raw binary command sequence is exactly the `0x02` start, `0x1x`
  bulk-write, `0x04` read and `0x06`/`0x07` ACK/NACK commands documented in
  [binary.md](binary.md).

## Related

- Binary command set: [binary.md](binary.md).
- Pull-ups and VPU: [wiring.md](wiring.md).
- The on-board EEPROM macros: [macros.md](macros.md).