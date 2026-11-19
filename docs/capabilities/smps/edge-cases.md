# SMPS - edge cases and failure modes

Part of the SMPS set (`docs/capabilities/smps/`).  Each entry is a concrete
trigger -> symptom -> behaviour -> recovery.  The commands are in
[binary.md](binary.md); the stop paths are in [exit.md](exit.md).

## The SMPS feature is not built in

- **Trigger:** send any `0xF0`-`0xFF` on the default v4 image or on any v3
  build.
- **Symptom:** every request answers `0x00`.  `0xF0` returns a **single**
  failure byte, not a two-byte sample.  A start (`0xF2`-`0xFF`) also answers
  `0x00` but **consumes** its operand, so the next request stays aligned:
  `0xF2 0x58` gives `0x00`, and the next byte is a new request.
  `0xF2 <operand>` replies `0x00`, consumes the operand, and **stays in
  `RAW1`**; this includes `0x00`: `0xF2 0x00` replies `0x00`, consumes the
  zero, and does not leave raw wire.
- **Behaviour:** the group is routed on every build; the handler refuses all
  requests when support is not compiled in.  The disabled path consumes a
  start's operand before replying, including an operand of `0x00`.
- **Recovery:** build a v4 image with SMPS enabled
  ([dev/building.md](../../dev/building.md)); a v3 build cannot enable it.
  In `RAW1`, send `0x01` to reprint `RAW1`; from `BBIO1`, send `0x05` first.

A host that expects two data bytes from `0xF0` on a disabled build will read
the next reply as the low byte.  On a refusal, read one byte.

## A start with no operand

- **Trigger:** send `0xF2`-`0xFF` and stop writing.
- **Symptom:** no reply; the layer appears blocked.
- **Behaviour:** every start reads one operand byte before it replies.
- **Recovery:** send the operand byte (any value), or reset the Bus Pirate.

## A requested value below `0x200`

- **Trigger:** on a build with SMPS support, try to express `0x000`-`0x1FF` by
  sending `0xF0` or `0xF1` plus an operand.
- **Symptom:** `0xF0` is a read, not a start: it returns its two data bytes,
  and the operand is then read as the next request.  `0xF1` is stop: it
  replies `0x01`, and the operand is then read as the next request.  For
  example, `0xF1 0x00` gives `0x01` and then leaves raw wire with `BBIO1`.
- **Behaviour:** the low nibble `0` and `1` are reserved for read and stop, so
  start values begin at `0xF2` and the range `0x000`-`0x1FF` is unreachable.
- **Recovery:** choose a reachable value (`0x200`-`0xFFF`).  After a stray
  operand, send `0x01` to reprint `RAW1`; if the operand was `0x00`, re-enter
  raw wire with `0x05`.

## Reading before the first start

- **Trigger:** `0xF0` on an enabled build before any start since power-up.
- **Symptom:** `0x00 0x00`.
- **Behaviour:** no sample has been stored.  `0xF0` reports the stored value
  and never starts a conversion, so this is not a measurement of 0 V.
- **Recovery:** start the loop first; a reading before that has no meaning.

## A reading after stop is stale

- **Trigger:** `0xF1` followed by `0xF0`.
- **Symptom:** the reading repeats the last sample from the run.
- **Behaviour:** stop turns the converter off and leaves the stored sample
  unchanged; `0xF0` never clears it.  Only a power cycle or a hardware reset
  reinitializes it.
- **Recovery:** treat a reading taken after stop as the last value from the
  previous run.  Start again to get new samples.

## No add-on, or no feedback connected

- **Trigger:** start with no add-on, or with the feedback wire open.
- **Symptom:** the start is accepted (`0x01`), but the sample is zero or an
  arbitrary floating level.
- **Behaviour:** the firmware has no presence test and no fault report.  It
  samples whatever is on `ADC`.  The duty follows the floating reading: it
  may sit at the computed duty or be cut to zero, and the output is
  uncontrolled.  A zero feedback can also mean a real 0 V output; the two
  cases look the same.
- **Recovery:** stop the loop and check the wiring.  Remove the load and do
  not touch the output until the feedback path is verified; a zero sample is
  not proof of 0 V output.  Treat the electrical result as `unverified`.

## A request above the firmware cutoff range

- **Trigger:** start at `0x527`-`0xFFF`.
- **Symptom:** the start is accepted and the PWM runs, but the loop never
  cuts the duty; the sample stays below the target.
- **Behaviour:** the comparison threshold is 1023 or higher, while a 10-bit
  sample cannot exceed 1023.  The comparison is strict, so the cutoff never
  fires and the PWM stays at the computed duty (open loop).
- **Recovery:** requests below `0x527` let the firmware cutoff act, but
  whether the add-on reaches the target is unknown.  Above that, treat the
  value as an open-loop duty setting.  There is no error reply.

## Leaving raw wire while the SMPS is running

- **Trigger:** send `0x00` without first sending `0xF1`.
- **Symptom:** `BBIO1` is printed, but the loop keeps running and the ADC
  refusals continue.
- **Behaviour:** the raw-wire exit path does not stop the loop, and the PWM
  and ADC are not cleaned up.  Only `0xF1` sent in `RAW1`, or a v4 terminal
  reset, stops them ([exit.md](exit.md)).
- **Recovery:** re-enter `RAW1` with `0x05` and send `0xF1`, or send `0x0F`
  from `BBIO1` for the terminal reset.

## `0xF1` sent after leaving raw wire

- **Trigger:** send `0xF1` at `BBIO1` as if it were a stop.
- **Symptom:** no stop result; one pin-level byte (`0x00`-`0x1F`) is
  returned, the target supply and pull-ups switch on, and the pin latches
  change.  The SMPS keeps running.
