# 2WIRE — macros

Part of the 2WIRE documentation set (`docs/2wire/`).  This page owns the `(0)`
macro list and the ISO 7816-3 ATR macros, which are 2WIRE's headline extra.
The smart-card context behind them is in [smartcard.md](smartcard.md).

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

There is **no bus-reset macro** in 2WIRE.  The `BUS RESET` string belongs to the
1-Wire mode, not here; the only firmware reset available in 2WIRE is the base
layer's `#`.

## Macro `(1)` — ISO 7816-3 ATR

`(1)` prints a two-line banner, drives the card reset, then parses the reply
(format illustration):

```
2WIRE>(1)
ISO 7816-3 ATR (RESET on CS)
RESET HIGH, CLOCK TICK, RESET LOW
ISO 7816-3 reply (uses current LSB setting): 0x3B 0x08 0x00 0x00
Protocol: unknown
Read type: to end
Data units: 2
Data unit length (bits): 128
2WIRE>
```

The reset is on **CS**, not on the bus lines.  The banner's three words name the
middle of the sequence; the macro's full edge order on CS is:

1. drive CS low and hold about 255 µs,
2. drive CS high — the banner's `RESET HIGH`,
3. advance one clock tick on SCL (CLK) — the banner's `CLOCK TICK`,
4. drive CS low — the banner's `RESET LOW`,

then read the reply.  See [smartcard.md](smartcard.md) for the card-contact
mapping this implies.

## Macro `(2)` — parse only

`(2)` runs only the parse step: it reads four values and decodes them, without
driving the reset.  Use it when you have triggered the ATR yourself (for
example by hand-driving CS and CLK) or when you want to re-read a reply.

## The parse

Both macros read **four** values from SDA (MOSI) and print them as hex on one
line, bit-reversed per value when LSB-first order is selected (`L`):

```
ISO 7816-3 reply (uses current LSB setting): <b0> <b1> <b2> <b3>
```

The parse assumes the default **8-bit width**; if `;N` has set another width,
each "value" is N bits.  The decoded fields come from the first two values.
Bits are numbered 0 (least-significant) to 7 (most-significant).  The first
value carries the protocol field in its high nibble; the second value carries
all three bit fields:

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

Printed after `Data unit length (bits): `.

## Limits

- The parse reads exactly four values and never looks further, so a reply of a
  different length leaves the bus mid-stream
  (see [edge-cases.md](edge-cases.md)).
- The protocol field recognises only the high nibbles `8`, `9`, `A` and `F`
  (§The parse).  A standard ATR starts with `TS` = `0x3B` (direct convention)
  or `0x3F` (inverse), whose high nibble is `3`, so a standard card prints
  `unknown`; the recognised nibbles are the firmware's own set, not the
  ISO 7816-3 `TS` values (**not verified**).
- The macros print no error when no card answers; the four values are just
  whatever the floating or idle lines read.
- The ATR path is **not verified** on hardware.

## v3 / v4

The macro menu, the trigger and the parse are the same in both builds.  The v3
path is **not verified**.