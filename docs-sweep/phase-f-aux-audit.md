# Phase F delta audit - `capabilities/auxiliary`

Scope: only the statements affected by the Phase F repairs.  Verified against
the PR worktrees, not the local combined `Firmware/` (`c57e223`):

- core `bpv4-usb@d07430ca` (`bp_reset_board_state` stops Timer2, clears the V4
  `AUX1` ODC, releases `AUX1`/`AUX2`, stops the SMPS on an SMPS build).
- #218 `bpv4-51wait@e83945a` (trailing-space `g`/`S`, wait expiry before CNIF,
  `smps_start` sample/ODC preload).
- #202 `bpv4-202@2d82dc0` (silent I2C `0x09`).
- #242 `bpv4-dio@b8c8296` (DIO `w` parks `AUX1`/`AUX2` on v4).

The local `c57e223` has none of those fixes (checked the same functions), so
the five areas below are the real delta.  The rest of the set was not
re-reviewed.

## Verdict

**Yes with fixes.**  Four behaviour changes have not reached the set yet: the
v4 reset now stops Timer2, the I2C `0x09` reply is silent, trailing spaces no
longer turn `g`/`S` into a setup, and a zero-timeout `0x70` can no longer
report an edge.  One smaller fact (DIO `w` now parks seven pins) is stale.  The
remaining Phase F claims I checked are already correct, including the
pre-existing reset release of `AUX1`/`AUX2`, the new one-line `g` range, the
duty quantisation, the servo v4 `+1` tick, the `f` autorange boundary and
five-second bound, and the SMPS refusal/ownership text.

## Findings

### 1. [severity: major] The v4 reset now stops Timer2; the set says it does not

