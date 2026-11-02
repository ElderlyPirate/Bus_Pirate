# LCD — worked sessions

Part of the LCD documentation set (`docs/lcd/`).  The sessions below are
**illustrative and code-derived** — the adapter is not available for testing,
so the exact on-display result and terminal spacing are **unverified**.  They
show the command shapes and the replies the firmware produces.

## First session — enter, power, init, write text

All-protocols build; the LCD entry is 9.  The mode has no setup prompts, so
entry goes straight to `Ready`:

```
HiZ>m
1. HiZ
...
9. LCD
...
x. exit(without change)

(1)>9
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
LCD>
```

Engage the supplies with `W`:

```
LCD>W
POWER SUPPLIES ON
This mode requires an adapter
Clutch engaged!!!
LCD>
```

Initialise the display as a two-line unit, put the cursor home, and write
text.  The data register is already selected on entry, so no wrapper is
needed before the string:

```
LCD>(2) 2
RESET
INIT

LCD>(4) 0
CURSOR SET

LCD>"Hello"
WRITE: "Hello"
LCD>
```

To send a controller command by hand, select the command register first, then
return to data for the text:

```
LCD>{
RS LOW, COMMAND MODE
LCD>0x01
WRITE: 0x01
LCD>}
RS HIGH, DATA MODE
LCD>"Bus Pirate"
WRITE: "Bus Pirate"
LCD>
```

(`0x01` is the controller's clear-display / cursor-home command — see the
instruction-byte table in [commands.md](commands.md).)

## Macro session

`(0)` lists the available macros:

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

`(2)` with no argument asks for the line mode (default Multiple); `(1)` and
`(3)` are single-step:

```
LCD>(2)
RESET
Display lines:
 1. 1
 2. Multiple

(2)>2
INIT

LCD>(3)
CLEAR

LCD>
```

`(6)` writes a repeating digit pattern and echoes it to the terminal:

```
LCD>(6) 20
01234567890123456789
LCD>
```

`(5)` no longer exists and is rejected:

```
LCD>(5)
Unknown macro, try ? or (0) for help

LCD>
```

## Defining a custom character

The firmware has no dedicated custom-character macro, but the controller's own
data path is available through the two registers.  Define glyph 0, then print
it:

```
LCD>{
RS LOW, COMMAND MODE
LCD>0x40
WRITE: 0x40
LCD>}
RS HIGH, DATA MODE
LCD>0x0E
WRITE: 0x0E
LCD>0x11
WRITE: 0x11
LCD>0x11
WRITE: 0x11
LCD>0x1F
WRITE: 0x1F
LCD>0x1B
WRITE: 0x1B
LCD>0x1B
WRITE: 0x1B
LCD>0x11
WRITE: 0x11
LCD>0x0E
WRITE: 0x0E
LCD>{
RS LOW, COMMAND MODE
LCD>0x80
WRITE: 0x80
LCD>}
RS HIGH, DATA MODE
LCD>0x00
WRITE: 0x00
LCD>
```

The sequence is: send `0x40` + character index (command mode) to address the
character-generator RAM, send the eight row bytes (data mode), then send
`0x80` (command mode) to return to the display, and finally send the character
code (data mode) to print the glyph.  The controller's address counter
advances after each byte, so the row bytes are sent in order without
re-addressing.

## Reading — not available

`r` has no effect in this mode; the read reports the base-layer error and the
read value follows on the next line:

```
LCD>r
READ: ERROR: command has no effect here
0x00
LCD>
```

The displayed text cannot be read back.

## Leaving the mode

```
LCD>m
...
(1)>1
Clutch disengaged!!!
Ready
HiZ>
```

Leaving releases the SPI interface; the display keeps its last content until
it is reset or re-initialised.

## v3 / v4

The sessions are the same on both builds; the v3 build is **not verified**.