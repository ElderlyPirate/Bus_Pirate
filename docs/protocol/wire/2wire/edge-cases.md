# 2-Wire — edge cases and failure modes

Part of the [2-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  Each row is a concrete trigger, symptom,
behaviour and recovery.

## Pull-up and output type

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Open drain with no pull-up, or no voltage on VPU | reads return arbitrary values (`0x00`, `0xFF`, or noise); an ATR returns meaningless bytes; nothing is flagged | the mode's start/stop do not report a short or a missing pull-up (unlike I2C); add one 2 kΩ–10 kΩ pull-up on each of SDA and SCL and feed VPU, or use normal output if the target tolerates a driven line.  Check the data pin with `.` and the pin table with `v` before trusting a read |
| Open drain, no pull-up, `[` pressed | the START line prints, but **no start condition is generated** | the firmware skips the start when either line reads low; fit the pull-up, then re-issue `[` |
| `P` after entering with output type `2` (normal) | `WARNING: pins not open drain (HiZ)` before `Pull-up resistors ON` | the pull-ups switch on but a driven-high pin ignores them; if a pull-up is needed, re-enter with output type `1` (open drain) |
| `P` with nothing on the Vpullup pin | `Pull-up resistors ON` then `Warning: no voltage on Vpullup pin`; the open-drain bus has no idle-high | on v4 select `e2` (3.3 V) or `e3` (5 V) and re-run `P`; on v3 feed Vpullup externally |
| `e` in normal output | `WARNING: pins not open drain (HiZ)` + syntax error | re-enter with open drain, then select the source |
| `e` while VPU already has a voltage | `Warning: already a voltage on Vpullup pin` + syntax error | an external source is already feeding VPU; drop the `e` attempt |
| `e` on v3 | `Syntax error at char 1` | v3 has no `e` command; feed Vpullup externally |

## Reads and bit operations

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `-` then `.` expected to report 1 in normal output | `.` reports `0` (or a floating level) | `.` makes the data pin an input, releasing the driven level; in normal output with no pull-up the line can fall.  Use an external pull-up or read with the target driving the line |
| `r` with nothing driving SDA | `READ: 0x00` or `0xFF`/noise, no error | the line is floating; confirm the target is powered and selected, and check the idle level with `.` |
| `;N` above 16 | syntax error; the command does not run | widths are 1–16; split the transfer |
| `^` output expected as `1` | `CLOCK TICKS: 0x01` | the count uses the current display format (HEX by default); use `o2` for decimal |

## ATR and smart card

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `(1)` expected to return usable card data | four values print and decode, but they are sampled with reset asserted | a card cannot answer while reset is asserted, so the four values are **not a valid ATR**; this is a known firmware limitation.  Do not use the decoded fields as card data.  `(2)` parses only; it does not fix the reset order |
| `(1)` or `(2)` with no card, no power, or the reset line unwired | four values print (often `0x00`/`0xFF` or noise), then the decode lines — **no error** | the mode does not sense a card and the parse reads exactly four values; confirm VCC (`W`), the pull-up (`P`) and the reset wiring (CS → card RST); use `.` to check the idle-high level |
| `(1)` finished, then the card expected to run | CS is left low, so the card's reset is still asserted | select CS (`C`) and drive it high (`A`); leaving the mode makes CS an input, which floats unless a pull-up is fitted |
| The card's ATR is longer or shorter than four values | the decode looks wrong and the bus is left mid-stream, so a following command starts from the wrong point | the parse always reads exactly four values and never drains a longer reply; re-enter the mode (or reset) to realign the bus |
| LSB-first order (`L`) was selected before the ATR macro | the reply header notes `uses current LSB setting`, and each value is bit-reversed | the parse reverses each read value under LSB-first order; select MSB-first (`l`) before running the macro |
| `;N` set a width other than 8 before `(1)`/`(2)` | each "value" the parse reads is N bits, so the decode is wrong | the parse assumes the default 8-bit width; re-enter the mode (leaving resets the width to 8) or set the width back |
| A standard ATR is parsed | `Protocol: unknown` | the protocol field reads the first value's high nibble; a standard `TS` byte (`0x3B`/`0x3F`) has nibble `3`, which is not one of the recognised codes |

## Speed and bus

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| The `Maximum speed` profile (`4`) against a slow device | the target does not respond or returns wrong data | the top profile applies no deliberate delay, so it clocks as fast as the firmware can; drop to a lower speed profile and re-enter the mode |
| Expecting a 2-Wire `BUS RESET` macro | `(0)` lists only the ATR macros | 2-Wire has no bus-reset macro; `BUS RESET` is a 1-Wire string.  Use the base layer's `#` reset, or re-enter the mode |

## Binary path

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| The per-byte `0x01` from `0x1x` read as a slave ACK | `0x01` after every byte | it is the Bus Pirate's own result byte; 2-Wire has no slave ACK |
| `0x8x` or `0x6x` sent while CS should stay low | CS jumps high | both commands force CS high; re-assert with `0x04` |
| `0x02` start with a line low | reply `0x01` but no start condition | the start is skipped, as in the terminal; fix the pull-up |
| `0x5x` on v3 | `0x00` | the pull-up feed is v4 only |
| A width other than 8 expected | there is no width command | the binary path is fixed at 8 bits |

## Unverified paths

- The binary path is **unverified**.
