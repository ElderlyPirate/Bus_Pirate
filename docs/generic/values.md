# Values — input, width, display and bit order

Part of the generic-layer documentation set (`docs/generic/`).  This page owns
the value model shared by every mode: how numbers are typed, how the read/write
width is set, how values are printed, and how bit order changes them.  Command
entries are in [commands.md](commands.md).

Send examples use a bus prompt (`UART>`).  A value sent from `HiZ>` has no bus
handler, so it reports `ERROR: command has no effect here` and a syntax error
instead of the `WRITE:` line.

## The value model

Every value is a 16-bit pattern.  Two settings decide what happens to it:

- **Display format** — HEX, DEC, BIN or RAW.  Set with `o`.
- **Width** — 8 bits by default, 1–16 bits with `;N`.  The width also
  selects the byte or word form of the display.

A third setting, bit order (`l` / `L`), reverses the pattern within the width
before it goes to the bus (writes) or after it is read (reads).

## Typing a number

| Form | Example | Notes |
|---|---|---|
| decimal | `123` | 0–65535 |
| hexadecimal | `0x1F` | `0x` or `0X`; digits 0–9, A–F, a–f |
| binary | `0b101` | `0b` or `0B`; digits 0 and 1 |
| string | `"abc"` | sends the bytes of the text, quotes included in the reply |

Values are unsigned: there are no negative numbers.  A leading `-` is not a
sign; it is the DATA HIGH bit op, so `-5` is `-` followed by the value 5.
`+5` is a syntax error.

A number that does not fit 16 bits, an empty `0x` or `0b` prefix, or any
unterminated syntax is a syntax error and nothing is sent.

The parser reads digits until the first character that is not valid for the
chosen base, then continues with that character as the next command.  There is
no separator requirement, so a letter can silently start a command:

```
UART>12a
WRITE: 0x0C
AUX LOW
UART>
```

`12a` writes 12 (0x0C) and then runs `a` (AUX low).  `0x1Fz` likewise writes
0x1F and then reports a syntax error at the `z`.

A quoted string sends each byte of the text and echoes it back:

```
UART>"OK"
WRITE: "OK"
UART>
```

The quotes are not sent.  A string cannot contain a `"`; the first closing quote
ends it.

## Repeat — `:N`

`:N` repeats the preceding command N times.  It must follow immediately:

| Input | Effect |
|---|---|
| `r:10` | ten reads |
| `0x55:3` | write 0x55 three times |
| `@:4` | sample the AUX pin four times |
| `d:3` | three ADC readings |
| `^:4` | four clock ticks |
| `!:4` | four bits |
| `[6:10]` | start, write 6 ten times, stop |

N is 1–32767; `:0` and values above 32767 are syntax errors.  A bare command
without `:N` runs once.  The repeat count is per command, not persistent.

## Width — `;N`

`;N` sets the display width and, in modes whose driver can move more than one
byte per value (including the I2C software driver), the transfer width.  In
byte-only modes (for example SPI) it changes only the displayed width and the
bit-order window; the transfer stays one byte.  It
must follow the value or the repeat suffix — the order is value, `:N`, `;N`:

| Input | Effect |
|---|---|
| `0x55;4` | write the low 4 bits of 0x55 |
| `0x55:3;4` | the same 4-bit write, three times |
| `r;16` | a 16-bit display/bit-order window; the I2C software driver clocks 16 bits, SPI still clocks one byte |
| `rx:4;16` | four 16-bit reads in HEX for this command only |

N is 1–16; `;0` and widths above 16 are syntax errors.  The width is **not**
per line: it stays in force for later values and reads until you change it, or
until a mode change, reset, or a mode cleanup restores 8 bits (DIO's `w`
restores 8).

When the width is neither 8 nor 16, the firmware appends `;N` to each value or
read it prints, so the echo tells you which width produced it:

```
UART>0x123;12
WRITE: 0x0123;12
UART>
```

Spaces are skipped as whitespace, but a space **before** `:` or `;` breaks the
suffix: the parser has already moved past the value, so the value is sent and
the colon then aborts the line with a syntax error.

