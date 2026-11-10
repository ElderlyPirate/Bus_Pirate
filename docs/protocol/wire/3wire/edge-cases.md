# 3WIRE — edge cases and failure modes

Part of the [3-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  Each row is a concrete trigger, symptom,
behaviour and recovery.

## Bus and CS

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Wrong CS polarity chosen | the target never selects; reads return idle/floating values and it ignores every write | with the default `/CS`, `[`/`{` assert CS **low**; the `CS` option asserts it **high** (see [bus-operations.md](bus-operations.md) §Which level is driven).  Choose `CS` only if the target needs active-high; the binary `RAW1` `0x04`/`0x05` drives CS by hand |
| A real SPI target is wired up and 3WIRE is used | no clock-polarity/phase control, no byte command set, no sniffer; reads do not line up with the SPI framing | 3WIRE is a bare bit bus with no SPI engine and no framing.  Use the **SPI** mode for SPI targets |
| `-` (data high) then `.` expected to report "1" | `.` reports `DATA STATE: 0` (or a floating level) even though MOSI was just driven high | MOSI is the data-out line and MISO is the data-in line; `.` samples MISO, never MOSI.  Wire MOSI to MISO for a loopback, or check the target's MISO output |

## Pull-up and power

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Open drain with no pull-up, or no voltage on VPU | reads return arbitrary values (`0x00`, `0xFF`, or noise) and nothing is flagged | the CS control does not check the released lines, so there is no short/pull-up warning (unlike I2C).  Add a 2 kΩ–10 kΩ pull-up and feed VPU (v4 `e2`/`e3`, v3 externally), or use normal output |
| `P` after entering with output type `2` (normal) | `WARNING: pins not open drain (HiZ)` before `Pull-up resistors ON` | the pull-ups switch on, but driven-high pins ignore them.  Re-enter with output type `1` (open drain) if a pull-up is needed; see [entry-and-setup.md](entry-and-setup.md) |
| `P` while nothing is on the Vpullup pin | `Pull-up resistors ON` then `Warning: no voltage on Vpullup pin`; no idle-high | the pull-ups reference a floating VPU pin.  On v4 select `e2` (3.3 V) or `e3` (5.0 V) and re-run `P`; on v3 feed Vpullup externally; see [wiring.md](wiring.md) |

## Commands and data

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `(0)` issued expecting a macro menu | `ERROR: command has no effect here` then `Syntax error at char <n>` | the mode has **no macros by design**; its macro handler is intentionally empty.  Use base-layer user macros `<n>`, a host script, or SPI for SPI targets; see [bus-operations.md](bus-operations.md) §No macros |
| A `BUS RESET` command expected (as in 1-Wire) | no such command; there is no reset output | `BUS RESET` is a 1-Wire string; 3WIRE has no bus-reset or device-search command.  Use `#` or re-enter the mode; drive CS or a data line by hand for a target reset |
| `r` with nothing driving MISO | `READ: 0x00` (or `0xFF`, or noise) with no error | the read clocks the bus and captures whatever MISO presents; the line is not validated.  Confirm the target is powered (`W`), selected (CS) and driving MISO; check the idle level with `.` and `v` |

## Limits

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `;N` with N greater than 16 (`0x55;17`) | `Syntax error at char <n>`; the command does not run | the value width is limited to 16 bits; a larger width is rejected (the shift would silently zero every bit).  Use 1–16 or split the transfer |
| The `Maximum speed` profile (`4`) against a slow device | the target does not respond or returns wrong data | the top profile applies no deliberate delay, so `/`, `\`, `-` and `_` carry almost no settle time.  Drop to a lower speed profile and re-enter the mode |
