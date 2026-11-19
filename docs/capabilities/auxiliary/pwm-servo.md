# PWM and servo output

Part of the AUX capability set.  This page owns the two terminal generators:
`g` for PWM and `S` for a servo.  Both drive the `AUX` pin with the same timer
and output channel, so only one of them can run at a time.  The pin and its
assignment are in [aux-pin.md](aux-pin.md); the wiring and load rules are in
[wiring.md](wiring.md); the binary forms are in [binary.md](binary.md).

## Scope

Covers the terminal generators, their parameters, the timer relation, the
messages they print, and how they stop.  Shared prompt grammar (`:N`, `m`,
`W`, `P`) is owned by [docs/generic/](../../generic/README.md).  The binary
`0x12` / `0x13` commands are summarised here and owned by
[binary.md](binary.md).

## The output pin

PWM and servo always drive the physical `AUX` pin — the `A0` target —
whatever `c` / `C` / `k` / `K` has selected.  The output is a 3.3 V logic
signal from the board's own rail, unless the active mode made the pin open
drain; it is not a target supply and it is not the `ADC` pin.  The generator
does not need `W` (target supplies) or `P` (pull-ups); it works as soon as a
non-HiZ mode is entered.

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
not 10 Hz, and a 4000 value is 4 MHz.  The command is lowercase `g`; there is
no uppercase `G` command.

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
`Duty cycle in %`; it is not shown here.  Unlike the servo prompts below,
these two do not print a blank line before `(…)>`.

| Item | One-line form | Interactive prompt |
|---|---|---|
| frequency | 1–4000 kHz | 1–4000 kHz, default 50 |
| duty | 0–99 % | 0–99 %, default 50 |
| values | both required | both required; Enter takes the default |
| out of range | falls into the interactive path | an answer outside 1–4000 / 0–99 prints `Invalid choice, try again` and re-prompts |

A value outside the range, a missing value, or a non-numeric value drops the
whole command into the interactive path and asks for **both** values again.
The one-line form accepts 0 % duty and rejects 100 %; the prompt's range also
stops at 99 %.  If you need 100 % duty, use the binary PWM command or BASIC
`DUTY`.  The binary command is in [binary.md](binary.md); the BASIC form is
in [BASIC board pins](../../tools/basic/board-pins.md).

`PWM active` is the start/change confirmation.  Any `g` line other than the
bare letter, while a generator is running, stops it first, before the
arguments are validated: `PWM disabled` prints first, then the new settings
are applied or, if the values fail, the interactive prompts ask for both
values again.

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
computed with integer division, so the actual output is at or above the
requested value.  The error is small in most of the range but can be large
where the divider lands just below a whole tick count: 3201 kHz, for example,
gives a period count of 3, which is four timer ticks at 16 MHz — about 4 MHz.

The duty is a whole number of high ticks, rounded down.  On v4 the 0 % code
is special and a one-tick high time is not representable, so a request that
would need one high tick rounds down to 0 %.  At 4 MHz the period is four
ticks; the achievable results are coarse:

| Requested duty at 4 MHz | High ticks | Actual high time |
|---|---|---|
| 0–49 % | 0 | 0 % |
| 50–74 % | 2 | 50 % |
| 75–99 % | 3 | 75 % |

At lower frequencies the steps are finer, but the duty is still at or below
the request.  Use a scope or a counter when an exact frequency or duty
matters.  On v3 the duty encoding differs and is not verified on v3 hardware.

### Changing and stopping PWM