- **Behaviour:** in `BBIO1`, `0xF1` is a pin-state byte, not an SMPS command.
- **Recovery:** send `0x05` to return to `RAW1` and then `0xF1`; or send
  `0x0F` for the terminal reset.

## The `AUX1` pin is taken over while running

- **Trigger:** start the loop while `AUX1` was being used as a GPIO output, or
  while a terminal `g`/`S` generator was active.
- **Symptom:** `AUX1` no longer follows the level it had before the start; it
  carries the supply PWM.  After `0xF1` the pin is an input at low level, not
  the old output.
- **Behaviour:** the start disables the shared output-compare channel,
  detaches the terminal generator from `AUX` and clears its flag, then routes
  the PWM to `AUX1`.  The terminal generator does not resume when the SMPS
  stops.
- **Recovery:** stop the loop before using `AUX1` for anything else.  After
  the stop the pin is an input; set its direction and level explicitly if you
  need a driven level.  The previous `AUX1` state is not restored.

## `AUX1` was left open drain

- **Trigger:** use the v4 OpenOCD path in open-drain mode, then start the
  SMPS without a power cycle.
- **Symptom:** the start is accepted and the loop runs, but a logic high on
  `AUX1` is a released pin.  The add-on's PWM input may never see a high, so
  the switch may never turn on and the output is not controlled.
- **Behaviour:** the OpenOCD open-drain mode sets `AUX1` open drain.  Leaving
  that path does not necessarily clear it, and the SMPS start does not clear
  it.  A v4 terminal reset does not clear it either; a power cycle or hardware
  reset does.
- **Recovery:** power-cycle the Bus Pirate before the SMPS start, or fit a
  pull-up on the add-on's PWM input to a permitted logic rail
  (`unverified`).  Treat the pin as open drain until a power cycle.

## The ADC probe is taken over while running

- **Trigger:** run `d`, `d:N` or `D` at a terminal prompt, or `0x14`/`0x15`
  in `BBIO1`, while the loop is running.
- **Symptom:** terminal commands print:

  ```
  HiZ>d
  ADC busy: SMPS feedback active; command rejected.
  Syntax error at char 1
  HiZ>
  ```

  `d:N` and `D` print the same two lines with no header and no reading.
  Binary `0x14` and `0x15` return `0xFF 0xFF` once; `0x15` does not stream,
  so the byte after the pair is the next command, not a stream stop.
- **Behaviour:** the loop owns the `ADC` channel; the probe commands refuse
  rather than take the converter.  The loop continues until `0xF1` or a v4
  terminal reset.
- **Recovery:** stop the SMPS with `0xF1` in `RAW1`, or send `0x0F` from
  `BBIO1` for the terminal reset.  The exact binary recovery is in
  [ADC binary](../adc/binary.md#smps-owns-the-adc).

## The self-test is refused while the loop runs

- **Trigger:** send `0x10` or `0x11` in `BBIO1` while the loop runs.
- **Symptom:** one refusal byte `0x01`; no check runs.  The self-test then
  sits in its idle echo loop: every byte except `0xFF` is echoed back with the
  error count added (so `0x05` returns `0x06`), and `0xFF` leaves with `0x01`
  without re-printing `BBIO1`.  A host that sends its next command gets an
  echo, not an error, and loses alignment.
- **Behaviour:** the refusal count is the whole result of the command; the
  self-test loop still runs.  The SMPS loop keeps running.
- **Recovery:** send `0xFF` to leave the idle loop, then stop the SMPS with
  `0xF1` in `RAW1`, or send `0x0F` for the terminal reset.  See
  [self-test binary](../selftest/binary.md).

## A source on `ADC` while running

- **Trigger:** connect an external source to `ADC` while the loop runs.
- **Symptom:** the probe commands are refused, and the source is connected to
  the loop's feedback node.
- **Behaviour:** the pin is the feedback input, not a free measurement point.
  An external source changes the feedback and therefore the regulation.
- **Recovery:** stop the loop, wire the source, then measure; restart the
  loop only after removing the source.

## Starting again while running

- **Trigger:** send a new start without a stop.
- **Symptom:** the reply is `0x01`; the new duty and target take effect.  A
  `0xF0` read immediately after the new start can still return the previous
  sample.
- **Behaviour:** the start routine re-runs in place.  It disables the old
  conversion and PWM, recomputes the values, re-routes `AUX1` and re-enables
  the feedback.  No stop is needed and no stop happens; the stored sample is
  not cleared, and there is no freshness flag.
- **Recovery:** none needed.  Discard the first read after the new start, or
  poll until the value changes.  Send `0xF1` when the new setpoint is no
  longer wanted.

## Stop is not a disconnect

- **Trigger:** assume `0xF1` turns the add-on's output off.
- **Symptom:** the firmware reports `0x01` and the PWM stops, but the add-on
  may hold its output or remain powered.
- **Behaviour:** stop removes the PWM drive and parks `AUX1`; it does not
  control the add-on's supply, and the firmware has no independent voltage,
  current or temperature protection.  The add-on's behaviour is a property of
  the add-on (`unverified`).
- **Recovery:** remove the add-on's supply or use its own enable or
  disconnect.  Treat `0xF1` as a control-path stop, not a safety disconnect.

## A v3 build cannot enable SMPS

- **Trigger:** enable SMPS in a v3 build.
- **Symptom:** the build stops at a compile-time error.
- **Behaviour:** the feature is v4-only; the v3 firmware has no `AUX1` target
  and no SMPS driver.
- **Recovery:** build v4.  A v3 image without the feature still routes the
  command group and refuses every request with `0x00` (not verified on v3
  hardware).
