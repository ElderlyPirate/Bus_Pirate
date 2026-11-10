# SMPS — edge cases and failure modes

Part of the SMPS documentation set (`docs/capabilities/smps/`).  Each entry is
a concrete trigger → symptom → behaviour → recovery.

## The SMPS feature is not built in

- **Trigger:** send any `0xF0`–`0xFF` on a default image (SMPS off) or on any
  v3 build.
- **Symptom:** every request is answered with `0x00`; in particular `0xF0`
  returns a single failure byte, not the two-byte reading.  A start
  (`0xF2`–`0xFF`) leaves its operand byte unread, so the operand is then
  parsed as the next raw-wire request: `0xF2 0x00` answers `0x00` and then
  leaves raw wire (`BBIO1`); `0xF2 0x58` answers `0x00` and then treats
  `0x58` as a pull-up request.
- **Behaviour:** the command group is still routed, but the handler refuses
  all requests when support is not compiled in.  The disabled path does not
  consume a start's operand.
- **Recovery:** enabling SMPS is a source build change, not a runtime setting
  (see [README.md](README.md) §Availability); on v3 it cannot be enabled at
  all.  If a stray operand left you in raw wire, send `0x01` to reprint the
  mode identifier (`RAW1`) and resynchronise; if the operand was `0x00`, you
  are back in `BBIO1` — re-enter raw wire with `0x05`.

## A requested value below `0x200`

- **Trigger:** try to start at `0xF0` or `0xF1` — any `0xFx` whose low nibble
  is `0` or `1` — with an operand byte.
- **Symptom:** `0xF0` is a read and `0xF1` is stop, so no start happens; the
  operand byte is then taken as the next request.  The sharpest case is
  `0xF1 <byte>`: it stops the board and then feeds `<byte>` as the next
  request — a `0x00` operand ejects you from raw wire entirely.
- **Behaviour:** the start encodings begin at `0xF2`, so requested values
  `0x000`–`0x1FF` are unreachable.
- **Recovery:** choose a reachable value (`0x200`–`0xFFF`) or rescale the
  intent.  After a stray operand, send `0x01` to reprint the mode identifier
  (`RAW1`) and resynchronise; if the operand was `0x00`, re-enter raw wire.

## No board connected, or nothing to read

- **Trigger:** `0xF0` before a start, or with no board on the pins.
- **Symptom:** the reading is `0x0000` or an arbitrary value.
- **Behaviour:** the ADC samples the `ADC` pin whatever is on it (floating →
  noise); the firmware does not detect the board and has no presence test.
  The scale is in [ADC probe](../adc/README.md).
- **Recovery:** start the board first and treat readings as `unverified`;
  there is no way to distinguish "no board" from "zero output".

## A reading taken after stop is stale

- **Trigger:** `0xF1` followed by `0xF0`.
- **Symptom:** the reading repeats the last sample taken while running.
- **Behaviour:** stop leaves the stored reading unchanged, and `0xF0` never
  clears it.
- **Recovery:** treat a reading taken after stop as the last value from the
  previous run, not a live measurement.

## Leaving raw wire while the board is running

- **Trigger:** send `0x00` (leave raw wire) without first sending `0xF1`.
- **Symptom:** the board is left switching; re-entering raw wire finds it
  still running.
- **Behaviour:** the raw-wire exit path does not stop the board, and the PWM
  and ADC are not otherwise cleaned up — only `0xF1` sent while still in raw
  wire stops them.  A hardware reset also stops the board; whether the
  terminal reset that returns from the binary layer stops it is `unverified`.
- **Recovery:** always send `0xF1` before leaving raw wire.  (`unverified`:
  the state seen on re-entry is inferred from the firmware.)

## The `AUX1` pin is taken over while running

- **Trigger:** start the board while `AUX1` was being used as a GPIO output
  or another peripheral.
- **Symptom:** `AUX1` no longer follows the `a`/`A` controls, and a stop
  detaches the PWM — the pin keeps its port direction and output latch state,
  so from a GPIO-output state it stays an output at its last level.
- **Behaviour:** starting reassigns `AUX1` to the PWM output; stopping detaches
  the PWM source but does not reset the pin's direction or latch.
- **Recovery:** stop the board before using `AUX1` for anything else, and set
  the pin's direction and level explicitly if you need a known state.

## The ADC probe is taken over while running

- **Trigger:** run `d`/`D`, or binary `0x14`/`0x15`, while the SMPS is
  running.
- **Symptom:** the reading follows the SMPS feedback, not a source connected
  to the `ADC` pin; a probe command also turns the converter off at the end,
  so the feedback loop stops updating until the next start.
- **Behaviour:** start selects the probe channel for SMPS feedback and enables
  continuous sampling; the probe commands read that same channel.
- **Recovery:** stop the SMPS before using the probe for an external source,
  or start the SMPS again afterwards.  Scale and limits:
  [ADC probe](../adc/README.md).

## Starting again while running

- **Trigger:** send a new start (`0xF2`–`0xFF` plus its operand) without a
  stop.
- **Symptom:** the reply is `0x01`; the new request takes effect.  A `0xF0`
  read immediately after the new start can still return the previous sample.
- **Behaviour:** the start routine re-runs in place — it recomputes the duty
  and target, re-selects the PWM and ADC, and re-applies the registers.  No
  stop is needed and no stop happens; the stored sample is not cleared.
- **Recovery:** none needed; send `0xF1` when the new setpoint is no longer
  wanted.

## The requested value is not a voltage

- **Trigger:** expecting the request or the reading to be in volts.
- **Symptom:** the numbers do not correspond to a known voltage.
- **Behaviour:** the firmware works in raw values and converts neither the
  request nor the report to volts.
- **Recovery:** treat the value as board-specific and `unverified`; do not
  calibrate against it without the board's own documentation.
