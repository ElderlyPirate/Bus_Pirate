# AUX family — edge cases and failure modes

Part of the AUX capability set.  Entries are grouped by function; each gives
the trigger, what you see, why, and what to do.  Normal usage is in the topic
pages: [aux-pin.md](aux-pin.md), [pwm-servo.md](pwm-servo.md),
[frequency.md](frequency.md), [edge-wait.md](edge-wait.md),
[binary.md](binary.md) and [wiring.md](wiring.md).

## Terminal AUX pin

### A selection key is followed by `a`, `A` or `@`

- Trigger: `C`, `k` or `K`, then `a`, `A` or `@`.
- You see: `AUX LOW`, `AUX HIGH` or `AUX INPUT/HI-Z, READ: ...`, even though
  the target is `CS`, `AUX1` or `AUX2`.
- Why: the drive and read messages use the literal `AUX` string; only the
  confirmation line names the real target.
- Do: trust the confirmation line, or use
  [Checking the target](aux-pin.md#checking-the-target) when it is gone.

### The confirmation line is gone at `HiZ>`

- Trigger: a target was selected earlier and the confirmation line has
  scrolled away, at `HiZ>`.
- You see: `i` prints only the banner; `v` lists pin names and directions but
  does not mark the selection.
- Why: the status block with the confirmation line is appended by `i` only in
  a bus mode; no `HiZ>` command reports the target without changing
  something.
- Do: reset (`#`; a hardware reset on v3) to return the target to `AUX`, or
  press the key for the pin you intend to use and read its line.  Do not press
  a selection key just to look.

### `A` (drive high), then `@`

- Trigger: `A`, then `@` on the same target.
- You see: the pin is no longer driven; the read shows the external level.
- Why: `@` changes the pin to an input before sampling, so it is a
  release-and-read.
- Do: `A` again to drive it; for a stable read, let the target drive the pin.

### Nothing drives the pin

- Trigger: a read, frequency measurement or edge wait with nothing driving
  the pin.
- You see: the level wanders, the frequency reads zero, or the wait times out
  unpredictably.
- Why: the pin is high impedance as an input, and the firmware does not enable
  a bias for it.
- Do: fit an external pull-up or pull-down to a permitted logic rail, or
  connect a source that always drives a defined level.

## PWM and servo

### `g` or `S` in HiZ

- Trigger: `g` or `S` at `HiZ>`.
- You see: `Command not used in this mode`; nothing is driven.
- Why: both commands require an active mode; the pin is not touched.
- Do: enter any non-HiZ mode (`m3`, for example) and run the command there;
  `W` is not needed first.

### `f` while a generator is running

- Trigger: `f` with a PWM or servo active.
- You see: `ERROR: PWM active, g to disable`.
- Why: the terminal counter refuses; it does not stop the generator.
- Do: a bare `g` or `S` stops it (`PWM disabled`), then `f` runs.

### A bare `g` or `S` with no generator running

- Trigger: `g` or `S` with nothing running.
- You see: the interactive setup appears, not a stop.
- Why: "bare" means "stop" only when a generator is already running.
- Do: answer the prompts; Enter takes each default and starts the generator.

### A bare `S` while a PWM is running

- Trigger: `S` with no position while a PWM runs.
- You see: `PWM disabled`.
- Why: both generators share one output; a bare command from either stops
  whichever is active.
- Do: none needed; run `g` or `S` with values to start again.

### Trailing spaces after `g` or `S`

- Trigger: `g ` or `S ` (a space before Enter).
- You see: the same as the bare command: `PWM disabled` when a generator runs,
  the interactive setup when none does.
- Why: spaces are skipped before the bare-command check, so a trailing space is
  not an argument.
- Do: none; the trailing space is accepted.

### A valid one-line `g` while running

- Trigger: `g<kHz> <duty>` with both values valid while a generator runs.
- You see: `PWM disabled` followed by `PWM active`.
- Why: the old generator is stopped, then the new settings are applied.
- Do: none; both lines are the expected sequence.

### `g` with a missing or invalid value while running

- Trigger: `g` with one value missing, or with a value outside the range,
  while a generator runs.
- You see: `PWM disabled` first, then `1kHz-4,000kHz PWM` and both prompts.
- Why: the running generator is stopped before the arguments are validated,
  even when the values are then rejected.
- Do: enter both values at the prompts, or use a valid `g<kHz> <duty>` line.

### `g` with one value only, or with an out-of-range value, while idle

- Trigger: `g` with one value, or with a value outside the range, and no
  generator running.
- You see: `1kHz-4,000kHz PWM`, then both prompts, defaulting to 50 kHz and
  50 %.
- Why: the one-line form needs both values; `g` accepts 0–99 % duty but not
  100 %.
- Do: enter values inside the prompt's range, or use `g<kHz> <duty>`.

### A servo position outside 0–180

- Trigger: `S200`, a negative position or a non-numeric position.
- You see: `Position in degrees`, then `(90)>`.
- Why: the invalid value is discarded and the interactive path starts with the
  default 90; `x` is not an exit key at this prompt.
- Do: enter 0–180, or Enter for 90; use `x` only at the later `(x)>` prompt.

### An invalid `S<position>` while a generator is running

- Trigger: `S` followed by a position outside 0–180, or by a non-numeric
  value, while a generator runs.
- You see: the current output stops silently, then `Position in degrees` and
  `(90)>`.
- Why: the servo setup stops the shared timer before validating the position;
  it does not print `PWM disabled`.
- Do: enter 0–180 (or Enter for 90), or reset the board if the old output was
  wanted.

### The interactive `S` path after a missing or invalid position

- Trigger: a position was entered at the `(90)>` prompt.
- You see: after `Servo active`, the firmware keeps asking `(x)>`.
- Why: each number moves the servo; `x` or an empty line leaves the loop with
  the last position still running.
- Do: send `x` or Enter, then use a bare `S` to stop if needed.

### `w` (supplies off) while a generator is running

- Trigger: `w` in a mode that does not use `AUX`, with a generator running.
- You see: the target supply is off, but the `AUX` waveform continues.
- Why: `w` parks the bus and turns the supply off; it is not a generator
  control.
- Do: stop with a bare `g` / `S`, or reset with a mode change (`mN`) or `#`.

### A mode change or `#` while a generator is running

- Trigger: `mN` or `#` with a generator active.
- You see: the output disappears with no `PWM disabled` line.
- Why: the reset clears the generator flag, stops Timer2 and detaches the
  output.  It does not disable the OC5 channel or clear its period/duty setup.
  On v3 `#` is a hardware reset; a mode change is the software reset.  A later
  bare `g` in a mode starts the interactive setup instead of acting as a stop.
- Do: none; the output is already stopped.  Send binary `0x13` first if the
  OC5 module must be fully idle.

### `g` or `S` in a mode that uses `AUX`

- Trigger: a generator started in a mode that uses `AUX` as one of its own
  signals (PIC and DIO, for example).
- You see: the mode's signal stops and the PWM/servo waveform appears.
- Why: the generator takes the pin; the mode is not notified.
- Do: stop the generator and restart the mode, or use a mode that leaves `AUX`
  free.

### The SMPS is running, then a terminal `g` or `S`

- Trigger: a terminal `g` or `S` on an SMPS-enabled build while the supply
  feedback loop runs.
- You see: `ADC busy: SMPS feedback active; command rejected.` then a syntax
  error.
- Why: the terminal generator is refused before it touches the channel; the
  SMPS keeps running.
- Do: stop the SMPS with `0xF1` in raw wire, or reset the board, then repeat.

### A terminal generator is active, then the SMPS starts

- Trigger: an SMPS start while a terminal PWM or servo runs.
- You see: the terminal waveform stops and the SMPS takes the channel.
- Why: starting the SMPS detaches the AUX output and clears the terminal
  generator flag.
- Do: stop the SMPS, then start `g` / `S` again.

### The PWM output is not the exact request

- Trigger: any PWM setting, especially near 3200–4000 kHz or with a short
  period.
- You see: a scope or counter shows a different frequency or duty.
- Why: the divider and duty are whole timer ticks.  The divider is rounded
  down, so the frequency is at or above the request; the duty is rounded down,
  so it is at or below.  On v4 a one-tick high time rounds down to 0 %, so
  at 4 MHz (a four-tick period) 0–49 % gives 0 %, 50–74 % gives 50 %, and
  75–99 % gives 75 %.
- Do: choose a lower frequency or accept the quantisation; measure the result
  if it matters ([pwm-servo.md](pwm-servo.md)).

## Frequency

### `f` sees no edges

- Trigger: `f` with no signal, a floating pin, or a signal below about 1 Hz.
- You see: `AUX Frequency:  autorange Frequencies < 1Hz are not supported.`
- Why: the one-second gate counted no edges; the pin is an input.
- Do: check the ground and the logic swing, add a pull-up for an open-drain
  source, or use a signal above 1 Hz.

### `f` catches an edge but the period capture times out

- Trigger: a signal below about 1 Hz, or a slow or irregular signal that gives
  the 1:1 gate one or more edges but not enough complete cycles.
- You see:
  `AUX Frequency:  autorange  autorange Frequencies < 1Hz are not supported.`
  — two `autorange` tokens and no `Hz` value.
- Why: the first gate counted an edge, so the 1:1 rerun ran and printed the
  second `autorange`; the period capture then reached its five-second bound
  and returned no period.
- Do: treat the input as below the supported range; use a faster signal or
  count events externally.  This path can run for about seven seconds because
  the two one-second gates come before the five-second period bound.

## Binary AUX

### `0x70` while the selected pin is still an output

- Trigger: a wait on a pin the firmware is driving as an output.
- You see: the wait times out, or the result is not repeatable.
- Why: the wait does not change direction; it watches for a change on a pin
  the firmware may be driving.  Change notification on a driven output is not
  confirmed.
- Do: set the pin input first (BBIO1 direction byte for `AUX` / `CS`; `@` in
  the terminal for `AUX1` / `AUX2`), then send the wait.

### `0x70` with a timeout of `0x0000`

- Trigger: a wait with a zero timeout.
- You see: result `0x00` with the current level, immediately.
- Why: expiry is checked before the change flag, so a zero timeout expires on
  the first pass and no edge can be reported.  Result `0x00` means "no edge
  waited for", not a failure of the pin.
- Do: none for a plain sample; use a non-zero timeout to wait for an edge.

### An edge arrives while the payload is still being sent

- Trigger: an edge before the selector and both timeout bytes have arrived.
- You see: the wait later times out even though the signal changed.
- Why: the command arms after all three payload bytes; an earlier edge is not
  latched.
- Do: send the full payload first, then generate the edge.

### An edge selector outside `0x01`–`0x03`

- Trigger: a selector byte other than rising, falling or either.
- You see: one `0x00` byte after the two timeout bytes have arrived.
- Why: the selector is validated only after the timeout is consumed.
- Do: send a valid selector; do not expect the timeout bytes to be skipped.

### A short `0x70` payload

- Trigger: fewer than three bytes after `0x70`.
- You see: no reply, and the layer appears blocked.
- Why: the missing bytes are still expected.
- Do: send the missing bytes to complete the frame, or reset the board.

### A short `0x12` payload

- Trigger: fewer than five bytes after `0x12`.
- You see: no reply, and the layer appears blocked.
- Why: the handler reads the full five-byte frame before it replies, even when
  the SMPS will refuse it.
- Do: send the remaining bytes to complete the frame, or reset the board.

### `0x71`–`0x7F` in a nibble-group mode

- Trigger: any `0x7x` byte other than `0x70`.
- You see: `0x00`, with no payload consumed.
- Why: only the low nibble `0` is the wait; the other `0x7x` bytes are
  rejected.
- Do: send exactly `0x70`.

### `0x09` in BBIO1

- Trigger: `0x09` sent in BBIO1.
- You see: AVRISP-MKII serial entry, or `0x00` then `BBIO1` when AVRISP is
  absent.
- Why: BBIO1 `0x09` is not an AUX command.
- Do: use the I2C binary mode for `0x09` AUX/CS, or the terminal for
  `a` / `A` / `@`.

### `0x09` drive, release or select replies

- Trigger: `0x09` followed by `0x00`, `0x01`, `0x02`, `0x10` or `0x20`.
- You see: exactly two bytes, `0x01` then `0x01`; nothing is printed.
- Why: the binary handler writes the pads directly and does not reuse the
  terminal pin messages.
- Do: read two bytes for every sub-command; `0x03` returns the pin level as
  the second byte, and an unknown sub-command returns `0x01 0x00`.

### `0x70` on `AUX1` / `AUX2` in I2C binary

- Trigger: a wait on `AUX1` or `AUX2` inside the I2C binary mode.
- You see: the wait follows `AUX`, not the selected pin.
- Why: I2C entry resets the target to `AUX`; its selector can choose only
  `AUX` / `CS`.
- Do: select the pin in the terminal and use BBIO1 or a nibble-group mode
  other than I2C.

### `0x70` on `AUX1` while the SMPS drives it

- Trigger: a wait on `AUX1` while the supply PWM runs.
- You see: the wait follows the supply PWM; the result depends on the
  switching phase.
- Why: the SMPS owns `AUX1` and the wait does not change direction.
- Do: wait on `AUX` / `CS`, or stop the SMPS first.

### `0x16` near the prescaler boundary

- Trigger: a frequency near 65,536 Hz.
- You see: the returned value is a multiple of 256 Hz and can differ from the
  terminal `f` reading.
- Why: the binary command reruns at 1:1 only when the first gate counted 255
  or fewer edges; the terminal reruns up to 16,383.  At exactly 65,536 Hz the
  gate can hold 255 or 256 counts depending on phase.
- Do: use the terminal `f` when 1 Hz resolution is needed below 4.194304 MHz,
  or accept the 256 Hz step.

### `0x16` while a terminal generator is running

- Trigger: a frequency count with a terminal PWM or servo active.
- You see: the generator stops; the command returns a reading.
- Why: `0x16` shuts down the generator timers and makes `AUX` an input instead
  of refusing.
- Do: re-issue `g` / `S` with values if you want the output back.

### `0x12`, then `0x0F` without `0x13`

- Trigger: binary PWM setup, then a return to the terminal without a clear.
- You see: on v4 the pin stops being driven and the terminal reset runs; on v3
  the board resets.
- Why: `0x0F` is a terminal/hardware reset, not the PWM-clear command.  The v4
  reset clears the generator flag, stops Timer2 and detaches the output, but
  it does not disable the OC5 channel or clear its period/duty setup.
- Do: send `0x13` before leaving if the OC5 module must be fully idle.

### `0x12` or `0x13` while the SMPS owns the channel

- Trigger: a PWM setup or clear on an SMPS-enabled build while the feedback
  loop runs.
- You see: `0x00`; `0x12` still consumes its five-byte frame.
- Why: both are refused without touching the channel.
- Do: stop the SMPS with `0xF1` in raw wire, or reset the board.

### `0x12` with a duty count above the period count

- Trigger: a duty count larger than the period count.
- You see: the output no longer follows the requested percentage.
- Why: the binary command writes the counts as given; it does not clamp the
  duty to the period, and a compare outside the period can leave the output
  high.
- Do: keep the duty count at or below the period count, or use the terminal
  `g` percentage form.

## Resets and persistence

### `@` removes a running waveform but keeps the generator flag

- Trigger: `@` while `AUX` is the selected target (the default), with a PWM
  or servo running.
- You see: the waveform disappears, but `f` still refuses with
  `ERROR: PWM active, g to disable`, and a bare `g` only stops the generator.
- Why: `@` makes `AUX` an input, which detaches the output driver, but it
  does not clear the firmware's generator flag.  The generator is still
  "active" even though the pin no longer shows it.
- Do: re-issue `g <kHz> <duty>` or `S<position>` with values to start the
  output again; a bare `g` clears the retained flag.  The exact pin outcome is
  unverified.

### Entering the binary layer removes the waveform

- Trigger: entering BBIO1 (or a binary sub-protocol) with a PWM or servo
  running.
- You see: the waveform stops; the binary commands work on the same channel.
- Why: binary entry makes `AUX` an input.  The generator flag stays set while
  the binary layer runs, but on v4 the return path (`0x0F`) runs the terminal
  reset, which clears the flag and returns the target to `AUX`; on v3 `0x0F`
  is a hardware reset.
- Do: none on return; the reset has already cleared the generator and the
  selection.

### A reset releases every AUX target

- Trigger: drive `AUX1` or `AUX2` with `k` / `K` and `a` / `A`, then run `#`,
  a mode change (`mN`), the binary return or the SUMP exit.
- You see: the pin returns to an input; the target returns to `AUX`.
- Why: the reset releases `AUX`, `CS` and (on v4) `AUX1` / `AUX2` and
  clears the generator output path.  They are all inputs afterwards.
- Do: none needed.  Re-select the target with `k` / `K` if you want to drive
  it again.

### A SUMP exit runs the terminal reset

- Trigger: leave SUMP with `0x0F`, `#` or `$`.
- You see: the banner and the `HiZ>` prompt; any later `a` / `A` / `@` acts on
  `AUX`, and a bare `g` in a mode starts a new setup instead of stopping.
- Why: entering SUMP makes the AUX pins inputs, so a running waveform stops;
  the exit prints the banner, runs the terminal reset, and leaves a `HiZ>`
  prompt.  The target returns to `AUX`, the generator flag is
  cleared, the pins are inputs, the target supplies and pull-ups are off, and
  the display is back to HEX.
- Do: none; the state is reset.  Select a target again if you need one.
