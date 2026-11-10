# 1-Wire — timing behaviour

Part of the [1-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  This page describes the observable timing
of the two speeds; it states behavioural values, not the firmware's internal
delay constants.

## Scope

The two speeds, their reset pulses, and their bit-slot lengths.  It does not
describe device-internal timing.

## The two speeds

| Speed | Menu | Nominal rate |
|---|---|---|
| standard | `1` | ~16.3 kbps |
| overdrive | `2` | ~160 kbps |

The menu labels are nominal.  The implemented slot timings are the table
below, about five to six times shorter in overdrive.  The speed is chosen at
mode entry and can be toggled in place with `O` (see
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
Nominal slot widths:

| Speed | Bit sent | Slot period | Data-low time |
|---|---|---|---|
| standard | `0` | ≈ 73–81 µs | ≈ 60–64 µs |
| standard | `1` | ≈ 70 µs | ≈ 6–7 µs |
| overdrive | `0` | ≈ 14 µs | ≈ 9 µs |
| overdrive | `1` | ≈ 12 µs | ≈ 1 µs |

Observable properties:

- A write-0 slot is mostly low; a write-1 slot is a short low pulse followed
  by a long high (release) period.
- Standard slots are about five to six times longer than overdrive slots.
- The `1` slot has a much shorter low time than the `0` slot at both speeds,
  which is what lets a device distinguish the two by measuring the low time.

## Overdrive

Overdrive is a property of the **device** as well as the host.  The Overdrive
Skip ROM command (`0x3C`) switches the parts to overdrive immediately; the
host must send `O` before the next reset to follow them.  If a reset runs at
standard speed, it returns overdrive-capable devices to standard.

`O` toggles only the Bus Pirate's timing.  To return the parts to standard,
toggle `O` to standard and issue a reset (`[` or `{`, at least 480 µs low); a
power cycle also does it.

## v3 / v4

v3 timing is **not verified**.