The new core `bp_reset_board_state()` writes `T2CONbits.TON = OFF` before it
retires the AUX state (core `d07430ca`, commit message: "the board reset clears
only Timer2 TON before retiring AUX state").  Every v4 reset path that uses
this function is affected: `#`, a mode change, the binary return and the SUMP
exit (and the v3 software paths too, because the line is outside the V4
conditional).  The pin/output side is unchanged: the output source is still
detached and the generator flag cleared.

Stale locations:

- `pwm-servo.md:128` "no `PWM disabled` line; Timer2 is not stopped".
- `pwm-servo.md:129` "`#` | v4 terminal reset: clears the flag and detaches the
  output, Timer2 is not stopped; v3 hardware reset".
- `pwm-servo.md:132` "binary `0x0F` | v4 terminal reset: clears the flag and
  detaches the output, Timer2 is not stopped".
- `pwm-servo.md:223` "`| timer registers | Timer2 and the OC5 compare | a new
  `g` / `S`; binary `0x12`, `0x13` or `0x16`; a v3 hardware reset |`".
- `pwm-servo.md:225-229` "A v4 reset ... clears the generator flag and detaches
  the output, but it does not stop Timer2.  Send `0x13` before `0x0F` if the
  timer must be idle. ... the reset also runs the SMPS stop path, which
  disables the OC5 channel; Timer2 still runs."
- `binary.md:85-87` "the reset does not stop Timer2, so the timer may still
  run."
- `edge-cases.md:159-160` "the reset clears the generator flag and detaches the
  output; Timer2 is not stopped."
- `edge-cases.md:349-351` "The v4 reset clears the generator flag and detaches
  the output, but it does not stop Timer2."

Why it matters: the text is the only place a reader learns what a reset leaves
behind; it currently tells them a timer keeps running when it is stopped.

Exact fix (all locations): replace "Timer2 is not stopped" / "does not stop
Timer2" / "Timer2 still runs" with "the reset stops Timer2 (`TON` cleared) and
detaches the output".  Keep the `0x13` advice but give the remaining reason:
"the reset stops Timer2 and detaches the pin, but it does not disable the OC5
channel or clear its period/duty setup; send `0x13` if the module must be fully
idle."  In the three-layer table, the timer-registers row becomes "a board
reset (stops Timer2; the OC5/period setup stays); a v3 hardware reset".  On v3
the mode change and SUMP exit also stop Timer2 now; only `#` and the binary
return are hardware resets.

### 2. [severity: major] I2C `0x09` is silent; the set documents terminal text replies

`bpv4-202@2d82dc0` makes the binary `0x09` handler write the pads directly and
reply only the acknowledgement plus the sub-result.  Every sub-command is
exactly two bytes: `0x01`, then the sub-result (`0x01` for `0x00`/`0x01`/
`0x02`/`0x10`/`0x20`, the pin level for `0x03`, `0x00` for an unknown
sub-command).  No `AUX LOW` / `AUX HIGH` / `AUX INPUT/HI-Z` text is sent.

Stale locations:

- `binary.md:22` "`0x01`, optional text for drive/release, then the
  sub-result".
- `binary.md:126-128` table rows "`AUX LOW` CR LF, then `0x01`", "`AUX HIGH`
  CR LF, then `0x01`", "`AUX INPUT/HI-Z` (no line break), then `0x01`".
- `binary.md:134-135` "The drive and release sub-commands reuse the terminal
  pin messages, so their reply is not two bytes:".
- `binary.md:137-149` the worked exchange ("host <- \"AUX LOW\" CR LF").
- `binary.md:151-154` "A host must read the text for `0x00`-`0x02`; do not
  assume a fixed two-byte reply.  If a host needs a clean reply for a drive or
  release, the terminal `a` / `A` / `@` commands act on one pin only."
- `edge-cases.md:301-306` "the reply contains `AUX LOW`, `AUX HIGH` or
  `AUX INPUT/HI-Z` between the acknowledgement and the sub-result. ... the I2C
  handler reuses the terminal pin messages. ... read the text, or use `0x03`,
  `0x10` and `0x20` for clean replies; a host that needs one-pin control should
  use the terminal `a` / `A` / `@`."

Why it matters: this is the byte-level contract a host parser is written
against; a host that waits for the text now reads the sub-result as text.

Exact fix: state that every `0x09` reply is exactly two bytes, `0x01` + the
sub-result; drive/release return `0x01` and print nothing; delete the text
table, the text exchange and the "read the text"/"terminal for a clean reply"
advice.  Keep "the first byte means the command was recognised, not that the
sub-command succeeded" and the unknown → `0x01 0x00` rule.  The edge-case entry
becomes "drive/release replies are `0x01 0x01`; nothing is printed" (or is
deleted as no longer a trap).

### 3. [severity: major] Trailing spaces are ignored now; the set says they take the setup path

`bpv4-51wait@e83945a` skips spaces before the bare-command check in both
`bp_pwm_setup()` and `bp_servo_setup()`.  `g ` and `S ` are therefore identical
to `g` and `S`: with a generator running they stop it, print `PWM disabled`
and return; with nothing running they enter the interactive setup.  The old
behaviour (a space is an argument; `g ` stops then sets up; `S ` stops
silently then sets up) is gone.

Stale locations:

- `pwm-servo.md:137-139` "The letter must be the end of the line: a trailing
  space is read as an argument, so `g ` and `S ` take the setup path, stopping
  a running output first (`g ` prints `PWM disabled`; `S ` stops silently)."
- `edge-cases.md:85-92` the whole entry "A stop command with a trailing space"
  ("`g ` prints `PWM disabled` and then starts the interactive setup; `S `
  stops the old output silently and starts the interactive servo setup. ...
  a space makes the command take the parameter path.  Do: send `g` or `S` with
  no trailing space ...").

Why it matters: the set tells a user to avoid something that is now safe and
inverts the `S ` behaviour (it now prints `PWM disabled`).

Exact fix: replace both with "Trailing spaces are ignored: `g ` and `S `
behave exactly like `g` and `S` - a stop when a generator runs, the setup when
none does."  Delete the edge-case entry (or reduce it to one line saying the
spaces are accepted).  Also adjust `pwm-servo.md:77-78` "Any `g` line other
than the bare letter ..." to "Any `g` line, including `g ` with only spaces,
stops a running generator first; if nothing but spaces follows it stops and
returns."

### 4. [severity: major] A zero-timeout `0x70` can no longer report `0x01`; the set says it can

`bpv4-51wait@e83945a` moved the timeout check ahead of the `CNIF` check
("Expiry wins over a pending edge, including the zero-timeout arm window").
With a timeout of `0x0000` the first loop iteration always expires, so the
reply is always `0x00` plus the level.  The old "a change between arming and
the first sample can still be reported" case no longer exists.

Stale locations:

- `edge-wait.md:74-78` "`0` does not wait for an edge: ... normally returns
  result `0x00` with the current level. ... A change that lands between arming
  and the first sample can still be reported as an edge, so `0x01` is possible;
  do not use a zero timeout as a guaranteed edge test."
- `edge-cases.md:245-248` "You see: normally result `0x00` with the current
  level, immediately. ... A change that lands between arming and the first
  sample can still be reported as `0x01`."

Why it matters: a host that treats `0x01` as possible from a zero timeout will
add a false edge path; the mode is now a deterministic sample.

Exact fix: "`0` does not wait: it always returns result `0x00` followed by the
current level.  Use it to sample the selected pin, not to test for an edge."
Drop "normally" and the `0x01` caveat in both places.  The existing
"documented way to read the selected pin" sentence stays; it is now strictly
true.

### 5. [severity: minor] DIO `w` now parks `AUX1`/`AUX2`; the wiring example still says five

`bpv4-dio@b8c8296` extends the V4 DIO cleanup mask to `AUX1` and `AUX2`
(`park_mask |= AUX1 | AUX2`), so `w` releases seven header pins on v4 (five on
v3).  The aux wiring page uses DIO as the example of a cleanup that leaves a
manually driven `AUX1`/`AUX2` alone.

Stale location:

- `wiring.md:147-150` "It may park the pins of the active mode (DIO parks its
  five signal pins, for example) but it does not guarantee that a manually
  driven `AUX1` / `AUX2` is released."

