# AUX — binary commands

Part of the AUX capability set.  This page owns the AUX-specific binary
surface: what each command does to the AUX pins, its payload and its reply.
The shared binary layer — 20 × `0x00` entry, `BBIO1`, result-byte
conventions, sub-protocol entry and leaving — is owned by
[docs/protocol/binary/](../../protocol/binary/README.md).  The terminal
equivalents are in [aux-pin.md](aux-pin.md), [pwm-servo.md](pwm-servo.md),
[frequency.md](frequency.md) and [edge-wait.md](edge-wait.md).

## Scope

Covered: `0x12` (PWM setup), `0x13` (PWM clear), `0x16` (frequency count),
`0x70` (wait for an edge) and the I2C `0x09` extended-AUX command.  Not
covered: how to enter the binary layer, the shared state/direction bytes, the
self-test, the ADC or the SMPS command group; those have their own pages.

## Command map

| Byte | Where it works | Action | Payload | Reply |
|---|---|---|---|---|
| `0x09` | I2C binary only | extended AUX: select, drive or read `AUX` / `CS` | one sub-command | `0x01`, then the sub-result (always two bytes) |
| `0x12` | BBIO1 only | set up PWM on `AUX` | prescaler, duty, period | `0x01`; `0x00` if the SMPS owns the channel |
| `0x13` | BBIO1 only | clear PWM and detach `AUX` | none | `0x01`; `0x00` if the SMPS owns the channel |
| `0x16` | BBIO1 only | count frequency on `AUX` | none | four bytes, big-endian, in Hz |
| `0x70` | BBIO1 and the shared `0x7x` group | wait for an edge on the selected target | edge, timeout | result, then pin level |

Two byte-name traps matter:

- In **BBIO1**, `0x09` is the AVRISP-MKII serial entry, not an AUX command.
  The AUX/CS `0x09` exists only inside the I2C binary mode.
- Inside SPI, I2C, UART, 1-Wire and `RAW1`, the `0x1x` range is the bulk
  transfer group.  `0x12`, `0x13` and `0x16` are **not** PWM or frequency
  commands there; return to `BBIO1` first.  Of the AUX commands, only the
  `0x7x` wait is shared with those modes.

## PWM setup — `0x12`

Five bytes follow the command, in this order:

| Byte | Field |
|---|---|
| 1 | prescaler, low two bits: `0` = 1:1, `1` = 1:8, `2` = 1:64, `3` = 1:256 |
| 2–3 | duty, a raw 16-bit timer count, high byte first |
| 4–5 | period, a raw 16-bit timer count, high byte first |

The reply is `0x01` after the complete five-byte sequence.  The output
appears on the plain `AUX` pin, which is forced to an output; it does not
follow the selected target.  Duty and period are **counts, not percentages**:

```
timer tick   = 16 MHz / prescaler
output rate  = timer tick / (period count + 1)
high time    = duty count + 1 ticks on v4 (duty 0 is the 0 % case)
```

On v4 the high time is one timer tick more than the duty count, because the
falling comparison takes effect on the next clock.  The terminal path
compensates for this; the binary path does not.  On v3 the count is used
directly (unverified).  The command does not clamp the duty to the period;
keep the duty count at or below the period count.

Example: prescaler `2` (1:64, tick 250 kHz), duty `0x0063` (99) and period
`0x00C7` (199) give about 1250 Hz with a 50 % high time on v4:

```
host -> 0x12 0x02 0x00 0x63 0x00 0xC7
host <- 0x01
```

On an SMPS-enabled v4 build, while the SMPS feedback loop owns the output
channel, `0x12` consumes its five-byte frame and replies `0x00` without
touching the channel.  Stop the SMPS first.

## PWM clear — `0x13`

`0x13` takes no payload.  It stops the timer, disables the output channel and
detaches the `AUX` output, then replies `0x01`.  There is no binary servo
command; `0x13` also stops the waveform of a terminal PWM or servo, because
they share the same channel, but it is not a servo-position command.  On an
SMPS-enabled build it replies `0x00` while the SMPS owns the channel and
leaves the SMPS alone.

`0x0F` is not a substitute for `0x13`.  On v4 it returns to the terminal and
runs the board reset, which clears the generator flag, stops Timer2 and
detaches the output.  It does not disable the OC5 channel or clear its
period/duty setup; send `0x13` if the PWM module must be left fully idle.  On
v3 it is a hardware reset and stops everything.

## Frequency count — `0x16`

