# AVRISP-MKII — wiring and electrical requirements

Part of the AVRISP-MKII documentation set.  This page owns the full wiring
tables **and the v4 header pin numbering** that the ADC and AUX sets quote;
the README carries the headline only.

## Bus Pirate v4 I/O header

Shrouded 2×6 header, key = pin 1.  Pins are numbered in pairs from the key
(1/2, 3/4, …); the key marks pin 1:

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
| 9 | `Vpu` (external pull-up reference) |
| 10 | `+3V3` (switched 3.3 V rail) |
| 11 | `+5V` (switched 5 V rail) |
| 12 | GND |

- The names are the board's labels.  The terminal displays `+3V3` and `+5V`
  as `3.3V` and `5.0V`, and the pull-up reference as `Vpu`.
- The reference/supply pins 9–11 are not used by this personality; the ADC
  and AUX sets use only pins 4–8 and 12.
- `AUX` is the pin the terminal calls `A0`; `AUX1` and `AUX2` are `A1` and
  `A2`.  Only `AUX` is used here.

The signals used by this personality are **MOSI (1)**, **CLK (2)**,
**MISO (3)**, **CS (4)**, **AUX (5)**, **ADC (8)** and **GND (12)**.
AUX1, AUX2 and the reference/supply pins are not used.

Two naming layers meet on this page.  The **Bus Pirate pins** are `MOSI`,
`MISO`, `CLK`, `AUX` and `CS`.  The **target signals** are `MOSI`, `MISO`,
`SCK` and `RESET`.  The two data wires run straight through, so `MOSI` and
`MISO` name the same signal at both ends; `CLK` is the Bus Pirate side of the
target's `SCK`, and `AUX` is the target's `RESET` on ISP and TPI.

## ISP (ATtiny / ATmega)

The target end is often a standard 2×3 ISP header.  Its pin order is not the
Bus Pirate header order:

| 6-pin ISP pin | Target signal |
|---|---|
| 1 | MISO |
| 2 | VCC (VTARGET sense) |
| 3 | SCK |
| 4 | MOSI |
| 5 | RESET |
| 6 | GND |

Check the target board's silkscreen before using that order: a 10-pin
header, a different pin order, or a programming header that is not the
target's SPI pins are all common.

| Programmer signal | Bus Pirate pin | Target signal |
|---|---|---|
| SCK | CLK (2) | SCK |
| MOSI (data out) | MOSI (1) | target MOSI / PDI pin (ISP data-in) |
| MISO (data in) | MISO (3) | target MISO / PDO pin (ISP data-out) |
| RESET | AUX (5) | RESET |
| VTARGET sense (optional) | ADC (8) | target VCC |
| rescue clock (rescue only) | CS (4) | XTAL1, not RESET |
| ground | GND (12) | GND |

- **No pull-up is required** on any ISP signal.  While the personality is
  active **`CS` outputs the 4 MHz rescue clock** for the session (paused only
  while a calibration command runs), so leave it open unless you are
  injecting that clock into the target's `XTAL1` — see
  [rescue-clock-osccal.md](rescue-clock-osccal.md).  It is not a chip select
  here.
- Asserting `RESET` uses the polarity selected by the host; the default is
  **active-low**, and **release tristates the line** (it is not driven
  high).  The target's own reset pull-up therefore governs the released
  level, as on any normal board.  A host can select active-high by writing
  `0` to the reset-polarity parameter; any nonzero value means active-low.
- **The programmer does not supply target power.**  Power the target
  externally and share ground.  The ISP clock/data pins are push-pull
  3.3 V outputs.
- **AVR053 calibration is a separate session, not the normal ISP wiring**:
  the calibration clock leaves on `MOSI` into the target's **MOSI** C-clock
  input (not `XTAL1`), the target's calibration OK output comes back on
  `MISO`, `RESET` is released by the command, and the AVR053 reference
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
- **No pull-up is fitted or enabled by the Bus Pirate.**  The reference
  design relies on the target's own line bias; whether a particular target
  board needs an external PDI_DATA pull-up is a target question
  (unverified).
- `CS` is the rescue clock on this port; it is not used for PDI, so leave
  `CS` open.
