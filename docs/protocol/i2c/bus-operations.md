# I2C — bus operations and command reference

Part of the I2C documentation set (`docs/protocol/i2c/`). This page owns the
on-bus terminal commands: start, stop and repeated start; byte writes and reads;
the deferred ACK/NACK model; the address scanner; and one worked EEPROM
transaction. Shared prompt grammar — value entry, repetition `:N`, width `;N`,
display format `o`, bit order, `(n)` macros — is owned by
[docs/generic/](../../generic/README.md); only the I2C effect is described here.
Clock-stretch failures are in [clock-stretching.md](clock-stretching.md); the
on-board EEPROM is in [onboard-eeprom.md](onboard-eeprom.md).

## I2C command map

| Input | I2C effect | Details |
|---|---|---|
| `[` | start condition; prints `I2C START BIT` | [Start and stop](#start-stop-and-repeated-start) |
| `{` | same as `[` | [Start and stop](#start-stop-and-repeated-start) |
| `]` | stop condition; prints `I2C STOP BIT` | [Start and stop](#start-stop-and-repeated-start) |
| `}` | same as `]` | [Start and stop](#start-stop-and-repeated-start) |
| `0xNN`, `0b…`, decimal | write one byte; print the slave's ACK/NACK | [Writing a byte](#writing-a-byte) |
| `value:N` | write the same byte N times | [generic/values.md](../../generic/values.md) |
| `r`, `r:N` | read one or N bytes; the ninth bit is deferred | [Reading a byte](#reading-a-byte-the-acknack-model) |
| `rx`, `rd`, `rb`, `rw` | one-shot read display format | [generic/values.md](../../generic/values.md) |
| `;N` | set the width; software clocks N bits | [Bit-level commands and width](#bit-level-commands-and-width) |
| `(0)` | list the mode macros | [macros.md](macros.md) |
| `(1)` | scan every 8-bit address | [Address scanner](#address-scanner-macro-1) |
| `(2)` | enter the sniffer | [sniffer.md](sniffer.md) |
| `(3)` | v4: switch to the on-board EEPROM bus; needs Hardware | [onboard-eeprom.md](onboard-eeprom.md) |
| `(4)` | v4: clear on-board write protect; after `(3)` | [onboard-eeprom.md](onboard-eeprom.md) |
| `W` / `w` | target supply on / off; `w` also runs the mode cleanup | [entry-and-setup.md](entry-and-setup.md#bring-up) |
| `P` / `p` | pull-up resistors on / off | [wiring.md](wiring.md#the-pull-up-is-required) |
| `e1` / `e2` / `e3` | v4 VPU source: none / 3.3 V / 5 V | [wiring.md](wiring.md#feeding-the-pull-up-voltage-vpullup) |
| `a` / `A` / `@` | AUX pin low / high / read; not SDA or SCL | [generic/commands.md](../../generic/commands.md#elements) |
| `v` | pin and rail table; its bus columns are `SCL` and `SDA` | [generic/commands.md](../../generic/commands.md#navigation-display-and-info) |
| `i` | status block, including the `I2C (mod spd)` header | [entry-and-setup.md](entry-and-setup.md#the-settings-header) |
| `o` | display format; the scanner and `READ:` lines follow it | [generic/values.md](../../generic/values.md) |
| `/` `\` `-` `_` `.` `^` `!` | no bus effect; prints an error | [Bit-level commands](#bit-level-commands-and-width) |
| `m` | leave or re-enter the mode; resets the bus | [generic/navigation.md](../../generic/navigation.md) |
| `#` / `$` | reset / bootloader | [generic/commands.md](../../generic/commands.md) |

## Start, stop and repeated start

| Input | Action | Firmware output |
|---|---|---|
| `[` | start condition | `I2C START BIT` |
| `{` | start condition (same as `[`) | `I2C START BIT` |
| `]` | stop condition | `I2C STOP BIT` |
| `}` | stop condition (same as `]`) | `I2C STOP BIT` |

The braces are the shared grammar's read-oriented start/stop forms. **I2C has
no read-back flag**: `{` is not different from `[`, and `}` is not different
from `]`. Use either form.

A start or stop that resolves a still-pending read ACK prints `NACK` first and
sends a NACK bit, then sends the condition:

```
I2C>r
READ: 0xAA
I2C>]
NACK
I2C STOP BIT
I2C>
```

If the condition cannot be completed, the command reports `I2C bus error`,
releases both pins, and is abandoned; the `I2C START BIT` / `I2C STOP BIT` line
is not printed. See [clock-stretching.md](clock-stretching.md).

### Repeated start

A second `[` while a transaction is still open — that is, with no `]` in
between — sends a **repeated start**. On the wire it is a new start condition
after the previous byte, not a stop followed by a start. The firmware prints
the same `I2C START BIT` line and keeps the transaction open.

```
I2C>[
I2C START BIT
I2C>0xA0
WRITE: 0xA0 ACK
I2C>0x00
WRITE: 0x00 ACK
I2C>[
I2C START BIT
I2C>0xA1
WRITE: 0xA1 ACK
I2C>r
READ: 0xAA
I2C>]
NACK
I2C STOP BIT
```

Repeated start is the normal way to switch from a pointer write to a read on a
register device without releasing the bus. If a read ACK is still pending, the
repeated start first prints `NACK` and sends it, which ends the read part.

There is no separate repeated-start character and no different output line; the
distinction is only whether a stop was sent before the start.

## Writing a byte

A number written in the mode is one I2C byte, and the slave's ACK is checked
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

The `ACK`/`NACK` here is the **slave's** response to the byte just written.

**A write also resolves a pending read bit first.** The `WRITE: ` header and
the value print first, then the deferred ACK of the previous read, then the
slave's response to this write. Both ACKs look the same, so read them in order:

```
I2C>r
READ: 0xAA
I2C>0xA0
WRITE: 0xA0 ACK ACK
```

The first `ACK` belongs to the previous read byte (sent before this write); the
second is the slave's ACK of `0xA0`. A NACK in the slave position reads:

```
I2C>r
READ: 0xAA
I2C>0x60
WRITE: 0x60 ACK NACK
```

The value is masked to the current width before it is sent (8 bits by default),
so a value above `0xFF` is truncated. The shared `:N` repeat sends the same
value N times; the `WRITE: ` prefix prints **once** for the line, and each
value is followed by its own ACK or NACK:

```
I2C>0x55:3
WRITE: 0x55 ACK0x55 ACK0x55 ACK
I2C>
```

## Reading a byte: the ACK/NACK model

**Quick tell:** if a `READ:` line shows **two spaces** after the colon, the
`ACK`/`NACK` that follows is the deferred ninth bit of the **previous** byte;
the value printed after it is this byte.

`r` reads one byte; `r:N` reads N bytes. The read byte's **ninth bit is not
sent when the byte is read**. The Bus Pirate remembers it as *pending* and
sends it with the next command:

| Next command | What happens to the pending bit |
|---|---|
| a read (`r`) or a write (value) | sent as **ACK**, then the new byte |
| a start (`[` / `{`) | sent as **NACK**, then the start |
| a stop (`]` / `}`) | sent as **NACK**, then the stop |
| a command that does not touch the bus (`v`, `o`, `P`, `t`, `i`, `(0)`, …) | stays pending |
| a macro that runs bus traffic (`(1)`, `(2)`, or `(3)` in Hardware) | the macro first sends NACK and a stop |
| `w` (supply off) | cleared by the mode cleanup, without a stop |

This is why reading two bytes in a row prints an `ACK` before the second value:

```
I2C>r
READ: 0xAA
I2C>r
READ:  ACK 0xBB
I2C>]
NACK
I2C STOP BIT
```

The two spaces after `READ:` are the header followed by the deferred `ACK`; the
final `]` resolves the second byte's pending bit as `NACK` and then sends the
stop.

Notes:

- `r:N` prints one `READ: ` header. The **first** byte prints directly after it;
  each later byte is preceded by the deferred ACK of the previous one. The last
  byte's bit stays pending.
- A single write does not defer anything: the slave's ACK is read during the
  write itself.
- No explicit "pending" warning is printed when a read is left unresolved; the
  state shows only as the `ACK`/`NACK` prefix or as the ACK inside the next
  write line.
- While the ninth bit is pending, the transfer is still open and the bus is not
  idle. Resolve it with `]` (or `[`) before a bring-up command (`W`, `e`, `P`)
  or a mode change. A mode change runs the mode cleanup and releases the bus,
  which ends the transaction abruptly.
- `w` clears the pending flag but does not send a stop or release a held SCL.
  Resolve the read first if the bus must be left idle.
- A read that times out is not reported as `NACK`. The read returns `0xFF`, the
  command reports `I2C bus error`, and the line is abandoned — see
  [clock-stretching.md](clock-stretching.md).

## Address scanner (macro 1)

The scanner checks every 8-bit address from `0x00` to `0xFF` with a start, the
address byte, an ACK read, and a stop. Each hit prints the raw 8-bit address,
then the 7-bit address in parentheses with `W` (R/W bit 0) or `R` (R/W bit
1). All hits print on one line.

**Hit example** — default HEX display format:

```
I2C>(1)
Searching I2C address space. Found devices at:
0xA0(0x50 W) 0xA1(0x50 R)
I2C>
```

The printed values follow the current display format (`o`); with decimal
display the same hit prints `160(80 W) 161(80 R)`.

**One device prints twice.** A single chip at 7-bit `0x50` answers both its
write address (`0xA0`) and its read address (`0xA1`), so the scan lists it once
with `W` and once with `R`. That is one device, not two.

**Empty bus** (no device responds): the header prints, no addresses follow, and
the prompt returns.

Behaviour:

- Before scanning, both lines are released high through the pull-up. The
  pre-check trips **only when a line still reads low** (a short, or a target
  holding it); it prints the header, then
  `Warning: *Short or no pull-up`, and returns **without scanning**. A floating
  line that reads high passes the check, the scan runs, and every address ACKs
  — the no-pull-up symptom in [wiring.md](wiring.md).
- If a read ACK is pending when `(1)` is issued, the scanner first sends NACK
  and a stop, so the scan starts on an idle bus.
- Every probe ends with a stop, hit or no hit, so the bus is idle between
  addresses.
- For a **read** address that ACKs, the scanner reads one byte, NACKs it, and
  sends a stop. This ends the transaction cleanly and avoids ghost addresses
  from a slave that started driving data.
- The scan uses the selected speed profile and, in the software driver, the
  current width. A scan at a width other than 8 sends N-bit addresses and
  normally finds nothing; return the width to 8 first.
- After `(3)`, the scan follows the on-board EEPROM bus instead of the external
  pins; see [onboard-eeprom.md](onboard-eeprom.md).
- The warning is printed **only** by this scanner. A start, stop, read or write
  that cannot complete reports `I2C bus error` instead.

## Worked session: EEPROM write and read-back

The device is a 24xx64-style EEPROM at 7-bit `0x50` (write `0xA0`, read `0xA1`)
with a two-byte memory address. The bring-up is in
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

The first data bytes after the address are the **memory address**, not stored
data. The chip commits the burst at the **STOP** and then runs an internal write
cycle; until it finishes, it answers its address with NACK. Poll it:

```
I2C>[
I2C START BIT
I2C>0xA0
WRITE: 0xA0 NACK
I2C>]
I2C STOP BIT
```

Repeat the start/address/stop poll until `0xA0` is ACKed. Then set the pointer
and read with a repeated start:

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

Set the pointer again before each read transaction; bytes within one
transaction auto-increment. A new read address does not continue the previous
write's pointer. Keep a burst inside one 32-byte page. The same transaction in
binary form is in [binary.md](binary.md#worked-exchange); the v4 on-board chip
uses this same sequence once `(3)` has selected its bus — see
[onboard-eeprom.md](onboard-eeprom.md).

## Bit-level commands and width

I2C has no free-running clock or data-pin model, so the shared bit-level
commands have no bus effect here:

| Input | Firmware prefix | Result |
|---|---|---|
| `/` `\` | `CLOCK, 1` / `CLOCK, 0` | no clock change; no-effect error |
| `-` `_` | `DATA OUTPUT, 1` / `DATA OUTPUT, 0` | no data change; no-effect error |
| `.` | `DATA STATE: ` | no state read; no-effect error |
| `^` | `CLOCK TICKS: ` | no clocks; no-effect error |
| `!` | `READ BIT: ` | no read; no-effect error |

Each prints its normal prefix and then
`ERROR: command has no effect here`; the line is followed by
`Syntax error at char N`. The prefix and the error can end up on the same line,
so the output looks mangled. Use the byte and bit forms above instead.

### Width `;N`

I2C is a byte protocol: leave the width at the default 8. The shared `;N`
suffix does more than change the display here:

| Driver | Any width other than 8 |
|---|---|
| Software | a value or read transfers exactly N bits; the slave sees N clocks, not a byte |
| Hardware | the engine still transfers 8 bits; only the display and bit-order window change |
| Scanner, software | the scan writes N-bit addresses and normally finds nothing |
| Scanner, hardware | the engine still writes 8-bit addresses |

A mode change or reset restores 8 bits. If a transfer starts behaving as if
every byte is shifted, check the width before anything else.

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| start / stop / repeated start | same | same |
| byte write and read | same | same |
| deferred ninth bit | same | same |
| scanner | same, software profile | same; follows the on-board EEPROM redirect |
| bit-level commands | no effect | no effect |
| width other than 8 on the wire | N bits (software) | software N bits / hardware still 8 |

## Related docs

- [clock-stretching.md](clock-stretching.md) — timeout, bus error,
  recovery.
- [macros.md](macros.md) — the scanner, sniffer and EEPROM macros.
- [onboard-eeprom.md](onboard-eeprom.md) — the v4 on-board chip and macros 3/4.
- [binary.md](binary.md) — the same operations from a host.
- [docs/generic/values.md](../../generic/values.md) — `:N`, `;N` and `o`.
