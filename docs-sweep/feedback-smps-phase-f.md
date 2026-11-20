# SMPS — Phase F delta audit

**Scope:** the C6 sample-clear change and the C2/ODC/reset changes in the six
`docs/capabilities/smps/` pages, checked against `Firmware/` at `9b179cb5`.
Unrelated SMPS content was not reopened.

**Verdict: No.** The SMPS set still teaches the pre-repair stale-sample and
power-cycle behaviour; the C6 and C2 deltas are not applied consistently.

## Findings

### 1. [blocker] `feedback-loop.md` still says a start does not clear the sample

**Location:** `docs/capabilities/smps/feedback-loop.md:82-86`

> - A start does not clear the stored sample.  The first read after a new start
>   can still be from the previous run.
> - There is no freshness flag.  Discard the first read after a start.  If you
>   need evidence of a new conversion, poll until the value changes; a stable
>   output can legitimately repeat, so that is a heuristic, not a guarantee.

**Why:** `Firmware/capability_smps/smps.c:63` sets
`smps_state.voltage_reading = 0` in `smps_start()`. A start clears the sample;
before the first new conversion `0xF0` reads zero. It cannot return the previous
run's sample.

**Fix:** Replace those two bullets with:

> A start clears the stored sample to zero.  Before the first new conversion,
> `0xF0` returns `0x00 0x00`; after a conversion it reports the new sample.
> There is no validity flag: a real zero sample also reads zero, so a zero first
> read is not proof that a conversion has happened.

### 2. [major] `binary.md` repeats the same stale-sample claim in the command reference

**Location:** `docs/capabilities/smps/binary.md:86`, `:118-120`, `:196-198`,
`:241-242`, and `:105-106`

> `| 0xF0 after a new start | possibly the previous run's sample | ... | no freshness flag; discard the first read or poll until it changes |`
>
> “The first `0xF0` after a start can return zero, the previous run's sample, or
> an already-fresh value.  There is no freshness flag; treat that first read as
> stale and use a later read.”
>
> “The stored sample is not cleared, so a `0xF0` read immediately after the new
> start can still return the previous sample; the freshness rule above applies.”
>
> “`0xF0` never triggers a conversion and never clears the stored sample.  The
> first read after a start can be stale; discard it or poll until it changes.”

The worked-session comment at `:106` also says “the pre-start value”.

**Why:** The start clears the sample (`smps.c:63`). The first read is zero until
the first new conversion; a fresh sample is possible, but a previous-run sample
is not.

**Fix:** In the reply table, make the new-start row “`0x00 0x00` until the first
conversion; then the new sample”. In the prose, say “zero if no conversion has
completed, otherwise the new sample; never the previous run's sample”. In
“Starting again”, say the start clears the sample and the immediate read is
zero. In Limits, say the first read is zero until the first conversion. Change
the worked-session comment to “the start cleared it; no conversion yet”. The
Python comment `first read: discard it` can become `first read: zero until the
first conversion`.

### 3. [major] `README.md` still tells the reader to expect the previous sample

**Location:** `docs/capabilities/smps/README.md:77-78`, `:99`, `:133-134`

> “The first read after a start can still be from the previous run: discard it
> or poll until it changes.”
>
> `| read, first sample (discard) | 0xF0 | two bytes, high byte first |`
>
> “`0xF0` can return the previous run's sample after a start; there is no
> freshness flag.”

**Why:** A start clears the sample; the first read is zero before the first
conversion. The “previous run” advice is no longer true.

**Fix:** Change the Read bullet and the limits row to “A start clears the stored
sample; before the first new conversion `0xF0` reads `0x00 0x00`.” Change the
table row to “read before the first conversion (zero)” and keep “read after a
conversion”. If the no-validity-flag nuance is kept, say a real zero sample also
reads zero.

### 4. [major] `edge-cases.md` “Starting again” still says the old sample survives

**Location:** `docs/capabilities/smps/edge-cases.md:194-203`

> “A `0xF0` read immediately after the new start can still return the previous
> sample.”
>
> “the stored sample is not cleared, and there is no freshness flag.”
>
> “Discard the first read after the new start, or poll until the value changes.”

**Why:** `smps_start()` clears the sample, including on a re-start while the
loop is running.

**Fix:** Say the re-start clears the sample; the immediate `0xF0` reads
`0x00 0x00` until the first new conversion. Remove the discard/poll workaround;
a zero sample can also be a real reading.

### 5. [major] `edge-cases.md` says only a power cycle/hardware reset clears the sample

**Location:** `docs/capabilities/smps/edge-cases.md:61-63`

> “stop turns the converter off and leaves the stored sample unchanged; `0xF0`
> never clears it.  Only a power cycle or a hardware reset reinitializes it.”

