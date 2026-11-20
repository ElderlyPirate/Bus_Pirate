# Self-test — terminal entry (`~`)

Part of the self-test documentation set (`docs/capabilities/selftest/`).  The
interactive `~` self-test: entry, the setup prompt, the full run, and how to
read its result.  The jumpers are in [wiring.md](wiring.md); the per-check
reference is [checks.md](checks.md); the scriptable bytes are in
[binary.md](binary.md).

## Entry

`~` at the prompt starts the self-test.  It is accepted in HiZ only:

```
HiZ>~
```

In any other mode it prints `Self-test in HiZ mode only` and takes no action;
the prompt does not change:

```
I2C>~
Self-test in HiZ mode only
I2C>
```

Return to HiZ (`m1` or `#`) and run `~` again.  `~` is a single-character
command: it starts as soon as the character arrives, without Enter.

The terminal `~` is always the **full test**, including the jumper checks.
The short pass exists only over the binary layer (`0x10`); see
[binary.md](binary.md).

## Setup

The run measures rails through header pins, so it asks for the jumpers before
it starts and waits for a byte.  The prompt is board-specific; the wiring is
in [wiring.md](wiring.md).

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

The prompt says Space, but any byte starts the run.  The byte is consumed by
the self-test, not by the terminal.  A byte already queued, such as a CR sent
after `~`, answers the prompt too.

