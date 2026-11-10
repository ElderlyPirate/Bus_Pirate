# LCD — entering the mode and setup

Part of the LCD documentation set (`docs/capabilities/lcd/`).  This page owns
the build path, the entry sequence, the build-dependent menu number, the
mode's defaults, the status output (`i` and `v`), and the `W`/`w` supply
steps.  Shared prompt mechanics are in
[the generic layer](../../generic/README.md).

## Availability — the mode is an optional build

The LCD mode is **not compiled into the default v3 or v4 images**.  It is
present only in a **custom build** in which the LCD feature is enabled in the
firmware configuration.  Build and flash are developer material: see
[dev/building.md](../../dev/building.md) and, for the source locations, the
maintainer [code map](../../dev/code-map.md).

In an image where LCD is **not** enabled there is no `LCD` entry in the mode
menu and the mode cannot be entered.  Selecting a number the image does not
have is rejected with `Nonexistent protocol!`.

No image with the LCD mode is distributed, so treat the build and the mode as
`unverified`.

## Build it in

The LCD mode is a per-module choice in the firmware configuration, not a
runtime setting.  Enabling it is a normal developer build:

1. Install the toolchain and learn the build and flash commands from
   [dev/building.md](../../dev/building.md) and
   [dev/flashing.md](../../dev/flashing.md).
2. Enable the LCD feature in the firmware configuration.  The per-module
   switch and the module's source file are named in the maintainer
   [code map](../../dev/code-map.md).
3. Build the image for your board and flash it.
4. The mode now appears in the `m` list; its number depends on which other
   modes that image has (next section).

The firmware configuration is the only place the mode is selected — no
runtime command adds it.  The feature is disabled in the shipped v4 and v3
feature sets.  Build success with the LCD feature enabled is `unverified` in
this tree.

## The mode menu number is build-dependent

The mode menu lists only the protocols compiled into that image, numbered in
order.  The LCD number therefore depends on which other modes are present.

The table below is the ordering in a build with **every** optional protocol
enabled; it is this set's reference build.  In a leaner build the number is
lower — always read your own `m` output.

| Menu entry | Mode |
|---|---|
| 1 | HiZ |
| 2 | 1WIRE |
| 3 | UART |
| 4 | I2C |
| 5 | SPI |
| 6 | 2WIRE |
| 7 | 3WIRE |
| 8 | KEYB |
| **9** | **LCD** |
| 10 | PIC |
| 11 | DIO |
| x | exit(without change) |

Read the actual menu (`m`) in your build — do not assume 9.

## Entry

Two ways to enter:

1. **Menu:** type `m`, read the list, then answer the numbered prompt with
   the LCD entry.
2. **Direct:** type `m` followed by the LCD number, for example `m9` in the
   all-protocols build.

The LCD mode has **no setup prompts** — unlike UART, it asks nothing.  Its
only setup is to force the pins to high impedance, so the mode always enters
in HiZ.  Example (all-protocols build, menu form):

```
HiZ>m
1. HiZ
2. 1WIRE
3. UART
4. I2C
5. SPI
6. 2WIRE
7. 3WIRE
8. KEYB
9. LCD
10. PIC
11. DIO
x. exit(without change)

(1)>9
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
LCD>
```

The direct form skips the menu and the `Clutch disengaged!!!` /
finish-setup lines and goes straight to `Ready`:

```
HiZ>m9
Ready
LCD>
```

Either way the prompt becomes `LCD>`.

## Defaults

| Setting | Default |
|---|---|
| pins | high impedance (HiZ) |
| setup prompts | none |
| register select | data after `W` (and after `}` / `]`); before `W` the bus is not driven, so the setting has no effect |
| display lines | chosen by macro `(2)`, default Multiple |

The mode adds no settings of its own; what `i` and `v` print is in the next
section.

## Status output (`i` and `v`)

In LCD mode `i` prints the version block **and** the standard status block:
the config words, the pin/voltage table, the supply / pull-up / VPU /
open-drain / bit-order state, the current bit width, and the AUX assignment.
LCD has no per-mode settings, so it adds no values of its own; an empty
settings line and the closing `*----------*` separator still follow.  In HiZ,
`i` prints only the version block.

`v` prints only the pin/voltage table — the same table the status block
contains.

The pin-state row inside that table is **not** an LCD or SPI name row: the
mode prints the I2C mode's four-label string.  The labels differ by board:

| Board | Four labels in the pin-state row |
|---|---|
| v4 | `-`, `-`, `SCL`, `SDA` |
| v3 | `SCL`, `SDA`, `-`, `-` |

Treat the four labels as not meaningful here; the active signals are MOSI,
CLK and CS ([wiring.md](wiring.md)).  The quirk and its recovery are in
[edge-cases.md](edge-cases.md#pin-labels-look-like-i2c).

## `W` — power the adapter

Sending `W` starts the on-board supplies and runs the mode's setup, which
configures the SPI pins and prints `This mode requires an adapter`.  Example:

```
LCD>W
POWER SUPPLIES ON
This mode requires an adapter
Clutch engaged!!!
LCD>
```

After `W` the supplies are engaged and the SPI pins are driven, so writes
reach the shift register (see [wiring.md](wiring.md) for the adapter's supply
caveat).  `W` is also what selects the data register (see
[commands.md](commands.md)).

The supplies are checked before the setup runs.  If a rail reads low, `W`
prints the failure and **does not** run the SPI setup or the adapter reminder
(see [edge-cases.md](edge-cases.md#w-supply-failure)).

The `This mode requires an adapter` line is the mode's own reminder that it
needs the 74HCT595 board; it is printed every time the supplies come up
successfully, not only the first time.

## Supply release (`w`)

```
LCD>w
Clutch disengaged!!!
POWER SUPPLIES OFF
LCD>
```

`w` runs the mode cleanup (it disables and detaches the SPI peripheral,
deassigns the SPI outputs and clears open-drain) and switches the regulated
supplies off.  It does **not** run the board reset, so the pin direction
registers keep the state `W` gave them: **MOSI, CLK and CS may stay driven**
until a mode change or reset.  Use `m` (any mode change) or `#` for a full
high-impedance state; `W` can be sent again to re-engage.

If the adapter takes its supply from the Bus Pirate VCC pin, `w` cuts that
supply and the display clears.

## Leaving the mode

Change mode with `m` as usual.  Leaving runs the mode cleanup and then the
board reset: the supplies switch off, the bus lines become inputs, and the
pull-ups are released (the generic reset detail is in
[generic usage](../../generic/usage.md#reset-and-the-idle-state)).

Because the board reset switches the regulated VCC off, what the display does
depends on its power source:

| Adapter supply | On leaving the mode |
|---|---|
| Bus Pirate VCC pin | power is cut; the display clears |
| its own supply | the display keeps its last content |

There is no persistent state in the Bus Pirate to clear.  On re-entry the
mode is in HiZ again; send `W`, then `(2)` to power and re-initialise the
display.

## v3 / v4

The entry sequence, the menu, the defaults, and the `W` message are the same
in both builds (the strings are identical).  The v3 build is **not verified**.
