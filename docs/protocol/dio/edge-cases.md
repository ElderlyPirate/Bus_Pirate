# DIO — edge cases and failure modes

Part of the DIO documentation set (`docs/protocol/dio/`).  Each entry gives
the trigger, the observable symptom, what the firmware does, and the recovery.
The normal behaviour is in [pin-control.md](pin-control.md),
[park-and-reset.md](park-and-reset.md) and [read-back.md](read-back.md).

## Symptom index

| Symptom | Command | Section |
|---|---|---|
| `WRITE: 0x01`; nothing is driven | `1` | [Values and writes](#values-and-writes) |
| `WRITE: 0x04`; `CLK` stays `I` | `0x04` | [Values and writes](#values-and-writes) |
| all five pins become outputs unexpectedly | `0x40`, `0x20` | [Values and writes](#values-and-writes) |
| pull-ups or other pins turn off | any state value | [Values and writes](#values-and-writes) |
| rails come up with no `POWER SUPPLIES ON` | `0xC0`, `0xE0` | [Values and writes](#values-and-writes) |
| an input pin does not take a written level | a state value on an input | [Values and writes](#values-and-writes) |
| an output comes up at an old level | a direction value | [Values and writes](#values-and-writes) |
| the echo shows a different value | a value above `0xFF` | [Values and writes](#values-and-writes) |
| the echo hides the action bit | `;N` below 8 | [A width below 8 hides the action bit](#a-width-below-8-hides-the-action-bit) |
| repeated writes alternate | `L` with `:N` | [LSB-first reverses the value](#lsb-first-reverses-the-value-before-the-pins-see-it) |
| pull-ups turn on with the supply | `0xE0` with pin bits | [Supply and pull-ups](#supply-and-pull-ups) |
| `WARNING: pins not open drain (HiZ)` on `P` | `P` | [Supply and pull-ups](#supply-and-pull-ups) |
| `Warning: no voltage on Vpullup pin` | `P` or state bit 5 | [Supply and pull-ups](#supply-and-pull-ups) |
| `e` refused | `e`, `e2`, `e3` | [Supply and pull-ups](#supply-and-pull-ups) |
| `VREG too low, is there a short?` | `W` | [Supply and pull-ups](#supply-and-pull-ups) |
| bit order, AUX target or display survive `w` | `L`, `C`, `o2` | [Parking and leaving](#parking-and-leaving) |
| `m10` releases all pins and drops the supply | `m10` in `DIO>` | [Parking and leaving](#parking-and-leaving) |
| `m` then `x` leaves the pins as they were | `m`, `x` | [Parking and leaving](#parking-and-leaving) |
| `W` does not restore the drives | `w` then `W` | [Parking and leaving](#parking-and-leaving) |
| `r` shows no direction or rails | `r` | [Reads and commands](#reads-and-commands) |
| an `r` bit changes between reads | floating input | [Reads and commands](#reads-and-commands) |
| `ERROR: command has no effect here` | `{`, wrappers, `(n)` | [Wrappers and mode macros](#wrappers-and-mode-macros-report-an-error) |
| an output fights an external driver | output against a driver | [An output fights an external driver](#an-output-fights-an-external-driver) |

## Values and writes

| Trigger | Symptom | Behaviour | Recovery |
|---|---|---|---|
| typing `1` to drive a pin high | `WRITE: 0x01`; `v` shows `CS` as `I`; nothing is driven | `0x01` is a direction value: bit 0 = 1 makes `CS` an input | use a state value: `0xC1` is supply on and `CS` high, `0x81` the same with the supply off; or select `CS` with `C` and send `A` |
| typing `0x04` to raise `CLK` | `WRITE: 0x04`; `v` shows `CLK` as `I`, not `H` | bit 7 clear selects a direction write; `0x04` sets bit 2, which makes `CLK` an input | send `0xC4` (supply on) or `0x84` (supply off) for `CLK` high |
| `0x40` to turn the supply on, or `0x20` to turn the pull-ups on | all five pins become outputs; the supply and pull-ups do not change | bit 7 is clear, so both are direction bytes; a direction write ignores bits 6–5 | `0xC0` turns the supply on with the pull-ups off; `0xE0` also turns the pull-ups on; `0xA0` turns the pull-ups on with the supply off |
| `0xC4` after the pull-ups were on, or after another pin was high | the pull-ups turn off and the other pins go low; the supply state is taken from bit 6 | a state value is absolute: it sets the supply, the pull-ups and all five latches from the one byte | include every bit you want; `0xC0` plus pin bits keeps the supply on with the pull-ups off, `0xE0` plus pin bits also keeps the pull-ups on |
| sending `0xC0` or `0xE0` without `W` | the rails come up, but there is no `POWER SUPPLIES ON` line and no rail check | state bit 6 drives the regulator directly | check the rails with `v`; use `W` when you want the check and the two engagement lines |
| a state value for a pin whose direction is still input | the pin does not change on `v`; it stays `I` and floating | the state bit is stored in the output latch, but the pin does not drive until its direction bit is 0 | send a direction value for that pin after the state value; state first, direction second avoids a glitch |
| making a pin an output without setting its level first | the pin can come up at an old level, not low | a direction change does not set a level; it enables the latch, which a previous mode or state write may have left high | send the state value first, then the direction value; after `w` the latches are low, so the pin comes up low |
| `0x100`, `0x180`, or any value above `0xFF` | the echo shows `0x00` or `0x80`; only the low byte took effect | the pin map sees the low byte only, and bit 7 of that byte selects direction or state | send a value in `0x00`–`0xFF` |

### LSB-first reverses the value before the pins see it

- Trigger: `L` followed by a value.
- Symptom: `0x01` is echoed as `WRITE: 0x01` but arrives as `0x80`, which is a
  state write instead of a direction write.
- Behaviour: `L` reverses the value after the echo and before the write; `r`
  reverses the printed read value within the width.
- Recovery: send `l` to return to MSB-first, or use `#` / a mode change.  Do
  not combine `L` with `:N`: the shared parser reverses the value again on
  each repeat, so the repeated writes alternate between the two forms.

### A width below 8 hides the action bit

- Trigger: `0x84;4`.
- Symptom: the echo is `WRITE: 0x04;4`, but the pin change is a state write.
- Behaviour: the echo is masked to the selected width; the full low byte is
  still applied.
- Recovery: set `;8` before using pin values.  A width above 8 can show bits
  the pins ignore, so keep the default 8 bits for direct I/O.

## Supply and pull-ups

| Trigger | Symptom | Behaviour | Recovery |
|---|---|---|---|
| using `0xE0` with pin bits only to keep the supply on | the pull-up resistors turn on as well | bit 5 is the pull-up bit; `0xE0` sets both bit 6 and bit 5 | use `0xC0` with the pin bits when the pull-ups must stay off, and feed `VPU` when they should be on |
| `P` in direct I/O | `WARNING: pins not open drain (HiZ)`, then `Pull-up resistors ON` | direct I/O is push-pull, so the shared command prints the informational warning; `(HiZ)` names the open-drain high state, not the mode | none; the resistors are on.  `p` turns them off |
| `P` with nothing on `VPU`, or state bit 5 set with `VPU` unpowered | `P` adds `Warning: no voltage on Vpullup pin`; a state value does not warn at all | `P` samples VPU once after enabling; the state bit only switches the resistors | feed `VPU` externally (both boards) before relying on the pull-ups; they are optional for direct I/O |
| `e`, `e2` or `e3` in direct I/O on v4 | `WARNING: pins not open drain (HiZ)` followed by `Syntax error at char 1`; nothing changes | `e` needs an open-drain mode, and direct I/O is push-pull; entering the mode also deselects an on-board source chosen earlier | feed `VPU` externally.  On v3 there is no `e` command at all |
| `W` with a shorted rail or a target holding a rail down | `VREG too low, is there a short?` then `POWER SUPPLIES OFF` | the regulator is switched off; the pins are unchanged | clear the fault or disconnect the target, then `W` again |

## Parking and leaving

| Trigger | Symptom | Behaviour | Recovery |
|---|---|---|---|
| `L`, `C` or `o2`, then `w` | the next writes are still LSB-first, still target `CS`, still print in decimal | `w` parks the five signal pins (v3) or seven pins (v4) and resets the width, but does not clear the other mode settings | use `#` for a full reset, or set each item back explicitly |
| `m10` while already in `DIO>` | the pins are released and the supply turns off; `Ready` is printed again | every mode change runs the cleanup and the board reset before the mode is re-entered | none needed; re-send the values you want |
| `m`, then `x` at `(1)>` | `no mode change`; the pins, supply and settings stay as they were | cancelling the menu skips the cleanup | `#` (or a mode change) when you also need the selector / bit-order / display reset; `w` releases the parked pins |
| `w` then `W` | the rails come up, but the pins stay inputs and the latches stay low (the pin levels float) | `W` only starts the supply; its setup hook is empty, so it does not restore the drives | send the state value, then the direction value |

### `w` releases the v4 extra pins

On v4, `w` sets both `AUX1` and `AUX2` to input and clears their latches,
regardless of which AUX target is selected.  Selecting one target does not
release a previously driven one; `w` (or `@` on that target) does.  See
[park-and-reset.md](park-and-reset.md).

## Reads and commands

| Trigger | Symptom | Behaviour | Recovery |
|---|---|---|---|
| expecting `r` to show a direction, the supply or the pull-ups | `READ: 0x04`; bits 7–5 are 0 with the default MSB-first order | `r` returns the five levels only | use `v` for directions and rails ([read-back.md](read-back.md)) |
| reading a pin that is an input with nothing connected | the bit changes between reads | the pin is floating; the value is the level on the header, not a stored state | drive the pin from the target or enable the pull-ups with a VPU reference |

### Wrappers and mode macros report an error

- Trigger: `{` then a value, expecting a `READ:` companion; or `[`, `]`, `}`,
  `/`, `\`, `-`, `_`, `.`, `^`, `!`, or `(n)`.
- Symptom: `ERROR: command has no effect here`, then
  `Syntax error at char <n>`; a bit op such as `/` prints its normal
  `CLOCK, 1` label first.  `{` adds no `READ:`, so the write still prints
  `WRITE:` only.  `(n)` adds a blank line before the syntax line.
- Behaviour: direct I/O has no start-with-read, stop, clock, data or macro
  callbacks; it clears any read-on-write setting when it is entered.
- Recovery: use `r` or `v` after the write; use values and user macros `<n>`.

## Electrical

### An output fights an external driver

- Trigger: a pin is an output while an external driver holds the same net at
  the opposite level.
- Symptom: excessive current, and possible damage to the target or the Bus
  Pirate pin.
- Behaviour: direct I/O does not detect contention; the output drives its
  level regardless.
- Recovery: set the pin to input before connecting a driver; never drive a
  pin against a supply.
