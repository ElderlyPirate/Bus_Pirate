# ADC probe and continuous sampling

**Status:** documented for v3 and v4; the v3 probe divider and input range
are not verified on v3 hardware.

The Bus Pirate has a dedicated analogue input on the pin it labels **`ADC`**.
An on-board divider scales the input into the 10-bit converter; with the
converter's 3.3 V reference at nominal, the nominal full-scale input is 6.6 V
and one count is about 6.45 mV.  The largest reading is 6.59 V (raw 1023).  The
terminal reads it with `d` (one sample) and `D` (a live voltmeter); the binary
layer exposes the same input as `0x14` (one sample) and `0x15` (a stream).

The probe is a separate pin from the digital `AUX` / `AUX1` / `AUX2` pins
([AUX set](../auxiliary/README.md)); those are digital I/O and generator
pins, not analogue inputs.

The firmware uses several labels for the same input.  The header pin and the
`v` table label it **`ADC`**; the `?` help lists the command pair `d/D` as
`Measure ADC (once/CONT.)`.  A one-shot print is headed `VOLTAGE PROBE: `,
and continuous mode is headed `VOLTMETER MODE`.  These are the same input and
the same converter; this set uses the pin name `ADC` and quotes the display
strings where they appear.  `VPU` is the `Vpullup` pin, a separate
housekeeping channel.

This README is a map, not the manual — the details live in the files below.
Read in this order: wire ([wiring.md](wiring.md)), take one reading
([terminal.md](terminal.md)), understand the scale ([probe.md](probe.md)),
then script it ([binary.md](binary.md)); failures are in
[edge-cases.md](edge-cases.md).

## Files

| File | Covers |
|---|---|
| [probe.md](probe.md) | converter, divider, raw to volts, channels, ownership, accuracy |
| [terminal.md](terminal.md) | `d` / `d:N` / `D`: output, stopping, SMPS refusal, sessions |
| [binary.md](binary.md) | `0x14` / `0x15`: bytes, stream/stop contract, SMPS refusal |
| [wiring.md](wiring.md) | pin, ground, input limits, what not to connect |
| [edge-cases.md](edge-cases.md) | over-range, stop/alignment quirks, SMPS ownership |

## The five-minute version

- Connect the signal to the `ADC` pin and the source ground to `GND`
  ([wiring.md](wiring.md)); stay within 0–6.6 V and remove any self-test
  jumper first.  No pull-up and no supply are needed.
- At any prompt (HiZ or a bus mode) type `d`: the reply is
  `VOLTAGE PROBE: 3.29V (raw 512)` (example; the display truncates, so
  3.30 V prints as `3.29V`).  Volts = raw / 1024 × 6.6; raw is 0..1023.
- `d:N` takes N readings, N = 1..32767, and prints N lines total with the
  header on the first; the repeat cannot be interrupted, so use a small N.
- `D` opens a live display headed `VOLTMETER MODE`; any key stops it.  The
  stopping byte is consumed and is not run as a command.
- Binary: in `BBIO1`, `0x14` returns two bytes, high byte first.  `0x15`
  streams two-byte samples until the host sends any byte; keep reading until
  the samples stop, then send the next command.
- On an SMPS-enabled v4 build, a running SMPS feedback loop owns the probe
  channel.  `d` / `D` are refused and `0x14` / `0x15` return `0xFF 0xFF`
  once; stop the SMPS first
  ([terminal recovery](terminal.md#smps-owns-the-adc),
  [binary bytes](binary.md#smps-owns-the-adc)).

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | always built (no feature switch) | always built (no feature switch) |
| terminal `d` / `D` | present, not verified on v3 | present |
| binary `0x14` / `0x15` | present, not verified on v3 | present |
| probe pin | the pin labelled `ADC` | `ADC`, pin 8 (key = pin 1) |
| divider | assumed 2:1, unverified | on-board 2:1 |
| nominal full scale | 6.6 V (firmware scale) | 6.6 V |
| largest reading | 6.59 V (raw 1023) | 6.59 V (raw 1023) |
| supply / pull-up needed | none | none |
| SMPS feedback can own the probe | — | optional build; refuses probe commands |

The full wiring table is in [wiring.md](wiring.md).

## Limits & out of scope

- 10-bit, single-ended, 0–6.6 V nominal.  The display resolves 0.01 V and
  truncates; the divider tolerance and the overall accuracy are not specified.
- Not an AC, current or differential instrument: it reads a DC level against
  the board's ground, with no isolation.
- Continuous sampling has no rate setting, no length and no timestamps; it is
  not a data logger.
- Only the probe channel has dedicated commands (`d` / `D`, binary `0x14` /
  `0x15`).  The switched 3.3 V output (the `3.3V` channel), the 5.0 V rail,
  VPU and (v4) Vusb channels are housekeeping, displayed by `v` and read by
  the self-test and the features that check them.
- Shared prompt grammar (`:N`, `o`, mode entry) is owned by
  [docs/generic/](../../generic/README.md); the binary framing is owned by
  [docs/protocol/binary/](../../protocol/binary/README.md).  This set does
  not repeat either.

## Related docs

| Page | Why |
|---|---|
| [docs/generic/commands.md](../../generic/commands.md) | the `d` / `D` entries and the shared `:N` grammar |
| [docs/generic/values.md](../../generic/values.md) | numeric input, `:N` and `o` |
| [docs/generic/usage.md](../../generic/usage.md) | the `v` table, including its `ADC` column |
| [docs/generic/edge-cases.md](../../generic/edge-cases.md) | the base-layer refusal list while the SMPS owns the ADC |
| [docs/protocol/binary/commands.md](../../protocol/binary/commands.md) | the BBIO1 map and the `0x14` / `0x15` rows |
| [docs/protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md) | entering `BBIO1`, result bytes, leaving |
| [docs/protocol/binary/sub-protocols.md](../../protocol/binary/sub-protocols.md) | where `0x1x` means bulk transfer, not ADC |
| [docs/capabilities/smps/README.md](../smps/README.md) | the SMPS feedback loop that can own the probe |
| [docs/capabilities/auxiliary/README.md](../auxiliary/README.md) | the digital `AUX` pin: PWM, servo, frequency, edge wait |
| [docs/capabilities/selftest/checks.md](../selftest/checks.md) | the `ADC` self-test check and its jumper |
| [docs/tools/basic/board-pins.md](../../tools/basic/board-pins.md) | the BASIC `ADC` token (raw counts) |
| [docs/SPEC.md](../../SPEC.md) | the capability rows |
