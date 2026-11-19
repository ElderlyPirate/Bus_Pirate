# LCD — adapter wiring and electrical requirements

Part of the LCD documentation set (`docs/capabilities/lcd/`).  This page owns
the adapter revisions, the Bus Pirate pins, the shift-register outputs, the
display power and contrast path, and the pull-up requirements.  The README
carries only the headline.

The published designs in the repository document the connector and supply
path for the revisions named below.  A different board or a clone may differ;
statements marked `unverified` need confirmation on the board you have.  The
last section lists what to confirm.

## The adapter

The Bus Pirate has no HD44780 interface of its own.  The mode drives a separate
board built around a **74HCT595** or **74HC595** 8-bit shift register.  It is
a Dangerous Prototypes design, not a stock Bus Pirate accessory.  The
repository has two design lines:

| Line | Revisions | Board facts |
|---|---|---|
| v3-era LCD adapter | v2, v2a, v2b, v2c, v3, v3a | 74HCT595; 10 kΩ contrast pot; backlight header, adjustable backlight resistor from v2b; the v3a has Contrast and Backlight pots |
| v4-era LCD adapter | v4 adapter v3 (2011), v4b | 74HCT595 (2011 v3) or 74HC595 (v4b); 3.3 V / 5.0 V select; contrast pot; v4b backlight is fixed-on or AUX0-controlled |

The firmware does not identify a board or revision.  Both lines use the same
Q0-Q7 assignment.  The repository also has a **passive v4** board: it is a pin
breakout with no shift register and is not the adapter this mode drives.

![Bus Pirate LCD adapter v2 with a 16x2 display][lcd-v2]

*The v2 board, its ribbon and a 16x2 glass.*

![Bus Pirate LCD adapter v3a][lcd-v3a]

*The v3a board: Contrast and Backlight pots, and the backlight jumpers.*

