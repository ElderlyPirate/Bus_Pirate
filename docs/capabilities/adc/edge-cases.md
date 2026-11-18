# ADC probe — edge cases and failure modes

Part of the ADC capability set (`docs/capabilities/adc/`).  Each entry gives a
trigger, the observable symptom, the current behaviour and the recovery.  The
scale is in [probe.md](probe.md); the front ends are in
[terminal.md](terminal.md) and [binary.md](binary.md).

## Terminal

### No source connected to the probe pin

- **Trigger:** take a reading with nothing wired to `ADC`.
- **Symptom:** some reading, usually near `0.00V`.
- **Behaviour:** the divider's lower leg pulls toward ground, but leakage or a
  nearby signal can move the pin.  There is no defined source.
- **Recovery:** connect the source and its ground before interpreting a
  reading.

### Input above 6.6 V

- **Trigger:** apply more than the nominal 6.6 V full-scale input.
- **Symptom:** the reading stops at `6.59V` (raw 1023); the pin may be
  damaged.
- **Behaviour:** the converter is at full scale and there is no over-range
  flag, so the reading cannot show how far over the input is.  Above 6.6 V is
  not a harmless clip: the pin's absolute maximum is undocumented and the
  input may be overstressed.
- **Recovery:** disconnect the source, divide it down, and measure again.

### Input below ground

- **Trigger:** apply a negative voltage.
- **Symptom:** no usable reading; the pin may be damaged.
- **Behaviour:** the converter cannot measure below 0 V.
- **Recovery:** do not apply a negative voltage.

### High-impedance source

- **Trigger:** measure a source whose impedance is high compared with the
  sample path.
- **Symptom:** the reading is low or unstable.
- **Behaviour:** the short sample window does not fully charge the input.
- **Recovery:** buffer the source or lower its impedance.

### An invalid `d:N` count

- **Trigger:** `d:0`, a count above 32767, or a non-numeric count.
- **Symptom:** `VOLTAGE PROBE: ` followed by `Syntax error at char N` on the
  same line; no reading.
- **Behaviour:** the header is printed before the count is parsed, then the
  count is rejected.  A zero, an out-of-range and a malformed count take the
  same path.
- **Recovery:** use a count of 1..32767.

### A very large `d:N`

- **Trigger:** run a repeat with a large valid count.
- **Symptom:** the command runs for a long time.
- **Behaviour:** the repeat loop does not watch the host for a stop.
- **Recovery:** press the reset button; a typed `#` is not read until the loop
  ends (its keys then run as commands).

### Extra keys typed during `D`

- **Trigger:** type more than one key while the voltmeter runs.
- **Symptom:** after the stop, the extra characters run as commands.
- **Behaviour:** only one byte is consumed to stop the loop.
- **Recovery:** press a single key; clear or account for any further keys.

### `D` never returns on its own

- **Trigger:** leave `D` running.
- **Symptom:** the value updates forever.
- **Behaviour:** there is no timeout and no sample limit.
- **Recovery:** send one byte; it stops the loop and is consumed.

### `D` returns at once with `0.00V`

- **Trigger:** a byte is already waiting when `D` starts (type-ahead, or the
  line-feed half of a CR+LF terminator left by the terminal).
- **Symptom:** the display shows `VOLTAGE PROBE: 0.00V` and returns to the
  prompt.
- **Behaviour:** the loop checks for input before the first sample, so the
  `0.00V` placeholder is printed and then the loop exits.  A terminal that
  always sends CR+LF leaves the LF waiting every time.
- **Recovery:** set the terminal to send CR only (not CR+LF), then retry; or
  use a host program that sends one byte.  Pressing more keys adds bytes; it
  does not drain a terminal that always sends both.

### `dx` typed

- **Trigger:** add a letter after `d`.
- **Symptom:** one reading, then `Syntax error at char 2`.
- **Behaviour:** `d` accepts only the optional `:N`; the letter is parsed as
  the next command and is not a display selector.
