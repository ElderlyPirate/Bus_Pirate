# 1-Wire — timing behaviour

Part of the 1-Wire documentation set (`docs/1wire/`).  This page describes the
observable timing of the two speeds; it states behavioural values, not the
firmware's internal delay constants.

## Scope

The two speeds, their reset pulses, and their bit-slot lengths.  It does not
describe device-internal timing.

## The two speeds

| Speed | Menu | Nominal rate (firmware wording) |
|---|---|---|
| standard | `1` | ~16.3 kbps |
| overdrive | `2` | ~160 kbps |

Overdrive is roughly ten times faster.  The speed is chosen at mode entry and
can be toggled in place with `O` (see
[entry-and-speed.md](entry-and-speed.md)).

## Reset pulse

The bus reset is a long low pulse; the length depends on the speed.  Nominal
low widths:

| Speed | Reset low width |
|---|---|
| standard | ≈ 490–501 µs |
| overdrive | ≈ 57–71 µs |

The standard pulse is comfortably longer than the 480 µs the spec requires;
the overdrive pulse sits inside the overdrive reset window.  The two are
clearly distinguishable, which is what `O` relies on to change the host
timing.

## Bit slots

Each bit is one time slot; the same slot shape is used to send and to receive.
Nominal slot widths (a `^:255` burst):

| Speed | Bit sent | Slot period | Data-low time |
|---|---|---|---|
| standard | `0` | ≈ 73–81 µs | ≈ 60–64 µs |
| standard | `1` | ≈ 70 µs | ≈ 6–7 µs |
| overdrive | `0` | ≈ 14 µs | ≈ 9 µs |
| overdrive | `1` | ≈ 12 µs | ≈ 1 µs |

Observable properties:

- A write-0 slot is mostly low; a write-1 slot is a short low pulse followed
  by a long high (release) period.
- Standard slots are about five to six times longer than overdrive slots,
  matching the ~10× nominal speed difference once the release/recovery margins
  are included.
- The `1` slot has a much shorter low time than the `0` slot at both speeds,
  which is what lets a device distinguish the two by measuring the low time.

## Overdrive persistence

Overdrive is a property of the **device** as well as the host.  A device put
into overdrive stays there until its own power is cycled; the host cannot talk
it back to standard over the bus.  `O` toggles only the Bus Pirate's timing,
so the host must follow the device.

## v3 / v4

v3 timing is **not verified**.