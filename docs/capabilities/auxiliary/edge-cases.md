# AUX family — edge cases and failure modes

Part of the AUX capability documentation set.  Each entry gives the trigger,
the observable symptom, the current behaviour and the recovery.  The topic
pages hold the normal usage: [aux-pin.md](aux-pin.md),
[pwm-servo.md](pwm-servo.md), [frequency.md](frequency.md),
[edge-wait.md](edge-wait.md) and [wiring.md](wiring.md).

## Terminal

| Trigger | Symptom | Behaviour | Recovery |
|---|---|---|---|
| `g` or `S` in HiZ | `Command not used in this mode`; nothing is driven | both commands require an active mode; the pin is not touched | enter any non-HiZ mode (`m3`, for example) and run the command there; `W` is not needed first |
| `f` while a generator is running | `ERROR: PWM active, g to disable` | the terminal counter refuses; it does not stop the generator | a bare `g` or `S` stops it (`PWM disabled`), then `f` runs |
| a bare `g` or `S` with no generator running | the interactive setup appears, not a stop | "bare" means "stop" only when a generator is already running | answer the prompts; Enter takes each default and starts the generator |
| a bare `S` while a PWM is running | `PWM disabled` | both generators share one output; a bare command from either stops whichever is active | none needed; run `g` or `S` with values to start again |
| a valid one-line `g` while running | `PWM disabled` followed by `PWM active` | the old generator is stopped, then the new settings are applied | none; both lines are the expected sequence |
| `g` with a missing or invalid value while running | `PWM disabled` first, then `1kHz-4,000kHz PWM` and both prompts | the running generator is stopped before the arguments are validated, even when the values are then rejected | enter both values at the prompts, or use a valid `g<kHz> <duty>` line |
| `g` with one value only, or with values outside the range, with no generator running | `1kHz-4,000kHz PWM`, then both prompts, defaulting to 50 kHz and 50 % | the one-line form needs both values and checks them strictly; the duty prompt accepts 0–99 even though the one-line form rejects 0 and 100 | enter values inside the prompt's range, or use `g<kHz> <duty>` |
| `S200`, a negative position or a non-numeric position | `Position in degrees`, then `(90)>` | the invalid value is discarded and the interactive path starts with the default 90 | enter 0–180, or `x` / Enter at the retune prompt to leave the current position running |
| the interactive `S` path after a missing or invalid position | after `Servo active`, the firmware keeps asking `(x)>` | each number moves the servo; `x` or an empty line leaves the loop with the last position still running | send `x` or Enter, then use a bare `S` to stop if needed |
| `w` (supplies off) while a generator is running in a mode that does not use `AUX` | the target supply is off, but the `AUX` waveform continues | `w` parks the bus and turns the supply off; it is not a generator control | stop with a bare `g` / `S`, or reset with `m1` / `#` |
| `m` / `mN`, `m1` or `#` while a generator is running | the output disappears with no `PWM disabled` line | the board reset detaches the output source and clears the generator state; a later bare `g` starts the interactive setup instead of acting as a stop | none; the output is already stopped.  Start again if wanted |
| `g` or `S` in a mode that uses `AUX` as one of its own signals (PIC and DIO, for example) | the mode's signal stops and the PWM/servo waveform appears | the generator takes the pin; the mode is not notified | stop the generator and restart the mode, or use a mode that leaves `AUX` free |
| the optional SMPS running, then a terminal `g` / `S` (or the reverse) | the output is no longer the requested waveform | both reprogram the same output-compare channel (`OC5`); see [SMPS](../smps/README.md) | stop one before starting the other |
| `A` (drive high) followed by `@` | the pin is no longer driven; the read shows the external level | `@` changes the pin to an input before sampling, so it is a release-and-read | use `A` again to drive it; for a stable read, let the target drive the pin |
| `C`, `k` or `K`, then `a`, `A` or `@` | `AUX LOW`, `AUX HIGH` or `AUX INPUT/HI-Z, READ: ...` even though the target is `CS`, `AUX1` or `AUX2` | the drive/read messages use the literal `AUX` string; the confirmation line records the real target | none; check the confirmation line when in doubt |
| `f` with no signal, a floating pin or a signal below about 1 Hz | `AUX Frequency:  autorange Frequencies < 1Hz are not supported.` | the one-second gate counted no edges; the pin is an input | check the ground and the logic swing, add a pull-up for an open-drain source, or use a signal above 1 Hz |
| a read, frequency measurement or edge wait with nothing driving the pin | the level wanders, the frequency reads zero, or the wait times out unpredictably | the pin is high impedance as an input, and the firmware does not enable a bias for it | fit an external pull-up or pull-down to a permitted logic rail, or connect a source that always drives a defined level |

