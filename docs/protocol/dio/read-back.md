# DIO — reading the pins

Part of the DIO documentation set (`docs/protocol/dio/`).  This page owns the
two read commands, `r` and `v`, and the write-with-read question.  The write
values themselves are in [pin-control.md](pin-control.md).

## `r` — pin levels

`r` reads the five controlled pins and prints their levels as one value.
With `CLK` high and the other four pins low (the
[worked session](pin-control.md#worked-session)), it prints:

```
DIO>r
READ: 0x04
DIO>
```

The value uses the same compact pin layout as the write values, so it can be
compared with the byte you sent:

| Bit | Value | Pin | 1 | 0 |
|---|---|---|---|---|
| 4 | `0x10` | `AUX` | high | low |
| 3 | `0x08` | `MOSI` | high | low |
| 2 | `0x04` | `CLK` | high | low |
| 1 | `0x02` | `MISO` | high | low |
| 0 | `0x01` | `CS` | high | low |

`r` reports **levels only**.  It does not report a pin's direction, the
supply, or the pull-ups.  In a write value, bit 7 = 0 selects a **direction**
byte and bit 7 = 1 a **state** byte; a read returns levels only, so bit 7 is 0
and bits 7–5 read as 0.  A default 8-bit read is therefore the whole value.

A 16-bit read is the same byte zero-extended; the default hex display shows it
as a word:

```
DIO>r;16
READ: 0x0004
DIO>
```

The `o` display mode only changes how the value is presented (hex, decimal,
binary, raw); it does not change the bits.  See
[commands.md](../../generic/commands.md) §Navigation, display and info.

The read follows the actual pin level.  An output pin reads the level it is
driven to.  An input pin with nothing connected is floating, so its bit is
indeterminate.

## `v` — directions, levels and rails

`v` is the full picture.  It prints one row of signal names, one row of
directions (`I` = input, `O` = output, `P` = power/analogue), and one row of
pin levels (`H` = high, `L` = low) alongside the supply rails:

```
DIO>v
Pinstates:
GND   5.0V   3.3V   VPU   ADC   AUX2  AUX1  AUX   CS    MISO  CLK   MOSI
P     P      P      I     I     I     I     O     O     O     O     O
GND   4.98V  3.29V  0.00V 0.00V L     L     L     L     L     H     L
DIO>
```

This continues the [worked session](pin-control.md#worked-session): all five
DIO pins are outputs, `CLK` is high, and the supply is on.  (The colour row is
omitted here; the rail readings are illustrative.  `v` is the shared base
command — see [commands.md](../../generic/commands.md) §Elements.)

Use `v`, not `r`, when you need to see directions, the supply rails or the
VPU voltage.

## Reading an external signal

Make the pin an input first with a direction value (for example `0x02` makes
`MISO` an input), then read it:

- `r` gives the level as a compact value;
- `v` gives the level and the direction in one table.

## Write-with-read

A DIO value always answers `WRITE:` only; it never prints a companion `READ:`.
DIO clears any read-on-write setting when the mode is entered, so a setting
left by another mode cannot make a DIO write read.  If you came from SPI or
3WIRE, where a wrapper can make a write also read, use `r` or `v` here.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| `r` layout | bits 4–0, 1 = high | bits 4–0, 1 = high |
| `r;16` | same byte zero-extended | same byte zero-extended |
| `v` table | 10 columns | 12 columns (`AUX1`, `AUX2` added) |

The v3 column is build-only (unverified).
