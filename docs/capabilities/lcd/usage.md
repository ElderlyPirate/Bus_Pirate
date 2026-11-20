# LCD — worked sessions

Part of the LCD documentation set (`docs/capabilities/lcd/`).  These sessions
show the command shapes and the replies.  Type text after the prompt; the
firmware echoes it.  The wiring and the adapter's electrical details are in
[wiring.md](wiring.md); the commands are in [commands.md](commands.md).

## Enter, power and inspect the pins

The all-features v4 build; LCD is entry 9.  The menu form prints the clutch and
finish-setup lines:

```
HiZ>m
1. HiZ
2. 1WIRE
3. UART
4. I2C
5. SPI
6. 2WIRE
7. 3WIRE
8. KEYB
9. LCD
10. PIC
11. DIO
x. exit(without change)

(1)>9
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
LCD>
```

Power and configure the link:

```
LCD>W
POWER SUPPLIES ON
This mode requires an adapter
Clutch engaged!!!
LCD>
```

`v` shows the pins.  The columns are tab-separated; the table is transcribed
with fixed spacing:

```
LCD>v
Pinstates:
12.(Bk)  11.(Wt)  10.(Gy)  9.(Pu)   8.(Bu)   7.(Gn)   6.(Yw)   5.(Or)   4.(Rd)   3.(Br)   2.(Bk)   1.(Wt)
GND      5.0V     3.3V     VPU      ADC      AUX2     AUX1     AUX      CS       MISO     CLK      MOSI
P        P        P        I        I        I        I        I        O        I        O        O
GND      4.98V    3.29V    0.00V    0.00V    L        L        L        L        H        L        L
LCD>
```

The last row is the live pin state; the voltages and the `MISO` level are
example values and depend on the attached circuit.  `CS`, `CLK` and `MOSI`
are outputs after `W`; `MISO` is unused.

## Initialise and write text

Initialise a two-line display.  `(2)` resets and configures it:

```
LCD>(2) 2
RESET
INIT

LCD>
```

`W` selected the data register, so text can be sent directly:

```
LCD>"Hello"
WRITE: "Hello"
LCD>
```

The glass should show `Hello` from column 1, with the cursor after the last
character.

Home the cursor, turn the display on with the cursor off, and write again.
`{` moves RS low for the command; `}` moves it high for data:

```
LCD>(4) 0
CURSOR SET

LCD>{
RS LOW, COMMAND MODE
LCD>0x0C
WRITE: 0x0C
LCD>}
RS HIGH, DATA MODE
LCD>"Bus Pirate"
WRITE: "Bus Pirate"
LCD>
```

The glass should now show `Bus Pirate` on row 1; `Hello` has been overwritten
from the home position.  Row 2 is still blank.

`0x0C` is the controller's display-on, cursor-off instruction; the other
instruction bytes are tabulated in
[display.md](display.md#controller-instruction-bytes).

## Macros

`(0)` lists the mode macros:

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

`(2)` without an argument prompts for the line mode; Enter accepts `Multiple`.
`(3)` is a single-step clear.  The first list line shown by the prompt ends
with a space in the firmware string.

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

`(2)` should leave the glass cleared at home; `(3)` clears it again.

`(6)` clears and writes a repeating digit pattern, echoing the same characters
to the terminal:

```
LCD>(6) 20
01234567890123456789
LCD>
```

On a 20x2 glass the full `01234567890123456789` should be visible on row 1.
On a 16x2 glass the last four characters are at off-screen addresses.

There is no macro 5:

```
LCD>(5)
Unknown macro, try ? or (0) for help

LCD>
```

## Reading the display

Reading is not implemented.  `r` prints the no-effect error, a dummy value and
the dispatcher's syntax-error line:

```
LCD>r
READ: ERROR: command has no effect here
0x00
Syntax error at char 1
LCD>
```

The display content cannot be read back.

## Defining a custom character

With the display powered (`W`) and initialised (`(2)`), define glyph 0, then
print it.  The eight row bytes are the glyph bitmap, one byte per row.

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

The eight row bytes form this 5x8 cell (all eight rows are part of the glyph):

```
.###.
#...#
#...#
#####
##.##
##.##
#...#
.###.
```

The glass should show this outline at the home position, with the cursor after
it if the cursor is still on.

The sequence addresses CGRAM (`0x40`), sends the eight row bytes, returns to
the display (`0x80`), then sends the glyph code (`0x00`).  The address counter
advances after each byte, so the rows are sent in order.  The on-glass glyph
is `unverified`; the CGRAM map is in
[display.md](display.md#the-character-generator).

## Leaving the mode

`m1` returns to HiZ directly:

```
LCD>m1
Ready
HiZ>
```

The menu form prints the clutch line instead of going straight to `Ready`:

```
LCD>m
...
(1)>1
Clutch disengaged!!!
Ready
HiZ>
```

Leaving runs the board reset, which releases the pins and switches the
regulated rails off.  If the display's VDD comes from the adapter's Bus Pirate
rail, the display clears; a display with its own VDD feed keeps its content.
Send `W` again on re-entry, and `(2)` if the display lost power
([entry-and-setup.md](entry-and-setup.md#leaving-the-mode)).

## v3 / v4

The sessions are v4 examples.  The mode messages are the same on v3, but the
menu number differs and the v3 hardware path is **unverified on v3**.
