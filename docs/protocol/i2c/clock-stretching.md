# I2C — clock stretching and bus errors

Part of the I2C documentation set (`docs/protocol/i2c/`). This page owns the
per-bit clock-stretch wait, its timeout, `I2C bus error`, pin release and
recovery in both drivers, and the binary status command. Bus operations are in
[bus-operations.md](bus-operations.md); the binary command set is in
[binary.md](binary.md).

## What clock stretching is

I2C slaves may **hold SCL low** after the master releases it, to pause the
master while they prepare data. The master must not continue until the slave
releases SCL. A master that ignores stretching reads garbage from slow devices.

## Software driver — a bounded wait on every clock

The software (bit-banged) driver releases SCL, waits for the wire to read high,
and only then runs its high-phase delay. This applies to every clock the master
produces:

- each data bit of a written byte;
- the slave's ACK slot after a written byte;
- each data bit of a read byte;
- the master's own ACK/NACK bit after a read;
- start, repeated start and stop.

The wait is a **fixed poll count**, not an exact wall-clock deadline. On v4 it
is roughly **25 ms** per clock wait, which covers parts with long stretches
such as the SHT3x family. It is not a portable SMBus timing guarantee.

## Two outcomes

| Outcome | What happens |
|---|---|
| SCL is released within the bound | the bit completes and the transfer continues |
| SCL is still low at the bound | abort: both pins released, `I2C bus error`, command abandoned |

**After a timeout:** the next operation starts from a fresh error state. It
does not retry the abandoned command, and it times out again if SCL is still
held.

A timed-out acknowledge slot is **not** reported as `NACK`. A read returns the
all-ones value (`0xFF`) and the command reports the bus error; a write prints no
ACK/NACK for the failed byte.

### Failure output

A start that cannot complete because SCL is held:

```
I2C>[
I2C bus error
Syntax error at char 1
I2C>
```

A read that times out — the value is printed after the error line because
the read command still returns a value:

```
I2C>r
READ: I2C bus error
0xFF
Syntax error at char 1
I2C>
```

The `Syntax error` line is the abandoned command line, not a separate fault. A
stop that has a pending read ACK prints `NACK` first, then the same error,
without an `I2C STOP BIT` line.

When a **write** fails mid-byte, the error is printed before the write command
has finished its own line, so the text is appended to the unfinished header:
`WRITE: 0xA0I2C bus error`. Read the error text, not the column position.

## Recovery

- **Software driver:** the next bus operation clears the sticky error and
  starts from the current line state; it does not retry the abandoned command.
  Clear the cause (release the slave, remove the short) before retrying.
- **Hardware driver:** the engine's clock synchronisation handles the per-bit
  stretch in hardware. The firmware bounds each master sequence with the same
  polling budget — start and repeated start, stop, ACK, transmit, receive, and
  the initial idle-line check. A bus-collision, write-collision or
  receive-overflow status aborts the operation: the selected module is disabled,
  its pins are released, and the pending ACK state is dropped.
- A new hardware **START** after a fault re-initialises the bus. If SDA is still
  held low (a slave stranded mid-read), that START first attempts the
  **nine-clock bus-clear**: nine clocks with SDA released, then a STOP, with a
  bounded SCL wait on each edge. If SDA is still held after that, the new START
  fails too.
- This recovery is for the Bus Pirate's single-master use. It does not resolve
  multi-master arbitration, and it cannot free a line that is shorted to ground.

A timed-out command does **not** leave the mode. The next command runs normally
until it reaches the bus again.

## What is left on the bus

- On a timeout both SDA and SCL are released (high impedance). The bus floats to
  the pull-up level unless something else holds it.
- A read that is left with its ninth bit pending keeps the transfer open: the
  master is still holding SCL low between the data byte and the ACK bit. Resolve
  it with a start or a stop before changing the wiring, the pull-ups or the
  supply.
- A pending read bit is dropped by a timeout, by `w` (mode cleanup) and by a
  mode change. `v`, `o`, `P`, `e` and `i` leave it pending. `W` re-runs the
  driver setup but leaves the pending bit set; resolve the read with `]` first.

## Binary status — `0x0A`

The binary `I2C1` sub-protocol reports the last bus operation with `0x0A`:

| Reply | Meaning |
|---|---|
| `0x01` | no bus fault |
| `0x00` | a timeout or bus fault occurred |

Reading `0x0A` does **not** clear the status; the next real bus operation does.
A request that is refused before touching the bus (a zero or over-limit `0x08`
count) also does not clear it.

- A bulk write latches a fault for the whole command: the remaining payload is
  consumed and answered NACK without further bus activity.
- A normal slave NACK is not a bus fault.
- `0x04` keeps its single-byte reply; on a fault that byte is `0xFF`, so
  the byte alone cannot distinguish a fault from a legitimate `0xFF` read. Use
  `0x0A`.
- Binary I2C is always the software driver. The nine-clock bus-clear belongs to
  the interactive Hardware driver; there is no automatic bus-clear in the binary
  path.

The command set is in [binary.md](binary.md).

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| software stretch wait | yes, ~25 ms per clock | yes, ~25 ms per clock |
| hardware sequence bound | optional build only | yes, ~25 ms per sequence |
| nine-clock bus-clear | only with the optional hardware driver | yes |

The v3 column is **not verified**.

## Related docs

- [bus-operations.md](bus-operations.md) — the operations that can time out.
- [binary.md](binary.md) — `0x0A`, bulk writes and `0x08` failures.
- [wiring.md](wiring.md) — pull-ups and line release.
- [edge-cases.md](edge-cases.md) — the trigger/symptom tables.