The displayed value is masked to the width; with the default 8 bits, `0x123`
displays as `0x23`.  The byte-oriented modes send the low byte at that width;
a mode that implements a wider transfer uses the bits its width defines.

## Display format — `o`

`o` selects the number format.  `o<N>` selects one directly; a bare `o` or an
invalid argument lists the choices and prompts `(1)>`:

| Number | Format |
|---|---|
| 1 | HEX |
| 2 | DEC |
| 3 | BIN |
| 4 | RAW |

The answer prints `Display format set`.  The default is HEX.

The format applies to values, reads, and the `^` tick count.  It does not apply
to `t`, delays, voltages, prompts, help text or macro numbers.

| Format | 8-bit value 0x2A | 16-bit value 0x1234 |
|---|---|---|
| HEX | `0x2A` | `0x1234` |
| DEC | `42` | `4660` |
| BIN | `0b00101010` | `0b00010010 0b00110100` |
| RAW | one byte, no prefix or digits | two bytes |

RAW prints the byte itself.  A printable byte shows as that character; `0x00`
prints nothing.  Do not use RAW for values you intend to read as text — the
prompt can run into the byte stream.

The format survives a mode change.  `#` restores HEX; so does the return from
the binary layer or a SUMP session.  `m1` does not restore it.

The `?` help calls `o` `Set output type`.  That is a misnomer: the per-mode
output type (open drain or push-pull) is an answer in the mode's setup, not the
`o` setting.

## One-shot read format — `rx` / `rd` / `rb` / `rw`

`r` accepts one display letter immediately after it.  The letter applies to
that read only; the format set with `o` is unchanged afterwards.

| Input | Format for this read |
|---|---|
| `rx` | HEX |
| `rd` | decimal |
| `rb` | binary |
| `rw` | RAW |

The letter comes before the suffixes: `rx:4` is four HEX reads and `rd;16` is
one 16-bit decimal read.  The letter must be lowercase and immediate.  Any other
character is not a display letter and is parsed as the next command, so in a
bus mode `rW` performs one read and then turns the supplies on.  A space after
`r` is not a display letter either: `r ` is a plain read in the current `o`
format.

## Bit order — `l` / `L`

`l` and `L` set the order of the bits inside the current width:

| Input | Message | Effect |
|---|---|---|
| `l` | `MSB set: MOST sig bit first` | most-significant bit first (default) |
| `L` | `LSB set: LEAST sig bit first` | least-significant bit first |

Lowercase `l` selects MSB-first — the reverse of what the letters suggest.
The help row `l/L  Bitorder (msb/LSB)` uses the same order.

With LSB-first the byte on the bus is the bit reverse of the number shown for a
write, and a read is reversed before it is shown.  The width matters: reversing
a 16-bit value is not the same as reversing two bytes.

Bit order returns to MSB-first on a board reset (`#`, `m` / `m1`, the
binary/SUMP return) and on a mode change.  `w` does not change it.

## Convert a value — `=X` and `|X`

`=` prints a value in hex, decimal and binary.  `|` first reverses the value
within the current width.  The output is independent of `o`:

```
HiZ>=5
0x05 = 5 = 0b00000101
HiZ>|5
0xA0 = 160 = 0b10100000
HiZ>
```

| Input | Effect |
|---|---|
| `=X` | show X as hex, decimal and binary |
| `\|X` | bit-reverse X within the current width, then show it |

X is 0–65535.  The conversion always prints **one byte**, even when X is
wider: `=0x1234` shows `0x34 = 52 = 0b00110100`.  With a width above 8,
`|X` reverses the wider pattern and the printed byte can be 0; set the width
back to 8 before using `|` if you expect a byte result.

The display format does not change the conversion line; it always uses the
three-base form above.

## Related docs

- [commands.md](commands.md) — the commands that consume and produce values.
- [macros.md](macros.md) — values stored in macro bodies.
- [edge-cases.md](edge-cases.md) — parser traps and truncation cases.
- [usage.md](usage.md) — worked value and read sessions.
