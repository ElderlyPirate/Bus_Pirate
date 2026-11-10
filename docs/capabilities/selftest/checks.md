# Self-test — check reference

Part of the self-test documentation set (`docs/capabilities/selftest/`).
Every check the self-test performs, what it verifies, which pass runs it, and
how it differs between boards.  The terminal run is in
[terminal.md](terminal.md); the scriptable variants are in
[binary.md](binary.md).

The firmware uses two forms for the same pin and rails: `Vpu` in the prompt is
the pin printed as `VPU` in the check labels, and the prompt rails `+5V` /
`+3.3V` are the labels `5V` / `3.3V`.  These are the same pin and the same
rails.

## How to read the tables

- **Check** — the exact label the firmware prints.  A check that fails prints
  ` FAIL`; a measurement check also prints its reading as `name(v.vv)`.
- **Jumper** — the check needs an external jumper wire to pass.
- **`0x10`** — `runs` / `skipped` in the binary short pass.  The full pass
  runs every check.  This column is the single definition of the short/full
  split.
- **v3** / **v4** — `yes` / `—` means the check is present / absent on that
  board.  A check absent on a board cannot run there.
- **If it fails** — the element to inspect, not a diagnosis.

## Control and supply checks

| Check | What it verifies | Jumper | `0x10` | v3 | v4 | If it fails |
|---|---|---|---|---|---|---|
| `AUX` | the AUX pin drives high when set | no | runs | yes | yes | the AUX line is held low; inspect the AUX pin and header |
| `MODE LED` | the MODE LED line goes high when set | no | runs | yes | yes | the MODE LED drive line is stuck; the LED itself may still work |
| `PULLUP H` | the pull-up control line goes high when the pull-ups are enabled | no | runs | yes | yes | the pull-ups cannot be enabled: control line fault |
| `PULLUP L` | the pull-up control line goes low when the pull-ups are disabled | no | runs | yes | yes | the pull-ups may stay on: control line fault |
| `VREG` | the regulator-enable line goes high when the supply is enabled | no | runs | yes | yes | the supply cannot switch on: regulator enable fault |
| `SCL` | the on-board EEPROM I2C clock line reads high | no | runs | — | yes | EEPROM clock line stuck low: bus, pull-up or short |
| `SDA` | the on-board EEPROM I2C data line reads high | no | runs | — | yes | EEPROM data line stuck low: bus, pull-up or short |
| `WP` | the on-board EEPROM write-protect line reads high | no | runs | — | yes | EEPROM write-protect line stuck low: WP control fault |
| `READ&WRITE` | the firmware writes the inverse of a stored EEPROM byte, reads it back, then restores the original | no | runs | — | yes | the EEPROM did not accept and return the test byte: EEPROM or bus fault |
| `Vusb` | the USB-supplied 5V rail is in range | no | runs | — | yes | the USB 5V rail is out of band: supply or USB power fault |
| `5V` | the +5V rail is in range | no | runs | yes | yes | the +5V rail is out of band: supply fault or short |
| `5V VPU` | the +5V pull-up rail, read on the pull-up pin, is in range | no | runs | — | yes | the on-board 5V pull-up rail is out of band: pull-up source fault |
| `ADC` | the external probe voltage on the `ADC` pin is in range | **yes** | **skipped** | yes | yes | the `ADC` jumper is missing or not seated, or the probe input is faulty |
| `3.3V` | the +3.3V rail is in range | no | runs | yes | yes | the +3.3V rail is out of band: regulator fault or short |
| `3.3V VPU` | the +3.3V pull-up rail, read on the pull-up pin, is in range | no | runs | — | yes | the on-board 3.3V pull-up rail is out of band: pull-up source fault |
| `VPU` | the +5V pull-up output, via the `Vpu`→`+5V` jumper, is in range | **yes** | **skipped** (v3) | yes | — | the `Vpu`→`+5V` jumper is missing or not seated, or the pull-up output is faulty |

The `AUX` check drives the AUX pin itself; the `c`/`C`/`k`/`K` assignment does
not change which pin it tests.  The `READ&WRITE` check also declines when the
hardware I2C engine is still enabled by another protocol; leave the other
protocol (`m1` or `#`) and rerun `~`.

## Pin checks

The three group headers below each run a check on every one of the four I/O
pins, printed one per line as `MOSI`, `CLK`, `MISO` and `CS`:

| Group header | What each pin check verifies | Jumper | `0x10` | v3 | v4 | If it fails |
|---|---|---|---|---|---|---|
| `Bus high` | the pin reads high when it is driven high | no | runs | yes | yes | the pin is stuck low: short, damaged pin or header |
| `Bus Hi-Z 0` | the pin reads low when it is driven low | no | runs | yes | yes | the pin is stuck high: short, damaged pin or header |
| `Bus Hi-Z 1` | the pin reads high as an input through the pull-ups | **yes** (v3) | **skipped** | yes | yes | a missing pull-up reference (on v3 the `Vpu`→`+5V` jumper) or a faulty pin/header |

`Bus Hi-Z 1` runs only in the full pass; on v3 it also needs the
`Vpu`→`+5V` reference.  On v4 the on-board 3.3V pull-up source supplies it,
so no external jumper is needed.

## Group headers

These lines appear on their own and carry no ` OK` / ` FAIL`:

| Header | Meaning |
|---|---|
| `Ctrl` | printed once before the first check (the control lines) |
| `EEPROM` | the on-board EEPROM block (v4 only) |
| `ADC and supply` | the supply and ADC checks |
| `Bus high`, `Bus Hi-Z 0`, `Bus Hi-Z 1` | the three pin-state groups |

## Voltage checks

A measurement check prints its measured voltage in parentheses as
`name(v.vv)`.  It is accepted when the reading is inside a fixed band of
about ±0.5 V around the nominal rail value; the exact limits are set in
firmware, so treat ±0.5 V as an approximation.  The same band applies to the
5V family (`Vusb`, `5V`, `5V VPU`, `VPU`) and to the 3.3V family (`ADC`,
`3.3V`, `3.3V VPU`).

## v3 / v4 summary

| | v3 | v4 |
|---|---|---|
| control-line checks (`AUX`, `MODE LED`, `PULLUP H/L`, `VREG`) | yes | yes |
| EEPROM block (`SCL`, `SDA`, `WP`, `READ&WRITE`) | — | yes |
| `Vusb`, `5V VPU`, `3.3V VPU` | — | yes |
| `VPU` (jumper) | yes | — |
| pin groups (`Bus high`, `Bus Hi-Z 0`, `Bus Hi-Z 1`) | yes | yes |

The v3 column is **not verified** here.

## Related docs

- [terminal.md](terminal.md) — the interactive run.
- [binary.md](binary.md) — the `0x10` / `0x11` variants and their coverage.
- [edge-cases.md](edge-cases.md) — failure modes and recovery.
