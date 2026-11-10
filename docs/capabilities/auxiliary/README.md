# AUX pin, PWM, servo, frequency and edge wait

**Status:** draft — the header AUX pin, its target assignment, the PWM and
servo generator, the frequency counter, and the binary wait-for-edge command.

The Bus Pirate has an auxiliary digital I/O pin, printed **`AUX`** in the pin
table.  From the prompt you can drive it low or high, read it, or reassign the
`a` / `A` / `@` controls to another header pin (`CS`, and on v4 `AUX1` /
`AUX2`).  The same pin is the output of the PWM and servo generator and the
input of the frequency counter; the binary layer adds a wait-for-edge command
on the selected AUX target.

The firmware names the same target two ways: the `?` help lists assignment
tokens `A0` / `CS` / `A1` / `A2` on v4 (v3 lists `aux` / `CS`), while the
confirmation messages, the `v` table and the header use the pin names `AUX` /
`CS` / `AUX1` / `AUX2`.  This set uses the pin names; the mapping table is
owned by the prompt grammar:
[commands.md](../../generic/commands.md#aux-targets-and-pin-names), and
[aux-pin.md](aux-pin.md) covers the controls.

There is no AUX mode.  The drive/read controls and the generators are
base-layer commands available at every prompt (with the HiZ exceptions in the
files below); the edge wait is reached through the binary layer.

This README is a map, not the manual — the details live in the files below.

## Files

| File | Covers |
|---|---|
| [aux-pin.md](aux-pin.md) | the pin, the target assignment, `a` / `A` / `@`, HiZ, reset behaviour |
| [pwm-servo.md](pwm-servo.md) | `g` PWM and `S` servo: setup, ranges, timer, stopping |
| [frequency.md](frequency.md) | `f`: input, gate, autorange, resolution |
| [edge-wait.md](edge-wait.md) | the binary wait-for-edge capability on the selected pin |
| [wiring.md](wiring.md) | header mapping, logic levels, servo/PWM loads, what not to drive |
| [edge-cases.md](edge-cases.md) | refused commands, prompt traps, floating inputs, recovery |

## The five-minute version

- Wire the pin, load or source first ([wiring.md](wiring.md)); a floating
  input reads arbitrarily and a PWM with nothing attached looks dead.
- `a` drives the selected pin low, `A` high, `@` reads it.  `c` / `C` select
  `AUX` / `CS`; v4 adds `k` / `K` for `AUX1` / `AUX2`.
- `g` starts PWM (frequency in **kHz**, duty in percent); `S` starts a servo
  (position 0–180).  Both are refused in HiZ; neither needs `W` or `P`.
- A bare `g` or `S` **stops** a running generator and prints `PWM disabled`.
- `f` measures frequency on `AUX` and refuses while the terminal generator
  is active.
- Binary: `0x12` / `0x13` set up and clear PWM, `0x16` measures frequency,
  `0x70` waits for an edge (see
  [protocol/binary/commands.md](../../protocol/binary/commands.md)).
- **The assignment rule:** `a` / `A` / `@` and the binary wait follow the
  selected target; PWM, servo and frequency always use the `AUX` pin.

## Availability (summary)

| Feature | v3 | v4 |
|---|---|---|
| `a` / `A` / `@`; `c` / `C` | present (build-only) | present |
| `k` / `K` (`AUX1` / `AUX2`) | — | present |
| `g` / `S` / `f` | present (build-only) | present |
| binary `0x12` / `0x13` / `0x16` / `0x70` | present (build-only) | present |
| header AUX pins | `AUX`, `CS` | `AUX`, `AUX1`, `AUX2`, `CS` |

The v3 column is build-only here and **unverified**.

## Wiring headline

All of these are 3.3 V logic signals on header pins, not supplies.  A PWM
output is meant for a load or a driver input; a servo needs its signal input,
its own supply and a common ground; and an input that can float needs an
external bias.  The pull-up and supply commands (`P` / `p`, `W` / `w`, and `e`
on v4) are owned by
[docs/generic/](../../generic/README.md); see [wiring.md](wiring.md) for what
this family needs.

## Limits & out of scope

- PWM: 1–3999 kHz and 1–99 % in the one-line form; the interactive prompts
  accept 1–4000 kHz and 0–99 %.  The actual output is approximated by whole
  timer ticks ([pwm-servo.md](pwm-servo.md)).
- Servo: integer positions 0–180, mapped to about 1–2 ms pulses in a 20 ms
  frame.
- Frequency: nominal 1 Hz minimum; ±1 count on the 1-second gate up to about
  4.19 MHz, then 256 Hz steps.  The maximum input is a hardware limit and is
  not stated here (unverified).
- Edge wait: binary only, with a 0–65535 ms timeout.
- One generator at a time; PWM and servo share the output.  The optional v4
  SMPS supply PWM shares the same output-compare channel and drives `AUX1`
  ([SMPS](../smps/README.md)).
- The ADC (`d` / `D`), the elapsed timer (`t` / `T`), the self-test and the
  shared prompt grammar are separate topics.

## Related docs

| Page | Why |
|---|---|
| [docs/generic/commands.md](../../generic/commands.md) | command entries for this family and `:N` |
| [docs/generic/usage.md](../../generic/usage.md) | the `v` table and the reset story |
| [docs/protocol/binary/commands.md](../../protocol/binary/commands.md) | the `0x12`–`0x70` byte framing |
| [docs/capabilities/adc/README.md](../adc/README.md) | the analogue probe; not the edge counter |
| [docs/capabilities/selftest/checks.md](../selftest/checks.md) | the `AUX` self-test check |
| [docs/SPEC.md](../../SPEC.md) | the capability rows |
