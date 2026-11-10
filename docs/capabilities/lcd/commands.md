# LCD — command and macro reference

Part of the LCD documentation set (`docs/capabilities/lcd/`).  This page owns
the LCD-specific commands, the `(0)` macro set, and the controller
instruction bytes.
The shared CLI (prompt, value entry, `W`/`w`, `P`/`p`, `A`/`a`/`@`, `i`,
`m`, `v`, user macros `<n>`) is documented once in
[the generic layer](../../generic/README.md) and is not repeated here.

## Scope

Only the keys whose effect is specific to LCD are tabulated.  A key that the
base layer owns keeps its base-layer behaviour but acts on this mode's pins.

## Register select — the two addresses

The display has two write targets.  The mode selects between them with the
wrapper keys; the selection persists until changed, and the next value or
string is written to the selected target.

| Input | Effect | Printed |
|---|---|---|
| `{` or `[` | select the **command** register (RS low) | `RS LOW, COMMAND MODE` |
| `}` or `]` | select the **data** register (RS high) | `RS HIGH, DATA MODE` |

All four keys act as simple on/off selectors here — `{`/`[` are equivalent,
and `}`/`]` are equivalent.  `W` selects the data register.  On a cold entry
before `W` the bus is not driven, so the selected register has no effect until
the supplies are on.

**Naming note:** these are the same keys the base layer calls "start" / "stop"
wrappers; in LCD they carry no start/stop semantics — they only move RS.

## Writing

| Input | Effect | Printed |
|---|---|---|
| `0xNN` | write hex byte to the selected register | `WRITE: <value>` |
| `0b########` | write binary byte | `WRITE: <value>` |
| `NNN` | write decimal byte | `WRITE: <value>` |
| `"text"` | write each byte of `text` | `WRITE: "text"` |

- Only the low eight bits are sent; the shift register is 8 bits wide.
- The value is echoed in the current display format (`o` selects HEX / DEC /
  BIN / RAW), so `WRITE: 0x41` is the HEX form of writing `0x41`.
- No acknowledgement is checked — the mode does not read the display back, so
  a write always reports success.  This is different from a mode that polls an
  ACK.
- `:N` repeats a write (`0x41:3` sends the byte three times); `;N` sets the bit
  width for the value (generic grammar).

To write text to the display: select data (`}`) and send `"Hello"`.  To send
commands to the controller: select command (`{`) and send the command byte.

## Reading is not implemented

`r` has no effect in this mode.  The mode's read handler prints the base-layer
error and sets the command error, which the dispatcher follows with the
syntax-error line:

```
LCD>r
READ: ERROR: command has no effect here
0x00
Syntax error at char 1
LCD>
```

The `0x00` is the dummy value the no-effect read returns; the
`Syntax error at char 1` line is the dispatcher's, not the mode's.  The
display's read line and busy flag are never used.  Do not expect `r` to return
display content.

## Macros — the `(0)` list

Mode macros are invoked as `(n)`.  `(0)` prints the list.  The firmware prints
exactly:

```
 0.Macro menu
 1.LCD Reset
 2.Init LCD
 3.Clear LCD
 4.Cursor position ex:(4) 0
 6.Write test numbers ex:(6) 80
 7.Write test characters ex:(7) 80
```

There is **no `(5)`**.  The historical "write test text" macro was removed;
entering `(5)` falls through to `Unknown macro, try ? or (0) for help`.

| Macro | Action | Printed | Argument |
|---|---|---|---|
| `(0)` | list the macros | the list above | — |
| `(1)` | reset the display to 4-bit mode | `RESET` | — |
| `(2)` | reset, then initialise the display | `RESET`, then `INIT` | display lines: `1` or `2` |
| `(3)` | clear the display and return home | `CLEAR` | — |
| `(4)` | set the cursor (DDRAM address) | `CURSOR SET` | address `0`–`127` |
| `(6)` | clear, then write `N` test digits | the characters | count `N` |
| `(7)` | clear, then write `N` test characters | the characters | count `N` |

Argument form is a space or nothing after the macro, for example `(4) 0`,
`(4)0`, `(6) 80`.  The historical `(4:0)` colon form is not what the firmware
menu shows; use the space form.

### `(1)` reset

