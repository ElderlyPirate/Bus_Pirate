# Worked sessions — base layer

Part of the generic-layer documentation set (`docs/generic/`).  Sessions are captured on the v4 bench; the development logs
live outside this repository.

## Usage — real terminal sessions

When a transcript shows `Clutch disengaged!!!`, the bus is parked — the mode's
I/O is not driven until `W` starts the supplies and prints `Clutch engaged!!!`
(see [README.md](README.md#glossary)).

### First session — UART (`m3`), supplies, pull-ups, display, volts

Captured on the v4 bench, 2026-11-01 (`m3` setup chain, `W`, `P`, `o1`, `v`):

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
12.(Bk)  11.(Wt)  10.(Gy)  9.(Pu)  8.(Bu)  7.(Gn)  6.(Yw)  5.(Or)  4.(Rd)  3.(Br)  2.(Bk)  1.(Wt)*
GND      5.0V     3.3V     VPU     ADC     AUX2    AUX1    AUX     -       RxD     -       TxD
P        P        P        I       I       I       I       I       I       I       I       O
GND      4.98V    3.29V    2.19V   0.00V   L       L       L       H       H       H       H
UART>
```

(* the `1.` label in the pin legend is garbled by the known string
defect — see [edge-cases.md](edge-cases.md).)

The `m3` setup prompts are UART-mode territory (speed, framing, polarity,
output type, duplex); the base layer only contributes `Clutch disengaged!!!` /
`Ready` at the end of the chain and the `UART>` prompt.  `W` starts the on-board
supplies and prints `Clutch engaged!!!`; `P` enables the pull-ups and warns when
nothing is on the Vpullup pin; `o1` selects HEX and prints `Display format
set`; `v` prints the pin/rail table (the `v` table is described below).

A reader might notice that `P` warns about no voltage while the following `v`
reads 2.19 V on VPU.  They are two separate samples of the same pin: `P` checks
the VPU sense once, right after switching the pull-ups on, and warns when it
reads below a small threshold (about 0.5 V); `v` takes a fresh ADC sample a
moment later.  With nothing driving the Vpullup pin it floats, so the two
readings can differ — the 2.19 V is a floating/leakage voltage, not a connected
supply.

### Mode change → bus → clean exit (UART, HDUART battery)

Captured on the v4 bench (`m`, reply `3`; setup prompts abridged; then `W`, `P`,
and a clean return to HiZ):

```
HiZ>
m
1. HiZ
...
10. DIO
x. exit(without change)

(1)>3
Set serial port speed: (bps)
 1. 300
...
11. Auto-Baud Detection (Activity Required)

(1)>5
Data bits and parity:
 1. 8, NONE *default 
...
(1)>1
Stop bits:
 1. 1 *default
 2. 2
(1)>1
Receive polarity:
...
(1)>1
Select output type:
...
(1)>1
Duplex:
 1. Full (TX+RX) *default
 2. Half (single wire)
(1)>2
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
UART>W
POWER SUPPLIES ON
Clutch engaged!!!
UART>P
Pull-up resistors ON
UART>m
...
(1)>1
Clutch disengaged!!!
Ready
HiZ>
```

The HDUART battery records `PASS mode menu -> UART`, `PASS supplies on (W)`,
`PASS pull-ups toggled (P)`, `PASS clean exit to HiZ`.  The intermediate
per-mode setup prompts are mode territory; only the base-layer framing
(`Clutch disengaged!!!` / `To finish setup…` / `Clutch engaged!!!` / prompt) is
documented here.

### Wrappers and bit ops (2WIRE)

Captured on the v4 bench, 2026-11-01.  `m6` selects 2WIRE and runs its two
setup prompts (`Set speed:`, `Select output type:`); answer each with its
default `1` (~6 kHz, open drain).  `W` then engages the bus, and each
wrapper/bit op is sent one at a time:

```
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

Legend (shared grammar; the bus wording is 2WIRE's):

- `[` — start a transaction (`I2C START BIT`).
- `/` — clock high (`CLOCK, 1`); `\` — clock low (`CLOCK, 0`).
- `-` — drive data high (`DATA OUTPUT, 1`); `_` — drive data low
  (`DATA OUTPUT, 0`).
- `^` — one clock tick (`CLOCK TICKS: 0x01`).
- `]` — stop the transaction (`I2C STOP BIT`).
- `r` — read a value (`READ: <value>`); `r:N` reads N times.

`{` / `}` are the "start with read" / "stop-from-read" variants (see
[commands.md](commands.md) §Transaction wrappers and bit ops).

A read session on the same bench (2WIRE, bus engaged, nothing on the bus), the
read and its framing sent one at a time:

```
2WIRE>[
(\-/_\-)I2C START BIT
2WIRE>/
CLOCK, 1
2WIRE>-
DATA OUTPUT, 1
2WIRE>r
READ: 0x00 
2WIRE>]
(\_/-)I2C STOP BIT
2WIRE>{
(\-/_\-)I2C START BIT
2WIRE>]
(\_/-)I2C STOP BIT
2WIRE>r:2
READ: 0x00 0x00 
2WIRE>
```

`r` reads once (`READ: 0x00`) and `r:2` reads twice (`READ: 0x00 0x00`); `{` is
the read-framed start and its matching `]` is the stop.  With nothing on the bus
the read is a static `0x00` — that is the bench capture, not a device value.

### `W` in a mode (supplies)

```
W
POWER SUPPLIES ON
Clutch engaged!!!
1-WIRE>
```

### `P` with no VPU voltage present

```
P
Pull-up resistors ON
Warning: no voltage on Vpullup pin
PIC>
```

Recovery: supply the Vpullup pin.  On v4 use `e` to route the on-board 3.3 V or
5.0 V rail; on v3 feed the pin externally (see
[commands.md](commands.md) §Feeding the VPU pin).  The warning alone does not
disable the pull-ups.

### `p`

```
p
Pull-up resistors OFF
```

### `e` in HiZ (v4) — refused with a syntax error

```
e2
Command not used in this mode
Syntax error at char 1
HiZ>
```

`e` is only accepted in a non-HiZ mode whose pins are open drain.  On v3 there
is no `e` command at all.

### `v` in HiZ

Captured on the v4 bench; transcribed with the columns aligned (`*`: the `1.`
label of pin 1 is garbled by a known string defect — the capture shows a stray
byte that eats it; see [edge-cases.md](edge-cases.md)):

```
v
Pinstates:
12.(Bk)  11.(Wt)  10.(Gy)  9.(Pu)  8.(Bu)  7.(Gn)  6.(Yw)  5.(Or)  4.(Rd)  3.(Br)  2.(Bk)  1.(Wt)*
GND      5.0V     3.3V     VPU     ADC     AUX2    AUX1    AUX     CS      MISO    CLK     MOSI
P        P        P        I       I       I       I       I       I       I       I       I
GND      0.00V    0.00V    0.00V   0.00V   L       L       H       L       H       H       H
HiZ>
```

`v` draws the pin table: row 1 is the pin colour/name legend, row 2
the signal names, row 3 the pin *dir*ections (`P` = power/analogue fixed,
`I` input, `O` output), row 4 the measured voltages then the pin *state*s
(`H`/`L`).  The last two rows are filled in by the active mode.  The signal
names in row 2 differ per mode (compare the UART session above, where the last
columns read `RxD`/`TxD`).  A `-` in row 2 marks a pin that has no signal name
in the current mode.

### Bootloader jump — `$`

`$` asks for confirmation (prompt `Are you sure? `), then sets the
persistent skip flag, prints `BOOTLOADER`, and resets into the bootloader.
`y`/`Y` proceeds; echoed with a newline.  Flashing procedure: `docs/dev/`
(stub).

### Reset — `#`

- v4: prints `RESET`, forces HiZ, prints the `i` banner.  Tools such as flashrom
  wait for the `HiZ>` prompt after a reset byte.
- v3: prints `RESET` then performs a real hardware reset/reboot.

`#` is **not** in the v4 `?` help; it *is* in v3 help.