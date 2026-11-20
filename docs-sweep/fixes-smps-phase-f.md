# SMPS docs - Phase F delta repair record

**Scope:** `docs/capabilities/smps/` plus this record.  Worktree
`bpv4-docssweep`, branch `docs/combined-sweep`, docs head `d5e5814c`; the
`Firmware/` tree is the combined image `9b179cb5`.  No firmware changes, no
commits, no pushes, no WORKLOG edits.

**Result:** 12 findings (1 blocker, 9 major, 2 minor).  All 12 are fixed in
the SMPS set.  0 are rejected.  0 code-owner candidates: the firmware already
implements the start-side sample/ODC clears and the reset-side ODC clear.  The
cross-set list has five pages plus one page that is already correct.

## Decisions

| # | Decision | What changed (file + short quote) | Evidence |
|---|---|---|---|
| 1 | fixed | `feedback-loop.md` "The sample": "A start clears the stored sample to zero.  Before the first new conversion, `0xF0` returns `0x00 0x00`; after a conversion it reports the new sample.  There is no validity flag: a real zero sample also reads zero, so a zero first read is not proof that a conversion has happened."  The old "does not clear" and discard/poll bullets are gone. | `Firmware/capability_smps/smps.c` `smps_start()`: `smps_state.voltage_reading = 0;` before the new setup.  `smps_adc()` sends the stored value; only `_ADC1Interrupt()` replaces it after a conversion. |
| 2 | fixed | `binary.md` reply table: "`0xF0` after a new start \| `0x00 0x00` until the first conversion; then the new sample \| ... the start clears the stored sample; no validity flag, so a real zero also reads zero".  The worked-session comment says "the start cleared it; no conversion yet"; the prose says "zero if no conversion has completed, otherwise the new sample; never the previous run's sample".  "Starting again" says the start clears the sample and the immediate read is zero.  Limits says the first read is zero until the first conversion.  The Python comment says "first read: zero until the first conversion". | Same `smps.c` facts.  The terminal-reset row still says the last stored sample because `smps_stop()` does not clear it. |
| 3 | fixed | `README.md` Read bullet: "A start clears the stored sample: before the first new conversion `0xF0` reads `0x00 0x00`.  There is no validity flag, so a real zero sample also reads zero."  The shortest-session row is "read before the first conversion (zero)"; the limits row says the first read is `0x00 0x00` until the first conversion and a real zero also reads zero. | Same `smps.c` facts. |
| 4 | fixed | `edge-cases.md` "Starting again": "A `0xF0` read immediately after the new start reads `0x00 0x00` until the first new conversion"; the start "clears the stored sample to zero, clears the `AUX1` open-drain bit"; recovery is only "Send `0xF1` ...", with no discard/poll. | Same `smps.c` facts; a re-start runs `smps_start()` again. |
| 5 | fixed | `edge-cases.md` "A reading after stop is stale": "stop turns the converter off and leaves the stored sample unchanged.  A new start clears it; a power cycle or hardware reset also reinitializes it."  The read-after-stop warning is kept. | `smps_stop()` leaves `voltage_reading`; `smps_start()` clears it; startup static initialization clears it. |
| 6 | fixed | `binary.md` "Read on a fresh power-up, and after stop" now shows `start`, then `host -> 0xF0` / `host <- 0x01 0xD3` "read after a conversion", then stop, then the stopped read of `0x01 0xD3`. | With the start-side clear, a stop with no intervening conversion would leave `0x00 0x00`; the added read makes the shown sample valid. |
| 7 | fixed | `README.md` wiring headline: "The SMPS start clears the `AUX1` open-drain bit, and a v4 terminal or board reset (`#`, `0x0F`, or a mode change) also clears it.  If a binary path drives `AUX1` before the start without a terminal reset, a leftover open-drain high can still need an add-on pull-up." | `smps.c` `smps_start()`: `BP_AUX1_ODC = OFF;`.  `Firmware/core_system/base.c` `bp_reset_board_state()`: `BP_AUX1_ODC = OFF;` on v4.  `#`, `0x0F` and `m` reach that reset. |
| 8 | fixed | `wiring.md` open-drain paragraph: "The SMPS start clears the `AUX1` open-drain bit and drives the PWM push-pull.  A v4 terminal or board reset (`#`, `0x0F`, or a mode change) also clears it; no power cycle is needed."  A pull-up remains only for a binary path driving `AUX1` before the start.  Signal levels now say "the SMPS start clears the open-drain bit, so the PWM is push-pull after a start." | Same `smps.c` / `base.c` evidence; the old power-cycle recovery is removed. |
| 9 | fixed | `edge-cases.md` entry retitled "`AUX1` is open drain before a binary SMPS start"; trigger is a binary path without a terminal reset that drives `AUX1` before the start.  Behaviour: the start clears the ODC bit and drives the pin push-pull; `#`, `0x0F` and `m` also clear it; no power cycle is required.  A pull-up remains for the pre-start binary path. | Same `smps.c` / `base.c` evidence. |
| 10 | fixed | `exit.md` stop table adds "`AUX1` ODC \| unchanged".  The paths table adds an `AUX1` ODC column: `0x0F`, `#`, `m` and hardware reset/power cycle = cleared on v4; `0xF1`, `0x00` and `0xF1` in `BBIO1` = unchanged.  The reset descriptions for `0x0F`, `#`, `m` and hardware reset now name the clear. | `bp_reset_board_state()` clears `BP_AUX1_ODC` on v4; `smps_stop()` does not touch it; `0xF1` in `BBIO1` is a pin-state byte and does not touch ODC. |
| 11 | fixed | `feedback-loop.md` "What a start configures": step 1 now ends "and clears the stored sample to zero"; step 4 now starts "Clears the `AUX1` open-drain bit, drives `AUX1` low, and routes the output-compare channel...". | `smps_start()` writes `smps_state.voltage_reading = 0` and `BP_AUX1_ODC = OFF` before/while routing the PWM. |
| 12 | fixed | `feedback-loop.md`: "Ownership lasts until `0xF1` or a v4 terminal/board reset (including a mode change); it is not tied to being in `RAW1`." | `bp_change_mode()` calls `bp_reset_board_state()`, which calls `smps_stop()` on an SMPS-enabled v4 build. |

