# Getting around — prompt, editing, navigation

Part of the generic-layer documentation set (`docs/generic/`).  The command
tables are the single authority in [`commands.md`](commands.md); this page
explains the same controls in prose and owns the line-editing keys and the
v3/v4 delta table.

## Availability

| | v3 | v4 |
|---|---|---|
| prompt / CLI | present | present (bench-default) |
| startup banner printed by firmware | yes | no — the banner only appears from `i`, reset `#`, or after leaving the binary layer |
| command history `h` | 7 entries | 15 entries |
| user macros `<n>` | 3 slots × 24 bytes | 5 slots × 32 bytes |
| terminal command buffer | 128 B | 256 B |
| VPU source `e` | — (no such command) | present |
| AUX assignment `c`/`C` (`k`/`K`) | A0 / CS only | A0 / CS / AUX1 / AUX2 |
| reset `#` | hardware reset (reboot) | simulated reset to HiZ + banner |
| bit-word separator example in help | `.` | `;` |
| `#` documented in `?` help | yes | no — the help slot is taken by `o`/`e` |

The command dispatcher is the same for both builds; the differences above
depend only on the board version.

## Entry & setup

### The prompt

After the serial port is up the firmware prints the current mode name, an
optional `(BASIC)` marker, and `>`:

```
HiZ>
```

`HiZ` is the default mode.  Entering a command starts with the echo of what you
typed; finish a command with `\r` or `\n`.  The firmware echoes printable input
and reprints the prompt after each command line.

### Line editing

The firmware supports cursor editing and history directly on the command line:

| Input | Action |
|---|---|
| printable ASCII `0x20`–`0x7E` | insert at cursor (append at end) |
| Backspace `0x08` | delete character left of cursor |
| `0x04` or Delete `0x7F` | delete character under cursor |
| Left / Right arrow (`ESC [ D` / `ESC [ C`, or `0x02` / `0x06`) | move cursor |
| Up / Down arrow (`ESC [ A` / `ESC [ B`, or `0x10` / `0x0E`) | recall newer / older history entry |
| Home / End (`ESC [ 1 ~` / `ESC [ 4 ~`, or `0x01` / `0x05`) | jump to line start / end |
| any other control byte | beep (`BEL`) |

### Banner / version info — `i`

