# Self-test — check reference

Part of the self-test documentation set (`docs/capabilities/selftest/`).  Every
check the self-test performs, what it drives or reads, the expected result, the
jumper it needs, which pass runs it, and how the boards differ.  The terminal
run is in [terminal.md](terminal.md); the bytes are in [binary.md](binary.md);
the wiring is in [wiring.md](wiring.md).

## Names

The firmware uses two forms for the pull-up pin: the prompt writes `Vpu` and
the check labels write `VPU`.  The prompt writes the rails as `+5V` / `+3.3V`;
the labels drop the plus sign.  These are the same pin and the same rails.

| Prompt form | Check label | Meaning |
|---|---|---|
| `Vpu` | `VPU` | the pull-up voltage pin |
| `+5V` | `5V` | the 5V rail; `5V VPU` is its pull-up source (v4) |
| `+3.3V` | `3.3V` | the 3.3V rail; `3.3V VPU` is its pull-up source (v4) |
| — | `Vusb` | the 5V arriving from USB (v4) |
| `MODE` (LED prompt) | `MODE LED` | the MODE LED; the check reads its drive line |

The `?` help lists the command as `Selftest`; this set writes self-test in
prose.  This set calls the two binary passes **short** (`0x10`) and **full**
(`0x11`).  The firmware prints no variant name, because both binary passes run
with text suppressed.

## What the checks cover

A run walks four blocks and prints each as it goes:

| Block | Header printed | Checks in it |
|---|---|---|
| control lines | `Ctrl` | `AUX`, `MODE LED`, `PULLUP H`, `PULLUP L`, `VREG` |
| on-board EEPROM | `EEPROM` | `SCL`, `SDA`, `WP`, `READ&WRITE` (v4 only) |
| supply and probe | `ADC and supply` | v3: `5V`, `VPU`, `3.3V`, `ADC`; v4: `Vusb`, `5V`, `5V VPU`, `ADC`, `3.3V`, `3.3V VPU` |
| pin states | `Bus high`, `Bus Hi-Z 0`, `Bus Hi-Z 1` | `MOSI`, `CLK`, `MISO`, `CS` in each group |

The EEPROM block is the only v4-only block; every other block runs on both
boards.  A passing check means the observed result matched the expected one.
It is a go/no-go board check, not a measurement of margin or quality.

The `0x10` column in the matrix below is the definition of the short/full
split: the full pass runs every check, and the short pass skips the rows
marked `skipped`.

## Check matrix

`0x10` is the binary short pass; `0x11`, `~` and every full run use the full
pass.  A group row is four checks, one per I/O pin.

| Check | Jumper | `0x10` | v3 | v4 |
|---|---|---|---|---|
| `AUX` | — | runs | yes | yes |
| `MODE LED` | — | runs | yes | yes |
| `PULLUP H` | — | runs | yes | yes |
| `PULLUP L` | — | runs | yes | yes |
| `VREG` | — | runs | yes | yes |
| `SCL` | — | runs | — | yes |
| `SDA` | — | runs | — | yes |
| `WP` | — | runs | — | yes |
| `READ&WRITE` | — | runs | — | yes |
| `Vusb` | — | runs | — | yes |
| `5V` | — | runs | yes | yes |
| `5V VPU` | — | runs | — | yes |
| `ADC` | `ADC` | skipped | yes | yes |
| `3.3V` | — | runs | yes | yes |
| `3.3V VPU` | — | runs | — | yes |
| `VPU` | `Vpu` | skipped | yes | — |
| `Bus high` (group) | — | runs | yes | yes |
| `Bus Hi-Z 0` (group) | — | runs | yes | yes |
| `Bus Hi-Z 1` (group) | v3: `Vpu` | skipped | yes | yes |

The jumper column names the wire the check needs; the wiring is in
[wiring.md](wiring.md).  `ADC` is `ADC`→`+3.3V` on both boards; `Vpu` is
`Vpu`→`+5V` on v3.  The `VPU` check and the two `... VPU` checks are
different: `VPU` measures the v3 pull-up pin, while `5V VPU` and `3.3V VPU`
measure the v4 internal sources.

## Control and EEPROM checks

These run first.  The EEPROM block tests the Bus Pirate's own EEPROM, not a
device on the header.

| Check | What it does | If it fails |
|---|---|---|
| `AUX` | drives the `AUX` pin high and reads it high | the pin is held low; inspect the `AUX` pin and header |
| `MODE LED` | drives the MODE LED line high and reads it high | the line is stuck low; the LED itself may still light |
| `PULLUP H` | enables the pull-ups and reads the control line high | the pull-ups cannot be enabled; `Bus Hi-Z 1` may follow |
| `PULLUP L` | requests the pull-ups off and reads the control line low | the line did not return low; the pull-ups may stay on |
| `VREG` | enables the regulator and reads the enable line high | the supply cannot switch on; the rail checks may follow |
| `SCL` | reads the on-board EEPROM clock line high | line stuck low: EEPROM bus, pull-up or short |
| `SDA` | reads the on-board EEPROM data line high | line stuck low: EEPROM bus, pull-up or short |
| `WP` | reads the on-board EEPROM write-protect line high | line stuck low; the EEPROM would not be write-protected |
| `READ&WRITE` | writes, reads back and restores one EEPROM byte | a read, write or restore step failed; it also declines if the hardware I2C engine is still enabled |

