# ADC probe and continuous sampling

**Status:** draft — the board's analogue probe input, read one sample at a
time or streamed continuously.

The Bus Pirate has a dedicated analogue input on the pin it labels **`ADC`**.
An on-board 2:1 divider scales the input into the 10-bit converter, so full
scale is 6.6 V and one count is about 6.45 mV.  The terminal reads it with `d`
(one sample) and `D` (a live voltmeter); the binary layer exposes the same
input as `0x14` (one sample) and `0x15` (a stream).

The firmware names the input three ways.  The header pin, the `v` table and
the `?` help call it **`ADC`** (`Measure ADC (once/CONT.)`).  A one-shot print
is headed `VOLTAGE PROBE`, and continuous mode is headed `VOLTMETER MODE`.
These are the same input and the same converter; this set writes the pin as
`ADC` and quotes the other strings where they appear.

This README is a map, not the manual — the details live in the files below.

## Files

| File | Covers |
|---|---|
| [probe.md](probe.md) | the converter and the divider, raw to volts, resolution and accuracy limits, the other channels |
| [terminal.md](terminal.md) | `d` / `d:N` / `D`, output shape, the display-format setting, worked sessions |
| [binary.md](binary.md) | `0x14` / `0x15` semantics, the two-byte sample, the stop contract, a worked exchange |
| [wiring.md](wiring.md) | the probe pin and ground, input range, source limits, what not to connect, v3/v4 |
| [edge-cases.md](edge-cases.md) | unconnected inputs, over-range, stop-byte quirks, desynchronised hosts |

## The five-minute version

Wire the source first ([wiring.md](wiring.md)), then read with `d`/`D`
([terminal.md](terminal.md)), then script it ([binary.md](binary.md)).

- Connect the source to the `ADC` pin and its ground
  ([wiring.md](wiring.md)), then type `d`: the reply is, for example,
  `VOLTAGE PROBE: 3.29V (raw 512)`.
- `d:N` takes N readings (`N` = 1..32767); the header prints once, on the first
  line only.
- `D` opens a live display headed `VOLTMETER MODE`; the value is rewritten in
  place.  Any key stops it.
- Binary: enter the bitbang layer and send `0x14` for two bytes, high byte
  first.  `0x15` streams two-byte samples until the host sends any byte; keep
  reading until the samples stop, then send the next command.
- The probe input measures 0–6.6 V against the board's ground through a 2:1
  divider.  No pull-up and no supply are needed.  The v4 divider is
  documented here; the v3 divider and range are unverified (see
  [wiring.md](wiring.md)).
- Volts = raw / 1024 × 6.6.  Raw is 0..1023.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| terminal `d` / `D` | present (build-only, unverified) | present |
| binary `0x14` / `0x15` | present (build-only, unverified) | present |
| probe input | present (divider unverified) | present |
| header pin | the pin labelled `ADC` | pin 8 (key = pin 1), labelled `ADC` |
| pull-up / supply needed | none | none |

The full wiring table is in [wiring.md](wiring.md).  The v3 column is
build-only here and **unverified**.

## Limits & out of scope

- 10-bit, single-ended, 0–6.6 V.  The display resolves 0.01 V and truncates;
  the converter's accuracy and the divider tolerance are not specified.
- Not an AC, current or differential instrument: it reads a DC level against
  the board's ground.
- Continuous sampling has no rate setting, no length and no timestamps; it is
  not a data logger.
- The supply rails and VPU are sampled by the same converter for `v` and the
  self-test; the `P` warning reads VPU.  Those channels are not directly
  selectable.
- Shared prompt grammar (`:N`, `o`, mode entry) is owned by
  [docs/generic/](../../generic/README.md); the binary byte framing is owned by
  [docs/protocol/binary/](../../protocol/binary/README.md).  This set does not
  repeat either.

## Related docs

| Page | Why |
|---|---|
| [docs/generic/commands.md](../../generic/commands.md) | the `d` / `D` entries and the shared `:N` grammar |
| [docs/generic/usage.md](../../generic/usage.md) | the `v` table, including its `ADC` column |
| [docs/protocol/binary/commands.md](../../protocol/binary/commands.md) | the BBIO1 command map and the `0x14` / `0x15` rows |
| [docs/protocol/binary/sub-protocols.md](../../protocol/binary/sub-protocols.md) | where `0x1x` means bulk transfer, not ADC |
| [docs/capabilities/selftest/checks.md](../selftest/checks.md) | the `ADC` self-test check and its jumper |
| [docs/tools/basic/language.md](../../tools/basic/language.md) | the BASIC `ADC` token (raw counts) |
| [docs/SPEC.md](../../SPEC.md) | the capability rows |
