# AUX pin, PWM, servo, frequency and edge wait

**Status:** documented for the v3 and v4 builds.  The v3 features are present
in the v3 build; their behaviour on v3 hardware is `unverified`.

This set covers the header pins the firmware calls **`AUX`**, **`AUX1`** and
**`AUX2`** (v4) and **`CS`**, the terminal controls that select and drive
them, the PWM and servo generator, the frequency counter, the binary
wait-for-edge, and the AUX-side binary commands.

The firmware names the same target two ways: the `?` help lists assignment
tokens `A0` / `CS` / `A1` / `A2` (v3: `aux` / `CS`), while the confirmation
message and the `v` table use the pin names `AUX` / `CS` / `AUX1` / `AUX2`.
This set uses the pin names; the token-to-pin table is owned by
[the prompt grammar](../../generic/commands.md#aux-targets-and-pin-names).

There is no AUX mode.  The drive/read controls, the generators and the counter
are base-layer commands available at every prompt (with the HiZ exceptions in
the topic files).  The edge wait is reached through the binary layer.

Read in this order: [wiring.md](wiring.md), [aux-pin.md](aux-pin.md), then
[pwm-servo.md](pwm-servo.md) or [frequency.md](frequency.md), and finish with
the binary pages ([binary.md](binary.md), [edge-wait.md](edge-wait.md)) and
[edge-cases.md](edge-cases.md).

This README is a map, not the manual; details live in the files below.

## Files

| File | Covers |
|---|---|
| [aux-pin.md](aux-pin.md) | the pins, target selection, `a` / `A` / `@`, HiZ, reset behaviour |
| [pwm-servo.md](pwm-servo.md) | `g` PWM and `S` servo: setup, ranges, timing, stopping, SMPS refusal |
| [frequency.md](frequency.md) | `f`: input, gate, autorange, display, resolution |
| [edge-wait.md](edge-wait.md) | the `0x70` wait: pin, payload, reply, timeout, direction |
| [binary.md](binary.md) | the AUX binary surface: `0x09`, `0x12`, `0x13`, `0x16`, `0x70` |
| [wiring.md](wiring.md) | header mapping, logic levels, loads, supplies, what not to drive |
| [edge-cases.md](edge-cases.md) | refused commands, prompt traps, quantisation, recovery |

## Command map

| Input | Action | Detail |
|---|---|---|
| `a` / `A` / `@` | drive the selected target low / high, or read it | [aux-pin.md](aux-pin.md) |
| `c` / `C` / `k` / `K` | select `AUX` / `CS` / `AUX1` / `AUX2` (v4) | [aux-pin.md](aux-pin.md) |
| `g` / `S` | PWM / servo on `AUX` | [pwm-servo.md](pwm-servo.md) |
| `f` | frequency counter on `AUX` | [frequency.md](frequency.md) |
| `0x12` / `0x13` | binary PWM setup / clear | [binary.md](binary.md) |
| `0x16` / `0x70` | binary frequency count / edge wait | [binary.md](binary.md), [edge-wait.md](edge-wait.md) |
| `0x09` | I2C binary `AUX` / `CS` control | [binary.md](binary.md) |

## The five-minute version

- Wire the pin, load or source first ([wiring.md](wiring.md)); a floating
  input reads arbitrarily, and a PWM or servo with nothing attached looks
  dead.  No Bus Pirate supply or pull-up is needed for the logic itself.
- `a` drives the selected target low, `A` high, `@` makes it an input and
  reads it.  `c` selects `AUX` (`A0`), `C` selects `CS`; v4 adds `k` (`A1`,
  `AUX1`) and `K` (`A2`, `AUX2`).
- `g<kHz> <duty>` starts PWM (1–4000 kHz, 0–99 %); `S<position>` starts a
  servo (0–180).  Both are refused in HiZ and need no `W` or `P`.
- A bare `g` or `S` stops a running generator and prints `PWM disabled`.
  `f` refuses while a generator is active.
- `f` measures frequency on `AUX` only: 1 Hz nominal minimum; 1 Hz gate
  resolution from 4 kHz to just below 4.194304 MHz, period averaging below
  4 kHz, then 256 Hz steps nominally at or above 4.194304 MHz.
- Binary: `0x12` / `0x13` set up and clear PWM, `0x16` counts frequency,
  `0x70` waits for an edge.  The `0x09` AUX/CS control is part of the I2C
  binary mode, not BBIO1.

## Availability (summary)

| Feature | v3 | v4 |
|---|---|---|
| `a` / `A` / `@`, `c` / `C`, `g` / `S` / `f` | present | present |
| binary `0x12` / `0x13` / `0x16` / `0x70`; I2C `0x09` | present | present |
| `k` / `K` (`AUX1` / `AUX2`) | — | present |
| header AUX pins | `AUX`, `CS` | `AUX`, `AUX1`, `AUX2`, `CS` |
| SMPS owns the output channel | — | optional build; off in the default image |

## Wiring headline

The outputs are 3.3 V logic signals on header pins, not supplies.  A PWM
output is meant for a load or a driver input; a servo needs its signal input,
its own supply and a common ground; an input that can float needs an external
bias.  The v4 header I/O is 5 V class; treat 5.5 V as the maximum (the exact
rating is not documented — see [wiring.md](wiring.md)).  A 5 V logic input
may not accept a 3.3 V high.  `P` / `p`, `W` / `w` and (v4) `e` belong to
[docs/generic/](../../generic/README.md).

## Limits & out of scope

- PWM: 1–4000 kHz and 0–99 % in both the one-line and interactive forms; the
  output is quantised to whole timer ticks and the duty is at or below the
  request.  The terminal `g` cannot request 100 %.
- Servo: integer positions 0–180, mapped to about 1–2 ms pulses in a 20 ms
  frame; no endpoint calibration and no fractional degree.
- Frequency: nominal 1 Hz minimum; 1 Hz gate resolution from 4 kHz to just
  below 4.194304 MHz, period averaging below 4 kHz, then 256 Hz steps
  nominally at or above 4.194304 MHz.  The maximum input rate is a hardware
  limit and is not stated here (unverified).
- Edge wait: binary only, 0–65535 ms, one selected pin per wait.
- One generator at a time: PWM and servo share the output.  BASIC `FREQ` /
  `DUTY` drives the same output in Hz.  On the optional v4 SMPS build, the
  supply PWM owns the channel and terminal `g` / `S` are refused.
- The ADC (`d` / `D`), the elapsed timer (`t` / `T`), the self-test and the
  shared prompt grammar are separate topics.

## See also

The prompt grammar, `v` table and idle state are in
[docs/generic/commands.md](../../generic/commands.md) and
[docs/generic/usage.md](../../generic/usage.md); the BBIO1 map and result bytes
are in [docs/protocol/binary/commands.md](../../protocol/binary/commands.md),
and the I2C `0x09` command is in
[docs/protocol/i2c/binary.md](../../protocol/i2c/binary.md#aux-0x09).  Related:
[ADC probe](../adc/README.md), [SMPS supply PWM](../smps/README.md),
[`AUX` self-test check](../selftest/checks.md),
[DIO AUX controls](../../protocol/dio/aux-pin.md),
[BASIC `FREQ` / `DUTY`](../../tools/basic/board-pins.md) and
[docs/SPEC.md](../../SPEC.md).
