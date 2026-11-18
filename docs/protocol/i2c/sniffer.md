# I2C — sniffer

Part of the I2C documentation set (`docs/protocol/i2c/`). The I2C sniffer is
macro `(2)`: it listens to a live bus and prints the traffic. The macro list is
in [macros.md](macros.md); the binary framing of the same sniffer is in
[binary.md](binary.md#sniffer-in-binary-mode-0x0f).

## Usage

```
I2C>(2)
Sniffer
Any key to exit
[0xA1+0xAA+0xBB-]
I2C>
```

- The sniffer runs until a key is pressed. That **stopping key is consumed**;
  characters typed after it are processed as normal commands.
- SDA and SCL are both switched to inputs while sniffing, so the Bus Pirate does
  not drive the bus.
- If a read ACK is pending when `(2)` is issued, the macro sends NACK and a stop
  first.
- In the Hardware driver the macro re-runs the hardware setup on exit, so bus
  operations work again without a mode change.
- The sniffer follows the bus, not the mode's on-board EEPROM redirect: after
  `(3)` it still samples the external SDA/SCL pins (see [macros.md](macros.md)).

## Output format

| Element | Shown as |
|---|---|
| start or repeated start condition | `[` |
| stop condition | `]` |
| each data byte | `0xNN`, uppercase hex |
| the ninth bit after each byte | `+` = ACK (SDA low), `-` = NACK (SDA high) |

The example `[0xA1+0xAA+0xBB-]` reads as: start, byte `0xA1` acknowledged,
byte `0xAA` acknowledged, byte `0xBB` not acknowledged, stop.

Bytes are sampled at the rising edge of SCL, eight bits per byte, and the ninth
bit is reported as ACK or NACK. There is no separator between one byte's marker
and the next byte, so a capture is one continuous token.

The text form always prints hex; it does **not** follow the `o` display format.
The scanner and the `READ:` lines do follow `o`.

## Output buffering and overflow

Sniffer output goes through the terminal output path. **The terminal output
buffer is 4096 bytes on both builds**, and it is not the command line buffer
(128 bytes on v3, 256 on v4). On v3 the sniffer appends to that ring; on v4 the
same calls write straight to the USB CDC queue, so the ring is not on the
sniffer path.

| Build | Path | When it fills |
|---|---|---|
| v3 | the shared 4096-byte output ring | further bytes are dropped silently and the MODE LED turns off |
| v4 | the USB CDC queue (the ring is not used) | the writer waits, the sniffer stalls, and bus edges are missed |

Neither path prints a message when output cannot keep up. For a long or fast
capture, stop the sniffer and let the output drain, or use a logic analyzer.

## Limits

- The sniffer is a **software** capture driven by a change-notification
  interrupt. It services one change at a time, so edges that arrive while the
  loop is busy are missed.
- The usable bus rate therefore depends on the build, the host link and the
  traffic pattern; **no maximum rate is specified** and exact rates are
  **unverified**. Do not treat an overflow-free capture as proof that nothing
  was lost at an arbitrary rate.
- It does not decode I2C transactions beyond the byte/ACK framing shown above:
  no register names, no device model, no direction labels.
- It cannot capture the on-board EEPROM's internal bus; that bus is not on the
  external pins.
- It starts and stops only at SCL-high SDA transitions. A bus that is already
  mid-transaction when the sniffer starts has no start marker until the next
  start or repeated start.

## Binary mode

The same sniffer is reachable from the binary `I2C1` sub-protocol (`0x0F`). The
framing changes for a host parser: `[`/`]` markers, `\` + one raw byte per data
byte, then `+`/`-`; the `0x01` reply is sent only after the exit byte. The exact
grammar is in [binary.md](binary.md#sniffer-in-binary-mode-0x0f).

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| macro `(2)` | yes | yes |
| text output format | same | same |
| terminal output buffer | 4096 bytes | 4096 bytes |
| sniffer queue | 4096-byte ring | USB CDC queue |
| overflow symptom | silent drop; MODE LED off | stall; missed edges |

The v3 column is **not verified**.

## Related docs

- [macros.md](macros.md) — the `(0)` list and the other macros.
- [binary.md](binary.md#sniffer-in-binary-mode-0x0f) — the binary framing.
- [wiring.md](wiring.md) — what the two sniffed lines need.
- [edge-cases.md](edge-cases.md) — overflow and stop-key failures.
