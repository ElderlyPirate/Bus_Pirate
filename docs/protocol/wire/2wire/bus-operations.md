# 2-Wire — bus operations

Part of the [2-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  This page owns the on-bus commands:
start/stop, byte read/write, the clock edges, and the bitwise controls.
Shared prompt grammar — value entry, repetition `:N`, width `;N`, the
wrappers, `(n)` macros — is owned by
[docs/generic/](../../../generic/README.md); only the 2-Wire effect is
described here.

## Start and stop

| Input | Action | Firmware output |
|---|---|---|
| `[` | start (I2C-style) | `(\-/_\-)I2C START BIT` |
| `{` | start-with-read form; same 2-Wire start | `(\-/_\-)I2C START BIT` |
| `]` | stop (I2C-style) | `(\_/-)I2C STOP BIT` |
| `}` | stop-from-read form; same 2-Wire stop | `(\_/-)I2C STOP BIT` |

The parenthesised text is the SDA/SCL waveform the condition draws.  The start
and stop are I2C-style conditions, but 2-Wire does **not** carry any
acknowledge logic on top of them: `{` and `}` are the same calls as `[` and
`]`, and no read-back flag is set.  This is the main way 2-Wire differs from
I2C — I2C adds an ACK/NACK model and a short/pull-up warning; 2-Wire adds
neither.

The start check is silent: in open drain, if SDA or SCL already reads low, the
firmware **skips** the start sequence but still prints the START line.  The
stop sequence is always generated.

## Clock edges

At `W` the firmware drives SCL low and makes SDA an input (released), so with
a pull-up SDA idles high and SCL low.  A start then raises both lines before
pulling SDA down.

| Operation | Edge order |
|---|---|
| write one bit | set SDA (settle) → SCL high (delay) → SCL low (delay) |
| read one bit | release SDA → SCL high (delay) → sample SDA → SCL low (delay) |
| `^` tick | SCL high (delay) → SCL low (delay) |
| `/` / `\` | set SCL high / low and hold (settle delay) |
| start | both high → SDA low → SCL low → SDA high |
| stop | both low → SCL high → SDA high |

A target should present its data while SCL is high; the read samples during
that high phase.  `-` and `_` only set the SDA level; they do not clock.

## Writing a value

Any number written in the mode is clocked out on SDA (MOSI) as the current
width in bits (8 by default), MSB-first by default, and **no acknowledge bit is
read**:

| Input | Meaning |
|---|---|
| `0xNN` | hex value (`0x01`, `0xA` — partial widths allowed) |
| `0b…` | binary value (partial widths allowed) |
| `NNN` | decimal value (no prefix) |
| `"text"` | the literal bytes of the string |

The reply echoes the value:

```
2WIRE>0x01
WRITE: 0x01
2WIRE>0xA5
WRITE: 0xA5
```

Each bit is driven, then clocked (data setup, clock high, clock low), for the
full width.  There is no ninth ACK bit and no slave response to read.

## Reading a value

`r` reads one value by clocking the current width in from SDA (MOSI),
MSB-first by default; `r:N` reads N values.

| Input | Meaning |
|---|---|
| `r` | read one value (`READ: <value>`) |
| `r:N` | read N values on one header line |

```
2WIRE>r
READ: 0x00
2WIRE>r:2
READ: 0x00 0x00
```

With nothing driving SDA the value is a floating level, not a device value.
The example values are illustrative; a floating bus can read either extreme.

## Width — `;N`

`;N` sets the number of bits for the value, from **1 to 16**; a width above 16
is a syntax error.  The width is remembered until changed or until the mode is
left.

```
2WIRE>r;16
READ: 0xFFFF
2WIRE>1;16
WRITE: 0x0001
```

Reads and writes both use the width: `r;16` clocks 16 bits in; `1;16` clocks 16
bits out.  The example values assume a floating bus and a 16-bit MSB-first
transfer.

## Bitwise controls

2-Wire's distinguishing feature over I2C is that the two lines can be driven
and clocked a bit at a time.  The grammar and firmware wording are shared with
the base layer (see [commands.md](../../../generic/commands.md) §Transaction
wrappers and bit ops); the 2-Wire effect is:

| Input | Action | Firmware output |
|---|---|---|
| `/` | drive SCL (CLK) high | `CLOCK, 1` |
| `\` | drive SCL (CLK) low | `CLOCK, 0` |
| `^` / `^:N` | one clock tick (N ticks) | `CLOCK TICKS: <n>` |
| `-` | drive SDA (MOSI) high | `DATA OUTPUT, 1` |
| `_` | drive SDA (MOSI) low | `DATA OUTPUT, 0` |
| `.` | read the SDA pin, no clock | `DATA STATE: 0` or `1` |
| `!` / `!:N` | read N bits, each with a clock | `READ BIT: <bits>  *pin is now HiZ` |

- `/` and `\` set the clock **level** (they do not pulse); `^` pulses the clock
  one full cycle per tick.  Each carries the selected profile's settle delay.
- `-` and `_` set the data **level** with the profile's settle delay.
- `.` samples SDA (MOSI) without clocking — a static level read.  It first
  makes the pin an input, so it **releases** a level that `-`/`_` just drove;
  in normal output with no pull-up the read can fall to 0.
- `!` clocks SDA while sampling: one read per clock tick.  The
  ` *pin is now HiZ` suffix is the base-layer string; `!` leaves the data pin
  as an input.
- `.` prints no trailing newline; the next prompt continues on the same line.
  The `^` count uses the current display format (HEX by default), so a single
  tick prints `CLOCK TICKS: 0x01`.

These primitives are what you use to hand-build a custom two-wire frame — bit
order, idle levels, setup/hold — when a protocol is not one the mode already
knows.

## Worked session

A byte write/read in the same style, bus engaged, nothing on the bus.  The
values are examples; with no pull-up the floating line can read either way.

```
2WIRE>[
(\-/_\-)I2C START BIT
2WIRE>0x01
WRITE: 0x01
2WIRE>r
READ: 0x00
2WIRE>]
(\_/-)I2C STOP BIT
2WIRE>
```

The write clocks `0x01` out on SDA; the read clocks a byte back in.  Nothing
drives the bus here, so the read is a floating `0x00`.

A shorter wrapper/bit-op session is in
[usage.md](../../../generic/usage.md#wrappers-and-bit-ops-2wire).