| Action | Reply |
|---|---|
| bare `g` while a generator is running | `PWM disabled` (stops it) |
| bare `S` while a generator is running | `PWM disabled` (stops it) |
| `g` with valid new values while running | `PWM disabled`, then `PWM active` |
| `g` with missing or invalid values while running | `PWM disabled` first, then the range line and both prompts |
| `S<position>` while running | `Servo active`; the output is reprogrammed, with no `PWM disabled` line |
| `S` with an invalid position while running | the current output stops without `PWM disabled`, then `Position in degrees` and `(90)>` |
| `w` | not a generator control; the active mode's cleanup may park `AUX` |
| a mode change (`mN`) | board reset clears the generator flag and detaches the output; no `PWM disabled` line; Timer2 is not stopped |
| `#` | v4 terminal reset: clears the flag and detaches the output, Timer2 is not stopped; v3 hardware reset |
| binary `0x13` | stops the timer and detaches the `AUX` output |
| binary `0x16` | stops the generator timers and measures `AUX` |
| binary `0x0F` | v4 terminal reset: clears the flag and detaches the output, Timer2 is not stopped; v3 hardware reset |
| SUMP exit | terminal reset on both boards: clears the flag and detaches the output; no `PWM disabled` line |

A **bare** `g` is a stop only when a generator is already running.  With no
generator running it starts the interactive setup instead, so `g` is not a
safe "do nothing" command.  The letter must be the end of the line: a trailing
space is read as an argument, so `g ` and `S ` take the setup path, stopping
a running output first (`g ` prints `PWM disabled`; `S ` stops silently).

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
break.  The PWM prompts in the previous section do not have it.

The frame is about 20 ms (1251 ticks of 16 µs).  The position maps to the
classic 1–2 ms pulse:

| Position | Compare ticks | Nominal width | v4 high time |
|---|---|---|---|
| 0° | 62 | 0.99 ms | 63 ticks, 1.01 ms |
| 90° | 94 | 1.50 ms | 95 ticks, 1.52 ms |
| 180° | 126 | 2.02 ms | 127 ticks, 2.03 ms |

The mapping is linear in timer ticks and then rounded; a nominal width is the
compare value at 16 µs per tick.  The compare value is what the firmware
writes.  On v4 the output high time is one timer tick longer, because the
falling comparison takes effect on the next clock, so the `v4 high time`
column is the actual pulse.  On v3 the compare value is used directly
(unverified).  There is no endpoint calibration and no fractional degree.
`Servo active` is the start/change confirmation.

### Changing and stopping a servo

- Bare `S` while any generator is running: `PWM disabled` (the stop message
  says PWM even for a servo).
- Bare `S` with no generator running: starts the interactive setup.
- `S<position>` while running: retunes to that position and does not print
  the stop message.
- An invalid `S<position>` while running stops the current output silently,
  then prompts for a new position.
- A mode change (`mN`), `#`, the binary return or the SUMP exit: the reset
  detaches the output, so the servo pulse stops.
- `w` does not stop the generator.

## Mode and timer side effects

PWM and servo share the same timer and output channel.  Three separate pieces
of state change independently:

| Layer | What it controls | Cleared or stopped by |
|---|---|---|
| generator flag | `f` refuses while it is set, and a bare `g` / `S` stops instead of setting up | a bare `g` / `S`; a new `g` / `S` with values replaces the setup; a board reset; an SMPS start |
| output path | whether the timer drives the `AUX` pin | `@` on `AUX`; entering BBIO1 or SUMP, which makes the pin an input; `a` / `A` on `AUX`, which make it an output again; binary `0x13`; a board reset, which detaches the timer source |
| timer registers | Timer2 and the OC5 compare | a new `g` / `S`; binary `0x12`, `0x13` or `0x16`; a v3 hardware reset |

A v4 reset (`#`, a mode change, the binary return, or the SUMP exit) clears
the generator flag and detaches the output, but it does not stop Timer2.
Send `0x13` before `0x0F` if the timer must be idle.  On an SMPS-enabled v4
build the reset also runs the SMPS stop path, which disables the OC5 channel;
Timer2 still runs.  On v3, `#` and the binary return are hardware resets and
stop everything; a mode change and the SUMP exit still clear the flag and
detach the output.

Consequences:

- Only one generator can own the output.  Starting one replaces the other.
- `@` while `AUX` is the selected target (the default) makes that pin an
  input and so removes a running waveform.  It leaves the generator flag set,
  so `f` still refuses and a bare `g` only stops.  Re-issue `g` or `S` with
  values to start the output again.  The exact pin outcome is unverified on
  hardware.
