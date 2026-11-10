# ADC probe — the input and the scale

Part of the ADC capability set (`docs/capabilities/adc/`).  This page explains
what the probe input measures, how a raw count becomes a voltage, and where the
limits are.  The terminal and binary front ends are in
[terminal.md](terminal.md) and [binary.md](binary.md); the wiring is in
[wiring.md](wiring.md).

## Scope

Covered: the on-board converter, the probe divider, the raw-to-volts rule,
resolution and accuracy limits, and the other channels the same converter
samples.  Not covered: the prompt grammar and the binary framing.

## The converter

- 10-bit converter, one conversion per reading.
- The result is an integer count, 0..1023 (1024 steps).
- The converter's reference is the board's 3.3 V rail; the input is divided by
  two before the converter, which is why full scale is 6.6 V.
- The firmware does not average or filter: `d` and `0x14` are one conversion,
  and each continuous sample is one conversion.
- No calibration constant is applied; the count is scaled by a fixed ratio.

## The divider and the scale

An on-board 2:1 divider sits between the `ADC` pin and the converter.  The
converter therefore sees half the pin voltage, and the firmware doubles it
back:

```
volts = raw / 1024 × 6.6
```

The same raw-to-volts relation is used for every ADC display: the `d` one-shot
and the `v` table print the two-decimal form above, the self-test prints the
same value, the AVRISP VTARGET reading reports the probe in tenths of a volt,
and OpenOCD's `buspirate adc` reads the same four channels (probe, VPU, 3.3 V
and 5.0 V).  The binary layer returns the raw count and leaves the conversion
to the host.

| Raw | Ideal input | Printed by `d` |
|---|---|---|
| 0 | 0.000 V | `0.00V` |
| 1 | 0.006 V | `0.00V` |
| 78 | 0.503 V | `0.50V` |
| 256 | 1.650 V | `1.64V` |
| 512 | 3.300 V | `3.29V` |
| 768 | 4.950 V | `4.94V` |
| 1023 | 6.594 V | `6.59V` |

The `Printed by d` column is the firmware's fixed-point result after
truncation, not a second measurement.

## Why 3.3 V can print 3.29 V

The firmware computes hundredths of a volt with a fixed-point ratio that is a
shade below the exact 6.6 / 1024 (the difference is under 1 mV over the full
scale), and the display truncates.  An exact 0.01 V step can therefore land
one step low: 1.650 V prints `1.64V`, and 3.300 V prints `3.29V`.

This is a rounding/display effect, not a 10 mV measurement error.  The raw
count in parentheses is the unrounded figure.  The top of the scale prints
`6.59V` for the same reason plus the 1023-count limit (1023 counts is
6.594 V).

## Resolution and accuracy

| Item | Value |
|---|---|
| converter resolution | 10 bits, 1024 counts |
| one count | 6.6 V / 1024 = 6.45 mV |
| displayed resolution | 0.01 V (about 1.55 counts) |
| arithmetic error | under 1 mV (fixed-point conversion) |
| reference | the board's 3.3 V rail, nominal |
| overall accuracy | not specified (unverified) |

- The raw count is finer than the display: use the `(raw n)` value, or the
  binary sample, when a change smaller than 10 mV matters.
- The conversion is ratiometric to the 3.3 V rail: if that rail is not at its
  nominal value, every reading shifts with it.
- Divider tolerance, offset and gain error are not characterised here; do not
  treat the reading as a calibrated instrument.

## The other channels

The same converter samples the board's own rails for housekeeping.  Only the
probe channel has user commands; the rest surface through other features.

| Channel | Measures | Reaches the user through |
|---|---|---|
| probe | the `ADC` pin, via the divider | `d` / `D`, binary `0x14` / `0x15`, BASIC `ADC`, AVRISP VTARGET, OpenOCD `buspirate adc` |
| 3.3 V rail | the on-board 3.3 V supply | `v`, the self-test, the `W` check, OpenOCD `buspirate adc` |
| 5.0 V rail | the on-board 5.0 V supply | `v`, the self-test, the `W` check, OpenOCD `buspirate adc` |
| VPU (the `Vpullup` pin) | the pull-up reference pin | `v`, the `P` warning, the self-test, OpenOCD `buspirate adc` |
| Vusb (v4) | the USB 5 V input | the self-test |

The firmware applies the same 2:1 scale to every channel it displays.  The rail
and VPU channels are housekeeping: they are read by the features listed, not
selected by the user.  In the optional SMPS build the probe channel is the
SMPS output-voltage feedback, so the probe pin is not free for measurements
while the SMPS is running.

## What the probe is good for

- Checking a rail or a reference before wiring a target.
- Reading a potentiometer wiper or a slow sensor output.
- Comparing a signal against a threshold when 10 mV is enough.

It is not suitable for AC or audio, for current without a shunt, for
differential measurements, or for anything needing a calibrated accuracy
figure.

## Related

- [terminal.md](terminal.md) — `d` and `D`.
- [binary.md](binary.md) — the raw two-byte sample.
- [wiring.md](wiring.md) — the pin, ground and input limits.
- [../selftest/checks.md](../selftest/checks.md) — the `ADC` self-test check.
