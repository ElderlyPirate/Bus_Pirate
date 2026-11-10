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
slow parts such as the SHT3x family).  After the timeout the bit is abandoned
and the bus moves on.  If the stuck bit was an acknowledge slot, the read is
reported as **NACK** (not ACK), so a timed-out acknowledge is never mistaken
for a valid one.

## Hardware driver

The hardware engine keeps SCL stretching enabled in its control register, so
stretching is handled by the on-chip I2C peripheral rather than by the firmware
wait loop.  The hardware path is **not verified**.

## v3 / v4

The per-bit wait lives in the shared bit-bang layer, which is the software
driver used by **both** boards.  Clock stretching therefore applies to v3
software I2C as well; the older "v3 does not support clock stretching" claim is
wrong.  v3 is **not verified**.

| | v3 | v4 |
|---|---|---|
| software driver stretch wait | yes | yes |
| ~25 ms bounded timeout | yes | yes |
| hardware driver stretch handling | — (no hardware driver in a default build) | yes |