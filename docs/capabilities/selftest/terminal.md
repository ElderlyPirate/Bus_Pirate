# Self-test — terminal

Part of the self-test documentation set (`docs/capabilities/selftest/`).  This
page covers the interactive `~` self-test: how it is entered, the setup it
prompts for, the output it prints, and how to read a failure.  The per-check
reference is in [checks.md](checks.md); the scriptable variants are in
[binary.md](binary.md).

## Scope

Covers the terminal self-test only.  It does not cover the binary variants,
and it does not reproduce the shared prompt grammar (owned by
`docs/generic/`).

## Entering the test

The self-test is entered with `~` at the prompt:

```
HiZ>~
```

**`~` works in HiZ only.**  If the current mode is anything else, the firmware
prints `Self-test in HiZ mode only` and does nothing:

```
I2C>~
Self-test in HiZ mode only
I2C>
```

Return to HiZ first (`m1` selects HiZ; `#` resets to HiZ) and run `~` again.

The terminal `~` always runs the **full test** — the same test the binary
layer exposes as `0x11`.  There is no short-test form in the terminal; the
short test exists only over the binary layer (see [binary.md](binary.md)).

## Setup

The full test measures board pins through external jumper wires, so it asks
for them before it starts, then waits for a key.  The prompt is
board-specific.

v4:

```
Disconnect any devices
Connect (ADC to +3.3V)
Space to continue
```

v3:

```
Disconnect any devices
Connect (Vpu to +5V) and (ADC to +3.3V)
Space to continue
```

| Board | Pins to jumper | Prompt line |
|---|---|---|
| v4 | `ADC` to `+3.3V` | `Connect (ADC to +3.3V)` |
| v3 | `Vpu` to `+5V`, and `ADC` to `+3.3V` | `Connect (Vpu to +5V) and (ADC to +3.3V)` |

- **Disconnect any target** from the header before running — the test drives
  the Bus Pirate's own pins and supply.
- **No external pull-up resistor is needed**: the test enables the pull-ups
  itself.  On v3 the `Vpu`→`+5V` jumper is the pull-up reference; on v4 the
  on-board 3.3V pull-up source is used.
- The firmware names the pull-up pin `Vpu` in the prompt and `VPU` in the
  check labels; the rails are `+5V` / `+3.3V` in the prompt and `5V` / `3.3V`
  in the labels.  These are the same pin and the same rails.
- After the run the board is back in HiZ with the four I/O pins as inputs,
  the supplies and pull-ups off, and the AUX assignment back to `A0`.

A missing jumper only affects the checks fed from it: a missing `ADC` jumper
fails `ADC`, and on v3 a missing `Vpu`→`+5V` jumper fails `VPU` and the four
`Bus Hi-Z 1` pin checks.  The rest of the test still runs.

## The run (v4, full test)

After `~` the firmware prints the alert, waits for a key, then runs every
check.  The bare `Ctrl` line near the top is a firmware header printed before
the first check.  A run with everything in order looks like this:

```
HiZ>~
Disconnect any devices
Connect (ADC to +3.3V)
Space to continue
Ctrl
AUX OK
MODE LED OK
PULLUP H OK
PULLUP L OK
VREG OK
EEPROM
SCL OK
SDA OK
WP OK
READ&WRITE OK
ADC and supply
Vusb(5.05) OK
5V(5.00) OK
5V VPU(4.98) OK
ADC(3.32) OK
3.3V(3.33) OK
3.3V VPU(3.31) OK
Bus high
MOSI OK
CLK OK
MISO OK
CS OK
Bus Hi-Z 0
MOSI OK
CLK OK
MISO OK
CS OK
Bus Hi-Z 1
MOSI OK
CLK OK
MISO OK
CS OK
MODE, VREG, and USB LEDs should be on!
Any key to exit
Found 0 errors.
HiZ>
```

How to read the output:

- `Space to continue` and `Any key to exit` each wait for one keypress.
- `EEPROM`, `ADC and supply`, `Bus high`, `Bus Hi-Z 0` and `Bus Hi-Z 1` are
  group headers with no `OK`/`FAIL` of their own.
- A measurement check prints its reading in parentheses as `name(v.vv)`, for
  example `5V(5.00)`.
- Each check prints `OK` or `FAIL`, and the run ends with
  `Found <n> errors.` — `0` means no check failed.

Because `~` is the full test, it includes the external-jumper checks: `ADC`
on v4, and `ADC`, `VPU` and the four `Bus Hi-Z 1` pins on v3.

## Reading a failure

A `FAIL` increments the error count, but the count is reported only as a
total, so the failed lines are what to read.  The per-check causes are the
`If it fails` column in [checks.md](checks.md).

| Symptom | What it means |
|---|---|
| one or more jumper checks fail (`ADC`; on v3 also `VPU` and the four `Bus Hi-Z 1` pins) | the matching jumper is missing or not seated |
| a rail line fails (`5V`, `3.3V`, `Vusb`) | supply fault or a short |
| several unrelated checks fail, or results change between runs | a target is still connected, or the board is damaged |

The acceptance window for a voltage check is a fixed band of about ±0.5 V
around the nominal rail value.  The printed voltage is the measured value;
compare it with the nominal to judge how far off the rail is.  The concrete
failure/recovery cases are in [edge-cases.md](edge-cases.md).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| test coverage | no EEPROM, `Vusb` or internal pull-up rail checks | adds EEPROM, `Vusb` and the internal pull-up rails |
| supply lines | `5V`, `VPU` (jumper), `3.3V`, `ADC` (jumper) | `Vusb`, `5V`, `5V VPU`, `ADC` (jumper), `3.3V`, `3.3V VPU` |
| jumper prompt | `Connect (Vpu to +5V) and (ADC to +3.3V)` | `Connect (ADC to +3.3V)` |
| LED prompt | `MODE and VREG LEDs should be on!` | `MODE, VREG, and USB LEDs should be on!` |

The v3 column is **not verified** here.

## Related docs

- [checks.md](checks.md) — every check and what it verifies.
- [binary.md](binary.md) — the scriptable `0x10` / `0x11` variants.
- [edge-cases.md](edge-cases.md) — failure modes and recovery.
- [docs/generic/](../../generic/README.md) — the shared prompt; its
  "Idle mode (HiZ)" section covers the mode the test requires.