The prompt writes `Vpu` and `+5V`; the check labels for the same pin and rail
are `VPU` and `5V`.  The mapping is in [checks.md](checks.md#names).

**No external pull-up resistor is needed**; the test enables the board's
pull-up network itself.  **No external supply is needed**; the test switches
the board's rails on and off itself.  See [wiring.md](wiring.md) for the
pull-up and supply details and the post-run state.

## The run (v4, full test)

After the start key the firmware prints `Ctrl` and walks every block.  A run
with everything in order:

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

The voltages are example readings; a real run prints its own.

How to read the lines:

| Line | Meaning |
|---|---|
| `Ctrl` | group header for the control checks; no result |
| `EEPROM`, `ADC and supply`, `Bus high`, `Bus Hi-Z 0`, `Bus Hi-Z 1` | group headers; no result |
| `name OK` | the check passed |
| `name FAIL` | the check failed; the error count goes up by one |
| `name(v.vv) OK` / `name(v.vv) FAIL` | a measurement check, with its reading |
| `MODE, VREG, and USB LEDs should be on!` | the manual LED prompt; not a check |
| `Any key to exit` | waits for a byte before the count |
| `Found <n> errors.` | the run's total; `0` is clean |

The count is the number of failed checks, not the number of checks.  The
firmware always prints `errors.`, so a single failure reads
`Found 1 errors.`.

The terminal form has no short pass.  To skip the jumper checks, use the
binary `0x10` pass instead.

## The run (v3, full test)

The v3 run has the same shape without the EEPROM block.  The order is:

1. `Ctrl`: `AUX`, `MODE LED`, `PULLUP H`, `PULLUP L`, `VREG`.
2. `ADC and supply`: `5V`, `VPU`, `3.3V`, `ADC`.
3. `Bus high`, `Bus Hi-Z 0`, `Bus Hi-Z 1`: `MOSI`, `CLK`, `MISO`, `CS` in
   each group.
4. `MODE and VREG LEDs should be on!`, `Any key to exit`, then
   `Found <n> errors.`

Each measurement line prints its own reading, so the volts differ from the v4
example.  The v3 column is **not verified on v3**.

## Reading a failure

A `FAIL` adds one to the total; the failed lines identify what failed.  The
per-check causes are the `If it fails` column in [checks.md](checks.md).  The
cross-cutting cases:

| Symptom | Meaning | Next step |
|---|---|---|
| `ADC(...) FAIL` alone | the `ADC` jumper is missing or loose, or the probe input is faulty | fit/reseat the jumper; if `3.3V(...)` also fails, suspect the switched 3.3 V output |
| `VPU(...) FAIL` and four `Bus Hi-Z 1` failures on v3 | the `Vpu`→`+5V` jumper is missing or loose | fit/reseat it; both symptoms share the reference |
| `VREG FAIL`, then rail failures | the regulator-enable line did not go high; the rails may be off | treat `VREG` as the first cause |
| all voltage checks fail, control checks pass | the shared ADC path or the supply is at fault, not each rail separately | check the power source and the converter path |
| one pin fails in one group | that pin is stuck or loaded | disconnect any target; inspect the pin and header |
| results differ between runs | a target is still connected, or a jumper is loose | disconnect/reseat and rerun |
| every check passes but an LED is dark at the prompt | the LED or its drive is faulty; this is not counted | note it; the line checks passed |

The voltage checks accept a wide band of about ±0.5 V around the nominal
rail, so they catch a dead or badly wrong rail, not a small calibration
error.  See the voltage checks in
[checks.md](checks.md#voltage-checks).

## When the run does not start

On a v4 build with SMPS support, the SMPS feedback loop owns the ADC and the
self-test refuses to start.  The refusal is a binary-layer condition: the
SMPS can be left running between raw wire and `BBIO1`, where `0x10` / `0x11`
return a refusal count of `0x01`.  The full sequence and the terminal
recovery are in
[checks.md](checks.md#when-the-run-does-not-start).

A terminal `~` at `HiZ>` does not normally meet it.  `~` runs only at HiZ,
and both routes back to `HiZ>` reset the board and stop the SMPS: the
terminal `#` command, and the binary `0x0F` leave whose v4 terminal reset
stops the feedback loop.  If a `~` still refuses, the screen shows:

```
HiZ>~
ADC busy: SMPS feedback active; command rejected.
Syntax error at char 1
HiZ>
```

No check runs and no `Found` line is printed.  The `Syntax error` line is the
dispatcher's response to the flagged command error, not a bad `~`.  Send `#`
to reset the board and stop the SMPS, then run `~` again.  A power-cycle or
the v4 reset button also stops it.  `0xF1` is a raw-wire command, so a
terminal-only user cannot send it from `HiZ>`.

The default image has SMPS support off, so this refusal cannot occur there;
v3 has no SMPS support.  The optional build is in
[capabilities/smps/README.md](../smps/README.md).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| control checks | `AUX`, `MODE LED`, `PULLUP H/L`, `VREG` | same |
| on-board EEPROM block | — | `SCL`, `SDA`, `WP`, `READ&WRITE` |
| supply block, in order | `5V`, `VPU`, `3.3V`, `ADC` | `Vusb`, `5V`, `5V VPU`, `ADC`, `3.3V`, `3.3V VPU` |
| setup jumpers | `Vpu`→`+5V`, `ADC`→`+3.3V` | `ADC`→`+3.3V` |
| LED prompt | `MODE and VREG LEDs should be on!` | `MODE, VREG, and USB LEDs should be on!` |
| checks, full / short | 21 / 15 | 27 / 22 |

The full table is in [checks.md](checks.md).  The v3 column is **not verified
on v3**.

## Limits

- The run has no abort key.  Once the setup key is sent it completes all of
  its checks and prints the count; to stop it, use the v4 reset button or a
  power-cycle.  `#` is not read until the run returns.
- The setup and LED prompts each read one byte.  A terminal host must answer
  both; the binary form has no prompts and is the one to script.
- The terminal form is always full; there is no short form and no per-check
  selection.

## Related docs

- [wiring.md](wiring.md) — jumpers, pull-ups, supply, post-run state.
- [checks.md](checks.md) — every check and its failure guidance.
- [binary.md](binary.md) — the `0x10` / `0x11` variants.
- [edge-cases.md](edge-cases.md) — failure and recovery cases.
- [generic/commands.md](../../generic/commands.md) — `~` in the shared prompt
  grammar.
