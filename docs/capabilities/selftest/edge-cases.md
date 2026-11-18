# Self-test — edge cases & failure modes

Part of the self-test documentation set (`docs/capabilities/selftest/`).
Concrete, non-happy-path behaviour: trigger, symptom, behaviour, recovery.
The checks are defined in [checks.md](checks.md); the wiring is in
[wiring.md](wiring.md); the bytes are in [binary.md](binary.md).

The v3 cases are **not verified on v3**.

## Index

| Group | Cases |
|---|---|
| Setup and entry | `~` outside HiZ; `~` reports SMPS busy; `0x10`/`0x11` while SMPS owns the ADC; unanswered prompts; stopping the run; binary from a mode; sub-protocol |
| Jumpers, rails and pins | missing/loose jumpers (v4, v3, binary); target left connected; VREG/rail failures; stuck pins; voltage-check patterns; one check fails |
| Binary idle loop | `0xFF` as data/exit; terminal expectation; `0x0F` idle; echoed values; stop after count; SMPS running |
| Notes / Related docs | shared notes and links |

## Setup and entry

### `~` outside HiZ

- **Symptom:** `Self-test in HiZ mode only`; the prompt does not change.
- **Behaviour:** the test does not run.
- **Recovery:** switch to HiZ (`m1`) or reset (`#`), then `~`.

### `~` reports the SMPS is busy (unexpected; the guard tripped)

- **Symptom:** `ADC busy: SMPS feedback active; command rejected.` then
  `Syntax error at char 1`.
- **Behaviour:** no setup prompt, no check and no `Found` line.  A normal
  `HiZ>` prompt means the SMPS is not running: both routes back to `HiZ>`,
  `#` and the binary `0x0F` leave, reset the board and stop the feedback
  loop.
- **Recovery:** send `#`; it stops the SMPS and returns to `HiZ>`.  Run `~`
  again.  A power-cycle or the v4 reset button also stops it.  `0xF1` is a
  raw-wire command, so it is not available from the terminal.

### `0x10` / `0x11` while the SMPS owns the ADC

- **Symptom:** the reply is `0x01`; no check runs.
- **Behaviour:** the SMPS keeps running.  `0x10` and `0x11` return the same
  byte, and the test enters its idle loop.
- **Recovery:** leave the idle loop with `0xFF` (reply `0x01`).  From
  `BBIO1`, send `0x05` for raw wire and then `0xF1` (reply `0x01`); `0xF1` in
  `BBIO1` is a pin-state byte.  Or, also from `BBIO1`, send `0x0F` to leave
  the layer; its v4 terminal reset stops the SMPS.  Then rerun from the
  terminal, or re-enter `BBIO1` and send the self-test byte again.

### terminal setup prompt left unanswered

- **Symptom:** `Space to continue` and no further output.
- **Behaviour:** the run waits for a byte; no check has started.
- **Recovery:** send a byte; or reset the board.

### terminal LED prompt left unanswered

- **Symptom:** `Any key to exit` and no count.
- **Behaviour:** the count is printed only after the byte; the regulator and
  LEDs stay on while it waits, so the final reset has not run.
- **Recovery:** send a byte; or reset the board.

### the run must be stopped after it starts

- **Symptom:** no abort key.
- **Behaviour:** the run completes every check, then prints the count.
- **Recovery:** use the v4 reset button or a power-cycle; `#` is not read
  until the run returns.

### binary entered from a non-HiZ mode

- **Symptom:** checks may fail, especially `READ&WRITE`.
- **Behaviour:** the binary entry resets the pins but does not run the
  previous mode's cleanup.
- **Recovery:** leave the terminal from HiZ, enter `BBIO1`, and rerun.

### `0x10` / `0x11` sent inside a sub-protocol

- **Symptom:** not the self-test.
- **Behaviour:** the `0x1x` group is bulk byte transfer there.
- **Recovery:** send `0x00` to the active mode until `BBIO1` appears.  The
  XSVF player ignores `0x00`, so leave it with `0x0F` and enter `BBIO1` again
  with 20 × `0x00`.  Then send the self-test byte.

