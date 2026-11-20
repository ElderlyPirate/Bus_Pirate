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
| `m`, then `2` | lists the mode menu, then runs the speed prompt | menu form also prints `Clutch disengaged!!!` |
| `m2` | direct entry, runs the speed prompt | no `Clutch` line |
| `m2 1` | direct entry, speed supplied on the same line | no prompt, no `Clutch` line |

The mode menu and the shared entry grammar are owned by
[docs/generic/](../../../generic/README.md); see
[navigation.md](../../../generic/navigation.md) §Mode menu.  After a menu-form
change the firmware prints the speed prompt first, then:

```
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
1WIRE>
```

The direct form prints only `Ready` and the prompt.  Until `W` is sent the bus
is parked (`Clutch disengaged`); the 1-Wire I/O line is not driven, so it sits
at the pull-up level if a pull-up is fitted.

The menu form, abridged (other modes elided):

```
HiZ>m
1. HiZ
...
2. 1WIRE
...
x. exit(without change)

(1)>2
Set speed:
 1. Standard (~16.3kbps)
 2. Overdrive (~160kps)
(1)>1
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
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

The complete direct form supplies the speed as the menu number; it prints no
prompt and no settings header:

```
HiZ>m2 1
Ready
1WIRE>
```

1-Wire has **no settings header** (unlike 2-Wire's `R2W (spd hiz)` header and
3-Wire's `R3W (spd csl hiz)` header); the chosen speed is visible only when it
is toggled or from the speed messages.

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
- A speed number outside `1`–`2`, or a partial one-line form such as `m2 3`,
  falls back to this prompt and the typed value is discarded.

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

Because `O` does not touch the bus, a part that has not received `0x3C` stays
at standard while the host runs overdrive, or the reverse; the mismatch
produces garbled reads and writes, not an error.  See
[edge-cases.md](edge-cases.md).