- **Recovery:** remove the letter.  (Only `r` takes a display letter.)

### `o1` (HEX) set, then `d`

- **Trigger:** change the display format, then read the probe.
- **Symptom:** the probe still prints decimal volts and a decimal raw count.
- **Behaviour:** the ADC path ignores the display format.
- **Recovery:** none needed.

### The self-test `ADC` jumper is still fitted

- **Trigger:** measure a source after a full self-test, with the
  `ADC` → `+3.3V` jumper left in place.
- **Symptom:** the reading is not the source.  With a high-impedance source it
  sits near the 3.3 V rail (about raw 512); with a low-impedance source it
  follows the source while the source back-drives the rail.
- **Behaviour:** the jumper is a plain wire from the probe pin to the `+3.3V`
  rail, not a resistor.  It hard-ties the two, so a connected source drives
  the rail and can damage the board or the source.
- **Recovery:** remove the jumper, then measure again.  Remove it after every
  self-test, before connecting anything to `ADC`
  ([selftest terminal](../selftest/terminal.md)).

### SMPS owns the ADC — `d` and `D`

- **Trigger:** run `d`, `d:N` or `D` on an SMPS-enabled v4 build while the
  SMPS feedback loop is running.
- **Symptom:** `ADC busy: SMPS feedback active; command rejected.` then
  `Syntax error at char 1`; no header and no reading.
- **Behaviour:** the loop uses the probe channel as its output-voltage
  feedback, so the foreground probe commands refuse rather than take the
  converter.  This is a defensive check: a normal return to `HiZ>` resets the
  board state and stops the loop, so this message should not normally appear.
