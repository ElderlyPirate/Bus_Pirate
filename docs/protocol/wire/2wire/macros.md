# 2-Wire — macros

Part of the [2-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  This page owns the `(0)` macro list and
the ISO 7816-3 ATR macros, which are 2-Wire's headline extra.  The smart-card
context behind them is in [smartcard.md](smartcard.md).

## The macro list — `(0)`

```
2WIRE>(0)
 0.Macro menu
 1.ISO7816-3 ATR
 2.ISO7816-3 parse only
2WIRE>
```

| `(#)` | Action |
|---|---|
| `(0)` | print the macro menu |
| `(1)` | ISO 7816-3 ATR: reset the card, then parse the reply |
| `(2)` | ISO 7816-3 parse only: parse a reply you drove yourself |

The firmware prints the menu entries without a space (`1.ISO7816-3 ATR`), but
its banner and reply header print the same name with a space (`ISO 7816-3 ATR`,
`ISO 7816-3 reply`).  This set writes `ISO 7816-3` in prose.

There is **no bus-reset macro** in 2-Wire.  The `BUS RESET` string belongs to
the 1-Wire mode, not here; the only firmware reset available in 2-Wire is the
base layer's `#`.

## Macro `(1)` — ISO 7816-3 ATR

`(1)` prints a two-line banner, runs the CS reset sequence, then parses the
four returned values:

```
2WIRE>(1)
ISO 7816-3 ATR (RESET on CS)
RESET HIGH, CLOCK TICK, RESET LOW
ISO 7816-3 reply (uses current LSB setting): 0x3B 0x08 0x00 0x00
Protocol: unknown
Read type: to end
Data units: 2Data unit length (bits): 128
2WIRE>
```

**The `Data units` line is run together as shown.**  The firmware prints
`Data units: 2` and `Data unit length (bits): 128` without a newline between
them; this is a firmware formatting defect, not a transcription error.  The
four printed values are examples.

The reset is on **CS**, not on the bus lines.  The banner's three words name
the middle of the sequence; the macro's full edge order on CS is:

1. drive CS low and hold about 255 µs,
2. drive CS high — the banner's `RESET HIGH`,
3. advance one clock tick on SCL (CLK) — the banner's `CLOCK TICK`,
4. drive CS low — the banner's `RESET LOW`,

then read the reply.  The read happens after the final CS low, so the card's
reset is asserted while the four values are sampled.  The macro **leaves CS
low** after it finishes.  See [smartcard.md](smartcard.md) for the contact
mapping and the release step.

**The four values are not a valid ATR.**  `(1)` samples them while the card's
reset is asserted, so a card cannot answer; this is a known firmware
limitation.  Do not use the decoded protocol, read-type or data-unit fields as
card data.

## Macro `(2)` — parse only

`(2)` runs only the parse step: it reads four values and decodes them, without
driving the reset.  Use it when you have triggered the ATR yourself (for
example by hand-driving CS and CLK) or when you want to re-read a reply.

## The parse

Both macros read **four** values from SDA (MOSI) and print them as hex on one
line after the header `ISO 7816-3 reply (uses current LSB setting): `;
each value is bit-reversed when LSB-first order is selected (`L`).  The `(1)`
example above shows the full line.

The parse assumes the default **8-bit width**; if `;N` has set another width,
each "value" is N bits and the decode below no longer matches.  The decoded
fields come from the first two values.  Bits are numbered 0
(least-significant) to 7 (most-significant).  The first value carries the
protocol field in its high nibble; the second value carries all three bit
fields:

| Second value bits | Mask | Field |
|---|---|---|
| 7 | `0x80` | read type |
| 6–3 | `0x78` | data units |
| 2–0 | `0x07` | data unit length |

### Protocol — high nibble of the first value

| First value, high nibble | Printed |
|---|---|
| `8` | `serial` |
| `9` | `3 wire` |
| `A` | `2 wire` |
| `F` | `RFU` |
| anything else | `unknown` |

Printed after `Protocol: `.  A standard ATR prints `unknown` here — see
§Limits.

### Read type — bit 7 of the second value

| Bit 7 | Printed |
|---|---|
| `0` | `to end` |
| `1` | `variable length` |

Printed after `Read type: `.

### Data units — bits 6–3 of the second value

| Code | Printed |
|---|---|
| `0` | `no indication` |
| `1`–`7` | the value from the table: `2`, `4`, `8`, `16`, `32`, `64`, `128` |
| `8`–`15` | `RFU` |

Printed after `Data units: `.

### Data unit length (bits) — bits 2–0 of the second value

| Code | Printed (bits) |
|---|---|
| `0` | `128` |
| `1` | `256` |
| `2` | `512` |
| `3` | `1024` |
| `4` | `2048` |
| `5` | `4096` |
| `6` | `8192` |
| `7` | `16384` |

Printed after `Data unit length (bits): `, on the same line as the data-units
field (see the run-together note above).

## Limits

- The parse reads exactly four values and never looks further, so a reply of a
  different length leaves the bus mid-stream
  (see [edge-cases.md](edge-cases.md)).
- The protocol field recognises only the high nibbles `8`, `9`, `A` and `F`
  (§The parse).  A standard ATR starts with `TS` = `0x3B` (direct convention)
  or `0x3F` (inverse), whose high nibble is `3`, so a standard card prints
  `unknown`.  The decode for the non-`TS` codes is taken from the firmware's
  own table; a standard ATR does not exercise those codes.
- The macros print no error when no card answers; the four values are just
  whatever the floating or idle lines read.
- The parse's field decode is only meaningful at 8-bit width.
- The `(1)` output is not a valid ATR (see the warning above).
- `(1)` leaves CS low; release it (or leave the mode) before using the card
  for a different purpose.
