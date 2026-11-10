# I2C — clock stretching and bus errors

Part of the I2C documentation set (`docs/protocol/i2c/`).  This page owns the
per-bit clock-stretch wait, the timeout, pin release, and recovery in both
drivers.

## What clock stretching is

I2C slaves may **hold SCL low** after the master releases it, to pause the
master while they prepare data.  The master must not continue until the slave
releases SCL.  A master that ignores stretching reads garbage from slow
devices.

## Software driver — per-bit wait, bounded

The software (bit-banged) driver waits for SCL to rise on **every** bit, not
only at the acknowledge slot:

- writing a byte: each bit is held after the clock is released until SCL
  reads high again;
- reading a byte: the data bit is sampled only after SCL is released;
- the acknowledge bit uses the same wait.

The wait is a fixed poll count, not an exact wall-clock deadline.  On the
documented v4 build it is roughly **25 ms**, enough for parts with long
stretches such as the SHT3x family.

## Three outcomes

| Outcome | What happens |
|---|---|
| SCL is released within the bound | the bit completes and the transfer continues |
| SCL still low at the bound | abort: both pins released, `I2C bus error`, command abandoned |
| the next command runs | it starts from a fresh error state; if SCL is still held, it waits and times out again |

A timed-out acknowledge slot is **not** reported as `NACK`.  The read path
returns an all-ones placeholder (`0xFF`) and the command reports the bus
error; a write prints no ACK/NACK for the failed byte.

A start that cannot complete because SCL is held:

```
I2C>[
I2C bus error
Syntax error at char 1
I2C>
```

The `Syntax error` line is the abandoned command line, not a separate fault.

## Recovery

- **Software driver:** the next bus operation clears the error flag and retries
  from the current line state.  Clear the cause (release the slave, remove the
  short) before retrying.
- **Hardware driver:** the engine uses the same bounded polling budget for
  every master sequence (start/repeated start, stop, ACK, transmit, receive,
  and the initial idle-line check).  A bus-collision, write-collision or
  receive-overflow status bit (`BCL` / `IWCOL` / `I2COV`) aborts the
  operation: the selected module is disabled, its pins are released, and the
  pending ACK state is dropped.
- A new hardware START after a fault re-initialises the bus.  If SDA is still
  held low (a slave stranded mid-read), that START first attempts the
  nine-clock bus-clear: nine released-SDA clocks, then a STOP, with a bounded
  SCL wait on each edge.  If SDA is still held after that, the new START fails
  too.

## Binary status — `0x0A`

The binary `I2C1` sub-protocol reports the last bus operation with `0x0A`:

| Reply | Meaning |
|---|---|
| `0x01` | no bus fault |
| `0x00` | a timeout or bus fault occurred |

Reading `0x0A` does **not** clear the status; the next bus operation does.
A bulk write latches a fault for the whole command: the remaining payload is
consumed and answered NACK without further bus activity.  A normal slave NACK
is not a bus fault.  The command set is in [binary.md](binary.md).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| software driver stretch wait | yes | yes |
| ~25 ms bounded timeout | yes | yes |
| hardware driver stretch handling | optional build (not in the default v3 build) | yes |

The v3 column is **not verified**.