**Why:** A new start also clears it (`smps.c:63`). Stop alone leaves the last
sample, which is the useful distinction; the absolute “only” is wrong.

**Fix:** “Stop leaves the stored sample unchanged. A new start clears it; a
power cycle or hardware reset also reinitializes it.” Keep the “reading after
stop repeats the last value” warning.

### 6. [major] The “fresh power-up, and after stop” example shows a sample that cannot exist

**Location:** `docs/capabilities/smps/binary.md:124-132`

```
host -> 0xF0
host <- 0x00 0x00        no sample stored since power-up
host -> 0xF2 0x00        start
host <- 0x01
host -> 0xF1             stop
host <- 0x01
host -> 0xF0
host <- 0x01 0xD3        the last sample from the stopped run
```

**Why:** The start clears the sample. No conversion is shown between the start
and the stop, so the stopped run's last sample would be `0x00 0x00`, not
`0x01 0xD3`. The example contradicts C6.

**Fix:** Add a conversion read before the stop:

```
host -> 0xF2 0x00        start
host <- 0x01
host -> 0xF0             read after a conversion
host <- 0x01 0xD3        example sample
host -> 0xF1             stop
host <- 0x01
host -> 0xF0
host <- 0x01 0xD3        the last sample from the stopped run
```

Alternatively, show the stopped read as `0x00 0x00` and say no conversion ran.

### 7. [major] `README.md` still says a power cycle/hardware reset is needed for `AUX1` open drain

**Location:** `docs/capabilities/smps/README.md:45-48`

> “If a previous mode left `AUX1` open drain, a logic high is a released pin:
> the add-on needs its own pull-up, or the Bus Pirate needs a power cycle or
> hardware reset before the start.”

**Why:** `smps_start()` clears the ODC bit (`smps.c:78`), and
`bp_reset_board_state()` clears it on V4 (`base.c:188`). A start or a terminal/
board reset fixes it; a power cycle is not required.

**Fix:** “The SMPS start clears the `AUX1` open-drain bit, and a v4 terminal or
board reset (`#`, `0x0F`, or a mode change) also clears it. If a binary path
drives `AUX1` before the start without a terminal reset, a leftover open-drain
high can still need an add-on pull-up.” Keep the pull-up warning only for that
pre-start binary case.

### 8. [major] `wiring.md` says the start and terminal reset do not clear ODC

**Location:** `docs/capabilities/smps/wiring.md:72-77` and `:113-115`

> “the SMPS start does not clear the open-drain bit.  A v4 terminal reset (`#` or
> `0x0F`) does not clear it either; a power cycle or hardware reset does.  After
> OpenOCD open-drain use, power-cycle the Bus Pirate before an SMPS start, or fit
> a pull-up …”
>
> “the SMPS start does not clear an open-drain setting, so a mode that left the
> pin open drain also needs a pull-up”

**Why:** Both statements are reversed by the repair. `smps_start()` writes
`BP_AUX1_ODC = OFF` (`smps.c:78`); `bp_reset_board_state()` also clears it
(`base.c:188`). `#`, `0x0F` and a mode change reach that reset.

**Fix:** Say the start clears the ODC bit and the PWM is push-pull; a v4
terminal/board reset also clears it; no power cycle is needed. Keep a pull-up
warning only for a binary path that drives `AUX1` before the start without a
terminal reset. In the signal-levels list, replace “the SMPS start does not
clear an open-drain setting” with “the SMPS start clears the open-drain bit”.

### 9. [major] `edge-cases.md` still presents the old open-drain/power-cycle failure

**Location:** `docs/capabilities/smps/edge-cases.md:129-142`

> “use the v4 OpenOCD path in open-drain mode, then start the SMPS without a
> power cycle.”
>
> “the SMPS start does not clear it.  A v4 terminal reset does not clear it
> either; a power cycle or hardware reset does.”
>
> “Treat the pin as open drain until a power cycle.”

**Why:** The start clears the ODC bit, and `#`/`0x0F`/`m` reset clears it. The
described symptom after a start is no longer true; the power-cycle recovery is
wrong.

**Fix:** Rewrite the entry as “`AUX1` is open drain before a binary SMPS start”:
the trigger is reaching SMPS through a binary path without a terminal reset and
driving `AUX1` before the start; the start clears the ODC bit; `#`, `0x0F` and a
mode change also clear it; no power cycle is needed. Keep the pull-up warning
for the pre-start binary path (or an add-on input that needs a pull-up).

### 10. [major] `exit.md` reset rows do not record the ODC clear

**Location:** `docs/capabilities/smps/exit.md:108-116`

