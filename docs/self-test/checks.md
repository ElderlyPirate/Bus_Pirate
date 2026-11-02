# Self-test — check reference

Part of the self-test documentation set (`docs/self-test/`).  Every check the
self-test performs, what it verifies, and how it differs between boards.  The
terminal run is in [terminal.md](terminal.md); the scriptable variants are in
[binary.md](binary.md).

The firmware uses two forms for the same pin and rails: `Vpu` in the prompt is
the pin printed as `VPU` in the check labels, and the prompt rails `+5V` /
`+3.3V` are the labels `5V` / `3.3V`.  These are the same pin and the same
rails.

## How to read the tables

- **Check** — the exact label the firmware prints.  A check that fails prints
  ` FAIL`; a measurement check also prints its reading as `name(v.vv)`.
- **Jumper** — the check needs an external jumper wire to pass.
- `yes` / `—` in the v3 and v4 columns means the check is present / absent on
  that board.

## Control and supply checks

| Check | What it verifies | Jumper | v3 | v4 |
|---|---|---|---|---|
| `AUX` | the AUX pin drives high when set | no | yes | yes |
| `MODE LED` | the MODE LED line goes high when set | no | yes | yes |
| `PULLUP H` | the pull-up control line goes high when the pull-ups are enabled | no | yes | yes |
| `PULLUP L` | the pull-up control line goes low when the pull-ups are disabled | no | yes | yes |
| `VREG` | the regulator-enable line goes high when the supply is enabled | no | yes | yes |
| `SCL` | the on-board EEPROM I2C clock line reads high | no | — | yes |
| `SDA` | the on-board EEPROM I2C data line reads high | no | — | yes |
| `WP` | the on-board EEPROM write-protect line reads high | no | — | yes |
| `READ&WRITE` | a byte written to the on-board EEPROM reads back unchanged | no | — | yes |
| `Vusb` | the USB-supplied 5V rail is in range | no | — | yes |
| `5V` | the +5V rail is in range | no | yes | yes |
| `5V VPU` | the +5V pull-up rail, read on the pull-up pin, is in range | no | — | yes |
| `ADC` | the external probe voltage on the `ADC` pin is in range | **yes** | yes | yes |
| `3.3V` | the +3.3V rail is in range | no | yes | yes |
| `3.3V VPU` | the +3.3V pull-up rail, read on the pull-up pin, is in range | no | — | yes |
| `VPU` | the +5V pull-up output, via the `Vpu`→`+5V` jumper, is in range | **yes** | yes | — |

## Pin checks

The three group headers below each run a check on every one of the four I/O
pins, printed one per line as `MOSI`, `CLK`, `MISO` and `CS`:

| Group header | What each pin check verifies | Jumper | v3 | v4 |
|---|---|---|---|---|
| `Bus high` | the pin reads high when it is driven high | no | yes | yes |
| `Bus Hi-Z 0` | the pin reads low when it is driven low | no | yes | yes |
| `Bus Hi-Z 1` | the pin reads high as an input through the pull-ups | **yes** (v3) | yes | yes |

`Bus Hi-Z 1` runs only in the full test: it needs the pull-up reference.  Its
Jumper value is `**yes**` for v3, where that reference is the external
`Vpu`→`+5V` jumper; on v4 the on-board 3.3V pull-up source supplies it, so no
external jumper is needed.

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