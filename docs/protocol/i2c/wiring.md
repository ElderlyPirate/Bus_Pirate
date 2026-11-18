# I2C — wiring and electrical requirements

Part of the I2C documentation set (`docs/protocol/i2c/`). This page owns the
wiring table, the pull-up requirement, the Vpullup feed and the voltage limit.
The README carries the headline only. Clock stretching is a bus behaviour, not
a wiring item; it is covered in [clock-stretching.md](clock-stretching.md).

## Wiring

| Bus Pirate pin | Direction | I2C signal | Notes |
|---|---|---|---|
| MOSI | ↔ | SDA (data) | open drain: only pulls low; the pull-up makes the high |
| CLK | → | SCL (clock) | open drain, same rule |
| GND | ⏚ | ground | common ground with the target |
| Vpullup (`Vpu`) | in | pull-up reference | the voltage the pull-ups tie SDA/SCL to; not a bus wire |
| MISO | — | not used | leave unconnected |
| CS | — | not used in the terminal mode | the binary `I2C1` entry drives it low |

The pin table (`v`) labels the two bus columns **SCL** and **SDA** over the CLK
and MOSI header pins. The mode name, the wiring and the pin table therefore use
two forms for the same two wires:

| Firmware form | Header pin | Bus name |
|---|---|---|
| `SCL` | CLK | serial clock |
| `SDA` | MOSI | serial data |

## The pull-up is required

**Pull-up resistors on SDA and SCL are mandatory.** I2C is open drain: every
device, including the Bus Pirate, can only pull a line low; the high level comes
only from the pull-up. The Bus Pirate never drives either line high, and I2C has
no output-type prompt that could change that.

Without a pull-up there is no idle high. The classic symptom is that the
**address scanner reports a device at every address**, because a floating line
reads as an ACK. A line that is held low is different: the scanner's pre-check
warns and does not scan (see [bus-operations.md](bus-operations.md)).

Value guidance:

| Situation | Pull-up |
|---|---|
| General bus | **2 kΩ – 10 kΩ** on each line, to Vpullup |
| Fast profiles or a long/loaded bus | the lower end; check the rise time |
| A target with its own pull-ups | leave them in place; external resistors are in parallel |

A stronger pull-up gives a faster rise but increases the current a slave must
sink for ACK and data. Both limits belong to the target's datasheet.

### On-board pull-ups

Both boards have switched on-board pull-ups that can serve as the bus pull-up
when a voltage is present on Vpullup (`P`). On v4 the values are:

| Line | On-board resistor |
|---|---|
| SDA (MOSI) | 2 kΩ |
| SCL (CLK) | 10 kΩ |
| MISO, CS | 10 kΩ each; not used by I2C |

External resistors are in parallel with these. The 10 kΩ SCL value gives a slow
rise on a loaded bus, so fast profiles often need a stronger external SCL
pull-up; roughly **1.5–2.2 kΩ** is a starting point. Select the value from
the actual bus capacitance and the sink-current rating, then check the rise
and the high time at the target. SDA's 2 kΩ is often sufficient; verify it
separately rather than fitting the same resistor to both lines.

The v3 on-board pull-up value is not covered here (**unverified**); the v3
Vpullup pin still needs an external feed.

## Output type

I2C is always **open drain / open collector**, and the mode forces that state.
There is no `Select output type:` prompt (that belongs to SPI, UART and the raw
modes) and no binary `0x8x` group. This is why the pull-up is unconditional: the
Bus Pirate never drives a line high itself.

## Feeding the pull-up voltage (Vpullup)

`P` connects the Bus Pirate's pull-up resistors between the I/O lines and the
physical **Vpullup** pin. The `v` table labels that column `VPU` and the
reference card marks the pin `Vpu`; this set uses Vpullup in prose and quotes
the other forms where they appear. That pin must already be at the pull-up
voltage:

| Selection | Source |
|---|---|
| `e1` (v4) | External (or None) |
| `e2` (v4) | on-board **3.3 V** |
| `e3` (v4) | on-board **5.0 V** |
| v3 | no `e`; feed the pin from an external 0–5 V supply (**not verified**) |

On v4, `e` is accepted only while the pins are open-drain high impedance and no
voltage is already on VPU.

If nothing is on VPU, `P` still switches the resistors on but prints
`Warning: no voltage on Vpullup pin`, and the bus has no idle high.

```
I2C>P
Pull-up resistors ON
Warning: no voltage on Vpullup pin
```

**Order:** select the VPU source (`e2`/`e3`) before `P`. `W` and `e` are
independent; `W` does not feed VPU and `e` does not power the target. The full
chain is in
[entry-and-setup.md](entry-and-setup.md#bring-up-supplies-vpu-and-pull-ups).

## Supplies and voltage limits

The target's VCC and Vpullup are separate:

- `W` switches the Bus Pirate's programmable 3.3 V and 5.0 V rails onto their
  header pins to power the target. Leave those pins disconnected from a
  self-powered target.
- VPU only sets the pull-up reference. It can be 3.3 V while the target runs at
  5 V, but then the bus high level is 3.3 V.

**Match VPU to the target's logic level.** A 5 V pull-up on a 3.3 V target can
damage it. Use `e2` for 3.3 V targets, `e3` only for 5 V targets, or feed Vpu
externally at the target's level.

The I2C interface and VPU are **5 V class**: the on-board VPU source tops out at
5.0 V, and the Vpullup pin is specified as 0–5 V on the reference card. Keep
external pull-ups and VPU within that range; do not apply more than 5 V. The
exact absolute-maximum rating of the I/O pins is not documented here
(**unverified**).

## Electrical behaviour of the lines

- Both lines idle high through the pull-up. The Bus Pirate releases a line
  (high impedance) for a logical 1 and pulls it to ground for a logical 0.
- An idle-high line that reads low is either a short or a target holding it.
  The scanner's pre-check reports `Warning: *Short or no pull-up`; a bus
  operation that cannot complete reports `I2C bus error` instead. The warning
  is only printed by the scanner.
- Bus capacitance and the pull-up set the rise time. Rise time is often the real
  limit at the fast profiles, not the labelled clock.
- A slave may hold SCL low to pause the master. The Bus Pirate waits for the
  release, bounded; see [clock-stretching.md](clock-stretching.md).

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| SDA / SCL pin | MOSI / CLK | MOSI / CLK |
| pull-up required | yes | yes |
| on-board pull-up | switched, value not covered (**unverified**) | SDA 2 kΩ, SCL 10 kΩ, MISO/CS 10 kΩ |
| VPU source selection | none; feed externally | `e1` / `e2` / `e3` |
| on-board EEPROM bus | — | separate internal bus, macros 3/4 |

v3 wiring and the v3 VPU route are **not verified**.

## Related docs

- [entry-and-setup.md](entry-and-setup.md) — driver, speeds and bring-up
  commands.
- [bus-operations.md](bus-operations.md) — the scanner pre-check and the bus
  error.
- [clock-stretching.md](clock-stretching.md) — stretch wait and recovery.
- [eeprom.md](eeprom.md) — the separate on-board EEPROM bus.
- [docs/generic/commands.md](../../generic/commands.md#feeding-the-vpullup-pin)
  — the shared `W`, `P` and `e` commands.