The v3a 3D model is in [Bus Pirate Lcd Adapter v3a.zip][lcd-v3a-zip].  The
published v2 and v4 adapter pages are
[v2](http://dangerousprototypes.com/docs/Bus_Pirate_LCD_adapter_v2) and
[v4](http://dangerousprototypes.com/docs/Bus_Pirate_v4_LCD_adapter).

## The chain

```
Bus Pirate           74x595                HD44780 display
 MOSI  ----------->  serial data
 CLK   ----------->  shift clock
 CS    ----------->  latch clock
 5.0V/3.3V ------->  adapter VCC ------>  display VDD (pin 2)
 GND   ----------->  common ground ---->  display VSS (pin 1)

                     8 outputs  ------->  RS, RW, EN, D4-D7
                                           (+ activity LED)
```

1. The Bus Pirate shifts one byte out on MOSI, clocked by CLK.
2. It pulses CS to latch the byte into the shift register's output register.
3. Seven outputs drive the display's register select, read/write, enable and
   four data lines; the eighth drives the activity LED.  The display is held
   in 4-bit mode by `(1)` / `(2)`.
4. A display byte is sent as two 4-bit nibbles, high nibble first; each nibble
   is three latched bytes, so one display byte takes six shift-and-latch
   cycles ([spi-link.md](spi-link.md#the-transfer-sequence)).

The link is **write-only**: the shift register is never read, the display's
R/W line is held at write, and the busy flag is not polled.

## Bus Pirate pins

| Bus Pirate pin | Direction | Adapter signal | Notes |
|---|---|---|---|
| MOSI | out | serial data (SER / DS) | data into the shift register |
| CLK | out | shift clock (SRCLK / SHCP) | clocks each bit in |
| CS | out | latch clock (RCLK / STCP) | driven low at setup, pulsed high after each byte |
| MISO | in | — | configured as an input; not used by the mode |
| 3.3V / 5.0V | supply | adapter VCC (and, on the published boards, display VDD) | v3-era uses the 5.0 V pin; v4 selects 3.3 V or 5.0 V; `W` switches both |
| GND | — | ground | common ground with the adapter and display |

The Bus Pirate pin functions above are fixed; the published boards use the
Bus Pirate header directly.  Confirm a different revision's connector
(`unverified`).

`CS` is **not** the display's enable.  It is the shift register's latch: the
display's EN is one of the eight outputs and is toggled by successive latches.
The firmware idles CS low and pulses it high after each byte; the shift
register latches on that rising edge.  `MISO` is not a busy or acknowledge
input.

The `v` / `i` table names the last four columns in the board's header order:
v4 `CS`, `MISO`, `CLK`, `MOSI`; v3 `CLK`, `MOSI`, `CS`, `MISO`
([entry-and-setup.md](entry-and-setup.md#status-output--i-and-v)).

## The eight shift-register outputs

The firmware shifts the byte most-significant bit first, so firmware bit 7
lands on output Q7 and bit 0 on Q0.  The assignment is fixed:

| Bit | Output | Display signal | Behaviour |
|---|---|---|---|
| 0 | Q0 | adapter activity LED | set high for each data/command nibble; high lights the LED in the published designs |
| 1 | Q1 | RS — register select | `0` command, `1` data |
| 2 | Q2 | RW — read/write | always `0` (write); the mode never reads |
| 3 | Q3 | EN — enable | low, then high, then low for each nibble |
| 4 | Q4 | D4 | data, high nibble first |
| 5 | Q5 | D5 | data |
| 6 | Q6 | D6 | data |
| 7 | Q7 | D7 | data |

The reset's first all-zero write clears all eight outputs, so the activity LED
is off until the first nibble.

The display's D0-D3 are not driven (4-bit interface).  The shift register's
serial output (Q7') is not used by the firmware.  The published v3-era and v4
boards route Q0-Q7 to these signals in this order; confirm a different
revision (`unverified`).

## Display power, contrast and backlight

`W` switches the Bus Pirate's regulated rails on and checks both the 5.0 V
and 3.3 V rails.  The adapter is a load on one of those rails; the display is
a load on the same rail when its VDD is tied to the adapter's VCC, which is
how the published boards are wired.

| Point | Published boards | Notes |
|---|---|---|
| adapter VCC | v3-era: Bus Pirate **5.0 V** pin; v4: jumper selects **3.3 V** or **5.0 V** | feeds the shift register and the display |
| display VDD (pin 2) | adapter VCC | the glass sees the selected rail |
| display VSS (pin 1) | GND | common with the Bus Pirate |
| V0 (pin 3) | 10 kΩ contrast pot wiper | on the adapter; set it for the glass |
| backlight A/K | board header or jumpers | v3a has a Backlight pot; v4b can use AUX0 |

- **Display voltage.**  Most HD44780 glasses are 5 V parts; check the
  datasheet.  The v3-era board supplies 5.0 V to the glass.  On a v4 board,
  set the voltage selector to the glass's VDD before applying power.  A 3.3 V
  glass on 5.0 V, or a 5 V glass on 3.3 V, can stay blank or be damaged.
- **Separate display feed.**  The published boards tie display VDD to adapter
  VCC.  A separate display feed needs that link broken; do not back-drive the
  adapter's VCC.  With a separate feed, `w` and a mode change do not switch
  the display, so it keeps its content.
- **`W` is still required with a separate feed.**  It runs the SPI setup and
  checks the Bus Pirate rails; the link does not work without it.
- **Logic levels.**  The Bus Pirate's SPI outputs are 3.3 V.  The v3-era
  board uses a 74HCT595, whose TTL input thresholds accept 3.3 V at 5 V VCC.
  The v4b uses a 74HC595; at 5 V its CMOS threshold is above the Bus Pirate's
  3.3 V high, so a 5 V setting is marginal.  Use the 3.3 V setting if the
  glass allows it; otherwise confirm the levels on the board (`unverified`).
- **Contrast (V0).**  V0 is an analogue input on the display, not a Bus Pirate
  setting and not a shift-register output.  The published boards have a 10 kΩ
  pot from VCC to GND with the wiper on V0.  A correctly wired display that
  stays blank is usually a supply or contrast problem.
- **Backlight.**  No shift-register output drives a backlight.  The v3-era
  board has a backlight header; from v2b an adjustable backlight resistor is
  fitted, and the v3a has the Backlight pot on the board.  The v4b can be
  jumpered for fixed
  on or for AUX0 control; with that jumper fitted, the shared `A` / `a`
  command drives it.  Backlight current and resistor values are board-specific
  (`unverified` for a different revision).
- **Current draw.**  The adapter and display current are not specified here
  and are `unverified`.

The display supply decides what happens when the mode is left.  A display
whose VDD is on the adapter's Bus Pirate rail loses power and clears; a
display with its own VDD feed keeps its content
([entry-and-setup.md](entry-and-setup.md#leaving-the-mode)).

## Pull-up statement

**No pull-up is required** on MOSI, CLK or CS in the normal case.  The Bus
Pirate drives all three actively, and the shift register's inputs are high
impedance; MISO is unused, so there is no open-drain bus and no idle-high
level to hold.  The LCD setup selects normal push-pull outputs and writes the
SPI open-drain bits off.
One board-level requirement:

- The shift register's output-enable must be tied low and its master-reset
  high for the outputs to drive.  The published designs do this; confirm it on
  a different revision (`unverified`).

`P` / `p` still work in LCD mode, but the Bus Pirate pull-ups are not part of
this interface; `P` may warn that there is no voltage on the Vpullup pin.

## What to confirm on the board

| Item | Why |
|---|---|
| adapter revision and connector labels | the firmware identifies no board |
| Q0-Q7 to display-signal routing | must match the assignment above |
| display VDD/VSS and voltage | the glass may be 5 V-only; the selected rail must match |
| supply input and voltage selector | `W` only switches the Bus Pirate rails |
| 3.3 V logic compatibility | the Bus Pirate drives 3.3 V outputs |
| shift-register output-enable / master-reset tie-offs | required for the outputs to drive |
| contrast V0 or an on-board pot | the usual cause of a blank display |
| backlight wiring, pot and resistor | no display-mode setting; v4b can use AUX0 |
| pull-ups or series parts on the adapter | not needed on the Bus Pirate side |
| a second enable line on the display | the adapter provides one EN; some 40×2 and 4-line glasses need two |

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| active Bus Pirate pins | MOSI / CLK / CS | MOSI / CLK / CS |
| `v` column order | `CLK MOSI CS MISO` | `CS MISO CLK MOSI` |
| supply control | `W` switches the regulated rails | same |
| pull-up needed | no | no |
| hardware path | unverified on v3 | unverified |

Both boards use the same three SPI signals and the same firmware path; only
the header order and the build's menu number differ.  The v3 hardware path is
**unverified on v3**.

[lcd-v2]: ../../assets/images/BP-LCDv2-250px.jpg
[lcd-v3a]: ../../assets/art/BP-LCD-v3a-600.jpg
[lcd-v3a-zip]: <../../assets/art/Bus Pirate Lcd Adapter v3a.zip>