`i` prints the version block and, when **not** in HiZ, also a status block.
In HiZ the exact output is (captured verbatim on the v4 bench — the git hash in
the firmware line is baked in at build time, so your build's hash will differ):

```
i
Bus Pirate v4
ElderlyPirate Firmware v8.0 - goo.gl/gCzQnW - af1f827 
[HiZ 1-WIRE UART I2C SPI 2WIRE 3WIRE KEYB PIC DIO]
DEVID:0x1019 REVID:0x0004 (24FJ256GB106 A6)
http://github.com/ElderlyPirate
HiZ>
```

- `Bus Pirate v4` — the board name (on v3 this line reads `Bus Pirate v3`).
- `ElderlyPirate Firmware v8.0 - goo.gl/gCzQnW - <hash> ` — the firmware string;
  `<hash>` is the git short hash baked in at build time, so the trailing space
  is part of the string.
- `[ ... ]` — the enabled mode list, in mode-menu order.  The width of this
  list is build-dependent; the example is the v4 default build, which has no
  LCD/SMPS and where AVRISP-MKII is a personality, not a menu mode.
- `DEVID:` / ` REVID:` — chip device-type and revision words read from flash;
  the values are chip-specific.
- ` (24FJ256GB106 A6)` — chip family + revision string.
- `http://github.com/ElderlyPirate` — printed with a newline.

In a non-HiZ mode `i` appends a status block: `CFG0/CFG1/CFG2` config words
(v4 only for CFG0), a `*----------*` separator, the `v` pin/voltage table (see
below), the supply/pull-up/VPU/open-drain/bit-order state, the current bit
width, the AUX assignment, then the mode's own settings.  Per-mode settings
text is owned by the mode docs.

### Help — `?`

`?` prints the 23-line help screen.  v4 output; columns are TAB-separated:

```
General					Protocol interaction
---------------------------------------------------------------------------
?	This help			(0)	List current macros
=X/|X	Converts X/reverse X		(x)	Macro x
~	Selftest			[	Start
o	Set output type			]	Stop
$	Jump to bootloader		{	Start with read
&/%	Delay 1 us/ms			}	Stop
a/A/@	AUXPIN (low/HI/READ)		"abc"	Send string
b	Set baudrate			123	Send integer value
c/C/k/K	AUX assignment (A0/CS/A1/A2)	0x123	Send hex value
d/D	Measure ADC (once/CONT.)	0b110	Send binary value
f	Measure frequency		r	Read
g/S	Generate PWM/Servo		/	CLK hi
h	Commandhistory			\	CLK lo
i	Versioninfo/statusinfo		^	CLK tick
l/L	Bitorder (msb/LSB)		-	DAT hi
m	Change mode			_	DAT lo
e	Set Pullup Method		.	DAT read
p/P	Pullup resistors (off/ON)	!	Bit read
s	Script engine			:	Repeat e.g. r:10
v	Show volts/states		;	Bits to read/write e.g. 0x55;2
w/W	PSU (off/ON)		<x>/<x= >/<0>	Usermacro x/assign x/list all
```

Two firmware string defects (wording, not behaviour):

- the `o` line says `Set output type`; `o` actually selects the **display
  format** (see §Display format);
- on v3 the help line for `v` shows the bit separator as `.` and three v3 lines
  differ (`#` reset present, `c/C` only AUX/CS).

### Mode menu — `m`

`m` with no argument lists the enabled modes and prompts for a number.  Captured
verbatim on the v4 bench:

```
m
1. HiZ
2. 1-WIRE
3. UART
4. I2C
5. SPI
6. 2WIRE
7. 3WIRE
8. KEYB
9. PIC
10. DIO
x. exit(without change)

(1)>
```

- The list is dynamic: it is built from the enabled modes in menu order, so it
  changes with the build feature set.  LCD/SMPS are off in the default v4
  build.
- Reply with a number → that mode.  Reply `x` (or press Enter for the default
  `1`) → `x` prints `no mode change`; Enter selects HiZ.
- `m<N>` (e.g. `m6`) changes directly, skipping the list; an out-of-range
  number prints `Nonexistent protocol!`.
- A direct `m<N>` runs the mode's **setup prompts** — answer each one (the
  default is `1`) — then prints `Ready`.  The prompts must be answered before
  the bus can be engaged: `W` starts the supplies only after the mode's setup
  chain has run.
- The `m` menu form runs the same setup prompts and additionally prints
  `Clutch disengaged!!!` + (non-HiZ) `To finish setup, start up the power
  supplies with command 'W'` before `Ready`.
- `x` leaves the current mode unchanged.

The mode's setup prompts (speed, output type, and any mode-specific settings)
belong to that mode's page.  "Clutch disengaged" is explained in
[README.md](README.md#glossary).

### Terminal baud — `b`

`b` sets the **terminal/link** baud rate.

- `b<N>` with `N` = 1…10 selects the corresponding rate from the port-speed
  menu directly.
- `b` with no/invalid argument prints `Set serial port speed: (bps)` and
  prompts `(9)>`: `1.300 · 2.1200 · 3.2400 · 4.4800 · 5.9600 · 6.19200 ·
  7.38400 · 8.57600 · 9.115200 · 10.Input Custom BAUD`.
- Option 10 asks for a raw BRG value (`Enter raw value for BRG`, default 34),
  then applies it.
- After selecting, the firmware prints `Adjust your terminal` + `Space to
  continue` and waits for a space before re-initialising the port.
- The menu text also lists option `11. Auto-Baud Detection`, but `b` limits the
  choice to 1…10, so option 11 is **not** selectable through `b` — it is only
  offered inside UART mode setup.

### Display format — `o`

`o` selects how values are printed:

- `o<N>` with `N` = 1 HEX, 2 DEC, 3 BIN, 4 RAW.
- `o` with no/invalid argument prints ` 1. HEX / 2. DEC / 3. BIN / 4. RAW` and
  prompts `(1)>`, then prints `Display format set`.

There is **no** line-length option on `o`; the `Display lines:` prompt belongs
to the HD44780 LCD mode, not here.  The help label `Set output type` is a
misnomer — `o` sets the *display* format; "output type" is a per-mode *setup*
prompt (`Select output type:`).

The default at boot is HEX.