Why it matters: the general advice ("confirm with `v`, or use a board reset")
is still safe, but the one concrete example is now the opposite of the
behaviour, and the DIO set contradicts it (see cross-set list).

Exact fix: "It may park the pins of the active mode (DIO parks its five signal
pins and, on v4, `AUX1` and `AUX2` - seven in total; other modes may release
fewer) but it does not guarantee that every manually driven pin is released."
No other aux page needs a change: `aux-pin.md:167-170` already says the `w`
outcome "depends on the active mode's cleanup", and `pwm-servo.md:127` says
"not a generator control".

### 6. [severity: minor] Reflow the three-layer table while editing the timer row

The table at `pwm-servo.md:219-223` has cells of 195 and 239 characters (the
generator-flag and output-path rows), and finding 1 changes the timer row.
Presentation is only being checked on changed passages; this one is changed and
the long rows render as an unreadable single line on a narrow terminal.

Exact fix: while editing, shorten the cells to the layer + the controlling
events (e.g. "generator flag - bare `g`/`S`, a new setup, a board reset, an
SMPS start"; "output path - `@` on `AUX`, BBIO1/SUMP entry, `a`/`A`, `0x13`, a
board reset"; "timer registers - a new `g`/`S`, `0x12`/`0x13`/`0x16`, a board
reset (stops Timer2), a v3 hardware reset"), or move the detail into the prose
below the table.

## Cross-set list (for the main session)

These pages still contradict the Phase F heads; they are owned by other set
repairs, not by this one.

1. `docs/protocol/i2c/binary.md` - the same silent-`0x09` change:
   - `:72` "`0x01`, optional text for drive/release, then the sub-result".
   - `:126-127` "release AUX with `0x09 0x02` (and consume the `AUX INPUT/HI-Z`
     text)".
   - `:232-245` "It is **not always two bytes**: the drive/release
     sub-commands print a terminal text line ... A host must consume that
     text".
   - `:265-266` "consume the `AUX INPUT/HI-Z` text".
2. `docs/protocol/binary/commands.md` - the reset/Timer2 change:
   - `:287-291` "`0x0F` does not stop the timer either ... the target stops
     being driven while Timer2 may still run ... Send `0x13` before leaving if
     you want the module idle."
