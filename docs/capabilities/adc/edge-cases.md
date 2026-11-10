# ADC probe — edge cases and failure modes

Part of the ADC capability set (`docs/capabilities/adc/`).  Each entry gives a
trigger, the observable symptom, the current behaviour and the recovery.
The scale is in [probe.md](probe.md); the front ends are in
[terminal.md](terminal.md) and [binary.md](binary.md).

## Terminal

| Trigger | Symptom | Behaviour | Recovery |
|---|---|---|---|
| no source connected to the probe pin | some reading, usually near `0.00V` | the divider's lower leg pulls toward ground, but leakage or a nearby signal can move it; there is no defined source | never interpret a reading without a connected source; connect the source or ground the pin |
| input above 6.6 V | reading stops at `6.59V` (raw 1023) | the converter is at full scale; there is no over-range flag | divide the source down; never rely on the reading above 6.6 V |
| input below ground | no usable reading; the pin may be damaged | the converter cannot measure below 0 V | do not apply a negative voltage |
| high-impedance source | reading low or unstable | the short sample window does not fully charge the input | buffer the source or lower its impedance |
| an invalid `d:N` count (0 or above 32767) | `VOLTAGE PROBE: ` followed by `Syntax error at char N` on the same line, no reading | the count is rejected before any sample; a non-numeric or out-of-range count takes the same path | use a count of 1..32767 |
| a very large `d:N` | the command runs for a long time | the repeat loop does not watch the host for a stop | press the reset button; a typed `#` is not read until the loop ends (its keys then run as commands) |
| extra keys typed during `D` | after the stop, the extra characters run as commands | only one byte is consumed to stop the loop | press a single key; any further keys become commands |
| `D` left running | the value updates forever | there is no timeout and no sample limit | send one byte |
| `D` returns at once with `0.00V` | the display shows the placeholder and returns to the prompt | a byte was already waiting (type-ahead, or a CR+LF terminator left by the terminal); the loop checks input before the first sample | drain the input and retry; the `0.00V` is the placeholder, not a reading |
| `o1` (HEX) set, then `d` | the probe still prints decimal volts and a decimal raw count | the ADC path ignores the display format | none needed |
| `dx` typed | one reading, then `x` is handled as the next command | `d` accepts only the optional `:N` | do not add a format letter |

## Binary

| Trigger | Symptom | Behaviour | Recovery |
|---|---|---|---|
| host waits for an ack after `0x14` | the first sample byte is taken as the ack; later bytes shift by one | there is no acknowledgement and no error byte | read exactly two bytes per request |
| a sample below 1.65 V (`0x15`) | a sample starts with `0x00` | that is a valid high byte, not the layer's failure byte | parse fixed two-byte samples |
| a byte is already queued when `0x15` starts | the stream sends one sample and stops | the loop checks for input after the first sample | drain the host-to-board direction first |
| `0x00` sent to stop `0x15` | `BBIO1` is not re-printed | the stop byte is consumed, not executed | send another `0x00` if you want the identifier |
| a real command sent instead of a stop byte | the command disappears | any byte stops the stream and is consumed | send the command again after the drain |
| `0x14` sent inside a sub-protocol | some other action, not an ADC reading | `0x1x` is the bulk transfer group in SPI, I2C, UART, 1-Wire and `RAW1` | return to `BBIO1` first |
| `0x14` sent from the terminal | no reading appears; the terminal rings the bell | `0x14` is a control byte, not a terminal command; it is rejected before it enters the line | enter the bitbang layer first |
| `0x15` stop sent, then the host stops reading | the board can stay blocked in the transmit path | the stop is only noticed at the loop boundary, after a sample has been handed to the buffered output | keep reading until the samples stop; reset only if the host stopped reading altogether |
| a whole `0x15` stream left unread | the output buffer fills and the board stalls | buffered output is delivered, but there is no sample history and no re-send | send a stop, keep reading until no sample arrives within the timeout, then continue — see [binary.md §Stopping the stream](binary.md#stopping-the-stream) |

## Why 3.3 V can print 3.29 V

The firmware's fixed-point scale is a shade below the exact 6.6 / 1024 (under
1 mV over the full scale) and the display truncates, so an exact step can
print one step low: 1.650 V prints `1.64V`, 3.300 V prints `3.29V`.  The full
explanation and the raw/ideal/printed table are in
[probe.md §Why 3.3 V can print 3.29 V](probe.md#why-33-v-can-print-329-v).

## Related

- [probe.md](probe.md) — the conversion and its limits.
- [terminal.md](terminal.md) — the terminal command detail.
- [binary.md](binary.md) — the byte-level detail and the stop contract.
- [docs/protocol/binary/edge-cases.md](../../protocol/binary/edge-cases.md) —
  failures shared by the whole binary layer.
