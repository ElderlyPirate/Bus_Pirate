# DIO — reading the pins

Part of the DIO documentation set (`docs/protocol/dio/`).  This page owns the
two read commands, `r` and `v`, and the write-with-read question.  The write
values are in [pin-control.md](pin-control.md); the single-pin `@` read is in
[aux-pin.md](aux-pin.md).

## `r` — the five levels

`r` reads the five signal pins and prints their levels as one value.  After
the [worked session](pin-control.md#worked-session) (`CLK` an output high, the
other four inputs, supply on) only `CLK` is driven; the other bits are
whatever the floating inputs read, so the result need not be `0x04`.  With
the four free inputs reading low, it prints:

```
DIO>r
READ: 0x04
DIO>
```

The value uses the same compact pin layout as the write values:

| Bit | Value | Pin | 1 | 0 |
|---|---|---|---|---|
| 4 | `0x10` | `AUX` | high | low |
| 3 | `0x08` | `MOSI` | high | low |
| 2 | `0x04` | `CLK` | high | low |
| 1 | `0x02` | `MISO` | high | low |
| 0 | `0x01` | `CS` | high | low |

`r` reports **levels only**.  It does not report a direction, the supply or
the pull-ups; the five-bit value has bits 7–5 clear.  With the default
MSB-first order, a default 8-bit read is therefore the whole value.  With
`L` (LSB-first) the printed value is bit-reversed within the width; send `l`
to return to the default.

An output pin reads the level it drives.  An input pin reads the level on the
header pin; with nothing connected it floats, so the bit is indeterminate and
can read either value.  That is a floating input, not a fault.

A 16-bit read is the same byte zero-extended; the default hex display shows it
as a word:

```
DIO>r;16
READ: 0x0004
DIO>
```

The display format (`o`) and the one-shot letters (`rx`, `rd`, `rb`, `rw`)
change only how the value is printed.  Repeat and width are shared grammar;
see [values.md](../../generic/values.md).

## `v` — directions, levels and rails

`v` is the full picture.  In the same state as the session above, on v4 (the
inputs float, so their `H` / `L` entries are examples):

```
DIO>v
Pinstates:
GND     5.0V    3.3V    VPU     ADC     AUX2    AUX1    AUX     CS      MISO    CLK     MOSI
P       P       P       I       I       I       I       I       I       I       O       I
GND     4.98V   3.29V   0.00V   0.00V   L       L       L       L       L       H       L
DIO>
```

The device emits tabs between columns; the spacing above is a fixed-column
transcription, and the real widths depend on your terminal's tab stops.
`Pinstates:` is the title.  The first row of the real output is the
probe-colour legend; it is omitted above.  After the legend come the column
labels, the directions and the levels:

| Row | Content |
|---|---|
| 1 | column labels: the rails, then the header pins |
| 2 | direction: `P` fixed power/analogue, `I` input, `O` output |
| 3 | rail voltages, then `H` / `L` pin levels |

On v4 the five direct I/O pins are the last five columns (`AUX`, `CS`, `MISO`,
`CLK`, `MOSI`); `AUX2` and `AUX1` are inputs unless the AUX-target path drives
them.  The first five columns are fixed for this table: `GND`, `5.0V`,
`3.3V`, `VPU`, `ADC`.  The rail values shown are examples, and the floating input levels are examples too.

Use `v`, not `r`, when you need directions, the supply rails or the VPU
voltage.  The table is a shared base command; its rows and the v3 column order
are owned by [usage.md](../../generic/usage.md#v-in-hiz).

## Reading an external signal

Make the pin an input first, then read it.  A direction value sets all five
directions; `0x1F` makes all five inputs, so no pin drives the target:

```
DIO>0x1F
WRITE: 0x1F
DIO>r
READ: 0x02
DIO>
```

With the external driver holding `MISO` high, bit 1 is 1.  The other four
inputs float, so their bits may also read 1; here they read low.  If you only
need one pin and it is an AUX target, `@` does the same for that pin and reads
it ([aux-pin.md](aux-pin.md)).

## Write-with-read

A direct I/O value always answers `WRITE:` only.  It never prints a companion
`READ:`.  The mode clears any read-on-write setting when it is entered, and
none of its own commands sets one.  The `{` wrapper reports
`ERROR: command has no effect here`, then the dispatcher's
`Syntax error at char 1`; nothing is enabled.

Use `r` or `v` after the write.  This differs from the binary bitbang layer,
where every pin state or direction byte is answered with the five levels
([binary/commands.md](../binary/commands.md)).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| `r` layout | bits 4–0, 1 = high | same |
| `r;16` | same byte zero-extended | same |
| `v` columns | 10 | 12 (`AUX2`, `AUX1` added) |

The v3 column order is in the shared `v` description
([usage.md](../../generic/usage.md#v-in-hiz)); the v3 table is unverified.
