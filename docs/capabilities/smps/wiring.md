# SMPS - wiring and electrical limits

Part of the SMPS set (`docs/capabilities/smps/`).  This page owns the wires,
the ground reference, the pull-up and supply requirements, the legacy add-on
connector, and the limits on what may be connected.  The loop is in
[feedback-loop.md](feedback-loop.md); the bytes are in [binary.md](binary.md).

## Scope

Covered: the Bus Pirate side of the connection, the legacy add-on's connector,
and the requirements the firmware imposes.  Not covered: the add-on's internal
schematic values, its output rating, or a v4 adapter cable.  Those are
`unverified` here.

## Add-on identification

The add-on named in this set is the legacy Dangerous Prototypes **Bus Pirate
SMPS** board.  The surviving schematic titles it `BusPirate-SMPS`; no revision
letter is marked.  It is a daughter board with a 2x5 header, a feedback-select
jumper, and a two-terminal screw output marked **`VPP`** and **`GND`**.

| Legacy add-on pin | Signal |
|---|---|
| 1 | VPPEN |
| 2 | MCLR |
| 3 | PGD |
| 4 | PGC |
| 5 | PWM input |
| 6 | VTARGET (power-stage input) |
| 7 | V3ADC (feedback, jumper 1-2) |
| 8 | 5V |
| 9 | V2G0ADC (feedback, jumper 2-3) |
| 10 | GND |

The feedback jumper `JP1` selects the ADC pin: `1-2` connects the feedback
divider to `V3ADC`, `2-3` connects it to `V2G0ADC`.  The schematic draws the
converter stage from `VTARGET`; it does not show a connection from the `5V`,
`PGC`, `PGD`, `MCLR` or `VPPEN` pins to that stage.

The schematic is not part of this documentation tree; obtain the board's
schematic from the Dangerous Prototypes hardware archive before wiring.  The
archived wiki page carries the same schematic and part list:
[Bus Pirate SMPS][smps-archive].
The schematic and its part list give no input or output voltage, current or
power rating.  Do not assume the Bus Pirate's `5.0V` or `3.3V` rail can power
the board; the v4 header has no `VTARGET` pin.  The add-on's supply, output
range and feedback scaling are `unverified`.

A board is a match only if it provides a PWM input, a feedback output, a
common ground and its own power feed.  The legacy board uses a 2x5 connector;
the v4 header is 2x6.  No adapter cable or direct plug-in is documented.  Do
not force the 2x5 connector onto the v4 header.  Wire the signal and power
wires individually, and confirm the board revision and continuity before
powering it.

## One line per wire

| Wire | Bus Pirate side | Legacy add-on side | Required |
|---|---|---|---|
| PWM control | `AUX1` | pin 5 (PWM) | yes |
| feedback | `ADC` | pin 7 (`V3ADC`, jumper 1-2) or pin 9 (`V2G0ADC`, jumper 2-3) | yes for closed-loop operation |
| ground | `GND` | pin 10 (GND) | yes |
| supply | none | pin 6 (`VTARGET`, power-stage input); pin 8 (`5V`) is also on the connector | the add-on's own feed; not switched by the firmware |

**No pull-up is required** on either the PWM line or the feedback line in the
normal push-pull case.  The PWM line is normally a push-pull output and the
feedback line is an analogue input; neither is a bus, and the `P`/`p` pull-up
network does not bias them.  The legacy board's PWM input is a resistor into
an NPN base, so it needs a driven high.  If a previous mode left `AUX1` open
drain, a logic high is a released pin and the add-on needs its own pull-up.

The v4 OpenOCD path uses `AUX1` as TRST, and its open-drain mode sets that pin
open drain.  Leaving that path does not necessarily restore push-pull, and the
SMPS start does not clear the open-drain bit.  A v4 terminal reset (`#` or
`0x0F`) does not clear it either; a power cycle or hardware reset does.  After
OpenOCD open-drain use, power-cycle the Bus Pirate before an SMPS start, or fit
a pull-up to a permitted logic rail (`unverified`).

**The firmware does not switch a Bus Pirate supply for SMPS.**  It never turns
on either on-board target rail for this path.  `W` and the binary `0x4x`
supply bit control those rails; they are not part of the SMPS command group.
While the loop runs, the terminal `W` is refused with the same `ADC busy` line
because `W` checks the rails through the ADC; `w` still turns them off and is
not blocked, and the binary `0x4x` supply bit is not blocked either.  The
add-on needs power from its own feed; its voltage, current and protection are
part of the add-on design (`unverified`).

## The pins

v4 header, counted from the key (key = pin 1) on the shrouded 2x6 I/O header:

| Signal | Pin label | Header pin | Direction at the Bus Pirate |
|---|---|---|---|
| PWM control | `AUX1` | 6 | output, 3.3 V logic (push-pull normally) |
| feedback | `ADC` | 8 | analogue input, 0-6.6 V nominal |
| ground | `GND` | 12 | - |

