# DIO — pin control (values and usage)

Part of the DIO documentation set (`docs/dio/`).  This page owns the write
value format, the complete bit map, worked sessions, reading the pins, and
the value reference.

## How a value reaches the pins

The terminal's normal value entry *is* the DIO write.  Type a decimal, `0x…`
hex, or `0b…` binary value and press Enter; the shared grammar is in
`docs/generic/` — see [commands.md](../../generic/commands.md) §Value entry
grammar.  DIO passes the value to the pin-control function, which uses the
top bit to choose one of two actions:

| Top bit | Value range | Action |
|---|---|---|
| bit 7 = 0 | `0x00`–`0x7F` | set pin **directions** |
| bit 7 = 1 | `0x80`–`0xFF` | set pin **states** (plus power / pull-up) |

Only the low byte is used.  A value above `0xFF` is truncated to its low byte;
`0x100` therefore behaves as `0x00`.  The firmware echoes the value you sent:

```
DIO>0x00
WRITE: 0x00
DIO>
```

The `WRITE:` line echoes the value you sent, **not** the resulting pin state.
DIO does not print the pin state after a write (unlike the binary layer, which
answers each pin command with a state byte).  Read the pins with `v`.

## Direction values (`0x00`–`0x7F`)

Bits 4–0 select each pin's direction; bits 6–5 are ignored on a direction
value.

| Bit | Value | Pin | 1 | 0 |
|---|---|---|---|---|
| 4 | `0x10` | `AUX` | input (Hi-Z) | output |
| 3 | `0x08` | `MOSI` | input | output |
| 2 | `0x04` | `CLK` | input | output |
| 1 | `0x02` | `MISO` | input | output |
| 0 | `0x01` | `CS` | input | output |

The bit order matches the binary bitbang layer's pin-direction value
(high-to-low `AUX` `MOSI` `CLK` `MISO` `CS`).  `0x00` makes all five outputs;
`0x1F` makes all five inputs.

## State values (`0x80`–`0xFF`)

Bit 7 selects the state action and is not itself a pin.  Bits 6–5 control the
on-board supply and the pull-up resistors; bits 4–0 set the pin levels.

| Bit | Value | Controls | 1 | 0 |
|---|---|---|---|---|
| 7 | `0x80` | selects a state write | — | — |
| 6 | `0x40` | `POWER` (on-board supply) | on | off |
| 5 | `0x20` | `PULLUP` (pull-up resistors) | on | off |
| 4 | `0x10` | `AUX` | high | low |
| 3 | `0x08` | `MOSI` | high | low |
| 2 | `0x04` | `CLK` | high | low |
| 1 | `0x02` | `MISO` | high | low |
| 0 | `0x01` | `CS` | high | low |

The bit order matches the binary bitbang state value
(`1|POWER|PULLUP|AUX|MOSI|CLK|MISO|CS`).

**Every state value carries the `POWER` and `PULLUP` bits**, so a state value
with bit 6 clear turns the on-board supply off, and a value with bit 5 clear
releases the pull-ups.  To keep power on while changing a pin, include bit 6
in each value (`0xE0 | pins`).

A pin takes its level only after it is an output (its direction bit = 0).
Setting a state bit for a pin that is still an input has no visible effect
until the pin is made an output.

### Example values

| Value | Effect |
|---|---|
| `0x00` | direction write: all five pins outputs |
| `0x1F` | direction write: all five pins inputs (Hi-Z) |
| `0x08` | direction write: `MOSI` input, the other four outputs |
| `0x87` | state write: power off, pull-up off; `CS`, `MISO`, `CLK` high; `MOSI`, `AUX` low |
| `0xE0` | state write: power on, pull-up on, all five pins low |
| `0xFF` | state write: power on, pull-up on, all five pins high |
| `0xE4` | state write: power on, pull-up on, `CLK` high, the rest low |

To build a value yourself: for a **direction** write, OR the `0x01`–`0x10`
bit of every pin you want as an **input** (1 = input, 0 = output).  For a
**state** write, start from `0x80` (selects the state action), OR `0x40` for
power on and `0x20` for pull-up on, then OR each pin you want **high**.

## Worked session

Enter DIO on v4, engage the supply, make all five pins outputs, then raise
`CLK` while keeping power and the pull-ups on:

```
HiZ>m10
Ready
DIO>W
POWER SUPPLIES ON
Clutch engaged!!!
DIO>0x00
WRITE: 0x00
DIO>0xE4
WRITE: 0xE4
DIO>
```

## Reading the pins

`v` is the reliable way to read the pins.  It prints one line per row — pin
labels, then each pin's direction (`I` = input, `O` = output), then each pin's
level (`H` = high, `L` = low) — alongside the rails.  Continuing the session
above:

```
DIO>v
Pinstates:
GND   5.0V   3.3V   VPU   ADC   AUX2  AUX1  AUX   CS    MISO  CLK   MOSI
P     P      P      I     I     I     I     O     O     O     O     O
GND   4.98V  3.29V  0.00V 0.00V L     L     L     L     L     H     L
DIO>
```

The direction row shows `O` for all five DIO pins; the state row shows `CLK`
high and the other pins low.  (The color row is omitted here; the rail
readings are illustrative of the `W` supply, the pin readings of the values
just written.  `v` is the shared base command — see
[commands.md](../../generic/commands.md) §Elements.)

To read an external signal, make the pin an input first with a direction
value (for example `0x02` makes `MISO` an input), then read its level with
`v`.

`r` runs the mode's read, which returns the raw hardware port register rather
than the compact byte the write values use.  Two cautions apply:

- the returned value uses the hardware port bit positions, **not** the compact
  bit positions the write values use, so it does not read back the byte you
  wrote;
- a default 8-bit read shows only the low byte of the register.  A 16-bit read
  (`r;16`, see [commands.md](../../generic/commands.md) §Value entry grammar)
  shows the whole register.

For both reasons, use `v` to read the five pins.  The exact `r` value on v4 is
**not verified**.

## Value reference

| Input | Action | Notes |
|---|---|---|
| `0x00`–`0x7F` | set pin directions | bits 4–0 = `AUX` `MOSI` `CLK` `MISO` `CS`; 1 = input, 0 = output; bits 6–5 ignored |
| `0x80`–`0xFF` | set pin states | bit 6 = power, bit 5 = pull-up, bits 4–0 = pin levels (1 = high, 0 = low) |
| `r` | read a raw port register | see [Reading the pins](#reading-the-pins); use `v` instead |
| `v` | pin directions, levels and rails | shared base command |
| `W` / `w` | supply on / off | shared base command |
| `P` / `p` | pull-ups on / off | optional; shared base command |
| `(n)` | mode macros | none — reports an error, see [edge-cases.md](edge-cases.md) |
| `[` `{` `]` `}` `/` `\` `-` `_` `.` `^` `!` | bus wrappers and bit ops | no-ops — report an error |

The binary bitbang command *codes* (`0x00` reset, `0x01` SPI, …) do **not**
apply here: in DIO every value is a pin direction or state, so `0x01` simply
sets `CS` as an input.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| direction / state bit map | same | same |
| `AUX` bit drives | `AUX` pin | `AUX` (`AUX0`) pin |
| `AUX1` / `AUX2` in the value | — | not addressable |
| `r` read-back | the five pins share the sampled port, at hardware bit positions; a default 8-bit read shows only the low byte | the five pins are on a different port than `r` samples, so the value does not follow the header pins; use `v` |

The `r` read-back behaviour is **unverified** on both builds.

The v3 column is **not verified**.