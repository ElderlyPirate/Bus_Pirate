# AVRISP-MKII — wiring and electrical requirements

Part of the AVRISP-MKII documentation set (`docs/avrisp/`).  This page owns
the full wiring tables; the README carries the headline only.

Bus Pirate v4 I/O header pin names used below (shrouded 2×6, key = pin 1):

| Pin | Name | Pin | Name |
|---|---|---|---|
| 1 | MOSI | 7 | AUX2 |
| 2 | CLK | 8 | ADC |
| 3 | MISO | 9 | VEXTERN |
| 4 | CS | 10 | SWV33 |
| 5 | AUX0 | 11 | SWV50 |
| 6 | AUX1 | 12 | GND |

The signals used by this personality are **MOSI (1)**, **CLK (2)**,
**MISO (3)**, **CS (4)**, **AUX0 (5)**, **ADC (8)** and **GND (12)**.
AUX1, AUX2, VEXTERN, SWV33 and SWV50 are not used.

## ISP (ATtiny / ATmega)

| Programmer signal | Bus Pirate pin | Target signal |
|---|---|---|
| SCK | CLK (2) | SCK |
| MOSI (data out) | MOSI (1) | target MOSI / PDI pin (ISP data-in) |
| MISO (data in) | MISO (3) | target MISO / PDO pin (ISP data-out) |
| /RESET | AUX0 (5) | /RESET |
| VTARGET sense (optional) | ADC (8) | target VCC |
| rescue clock (rescue only) | CS (4) | XTAL1, not /RESET |
| ground | GND (12) | GND |

- **No pull-up is required** on any ISP signal.  The `CS` line idles and is
  used only for the rescue clock.
- Asserting `/RESET` uses the polarity selected by the host; the default is
  **active-low**, and **release tristates the line** (it is not driven
  high).  The target's own reset pull-up therefore governs the released
  level, as on any normal board.
- **The programmer does not supply target power.**  Power the target
  externally and share ground.  The ISP clock/data pins are push-pull
  3.3 V outputs.

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
  `AUX0` and do not press the target's reset button while programming.
- The DATA line is **half duplex**: the Bus Pirate's MOSI and MISO are
  joined into a single node through two resistors (220 Ω each in the LUFA
  reference design).  A single series resistor (for example 330 Ω) between
  the target DATA pin and the MOSI+MISO junction also works — the
  transmitter drives through it and the receiver senses through it.
- **No pull-up is required.**  The `CS` rescue clock is not used for PDI
  (leave CS open).
- All XMEGA logic is 3.3 V; no level shifting is needed.

## TPI (tinyAVR: ATtiny4/5/9/10)

| Programmer signal | Bus Pirate pin | Target signal |
|---|---|---|
| clock | CLK (2) | TPI_CLK |
| data | MOSI (1) **and** MISO (3), each through a resistor | TPIDATA |
| /RESET | AUX0 (5) | /RESET |
| VTARGET sense (optional) | ADC (8) | target VCC |
| ground | GND (12) | GND |

- The DATA line is joined exactly as for PDI.
- **No pull-up is required** by the Bus Pirate.
- NVM programming of ATtiny4/5/9/10 requires a **5 V target supply** (see
  [tpi.md](tpi.md)); use suitable level translation if the target runs at
  a voltage the Bus Pirate's 3.3 V push-pull outputs cannot drive safely.

## Voltage, VTARGET and logic levels

- The programmer drives **push-pull 3.3 V** signals, including the rescue
  clock.  Inputs are 5 V tolerant; a 5 V input is not a 5 V output.
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
the default state.