## Cross-set changes for the main session

1. **`docs/protocol/binary/commands.md`** - the v4 terminal-reset paragraph
   (around line 212) lists pins to inputs, pull-ups released, supplies off and
   an SMPS loop stopped.  Add "clears the v4 `AUX1` open-drain bit".
2. **`docs/protocol/binary/entering-and-framing.md`** - the v4 terminal-reset
   bullet (around line 186) has the same list.  Add the `AUX1` ODC clear.
3. **`docs/capabilities/auxiliary/aux-pin.md`** - "Reset and persistence"
   (around line 158) says the board reset returns the assignment to `A0` and
   releases every AUX target.  Add that, on v4, it also clears the `AUX1`
   open-drain bit.
4. **`docs/capabilities/auxiliary/wiring.md`** - "Leaving the board" (around
   line 140) repeats the reset/release summary.  Add the v4 `AUX1` ODC clear.
5. **`docs/capabilities/auxiliary/pwm-servo.md`** - the reset table's `#` /
   `0x0F` / mode-change rows (around line 129) list flag, Timer2 and output
   changes.  Optional consistency edit: add the v4 `AUX1` ODC clear.
6. **`docs/protocol/binary/sub-protocols.md`** - already correct for C6
   ("a start clears it ... a read after the new start is `0x00 0x00` until the
   first conversion completes; a stop leaves the last sample unchanged").
   No change.

## Verification performed

- `python test_binary_capabilities.py --case smps` -> PASS: 3584 SMPS targets,
  stale-sample clearing, push-pull restoration, active restart, ownership and
  repeated STOP.
- `python test_core_reset.py` -> PASS: V4 `AUX1` ODC release, Timer2 stop,
  SMPS/ADC ordering, and `m1`/mode changes, scripted `MODE`, `#` and the
  binary return sharing the board reset.
- Static read of `Firmware/capability_smps/smps.c`,
  `Firmware/core_system/base.c`, `Firmware/core_system/proc_menu.c`,
  `Firmware/protocol_binary/binary_io.c` and
  `Firmware/tool_openocd/openocd.c`.
- Link/anchor check over the six SMPS files; no TAB characters; no prose line
  over 80 columns apart from link/reference lines.
- No bench or electrical verification was performed.

## Files changed

- `docs/capabilities/smps/README.md`
- `docs/capabilities/smps/binary.md`
- `docs/capabilities/smps/feedback-loop.md`
- `docs/capabilities/smps/wiring.md`
- `docs/capabilities/smps/exit.md`
- `docs/capabilities/smps/edge-cases.md`
- `docs-sweep/fixes-smps-phase-f.md` (this record)
