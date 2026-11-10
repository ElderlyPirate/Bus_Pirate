# DIO — pin control (values and usage)

Part of the DIO documentation set (`docs/protocol/dio/`).  This page owns the
write value format, the complete bit map, and worked sessions; read-back
(`r`, `v` and write-with-read) is in [read-back.md](read-back.md).

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
A write answers `WRITE:` only; DIO clears any read-on-write setting when the
mode is entered.  Read the pins with `r` or `v` — see
[read-back.md](read-back.md).

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
releases the pull-ups.  To keep power on while changing a pin, include bit 6:
`0xC0 | pins` leaves the pull-ups off, and `0xE0 | pins` also enables them.
Send the complete set of bits you want — a state value does not preserve the
previous power or pull-up state.

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
| `0xC4` | state write: power on, pull-up off, `CLK` high, the rest low |
| `0xFF` | state write: power on, pull-up on, all five pins high |
| `0xE4` | state write: power on, pull-up on, `CLK` high, the rest low |

To build a value yourself: for a **direction** write, OR the `0x01`–`0x10`
bit of every pin you want as an **input** (1 = input, 0 = output).  For a
**state** write, start from `0x80` (selects the state action), OR `0x40` for
power on and `0x20` for pull-up on, then OR each pin you want **high**.

## Worked session

Enter DIO on v4, engage the supply, make all five pins outputs, then raise
`CLK` while keeping power on and the pull-ups off:

```
HiZ>m10
Ready
DIO>W
POWER SUPPLIES ON
Clutch engaged!!!
DIO>0x00
WRITE: 0x00
DIO>0xC4
WRITE: 0xC4
DIO>
```

`0xC4` is `0x80` (state write) + `0x40` (power on) + `0x04` (`CLK` high).
`0xE4` does the same with the pull-ups on; the pull-ups need a voltage on VPU
(see [wiring.md](wiring.md) §Power and pull-ups).

## Reading the pins

Read-back has its own page: [read-back.md](read-back.md) owns `r` (the
compact level value), `v` (directions, levels and rails), reading an external
signal, and write-with-read.

## Value reference

| Input | Action | Notes |
|---|---|---|
| `0x00`–`0x7F` | set pin directions | bits 4–0 = `AUX` `MOSI` `CLK` `MISO` `CS`; 1 = input, 0 = output; bits 6–5 ignored |
| `0x80`–`0xFF` | set pin states | bit 6 = power, bit 5 = pull-up, bits 4–0 = pin levels (1 = high, 0 = low) |
| `r` | read the pin levels | bits 4–0, 1 = high; levels only — see [read-back.md](read-back.md) |
| `v` | pin directions, levels and rails | shared base command |
| `W` / `w` | supply on / off | `w` leaves directions, levels and pull-ups as set |
| `P` / `p` | pull-ups on / off | optional; shared base command |
| `(n)` | mode macros | none — reports an error, see [edge-cases.md](edge-cases.md) |
| `<n>` | user macros | shared; runs a stored sequence (see [commands.md](../../generic/commands.md) §Macros) |
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
| `r` read-back | levels on bits 4–0, 1 = high | levels on bits 4–0, 1 = high |

The v3 column is **not verified**.