- **Recovery:** at `HiZ>`, send `#` to reset the board state (this stops the
  SMPS), or press the reset button, then repeat the command.  In the binary
  layer use the sequence in
  [binary.md §SMPS owns the ADC](binary.md#smps-owns-the-adc).  This cannot
  happen on v3 or on a build without SMPS support.

### A source connected while the SMPS loop runs

- **Trigger:** wire an external source to `ADC` with the SMPS feedback loop
  running.
- **Symptom:** the probe commands are refused, and the source is connected to
  the loop's feedback node.
- **Behaviour:** the pin is the SMPS feedback input, not a free measurement
  input.
- **Recovery:** stop the SMPS, wire the source, then measure.

## Binary

### Host waits for an acknowledgement after `0x14`

- **Trigger:** read a result byte before the sample, expecting `0x01`.
- **Symptom:** the first sample byte is consumed as the expected ack, or the
  host keeps waiting for a `0x01` that is not sent; later bytes shift.
- **Behaviour:** there is no acknowledgement and no error byte on `0x14`.
- **Recovery:** read exactly two bytes per request, high byte first.

### A sample high byte is `0x00`

- **Trigger:** read a sample below 256 counts (about 1.65 V) from `0x15` or
  `0x14`.
- **Symptom:** the two bytes start with `0x00`.
- **Behaviour:** `0x00` is a valid high byte; there is no result byte in this
  reply to confuse it with.
- **Recovery:** none; parse fixed two-byte samples.

### A byte is already queued when `0x15` starts

- **Trigger:** start the stream with a byte already waiting in the
  host-to-board direction.
- **Symptom:** the stream sends one sample and stops.
- **Behaviour:** the loop checks for input after the first sample.
- **Recovery:** drain the host-to-board direction first.

### `0x00` sent to stop `0x15`

- **Trigger:** use `0x00` as the stop byte.
- **Symptom:** `BBIO1` is not re-printed.
- **Behaviour:** the stop byte is consumed, not executed as a command.
- **Recovery:** send another `0x00` if you want the identifier.

### A real command sent instead of a stop byte

- **Trigger:** send `0x14`, `0x0F` or another command while the stream runs.
- **Symptom:** the command disappears.
- **Behaviour:** any byte stops the stream and is consumed.
- **Recovery:** send the command again after the drain.

### `0x14` sent inside a sub-protocol

- **Trigger:** send `0x14` while in SPI, I2C, UART, 1-Wire or `RAW1`.
- **Symptom:** some bus action, not an ADC reading; in those modes `0x14` is
  the five-byte bulk-transfer command.
- **Behaviour:** `0x1x` is the bulk group outside `BBIO1`.
- **Recovery:** return to `BBIO1` first
  ([sub-protocols](../../protocol/binary/sub-protocols.md)).

### `0x14` sent from the terminal

- **Trigger:** send the control byte `0x14` at the interactive prompt.
- **Symptom:** no reading appears; the terminal rings the bell.
- **Behaviour:** the control byte is rejected before it enters the command
  line.
- **Recovery:** enter the bitbang layer first and send `0x14` there.

### Stop sent, then the host stops reading

- **Trigger:** send the stop byte and stop reading while the transmit path is
  full.
- **Symptom:** the board can stay blocked in the transmit path.
- **Behaviour:** the stop is noticed only at the loop boundary, after a sample
  has been handed to the buffered output.
- **Recovery:** keep reading until the samples stop.  Reset only if the host
  abandoned the read altogether.

### A whole `0x15` stream left unread

- **Trigger:** start `0x15` and never read the output.
- **Symptom:** the output buffer fills and the board stalls.
- **Behaviour:** buffered output is delivered, but there is no sample history
  and no re-send.
- **Recovery:** send a stop, keep reading until no sample arrives within the
  timeout, then continue.  See
  [binary.md §Stopping the stream](binary.md#stopping-the-stream).

### `0x14` issued before the `0x15` drain completes

- **Trigger:** stop the stream, then immediately send `0x14`.
- **Symptom:** the next bytes are buffered stream samples, so a host that
  reads two bytes as the `0x14` reply gets stale data.
- **Behaviour:** already-generated samples are delivered before the new
  command's reply.
- **Recovery:** drain until the timeout before sending any command.

### SMPS owns the ADC — `0x14` and `0x15`

- **Trigger:** send `0x14` or `0x15` on an SMPS-enabled v4 build while the
  SMPS feedback loop is running.
- **Symptom:** `0xFF 0xFF` once; `0x15` does not stream.
- **Behaviour:** the pair is an impossible sample (a valid high byte is
  `0x00`..`0x03`), sent instead of ASCII or a one-byte status.  The loop keeps
  ownership until it is stopped.
- **Recovery:** stop the SMPS with `0xF1` in raw wire.  From `BBIO1`, send
  `0x05` first to enter `RAW1`, then `0xF1`, then `0x00` back to `BBIO1`.
  Do not send `0xF1` in `BBIO1`: there it is a pin-state byte that switches
  the target supply and pull-ups on.  Alternatively send `0x0F` from `BBIO1`
  to return to the terminal and reset the board state.  `0x00` back to
  `BBIO1` does not stop it
  ([binary.md §SMPS owns the ADC](binary.md#smps-owns-the-adc)).

### A stop byte sent after a refused `0x15`

- **Trigger:** send `0x15`, receive `0xFF 0xFF`, then send a stop byte as if a
  stream had started.
- **Symptom:** the stop byte is not consumed by a stream; it is read as the
  next command (for example `0x00` reprints `BBIO1`).
- **Behaviour:** the SMPS-owned branch sends the pair and returns before the
  stream loop starts, so there is no stop to consume.
- **Recovery:** after a refusal, send the next command directly; do not send a
  stop byte.

## Related

- [probe.md](probe.md) — the conversion, channels and ownership.
- [terminal.md](terminal.md) — the terminal command detail.
- [binary.md](binary.md) — the byte-level detail and the stop contract.
- [docs/protocol/binary/edge-cases.md](../../protocol/binary/edge-cases.md) —
  failures shared by the whole binary layer.
