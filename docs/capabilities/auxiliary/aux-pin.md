# AUX pin — drive, read and target selection

Part of the AUX capability set.  This page owns the pins and the selection:
what the firmware calls them, how `c` / `C` / `k` / `K` assign the drive/read
controls, the `a` / `A` / `@` commands, HiZ availability, and what a reset
does.  The generators and the counter are in [pwm-servo.md](pwm-servo.md) and
[frequency.md](frequency.md); the binary wait is in
[edge-wait.md](edge-wait.md).

## Scope

Covers the extra digital I/O pins and the assignment of the drive/read
controls.  Not covered: the shared prompt grammar (`:N`, `m`, `W`, `P`, `v`,
`o`) — see [docs/generic/](../../generic/README.md); the pull-up and supply
rules — see [wiring.md](wiring.md); and the generators, which always use the
`AUX` pin and do not follow the assignment.

## The pins

The Bus Pirate's four bus lines are `MOSI`, `CLK`, `MISO` and `CS`.  The AUX
family adds `AUX`, a fifth digital I/O line on the same header, separate from
the bus.  It is the pin the firmware prints as `AUX` in the `v` table and the
pin used by the frequency counter and by the PWM and servo generator.

v4 breaks out two more lines, `AUX1` and `AUX2`, and lets all four be used as
the target of the same drive/read controls.  v3 has only `AUX` and `CS`.

## Targets and the two naming forms

