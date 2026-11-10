# ADC probe — wiring and input limits

Part of the ADC capability set (`docs/capabilities/adc/`).  This page owns the
pin, the ground, the input range and the safety limits.  The scale is in
[probe.md](probe.md).

## Scope

Covered: which pin is the probe, how to reference the measurement, the allowed
input range, the source requirements, and what not to connect.  Not covered:
other functions that happen to read the same pin (the self-test jumper, AVRISP
VTARGET) — those pages own their own wiring.  One exception: a fitted
self-test jumper is a hazard for this page's reader, so it is repeated under
What not to connect.

## One line per wire

| Wire | From | To | Required |
|---|---|---|---|
| signal | the node to measure | the `ADC` pin | yes |
| ground | the source's ground | the `GND` pin | yes |

**No pull-up is required.**  The probe is a divider input, not a bus line, so
there is no pull-up and nothing to enable with `P`.  **No supply is required:**
the reading works with the supplies off and in HiZ, because the converter's
reference is the board's own 3.3 V rail.

## The pin

| Board | Pin | Header |
|---|---|---|
| v4 | `ADC` | pin 8 of the shrouded 2×6 I/O header, counted from the key (key = pin 1) |
| v3 | `ADC` | the pin labelled `ADC` on the I/O header (v3 is build-only here, unverified) |

The silkscreen label `ADC` is the authority; the pin number is a convenience
for the v4 header and uses the same convention as the
[AVRISP wiring table](../../programmers/avrisp/wiring.md).

The pin sits behind an on-board 2:1 divider.  It is **not** one of the digital
I/O pins: no mode drives it, and no `m` setting changes it.

## Input range

| Item | Value |
|---|---|
| measurable range | 0 to 6.6 V |
| divider | 2:1 (the converter sees half the input) |
| full-scale reading | 6.59 V (raw 1023) |
| below ground | not measurable |
| above 6.6 V | out of range; the scale ends at 6.59 V |

The reading is single-ended: it is measured against the Bus Pirate's `GND`.
Connect the source ground to the Bus Pirate ground; without it the reading is
meaningless.

The range and divider above describe the v4 board.  On v3 the firmware uses the
same scale, but the v3 divider and its safe input range are **assumed 2:1 and
unverified** — check the board before applying a voltage near 6.6 V.

## Source requirements

- The source must stay within 0–6.6 V.
- The reading is an instantaneous sample, not an average; a changing signal
  gives an arbitrary point in its waveform.
- The converter samples through a short, fixed window, so a high-impedance
  source may read low or wander.  Keep the source impedance low; the maximum
  usable source impedance is not specified (unverified).
- The divider's resistor values, and therefore the input impedance, are not
  stated here (unverified).  Treat the input as a sense input, not as a load.
- A change smaller than about 6.45 mV is below one count; a change smaller
  than 10 mV is below the terminal display.

## What not to connect

- More than 6.6 V.  The scale stops there, and the pin's absolute maximum is
  not documented here; 6.6 V is the limit that can be relied on.  On v3 this
  limit is unverified (see above).
- A negative voltage.
- Mains, or any circuit that is not isolated from the mains.  The probe
  shares ground with the Bus Pirate and the host; there is no isolation.
- The probe as an output.  Nothing in the firmware drives this pin.
- A source while the self-test `ADC` → `+3.3 V` jumper is still fitted —
  remove the jumper first.  The full self-test asks for it; leaving it in ties
  the `ADC` pin toward 3.3 V and corrupts any measurement.  See
  [../selftest/terminal.md](../selftest/terminal.md).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| probe pin | `ADC` | `ADC` (pin 8, key = pin 1) |
| divider | assumed 2:1 (unverified) | on-board 2:1 |
| range | 0–6.6 V (firmware scale; divider unverified) | 0–6.6 V |

The v3 behaviour is code-derived and **unverified**; use the silkscreen to
locate the pin, and treat the divider and range as assumptions until the board
is checked.

## Related

- [probe.md](probe.md) — the conversion and its accuracy.
- [terminal.md](terminal.md) — how to take a reading.
- [../selftest/terminal.md](../selftest/terminal.md) — the `ADC` jumper check.
