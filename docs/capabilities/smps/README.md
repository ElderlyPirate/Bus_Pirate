# SMPS (switched-mode power supply add-on)

**Status:** optional **v4-only** build; off in the default v4 image and not
available on v3.  The firmware side is documented here; the add-on is the
legacy Dangerous Prototypes **Bus Pirate SMPS** board, and its electrical
values are `unverified`.

The SMPS capability drives the legacy Bus Pirate SMPS add-on.  The add-on
provides the power path; the Bus Pirate supplies the PWM control and reads the
feedback.  The SMPS path does not supply power to the add-on and does not
switch either on-board target rail for it.  The PWM appears on the pin
labelled **`AUX1`** and the feedback on the pin labelled **`ADC`**; a firmware
loop adjusts the duty cycle so the feedback count tracks a target derived from
the requested value.  There is no terminal mode and no setup menu: the only
control path is the raw-wire binary command group `0b1111xxxx` (`0xF0`-`0xFF`).
This is a capability, not a tool.  It is written for a host programmer driving
the add-on and for an owner checking the wiring and limits.

The firmware uses two forms for the same pin: the header and `v`-table name is
**`AUX1`**, and the `?`-help assignment token is **`A1`** (selected with
`k`).  This set uses `AUX1`; the token-to-pin table is owned by
[the prompt grammar](../../generic/commands.md#aux-targets-and-pin-names).  No
`k` selection is needed for SMPS: the start routes the PWM to `AUX1`
regardless of the selected AUX target.  The feedback pin is labelled **`ADC`**;
the ADC set calls the same input the probe.  The firmware names the request
field as a voltage; this set calls it the requested value, because no unit is
defined.  This README is a map, not the manual - the details live in the files
below.

| File | Covers |
|---|---|
| [binary.md](binary.md) | entry, the `0xF0`-`0xFF` group, value encoding, replies, worked exchanges, disabled builds |
| [feedback-loop.md](feedback-loop.md) | what a start configures, duty and target arithmetic, the sample, firmware cutoff limits, ownership |
| [wiring.md](wiring.md) | pins, ground, pull-up and supply requirements, the legacy add-on connector |
| [exit.md](exit.md) | `0xF1` stop, leaving raw wire, the terminal reset, what each stop path changes |
| [edge-cases.md](edge-cases.md) | concrete failure modes and recovery |

Read in this order: wiring -> binary -> feedback-loop -> exit -> edge-cases.
Wire the Bus Pirate side before the first start.

## Wiring headline

The firmware drives **`AUX1`** (v4 header pin 6, key = pin 1) and samples
**`ADC`** (pin 8).  Connect the add-on's ground to **`GND`** (pin 12).
**No pull-up is required** on either line in the normal push-pull case.  The
SMPS start clears the `AUX1` open-drain bit, and a v4 terminal or board reset
(`#`, `0x0F`, or a mode change) also clears it.  If a binary path drives
`AUX1` before the start without a terminal reset, a leftover open-drain high
can still need an add-on pull-up ([wiring.md](wiring.md)).
**The firmware does not switch a Bus Pirate supply**
for SMPS: the add-on's power feed is part of the add-on wiring.

The add-on is the legacy **Bus Pirate SMPS** board.  It has a 2x5 connector
and a two-terminal **VPP**/**GND** output; a jumper selects which feedback pin
is used.  The connector map and the supply caveats are in
[wiring.md](wiring.md).  The electrical values, the output range and a v4
adapter are not documented (`unverified`); do not derive a power rating from
these pages.

## The five-minute version

- **v4 only, off in the default image.**  Enabling SMPS is a build change
  ([dev/building.md](../../dev/building.md)); a v3 build cannot enable it.
- Enter binary bitbang with 20 x `0x00` (`BBIO1`), then send `0x05` for the
  shared raw-wire mode (`RAW1`).  The NULs need a host program or script; a
  terminal cannot send them (the Python example is in
  [binary.md](binary.md#a-host-sequence)).  The SMPS group exists only inside
  `RAW1`.
- **Check the build:** once in `RAW1`, send `0xF1`.  `0x01` means SMPS is
  built in; `0x00` means it is not.  It does not start the supply; if a loop
  is already running, it stops it.
- **Start:** `0xF2`-`0xFF` plus one operand byte.  The command's low nibble is
  the top nibble of the requested value: `0xF2 0x00` requests `0x200`, the
  lowest reachable value.  Reply `0x01`.  The output is undefined; see the
  warning below.
- **Read:** `0xF0` returns the last stored feedback sample as two big-endian
  bytes, with no result byte.  It does not take a new sample and is not a
  calibrated output voltage.  A start clears the stored sample: before the
  first new conversion `0xF0` reads `0x00 0x00`.  There is no validity flag, so
  a real zero sample also reads zero.
- **Stop:** `0xF1` replies `0x01` and removes the PWM drive.  Send it before
  leaving `RAW1`: leaving raw wire does not stop the loop.  A v4 terminal
  reset (`0x0F` or `#`) does ([exit.md](exit.md)).
- No pull-up is needed in the normal push-pull case, and the firmware does not
  switch a Bus Pirate supply for SMPS.  The add-on needs its own power feed, a
  common ground, and its feedback node on `ADC` ([wiring.md](wiring.md)).

## Shortest byte sequence (not a safety claim)

The request has no defined unit, the add-on's output range and feedback
scaling are unknown, and the firmware has no independent over-voltage,
over-current or temperature protection.  Keep the load disconnected and
verify the add-on output with your own measurement before connecting anything.

| Step | Bytes | Reply |
|---|---|---|
| enter bitbang | `0x00` x 20 | `BBIO1` |
| raw wire | `0x05` | `RAW1` |
| check the build (optional) | `0xF1` | `0x01` built in; `0x00` not built |
| start, lowest request `0x200` | `0xF2` `0x00` | `0x01` |
| read before the first conversion (zero) | `0xF0` | two bytes, high byte first |
| read after a conversion | `0xF0` | two bytes, high byte first |
| stop | `0xF1` | `0x01` |
| leave raw wire | `0x00` | `BBIO1` |
| leave binary | `0x0F` | `0x01`, banner, `HiZ>` |

If the optional check replies `0x00`, the build does not have SMPS and the
rest of the table does not apply.  Without the check, the byte stream is
20 x `0x00`, then `0x05`, `0xF2` `0x00`, `0xF0`, `0xF0`, `0xF1`, `0x00`,
`0x0F`.  Shared entry, framing and the result bytes are owned by
[protocol/binary/](../../protocol/binary/README.md); `RAW1` framing is owned by
[protocol/wire/binary.md](../../protocol/wire/binary.md).

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | not available; enabling is a compile-time error | optional; **off** in the default image |
| group `0xF0`-`0xFF` | routed; every request answers `0x00` (not verified on v3) | routed; active only when built in |
| terminal UI | none | none |
| hardware | no `AUX1` target in the v3 firmware | legacy Bus Pirate SMPS board on `AUX1` and `ADC`; v4 adapter not documented |

The group is reachable only in `RAW1`; in `BBIO1` the same bytes are pin-state
bytes.  The full command detail and the build path are in [binary.md](binary.md)
and [wiring.md](wiring.md).

## Limits & out of scope

- No terminal interface and no setup menu; the only control path is the binary
  group.
- Raw numbers only: neither the request nor the sample is converted to volts.
- The firmware's cutoff can act only for requests `0x200`-`0x526`; whether the
  add-on can reach the target is unknown, and no request is a promise of a
  regulated output ([feedback-loop.md](feedback-loop.md)).
- A start clears the stored sample.  The first `0xF0` read is `0x00 0x00`
  until the first conversion; there is no validity flag, so a real zero sample
  also reads zero ([binary.md](binary.md)).
- `0xF1` removes the firmware's PWM drive; it does not prove the add-on's
  output is off, and the firmware has no independent over-voltage,
  over-current or temperature protection.
- The add-on's electrical design, output range and every voltage or current
  claim are `unverified`; the legacy schematic gives no rating.
- Shared prompt grammar, pull-ups, supplies and the `AUX` controls belong to
  [docs/generic/](../../generic/README.md) and the
  [AUX set](../auxiliary/README.md) and are not repeated here.

## Related docs

| Page | Why |
|---|---|
| [protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md) | entry, result bytes, leaving |
| [protocol/binary/sub-protocols.md](../../protocol/binary/sub-protocols.md) | where `0xFx` sits in `RAW1` |
| [protocol/wire/binary.md](../../protocol/wire/binary.md) | shared `RAW1` framing |
| [ADC probe](../adc/README.md) | sample scale and converter ownership |
| [AUX PWM and servo](../auxiliary/pwm-servo.md) | the same output channel |
| [dev/building.md](../../dev/building.md) | enabling the optional v4 build |
| [SPEC.md](../../SPEC.md) | capability index |
