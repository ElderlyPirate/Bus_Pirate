# LCD — entry, build and setup

Part of the LCD documentation set (`docs/capabilities/lcd/`).  This page owns
the optional build, the build-dependent menu number, the entry sequence, the
mode defaults, the status output (`i` and `v`), and the `W`/`w` supply steps.
Shared prompt mechanics are in [the generic layer](../../generic/README.md).

**Build status:** the v4 all-features build includes the LCD and SPI modules
together and builds; the LCD path is self-contained, so SPI support does not
block it.  The same all-features set on v3 stops at the SMPS check (SMPS is
v4-only).  On v3, enable LCD in the default v3 set; the v3 part has less
program flash, so check the link result.

## Availability — the mode is an optional build

The LCD mode is **not compiled into the default v3 or v4 images**.  It is
present only in a build that enables the LCD adapter in the firmware
configuration.  In an image without it there is no `LCD` entry in the mode
menu, and the mode cannot be entered.

| | v3 | v4 |
|---|---|---|
| default image | no LCD | no LCD |
| all-features set | cannot build: SMPS is v4-only and stops the build | **includes LCD and SPI; builds** |
| own-set build | enable the LCD module in the v3 configuration | enable the LCD module in the v4 configuration |
| program flash | the v3 part is the smaller device; check the link result | the all-features build leaves room |
| hardware | adapter + display; unverified on v3 | adapter + display; unverified |

On v4, the all-features build includes the LCD adapter and is the shortest
route; it also includes SPI, and the two coexist.  On v3 the all-features set
cannot build because it enables SMPS; add the LCD module to the v3 feature set
instead.  Toolchain, build and flash commands are in
[dev/building.md](../../dev/building.md) and
[dev/flashing.md](../../dev/flashing.md).  The mode is selected at build time
only; no runtime command adds it.

## The menu number is build-dependent

The mode menu lists only the protocols compiled into that image, numbered in
order, and `LCD` sits between `KEYB` and `PIC`.  In the all-features v4 build,
LCD is entry **9**.  Other sets number it differently:

| Build | `LCD` | `PIC` | `DIO` |
|---|---|---|---|
| v4 all-features (`KEYB` on) | 9 | 10 | 11 |
| v3 with LCD (`KEYB` off) | 8 | 9 | 10 |
| v4 default (no LCD) | — | 9 | 10 |
| v3 default (no LCD) | — | 8 | 9 |

Consequences:

- On a stock v4 image, `m9` enters **PIC**, not LCD; on a stock v3 image,
  `m9` enters **DIO**.
