# LCD — display modes, sizes and addresses

Part of the LCD documentation set (`docs/capabilities/lcd/`).  This page owns
the HD44780 side of the mode: the 4-bit path, the init modes, display geometry
and addresses, the character generator, and the controller instruction bytes.
The terminal commands are in [commands.md](commands.md); the shift link is in
[spi-link.md](spi-link.md).

The controller behaviour below is the standard HD44780 instruction set, which
the firmware passes through.  The on-glass result and the address map of a
particular display are `unverified`; confirm them against the display's
datasheet.

## The 4-bit write-only path

The adapter drives the display's 4-bit interface: RS, R/W, EN and D4-D7.  D0-D3
are not driven.  The firmware sends every display byte as two nibbles, high
nibble first, with EN pulsed once per nibble.  R/W is held at write, so the
display's busy flag and data are never read.

`(1)` and `(2)` put the controller into 4-bit mode with the power-on wake-up
sequence.  A display that has just been powered up is in 8-bit mode until that
sequence runs; text sent before it is not interpreted as intended
([edge-cases.md](edge-cases.md#text-before-initialisation)).

## Init modes — one line or two

`(2)` resets the controller, sends the function set, turns the display off,
clears it, then turns it on with the cursor shown and blink off.  It offers two
line modes:

| Input | Function set | Use |
|---|---|---|
| `(2) 1` | 4-bit, 5×7 font, one-line | a one-line glass |
| `(2) 2` | 4-bit, 5×7 font, two-line | a two-line glass (default) |

Without an argument, with `0`, or with an argument outside `1`-`2`, the macro
prompts:

```
Display lines:
 1. 1 
 2. Multiple

(2)>
```

The prompt's default is `2` (Multiple): press Enter to select it.  A value
outside `1`-`2` at the prompt prints `Invalid choice, try again` and repeats
the prompt.  The first list line (` 1. 1`) ends with a space in the firmware
string.

Fixed choices the mode does not expose:

| Property | Value |
|---|---|
| data width | 4-bit |
| font | 5×7 |
| lines | 1 or 2, from `(2)` |
| display | on |
| cursor | on (underscore), blink off |
| entry mode | not sent as its own instruction; the clear inside `(2)`/`(3)` sets increment mode |
| display shift | not used |
| busy flag | not polled; fixed delays cover the slow commands |

`(1)` performs only the 4-bit wake-up: it does not turn the display on and does
not clear it.  `(2)` does both (clear, then on).  `(3)` clears and homes without
re-sending the function set.

## Sizes and geometry

The firmware has **no width setting**.  It writes bytes at the controller's
current DDRAM address and lets the controller increment the address.  Any
HD44780-compatible glass can be used as far as the byte path is concerned, but
the visible result depends on the glass's DDRAM map:

| Glass | Init | What to expect |
|---|---|---|
| 1-line, 8-80 characters | `(2) 1` | one visible row; the address counter walks the whole DDRAM |
| 2-line, 8-40 characters | `(2) 2` (default) | two rows; the second row starts at DDRAM address 0x40 on the standard map |
| 4-line, 16×4 or 20×4 | `(2) 2` (no separate 4-line init) | a single-controller 4-line glass may work; its line addresses are glass-specific (a common 20×4 map is 0x00, 0x40, 0x14, 0x54), so position each line with `(4)`; `unverified` |
| 40×2 with two enable lines | not offered | the adapter provides one EN output; a second enable needs board-level handling |

What a full row looks like after `(4) 0` and 20 bytes:

| Glass | Expected result |
|---|---|
| 16x2 | 16 characters on row 1; the last 4 are at off-screen addresses |
| 20x2 | all 20 on row 1 |
| 20x4 | 20 on row 1; the other lines start at the glass's map addresses (a common map is 0x40, 0x14 and 0x54), so position them with `(4)` (`unverified`) |

Consequences:

- **On a 2-line glass, text does not wrap to the second row at the visible
  edge.**  The address counter walks the DDRAM map, so after the visible end
  of row 1 it runs through addresses that are not displayed before it reaches
  0x40.  Use `(4) 64` to move to the second row of a standard two-line glass.
- **`(4)` takes a raw address**, not a row and column.  Only the low seven
  bits reach the controller, so values above 127 wrap: `(4) 192` behaves like
  `(4) 64`.
- **The test macros count bytes, not positions.**  `(6)` and `(7)` clear the
  display first, then write N characters from home; N has no relation to the
  row width or the visible area.  The `80` in the macro list is an example
  count, not a geometry.
- A 4-line glass that works as two independent 2-line controllers needs its
  own addressing and enable wiring.  A single-controller 4-line glass may work
  with the 2-line init and explicit `(4)` positioning.  Both are `unverified`;
  check the glass's map.

## The character generator

The controller has 64 bytes of character-generator RAM: eight glyphs of eight
rows each, selectable as character codes 0-7.  The firmware does not reserve
any glyph.  Define one by writing the controller instructions yourself:

1. Select the command register (`{`).
2. Send `0x40 + (glyph << 3)` to address the glyph's first row.
3. Select data (`}`) and send eight row bytes.  The low five bits of each byte
   are the pixels in a 5×8 cell (a 5×7 glyph plus the cursor row); the address
   counter advances after each byte.
4. Select command (`{`) and send `0x80` (or `0x80 + address`) to return to the
   display.
5. Select data (`}`) and send the glyph's code (0-7) to print it.

A worked sequence is in [usage.md](usage.md#defining-a-custom-character).  The
on-glass glyph is `unverified`.

## Controller instruction bytes

Select the command register (`{`) and write one byte.  The firmware builds its
own init commands from the same base values and performs no validation, so a
wrong byte is passed through unchanged.  The low bits select the options:

| Base byte | Instruction | Option bits |
|---|---|---|
| `0x01` | clear display, cursor home | complete instruction |
| `0x02` | return home | complete instruction |
| `0x04` | entry mode set | I/D (bit 1), S (bit 0); `0x06` = increment, no shift |
| `0x08` | display on/off control | D (bit 2), C (bit 1), B (bit 0) |
| `0x10` | cursor or display shift | S/C (bit 3), R/L (bit 2) |
| `0x20` | function set | DL (bit 4), N (bit 3), F (bit 2); the firmware uses `0x20`/`0x28` |
| `0x40` | set CGRAM address | 6-bit character-generator address |
| `0x80` | set DDRAM address | 7-bit display address |

Two traps:

- `0x08` alone is display **off** (D=0).  To turn the display on, send `0x0C`
  (D=1, cursor off) or `0x0E` (D=1, cursor on).
- `0x04` alone is entry mode with decrement (I/D=0).  For normal
  left-to-right text send `0x06`.

The firmware's init does **not** send an entry-mode instruction; it relies on
the power-on default.  The clear instruction inside `(2)` and `(3)` sets
increment mode (I/D=1) but leaves the S bit alone, so a hand-written decrement
(`0x04`) is undone by the next `(2)` or `(3)`.  Display shift and display
on/off persist until changed or until the display loses power.

## What the mode does not expose

| Feature | Status |
|---|---|
| width setting | none; the glass's map decides |
| 4-line init | none specific; a single-controller 4-line glass may work with `(2) 2` and explicit `(4)` addresses |
| backlight control | none as an LCD mode setting; board pots/jumpers, or the v4b AUX0 path driven by the shared `a`/`A` command |
| busy-flag polling | none; fixed delays |
| display or CGRAM read-back | none; `r` reports no effect |
| user-defined font upload beyond the eight CGRAM glyphs | none |
| speed/timing | fixed ([spi-link.md](spi-link.md)) |

## v3 / v4

The controller path, the init choices and the instruction bytes are the same on
both builds; the mode messages are identical.  The v3 hardware path is
**unverified on v3**.
