# I2C — sniffer

Part of the I2C documentation set (`docs/i2c/`).  The I2C sniffer is macro
`(2)`: it listens to a live bus and prints the traffic.

## Usage

**Example output:**

```
I2C>(2)
Sniffer
Any key to exit
[0x40-][0x40-0x40-0x30-0x56-0x77-]
I2C>
```

- The sniffer runs until a key is pressed.  That **stopping key is consumed**;
  characters typed after it are processed as normal commands.
- SDA and SCL are both switched to inputs while sniffing, so the Bus Pirate
  does not drive the bus.

## Output format

| Element | Shown as |
|---|---|
| start condition | `[` |
| stop condition | `]` |
| each data byte | `0xNN` in hex |
| bit after each byte | `+` = ACK (SDA low), `-` = NACK (SDA high) |

So `[0x40-][0x40-0x40-0x30-0x56-0x77-]` reads as: start, byte `0x40`, NACK,
stop; start, `0x40`, NACK, `0x40`, NACK, `0x30`, NACK, `0x56`, NACK, `0x77`,
NACK, stop.  (This is the format the firmware produces; the example line is a
format illustration.)

Bytes are sampled at the rising edge of SCL, eight bits per byte, and the
ninth bit is reported as ACK or NACK.

## Output buffering and overflow

Sniffer output is queued in the terminal ring buffer and pushed to the host as
the UART frees up, so a slow display does not by itself drop bytes:

- v4 buffer: **4096 bytes**.
- v3 buffer: **2048 bytes**.

A long enough burst still overtakes the buffer; once it is full, further bytes
are **dropped silently** (no error is printed).  For long captures, stop the
sniffer and let it drain, or use a logic analyzer.

## Limits

- The sniffer is a **software** capture on a change-notification interrupt; its
  usable bus rate depends on the interrupt and serial load, not on a fixed
  number.  The older published claim of "up to 100 kHz" is **unverified** —
  treat the maximum rate as unknown.  It is not a substitute for a logic
  analyzer.
- The sniffer follows the bus; it does not decode I2C transactions beyond the
  byte/ACK framing above (no register names, no device model).

## Binary mode

The same sniffer is reachable from the binary `I2C1` sub-protocol (command
`0x0F`).  In that path each byte is framed with a `\` escape byte rather than
the `0xNN` text form — see [binary.md](binary.md).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| macro `(2)` | present | present |
| ring buffer | 2048 bytes | 4096 bytes |
| text output format | same | same |

v3 is **not verified**.