Puts the controller back into 4-bit mode (the power-on wake-up sequence).  It
does **not** turn the display on or clear it — use `(2)` to initialise, or
`(3)` to clear.

### `(2)` init

Resets and then writes the standard initialisation: 4-bit, 5×7 font, one or
two lines, then display on with the cursor shown and blinking off.

The display-lines argument selects the controller's line mode:

- `(2) 1` → one-line mode,
- `(2) 2` → multiple-line mode.

If the argument is missing or not `1`/`2`, the firmware prompts:

```
Display lines:
 1. 1
 2. Multiple
```

with default `2` (Multiple) — pressing Enter at the `(2)>` prompt selects
multiple lines.

### `(4)` cursor

Moves the cursor by loading a display-data-RAM address.  `(4) 0` is the
home position.  On a two-line display the second line normally starts at
address `64` (`(4) 64`).  The argument is the raw address, not a row/column
pair.

### `(6)` / `(7)` self-test patterns

Both clear the display first, then write a repeating pattern and echo the same
characters to the terminal:

- `(6)` writes the digits `0`–`9` repeatedly.
- `(7)` writes characters starting at `!` upward, wrapping after the top of
  the seven-bit range.

A count argument sets how many characters are written; with no argument the
count is 80.  The serial echo lets you see the pattern without looking at the
display.

## Controller instruction bytes

Because the mode simply shifts bytes to the controller in command mode, you
can drive the HD44780 directly by selecting the command register (`{`) and
writing a byte.  The values below are the HD44780 **instruction base bytes**
the firmware builds its own commands from.  Only `0x01` and `0x02` are
complete instructions on their own; for the rest the low bits select the
options, so the base byte alone is **not** the effect you want:

| Base byte | HD44780 instruction | Low bits that select the option |
|---|---|---|
| `0x01` | clear display, cursor home | complete instruction |
| `0x02` | return home | complete instruction |
| `0x04` | entry mode set | I/D direction, S display shift |
| `0x08` | display on/off control | D display, C cursor, B blink |
| `0x10` | cursor or display shift | S/C select, R/L direction |
| `0x20` | function set | DL data width, N lines, F font |
| `0x40` | set CGRAM address | 6-bit character address |
| `0x80` | set DDRAM address | 7-bit display address |

For example, writing the base byte `0x08` alone is display **off** (`D=0`),
not on: to turn the display on write `0x0C` (display-on bit added).  Likewise
`0x04` alone selects entry mode with decrement; write `0x06` for increment.
The firmware's own `(2)` init builds these full values internally, and it does
no validation, so a wrong byte is passed through unchanged.  Defining a custom
character uses `0x40` and `0x80` with eight data bytes; see
[usage.md](usage.md).

`unverified`: the exact on-display result of hand-written instruction bytes is
not confirmed.  The base byte values and option-bit positions match the
HD44780 definitions the firmware uses.

## Shared keys that still apply

| Key | In LCD mode |
|---|---|
| `W` / `w` | engage / release the supplies and the SPI setup; `w` may leave the pins driven (see [entry-and-setup.md](entry-and-setup.md#supply-release-w)) |
| `i` | version block plus the standard status block in LCD (see [entry-and-setup.md](entry-and-setup.md#status-output-i-and-v)) |
| `v` | the pin/voltage table only; same I2C-style labels (see [entry-and-setup.md](entry-and-setup.md#status-output-i-and-v) and [edge-cases.md](edge-cases.md#pin-labels-look-like-i2c)) |
| `m`, `o`, `?` | base-layer mode change, display format, help |
| `#` / `m1` | leave for HiZ / reset: `#` is a full terminal reset on v4 and a reboot on v3; `m1` is the HiZ mode ([generic reset rules](../../generic/usage.md#reset-and-the-idle-state)) |
| `P` / `p` | accepted; enable / disable the Bus Pirate pull-ups, which this mode does not use.  `P` may warn that no voltage is present on the VPU pin (see [wiring.md](wiring.md)) |
| `A` / `a` / `@` | drive / read the AUX pin |
| `<n>` | user macros (base-layer storage and replay) |

## v3 / v4

The command set, the macro list, and the printed strings are identical in both
builds.  The v3 build is **not verified**.