- The Bus Pirate drives 3.3 V push-pull.  An XMEGA target running at 3.3 V
  needs no level shifting; a target running **below** 3.3 V does — do not
  apply 3.3 V logic to a lower-voltage part.  The target is not powered by
  the Bus Pirate.  The interface-change procedure is in
  [command-model.md](command-model.md) §Changing target interface.

## TPI (tinyAVR: ATtiny4/5/9/10)

| Programmer signal | Bus Pirate pin | Target signal |
|---|---|---|
| clock | CLK (2) | TPI_CLK |
| data | MOSI (1) **and** MISO (3), each through a resistor | TPIDATA |
| RESET | AUX (5) | RESET |
| VTARGET sense (optional) | ADC (8) | target VCC |
| ground | GND (12) | GND |

- The DATA line is joined exactly as for PDI.
- **The Bus Pirate holds `RESET` low for the whole TPI session** — the
  tinyAVR TPI physical layer requires it.  `AUX` drives that line; the
  firmware releases it when the session ends.
- **No pull-up is fitted or enabled by the Bus Pirate.**  `CS` still outputs
  the rescue clock; it is not used for TPI.
- NVM programming of ATtiny4/5/9/10 requires a **5 V target supply** (the
  target datasheet allows NVM programming at 5 V only).  This is a target
  requirement, not a Bus Pirate output: power the target at 5 V from its own
  supply.  The ATtiny4/5/9/10 VIH at 5 V is 3.0 V, so the 3.3 V push-pull
  high is within the specified range with a small margin; a level shifter is
  optional.  See [tpi.md](tpi.md).
- A target with the `RSTDISBL` fuse programmed needs 12 V on `RESET` for
  TPI; this personality cannot generate it and the part is out of scope.
- The interface-change procedure is in
  [command-model.md](command-model.md) §Changing target interface.

## Voltage, VTARGET and logic levels

- The programmer drives **push-pull 3.3 V** signals, including the rescue
  clock.  The shared v4 header I/O class and absolute limits are owned by
  [the AUX wiring page](../../capabilities/auxiliary/wiring.md): the header
  inputs are 5 V class, while the programmer's own outputs are 3.3 V.  A
  target that needs a 5 V logic high must be level-shifted; a target that
  drives 5 V logic into `MISO` or `ADC` is within the input class.
- The **VTARGET** value reported to the host is measured on the `ADC` pin,
  which has a 2:1 divider and accepts 0 to about 6.6 V.  VTARGET is
  **informational only**: it does not set the I/O voltage and does not
  interlock programming.  If nothing is wired to `ADC`, the reading is a
  floating input and means nothing.
- Use bidirectional level translation for targets whose logic thresholds
  or supply voltage do not match 3.3 V.  The series resistors in the PDI
  and TPI DATA join are **not** level shifters.
- **Never connect a 5 V or 12 V supply rail to a signal pin**, and never
  drive a signal into a pin the programmer is driving.  Never allow a 12 V
  reset on any pin.  Never enable the Bus Pirate's own power outputs into a
  target that is already powered.  This personality never switches the
  supply rails on; powering the target is the operator's job.

## Entering the mode resets the board state

Entering from binary bitbang mode **disables the Bus Pirate's target
supplies and pull-ups** and releases the signal pins; `CS` then starts the
rescue clock.  Any earlier power state or `Vpu` source is not carried into
the personality, and re-entering the terminal afterwards starts from the
default state.  The lifecycle itself is in [entry.md](entry.md).

## Related docs

- [entry.md](entry.md) — entry, deadline and the return to `BBIO1`.
- [isp.md](isp.md) / [pdi.md](pdi.md) / [tpi.md](tpi.md) — the target paths.
- [rescue-clock-osccal.md](rescue-clock-osccal.md) — the `CS` clock and the
  AVR053 wiring.
- [edge-cases.md](edge-cases.md) — wiring and voltage failures.
- [ADC wiring](../../capabilities/adc/wiring.md) — the `ADC` pin and its
  scale.
- [AUX wiring](../../capabilities/auxiliary/wiring.md) — the shared header
  I/O class.
