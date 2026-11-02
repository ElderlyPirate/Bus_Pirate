# 2WIRE — wiring and electrical requirements

Part of the 2WIRE documentation set (`docs/2wire/`).  This page owns the full
wiring table, the output-type behaviour, and the pull-up statement; the README
carries the headline only.

## Wiring

| Bus Pirate pin | Direction | 2WIRE signal | Notes |
|---|---|---|---|
| MOSI | ↔ | SDA (data) | bidirectional data line; the mode reads and writes it |
| CLK | → | SCL (clock) | clock, driven by the Bus Pirate |
| GND | ⏚ | ground | common ground with the target |
| VPU (`Vpu`) | in | pull-up voltage reference | the voltage the pull-ups tie SDA/SCL to for open drain; not a wire to the bus |
| CS | → | card reset (ATR only) | used **only** by the ISO 7816-3 ATR macros — see [smartcard.md](smartcard.md) |
| MISO | — | — | not used in this mode |

The Bus Pirate's pin is labelled **MOSI**; the mode's own pin table (`v`) prints
the line name **SDA** for it, and **SCL** for CLK — the same labels I2C shows.
This set writes the mode label first with the pin in parentheses: **SDA (MOSI)**
and **SCL (CLK)**.

The two on-board lines are read back for the bit and byte reads; the mode reads
SDA (MOSI) as its input.

## Output types — stated explicitly

The mode asks for an output type at entry (see
[entry-and-setup.md](entry-and-setup.md)).  The two choices differ in what a
"high" means:

| Output type | High level | Low level | Pull-up |
|---|---|---|---|
| **Open drain** (`1`) | released — a pull-up makes it high | pulled to ground | **required** |
| **Normal** (`2`) | driven to 3.3 V | pulled to ground | **not needed** |

This is why the pull-up rule is **conditional**: it follows the output type, not
the mode.

## The pull-up — open drain only

**Open-drain 2WIRE needs a pull-up on the open-drain lines.**  In open drain the
Bus Pirate can only pull a line low; the high level comes solely from the
pull-up resistor.  Value guidance: **2 kΩ – 10 kΩ** to the pull-up voltage.

The Bus Pirate's own on-board pull-ups can serve when a voltage is present on
VPU (`P`).  `P` switches them as a group: the on-board pull-ups tie **both**
I/O lines — SDA and SCL — to VPU at once, with no per-line control.  The v3a
and later boards are documented with an on-board 2 kΩ pull-up; older boards
need an external resistor.  **The on-board value is unverified for v4.**

Unlike I2C, 2WIRE's start/stop do **not** print a short/pull-up warning — the
mode does not check the released lines.  With no pull-up in open drain, a
floating line reads as a level and a read (or an ATR) returns meaningless data
without any error.  Use `.` to check the data pin, or `v` for the pin states.
See [edge-cases.md](edge-cases.md).

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
`Warning: no voltage on Vpullup pin`, and the open-drain bus has no idle-high.
The shared `P`/`e` commands and the VPU routing are documented in
`docs/generic/` — see [commands.md](../generic/commands.md).

The target's own supply (VCC) is separate from VPU: `W` enables the Bus
Pirate's programmable supply to power the target, while VPU only sets the
pull-up reference.

## Voltage limit

The 2WIRE interface is 5 V safe; do not exceed **5.5 V** on SDA/SCL.  The Bus
Pirate's own pull-up source is at most the on-board 5.0 V rail.

## Wiring a smart card

The ISO 7816-3 ATR macros repurpose **CS as the card's reset line** and the
supply as the card's VCC.  The card-contact mapping and the flow are in
[smartcard.md](smartcard.md).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| SDA / SCL pin | MOSI / CLK | MOSI / CLK |
| output-type choice | open drain or normal | open drain or normal |
| pull-up required (open drain) | yes | yes |
| on-board pull-up | documented 2 kΩ (v3a+) | unverified |
| VPU source selection | none — feed externally | `e1` / `e2` / `e3` |
| card reset line | CS | CS |

v3 wiring and the v3 VPU route are **not verified**.