- An out-of-range direct form, such as `m12` in the all-features build, prints
  `Nonexistent protocol!`; an out-of-range answer at the `m` prompt prints
  `Invalid choice, try again` and repeats the prompt
  ([generic navigation](../../generic/navigation.md#mode-menu--m)).
- Always read your own `m` list; the firmware's menu is authoritative.

## Entry

Two ways to enter:

1. **Menu:** type `m`, read the list, then answer the numbered prompt.
2. **Direct:** type the mode number, for example `m9` in the all-features
   build.

The LCD mode has **no setup prompts**.  The menu form prints the clutch line,
the finish-setup hint and `Ready`; the direct form prints only `Ready`.  The
direct form takes no setup answers: in a build that has LCD at entry 9,
`m9 2` enters the mode and ignores the trailing `2`; select the line mode with
`(2) 1` or `(2) 2` after `W`.

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

The direct form skips the menu and the clutch/finish-setup lines:

```
HiZ>m9
Ready
LCD>
```

Either way the prompt becomes `LCD>` and the mode is parked: the supplies are
off and the SPI pins are not driven.

## Defaults

| Setting | Value |
|---|---|
| pins at entry | high impedance; supplies off |
| setup prompts | none |
| register select | data after `W` (and after `}` / `]`); before `W` the LCD setup has not run, so the selection has no effect on the display |
| display lines | chosen by `(2)`; default `2` (Multiple) |
| display interface | after `(2)`: 4-bit, 5×7 font, display on, cursor on, blink off |
| SPI shift | fixed; not a user setting ([spi-link.md](spi-link.md)) |
| mode settings line | empty |

The mode adds no settings of its own; the shared width, bit order and display
format still exist but should be left at their defaults
([commands.md](commands.md#shared-settings-and-keys)).

## Status output — `i` and `v`

In any non-HiZ mode, `i` prints the version block and the standard status
block.  LCD contributes no settings of its own, so the settings line is empty
and the block ends with the closing `*----------*` separator.  The mode list
in the banner gains `LCD`:

```
[HiZ 1WIRE UART I2C SPI 2WIRE 3WIRE KEYB LCD PIC DIO]
```

The mode-relevant status lines after `W` are:

```
POWER SUPPLIES ON, Pull-up resistors OFF, Normal outputs (H=3.3v, L=GND)
MSB set: MOST sig bit first, Number of bits read/write: 8
a/A/@ controls AUX pin
```

Before `W`, the supply field reads `POWER SUPPLIES OFF`.  The `Normal outputs`
text is the mode's output-type setting: entering the mode selects normal
outputs, and the LCD setup writes the MOSI/CLK/CS open-drain bits off, so the
SPI pins are driven push-pull
([spi-link.md](spi-link.md#output-type)).

`v` prints only the pin/voltage table.  The full banner and table layout are
owned by
[generic/navigation.md](../../generic/navigation.md#version-and-status--i) and
[generic/usage.md](../../generic/usage.md#v-in-hiz).  In LCD the last four
signal-name columns are the board's SPI header order:

| Board | Last four signal-name columns | LCD uses |
|---|---|---|
| v4 | `CS  MISO  CLK  MOSI` | `CS`, `CLK`, `MOSI`; `MISO` unused |
| v3 | `CLK  MOSI  CS  MISO` | `CLK`, `MOSI`, `CS`; `MISO` unused |

The table is tab-separated on the wire, so column spacing follows the
terminal's tab stops.  The `P` warning calls the pull-up supply pin `Vpullup`;
the `v` table labels the same pin `VPU`.

With SMPS feedback active (all-features build), `v` is refused and the pin
table inside `i` is skipped; the refusal message and recovery are in
[generic edge cases](../../generic/edge-cases.md).

## `W` — power the adapter

`W` switches the regulated supplies on, checks both rails, runs the mode's SPI
setup and prints the adapter reminder:

```
LCD>W
POWER SUPPLIES ON
This mode requires an adapter
Clutch engaged!!!
LCD>
```

- The supplies are checked before the setup runs.  If either rail reads low,
  `W` prints the failure and **does not** run the SPI setup or the adapter
  reminder ([edge-cases.md](edge-cases.md#w-supply-failure)).
- `This mode requires an adapter` is a fixed reminder, not detection.  It is
  printed on every successful `W`, whether or not an adapter is connected.
- `W` is still required when the display has its own VDD feed: it runs the SPI
  setup and checks the Bus Pirate rails.
- The setup configures the SPI pins, writes the MOSI/CLK/CS open-drain bits
  off, and selects the **data** register.  On a clean entry, until `W` succeeds
  the SPI port is disabled, so writes and macros cannot shift bytes.
- `W` again re-runs the setup, reselects data and prints the reminder again.
  A display that stayed powered keeps its content.

## `w` — release the supply

```
LCD>w
Clutch disengaged!!!
POWER SUPPLIES OFF
LCD>
```

`w` runs the mode cleanup, then switches the regulated supplies off.  The
cleanup disables the SPI module, releases the MOSI/CLK/MISO/CS pads to inputs,
removes the pin routing and returns the shared width to 8 bits.  It does
**not** run the board reset or change the bit order and display format.

After `w` the link is released, but the adapter's shift register holds its last
latched outputs while the adapter stays powered.  If the display's VDD comes
from the adapter's Bus Pirate rail, the display loses power and clears; if the
display has its own VDD feed, it keeps its content.

To resume: send `W`.  If the display lost power, also send `(2)` to
re-initialise it; an externally powered display that kept its state needs only
`W`.

## Leaving the mode

| Route | What happens |
|---|---|
| `m`, then a mode number | cleanup, board reset, then the new mode |
| `m<N>` | direct change; prints `Ready` only |
| `m1` | returns to HiZ; prints `Ready` |
| `#` | v4: terminal reset, banner and `HiZ>`; v3: reboot |

A mode change and `#` run the board reset, which releases the pins and
switches the regulated rails off.  What the display does depends on its
supply:

| Display VDD source | On leaving the mode |
|---|---|
| the adapter's Bus Pirate rail | power is cut; the display clears |
| a separate display feed | the display keeps its last content |

On re-entry the mode is in HiZ again.  Send `W`; if the display lost power,
send `(2)` to re-initialise it.

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| default build | off | off |
| all-features build | cannot build it (SMPS is v4-only) | includes LCD and SPI |
| menu number in the documented build | 8 (KEYB off) | 9 (KEYB on) |
| mode messages | identical | identical |
| hardware path | unverified on v3 | unverified |

The build path, the menu number and the `v` column order differ; the mode
messages are the same.  The v3 hardware path is **unverified on v3**.
