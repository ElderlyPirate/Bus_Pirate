# PWM and servo output

Part of the AUX capability documentation set.  This page owns the two
generators: `g` for PWM and `S` for a servo.  Both drive the `AUX` pin with
the same timer and output-compare channel, so only one of them can run at a
time.  The pin and its assignment are in [aux-pin.md](aux-pin.md); the wiring
and load rules are in [wiring.md](wiring.md).

## Scope

Covers the terminal generators, their parameters, the timer relation, the
messages they print, and how they stop.  The binary forms `0x12` / `0x13` are
summarised and linked, not repeated.  Shared prompt grammar (`:N`, `m`, `W`,
`P`) is owned by [docs/generic/](../../generic/README.md).

## The output pin

PWM and servo always drive the physical `AUX` pin — the `A0` target —
whatever `c` / `C` / `k` / `K` has selected.  The output is a 3.3 V logic
signal from the board's own rail; it is not a target supply and it is not the
`ADC` pin.  The generator does not need `W` (target supplies) or `P`
(pull-ups); it works as soon as a non-HiZ mode is entered.

Both commands are refused in HiZ and work in any other mode.  In a mode that
uses the `AUX` pin as one of its own signals (PIC and DIO are examples),
starting a generator overrides that signal.

## PWM — `g`

The one-line form takes the frequency in **kHz** and the duty cycle in
percent:

```
UART>g10 50
PWM active
UART>
```

That is 10 kHz at 50 %.  The frequency unit is easy to miss: `g10` is 10 kHz,
not 10 Hz, and a 4000 value is about 4 MHz.

The interactive form asks for the same two values and shows the accepted
ranges:

```
UART>g
1kHz-4,000kHz PWM
Frequency in kHz

(50)>10
Duty cycle in %

(50)>50
PWM active
UART>
```

The firmware prints a trailing space after `Frequency in kHz` and
`Duty cycle in %`; it is not shown here.

| Item | One-line form | Interactive prompt |
|---|---|---|
| frequency | 1–3999 kHz | 1–4000 kHz, default 50 |
| duty | 1–99 % | 0–99 %, default 50 |
| values | both on the command line | both required; Enter takes the default |

A value outside the range, a missing value, or a non-numeric value drops the
whole command into the interactive path and asks for **both** values again.
The duty check is strict in the one-line form: 0 and 100 are rejected, even
though the prompt's range starts at 0.

`PWM active` is the start/change confirmation.  Any `g` line while a generator
is running stops it first, before the arguments are validated: `PWM disabled`
prints first, then the new settings are applied or, if the values fail, the
interactive prompts ask for both values again.

### Timer and prescaler

The generator picks one of four timer prescalers from the requested
frequency.  The timer tick is the 16 MHz peripheral clock divided by the
prescaler; the period register is a whole number of ticks.

| Requested frequency | Prescaler | Timer tick | Period count |
|---|---|---|---|
| 1–3 kHz | 1:256 | 62.5 kHz | 19–61 |
| 4–30 kHz | 1:64 | 250 kHz | 7–61 |
| 31–244 kHz | 1:8 | 2 MHz | 7–63 |
| 245–4000 kHz | 1:1 | 16 MHz | 3–64 |

The output frequency is `timer tick / (period count + 1)`.  The divider is
rounded down, so the actual output is at or above the requested value.  The
error is small in most of the range but can be large where the divider lands
just below a whole tick count: 3201 kHz, for example, gives about 4 MHz.

The duty cycle is an integer percentage of the period count and is rounded
down.  It is therefore at or below the request.  At the top of the range the
period is very short and the steps are coarse: at 4 MHz, 50 % is one tick of
four, so the high time is 25 %.  Use a scope or a counter when an exact
frequency or duty matters.

### Changing and stopping PWM

| Action | Reply |
|---|---|
| bare `g` while a generator is running | `PWM disabled` (stops it) |
| bare `S` while a generator is running | `PWM disabled` (stops it) |
| `g` with valid new values while running | `PWM disabled`, then `PWM active` |
| `g` with missing or invalid values while running | `PWM disabled` first, then both prompts |
| `w` | does not stop the generator |
| `m` / `m1` / `#` | board reset detaches the output; the pin stops being driven |
| binary `0x0F` | v4 terminal reset detaches the output; v3 hardware reset |
| binary `0x13` | stops the timer and detaches the output |

A **bare** `g` is a stop only when a generator is already running.  With no
generator running it starts the interactive setup instead, so `g` is not a
safe "do nothing" command.

## Servo — `S`

The one-line form takes the position in degrees, 0–180:

```
UART>S90
Servo active
UART>
```

The interactive form asks for a position, defaulting to 90:

