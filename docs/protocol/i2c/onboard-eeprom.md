# I2C — on-board EEPROM (v4)

Part of the I2C documentation set (`docs/protocol/i2c/`). This page owns the v4
on-board EEPROM: its geometry, address sequence, write protect, and the macro
redirect that reaches it. The macro list is in [macros.md](macros.md); the
shared byte operations are in [bus-operations.md](bus-operations.md); the
**external** EEPROM host frontend is in [host-eeprom.md](host-eeprom.md).

## The chip

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
driver; macro `(4)` clears the write protect.

### What the array is for

The 8192-byte array is **opaque board-local storage**. The firmware does not
store settings, version strings, CRCs or slots in it, does not mount a
filesystem on it, and does not read it during normal operation. Outside the
terminal macros, the only firmware access is the preserving self-test at offset
0, which reads the byte, writes its complement, then restores and verifies the
original.

Clearing the write protect and writing the array therefore does not change the
Bus Pirate's configuration. The contents are yours to use; nothing in the
firmware depends on them.

## Reaching it: macros 3 and 4

Macro 3 selects the internal bus; macro 4 clears the write protect. Both need
the mode to have been entered with **2 Hardware**:

```
I2C>(3)
Now using on-board EEPROM I2C interface
I2C>(4)
On-board EEPROM write protect disabled
I2C>
```

The order matters. `(4)` does nothing unless `(3)` has run in this session.

| Operation | After `(3)` |
|---|---|
| start, stop, repeated start | internal bus (hardware engine) |
| byte write and read | internal bus (hardware engine) |
| address scanner `(1)` | internal bus |
| sniffer `(2)` | still the external SDA/SCL pins |
| leaving or re-entering the mode | back to the external bus |

The redirect covers the byte operations and the scanner. The sniffer is the one
exception. The redirect and the cleared write protect both last only for the
current session; a reset or a mode change returns to the external bus and
re-asserts the protect. `w` (supplies off) does not re-assert the protect.

From the Software driver `(3)` is refused, and `(4)` is silent because the
redirect was never made:

```
I2C>(3)
Onboard EEPROM requires hardware I2C
Syntax error at char 3
I2C>
```

## Address sequence

A memory access starts with the device address and then the two-byte memory
address, high byte first:

```
write: 0xA0 addr_hi addr_lo data ...
read:  0xA0 addr_hi addr_lo   (repeated START)
       0xA1 read ...
```

The first data bytes after the address are the **memory address**, not stored
data. Writes commit at **STOP**, then the chip runs an internal write cycle.
Until it finishes it NACKs its address; poll `0xA0` (start, address, stop) until
it ACKs before the next access. Keep a burst within one 32-byte page: the chip's
page write wraps at the page boundary.

## Example: selecting the on-board bus

This example assumes the mode was entered with `m4 2 <speed>`.

```
I2C>(3)
Now using on-board EEPROM I2C interface
I2C>(4)
On-board EEPROM write protect disabled
I2C>[
I2C START BIT
I2C>0xA0
WRITE: 0xA0 ACK
```

From there the byte sequence is exactly the external EEPROM session in
[bus-operations.md](bus-operations.md#worked-session-eeprom-write-and-read-back):
address high, address low, data, stop, then the `0xA0` write-cycle poll, then a
repeated start and a read of `0xA1`. The only difference is which bus `(3)`
selected. Keep the burst inside the 32-byte page.

## Is the chip there?

A missing chip does not make `(3)` fail: the macro switches the internal bus
without probing the part. Use the scanner to test the chip:

1. Enter with Hardware and run `(3)`.
2. Run `(1)`. A hit at `0x50` is the on-board EEPROM answering.
3. If the header prints but no address follows, the chip is absent or not
   responding. Re-check the driver: `(3)` is refused under Software, so a
   successful redirect with an empty scan is a chip-side problem.

The HiZ self-test (`~`) also checks the on-board EEPROM as part of its v4
checks; run it from `HiZ>` before a first write if the board's history is
unknown. See [the self-test set](../../capabilities/selftest/README.md).

## Limits

- **v4 only.** v3 has no hardware engine, no on-board EEPROM and no macros 3/4.
- The array is not reachable from the external pins. A scan before `(3)` does
  not see it; a scan after `(3)` is not a scan of your external bus.
- The `bp eeprom` host frontend cannot reach it. Use the terminal macros.
- The chip's 7-bit address `0x50` is also a common external EEPROM address. If a
  scan after `(3)` finds `0x50`, that is the on-board part, not an external one.
- Multi-byte bursts wrap inside the 32-byte page; split the data at page
  boundaries.

## Related docs

- [macros.md](macros.md) — the macro list and the redirect rules.
- [bus-operations.md](bus-operations.md#worked-session-eeprom-write-and-read-back)
  — the shared byte sequence.
- [host-eeprom.md](host-eeprom.md) — the external EEPROM frontend.
- [wiring.md](wiring.md) — the external bus and Vpullup.
- [edge-cases.md](edge-cases.md#macros-and-the-on-board-eeprom) — session state
  and write-cycle failures.