## Jumpers, rails and pins

### v4 full run: `ADC` jumper missing or loose

- **Symptom:** `ADC(...) FAIL` (for example `ADC(0.00) FAIL`) and the count
  rises by one.
- **Behaviour:** the probe reading is outside the 3.3V window; every other
  check still runs, and `Bus Hi-Z 1` still passes from the internal 3.3V
  source.
- **Recovery:** fit or reseat `ADC`→`+3.3V` and rerun.

### v3 full run: `Vpu`→`+5V` missing or loose

- **Symptom:** `VPU(...) FAIL` and the four `Bus Hi-Z 1` checks fail.
- **Behaviour:** five failures from the same missing reference; `ADC` can
  still pass.
- **Recovery:** fit or reseat the jumper and rerun.

### v3 full run: both jumpers missing

- **Symptom:** `VPU(...) FAIL`, four `Bus Hi-Z 1` failures and
  `ADC(...) FAIL`.
- **Behaviour:** six failures: five from the missing `Vpu` reference and one
  from the missing `ADC` jumper.
- **Recovery:** fit both jumpers and rerun.

### binary `0x11` with a missing jumper

- **Symptom:** v4: count `0x01` when the `ADC` jumper is missing; v3:
  count `0x05` when the `Vpu` jumper is missing, `0x01` when only the `ADC`
  jumper is missing, or `0x06` when both are missing.
- **Behaviour:** the same checks fail as in the terminal run, with no prompt
  and no per-check text.
- **Recovery:** fit the jumpers and rerun; or use `0x10`.

### any target or external driver left on the header

- **Symptom:** checks fail unpredictably, or results change between runs.
- **Behaviour:** the test drives its own pins and switches its rails with the
  target still attached.
- **Recovery:** disconnect every device, probe and external driver, then
  rerun.

### `VREG FAIL`, followed by rail failures

- **Symptom:** `VREG FAIL`, then rail failures.
- **Behaviour:** the regulator-enable line did not read high, so the rails
  may be off and the later voltage checks can fail as a consequence.
- **Recovery:** treat `VREG` as the first cause; check the supply and the
  enable line.

### a rail shorted or unpowered

- **Symptom:** `5V(...) FAIL`, `3.3V(...) FAIL` or `Vusb(...) FAIL`, usually
  near `0.00`.
- **Behaviour:** that rail fails; other rails may still pass.
- **Recovery:** clear the short or check the power source, then rerun.

### v4 3.3V rail off

- **Symptom:** `3.3V(...) FAIL`, `3.3V VPU(...) FAIL`, `ADC(...) FAIL` and
  the four `Bus Hi-Z 1` checks fail.
- **Behaviour:** seven failures from the one rail: the internal checks, the
  probe jumper and the pull-up source.
- **Recovery:** fix the 3.3V rail, then rerun.

### v3 5V rail off with the `Vpu` jumper fitted

- **Symptom:** `5V(...) FAIL`, `VPU(...) FAIL` and the four `Bus Hi-Z 1`
  checks fail.
- **Behaviour:** six failures from the one rail: the rail, its pull-up
  reference and the pulled-up inputs.
- **Recovery:** fix the 5V rail, then rerun.

### an I/O pin stuck low

- **Symptom:** that pin prints `FAIL` in `Bus high`.
- **Behaviour:** the pin cannot be driven high.
- **Recovery:** inspect the pin and header for a short or damage.

### an I/O pin stuck high

- **Symptom:** that pin prints `FAIL` in `Bus Hi-Z 0`.
- **Behaviour:** the pin cannot be driven low.
- **Recovery:** inspect the pin and header for a short or damage.

### an I/O pin does not pull high as an input

- **Symptom:** that pin prints `FAIL` in `Bus Hi-Z 1`.
- **Behaviour:** the pull-up reference or the pin is at fault.
- **Recovery:** check the pull-up source (on v3 the `Vpu`→`+5V` jumper).