| Step | Firmware prints | You type |
|---|---|---|
| start, idle generator | `Position in degrees`, then `(90)>` | a position 0–180, or Enter for 90 |
| running | `Servo active`, then `(x)>` | a new position, or `x` / Enter to leave |

The `(x)>` step is a live retune loop: each number moves the servo and prints
`Servo active` again; `x` or an empty line leaves the loop with the last
position still running.  Only the `(x)>` prompt accepts `x`; the first
`(90)>` prompt has no exit key (Enter takes the default 90 and enters the
loop).  A position outside 0–180, or a non-numeric value, also takes the
interactive path.  The direct form `S90` does not enter the loop.

A bare `S` with no generator running, one retune, then exit:

```
UART>S
Position in degrees

(90)>45
Servo active

(x)>90
Servo active

(x)>x
UART>
```

The blank line before each `(…)>` prompt is the prompt's own leading line
break.

The frame is about 20 ms (1251 ticks of 16 µs).  The position maps to the
classic 1–2 ms pulse:

| Position | Compare ticks | Pulse width |
|---|---|---|
| 0° | 62 | 0.99 ms |
| 90° | 94 | 1.50 ms |
| 180° | 126 | 2.02 ms |

The mapping is linear in timer ticks and then rounded; the pulse widths are
approximate.  There is no endpoint calibration and no fractional degree.
`Servo active` is the start/change confirmation.

### Changing and stopping a servo

- Bare `S` while any generator is running: `PWM disabled` (the stop message
  says PWM even for a servo).
- Bare `S` with no generator running: starts the interactive setup.
- `S<position>` while running: retunes to that position and does not print
  the stop message.
- `m` / `m1` / `#` / the binary return: the board reset detaches the
  output, so the servo pulse stops.
- `w` does not stop the generator.

## Mode and timer side effects

PWM and servo share the same timer and output-compare channel.
Consequences:

- Only one generator can own the output.  Starting one replaces the other.
- The optional v4 SMPS build drives its supply PWM through the same
  output-compare channel (`OC5`) and routes it to the `AUX1` pin.  Do not run
  the terminal generator and the SMPS at the same time; each reprograms the
  channel.  See [SMPS](../smps/README.md).
- `@` while `AUX` is the selected target (the default) makes that pin an input
  and so removes a running waveform; entering the binary layer also makes
  `AUX` an input.  Neither clears the generator state: `f` still refuses and a
  later bare `g` only stops.  Re-issue `g` or `S` with values to start the
  output again.  This follows from the reset's release mechanism; the pin
  outcome is unverified on hardware.
- A mode change or a board reset detaches the output source and clears the
  generator state; the `AUX` pin returns to an input as part of the reset.  A
  later bare `g` therefore starts a new setup instead of acting as a stop.
- `w` (supplies off) does not touch the generator; the signal keeps running
  in a mode that does not use the `AUX` pin.  Stop it explicitly.
- On v4 `#` is a terminal reset and `m1`, a mode change, the binary return
  and the SUMP exit run the same firmware reset path; on v3 `#` is a
  hardware reset.  All of them detach the output.
- The binary `0x13` command is the explicit "stop and detach" for PWM started
  from the binary layer.

## Binary layer

The binary layer programs the same timer directly with raw counts.  `0x12` is
followed by a prescaler selector and 16-bit duty and period counts; it replies
`0x01`.  `0x13` stops the timer, disables the output compare and detaches the
`AUX` output; it also replies `0x01`.  There is no kHz or percent conversion
in the binary path — the host supplies counts.

The field order, the prescaler encoding, the worked exchange and the
`0x0F` caveat live in
[protocol/binary/commands.md](../../protocol/binary/commands.md).  Do not
send `0x0F` as a substitute for `0x13` if you want the PWM module idle: on v4
it detaches the pin but does not clear the timer setup, and on v3 it is a
hardware reset.

## Limits

- Frequency is approximate: whole timer ticks, at or above the request.
- Duty is approximate: whole ticks, at or below the request, with coarse
  steps at high frequency.
- Servo positions are integer degrees; the pulse endpoints are nominal.
- The generators are refused in HiZ; `f` and the drive/read controls are not.
- v3 is build-only here and unverified.

## Related

- [aux-pin.md](aux-pin.md) — the pin and the assignment.
- [frequency.md](frequency.md) — the counter that shares the pin.
- [wiring.md](wiring.md) — loads, drivers and the servo supply.
- [edge-cases.md](edge-cases.md) — prompt traps and failure recovery.
- [docs/generic/commands.md](../../generic/commands.md) — the `g` / `S`
  entries and HiZ.
- [docs/protocol/binary/commands.md](../../protocol/binary/commands.md) —
  `0x12` / `0x13` framing.
- [docs/SPEC.md](../../SPEC.md) — the PWM/servo capability row.
