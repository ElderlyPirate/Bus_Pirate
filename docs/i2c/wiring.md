# I2C — wiring and electrical requirements

Part of the I2C documentation set (`docs/i2c/`).  This page owns the full
wiring table and the pull-up statement; the README carries the headline only.

## Wiring

| Bus Pirate pin | Direction | I2C signal | Notes |
|---|---|---|---|
| MOSI | ↔ | SDA (data) | open-drain: the Bus Pirate only pulls it low; the pull-up makes the high |
| CLK | → | SCL (clock) | open-drain, same rule |
| GND | ⏚ | ground | common ground with the target |
| VPU (`Vpu`) | in | pull-up voltage reference | the voltage the pull-ups tie SDA/SCL to; **not a wire to the bus** — see below |

The Bus Pirate's **MISO** pin is not used in I2C mode.  SDA is the pin the mode
itself labels `SDA` and SCL the one it labels `SCL` in its pin table (`v`).

The two on-board lines are read back by the mode for the bus checks; the
address scanner and the clock-stretch wait both sample them.

## The pull-up is required — stated explicitly

**Pull-up resistors on SDA and SCL are mandatory.**  I2C is an open-drain bus:
every device (including the Bus Pirate) can only pull a line low; the high
level comes only from the pull-up.  With no pull-up there is no idle-high and
the classic symptom is that the **address scanner reports a device at every
address**, because a floating line reads as an ACK.

Value guidance:

| Situation | Pull-up |
|---|---|
| General bus | **2 kΩ – 10 kΩ**, one on SDA and one on SCL, to the pull-up voltage |
| Longer/faster bus or many devices | the lower end (≈ 2 kΩ) for a faster edge |

The Bus Pirate's own on-board pull-up resistors can serve as the bus pull-up
when a voltage is present on VPU (`P`).  The v3a and later boards are
documented with an on-board 2 kΩ pull-up; older boards need an external
resistor.  **The on-board value has not been re-measured on this v4 bench —
treat the 2 kΩ figure as unverified for v4.**

The bus checks use the pull-up: the scanner and the start condition warn with
`Warning: *Short or no pull-up` when a line that should have been released to
the high level is still low (see [bus-operations.md](bus-operations.md)).

## Output type

I2C is always **open-drain / open-collector** and the mode forces that state;
there is no `Select output type` prompt (that prompt belongs to SPI, UART and
the raw modes).  This is why the pull-up is unconditional: the Bus Pirate
never drives a line high itself.

## Feeding the pull-up voltage (VPU)

`P` connects the Bus Pirate's pull-up resistors between the I/O lines and the
physical **Vpullup** pin.  That pin must already be at the pull-up voltage:

- **v4** — the on-board source is selected with `e`:
  - `e1` External (or None),
  - `e2` on-board **3.3 V**,
  - `e3` on-board **5.0 V**.
  The bench session used `e2` (3.3 V).  `e` is only accepted while the pins
  are open-drain HiZ and no voltage is already on VPU.
- **v3** — there is no `e`; feed the Vpullup pin from an external 0–5 V
  supply.  (Derived from source — **not verified on this bench**.)

If nothing is on VPU, `P` still enables the pull-ups but prints
`Warning: no voltage on Vpullup pin`, and the bus has no idle-high.  The
shared `P`/`e` commands and the VPU routing are documented in
`docs/generic/` — see [commands.md](../generic/commands.md).

The target's own supply (VCC) is separate from VPU: `W` enables the Bus
Pirate's programmable supply to power the target, while VPU only sets the
pull-up reference.  Both are base-layer behaviour — see `docs/generic/`.

## Voltage limit

The I2C interface is 5 V safe; do not exceed **5.5 V** on SDA/SCL.  The Bus
Pirate's own pull-up source is at most the on-board 5.0 V rail.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| SDA / SCL pin | MOSI / CLK | MOSI / CLK |
| pull-up required | yes | yes |
| on-board pull-up | documented 2 kΩ (v3a+) | not re-measured here |
| VPU source selection | none — feed externally | `e1` / `e2` / `e3` |
| on-board EEPROM bus | — | separate internal bus, macros 3/4 (see [eeprom.md](eeprom.md)) |

v3 wiring and the v3 VPU route are read from the firmware/hardware
documentation and are **not verified on this bench** (no v3 hardware).