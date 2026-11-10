# LCD — entering the mode and setup

Part of the LCD documentation set (`docs/lcd/`).  This page owns the entry
sequence, the build-dependent menu number, the mode's defaults, and the `W`
setup step.  Shared prompt mechanics are in
[the generic layer](../../generic/README.md).

## Availability — the mode is an optional build

The LCD mode is **not compiled into the default v3 or v4 images**.  It is only
present in a **custom build** that enables the feature.  The build flag lives
in the firmware configuration; build and flash tooling is developer material
(`docs/dev/`, stub).

In an image where LCD is **not** enabled, there is no `LCD` entry in the mode
menu and the mode cannot be entered.  Selecting a number the image does not
have is rejected with `Nonexistent protocol!`.

## The mode menu number is build-dependent

The mode menu lists only the protocols compiled into that image, numbered in
order.  The LCD number therefore depends on which other modes are present.

The table below is the ordering in a build with **every** optional protocol
enabled; it is this set's reference build.  In a leaner build the number is
lower — always read your own `m` output.

| Menu entry | Mode |
|---|---|
| 1 | HiZ |
| 2 | 1-WIRE |
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

Two equivalent ways to enter:

1. **Menu:** type `m`, read the list, then answer the numbered prompt with the
   LCD entry.
2. **Direct:** type `m` followed by the LCD number, for example `m9` in the
   all-protocols build.

The LCD mode has **no setup prompts** — unlike UART, it asks nothing.  Its
only setup is to force the pins to high impedance, so the mode always enters
in HiZ.  Example (all-protocols build, menu form):

```
HiZ>m
1. HiZ
2. 1-WIRE
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

The direct form skips the menu and the `Clutch disengaged!!!` / finish-setup
lines and goes straight to `Ready`:

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
| register select on entry | data (`RS HIGH`, whenever a byte is next written) |
| display lines | chosen by macro `(2)`, default Multiple |

The mode prints no settings block (`i` shows the version banner only), and it
has no speed or output-type option.

## `W` — power the adapter

Sending `W` starts the on-board supply and runs the mode's setup, which
configures the SPI pins and prints `This mode requires an adapter`.  Example:

```
LCD>W
POWER SUPPLIES ON
This mode requires an adapter
Clutch engaged!!!
LCD>
```

After `W` the supplies are engaged and the SPI pins are driven, so writes reach
the shift register (see [wiring.md](wiring.md) for the adapter's supply
caveat).  `w` disables the supplies and releases the SPI interface
(the pins return to high impedance), and `W` can be sent again to re-engage.

The `This mode requires an adapter` line is the mode's own reminder that it
needs the 74HCT595 board; it is printed every time the supplies are started,
not only the first time.

## Leaving the mode

Change mode with `m` as usual; leaving runs the mode cleanup, which disables
the SPI interface and releases the pins.  There is no persistent state to
clear — the display keeps whatever it was last showing until it is reset or
re-initialised.

## v3 / v4

The entry sequence, the menu, the defaults, and the `W` message are the same
in both builds (the strings are identical).  The v3 build is **not verified**.