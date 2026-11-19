# SMPS - PWM and ADC feedback loop

Part of the SMPS set (`docs/capabilities/smps/`).  This page covers what a
start configures, the duty and target arithmetic, the sample that `0xF0`
reports, the firmware cutoff limits, and what ADC ownership means.  The command
bytes are in [binary.md](binary.md); the stop and reset paths are in
[exit.md](exit.md); the pins are in [wiring.md](wiring.md).

## Scope

Covered: the control loop and its observable behaviour.  Not covered: the
add-on's internal switching design, its output range, or any voltage or
current calibration.  Those are properties of the external add-on and are
`unverified`.

## What a start configures

A start (`0xF2`-`0xFF` plus one operand) runs this sequence before it replies
`0x01`:

1. Releases any previous owner: disables the ADC interrupt, turns the
   converter and auto-sampling off, and disables the output-compare channel.
2. Detaches the terminal generator from the `AUX` output and clears its mode
   flag, so a `g`/`S` waveform does not remain mirrored on the shared channel.
   The terminal generator does not resume when the SMPS stops.
3. Computes the duty compare value and the feedback target from the request.
4. Drives `AUX1` low and routes the output-compare channel to the pin, with a
   period register of `0x7F` and edge-aligned PWM.
5. Selects the `ADC` input, enables auto-sampling, and enables the
   conversion-complete interrupt only after the loop state is ready.
6. From then on, every conversion stores a new sample and adjusts the duty.

Ownership lasts until `0xF1` or a v4 terminal reset; it is not tied to being
in `RAW1` ([exit.md](exit.md)).

## Duty and target

Both values are derived from the requested value:

```
duty   = 128 - (64000 / requested)
target = requested * 45 / 58
```

Both divisions are integer.  The target product is formed before the
division, so the full reachable range produces a defined target and does not
wrap.  The request cannot be zero, so the division is safe.

- The duty value is loaded as the PWM compare value against a period of
  `0x7F` (127).  Over the reachable requests it ranges from 3 to 113.  A
  higher requested value gives a higher or equal duty; the step is not always
  visible because the division is integer.
- `target` is a raw comparison threshold on the same numeric scale as the
  10-bit sample, but it is a 16-bit value.  It ranges from 397 at `0x200` to
  3177 at `0xFFF`; a 10-bit sample can never reach a target above 1023.

| Requested | Example command | Target (threshold) | Duty compare |
|---|---|---|---|
| `0x200` | `0xF2` `0x00` | 397 | 3 |
| `0x258` | `0xF2` `0x58` | 465 | 22 |
| `0x526` | `0xF5` `0x26` | 1022 | 80 |
| `0x527` | `0xF5` `0x27` | 1023 | 80 |
| `0xFFF` | `0xFF` `0xFF` | 3177 | 113 |

The unit of the requested value is not defined by the firmware; the table is
an arithmetic aid, not a voltage table.

## The sample

- The converter is 10-bit: a sample is a raw count in `0`-`1023`.
- The `ADC` pin is the Bus Pirate's probe input, behind the on-board 2:1
  divider: nominally 0-6.6 V at the pin, about 6.45 mV per count, with raw 1023
  as the largest reading.  The scale and accuracy are owned by
  [the ADC set](../adc/README.md).
- How the add-on's output maps to the `ADC` pin, and therefore to counts, is
  part of the add-on design (`unverified`).  The scaling may use the pin's own
  divider, a divider on the add-on, or both.
- While the loop runs, each conversion stores the new sample.  `0xF0` reports
  that stored value; it does **not** start a conversion.
- After stop, the converter is off, so the stored sample stops changing.
  `0xF0` then repeats the last sample from the run.
- A start does not clear the stored sample.  The first read after a new start
  can still be from the previous run.
- There is no freshness flag.  Discard the first read after a start.  If you
  need evidence of a new conversion, poll until the value changes; a stable
  output can legitimately repeat, so that is a heuristic, not a guarantee.
- At power-up the stored sample is zero, so `0xF0` before the first start
  returns `0x00 0x00`.

## Regulation

On every ADC conversion the firmware compares the new sample with the target:

```
if sample > target:  duty = 0
else:                duty = computed duty
```

The comparison is strict.  A sample exactly equal to the target leaves the
computed duty applied; only a sample above the target cuts the drive to zero.
The control is bang-bang: the duty is either zero or the computed value, with
no proportional or integral term.  The duty changes once per ADC conversion,
not once per PWM cycle, so the loop corrects in steps.

