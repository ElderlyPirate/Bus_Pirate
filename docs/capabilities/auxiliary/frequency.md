# Frequency measurement

Part of the AUX capability documentation set.  This page owns the terminal
frequency counter (`f`) and the capability behind the binary frequency
command.  The pin and its assignment are in [aux-pin.md](aux-pin.md); the
binary byte framing is in
[protocol/binary/commands.md](../../protocol/binary/commands.md).

## Scope

Covers what `f` measures, the gate and autorange behaviour, the display
resolution, and the interaction with the PWM/servo generator.  It is not the
ADC: `d` / `D` measure a voltage on the `ADC` pin, while `f` counts edges on
the `AUX` pin.  It is not a duty-cycle or pulse-width meter either.

## The input pin

The counter always uses the physical `AUX` pin — the `A0` target — whatever
`c` / `C` / `k` / `K` has selected.  It makes that pin an input for the
measurement, so a manual `A` on `AUX` is overridden.  The pin remains an
input after the measurement.

`f` works in HiZ as well as in a bus mode.  It needs no target supply and no
Bus Pirate pull-up; the signal must provide its own logic levels and share the
board's ground ([wiring.md](wiring.md)).

## Usage

The command is `f`.  The header `AUX Frequency: ` is printed, the measurement
runs, and the value is printed with ` Hz`.  A 10 kHz input reads:

```
UART>f
AUX Frequency:  autorange 10,000 Hz
UART>
```

A 1 kHz input reads:

```
UART>f
AUX Frequency:  autorange  autorange 1,000.00 Hz
UART>
```

The firmware prints thousands separators in decimal values, so `1,000.00 Hz`
is 1000 Hz and `10,000 Hz` is 10 kHz.

`autorange` appears once for each range step the firmware takes.  A reading
above about 4.19 MHz skips the rerun and prints no `autorange`; a reading
below 4 kHz takes both steps and prints it twice.  A zero count takes the
first step and prints one `autorange` before the unsupported message.  The
doubled token and the padding spaces are part of the output, not a
transcription error.

With no edges in the gate:

```
UART>f
AUX Frequency:  autorange Frequencies < 1Hz are not supported.
UART>
```

While the terminal generator is running, `f` refuses before it prints the
header:

```
UART>f
ERROR: PWM active, g to disable
UART>
```

The message says PWM even when a servo is the active generator.  Stop it with
a bare `g` or `S` ([pwm-servo.md](pwm-servo.md)).

A measurement takes about one second above 4.19 MHz, about two seconds in
the 4 kHz to 4.19 MHz range, and about three seconds for a low-frequency
reading (two gates plus the period average).

## Gate and autorange

1. Timer 2 counts `AUX` edges for one second with a 1:256 input prescaler.
2. If the count is 16383 or less, the firmware prints `autorange`, switches
   to the 1:1 prescaler and measures again.
3. A result above 3999 Hz is printed as an integer in Hz.
4. A result from 1 to 3999 Hz goes through a second `autorange` step: the
   firmware measures the period over about one second of cycles and prints a
   decimal value.
5. If the 1:1 count is zero, the firmware prints the
   `Frequencies < 1Hz are not supported.` line after the first `autorange`
   step.

The consequence is a split range:

| Input | Measurement | Resolution |
|---|---|---|
| above about 4.19 MHz | 1:256 prescaler, value multiplied by 256 | 256 Hz steps |
| about 4 kHz to 4.19 MHz | 1:1 gate | 1 Hz |
| 1 Hz to 3999 Hz | period averaging | 1–5 decimal places |
| no edges | — | unsupported message |

The decimal places in the low range follow the measured period: about five
places below 40 Hz, four to about 126 Hz, three to about 400 Hz, two to about
1.26 kHz, and one above that.  They express the averaged period, not a
separate accuracy specification.

## Range and accuracy

- A gate with no edges reports the unsupported message; a signal below about
  1 Hz can miss the gate entirely.  When an edge is caught, the period path
  can display a value below 1 Hz, but the result is not reliable.
- On the 1:1 path the resolution is one count in one second, i.e. 1 Hz.  The
  gate and reference-clock tolerances are not specified here (unverified).
- Above about 4.19 MHz the prescaler stays engaged and the reading is a
  multiple of 256 Hz, so it is coarser, not more precise.
- The highest input the pin and counter can accept is a hardware limit; it is
  not stated in the documentation (unverified).  The counter itself is
  32-bit.
- A slow or noisy edge can be counted more than once.  Drive the pin with a
  clean logic signal.

## The binary command

The binary layer exposes the same counter as `0x16`.  It returns four bytes,
most significant first, in Hz, and returns `0` when no edges are counted.  It
forces `AUX` to an input and uses the same one-second gate, but its rerun
threshold is different: it switches to 1:1 only when the first gate counted
255 or fewer edges.  Above about 65.28 kHz the binary reading is therefore a
multiple of 256 Hz, while the terminal `f` remains resolved to 1 Hz up to
about 4.19 MHz.  Allow about one second above about 65.28 kHz; the measurement
takes about two seconds when the 1:1 rerun runs.

`0x16` stops a running PWM rather than refusing.  The terminal `f` refuses
instead.  The byte framing and the full failure behaviour are in
[protocol/binary/commands.md](../../protocol/binary/commands.md) and
[protocol/binary/edge-cases.md](../../protocol/binary/edge-cases.md).

## Limits

- `AUX` only; `CS`, `AUX1` and `AUX2` cannot be used as the counter input.
- Frequency only: no duty cycle, pulse width, period or edge count.
- The pin must be an input; `f` changes it to one, releasing a manual drive.
- While the terminal generator is active, use the binary `0x16` or stop the
  generator first.
- v3 is build-only here and unverified.

## Related

- [aux-pin.md](aux-pin.md) — the pin and the target assignment.
- [pwm-servo.md](pwm-servo.md) — the generator that shares the pin.
- [edge-wait.md](edge-wait.md) — waiting for one edge instead of counting.
- [wiring.md](wiring.md) — logic levels and grounding.
- [docs/generic/commands.md](../../generic/commands.md) — the `f` entry.
- [docs/capabilities/adc/README.md](../adc/README.md) — the analogue probe.
- [docs/protocol/binary/commands.md](../../protocol/binary/commands.md) —
  `0x16`.
- [docs/SPEC.md](../../SPEC.md) — the frequency-counter row.
