# AVRISP-MKII — wiring and electrical requirements

Part of the AVRISP-MKII documentation set (`docs/programmers/avrisp/`).  This
page owns the full wiring tables **and the v4 header pin numbering** that the
ADC and AUX sets quote; the README carries the headline only.

## Bus Pirate v4 I/O header

Shrouded 2×6 header, key = pin 1, pins 1–6 down one row and 7–12 down the
other:

| Pin | Name |
|---|---|
| 1 | MOSI |
| 2 | CLK |
| 3 | MISO |
| 4 | CS |
| 5 | AUX |
| 6 | AUX1 |
| 7 | AUX2 |
| 8 | ADC |
| 9–11 | `Vpu`, `+3V3`, `+5V` (reference/supply; order within 9–11 unconfirmed) |
| 12 | GND |

- The names are the board's labels (`+3V3` and `+5V` are the two switchable
  supply rails; the firmware calls them `3V3` and `5V0`).  Locate a pin by
  its silkscreen name where the board prints one; the pin numbers are this
  set's published convention for the v4 header.
- Pins 9–11 are the reference/supply group; which of the three is which is
  **not confirmed** on a v4 silkscreen.  This personality uses none of them,
  so the ambiguity cannot affect its wiring, and the ADC and AUX sets use
  only pins 4–8.
- `AUX` is the pin the terminal calls `A0`; `AUX1` and `AUX2` are `A1` and
  `A2`.  Only `AUX` is used here.

The signals used by this personality are **MOSI (1)**, **CLK (2)**,
**MISO (3)**, **CS (4)**, **AUX (5)**, **ADC (8)** and **GND (12)**.
AUX1, AUX2 and the reference/supply pins are not used.

## ISP (ATtiny / ATmega)

| Programmer signal | Bus Pirate pin | Target signal |
|---|---|---|
| SCK | CLK (2) | SCK |
| MOSI (data out) | MOSI (1) | target MOSI / PDI pin (ISP data-in) |
| MISO (data in) | MISO (3) | target MISO / PDO pin (ISP data-out) |
| /RESET | AUX (5) | /RESET |
| VTARGET sense (optional) | ADC (8) | target VCC |
| rescue clock (rescue only) | CS (4) | XTAL1, not /RESET |
| ground | GND (12) | GND |

- **No pull-up is required** on any ISP signal.  While the personality is
  active **`CS` outputs the 4 MHz rescue clock** for the session (paused only
  while a calibration command runs), so leave it open unless you are
  injecting that clock into the target's XTAL1 — see
  [rescue-clock-osccal.md](rescue-clock-osccal.md).  It is not a chip select
  here.
- Asserting `/RESET` uses the polarity selected by the host; the default is
  **active-low**, and **release tristates the line** (it is not driven
  high).  The target's own reset pull-up therefore governs the released
  level, as on any normal board.
- **The programmer does not supply target power.**  Power the target
  externally and share ground.  The ISP clock/data pins are push-pull
  3.3 V outputs.
- **AVR053 calibration is a separate session, not the normal ISP wiring**:
  the calibration clock leaves on `MOSI` into the target's **MOSI** C-clock
  input (not XTAL1), the target's calibration OK output comes back on
  `MISO`, `/RESET` is released by the command, and the AVR053 reference
  wiring leaves the ISP `SCK` wire off.  `MOSI`/`MISO` are the ISP data
  pins, repurposed for calibration.  The full table and the order of
  operations are in [rescue-clock-osccal.md](rescue-clock-osccal.md) §AVR053
  wiring.

### ISP pin mapping is not the SPI pin mapping

On many ATmega parts the serial-programming data pins are **not** the
ordinary SPI pins.  For example, the ATmega64A takes programming data on
PE0/PDI (MOSI, into the target) and PE1/PDO (MISO, out of the target), with
SCK on PB1 — not on PB2/PB3.  Check the exact target's datasheet
programming-pin section before wiring; a programming header labelled
"MOSI/MISO" is not a guarantee.