The PWM period register is `0x7F`.  The switching frequency is approximately
125 kHz (`unverified`).

### Where the firmware cutoff can act

This table describes the firmware comparison, not the add-on's regulated
output range.

| Requested | Target | Behaviour |
|---|---|---|
| `0x200`-`0x526` | 397-1022 | a 10-bit sample can exceed the target, so the cutoff can fire; whether the add-on reaches the target is unknown |
| `0x527`-`0xFFF` | 1023-3177 | the target is at or above every possible sample, the cutoff never fires, and the PWM stays at the computed duty (open loop) |

At `0x527` the target is exactly 1023, the largest possible sample.  Because
the comparison is strict, no sample can exceed it, so the cutoff never fires.
There is no overflow wrap and no error report: a request above `0x526` is
simply an open-loop duty setting.

Even inside `0x200`-`0x526`, the cutoff acting is not a promise that the output
reaches the requested value.  The add-on must be able to drive its feedback
node to the target.  The firmware cannot detect a missing add-on, a
disconnected feedback wire, or an add-on that cannot reach the requested
output; it only sees the `ADC` counts.

## Ownership

While the loop runs, the feedback owns the ADC and the supply PWM owns the
output channel:

| Attempt | Where | Result | Detail owner |
|---|---|---|---|
| `d`, `d:N` or `D` | terminal | refused: `ADC busy: SMPS feedback active; command rejected.` then `Syntax error at char 1` | [ADC terminal](../adc/terminal.md#smps-owns-the-adc) |
| `0x14` / `0x15` | `BBIO1` | `0xFF 0xFF` once; `0x15` does not stream | [ADC binary](../adc/binary.md#smps-owns-the-adc) |
| `g` / `S` | terminal | refused with the same `ADC busy` line and syntax error | [AUX PWM and servo](../auxiliary/pwm-servo.md#smps-owns-the-channel) |
| `0x12` / `0x13` | `BBIO1` | `0x00`; `0x12` consumes its five-byte frame first | [binary commands](../../protocol/binary/commands.md) |
| `0x10` / `0x11` | `BBIO1` | refusal count `0x01`; no check runs; the self-test then sits in its idle echo loop - send `0xFF` to leave (`BBIO1` is not re-printed) | [self-test binary](../selftest/binary.md) |
| `P` | terminal | refused | [generic edge cases](../../generic/edge-cases.md) |
| `W` | terminal | refused with the same `ADC busy` line and syntax error; `w` still works | [generic edge cases](../../generic/edge-cases.md) |
| `0x5x` | `RAW1` | `0x00`; no pull-up feed | [the shared `RAW1` page](../../protocol/wire/binary.md) |

The full refusal list and the exact per-command shapes are owned by those
pages.  The terminal `W` refusal happens because `W` checks the rails through
the ADC; `w` only turns the rails off and is not blocked, and the binary
`0x4x` supply bit is not blocked either.  Two points matter here:

- The terminal `d`/`D` refusal is defensive.  Every normal route from the
  binary layer back to `HiZ>` runs the v4 terminal reset, which stops the
  loop, so a terminal prompt should not normally reach it.  If it appears,
  send `#`.
- Leaving `RAW1` with `0x00` does **not** release the ADC or `AUX1`.  The loop
  keeps running in `BBIO1` and the refusals continue until `0xF1` or a v4
  terminal reset ([exit.md](exit.md)).

## What stop changes

`0xF1` runs the stop routine; the full path and reset variants are in
[exit.md](exit.md).  In loop terms it:

- disables the ADC interrupt, turns the converter and auto-sampling off;
- sets the duty and period to zero, clears the output-compare sync source and
  disables the channel;
- makes `AUX1` an input, clears its output routing and drives its latch low;
- leaves the stored sample unchanged.

## Limits

- Bang-bang control has no hysteresis setting, no current limit and no fault
  detection.  Ripple and settling are properties of the add-on and the load.
- `0xF0` is an unfiltered stored count, not an average, RMS value or
  calibrated voltage.
- No unit, no output calibration and no add-on detect.
- The loop can only act on the `ADC` node; it cannot tell "zero output" from
  "no feedback" when both read zero.
- The add-on's output range, isolation, protection and safe operating limits
  are not defined by the firmware (`unverified`).
