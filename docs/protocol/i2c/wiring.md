# I2C — wiring and electrical requirements

Part of the I2C documentation set (`docs/protocol/i2c/`).  This page owns the
wiring table and the pull-up statement; the README carries the headline only.

## Wiring

| Bus Pirate pin | Direction | I2C signal | Notes |
|---|---|---|---|
| MOSI | ↔ | SDA (data) | open-drain: only pulls low; the pull-up makes the high |
| CLK | → | SCL (clock) | open-drain, same rule |
| GND | ⏚ | ground | common ground with the target |
| VPU (`Vpu`) | in | pull-up reference | the voltage the pull-ups tie SDA/SCL to; not a bus wire |

The Bus Pirate's **MISO** pin is not used in I2C mode.  SDA is the pin the
mode labels `SDA` and SCL the one it labels `SCL` in its pin table (`v`).

## The pull-up is required

**Pull-up resistors on SDA and SCL are mandatory.**  I2C is open-drain: every
device (including the Bus Pirate) can only pull a line low; the high level
comes only from the pull-up.  With no pull-up there is no idle-high and the
classic symptom is that the **address scanner reports a device at every
address**, because a floating line reads as an ACK.

Value guidance:

| Situation | Pull-up |
|---|---|
| General bus | **2 kΩ – 10 kΩ**, one on SDA and one on SCL, to VPU |
| Longer/faster bus, many devices | the lower end (≈ 2 kΩ) for a faster edge |

### On-board pull-ups (v4)

The Bus Pirate's switched on-board pull-ups can serve as the bus pull-up when
a voltage is present on VPU (`P`):

| Line | On-board resistor |
|---|---|
| SDA | 2 kΩ |
| SCL | 10 kΩ |

External resistors are in parallel with these.  The 10 kΩ SCL value gives a
slow rise on a loaded bus, so fast modes often need a stronger external SCL
pull-up: roughly 1.5–2.2 kΩ is a starting point.  Select the value from the
actual bus capacitance and the sink-current rating, then check the rise and
the high time at the target.  SDA's 2 kΩ is often sufficient; verify it
separately.  A stronger pull-up increases the current a slave must sink for
ACK/data.

The v3 on-board value is not covered here; feed VPU externally (unverified).

## Output type

I2C is always **open-drain / open-collector** and the mode forces that state;
there is no `Select output type` prompt (that belongs to SPI, UART and the raw
modes).  This is why the pull-up is unconditional: the Bus Pirate never drives
a line high itself.

## Feeding the pull-up voltage (VPU)

`P` connects the Bus Pirate's pull-up resistors between the I/O lines and the
physical **Vpullup** pin.  That pin must already be at the pull-up voltage:

- **v4** — the on-board source is selected with `e`:
  - `e1` External (or None),
  - `e2` on-board **3.3 V**,
  - `e3` on-board **5.0 V**.
  `e` is only accepted while the pins are open-drain HiZ and no voltage is
  already on VPU.
- **v3** — there is no `e`; feed the Vpullup pin from an external 0–5 V
  supply.  (**Not verified**.)

If nothing is on VPU, `P` still enables the pull-ups but prints
`Warning: no voltage on Vpullup pin`, and the bus has no idle-high.  See
[docs/generic/commands.md](../../generic/commands.md) §Feeding the VPU pin.

The target's own supply (VCC) is separate from VPU: `W` enables the Bus
Pirate's programmable supply to power the target, while VPU only sets the
pull-up reference.

## Voltage limits

The I2C interface and VPU are **5 V class**: the on-board VPU source tops out
at 5.0 V, and the Vpullup pin is specified as 0–5 V on the reference card.
Keep external pull-ups and VPU within that range; do not apply more than 5 V.
The exact absolute-maximum rating of the I/O pins is not documented here
(unverified).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| SDA / SCL pin | MOSI / CLK | MOSI / CLK |
| pull-up required | yes | yes |
| on-board pull-up | not covered (unverified) | SDA 2 kΩ, SCL 10 kΩ |
| VPU source selection | none — feed externally | `e1` / `e2` / `e3` |
| on-board EEPROM bus | — | separate internal bus, macros 3/4 |

v3 wiring and the v3 VPU route are **not verified**.
