# 1-Wire — entry & speed

Part of the [1-Wire documentation set](README.md) in the
[wire protocol set](../README.md).

## Scope

How the `1WIRE` mode is entered and how its bus speed is chosen, including the
in-place `O` toggle.  Wiring is in [wiring.md](wiring.md); bus operations are in
[bus-operations.md](bus-operations.md).

## Entering 1-Wire

`1WIRE` is mode number **2** in the mode menu (HiZ is 1).  There are three
equivalent entries:

| Input | Behaviour | Notes |
|---|---|---|
| `m`, then `2` | lists the mode menu, then runs the setup prompt | menu form also prints `Clutch disengaged!!!` |
| `m2` | direct entry, runs the setup prompt | no `Clutch` line |
| `m 2 1` | direct entry, speed supplied on the same line | no `Clutch` line |

The mode menu and the shared entry grammar are owned by `docs/generic/` (see
[navigation.md](../../../generic/navigation.md) §Mode menu).  After a menu-form
change the firmware prints:

```
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
1WIRE>
```

The direct form prints only `Ready` and the prompt.  Until `W` is sent the bus
is parked (`Clutch disengaged`); the 1-Wire I/O line is not driven.

The direct form as an example:

```
HiZ>m 2 1
Ready
1WIRE>
```

The `m2` form runs the same speed prompt; it skips the
`Clutch disengaged!!!` line the `m`-menu form prints:

```
HiZ>m2
Set speed:
 1. Standard (~16.3kbps)
 2. Overdrive (~160kps)
(1)>1
Ready
1WIRE>
```

## The speed setup menu

Entering 1-Wire asks one question.  The exact firmware wording is:

| Prompt | Option | Text |
|---|---|---|
| `Set speed:` | `1` | `Standard (~16.3kbps)` |
| | `2` | `Overdrive (~160kps)` |

- The default is `1` — pressing Enter at `(1)>` selects standard speed.
- The option-2 string reads `~160kps` (a missing `b`); this is a firmware
  wording defect, not a different rate.  Transcribe it as shown.
- The answer is stored as the mode's speed setting and applies to resets and
  to every bit from then on.

## In-place speed toggle — `O`

`O` switches the active speed **without leaving the mode**, so the power
supplies and the pull-up resistors keep running and any device that has
already been put into overdrive stays powered.  It is dispatched by the base
layer but only accepted in 1-Wire mode.

| Input | Result |
|---|---|
| `O` in `1WIRE` | toggles the speed and prints the new one |
| `O` in any other mode (including HiZ) | prints `Command not used in this mode` |

The two printed strings are:

```
1-WIRE speed: overdrive
1-WIRE speed: standard
```

### How overdrive is entered and left

A common 1-Wire sequence sends the **Overdrive Skip ROM** command (`0x3C`) at
standard speed.  The parts switch to overdrive timing immediately; send `O`
before the next reset so the host follows them.  If the next reset runs at
standard speed, that reset returns the parts to standard.

`O` toggles only the Bus Pirate's timing.  To return the parts to standard,
toggle `O` to standard and issue a reset (`[` or `{`, at least 480 µs low);
that reset returns overdrive-capable devices to standard.  A power cycle also
does it.

### Example: toggling in place

The toggle on its own (the full sequence is in
[bus-operations.md](bus-operations.md)):

```
1WIRE>O
1-WIRE speed: overdrive
1WIRE>O
1-WIRE speed: standard
1WIRE>
```

## v3 / v4

| | v3 | v4 |
|---|---|---|
| mode number 2, speed menu, `O` toggle | present | present |
| `O` behaviour | **not verified** | present |

The `O` handler and the speed-menu strings are the same in both builds; v3
behaviour is not verified.
