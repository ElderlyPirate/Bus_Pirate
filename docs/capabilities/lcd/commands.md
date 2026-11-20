# LCD — commands and macros

Part of the LCD documentation set (`docs/capabilities/lcd/`).  This page owns
the LCD-specific keys: register select, the write forms, `r`, and the `(0)`
macro set.  The shared prompt grammar (prompt, numeric input, `:N`, `;N`, `o`,
`W`/`w`, `P`/`p`, `A`/`a`/`@`, `<n>`, `m`, `#`) is documented once in
[the generic layer](../../generic/README.md) and is not repeated here.

The controller instruction bytes are in
[display.md](display.md#controller-instruction-bytes); the transfer sequence is
in [spi-link.md](spi-link.md#the-transfer-sequence).

## Register select — the two write targets

The display has two write targets.  The wrapper keys move RS; the selection
persists until changed.

| Input | Effect | Printed |
|---|---|---|
| `{` or `[` | select the command register (RS low) | `RS LOW, COMMAND MODE` |
| `}` or `]` | select the data register (RS high) | `RS HIGH, DATA MODE` |

All four keys only move RS in this mode.  `{`/`[` are equivalent and `}`/`]`
are equivalent; there is no start/stop or read-framing meaning.  `W` selects
the data register when it runs.  Before `W` the LCD setup has not run; on a
clean entry the SPI port is disabled, so the selection has no effect on the
display.

The macros send their own RS with every byte; they do not change the selected
register.  A value written after a macro goes to the register selected before
it.

## Writing

| Input | Effect | Printed |
|---|---|---|
| `0xNN` | write a hex byte | `WRITE: <value>` |
| `0b########` | write a binary byte | `WRITE: <value>` |
| `NNN` | write a decimal byte | `WRITE: <value>` |
| `"text"` | write each character of `text` | `WRITE: "text"` |

- Only the low eight bits are sent; the shift register is 8 bits wide.  A
  larger numeric value is masked for the echo and truncated for the shift.
- The echo uses the current display format (`o`: HEX, DEC, BIN, RAW).  It is
  an echo, not a read-back.
- No acknowledgement is checked.  The mode does not read the display, so a
  write reports success even with no adapter connected.
- `:N` repeats a **numeric** write; the value is echoed once per repeat.  A
  quoted string is sent once, character by character, and does not take `:N`.
- A quoted string cannot contain a quote; write `0x22` numerically if you need
  that character.
- The shared `l`/`L` bit order can reverse the value before the shift; `;N`
  changes the echo width (and the reversal width when `L` is set).  Leave them
  at 8 bits and MSB first for normal text
  ([spi-link.md](spi-link.md#shared-settings-that-still-affect-a-write)).

To write text: select data (`}`) and send `"Hello"`.  To send a controller
instruction: select command (`{`) and send the instruction byte.

## Reading is not implemented

`r` runs the shared no-effect read handler.  It prints the error, a dummy
value, and then the dispatcher's syntax-error line:

```
LCD>r
READ: ERROR: command has no effect here
0x00
Syntax error at char 1
LCD>
```

The `0x00` is the handler's return value, and the syntax-error line belongs to
the dispatcher, not the mode.  The display's read line and busy flag are never
used.  Keep a copy of what you wrote; the display content cannot be read back.

## The macro set

Mode macros are invoked as `(n)`.  `(0)` prints the list:

```
LCD>(0)
 0.Macro menu
 1.LCD Reset
 2.Init LCD
 3.Clear LCD
 4.Cursor position ex:(4) 0
 6.Write test numbers ex:(6) 80
 7.Write test characters ex:(7) 80

LCD>
```

| Macro | Action | Printed | Argument |
|---|---|---|---|
| `(0)` | list the macros | the list above | ignored |
| `(1)` | 4-bit wake-up sequence | `RESET` | ignored |
| `(2)` | wake-up, then initialise | `RESET`, then `INIT` | lines: `1` or `2` |
| `(3)` | clear the display and home | `CLEAR` | ignored |
| `(4)` | set the DDRAM address | `CURSOR SET` | address `0`-`127`; omitted = `0` |
| `(6)` | clear, then write test digits | the digits | count; `0` or omitted = `80` |
| `(7)` | clear, then write test characters | the characters | count; `0` or omitted = `80` |

There is **no macro 5**.  `(5)` prints
`Unknown macro, try ? or (0) for help` and a blank line, then the prompt.

Argument forms are a space or nothing after the macro: `(4) 0` and `(4)0` both
work, as do `(6) 20` and `(6)20`.  The historical colon form `(4:0)` is
**rejected**: the dispatcher does not see the closing `)`, and the line ends
with a syntax error.

### `(1)` reset

Sends the power-on wake-up sequence that puts the controller into 4-bit mode.
It does not turn the display on, clear it or change the selected register.
Use `(2)` to initialise or `(3)` to clear.

### `(2)` init

Resets, then sends the function set, turns the display off, clears it, and
turns it on with the cursor shown and blink off.  The line mode is the
argument:

- `(2) 1` — one-line mode;
- `(2) 2` — multiple-line mode.

If the argument is missing, `0`, or outside `1`-`2`, the firmware prints the
line-mode menu and prompts.  The prompt's default is `2` (Multiple); an
invalid answer prints `Invalid choice, try again` and repeats the prompt.

### `(3)` clear

Sends the controller's clear-display instruction and waits 15 ms.  It homes the
cursor and sets the entry mode to increment.  It does not re-send the function
set; use `(2)` after a power-up or a reset.

### `(4)` cursor

Loads a raw DDRAM address; it is not a row and column.  `(4) 0` is home; on the
standard two-line map the second row starts at `0x40`, so `(4) 64`.  Only the
low seven bits reach the controller, so values above 127 wrap.  The visible
result depends on the glass's map ([display.md](display.md#sizes-and-geometry)).

### `(6)` and `(7)` test patterns

Both clear the display first, then write a repeating pattern and echo the same
characters to the terminal:

- `(6)` writes the digits `0`-`9` repeatedly;
- `(7)` writes character codes from `!` (0x21) upward, wrapping from 0x7F back
  to 0x21.

The count is a number of bytes, not a row width, and `0` means 80.  A count
larger than the visible area continues into controller addresses that may be
off-screen.  The 0x7F character is sent and echoed like any other code; some
terminals do not render it.

## Shared settings and keys

| Key | In LCD mode |
|---|---|
| `W` / `w` | engage / release the supplies and the SPI setup — [entry-and-setup.md](entry-and-setup.md#w--power-the-adapter) |
| `i` / `v` | version block plus status / the pin table only — [entry-and-setup.md](entry-and-setup.md#status-output--i-and-v) |
| `o` | display format for the `WRITE:` echo |
| `;N`, `l`, `L` | `;N` changes the echo width; `l` / `L` set the bit order, and `L` reverses the value before the shift |
| `P` / `p` | accepted; the Bus Pirate pull-ups are not used by this interface; `P` may warn about the Vpullup pin, and is refused while SMPS owns the ADC (`p` is not) ([generic edge cases](../../generic/edge-cases.md)) |
| `A` / `a` / `@` | drive the selected AUX target high / low / read it (default `AUX`) |
| `<n>` | shared user macros (stored lines, not mode macros) |
| `m`, `m<N>`, `#`, `m1` | change mode / reset / return to HiZ — [entry-and-setup.md](entry-and-setup.md#leaving-the-mode) |

## Keys that do not apply

The SPI-mode pin keys are not repurposed for the display.  `/`, `\`, `-`, `_`,
`.`, `^` and `!` print their shared prefix (or begin it), then the null
handler reports `ERROR: command has no effect here`, and the dispatcher adds a
syntax-error line.  Nothing on the adapter changes.  Use the register-select
keys and a write instead.

The SPI mode's sniffer, speed, polarity and CS macros do not exist here; `(n)`
is the LCD macro set above.

## v3 / v4

The keys, macro list, mode messages and argument rules are identical in both
builds.  The v3 hardware path is **unverified on v3**.
