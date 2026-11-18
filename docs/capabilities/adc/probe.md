# ADC probe — the input, the channels and the scale

Part of the ADC capability set (`docs/capabilities/adc/`).  This page explains
what the probe input measures, how a raw count becomes a voltage, which
channels share the converter, and how ownership works.  The terminal and
binary front ends are in [terminal.md](terminal.md) and [binary.md](binary.md);
the wiring is in [wiring.md](wiring.md).

## Scope

Covered: the on-board converter, the probe divider, the raw-to-volts rule,
resolution and accuracy, the channel map, ownership of the converter, and the
v3/v4 difference.  Not covered: the prompt grammar and the binary framing.

## The converter

- 10-bit converter, one conversion per reading.
- The result is an integer count, 0..1023 (1024 steps).
- The converter's reference is the board's 3.3 V rail — the converter's own
  supply, AVdd — and the firmware assumes 3.3 V.  The probe input is divided
  by two before the converter, which is why the nominal full scale is 6.6 V.
- The firmware does not average or filter: `d` and `0x14` are one conversion,
  and each continuous sample is one conversion.
- No calibration constant is applied; the count is scaled by a fixed ratio.
- The probe reading is ratiometric to the 3.3 V rail: if that rail is not at
  its nominal value, the probe and the other external channels shift with it.
  The 3.3 V channel is the exception: it is the same rail through a divider,
  so it is measured against itself (see [The channels](#the-channels)).

## The divider and the scale

An on-board 2:1 divider sits between the `ADC` pin and the converter.  The
converter therefore sees half the pin voltage, and the firmware doubles it
back:

```
volts = raw / 1024 × 6.6
```

The same raw-to-volts relation is used for every ADC display: the `d`
one-shot and the `v` table print the two-decimal form above, the self-test
prints the same value, the AVRISP VTARGET reading reports the probe in tenths
of a volt, and OpenOCD's `buspirate adc` reads the same four channels (probe,
VPU, 3.3 V and 5.0 V).  The binary layer returns the raw count and leaves the
conversion to the host.

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
| reference | the board's 3.3 V rail (converter supply), nominal |
| 3.3 V channel | ratiometric to that rail; divider/rail-present check only |
| overall accuracy | not specified (unverified) |

- The raw count is finer than the display: use the `(raw n)` value, or the
  binary sample, when a change smaller than 10 mV matters.
- Divider tolerance, offset and gain error are not characterised here; do not
  treat the reading as a calibrated instrument.
- The divider's resistor values, and therefore the input impedance, are not
  stated here (`unverified`).  Treat the input as a sense input, not a load.

## The channels

The converter is shared by dedicated inputs.  Only the probe channel has
dedicated commands.

| Channel | v3 | v4 | Reaches the user through |
|---|---|---|---|
| probe | yes | yes | `d` / `D`, binary `0x14` / `0x15`, BASIC `ADC`, AVRISP VTARGET, OpenOCD `buspirate adc` |
| 3.3 V rail | yes | yes | `v`, the `W` rail check, the self-test, OpenOCD `buspirate adc` (ratiometric; see below) |
| 5.0 V rail | yes | yes | `v`, the `W` rail check, the self-test, OpenOCD `buspirate adc` |
| VPU (the `Vpullup` pin) | yes | yes | `v`, the `P` warning, the self-test, OpenOCD `buspirate adc` |
| Vusb | — | yes | the self-test |

- These are dedicated converter inputs, not the digital I/O pins.  No `m`
  setting and no AUX assignment selects them; `a` / `A` / `@` / `k` drive the
  digital `AUX` pins, not the probe ([AUX set](../auxiliary/README.md)).
- The firmware applies the same 2:1 scale to every channel it displays.
- Binary `0x14` / `0x15` read the probe only.

The 3.3 V channel is the rail that supplies the converter, brought in through
its own divider.  It is therefore ratiometric too: the displayed figure is the
divider ratio expressed on the firmware scale, not an independent measurement
of the rail.  A rail that sags while the board still runs can still read about
3.3 V, because the reference moves with it.  The `v` column, the `W` rail
check and the self-test 3.3 V window are go/no-go checks on the divider and
on the channel being present; they cannot prove the rail is at 3.3 V or catch
a small droop.  The 5.0 V, Vusb and VPU channels are measured against the
3.3 V reference, so those readings do move with it.

## Who owns the converter

1. Every foreground prompt command and every BBIO1 request that measures a
   voltage takes the converter for its own measurement and turns it off
   afterwards.  There is one converter, so two foreground operations cannot
   overlap.
2. The optional SMPS feedback loop is the exception.  Starting the SMPS
   (`0xF2`–`0xFF` in raw wire) selects the probe channel, enables continuous
   sampling and installs a conversion handler.  That handler makes the probe
   the SMPS output-voltage feedback.
3. While the loop runs it **owns** the converter.  Commands that need a
   foreground measurement check ownership first and refuse rather than
   stealing the channel.
4. Ownership ends at the SMPS stop (`0xF1` in raw wire), the v4 terminal
   reset (`0x0F` in `BBIO1`: it replies `0x01`, returns to the interactive
   terminal, and resets the board state, which stops the loop), or a hardware
   reset.  Leaving raw wire with `0x00` does not stop the loop.

While ownership is held, the front ends behave like this:

| Command | Result while the SMPS feedback loop owns the converter |
|---|---|
| terminal `d`, `D`, `d:N` | `ADC busy: SMPS feedback active; command rejected.` then `Syntax error at char 1`; no sample |
| binary `0x14` | `0xFF 0xFF` once; no result byte |
| binary `0x15` | `0xFF 0xFF` once; no stream |
| terminal `P`, `e`, `g`, `S`, `v`, `~` | refused; see [generic edge cases](../../generic/edge-cases.md) |
| binary `0x12`, `0x13`, `0x5x`, `0x10`, `0x11` | refused; see [binary edge cases](../../protocol/binary/edge-cases.md) |
| BASIC `ADC` | `Error(3)`; see [BASIC edge cases](../../tools/basic/edge-cases.md) |

Two readers do not check ownership: the AVRISP VTARGET report and OpenOCD's
`buspirate adc` read the probe directly.  Do not leave an SMPS loop running
while using either.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| probe pin | the pin labelled `ADC` | `ADC`, pin 8 (key = pin 1) |
| divider | assumed 2:1, unverified | on-board 2:1 |
| nominal full scale | 6.6 V (firmware scale) | 6.6 V |
| Vusb channel | — | yes |
| SMPS feedback can own the probe | — | optional build |

The firmware scale is shared.  On v3 the divider and the safe input range are
assumptions: use the silkscreen to locate the pin and treat the numbers as
`unverified` until the board is checked.

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
- [../smps/README.md](../smps/README.md) — the SMPS loop that owns the probe.
- [../selftest/checks.md](../selftest/checks.md) — the `ADC` self-test check.
- [../auxiliary/README.md](../auxiliary/README.md) — the digital AUX pins.