`WP` reads the line high first; `READ&WRITE` then drives the same line low for
the write and restores the saved level (high after a normal `WP` pass).  The
write is the inverse of the stored byte; the check reads it back, restores
the original, reads it back again, and restores the saved WP level.  That is
why the two adjacent checks do not contradict: they test the same line in
opposite states.  `READ&WRITE` uses one byte, not the whole EEPROM.  A pass
means the original byte was restored and read back.  A failure can be the
initial read, the write, a read-back or the restore.

A `PULLUP L` pass only proves the control line read low after the pull-ups
were requested off.  A line stuck low also passes it.  Read it with `PULLUP H`
and `Bus Hi-Z 1`, which exercise the same control line; the same limit is
listed in [What is not checked](#what-is-not-checked).

## Supply and probe checks

These print `name(v.vv)` because they are measurements.  The `ADC` and `VPU`
rows need the setup jumpers; the other rows use an internal source.

| Check | What it does | If it fails |
|---|---|---|
| `Vusb` | reads the USB 5V input rail in the 5V window | the USB input is out of band; check the USB power |
| `5V` | reads the 5V rail in the 5V window | the rail is out of band: supply fault or short |
| `5V VPU` | selects the v4 5V pull-up source and reads it in the 5V window | the source is out of band; if `5V` also fails, suspect the rail |
| `ADC` | reads the `ADC` pin in the 3.3V window; the jumper ties it to `+3.3V` | the jumper is missing or loose, the probe input is faulty, or `3.3V` is low |
| `3.3V` | reads the 3.3V rail in the 3.3V window | the rail is out of band: regulator fault or short |
| `3.3V VPU` | selects the v4 3.3V pull-up source and reads it in the 3.3V window | the source is out of band; if `3.3V` also fails, suspect the rail |
| `VPU` | reads the v3 pull-up pin in the 5V window; the jumper ties it to `+5V` | the jumper is missing or loose, the pull-up output is faulty, or `5V` is low |

The v3 supply block prints `5V`, `VPU`, `3.3V`, `ADC` in that order.  The v4
block prints the table order.  A reading near `0.00` on a rail means the rail
is off or shorted; a reading inside the window is a pass even if it is a few
hundred millivolts from nominal.  The limits are in
[Voltage checks](#voltage-checks).

## Pin-state checks

Each group prints four checks, one per pin.  All four pins have the same
expectation in a group; a failure names the pin that did not match.

| Group header | What each pin check does | If it fails |
|---|---|---|
| `Bus high` | drives the pin high and reads it high | the pin is stuck low: short, damage or a connected device |
| `Bus Hi-Z 0` | drives the pin low and reads it low | the pin is stuck high: short, damage or a connected device |
| `Bus Hi-Z 1` | releases the pin as an input with the pull-ups on and reads it high | the pull-up reference (v3 `Vpu`) or the pin/header |

`Bus Hi-Z 1` runs only in the full pass, even on v4 where no external jumper
is needed.  On v3 the same `Vpu`→`+5V` reference feeds this group and the
`VPU` check.

## Group headers

These lines appear on their own and carry no ` OK` / ` FAIL`:

| Header | Meaning |
|---|---|
| `Ctrl` | the control-line block |
| `EEPROM` | the on-board EEPROM block (v4) |
| `ADC and supply` | the supply and probe block |
| `Bus high`, `Bus Hi-Z 0`, `Bus Hi-Z 1` | the three pin-state blocks |

## Result and count

- A check prints ` OK` or ` FAIL`.  Only a `FAIL` adds one to the error count,
  and a failure never stops the run.  The SMPS refusal is the one case where
  a reply count does not come from a failed check; see
  [When the run does not start](#when-the-run-does-not-start).
- A measurement check prints its reading before the result, as
  `name(v.vv)`.
- The terminal run ends with `Found <n> errors.`, with `<n>` in decimal;
  `0` is clean.  The wording is always `errors.`, so one failure reads
  `Found 1 errors.`.
- The binary pass prints nothing and returns one raw byte: the number of
  failed checks, `0x00` clean.  It is not an ASCII digit.
- The group headers and the manual LED prompt are not checks and do not add
  to the count.

The check count per pass:

| Pass | v3 | v4 |
|---|---|---|
| full (`~` in the terminal, `0x11` in binary) | 21 | 27 |
| short (`0x10` in binary) | 15 | 22 |
| SMPS refusal (`0x10` or `0x11`) | — | 0 checks; reply `0x01` |

## Voltage checks

The exact limits are fixed; treat the windows as approximate.  The firmware
compares a raw ADC sample and prints volts using its own scale (see
[capabilities/adc/probe.md](../adc/probe.md)).

| Family | Checks | Nominal | Accepted, about |
|---|---|---|---|
| 5V | `Vusb`, `5V`, `5V VPU`, `VPU` | 5.0 V | 4.5–5.5 V |
| 3.3V | `ADC`, `3.3V`, `3.3V VPU` | 3.3 V | 2.8–3.8 V |

The `ADC` check is in the 3.3V family because the setup jumper ties the probe
pin to `+3.3V`.  The `VPU` check is in the 5V family because its jumper ties
the pull-up pin to `+5V`.  A reading near `0.00` on a rail means the rail is
off or shorted; a reading inside the window is a pass even if it is a few
hundred millivolts from nominal.  The v3 / v4 table below says which checks
are present on which board.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| control-line block | yes | yes |
| on-board EEPROM block | — | yes |
| `Vusb` | — | yes |
| `5V VPU` / `3.3V VPU` | — | yes |
| `VPU` jumper check | yes | — |
| supply block, in order | `5V`, `VPU`, `3.3V`, `ADC` | `Vusb`, `5V`, `5V VPU`, `ADC`, `3.3V`, `3.3V VPU` |
| LED prompt | `MODE and VREG LEDs should be on!` | `MODE, VREG, and USB LEDs should be on!` |
| checks, full / short | 21 / 15 | 27 / 22 |
| SMPS-owned ADC refusal | — | optional build only |

The v3 column is **not verified on v3**.

## What is not checked

- A target, a bus protocol, or any mode's command set.  A clean run does not
  prove that SPI, I2C or another mode works.
- The USB data path beyond the terminal output, the bootloader, or the v4
  NORMAL button.
- The `AUX1` / `AUX2` pins.  The `AUX` check uses the `AUX` pin only.
- The whole on-board EEPROM.  `READ&WRITE` uses one byte.
- Pull-up resistor values, rail current capability, or pin drive strength.
- The `PULLUP L` result on its own.  A control line stuck low also reads low;
  use `PULLUP H` and `Bus Hi-Z 1` to catch that.
- Timing, signal integrity, or ADC calibration.  The voltage checks are
  in-band go/no-go tests.
- The LEDs themselves.  `MODE LED` reads the drive line; the end-of-run
  prompt asks you to look at the LEDs and is not counted.
- The AUX assignment or the other modes' state.  The run restores the
  default state instead of testing it.

## When the run does not start

On a v4 build with SMPS support, a running SMPS feedback loop owns the ADC.
The self-test checks that ownership before the setup prompt and before any
check.  In the binary layer both `0x10` and `0x11` then return the refusal
count `0x01` without running a check.  The reachable sequence is:

```
host -> 0x00 x20         enter the binary layer
host <- "BBIO1"
host -> 0x05             select raw wire
host <- "RAW1"
host -> 0xF2 0x58        start the SMPS; the value is SMPS-specific
host <- 0x01             accepted
host -> 0x00             back to BBIO1; the SMPS keeps running
host <- "BBIO1"
host -> 0x10             short self-test
host <- 0x01             refusal count; no check ran
host -> 0xFF             leave the self-test
host <- 0x01
host -> 0x05             raw wire
host <- "RAW1"
host -> 0xF1             stop the SMPS
host <- 0x01             stopped
host -> 0x00             back to BBIO1
host <- "BBIO1"
host -> 0x0F             leave to the terminal
host <- 0x01             then the banner and HiZ>
```

`0x11` returns the same `0x01` as `0x10`.  The short pass skips the `ADC`
check, but its rail checks still need the converter, so it refuses too.  The
`0x01` is a refusal count, not one failed check; the byte alone cannot
distinguish the two.  Know whether the SMPS is running.

To recover:

- A binary host leaves the idle loop with `0xFF` (reply `0x01`).  From
  `BBIO1`, send `0x05` (reply `RAW1`) before `0xF1` (reply `0x01`); `0xF1` in
  `BBIO1` is a pin-state byte, not an SMPS command.
- `0x0F` leaves the binary layer.  Its v4 terminal reset runs the board reset
  and stops the SMPS; on v3 it is a hardware reset.
- A terminal user sends `#`, the terminal reset, which also stops the SMPS on
  an SMPS-enabled v4 build.  A power-cycle or the v4 reset button stops it
  too.  A terminal user does not normally meet the refusal: reaching `HiZ>`
  already reset the board, so the SMPS is not running and `~` starts
  normally.  If the guard does trip, the terminal prints
  `ADC busy: SMPS feedback active; command rejected.` then
  `Syntax error at char 1`, with no setup prompt and no `Found` line.

The default image has SMPS support off, so this refusal cannot occur there.
v3 has no SMPS support.  The optional build is in
[capabilities/smps/README.md](../smps/README.md); the binary layer's view of
the same rule is in
[protocol/binary/edge-cases.md](../../protocol/binary/edge-cases.md).

## Related docs

- [terminal.md](terminal.md) — the interactive run and its output.
- [binary.md](binary.md) — the `0x10` / `0x11` variants and the count byte.
- [wiring.md](wiring.md) — the jumpers and the post-run state.
- [edge-cases.md](edge-cases.md) — failure and recovery cases.
- [capabilities/adc/probe.md](../adc/probe.md) — the ADC scale and the
  SMPS feedback ownership.
