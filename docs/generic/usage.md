# Worked sessions — base layer

Part of the generic-layer documentation set (`docs/generic/`).  Every session
here is an example: the commands and replies are the firmware's own strings,
while voltages and pin levels depend on what is connected.

Every session starts at `HiZ>` and the example shows that prompt.  The
sessions are independent; if you are following straight through from the
previous one, run `m1` first to return to HiZ.

## First contact — `i` in HiZ

Wiring: nothing connected; the board is powered from USB only.

```
HiZ>i
Bus Pirate v4
ElderlyPirate Firmware v8.0 - goo.gl/gCzQnW - <hash> 
[HiZ 1WIRE UART I2C SPI 2WIRE 3WIRE KEYB PIC DIO]
DEVID:0x1019 REVID:0x0004 (24FJ256GB106 A6)
http://github.com/ElderlyPirate
HiZ>
```

In HiZ `i` prints the banner and stops.  The fields are explained field by
field in [navigation.md](navigation.md); in short, the banner carries the board
name, the firmware string and build hash, the compiled mode list, the chip ID
and the project URL.

`v` draws the pin and rail table; it is shown in §[`v` in HiZ](#v-in-hiz).

## Enter UART — setup chain, supplies, pull-ups, display, volts

Wiring: USB only; no target.  The UART pins are left unconnected, and the
Vpullup pin has no source, which is why `P` warns.

`m3` selects UART and runs its six setup prompts.  Answer each with its default
`1`; the replies are `Ready`, then the `UART>` prompt.

```
HiZ>m3
Set serial port speed: (bps)
 1. 300
 2. 1200
 3. 2400
 4. 4800
 5. 9600
 6. 19200
 7. 38400
 8. 57600
 9. 115200
10. Input Custom BAUD
11. Auto-Baud Detection (Activity Required)

(1)>1
Data bits and parity:
 1. 8, NONE *default 
 2. 8, EVEN 
 3. 8, ODD 
 4. 9, NONE

(1)>1
Stop bits:
 1. 1 *default
 2. 2

(1)>1
Receive polarity:
 1. Idle 1 *default
 2. Idle 0

(1)>1
Select output type:
 1. Open drain (H=Hi-Z, L=GND)
 2. Normal (H=3.3V, L=GND)

(1)>1
Duplex:
 1. Full (TX+RX) *default
 2. Half (single wire)

(1)>1
Ready
UART>W
POWER SUPPLIES ON
Clutch engaged!!!
UART>P
Pull-up resistors ON
Warning: no voltage on Vpullup pin
UART>o1
Display format set
UART>v
Pinstates:
12.(Bk)  11.(Wt)  10.(Gy)  9.(Pu)   8.(Bu)   7.(Gn)   6.(Yw)   5.(Or)   4.(Rd)   3.(Br)   2.(Bk)   1.(Wt)
GND      5.0V     3.3V     VPU      ADC      AUX2     AUX1     AUX      -        RxD      -        TxD
P        P        P        I        I        I        I        I        I        I        I        O
GND      4.98V    3.29V    2.19V    0.00V    L        L        L        H        H        H        H
UART>
```

- `W` starts the on-board rails and prints `Clutch engaged!!!`.
- `P` enables the pull-ups; with nothing on Vpullup it warns but leaves them
  on.  `P` samples the pin once, right after switching them on, and warns below
  about 0.5 V.  `v` takes a fresh sample a moment later, so a floating pin can
  show a stray voltage (the `2.19V` above) instead of a connected supply.
- `o1` re-selects HEX, the default (no change); `v` is independent of the
  display format and shows the UART signal names.  UART's `TxD` is the `MOSI`
  column and is an output; `RxD` is `MISO`.

The row layout of the `v` table is explained in §[`v` in HiZ](#v-in-hiz).

The setup prompts are UART-mode territory; the base layer contributes the
`Ready`, `Clutch engaged!!!` and prompt framing.

## Mode change and clean exit — menu form

Wiring: none beyond the USB link.  This session only shows the menu framing;
the setup answers are abridged.

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
9. PIC
10. DIO
x. exit(without change)

(1)>3
Set serial port speed: (bps)
...
(1)>5
...
(1)>1
...
(1)>1
...
(1)>1
...
(1)>1
...
(1)>2
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
UART>W
POWER SUPPLIES ON
Clutch engaged!!!
UART>m
...
(1)>1
Clutch disengaged!!!
Ready
HiZ>
```

The menu form prints `Clutch disengaged!!!`; the non-HiZ exit also prints
the `To finish setup…` line.  Choosing `1` (HiZ) omits that line because
there is no bus to power.  In both cases the mode starts parked; `W` starts
the supplies and runs the mode's bus setup (it does not repeat the setup
prompts).  DIO is the exception: its pin writes work without `W` (see
[DIO](../protocol/dio/README.md)).

A direct change runs the same entry prompts but skips the announcement:

```
HiZ>m3 5 1 1 1 1 2
UART (spd brg dbp sb rxp hiz dup)=( 4 416 0 0 0 1 1 )
Ready
UART>
```

The six answers are speed, data bits/parity, stop bits, polarity, output type
and duplex; an incomplete set reopens the prompts.  The summary prints stored
settings, not the menu numbers.  Six answers produce seven fields because
`brg` is computed, not typed:

| Token | Meaning | Values |
|---|---|---|
| `spd` | speed menu entry, zero-based | `0`–`8` for the nine preset rates; `9` for a custom rate or raw BRG |
| `brg` | baud-rate generator value computed from the speed settings | `34` is the 115200 bps preset, `416` the 9600 bps preset |
| `dbp` | data bits and parity, zero-based | `0` 8N, `1` 8E, `2` 8O, `3` 9N |
| `sb` | stop bits, zero-based | `0` one, `1` two |
| `rxp` | receive polarity, zero-based | `0` idle 1, `1` idle 0 |
| `hiz` | output type | `1` open drain, `0` normal |
| `dup` | duplex | `0` full, `1` half |

`hiz` is the output-type answer, not the HiZ mode: `1` means the high state is
high-impedance (open drain).

## Wrappers and bit ops (2WIRE)

Wiring: 2WIRE is an open-drain bus.  This example has nothing connected and no
Vpullup source, so the data line floats; the reads show one possible result, not
a device value.  For a real target, feed Vpullup, answer open drain in the
setup, and use `P`.

`m6 2 1` selects 2WIRE at the second speed (`~60kHz`) with open-drain
outputs.  `R2W` is the firmware's settings-header prefix for `2WIRE`; the two
answers are speed and output type, decoded in the 2-Wire entry page
([entry and setup](../protocol/wire/2wire/entry-and-setup.md), §The one-line
settings form).

```
HiZ>m6 2 1
R2W (spd hiz)=( 1 1  )
Ready
2WIRE>W
POWER SUPPLIES ON
Clutch engaged!!!
2WIRE>[
(\-/_\-)I2C START BIT
2WIRE>/
CLOCK, 1
2WIRE>-
DATA OUTPUT, 1
2WIRE>_
DATA OUTPUT, 0
2WIRE>^
CLOCK TICKS: 0x01
2WIRE>\
CLOCK, 0
2WIRE>]
(\_/-)I2C STOP BIT
2WIRE>
```

| Input | Meaning | Reply |
|---|---|---|
| `[` | start a transaction | `(\-/_\-)I2C START BIT` |
| `/` | clock high | `CLOCK, 1` |
| `\` | clock low | `CLOCK, 0` |
| `-` | data high | `DATA OUTPUT, 1` |
| `_` | data low | `DATA OUTPUT, 0` |
| `^` | one clock tick | `CLOCK TICKS: 0x01` |
| `]` | stop | `(\_/-)I2C STOP BIT` |

The wrapper characters are shared; the 2WIRE firmware displays the shared I2C
wrapper wording (`I2C START BIT` / `I2C STOP BIT`).  Other modes print their
own start/stop messages.

A read session, one command at a time:

```
2WIRE>{
(\-/_\-)I2C START BIT
2WIRE>r
READ: 0x00 
2WIRE>]
(\_/-)I2C STOP BIT
2WIRE>r:2
READ: 0x00 0x00 
2WIRE>
```

`r` reads once (`READ: 0x00`); `r:2` reads twice.  `{` is the read-framed start;
either `]` or `}` stops it, and both run the same stop.  Reads do not need an
active transaction, so `r:2` after the stop is an ordinary unframed read.  With
nothing on the bus the data line floats, so the value can read either way; the
example shows `0x00`.

## `v` in HiZ

Wiring: nothing connected.  `v` works in every mode; in HiZ it uses the default
signal names and every I/O pin is an input.

```
HiZ>v
Pinstates:
12.(Bk)  11.(Wt)  10.(Gy)  9.(Pu)   8.(Bu)   7.(Gn)   6.(Yw)   5.(Or)   4.(Rd)   3.(Br)   2.(Bk)   1.(Wt)
GND      5.0V     3.3V     VPU      ADC      AUX2     AUX1     AUX      CS       MISO     CLK      MOSI
P        P        P        I        I        I        I        I        I        I        I        I
GND      0.00V    0.00V    0.00V    0.00V    L        L        H        L        H        H        H
HiZ>
```

The device emits tabs between columns, so the exact spacing depends on your
terminal's tab stops; the table above is transcribed with fixed columns.  The
same layout is used by the `i` status block.

The table is four rows:

| Row | Content |
|---|---|
| 1 | column label and colour legend |
| 2 | signal name in the current mode; `-` means no name |
| 3 | direction: `P` fixed power/analogue, `I` input, `O` output |
| 4 | rail voltages for the first five columns, then `H` / `L` pin states |

The colour codes in row 1 are the probe-cable colours:

| Code | Colour | Code | Colour |
|---|---|---|---|
| `Bk` | black | `Gn` | green |
| `Wt` | white | `Yw` | yellow |
| `Gy` | grey | `Or` | orange |
| `Pu` | purple | `Rd` | red |
| `Bu` | blue | `Br` | brown |

The last four signal names come from the active mode; HiZ uses the default SPI
names `CS`, `MISO`, `CLK`, `MOSI`.  In HiZ every I/O pin is an input, so the
`H` / `L` entries are floating levels, not driven states — your values will
differ from the example.

On v3 the table has ten columns in this order: `GND`, `3.3V`, `5.0V`, `ADC`,
`VPU`, `AUX`, `CLK`, `MOSI`, `CS`, `MISO`.  Its legend numbers the ten columns
`1.(Br)` through `9.(Wt)` and then `0.(Bk)`; the `0` is the firmware's label for
the tenth column (`MISO`), not a typo and not a separate pin.  The v3 table is
not verified on v3 hardware.

## Reset and the idle state

Wiring: none.  On v4 `#` is a full terminal reset:

```
HiZ>#
RESET
Bus Pirate v4
ElderlyPirate Firmware v8.0 - goo.gl/gCzQnW - <hash> 
[HiZ 1WIRE UART I2C SPI 2WIRE 3WIRE KEYB PIC DIO]
DEVID:0x1019 REVID:0x0004 (24FJ256GB106 A6)
http://github.com/ElderlyPirate
HiZ>
```

It returns to HiZ and resets the board state: the bus lines, `CS` and `AUX`
become inputs, the pull-ups are released, the supplies switch off, the AUX
assignment returns to `A0` (the `AUX` pin), the display format returns to HEX,
the bit order returns to MSB-first, and the width returns to 8 bits.  Anything
typed after the `#` on the same line is discarded.

On v3 `#` prints `RESET` and performs a real hardware reset; the startup banner
appears as the firmware reboots.

`m1` returns to HiZ and runs the same board-state reset but does not restore
the display format and prints `Ready` instead of the banner.  Neither `#` nor
`m1` can be typed inside BASIC: the interpreter receives every line, so both
are syntax errors there.  Type `EXIT` first.  On v4 the reset also releases
`AUX1` / `AUX2` and clears their output routing, so every header I/O pin is an
input afterwards.

The binary return (`0x0F`) and the SUMP exit also land in HiZ with the banner.
On v4 the binary return runs the same terminal reset as `#`; on v3 it performs
a hardware reset.

## Bootloader jump — `$`

Wiring: none beyond the USB/serial link.  Close any terminal or program that
holds the port before the flash; the bootloader needs the port.

`$` asks for confirmation, then hands the board to the resident bootloader:

```
HiZ>$
Are you sure? y
BOOTLOADER
```

The firmware sets a persistent flag so the bootloader stays in its loader
instead of jumping back to the application.  On v4 the board re-enumerates as a
different USB device, so the port number can change.  On v3 the flag is only
understood by the current 4.5 loader; an older loader ignores it and returns to
the application, so use the PGC/PGD jumper instead.  This is not verified on v3
hardware.

Cancel with any other byte:

```
HiZ>$
Are you sure? n
HiZ>
```

The prompt reads exactly **one** byte; `y` or `Y` accepts and anything else
cancels.  Type `y` or `n` on its own.  If you type `no` and press Enter, the
`n` cancels and the `o` plus the Enter are then handled as prompt input — `o`
opens the display-format menu.

A `$` byte inside a SUMP session exits the analyzer; it does not request the
bootloader.  Use `$` at the terminal prompt for that.  The flashing procedure
is [../dev/flashing.md](../dev/flashing.md).