> `| Path | Where sent | Stops PWM and ADC | Releases AUX1 | Stored sample |`
>
> `| 0x0F | BBIO1 | yes on v4 (terminal reset) | yes | kept |`
> `| # | terminal | yes on v4 (terminal reset) | yes | kept |`
> `| m (mode change) | terminal | yes on v4 (board reset) | yes | kept |`
> `| hardware reset or power cycle | Bus Pirate | yes | yes | cleared at startup |`

**Why:** `bp_reset_board_state()` clears `BP_AUX1_ODC` (`base.c:188`); `#`,
`0x0F` and `m` all reach it. The table currently says nothing about ODC, so a
reader cannot see that the reset path fixes the open-drain state. The stop table
at `:35` also omits that `0xF1` leaves ODC unchanged.

**Fix:** Add an `AUX1 ODC` column: `0x0F`, `#`, `m`, and hardware reset/power
cycle = cleared on v4; `0xF1`, `0x00`, and `0xF1` in `BBIO1` = unchanged. Add
“clears the `AUX1` open-drain bit” to the reset descriptions at `:80`, `:91`,
`:95` and `:102`. Add `AUX1 ODC | unchanged` to the stop table row at `:35`.

### 11. [minor] The start sequence does not describe the two new start-side clears

**Location:** `docs/capabilities/smps/feedback-loop.md:26-31`

> “3. Computes the duty compare value and the feedback target from the request.”
>
> “4. Drives `AUX1` low and routes the output-compare channel to the pin, with a
> period register of `0x7F` and edge-aligned PWM.”

**Why:** `smps_start()` also clears the stored sample (`smps.c:63`) and the
`AUX1` ODC bit (`smps.c:78`). The page that owns “what a start configures” does
not mention either, so the stale sample/ODC statements elsewhere are the only
description a reader gets.

**Fix:** In step 1 (or a new step 2), say the start clears the stored sample to
zero. In step 4, say it clears the `AUX1` open-drain bit before driving the pin
low and routing the PWM. That makes the start description match the firmware.

### 12. [minor] The ownership sentence omits the mode-change reset path

**Location:** `docs/capabilities/smps/feedback-loop.md:33`

> “Ownership lasts until `0xF1` or a v4 terminal reset; it is not tied to being
> in `RAW1`.”

**Why:** A mode change (`m`) calls `bp_reset_board_state()` and stops the SMPS
loop too. The exit table lists `m`, but the ownership sentence says only
terminal reset.

**Fix:** “Ownership lasts until `0xF1` or a v4 terminal/board reset (including a
mode change); it is not tied to being in `RAW1`.”

## Cross-set list

These are outside the SMPS set but carry the C2 reset/ODC fact. They do not
currently make a false “power cycle only” claim; they simply omit the clear.

- **`docs/protocol/binary/commands.md:211-215`** — the v4 terminal-reset
  paragraph lists “pins to inputs, pull-ups released, supplies off, an SMPS
  feedback loop stopped …” but not “clears the V4 `AUX1` open-drain bit.” Add it.
- **`docs/protocol/binary/entering-and-framing.md:186-190`** — same reset bullet;
  add the ODC clear.
- **`docs/capabilities/auxiliary/aux-pin.md:158-161`** — “The board reset returns
  the assignment to `A0` and releases every AUX target” does not mention the V4
  `AUX1` ODC clear; add it.
- **`docs/capabilities/auxiliary/wiring.md:140-145`** — “A board reset returns the
  assignment to `AUX` (`A0`) and releases every AUX target” also omits the ODC
  clear; add it.
- **`docs/capabilities/auxiliary/pwm-servo.md:129-134`** — the reset table’s
  `#`/`0x0F` rows list flag/Timer2/output changes but not the V4 `AUX1` ODC
  clear; optional consistency edit.
- **`docs/protocol/binary/sub-protocols.md:290-296`** — already correct for C6:
  “a start clears it … a read after the new start is `0x00 0x00` until the first
  conversion completes; a stop leaves the last sample unchanged.” No change.

## Presentation check

- No TAB characters in the six SMPS files.
- Captures in `binary.md` and `exit.md` still align at column 26.
- No menu/option-list bullet chains were introduced; the changed material is
  prose and existing tables.
- No prose line exceeds 80 columns. `exit.md:142` is an 81-column link line and
  `wiring.md:191` is a 110-column reference-link definition; neither is a prose
  wrap.
- No dates, bench prose, pass counts, “captured on” provenance or internal log
  names in the changed pages; only the existing `unverified` / “not verified on
  v3” caveats.

**Finding count:** 12 (1 blocker, 9 major, 2 minor).  Cross-set: 5 pages plus
one verified-correct page.
