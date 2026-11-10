# LCD — adapter wiring and electrical requirements

Part of the LCD documentation set (`docs/capabilities/lcd/`).  This page owns
the wiring table, the adapter, the supply / contrast / pull-up statement; the
README carries the headline only.

## Scope

How the 74HCT595 adapter connects the Bus Pirate to an HD44780 display, and
what electrical conditions the firmware expects.  The electrical statements
here are derived from the firmware and the adapter's published design and are
`unverified`.

## The adapter

The Bus Pirate has no HD44780 interface of its own.  The mode drives a small
adapter board built around a **74HCT595** 8-bit shift register: the
**Bus Pirate LCD adapter**.  It is a separate Dangerous Prototypes design, not
a stock Bus Pirate accessory — build one or buy one:

- Bus Pirate LCD adapter v2 (the v3 design):
  <http://dangerousprototypes.com/docs/Bus_Pirate_LCD_adapter_v2>
- Bus Pirate v4 LCD adapter:
  <http://dangerousprototypes.com/docs/Bus_Pirate_v4_LCD_adapter>

The firmware does not identify a particular board or revision, so treat the
specific board, its connector markings and its electrical details as that
design's own.  The physical pin-out is `unverified` here.

## The adapter chain

```
Bus Pirate  --SPI-->  74HCT595  --parallel-->  HD44780
 MOSI/CLK/CS           shift register            4-bit bus
 VCC/GND               (8 outputs)               RS / EN / D4-D7
```

1. The Bus Pirate shifts an 8-bit word out over MOSI, clocked by CLK.
2. It pulses CS to latch the word into the shift register's output register.
3. The eight outputs drive the display: register select, enable, and the
   four data lines of the display's 4-bit interface.
4. A byte is sent as two 4-bit nibbles (enable pulsed once per nibble).

The mode does not read the display back: the shift register is write-only
here and the display's read line is not used.

## Bus Pirate pins

| Bus Pirate pin | Direction | Adapter signal | Notes |
|---|---|---|---|
| MOSI | out | shift-register serial data | data into the 74HCT595 |
| CLK | out | shift-register shift clock | clocks each bit in |
| CS | out | shift-register latch clock | pulsed after each byte to latch |
| MISO | in | — | configured as input, not used by the mode |
| VCC | supply | adapter / display logic supply | enabled by `W`; adapter supply routing is board-specific |
| GND | — | ground | common ground with the adapter |

The active signals are the SPI pins on both v3 and v4; the board-level pin
order differs, the electrical function does not.  The `v` pin-state row does
**not** name these signals — the LCD mode reuses the I2C mode's four-label
string, so those labels are not meaningful here.  See
[edge-cases.md](edge-cases.md#pin-labels-look-like-i2c) for the exact labels,
which differ by revision.

**Requires verification:** the adapter's physical connector pin-out — which
Bus Pirate header pin lands on which adapter pad — is not confirmed here.
Follow the adapter board's own markings.

## The eight adapter outputs

The firmware assigns the eight shift-register outputs to the display signals:

| Adapter output (firmware order) | Display signal |
|---|---|
| bit 0 | activity LED on the adapter |
| bit 1 | RS — register select (`0` command, `1` data) |
| bit 2 | RW — read/write (held at write; not driven high) |
| bit 3 | EN — enable / data clock |
| bit 4 | D4 |
| bit 5 | D5 |
| bit 6 | D6 |
| bit 7 | D7 |

The display is initialised for the **4-bit** interface, 5×7 font.  Reading is
not implemented, so RW is never driven to read and the busy flag is not
polled — the firmware uses fixed delays between operations instead.

`bit 0` is the adapter's indicator LED.  Its on/off sense is not confirmed;
the wiring itself does not depend on it.

## Supply, contrast and pull-ups

- **No pull-up resistors are required** (unverified).  The 74HCT595 has
  push-pull CMOS outputs that drive the display inputs directly; there is no
  open-drain bus and no idle-high level to maintain, so `P`/`p` play no role
  in this mode.  Treat this as a property of the 74HCT595 design rather than
  of one board.
- **Power the adapter.**  In the published design the Bus Pirate's VCC rail
  (switched on by `W`) supplies the adapter and display, with GND as the
  common return; but the adapter's own power input — whether it takes VCC
  from the Bus Pirate header or a separate supply — is board-specific, so
  confirm it on the adapter (**Requires verification**).  Treat 3.3 V / 5 V
  compatibility of the display and adapter as a property of that board too.
- **Display contrast (V0).**  Contrast is a display-board adjustment, not a
  Bus Pirate setting; set V0 per the display's datasheet.  A correctly wired
  display that stays blank is usually a supply or contrast problem.
- **`W` must be sent before writes have any chance of reaching the display.**
  Until `W`, the supplies are off and the bus is parked
  (`Clutch disengaged!!!`); the SPI pins are not driven.
- **The supply can stop again.**  `w`, a mode change and `#` all switch the
  regulated VCC off; a BP-powered display loses power and clears, while an
  externally powered display keeps its content
  ([entry-and-setup.md](entry-and-setup.md)).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| active pins | MOSI / CLK / CS | MOSI / CLK / CS |
| pin-table order differs | yes | yes |
| supply | `W` enables VCC | `W` enables VCC |
| pull-up needed | no (unverified) | no (unverified) |

Both boards use the same three SPI signals and the same firmware path; the v3
build is **not verified**.
