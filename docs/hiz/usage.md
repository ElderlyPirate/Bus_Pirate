# Worked sessions — HiZ

Part of the HiZ documentation set (`docs/hiz/`). These are example terminal
sessions: the idle prompt, the pin table, the commands HiZ refuses, the
self-test entry, and leaving and returning to HiZ. The shared grammar (line
editing, `?`, `m`, wrappers, macros) is in `docs/generic/`.

## First contact — the idle prompt

On v4 the board prints only the prompt at boot (no banner); on v3 the version
banner is printed at boot as well.

```
HiZ>
```

Idle mode is now active: nothing is driven, no supply and no pull-up is on.
Ask for the version and status with `i`:

```
i
Bus Pirate v4
ElderlyPirate Firmware v8.0 - goo.gl/gCzQnW - <hash> 
[HiZ 1-WIRE UART I2C SPI 2WIRE 3WIRE KEYB PIC DIO]
DEVID:0x1019 REVID:0x0004 (24FJ256GB106 A6)
http://github.com/ElderlyPirate
HiZ>
```

`<hash>` is the git short hash baked in at build time, so it differs per
build. The `[ … ]` line is the enabled mode list. In HiZ `i` prints **no**
status block — the `CFG`/pin/voltage status block is added only outside HiZ.
The fields are explained in `docs/generic/navigation.md`.

## The pin table — `v`

```
v
Pinstates:
12.(Bk)  11.(Wt)  10.(Gy)  9.(Pu)  8.(Bu)  7.(Gn)  6.(Yw)  5.(Or)  4.(Rd)  3.(Br)  2.(Bk)  1.(Wt)*
GND      5.0V     3.3V     VPU     ADC     AUX2    AUX1    AUX     CS      MISO    CLK     MOSI
P        P        P        I       I       I       I       I       I       I       I       I
GND      0.00V    0.00V    0.00V   0.00V   L       L       H       L       H       H       H
HiZ>
```

Every I/O pin shows direction `I` (input): nothing is driven. The last row is
the measured rail voltages followed by the read-back pin states; with nothing
connected the pin states float. The row meanings are in
[overview.md](overview.md); the general table format is in
`docs/generic/usage.md`.

(\* the `1.` label of pin 1 is garbled by a known firmware string defect; the
values are unaffected — see `docs/generic/edge-cases.md`.)

## Commands HiZ refuses

The elements that need an active bus are refused with
`Command not used in this mode`.

Supplies on (`W`):

```
W
Command not used in this mode
HiZ>
```

Pull-ups (`P`) and PWM (`g`) behave the same way:

```
P
Command not used in this mode
HiZ>
g
Command not used in this mode
HiZ>
```

The v4 VPU pull-up source command `e` is also refused, and because it raises an
error it is followed by a syntax error:

```
e2
Command not used in this mode
Syntax error at char 1
HiZ>
```

Switch to a mode to use any of these. The exact availability rules are in
[commands.md](commands.md).

## Bus operations have no effect

A bus operation in HiZ reports no-effect. Here `(0)` asks for the mode macros,
which HiZ does not have:

```
(0)
ERROR: command has no effect here

Syntax error at char 3
HiZ>
```

The same applies to `r`, `[`, `]`, `^`, `!` and a bare value (`0x55`). See
[commands.md](commands.md) and `docs/generic/edge-cases.md`.

## The self-test — `~` (HiZ only)

```
~
Disconnect any devices
Connect (ADC to +3.3V)
Space to continue
```

The test runs after a keypress, drives the Bus Pirate's own pins and supply,
and ends with `Found <n> errors.` (`0` means no check failed). **Disconnect any
target first.** The full run, the jumper setup and how to read a failure are in
[`docs/self-test/`](../self-test/README.md).

Outside HiZ, `~` is refused:

```
I2C>~
Self-test in HiZ mode only
I2C>
```

## Leaving HiZ and returning

Enter a mode with the mode menu (`m`) or directly (`m<N>`); the target mode's
setup prompts run. To come back to HiZ, use `m1`:

```
UART>m1
Ready
HiZ>
```

The menu form additionally prints the clutch line:

```
UART>m
1. HiZ
…
10. DIO
x. exit(without change)

(1)>1
Clutch disengaged!!!
Ready
HiZ>
```

A mode change into HiZ releases the pull-ups and turns the supplies off, so the
board is left idle.

### Reset — `#`

```
#
RESET
Bus Pirate v4
ElderlyPirate Firmware v8.0 - goo.gl/gCzQnW - <hash> 
[HiZ 1-WIRE UART I2C SPI 2WIRE 3WIRE KEYB PIC DIO]
DEVID:0x1019 REVID:0x0004 (24FJ256GB106 A6)
http://github.com/ElderlyPirate
HiZ>
```

On v3 `#` instead performs a real hardware reset/reboot. See
[overview.md](overview.md) for what a reset does on each board.

## Related docs

- [README.md](README.md) — the set map.
- [overview.md](overview.md) — pin-state model and entering/leaving.
- [commands.md](commands.md) — availability rules.
- [edge-cases.md](edge-cases.md) — failure modes.
- [../generic/usage.md](../generic/usage.md) — base-layer worked sessions.
- [../self-test/README.md](../self-test/README.md) — the `~` self-test.