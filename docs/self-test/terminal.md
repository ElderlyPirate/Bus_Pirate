# Self-test — terminal

Part of the self-test documentation set (`docs/self-test/`).  This page covers
the interactive `~` self-test: how it is entered, the jumper setup it prompts
for, the output it prints, and how to read a failure.  The per-check reference
is in [checks.md](checks.md); the scriptable variants are in
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

## Jumper setup

The full test measures board pins through external jumper wires, so it
prompts for them before it starts.  The prompt is board-specific:

| Board | Jumper(s) to place |
|---|---|
| v4 | `ADC` pin to `+3.3V` |
| v3 | `Vpu` pin to `+5V`, and `ADC` pin to `+3.3V` |

- **No external pull-up resistor is needed**: the test enables the pull-ups
  itself.  On v3 the `Vpu`→`+5V` jumper is the pull-up reference; on v4 the
  on-board 3.3V pull-up source is used.
- **Disconnect any target** from the header before running — the test drives
  the Bus Pirate's own pins and supply.
- The firmware names the pull-up pin `Vpu` in the prompt and `VPU` in the
  check labels; the rails are `+5V` / `+3.3V` in the prompt and `5V` / `3.3V`
  in the labels.  These are the same pin and the same rails.

If a jumper is absent, only the checks that use it fail (see
[checks.md](checks.md) and [edge-cases.md](edge-cases.md)); the rest of the
test still runs.

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
```

How to read the output:

- `Space to continue` and `Any key to exit` each wait for one keypress.
- `EEPROM`, `ADC and supply`, `Bus high`, `Bus Hi-Z 0` and `Bus Hi-Z 1` are
  group headers with no `OK`/`FAIL` of their own.
- A measurement check prints its reading in parentheses as `name(v.vv)`, for
  example `5V(5.00)`.
- Each check prints `OK` or `FAIL`, and the run ends with
  `Found <n> errors.` — `0` means no check failed.

Because `~` is the full test, it includes the jumper checks `ADC` and
`Bus Hi-Z 1`.  Without the jumper, `ADC` prints `FAIL`.

## Reading a failure

A `FAIL` increments the error count, but the count is reported only as a
total, so the failed lines are what to read:

| Symptom | Likely cause |
|---|---|
| a measurement line prints `FAIL` | the rail or probe is outside the acceptance window |
| `ADC FAIL` on a full run | the `ADC` jumper is missing or not seated |
| `VPU FAIL` on a v3 full run | the `Vpu`→`+5V` jumper is missing |
| a rail line fails (`5V`, `3.3V`, `Vusb`) | supply fault or a short |
| a `Bus high` pin fails | that pin is stuck low |
| a `Bus Hi-Z 1` pin fails | the pin does not pull high |

The acceptance window for a voltage check is a fixed band of about ±0.5 V
around the nominal rail value.  The printed voltage is the measured value;
compare it with the nominal to judge how far off the rail is.  The concrete
failure/recovery cases are in [edge-cases.md](edge-cases.md).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| test coverage | no EEPROM, `Vusb` or internal VPU checks | adds EEPROM and `Vusb` |
| supply lines | `5V`, `VPU` (jumper), `3.3V`, `ADC` (jumper) | `Vusb`, `5V`, `5V VPU`, `ADC` (jumper), `3.3V`, `3.3V VPU` |
| jumper prompt | `Connect (Vpu to +5V) and (ADC to +3.3V)` | `Connect (ADC to +3.3V)` |
| LED prompt | `MODE and VREG LEDs should be on!` | `MODE, VREG, and USB LEDs should be on!` |

The v3 column is **not verified** here.

## Related docs

- [checks.md](checks.md) — every check and what it verifies.
- [binary.md](binary.md) — the scriptable `0x10` / `0x11` variants.
- [edge-cases.md](edge-cases.md) — failure modes and recovery.
- `docs/hiz/` — HiZ, the mode the test requires.
- `docs/generic/` — the shared prompt.