### all voltage checks fail, control checks pass

- **Symptom:** every `(v.vv)` line is out of band or `0.00`.
- **Behaviour:** the shared ADC path or the supply is at fault, not each rail
  separately.
- **Recovery:** check the power source and the converter path.

### `READ&WRITE FAIL` while `SCL`, `SDA` and `WP` pass

- **Symptom:** `READ&WRITE FAIL`.
- **Behaviour:** the initial read, the write, a read-back or the restore
  failed; the on-board EEPROM did not return the written inverse or the
  original.  The check also declines if the hardware I2C engine is still
  enabled by another protocol.
- **Recovery:** reset (`#`) and rerun; if it persists, the on-board EEPROM may
  be faulty.

### one check fails

- **Symptom:** `Found 1 errors.`
- **Behaviour:** the wording is always `errors.`; there is one failed check.
- **Recovery:** read the failed line, not the grammar.

## Binary idle loop

### the host sends `0xFF` as data

- **Symptom:** the test exits and replies `0x01`.
- **Behaviour:** `0xFF` is the leave byte, not data; the byte is not echoed.
- **Recovery:** rerun if needed; avoid `0xFF` while the test is idle.

### the host expects the terminal after `0xFF`

- **Symptom:** no `HiZ>` prompt and no `BBIO1` string.
- **Behaviour:** the firmware is back in `BBIO1` silently; the host must
  track the layer.
- **Recovery:** send `0x0F` to leave the layer.

### the host sends `0x0F` while the test is idle

- **Symptom:** the reply is `0x0F` plus the count.
- **Behaviour:** `0x0F` is data here, not a leave.
- **Recovery:** send `0xFF` first, then `0x0F`.

### the host sends `0xFE` with two errors

- **Symptom:** the reply is `0x00`.
- **Behaviour:** the echo is byte + count, wrapping at 256.
- **Recovery:** expected; the reply is not a result.

### the host sends `0x00` with two errors

- **Symptom:** the reply is `0x02`.
- **Behaviour:** the echo equals the count, which can look like a second
  count.
- **Recovery:** read the count once, before the data.

### the host stops after reading the count

- **Symptom:** the board stays in the idle loop.
- **Behaviour:** there is no timeout.
- **Recovery:** send `0xFF` to leave.

### the binary host reads `0x01` while the SMPS is running

- **Symptom:** the SMPS refused the run.
- **Behaviour:** no check ran; this is not one failed check.  The byte alone
  cannot distinguish a refusal from one failed check.
- **Recovery:** leave the idle loop with `0xFF`, then stop the SMPS in raw
  wire (`0x05`, then `0xF1`) or leave with `0x0F` (its v4 reset stops it) and
  rerun.  `~` cannot be run from `BBIO1`.

## Notes

- A `FAIL` means the observed result did not match the expected one.  A
  missing jumper and a faulty element both produce it; the label identifies
  the failed check, not the cause.
- The group headers `Ctrl`, `EEPROM`, `ADC and supply`, `Bus high`,
  `Bus Hi-Z 0` and `Bus Hi-Z 1` never fail on their own.  Only the labelled
  checks below them do.
- The terminal count is a total.  The failed lines are the detail; the
  binary count is only a total, so use the terminal `~` run to locate a
  failure.
- An early failure can cascade: `VREG` before the rails, `PULLUP H/L` before
  `Bus Hi-Z 1`, and a rail before the checks fed from it.  Read the first
  failure first.
- The SMPS refusal is the only case where the binary count is `0x01` with no
  failed check.  In the terminal the guard prints the busy line instead of a
  `Found` line.

## Related docs

- [terminal.md](terminal.md) — the interactive run.
- [checks.md](checks.md) — every check and its failure guidance.
- [binary.md](binary.md) — the count byte and the idle loop.
- [wiring.md](wiring.md) — the jumpers and the post-run state.