`0x16` takes no payload and returns four bytes, most significant first, in
Hz.  It forces `AUX` to an input, uses a one-second gate, and returns `0`
when no edges are counted.  The first gate runs the 1:256 prescaler; it
reruns at 1:1 only when that gate counted 255 or fewer edges.  A first gate of
256 or more is multiplied by 256 and returned directly; nominally that is
65,536 Hz and above, where the result is a multiple of 256 Hz.  At exactly
65,536 Hz the gate can hold 255 or 256 counts depending on phase, so the Hz
value is nominal.  Allow about one second when the prescaled gate is used,
about two seconds when the rerun runs.

The command **stops a running generator** rather than refusing: the
generator timers are shut down and `AUX` becomes an input.  The terminal `f`
refuses instead.  Neither command is blocked by the SMPS feedback loop, which
uses `AUX1` and the `ADC` pin.  Units and the terminal differences are in
[frequency.md](frequency.md).

## Wait for an edge — `0x70`

In BBIO1 the request is exactly `0x70`, followed by an edge selector and a
16-bit big-endian timeout in milliseconds.  The reply is a result byte (edge
seen or timeout) and the pin level.  The command follows the selected AUX
target, does not change its direction, and is shared by the nibble-group
modes.  The payload, reply, timeout and direction contracts are in
[edge-wait.md](edge-wait.md).

## I2C extended AUX (0x09)

`0x09` is a command of the **I2C binary mode**, not BBIO1.  The host sends one
sub-command byte.  Every reply is exactly **two bytes**: the first byte is
always `0x01` and reports that the command byte was recognised, not that the
sub-command succeeded; the sub-result follows.

| Sub-command | Action | Sub-result after the leading `0x01` |
|---|---|---|
| `0x00` | drive the selected pin low | `0x01` |
| `0x01` | drive the selected pin high | `0x01` |
| `0x02` | release the selected pin to high impedance | `0x01` |
| `0x03` | read the selected pin (it is made an input first) | the level, `0x00` or `0x01` |
| `0x10` | select `AUX` as the target | `0x01` |
| `0x20` | select `CS` as the target | `0x01` |
| other | none | `0x00` |

The drive and release sub-commands write the pads directly and print nothing,
so `0x09 0x00`, `0x09 0x01` and `0x09 0x02` each answer `0x01 0x01`.  An
unknown sub-command gives `0x01 0x00`.

```
host -> 0x09 0x00             extended AUX: drive the selected pin low
host <- 0x01 0x01             command accepted, sub-command done
```

There is no per-pin write in BBIO1: its state byte rewrites all five latches
plus the supply and pull-up bits, and its direction byte rewrites all five
directions.

Entering the I2C binary mode resets the target to `AUX`; `0x10` and `0x20`
select `AUX` or `CS` again.  They do not drive the pin.  Mode entry and the
command map are in
[protocol/i2c/binary.md](../../protocol/i2c/binary.md#aux-0x09).

In BBIO1 the same byte `0x09` is the AVRISP-MKII serial entry, or a refusal on
a build without AVRISP support; it is never an AUX command there.

## Limits

- `0x12`, `0x13` and `0x16` are BBIO1-only; inside a sub-protocol they mean
  bulk transfer.
- `0x12` accepts raw counts, not kHz or percent; it does not validate or clamp
  them.
- `0x13` is the explicit clear; `0x0F` is a terminal/hardware reset instead.
- `0x16` is `AUX`-only and returns a rate, not a duty cycle or pulse width.
- `0x70` is binary-only and reports one edge, not a count.
- `0x09` exists only in the I2C binary mode, and only for `AUX` / `CS`.
- On an SMPS-enabled build the SMPS owns the output channel; `0x12` / `0x13`
  are refused while it runs.

## Related

- [aux-pin.md](aux-pin.md) — the pins and the target selection.
- [pwm-servo.md](pwm-servo.md) — the terminal `g` / `S` generator.
- [frequency.md](frequency.md) — the terminal counter and its display.
- [edge-wait.md](edge-wait.md) — the `0x70` capability in full.
- [wiring.md](wiring.md) — pins, levels and loads.
- [docs/protocol/binary/commands.md](../../protocol/binary/commands.md) — the
  shared BBIO1 map and result bytes.
- [docs/protocol/i2c/binary.md](../../protocol/i2c/binary.md#aux-0x09) — the
  I2C `0x09` command.
- [docs/capabilities/smps/README.md](../smps/README.md) — the optional supply
  PWM that owns the channel.
- [docs/SPEC.md](../../SPEC.md) — the binary capability row.
