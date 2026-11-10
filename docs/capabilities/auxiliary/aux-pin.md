# AUX pin — drive, read and target selection

Part of the AUX capability documentation set.  This page owns the pin itself:
what the firmware calls it, how the target assignment works, the `a` / `A` /
`@` controls, and what a reset does to them.  The generators and the counter
are in [pwm-servo.md](pwm-servo.md) and [frequency.md](frequency.md); the
binary wait is in [edge-wait.md](edge-wait.md).

## Scope

Covers the extra digital I/O pin or pins and the assignment of the drive/read
controls.  Not covered: the shared prompt grammar (`:N`, `m`, `W`, `P`, `v`,
`o`) — see [docs/generic/](../../generic/README.md); the pull-up and supply
rules — see [wiring.md](wiring.md); and the generators, which use the `AUX`
pin only and do not follow the assignment.

## What the AUX pin is

The Bus Pirate's four bus lines are `MOSI`, `CLK`, `MISO` and `CS`.  The AUX
pin is a fifth digital I/O line on the same header, separate from the bus.  It
is the pin the firmware prints as `AUX` in the `v` table, and it is the pin
used by the frequency counter and by the PWM and servo generator.

v4 breaks out two more lines, `AUX1` and `AUX2`, and lets all four be used as
the target of the same drive/read controls.  v3 has only `AUX` and `CS`.

## Targets and the two naming forms