- Entering the binary layer also makes `AUX` an input and removes the
  waveform; the flag stays set while the layer runs, but the return path
  (`0x0F`) clears it (terminal reset on v4, hardware reset on v3).  A later
  bare `g` in a mode therefore starts a new setup.
- `a` / `A` on `AUX` while the generator flag is set make the pin an output
  again, so the timer waveform returns; the latch level is not what appears
  on the pin.  Stop the generator first if you want a static level; the
  resulting pin level is unverified.
- A mode change or a board reset clears the flag and detaches the output
  source; the `AUX` pin returns to an input as part of the reset.  A later
  bare `g` in a mode therefore starts a new setup instead of acting as a stop.
- `w` (supplies off) does not touch the generator; the signal keeps running
  in a mode that does not use the `AUX` pin.  Stop it explicitly.
- Entering SUMP makes the AUX pins inputs and removes a running waveform.
  The SUMP exit prints the banner and then runs the terminal reset, so the
  generator flag is cleared and a later bare `g` in a mode starts a new setup
  ([SUMP](../../protocol/sump/README.md)).
- The binary `0x13` command is the explicit "stop and detach" for PWM started
  from the binary layer; it also stops a terminal servo because they share
  the channel.
- BASIC `FREQ` / `DUTY` drives the same output, but in **Hz** and with a
  different range ([BASIC board pins](../../tools/basic/board-pins.md)).

### SMPS owns the channel

On an SMPS-enabled v4 build, the supply PWM owns the same output channel
while its feedback loop runs — even if the PWM output is momentarily at zero.
Terminal `g` and `S` are refused:

```
UART>g10 50
ADC busy: SMPS feedback active; command rejected.
Syntax error at char 1
UART>
```

The refusal happens before any stop; nothing is driven and the running
generator flag is not changed.  Stop the SMPS with `0xF1` in raw wire, or
with a board reset (`#` on v4, a mode change `mN`, or the binary return),
then repeat the command.  This is a defensive guard: every normal route back
to a terminal prompt stops the SMPS, so a terminal `g` should not normally
reach it ([SMPS](../smps/README.md)).

Starting the SMPS through its binary command takes the channel and stops a
running terminal generator; it does not share control with `g` / `S`.  Do not
connect a load to `AUX1` and drive it with `k` / `K` while the SMPS is
running.

## Binary layer

The binary layer programs the same timer directly with raw counts.  `0x12`
takes a prescaler selector and 16-bit duty and period counts; it replies
`0x01`.  `0x13` stops the timer, disables the output channel and detaches the
`AUX` output; it also replies `0x01`.  There is no kHz or percent conversion
in the binary path — the host supplies counts.  On an SMPS-enabled build both
commands reply `0x00` while the SMPS owns the channel, and `0x12` consumes
its five-byte frame first.

The field order, the prescaler encoding, the raw-count duty rule, the worked
exchange and the `0x0F` caveat live in [binary.md](binary.md).  Do not send
`0x0F` as a substitute for `0x13` if you want the PWM module idle.

## Limits

- Frequency is approximate: whole timer ticks, at or above the request.
- Duty is approximate: whole ticks, at or below the request, with coarse
  steps at high frequency.  The terminal `g` cannot request 100 %.
- Servo positions are integer degrees; the pulse endpoints are nominal.
- The generators are refused in HiZ; `f` and the drive/read controls are not.

## Related

- [aux-pin.md](aux-pin.md) — the pin and the assignment.
- [frequency.md](frequency.md) — the counter that shares the pin.
- [binary.md](binary.md) — the `0x12` / `0x13` raw-count commands.
- [wiring.md](wiring.md) — loads, drivers and the servo supply.
- [edge-cases.md](edge-cases.md) — prompt traps and failure recovery.
- [docs/generic/commands.md](../../generic/commands.md) — the `g` / `S`
  entries and HiZ.
- [docs/capabilities/smps/README.md](../smps/README.md) — the optional supply
  PWM that owns the channel.
- [docs/SPEC.md](../../SPEC.md) — the PWM/servo capability row.