3. `docs/capabilities/smps/` - RA-15/RA-16 both need the flip:
   - `binary.md:82-86` "A start does not clear the stored sample ... possibly
     the previous run's sample ... no freshness flag"; also `:196-198`.
   - `feedback-loop.md:82` "A start does not clear the stored sample."
   - `README.md:133` "`0xF0` can return the previous run's sample after a
     start"; `edge-cases.md:60-65`, `:196-199`.
   - `wiring.md:69-75` "the SMPS start does not clear the open-drain bit.  A
     v4 terminal reset (`#` or `0x0F`) does not clear it either"; also
     `README.md:46-48` and `edge-cases.md:129-142`.
4. `docs/capabilities/lcd/` - RA-17:
   - `spi-link.md:114-123` "If a binary SPI session left the SPI outputs open
     drain, the bits persist ... Power-cycle the Bus Pirate, or enter the SPI
     mode ...".
   - `edge-cases.md:106-112` the `Open drain outputs` workaround.
   - `README.md:55-57` "still says `Open drain outputs`; that is the mode's
     flag, not the pad state."
5. `docs/protocol/dio/` - RA-7, the largest cross-set block.  The set still
   documents the old five-pin park as a contract:
   - `park-and-reset.md:24-26`, `:34-35`, `:52`, `:58-59`, `:70-74`,
     `:112-125`, `:133-134`, `:146-151` ("`w` does not release `AUX1` or
     `AUX2`"; "`AUX1` / `AUX2` | **not changed**").
   - `edge-cases.md:27`, `:86`, `:89-95` ("`w` leaves `AUX1` / `AUX2` driven
     (v4)").
   - `README.md:56-57`, `:112`; `aux-pin.md:143` ("`w` releases a driven
     `AUX1` / `AUX2` target | - | no"); `pin-control.md:174`;
     `entry-and-setup.md:124`, `:133-136`; `wiring.md:140-142`.
6. Not contradictory, checked and consistent: `docs/generic/*` (no `g`/`S`
   trailing-space or `0x70` zero-timeout claim), `docs/capabilities/adc/*`
   (SMPS refusal framing), `docs/SPEC.md` (AUX rows already point at this set).

## Checked and unchanged (delta only)

- Reset releases `AUX`/`CS`/`AUX1`/`AUX2` and returns the target to `A0` -
  already true in `c57e223` and still true in the new core (the new part is
  Timer2 + the `AUX1` ODC bit).
- Terminal `g` one-line range 1-4000 kHz and 0-99 % - matches
  `valid_arguments` in `#218`; 100 % is rejected in both forms.
- v4 duty encoding (0 % special, a one-tick high rounds down, 4 MHz table
  0-49/50-74/75-99 % -> 0/50/75 %) - matches `pwm_duty_register()`.
- Servo compare values 62/94/126 and the v4 `+1` tick high time - matches
  `bp_servo_setup()` and the same falling-comparison rule.
- `f` autorange at 16,384 counts, the `0x16` rerun at 255 counts, the 256-count
  boundaries and the five-second period bound (~7 s total) - match
  `bp_frequency_counter_setup()` / `average_sample_frequency()`.
- SMPS refusal text and capture (`ADC busy: SMPS feedback active; command
  rejected.` + syntax error), `0x12`/`0x13` -> `0x00` with the five-byte frame
  consumed, and SMPS start clearing the terminal generator - match `#218`.
- I2C binary entry resets the target to `AUX` - matches `#202`.

## Unknowns

- The v3 side of the unified reset (`T2CON.TON = OFF` is outside the V4
  conditional) was not run on hardware; the v3 statements already say
  "unverified".
- Whether the DIO seven-pin park is visible on the bench: #242 is
  build/native/codegen only and the aux set does not need a bench claim for
  this wording.
- The `AUX2` ODC bit is not cleared by the reset (core clears only `AUX1`, the
  SMPS pin).  No aux page claims either way; noted only in case the asymmetry
  is unintended at code-owner level.