The `?` help lists the assignment targets as `A0` / `CS` / `A1` / `A2` on
v4 (v3 lists `aux` / `CS`); the confirmation line, the `v` table and the
header use the pin names.  The two forms name the same pins.  This set uses
the pin names.  The full token-to-pin mapping table is owned by the prompt
grammar:
[commands.md](../../generic/commands.md#aux-targets-and-pin-names).

On v3 the help prints the assignment as `aux/CS`, and there is no `k` / `K`.
The default target after a board reset is `A0` — the pin printed `AUX`.

The selection keys are `c` (`AUX`), `C` (`CS`), and on v4 `k` (`AUX1`) and
`K` (`AUX2`).  Selecting a target only changes which pin the next control acts
on.  It does not change that pin's direction or level, and it does not affect
the other pins.

## Drive and read

| Input | Action | Reply |
|---|---|---|
| `a` | drive the selected pin low (output) | `AUX LOW` |
| `A` | drive the selected pin high (output) | `AUX HIGH` |
| `@` | make the selected pin an input, then read it | `AUX INPUT/HI-Z, READ: 0` or `... 1` |
| `@:N` | repeat the read N times | one line per sample |

The `?` help labels the group `AUXPIN (low/HI/READ)`; the replies and the `v`
table print the pin as `AUX`.

Two details matter:

- **`@` releases the pin.**  The read first changes the selected pin to an
  input, so it is a release-and-read, not a readback of the driven level.  A
  pin driven high with `A` does not stay high after `@`; it becomes an input
  and then reads whatever the external circuit presents.  If `AUX` is the
  selected target, an `@` while a generator runs removes its waveform — see
  [pwm-servo.md](pwm-servo.md#mode-and-timer-side-effects).
- **The reply text always says `AUX`.**  The drive and read messages use the
  literal string `AUX` even when the target is `CS`, `AUX1` or `AUX2`.  The
  confirmation line from `c` / `C` / `k` / `K` is the record of the current
  target.

Sessions (the target drives the level in the read examples):

```
HiZ>a
AUX LOW
HiZ>A
AUX HIGH
HiZ>
```

```
HiZ>c
a/A/@ controls AUX pin
HiZ>@
AUX INPUT/HI-Z, READ: 1
HiZ>
```

```
HiZ>k
a/A/@ controls AUX1 pin
HiZ>@:2
AUX INPUT/HI-Z, READ: 0
AUX INPUT/HI-Z, READ: 0
HiZ>
```

The last capture uses the v4-only `AUX1` target (`k`); v3 has no `k` / `K`.
The `:N` suffix is shared prompt grammar; it is documented in
[docs/generic/commands.md](../../generic/commands.md).

## The assignment rule

**The assignment follows the selected target: `a` / `A` / `@` act on the
selected pin.**  After `C`, `a` drives `CS`; after `k`, `@` reads `AUX1`.  The
binary wait-for-edge command follows the same selection
([edge-wait.md](edge-wait.md)).

**PWM, servo and frequency do not follow the selection.**  They always use
the physical `AUX` pin (the `A0` target).  Selecting `CS` or `AUX1` does not
move the generator or the counter.  The binary state and direction bytes also
keep their AUX bit on `AUX`; only the wait follows the selection
([protocol/binary/commands.md](../../protocol/binary/commands.md)).

## HiZ and other modes

`a` / `A` / `@` are the only element group that works in HiZ; `f` and the
selection keys also work (below).  At the `HiZ>` prompt the pin can be driven
and read with no bus mode and no supplies; this is the safe state to leave the
board in.  The other AUX elements are not all available there:

| At `HiZ>` | Behaviour |
|---|---|
| `a` / `A` / `@` | work; the selected pin is driven or read |
| `g` / `S` | refused: `Command not used in this mode` |
| `f` | works; the pin is an input and the counter measures it |
| `c` / `C` / `k` / `K` | work; the selection changes |

The controls are not restricted to HiZ: they work in every mode.  That also
means they can override a pin the active mode is using.  For example, `C`
followed by `a` drives `CS` low in an SPI session, and `c` followed by `A`
drives `AUX` even in a mode that expects it as an input.  The mode is not
notified; its next operation may fail.

`a` / `A` set the pin's direction and latch level, but not its output type.
If the active mode configured that pin as open drain, `A` releases the pin
(high impedance) instead of driving 3.3 V.  This is the mode's setting, not a
property of `a` / `A`.

## Reset and persistence

The board reset returns the assignment to `A0`.  The firmware reset path runs
on `m1`, on any mode change, on the binary return `0x0F` and on the SUMP
exit; `#` runs it on v4 (terminal reset) and is a hardware reset on v3.  The
reset also returns the bus lines, `CS` and `AUX` to inputs, so a pin driven
there with `a` / `A` is released.

**`AUX1` and `AUX2` are not released by that reset.**  A pin driven with
`a` / `A` while `AUX1` or `AUX2` was selected stays an output at its last
level; the assignment has already gone back to `A0`, so `a` / `A` / `@` no
longer address it, and the `v` table still shows it as an output.  Select it
again (`k` / `K`) and read it (`@`) to make it an input before leaving.
Driving it low before the reset leaves a safer level, but it is still an
output.  This is the current firmware behaviour (v4 only).

Entering the binary layer does not reset the assignment by itself; the
terminal selection is still in force for the BBIO1 wait.  It does make `AUX`
an input, which removes a running waveform while leaving the generator state
active ([pwm-servo.md](pwm-servo.md#mode-and-timer-side-effects)).  The
assignment is reset when the terminal reset path runs on the way back.

## Limits

- One target at a time.  There is no command that reads or drives two of the
  available pins in one operation.
- No pull-down and no toggle: `a` is low, `A` is high, `@` is read.
- No per-pin open-drain selection from these commands; the output type comes
  from the active mode.
- The self-test's `AUX` check always drives and reads the `AUX` pin; the
  assignment does not change which pin it tests
  ([self-test checks](../selftest/checks.md)).
- On v4 the optional SMPS build also drives `AUX1` with its supply PWM; that
  is a separate capability ([SMPS](../smps/README.md)).
- `AUX1` / `AUX2` are v4-only; v3 offers `AUX` and `CS`.  The v3 behaviour is
  build-only here and unverified.

## Related

- [pwm-servo.md](pwm-servo.md) — the generator on the `AUX` pin.
- [frequency.md](frequency.md) — the counter on the `AUX` pin.
- [edge-wait.md](edge-wait.md) — the binary wait that follows the target.
- [wiring.md](wiring.md) — header pins and electrical limits.
- [docs/generic/commands.md](../../generic/commands.md) — the `a` / `A` / `@`
  and `c` / `C` / `k` / `K` entries, `:N`, and HiZ.
- [docs/protocol/binary/commands.md](../../protocol/binary/commands.md) — the
  state byte's AUX bit and the wait command.
- [docs/SPEC.md](../../SPEC.md) — the AUX capability rows.
