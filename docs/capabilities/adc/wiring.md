# ADC probe — wiring and input limits

Part of the ADC capability set (`docs/capabilities/adc/`).  This page owns the
pin, the ground, the input range and the safety limits.  The scale is in
[probe.md](probe.md).

## Scope

Covered: which pin is the probe, how to reference the measurement, the allowed
input range, the source requirements, and what not to connect.  Not covered:
other functions that happen to read the same pin (the self-test jumper,
AVRISP VTARGET) — those pages own their own wiring.  One exception is
repeated below: a fitted self-test jumper is a direct hazard for anyone
measuring with the probe.

## One line per wire

| Wire | From | To | Required |
|---|---|---|---|
| signal | the node to measure | the `ADC` pin | yes |
| ground | the source's ground | a `GND` pin | yes |

**No pull-up is required.**  The probe is a divider input, not a bus line, so
there is no pull-up and nothing to enable with `P`.  **No supply is
required:** the reading works with the supplies off and in HiZ, because the
converter's reference is the board's own 3.3 V rail.

## The pin

| Board | Pin | Header |
|---|---|---|
| v4 | `ADC` | pin 8 of the shrouded 2×6 I/O header, counted from the key (key = pin 1); `GND` is pin 12 |
| v3 | `ADC` | the pin labelled `ADC` on the I/O header |

The silkscreen label `ADC` is the authority.  The v4 numbering is the
convention published in the
[AVRISP wiring table](../../programmers/avrisp/wiring.md); this page does not
define a second one.

The probe is **not** one of the digital I/O pins: no mode drives it, and no
`m` setting or AUX target changes it.  The digital `AUX` / `AUX1` / `AUX2`
pins are separate ([AUX wiring](../auxiliary/wiring.md)).

## Input range

| Item | Value |
|---|---|
| measurement range (nominal) | 0 to 6.6 V |
| divider | 2:1 (the converter sees half the input) |
| largest reading | 6.59 V (raw 1023) |
| below ground | not measurable; may damage the pin |
| above 6.6 V | clipped reading; potentially damaging |

The 6.6 V figure is the firmware's scale with the converter reference at
nominal 3.3 V: it is the reliable measurement limit, not a documented absolute
maximum for the pin.  Above 6.6 V the reading stops at 6.59 V and cannot tell
you how far over the input is; treat any voltage above 6.6 V as potentially
damaging, not as a harmless clip, and disconnect it.

The reading is single-ended: it is measured against the Bus Pirate's `GND`.
Connect the source ground to the Bus Pirate ground; without it the reading is
meaningless.

The range and divider above describe the v4 board.  On v3 the firmware uses
the same scale, but the v3 divider and its safe input range are **assumed 2:1
and unverified** — use the silkscreen to locate the pin and check the board
before applying a voltage near 6.6 V.

## Source requirements

- The source must stay within the 0–6.6 V measurement range; above it the
  input is potentially damaging, not merely clipped.
- The reading is an instantaneous sample, not an average; a changing signal
  gives an arbitrary point in its waveform.
- The converter samples through a short, fixed window, so a high-impedance
  source may read low or wander.  Keep the source impedance low; the maximum
  usable source impedance is not specified (`unverified`).
- The divider's resistor values, and therefore the input impedance, are not
  stated here (`unverified`).  Treat the input as a sense input, not a load.
- A change smaller than about 6.45 mV is below one count; a change smaller
  than 10 mV is below the terminal display.

## The SMPS feedback path

On an SMPS-enabled v4 build, a running SMPS feedback loop uses the `ADC` pin
as its output-voltage feedback ([probe.md](probe.md#who-owns-the-converter)).
While the loop runs:

- the probe commands are refused (`d` / `D` print the rejection line;
  `0x14` / `0x15` return `0xFF 0xFF` once);
- do **not** connect an external source to the `ADC` pin.  The pin is the
  feedback node, not a free measurement input.

Stop the loop with `0xF1` in raw wire first (from `BBIO1`, send `0x05` before
`0xF1`), then wire the source.  The full sequence is in
[binary.md §SMPS owns the ADC](binary.md#smps-owns-the-adc).

## What not to connect

| Do not | Because |
|---|---|
| Apply more than 6.6 V | Potentially damaging; the reading clips at 6.59 V with no over-range flag.  The absolute maximum is undocumented.  On v3 the limit is unverified. |
| Apply a negative voltage | Not measurable below ground, and the pin may be damaged. |
| Connect mains or a non-isolated circuit | The probe shares ground with the Bus Pirate and the host; there is no isolation. |
| Use the probe as an output | Nothing in the firmware drives this pin; it is an input only. |
| Leave the self-test `ADC`→`+3.3V` jumper fitted with a source connected | Plain wire to the `+3.3V` rail; a connected source back-drives it and can damage the board or source. |
| Connect an external source while the SMPS loop runs | The pin is the loop's feedback node, not a free input (see above). |

## v3 / v4

| | v3 | v4 |
|---|---|---|
| probe pin | the pin labelled `ADC` | `ADC`, pin 8 (key = pin 1) |
| ground | `GND` | `GND`, pin 12 |
| divider | assumed 2:1, unverified | on-board 2:1 |
| range | 0–6.6 V (firmware scale; divider unverified) | 0–6.6 V |
| SMPS feedback on this pin | — | optional build |

The v3 column is **not verified on v3 hardware**.

## Related

- [probe.md](probe.md) — the conversion, channels and ownership.
- [terminal.md](terminal.md) — how to take a reading.
- [../selftest/terminal.md](../selftest/terminal.md) — the `ADC` jumper check.
- [../smps/README.md](../smps/README.md) — the SMPS feedback loop.
- [../../programmers/avrisp/wiring.md](../../programmers/avrisp/wiring.md) —
  the v4 header numbering.