## Binary

| Trigger | Symptom | Behaviour | Recovery |
|---|---|---|---|
| `0x70` while the selected pin is still an output | the wait times out, or the result is not repeatable | the wait does not change direction; it watches for a change on a pin the firmware may be driving | set the pin input first (BBIO1 direction byte for `AUX` / `CS`; `@` in the terminal for `AUX1` / `AUX2`), then send the wait |
| `0x70` with a timeout of `0x0000` | result `0x00` with the current level, immediately | the command samples the pin once; result `0x00` means "no edge waited for", not a failure of the pin | none; use a non-zero timeout to actually wait |
| an edge arrives while the selector and timeout bytes are still being sent | the wait later times out even though the signal changed | the command arms after all three payload bytes; an earlier edge is not latched | send the full payload first, then generate the edge |
| an edge selector outside `0x01`–`0x03`, or a short payload | an invalid selector returns one `0x00` byte after all three payload bytes; a short payload produces no reply and blocks the layer | the selector is validated only after the timeout is consumed | finish the three-byte payload, or reset and re-enter |
| `0x16` on a signal above about 65.28 kHz | the returned value is a multiple of 256 Hz and can differ from the terminal `f` reading | the binary command reruns at 1:1 only when the first gate counted 255 or fewer edges; the terminal reruns up to 16383 | use the terminal `f` when 1 Hz resolution is needed up to about 4.19 MHz, or accept the 256 Hz step |
| binary `0x12`, then `0x0F` without `0x13` | on v4 the pin stops being driven but the timer setup is not cleared; on v3 the board resets | `0x0F` runs the terminal reset on v4 and a hardware reset on v3; it is not the PWM-clear command | send `0x13` before leaving if the PWM module must be idle |

## A driven `AUX1` / `AUX2` survives a reset

- Trigger: `k` / `K`, drive the pin with `a` / `A`, then `m1`, `#`, a mode
  change or the binary return.
- Symptom: `AUX1` / `AUX2` keeps driving its last level; the assignment has
  returned to `A0`, so `a` / `A` / `@` no longer address it.
- Behaviour: the board reset releases the bus lines, `CS` and `AUX`, but not
  `AUX1` / `AUX2`.  This is v4-only.
- Recovery: select the pin again (`k` / `K`) and read it with `@` to make it
  an input; if you only need a safe level before leaving, drive it low, but
  that still leaves it an output.  Do not leave it wired to a target input
  when you power down ([wiring.md](wiring.md)).

## The PWM output is not the exact request

- Trigger: any PWM setting, especially near 3200–4000 kHz or with a short
  period.
- Symptom: a scope or counter shows a slightly different frequency or duty.
- Behaviour: the divider and duty are whole timer ticks.  The divider is
  rounded down, so the frequency is at or above the request; the duty is
  rounded down, so it is at or below.  At 4 MHz a 50 % request is one tick of
  four.
- Recovery: choose a lower frequency or accept the quantisation; measure the
  result if it matters ([pwm-servo.md](pwm-servo.md)).

## A manual read or binary entry removes a running waveform

- Trigger: `@` while `AUX` is the selected target (the default), or entering
  the binary layer, with a PWM or servo running.
- Symptom: the waveform disappears, but `f` still refuses with
  `ERROR: PWM active, g to disable`, and a bare `g` only stops the generator.
- Behaviour: both actions make `AUX` an input, which detaches the output
  driver, but neither clears the firmware's generator state.  The generator
  is still "active" even though the pin no longer shows it.
- Recovery: re-issue `g <kHz> <duty>` or `S<position>` with values to start
  the output again; a bare `g` stops the retained state.  The pin outcome
  follows from the reset's release mechanism and is unverified on hardware.