The `PDI` and `PDO` in the ISP table above are ISP signal names
(program-data-in / program-data-out); they are not the XMEGA **PDI**
interface documented in [pdi.md](pdi.md).

## PDI (XMEGA)

| Programmer signal | Bus Pirate pin | Target signal |
|---|---|---|
| clock | CLK (2) | PDI_CLK (the XMEGA's **RESET** pin) |
| data | MOSI (1) **and** MISO (3), each through a resistor | PDI_DATA |
| VTARGET sense (optional) | ADC (8) | target VCC |
| ground | GND (12) | GND |

- On PDI the clock line is the target's **RESET/PDI_CLK** pin; do not wire
  `AUX` and do not press the target's reset button while programming.
- The DATA line is **half duplex**: the Bus Pirate's MOSI and MISO are
  joined into a single node through two resistors (220 Ω each in the LUFA
  reference design this port follows).  The firmware never drives both at
  once: the transmitter drives through the join and the receiver senses the
  same node.
- A single series resistor (for example 330 Ω) between the target DATA pin
  and the MOSI+MISO junction is an alternative that is **not verified** and
  not used by the reference design.
- **No pull-up is required.**  `CS` is the rescue clock on this port; it is
  not used for PDI, so leave `CS` open.
- All XMEGA logic is 3.3 V; no level shifting is needed.

## TPI (tinyAVR: ATtiny4/5/9/10)

| Programmer signal | Bus Pirate pin | Target signal |
|---|---|---|
| clock | CLK (2) | TPI_CLK |
| data | MOSI (1) **and** MISO (3), each through a resistor | TPIDATA |
| /RESET | AUX (5) | /RESET |
| VTARGET sense (optional) | ADC (8) | target VCC |
| ground | GND (12) | GND |

- The DATA line is joined exactly as for PDI.
- **No pull-up is required** by the Bus Pirate.  `CS` still outputs the
  rescue clock; it is not used for TPI.
- NVM programming of ATtiny4/5/9/10 requires a **5 V target supply** (see
  [tpi.md](tpi.md)); use suitable level translation if the target runs at
  a voltage the Bus Pirate's 3.3 V push-pull outputs cannot drive safely.

## Voltage, VTARGET and logic levels

- The programmer drives **push-pull 3.3 V** signals, including the rescue
  clock.  **Treat every pin as a 3.3 V interface**: the tolerance above
  3.3 V is not documented here and is **unverified**, so level-shift
  anything whose logic levels are not 3.3 V.
- The **VTARGET** value reported to the host is measured on the ADC pin,
  which has a 2:1 divider and accepts 0 to about 6.6 V.  VTARGET is
  **informational only**: it does not set the I/O voltage and does not
  interlock programming.  If nothing is wired to ADC, the reading is a
  floating input and means nothing.
- Use bidirectional level translation for targets whose logic thresholds
  or supply voltage do not match 3.3 V.  The series resistors in the PDI
  and TPI DATA join are **not** level shifters.
- **Never connect a 5 V or 12 V supply to a Bus Pirate pin**, and never
  enable the Bus Pirate's own power outputs into a target that is already
  powered.  This personality does not touch the supply rails; powering the
  target is the operator's job.

## Entering the mode resets the board state

Entering from binary bitbang mode **disables the Bus Pirate's power
supplies and pull-ups**.  Any VPU source selected earlier is not carried
into the personality, and re-entering the terminal afterwards starts from
the default state.  The lifecycle itself is in [entry.md](entry.md).

## Related docs

- [entry.md](entry.md) — entry, deadline and the return to `BBIO1`.
- [isp.md](isp.md) / [pdi.md](pdi.md) / [tpi.md](tpi.md) — the target paths.
- [rescue-clock-osccal.md](rescue-clock-osccal.md) — the `CS` clock and the
  AVR053 wiring.
- [edge-cases.md](edge-cases.md) — wiring and voltage failures.
