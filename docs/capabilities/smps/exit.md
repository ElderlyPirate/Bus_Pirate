# SMPS - stopping and leaving

Part of the SMPS set (`docs/capabilities/smps/`).  This page owns the paths
that end the feedback loop, the state each path leaves behind, and the paths
that look like an exit but do not stop anything.  The commands are in
[binary.md](binary.md); the loop is in [feedback-loop.md](feedback-loop.md).

## Scope

Covered: `0xF1` in-band stop, leaving raw wire, returning to the terminal,
the terminal reset and a hardware reset.  Not covered: the shared entry
sequence and framing ([the binary layer](../../protocol/binary/README.md)).

## In-band stop: `0xF1` in `RAW1`

This is the normal stop and the only one that needs no mode change:

```
host -> 0xF1
host <- 0x01
```

The reply means the stop routine ran.  It is idempotent: repeating `0xF1`,
or sending it before any start, replies `0x01` and leaves the outputs in the
parked state described below.

What the stop changes:

| Element | After `0xF1` |
|---|---|
| ADC interrupt | off |
| ADC converter and auto-sampling | off |
| output-compare channel | disabled, sync source cleared |
| PWM duty and period | zero |
| `AUX1` pin | input, output routing cleared, latch low |
| `AUX1` ODC | unchanged |
| stored sample | unchanged |
| add-on output | no PWM drive; the add-on's own state depends on its design (`unverified`) |

The stored sample is deliberately kept: `0xF0` after `0xF1` repeats the last
sample from the run.  It is not a live measurement.

`0x01` proves only that the firmware accepted the stop and parked its own
hardware.  It does not prove that the add-on's output is off or that its
supply is removed.

## Leaving raw wire: `0x00`

`0x00` in `RAW1` leaves for `BBIO1`:

```
host -> 0x00
host <- "BBIO1"
```

This is **not** a stop.  The feedback loop keeps running in the background,
`AUX1` stays owned, and the refusals for the ADC, PWM and self-test commands
continue:

```
host -> 0x14             ADC one-shot while the loop still runs
host <- 0xFF 0xFF        impossible sample; not a reading
```

To stop after leaving raw wire, re-enter `RAW1` with `0x05` and send `0xF1`,
or return to the terminal with `0x0F` (below).  The same applies if you enter
another binary sub-protocol: the loop is not stopped by a mode change inside
the binary layer.

## Returning to the terminal: `0x0F`

`0x0F` is a `BBIO1` request.  It is not a raw-wire command:

```
host -> 0x00             leave RAW1 first
host <- "BBIO1"
host -> 0x0F
host <- 0x01             then the terminal reset runs
```

On v4 the return runs the **terminal reset** (the firmware's board reset).
That reset also calls the SMPS stop path on a build with SMPS support, and it
clears the `AUX1` open-drain bit.  So `0x0F` from `BBIO1` stops the loop,
releases `AUX1`, clears the open-drain bit, and then prints the version banner
and `HiZ>`.

`0x0F` sent while still in `RAW1` is not a return: it belongs to the raw-wire
generic group and is answered with `0x00`, leaving you in `RAW1`.  Leave with
`0x00` first.

## Terminal reset: `#`

At a terminal prompt, `#` prints `RESET`, runs the same terminal reset and
re-prints the version banner.  On v4 that reset clears the `AUX1` open-drain
bit; on an SMPS-enabled build it also runs the SMPS stop path, so it stops the
loop and parks `AUX1`.

A mode change (`m`) also runs the same board reset before the new mode's
setup.  On v4 that clears the `AUX1` open-drain bit; on an SMPS-enabled build
it also stops the loop by the same path.  This matters only if a loop was left
running by an unusual route: every normal route out of the binary layer
passes through `0x0F`, which stops it.

## Hardware reset and power cycle

A hardware reset or power cycle stops the processor, so the PWM and ADC stop
with it.  The reset clears the `AUX1` open-drain bit and reinitializes the
stored sample at startup, so the next `0xF0` before a start returns
`0x00 0x00`.

## The paths compared

| Path | Where sent | Stops PWM and ADC | Releases `AUX1` | `AUX1` ODC | Stored sample |
|---|---|---|---|---|---|
| `0xF1` | `RAW1` | yes | yes | unchanged | kept |
| `0x00` | `RAW1` | no | no | unchanged | kept |
| `0x0F` | `BBIO1` | yes on v4 (terminal reset) | yes | cleared on v4 | kept |
| `#` | terminal | yes on v4 (terminal reset) | yes | cleared on v4 | kept |
| `m` (mode change) | terminal | yes on v4 (board reset) | yes | cleared on v4 | kept |
| hardware reset or power cycle | Bus Pirate | yes | yes | cleared on v4 | cleared at startup |
| `0xF1` sent in `BBIO1` | `BBIO1` | **no** | no | unchanged | kept |

## `0xF1` in `BBIO1` is not a stop

After `0x00` leaves raw wire, `0xF1` is a **pin-state byte** in `BBIO1`, not
an SMPS command.  It switches the target supply and the pull-ups on and
changes the pin latches; it returns one pin-level byte (`0x00`-`0x1F`), not a
result byte.  The SMPS keeps running.

Recovery: send `0x05` to re-enter `RAW1`, then `0xF1`; or send `0x0F` for the
terminal reset.  Do not use `0xF1` as a stop byte outside `RAW1`.

## The terminal `d`/`D` refusal

The refusal listed in [feedback-loop.md](feedback-loop.md#ownership) is a
guard, not a normal state.  A loop left running in the binary layer cannot
reach the terminal without `0x0F`, which stops it.  If a terminal prompt does
show `ADC busy: SMPS feedback active; command rejected.` followed by
`Syntax error at char 1`, the loop was left running by an unexpected path;
send `#` to reset the Bus Pirate board state, then repeat the probe command.

## Related

- [binary.md](binary.md) - `0xF1`, `0x0F` and the exact reply bytes.
- [feedback-loop.md](feedback-loop.md) - the state the stop clears.
- [wiring.md](wiring.md) - the pins and the add-on supply.
- [the shared terminal-reset path](../../protocol/binary/entering-and-framing.md)
  - entry, leaving and the v4 reset.