The header numbering convention and the `ADC` input scale are owned by
[the ADC wiring page](../adc/wiring.md); the AUX pin map is owned by
[the AUX wiring page](../auxiliary/wiring.md).  The `AUX1` token form is
`A1`, selected with `k`; the pin name used here is the header label.  No `k`
selection is needed for SMPS: the start routes the PWM to `AUX1` regardless of
the selected AUX target.

v3 has no `AUX1` target in its firmware and cannot enable SMPS, so this
wiring does not apply to a v3 board.

## Signal levels

- `AUX1` is a **3.3 V logic output**, not 5 V and not a power output.  It
  drives a control input only.  If the add-on expects a 5 V logic high, use a
  level shifter or a driver; do not assume 3.3 V is enough.  The output type
  is normally push-pull; the SMPS start does not clear an open-drain setting,
  so a mode that left the pin open drain also needs a pull-up
  (`unverified`).
- The pin current rating is a Bus Pirate limit not stated here.  Treat the
  line as a logic signal, not a load supply.
- `ADC` is the probe input behind the on-board 2:1 divider: 0-6.6 V nominal,
  about 6.45 mV per count, raw 1023 as the largest reading.  Above that range
  the reading clips and the input may be damaged; below ground it is not
  measurable.
- The two boards must share `GND`.  There is no isolation on the Bus Pirate
  side: the PWM and feedback reference is common ground.

## The feedback node

- The feedback node must stay within the `ADC` pin's input range.  How the
  add-on's output maps to the `ADC` pin, and therefore to counts, is part of
  the add-on design (`unverified`); the scaling may use the pin's own divider,
  a divider on the add-on, or both.
- The `ADC` pin is high impedance; a high-impedance feedback source may read
  low or wander.  Keep the source impedance low; the maximum usable value is
  not specified ([the ADC probe](../adc/probe.md)).
- While the loop runs, the `ADC` pin is the feedback input, not a free
  measurement point.  Do not connect an external source to it.
- A disconnected feedback wire reads an arbitrary level.  The loop has no
  presence test and no fault report.  The duty follows the floating reading:
  it may sit at the computed duty or be cut to zero, and the output is
  uncontrolled.  A zero sample is not proof of 0 V output.

## Add-on-side caveats

- The connector pinout above is from the legacy schematic, not from the
  firmware.  The firmware defines only the Bus Pirate pins.
- The add-on is a legacy Dangerous Prototypes design.  Whether a given add-on
  matches the schematic, and whether it matches the current firmware, is not
  established (`unverified`).
- The firmware calls the request a voltage but defines no unit.  The add-on's
  own scaling and output range are not defined here (`unverified`).
- The firmware provides no independent over-voltage, over-current or
  temperature protection.  `0xF1` removes the PWM drive; it does not
  disconnect the add-on's supply or prove its output is off.

## What not to connect

| Do not | Because |
|---|---|
| Apply more than 6.6 V to `ADC` | Clipped reading at best; the input may be damaged. |
| Apply a negative voltage to `ADC` | Not measurable below ground; may damage the pin. |
| Power the add-on from `AUX1` | It is a logic signal, not a supply. |
| Force the 2x5 add-on connector onto the v4 2x6 header | The pin rows do not align; no adapter is documented. |
| Drive `AUX1` externally while the loop runs | Two outputs on one net. |
| Connect a source to `ADC` while the loop runs | The pin is the loop's feedback node. |
| Assume the control path is isolated | The Bus Pirate shares ground with the host and the feedback reference. |
| Assume the add-on's output is isolated or safe to touch | Its voltage, current and isolation are not defined here (`unverified`); the Bus Pirate side is not isolated. |
| Use `0xF1` as a safety disconnect | It stops the firmware's drive only; the add-on's output state depends on its design. |

## v3 / v4

| | v3 | v4 |
|---|---|---|
| SMPS build | not available; enabling is a compile-time error | optional; off in the default image |
| `AUX1` target | absent from the v3 firmware | present, pin 6 |
| `ADC` feedback pin | present, but no SMPS path | present, pin 8 |
| add-on connector | the legacy board has a 2x5 header, but v3 cannot enable SMPS | the same 2x5 header needs a wired adapter (not documented) |
| pull-up required | - | none in the normal push-pull case |
| Bus Pirate switched supply | - | not used by the firmware |
| add-on power | - | its own feed on `VTARGET`; rating unverified |

## Related

- [feedback-loop.md](feedback-loop.md) - what the two wires do.
- [binary.md](binary.md) - the command group and the value encoding.
- [exit.md](exit.md) - stopping and resetting.
- [../adc/wiring.md](../adc/wiring.md) - the `ADC` input range.
- [../auxiliary/wiring.md](../auxiliary/wiring.md) - `AUX1` and the other
  header pins.
- [docs/generic/commands.md](../../generic/commands.md) - `P`/`p`, `W`/`w`
  and the idle state.

[smps-archive]: https://web.archive.org/web/20151019102049/http://dangerousprototypes.com/docs/Bus_Pirate_SMPS