The `?` help lists the assignment targets as `A0` / `CS` / `A1` / `A2` on
v4 (v3 prints `aux` / `CS`); the confirmation line, the `v` table and the
header use the pin names.  Both forms name the same pins.  This set uses the
pin names; the token-to-pin table is owned by the prompt grammar:
[commands.md](../../generic/commands.md#aux-targets-and-pin-names).

In short: `c` selects `A0`, the pin printed `AUX`; `C` selects `CS`; on v4
`k` selects `A1` (`AUX1`) and `K` selects `A2` (`AUX2`).

| Key | Confirmation line | Selected pin |
|---|---|---|
| `c` | `a/A/@ controls AUX pin` | `AUX` (`A0`) |
| `C` | `a/A/@ controls CS pin` | `CS` |
| `k` (v4) | `a/A/@ controls AUX1 pin` | `AUX1` (`A1`) |
| `K` (v4) | `a/A/@ controls AUX2 pin` | `AUX2` (`A2`) |

Selecting a target only changes which pin the next control acts on.  It does
not change that pin's direction or level, and it does not affect the other
pins.  On v3 there is no `k` / `K`; the two v4-only lines are absent.

### Checking the target

The confirmation line is printed when a selection key is pressed, and it is
repeated in the `i` status block in a bus mode.  At `HiZ>` there is no
non-destructive readout: `i` prints only the banner, and `v` shows pin
directions and levels, not the selection.  If the line has scrolled away,
either reset the board (`#` on v4; `#` is a hardware reset on v3), which
returns the target to `AUX` (`A0`), or press the key for the pin you intend to
use and read the line it prints.  Do not press a selection key merely to look:
it changes the target.

## Drive and read

| Input | Action | Reply |
|---|---|---|
| `a` | drive the selected pin low (output) | `AUX LOW` |
| `A` | drive the selected pin high (output) | `AUX HIGH` |
| `@` | make the selected pin an input, then read it | `AUX INPUT/HI-Z, READ: 0` or `... 1` |
| `@:N` | repeat the read N times, N = 1..32767 | one line per sample |

The `?` help labels the group `AUXPIN (low/HI/READ)`.  The replies print the
literal `AUX`; the `v` table lists the header pins by name but shows
directions and levels, not the selected target.  Only the confirmation line
names the selection.

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

The last session uses the v4-only `AUX1` target (`k`); v3 has no `k` / `K`.
The `:N` suffix is shared prompt grammar; it is documented in
[docs/generic/commands.md](../../generic/commands.md).

## The assignment rule

**The assignment follows the selected target: `a` / `A` / `@` act on the
selected pin.**  After `C`, `a` drives `CS`; after `k`, `@` reads `AUX1`.  The
binary wait-for-edge command follows the same selection
([edge-wait.md](edge-wait.md)).

**PWM, servo and frequency do not follow the selection.**  They always use
the physical `AUX` pin (the `A0` target).  Selecting `CS` or `AUX1` does not
move the generator or the counter.  The BBIO1 state and direction bytes also
keep their AUX bit on `AUX`; only the wait follows the selection
([binary.md](binary.md)).

## HiZ and other modes

`a` / `A` / `@` work in HiZ; `f` and the selection keys also work (below).
At the `HiZ>` prompt the pin can be driven and read with no bus mode and no
supplies.  The other AUX elements are not all available there:

| At `HiZ>` | Behaviour |
|---|---|
| `a` / `A` / `@` | work; the selected pin is driven or read |
| `g` / `S` | refused: `Command not used in this mode` |
| `f` | works; the pin is an input and the counter measures it |
| `c` / `C` (v4: `k` / `K`) | work; the selection changes |

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

The board reset returns the assignment to `A0` and releases every AUX target:
`AUX` and `CS` (and, on v4, `AUX1` / `AUX2`) become inputs and the generator
output routing is detached.  On v4 the same reset clears the `AUX1` open-drain
bit; an SMPS start clears it too.  On v4 the terminal reset is `#`; a mode
change (`mN`), the binary return and the SUMP exit also return the target to
`A0` and release the pins.  On v3 `#` is a hardware reset; the SUMP exit still
returns the target to `A0` and releases the pins.  A later `g` or `S` in a
mode therefore starts from an idle state, and `v` shows every header I/O pin as
an input.

`w` is not a board reset.  It does not change the target assignment and is
not a generator control; whether it stops a running waveform or releases a
manually driven pin depends on the active mode's cleanup, not on this
capability.

Entering the binary layer does not reset the assignment by itself: BBIO1
leaves the selection alone, so the terminal selection is still in force for
the `0x70` wait.  It does make `AUX` an input, which removes a running
waveform while leaving the generator flag set for the rest of the binary
session ([pwm-servo.md](pwm-servo.md#mode-and-timer-side-effects)).  The
binary I2C mode is different: its entry forces the target back to `AUX`, and
its
`0x09` selector can choose only `AUX` or `CS`
([binary.md](binary.md#i2c-extended-aux-0x09)).  The reset on the way back to
the terminal (`0x00` returns to `BBIO1`, then `0x0F` on v4; a hardware reset
on v3) restores `AUX` and releases the pins.

Entering SUMP makes the AUX pins inputs, so a running waveform stops.  The
SUMP exit prints the banner, runs the terminal reset, and leaves a `HiZ>`
prompt: pins to inputs, target supplies and pull-ups off, display
back to HEX, the target back to `AUX`, and the generator flag cleared.  A
later bare `g` in a mode therefore starts a new setup, not a stop.

**`@` versus a running generator.**  `@` on `AUX` makes the pin an input and
removes the waveform, but the firmware keeps the generator flag set.  `f`
still refuses and a bare `g` only stops.  Re-issue `g` or `S` with values to
start the output again.  Driving the pin with `a` / `A` does not clear the
flag either; stop the generator first if a static level is wanted.  The exact
pin outcome of re-driving an active generator is unverified.

## Limits

- One target at a time.  There is no AUX-target command that reads or drives
  two of the available pins in one operation.
- No pull-down and no toggle: `a` is low, `A` is high, `@` is read.
- No per-pin open-drain selection from these commands; the output type comes
  from the active mode.
- The self-test's `AUX` check always drives and reads the `AUX` pin; the
  assignment does not change which pin it tests
  ([self-test checks](../selftest/checks.md)).
- On v4 the optional SMPS build also drives `AUX1` with its supply PWM; that
  is a separate capability ([SMPS](../smps/README.md)).
- `AUX1` / `AUX2` are v4-only; v3 offers `AUX` and `CS`.

## Related

- [pwm-servo.md](pwm-servo.md) — the generator on the `AUX` pin.
- [frequency.md](frequency.md) — the counter on the `AUX` pin.
- [edge-wait.md](edge-wait.md) — the binary wait that follows the target.
- [binary.md](binary.md) — the binary AUX control and PWM/frequency commands.
- [wiring.md](wiring.md) — header pins and electrical limits.
- [docs/generic/commands.md](../../generic/commands.md) — the `a` / `A` / `@`
  and `c` / `C` / `k` / `K` entries, `:N`, and HiZ.
- [docs/protocol/dio/aux-pin.md](../../protocol/dio/aux-pin.md) — the same
  controls from the direct-I/O perspective.
- [docs/SPEC.md](../../SPEC.md) — the AUX capability rows.
