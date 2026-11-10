# AUX family — wiring and electrical limits

Part of the AUX capability documentation set.  This page owns the header
mapping, the logic levels, the load rules and the safety limits for the AUX
pin, its alternate targets, PWM/servo output, the frequency input and the
edge wait.

## Scope

Covers the wires and the electrical contract.  It does not repeat the shared
pull-up/supply commands: `P` / `p`, `W` / `w` and (v4) `e` are owned by
[docs/generic/commands.md](../../generic/commands.md).  The pin behaviour is
in [aux-pin.md](aux-pin.md); the generators are in
[pwm-servo.md](pwm-servo.md).

## One line per wire

| Function | Signal wire | Return | Extra |
|---|---|---|---|
| drive / read | the target's logic node → the selected pin | `GND` | none |
| PWM | `AUX` → the load or driver input | `GND` | a driver for non-logic loads |
| servo | `AUX` → the servo signal input | `GND` | the servo's own supply and a common ground |
| frequency | the signal source → `AUX` | `GND` | a defined logic level at the pin |
| edge wait | the signal source → the selected pin | `GND` | bias if the source floats |

**No Bus Pirate supply and no Bus Pirate pull-up are required for the logic
itself.**  PWM, servo and frequency are generated or measured from the
board's own 3.3 V rail.  A servo needs its own power supply; a source that can
float needs an external bias.  See the sections below.

## The pins

v4 header, counted from the key (key = pin 1) on the shrouded 2×6 I/O
header, using the same convention as the [ADC wiring page](../adc/wiring.md):

| Pin name | Header pin | Firmware token | Used by |
|---|---|---|---|
| `AUX` | pin 5 | `A0` | drive/read, PWM, servo, frequency, edge wait |
| `AUX1` | pin 6 | `A1` | drive/read, edge wait (v4); optional SMPS supply PWM (v4) |
| `AUX2` | pin 7 | `A2` | drive/read, edge wait (v4 only) |
| `CS` | pin 4 | `CS` | drive/read, edge wait; also the bus chip select |

The optional v4 SMPS build routes its supply PWM to the pin labelled `AUX1`
through the same output-compare channel the terminal generator uses; see
[SMPS](../smps/README.md).

On v3 the header has the pin labelled `AUX` and the pin labelled `CS`; there
is no `AUX1` or `AUX2`.  Use the silkscreen: the v3 pin numbering is not
documented here.  The v3 build is **unverified**.

The pin names are the header and `v`-table names.  The `?` tokens (`A0`,
`CS`, `A1`, `A2`) are the assignment names for the same pins
([aux-pin.md](aux-pin.md)).

## Logic levels

- The AUX pins are **3.3 V logic I/O**, not 5 V outputs and not open-drain by
  default.  A high is the board's 3.3 V rail; a low is ground.
- As an input the pin is high impedance.  `f` makes the pin an input; `@`
  also does, as part of the read.  The edge wait does not change direction,
  so configure the pin as an input before using it.
- The output type is whatever the active mode configured for that pin.  In a
  mode that set it open drain, a high is a released pin, not 3.3 V.
- A 5 V push-pull source is not documented as safe to connect directly; the
  board's input tolerance is **unverified**.  Use a level shifter when the
  source is 5 V.

## PWM loads

The PWM output is a **signal**, not a power output.  Do not power a lamp,
motor, relay or speaker from the pin.

- Logic input (another MCU, a gate, a counter): connect directly, with a
  common ground.
- LED: a series resistor to ground, sized for the pin's current limit.  The
  pin current rating is a board limit not stated here; treat it as a logic
  load.
- Motor, relay or solenoid: a transistor or MOSFET driver, with the load's
  own supply and a flyback diode for an inductive load.
- The PWM high level is the pin's high level: 3.3 V push-pull unless the
  active mode made the pin open drain.  A 5 V load that needs a 5 V logic
  high needs a driver or level shifter.

## Servo

- Signal: `AUX` to the servo's signal input.  Ground: the servo supply ground
  and the Bus Pirate ground must be common.
- Power the servo from a **separate supply** sized for its stall current.  Do
  not power a servo from the Bus Pirate's regulated target supply.
- A servo specified for 5 V logic may not accept a 3.3 V high as a valid
  logic high.  Check its datasheet; use a level shifter if needed.
- The generator produces about a 1–2 ms pulse in a 20 ms frame
  ([pwm-servo.md](pwm-servo.md)).  A servo that expects a different pulse
  range may not reach its full travel.

## Frequency and edge-wait inputs

- Drive the pin with a clean logic signal between ground and 3.3 V.  A slowly
  changing or noisy edge can be counted more than once.
- An open-collector or open-drain source needs a pull-up to a permitted logic
  rail.  A bare switch needs a pull-up or pull-down and will bounce.
- The frequency input measures rate, not amplitude.  A signal that is too
  small or centred outside the logic range is not a valid input.
- The same rules apply to the edge wait: define the idle level, then let the
  source drive the edge.

## Pull-up and supply ownership

- `P` / `p` control the shared bus pull-up network (see
  [docs/generic/commands.md](../../generic/commands.md)).  They are not a
  per-pin bias control for the AUX pins; do not rely on `P` to define an AUX
  input level.
- `W` / `w` control the target supply on the `VCC` pin.  They do not power
  the AUX output; the generator runs from the board's own rail.
- On v4, `e` selects the source feeding the pull-up network.  It is not an
  AUX output-level control.
- Fit an external pull-up or pull-down (1–10 kΩ is typical) when the AUX
  source can float, referenced to a permitted logic rail.

## What not to drive

- A voltage source into a pin that is configured as an output.
- A load directly from the pin; use a driver.
- More than 3.3 V logic without checking the board's input tolerance (which
  is unverified here).
- The `CS` pin while the active bus mode is using it as its chip select.
- The `ADC` pin as if it were a digital I/O; it is the analogue probe input.
- Mains, or any circuit not isolated from the mains.  The Bus Pirate shares
  ground with the host and the target; there is no isolation.
- Two outputs on the same net, including a manual `a` / `A` on a pin the
  active mode is also driving.
- `AUX1` while the optional SMPS build is running: the supply PWM owns the
  pin and the output-compare channel ([SMPS](../smps/README.md)).

Before leaving, release a manually driven `AUX1` / `AUX2` with `@`.  A board
reset returns the assignment to `A0` and releases `AUX`, `CS` and the bus
lines, but it does not release a pin left driven on `AUX1` / `AUX2`
([aux-pin.md](aux-pin.md)).  Do not leave a driven `AUX1` / `AUX2` wired to a
target input when you power down: the reset will not release it, so the target
keeps seeing the driven level.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| AUX targets | `AUX`, `CS` | `AUX`, `AUX1`, `AUX2`, `CS` |
| PWM / servo / frequency pin | `AUX` | `AUX` |
| logic level | 3.3 V (build-only, unverified) | 3.3 V |
| on-board pull-up source `e` | — | present |
| header numbering in this set | by silkscreen only | key = pin 1 |

The v3 column is code-derived and **unverified**.

## Related

- [aux-pin.md](aux-pin.md) — the targets and the drive/read controls.
- [pwm-servo.md](pwm-servo.md) — the generator.
- [frequency.md](frequency.md) — the counter.
- [edge-wait.md](edge-wait.md) — the binary wait.
- [docs/generic/commands.md](../../generic/commands.md) — `P` / `p`,
  `W` / `w`, `e`.
- [docs/capabilities/adc/wiring.md](../adc/wiring.md) — the analogue input,
  a different pin.
- [docs/SPEC.md](../../SPEC.md) — the capability rows.
