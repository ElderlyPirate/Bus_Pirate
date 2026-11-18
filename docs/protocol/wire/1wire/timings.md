# 1-Wire — timing behaviour (AN126 classes)

Part of the [1-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  This page describes the observable timing
of the two speeds and the AN126 parameter classes the implementation uses; it
states behavioural values, not the firmware's private delay constants.

## Scope

The two speeds, their reset and presence sequence, and their bit slots.  It
does not describe device-internal timing.

## The two speeds

| Speed | Menu | Menu label | Nominal slot period |
|---|---|---|---|
| standard | `1` | `Standard (~16.3kbps)` | ≈ 70 µs |
| overdrive | `2` | `Overdrive (~160kps)` | ≈ 12 µs |

The menu labels are nominal and come from the firmware's own strings (the
overdrive label is spelt `~160kps`).  The implemented slots are about five to
six times shorter in overdrive.  A 70 µs slot is about 14.3 kbps and a 12 µs
slot about 83 kbps, so the label rates are names, not the reciprocal of the
slot period.  The speed is chosen at mode entry and can be toggled in place
with `O` (see [entry-and-speed.md](entry-and-speed.md)).

## AN126 parameter classes

The implementation follows the AN126 1-Wire timing classes.  The values below
are the implemented targets; the right-hand column gives the device rule the
value is chosen against.

| Class | Phase | Standard | Overdrive | Rule |
|---|---|---|---|---|
| A | initial low at the start of a slot | ≈ 6 µs | ≈ 1.4 µs | 5 µs minimum standard; 1.0–1.85 µs overdrive |
| C | additional low for a write-0 | ≈ 49 µs | ≈ 7 µs | total write-0 low at least 60 µs standard / 7 µs overdrive; loop overhead makes the actual low ≈ 62 µs / ≈ 8 µs |
| D | recovery after a write-0 | ≈ 8 µs | ≈ 3 µs | 10 µs recommended standard / 2.5 µs overdrive; 1 µs device minimum |
| E | release-to-sample delay in a read slot | ≈ 7 µs | ≈ 1 µs | E delay in the 5–12 µs standard / 0.5–0.85 µs overdrive window; the sample point is then ≈ 13 µs / ≈ 2.4 µs from the slot start |
| F | read-slot recovery | ≈ 50 µs | ≈ 7 µs | 55 µs recommended standard / 7 µs overdrive |
| G | overdrive guard before the reset pull-down | — | ≈ 3 µs | 2.5 µs minimum overdrive |
| H | reset low time | ≈ 500 µs | ≈ 70 µs | 480 µs minimum standard; 48–80 µs overdrive |
| I | release-to-presence-sample delay | ≈ 65 µs | ≈ 8 µs | 70 µs recommended standard; 7.2 µs minimum overdrive |
| J | reset recovery before the short check | ≈ 500 µs | ≈ 40 µs | 39.5 µs minimum overdrive |

The slot totals that follow from these phases:

| Speed | Bit sent | Slot period | Data-low time | Sample point |
|---|---|---|---|---|
| standard | `0` | ≈ 70 µs | ≈ 60 µs | ≈ 13 µs |
| standard | `1` | ≈ 70 µs | ≈ 6 µs | ≈ 13 µs |
| overdrive | `0` | ≈ 12 µs | ≈ 9 µs | ≈ 2.4 µs |
| overdrive | `1` | ≈ 12 µs | ≈ 1.4 µs | ≈ 2.4 µs |

Observable properties:

- A write-0 slot is mostly low; a write-1 slot is a short low pulse followed
  by a long high (release) period.
- The `1` slot has a much shorter low time than the `0` slot at both speeds,
  which is what lets a device distinguish them by measuring the low time.
- A read slot sends a `1` (releases the line), waits class E, samples the line,
  then waits class F.  The sample point is inside the device's valid window at
  both speeds.

## Reset and presence

The reset sequence, with the class letters:

1. Release the line, wait G (overdrive only), then pull it low.
2. Hold low for H.
3. Release the line and wait I.
4. Sample: high → `*No device detected`; low → presence detected.
5. Wait J and sample again: low → `*Short or no pull-up` (this result replaces
   the first one); high → the first result stands (`OK` after a presence,
   `*No device detected` after a high first sample).

The standard reset is comfortably longer than the 480 µs the spec requires;
the overdrive reset sits inside the overdrive reset window.  The two are
clearly distinguishable, which is what `O` relies on to change the host
timing.

## Overdrive

Overdrive is a property of the **device** as well as the host.  The Overdrive
Skip ROM command (`0x3C`) switches the parts to overdrive immediately; the
host must send `O` before the next reset to follow them.  If a reset runs at
standard speed, it returns overdrive-capable devices to standard.

`O` toggles only the Bus Pirate's timing.  To return the parts to standard,
toggle `O` to standard and issue a reset (`[` or `{`, at least 480 µs low); a
power cycle also does it.
