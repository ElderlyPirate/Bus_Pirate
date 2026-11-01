# I2C — clock stretching

Part of the I2C documentation set (`docs/i2c/`).  This is the headline
correction to the older published I2C page, which stated that v3 does not
support clock stretching.  Both the software and the hardware driver handle
it.

## What clock stretching is

I2C slaves may **hold SCL low** after the master releases it, to pause the
master while they prepare data (clock stretching).  The master must not
continue until the slave releases SCL.  A master that ignores stretching reads
garbage from slow devices.

## Software driver — per-bit wait, bounded

The Bus Pirate's software (bit-banged) I2C driver waits for the slave to
release SCL on **every** bit, not only at the acknowledge slot:

- while **writing** a byte, each bit is held after the clock is raised until
  SCL reads high again;
- while **reading** a byte, the data bit is sampled only after SCL is released;
- the same wait runs for the ACK bit.

A slave that holds SCL low **forever** cannot hang the Bus Pirate.  There is a
fixed timeout of about **25 ms** (chosen to cover the SMBus timeout minimum and
slow parts such as the SHT3x family).  After the timeout the bit is abandoned,
the event is counted internally, and the bus moves on.  If the stuck bit was
an acknowledge slot, the read is reported as **NACK** (not ACK), so a timed-out
acknowledge is never mistaken for a valid one.

### Bench evidence (v4, emulated slave, 2026-10-26/27)

The bench rig is an emulated slave with a configurable clock stretch applied at
every ACK slot.  With a 200 µs stretch on every acknowledge, full write/read
and re-point/read frames completed with all ACKs and read back the expected
bytes; the slave counted the stretches it applied.  With the bus **held low
and never released**, a byte command still returned a reply, with a measured
wait in the tens of milliseconds (well under the ~25 ms timeout plus overhead),
and the bus recovered once the slave released the lines — no hang.

| Case | Observed |
|---|---|
| stretch 200 µs on every ACK | frames all ACK; read-back correct; slave counted the stretches |
| stretch 1000 µs | read-back correct |
| SCL held low, never released | byte command returned (bounded wait); bus usable again after release |

## Hardware driver

The hardware engine keeps SCL stretching enabled in its control register, so
stretching is handled by the on-chip I2C peripheral rather than by the firmware
wait loop.  The hardware path was **not captured** on this bench (only the
software driver was exercised); the register setting is read from the firmware.

## v3 / v4

The per-bit wait lives in the shared bit-bang layer, which is the software
driver used by **both** boards.  Clock stretching therefore applies to v3
software I2C as well; the older "v3 does not support clock stretching" claim is
wrong.  v3 is read from the source (no v3 hardware here) and is **not verified
on this bench**.

| | v3 | v4 |
|---|---|---|
| software driver stretch wait | yes (from source) | yes (bench-verified) |
| ~25 ms bounded timeout | yes (from source) | yes (bench-observed bounded) |
| hardware driver stretch handling | — (no hardware driver in a default build) | yes (from source) |