# SMPS — edge cases and failure modes

Part of the SMPS documentation set (`docs/smps/`).  Each entry is a concrete
trigger → symptom → behaviour → recovery.

## The SMPS feature is not built in

- **Trigger:** send any `0xF0`–`0xFF` on a default image (SMPS off) or on any
  v3 build.
- **Symptom:** every request is answered with `0x00`; in particular `0xF0`
  returns a single failure byte, not the two-byte reading.
- **Behaviour:** the command group is still routed, but the handler refuses
  all requests when support is not compiled in.
- **Recovery:** build a custom v4 image with SMPS support enabled; on v3 it
  cannot be enabled at all.

## A requested value below `0x200`

- **Trigger:** try to start at `0xF0` or `0xF1` (or any code whose low nibble
  is `0` or `1`) with an operand byte.
- **Symptom:** `0xF0` is a read and `0xF1` is stop, so no start happens; the
  operand byte is then taken as the next request.  The sharpest case is
  `0xF1 <byte>`: it stops the board and then feeds `<byte>` as the next
  request — a `0x00` operand ejects you from raw wire entirely.
- **Behaviour:** the start encodings begin at `0xF2`, so requested values
  `0x000`–`0x1FF` are unreachable.
- **Recovery:** choose a reachable value (`0x200`–`0xFFF`) or rescale the
  intent; after a stray operand, send a harmless request or leave and re-enter
  raw wire to resynchronise.  Re-enter raw wire if a `0x00` operand ejected
  you.

## No board connected, or nothing to read

- **Trigger:** `0xF0` before a start, or with no board on the pins.
- **Symptom:** the reading is `0x0000` or an arbitrary value.
- **Behaviour:** the ADC samples the `ADC` pin whatever is on it (floating →
  noise); the firmware does not detect the board and has no presence test.
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
  and ADC are not otherwise cleaned up — only `0xF1` (or a reset) stops them.
- **Recovery:** always send `0xF1` before leaving raw wire.  (`unverified`:
  the state seen on re-entry is inferred from the firmware.)

## The `AUX1` pin is taken over while running

- **Trigger:** start the board while `AUX1` was being used as a GPIO output
  or another peripheral.
- **Symptom:** `AUX1` no longer follows the `a`/`A` controls, and a stop
  returns it to an undriven state.
- **Behaviour:** starting reassigns `AUX1` to the PWM output; stopping frees
  the pin.
- **Recovery:** stop the board before using `AUX1` for anything else.

## The requested value is not a voltage

- **Trigger:** expecting the request or the reading to be in volts.
- **Symptom:** the numbers do not correspond to a known voltage.
- **Behaviour:** the firmware works in raw values and converts neither the
  request nor the report to volts.
- **Recovery:** treat the value as board-specific and `unverified`; do not
  calibrate against it without the board's own documentation.