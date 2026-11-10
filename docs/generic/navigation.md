# Getting around — prompt, editing, navigation

Part of the generic-layer documentation set (`docs/generic/`).  The command
tables are the single authority in [`commands.md`](commands.md); this page
explains the same controls in prose and owns the line-editing keys and the
v3/v4 delta table.

## Availability

| Feature | v3 | v4 |
|---|---|---|
| prompt / CLI | present | present |
| startup banner printed by firmware | yes, with ` Bootloader v<major>.<minor>` after the mode list | no — the banner appears from `i`, reset `#`, leaving the binary layer, or leaving SUMP |
| command history `h` | 7 entries | 15 entries |
| user macros `<n>` | 3 slots × 24 bytes | 5 slots × 32 bytes |
| terminal command buffer | 128 B | 256 B |
| VPU source `e` | — (no such command) | present |
| AUX assignment `c`/`C` (`k`/`K`) | A0 / CS only | A0 / CS / A1 / A2 |
| reset `#` | real reboot | returns to HiZ, runs the board-state reset, prints the banner |
| `v` pin table | 10 columns | 12 columns |
| `#` documented in `?` help | yes | no — the help slot is taken by `o` |

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
In HiZ the exact output is (the git hash in the firmware line is baked in at
build time, so your build's hash will differ):

```
i
Bus Pirate v4
ElderlyPirate Firmware v8.0 - goo.gl/gCzQnW - af1f827 
[HiZ 1WIRE UART I2C SPI 2WIRE 3WIRE KEYB PIC DIO]
DEVID:0x1019 REVID:0x0004 (24FJ256GB106 A6)
http://github.com/ElderlyPirate
HiZ>
```

- `Bus Pirate v4` — the board name.  On v3 this line reads
  `Bus Pirate v3.<hardware revision>`.
- `ElderlyPirate Firmware v8.0 - goo.gl/gCzQnW - <hash> ` — the firmware
  string;
  `<hash>` is the git short hash baked in at build time, so the trailing space
  is part of the string.
- `[ ... ]` — the enabled mode list, in mode-menu order.  The width of this
  list is build-dependent; the example is the v4 default build, which has no
  LCD/SMPS and where AVRISP-MKII is a personality, not a menu mode.  On v3
  the firmware appends ` Bootloader v<major>.<minor>` to this line.
- `DEVID:` / ` REVID:` — chip device-type and revision words read from flash;
  the values are chip-specific.
- ` (24FJ256GB106 A6)` — chip family + revision string.
- `http://github.com/ElderlyPirate` — printed with a newline.

In a non-HiZ mode `i` appends a status block: `CFG0/CFG1/CFG2` config words
(v4 only for CFG0), a `*----------*` separator, the `v` pin/voltage table,
the supply/pull-up/VPU/open-drain/bit-order state, the current bit
width, the AUX assignment, then the mode's own settings.  Per-mode settings
text is owned by the mode docs.

### Help — `?`

`?` prints the 23-line help screen (v4 default).  The device shows it as two
tab-separated columns; transcribed here with the columns aligned:

```
General                                Protocol interaction
---------------------------------------------------------------------------
?        This help                     (0)             List current macros
=X/|X    Converts X/reverse X          (x)             Macro x
~        Selftest                      [               Start
o        Set output type               ]               Stop
$        Jump to bootloader            {               Start with read
&/%      Delay 1 us/ms                 }               Stop
a/A/@    AUXPIN (low/HI/READ)          "abc"           Send string
b        Set baudrate                  123             Send integer value
c/C/k/K  AUX assignment (A0/CS/A1/A2)  0x123           Send hex value
d/D      Measure ADC (once/CONT.)      0b110           Send binary value
f        Measure frequency             r               Read
g/S      Generate PWM/Servo            /               CLK hi
h        Commandhistory                \               CLK lo
i        Versioninfo/statusinfo        ^               CLK tick
l/L      Bitorder (msb/LSB)            -               DAT hi
m        Change mode                   _               DAT lo
e        Set Pullup Method             .               DAT read
p/P      Pullup resistors (off/ON)     !               Bit read
s        Script engine                 :               Repeat e.g. r:10
v        Show volts/states             ;               Bits to read/write e.g. 0x55;2
w/W      PSU (off/ON)                  <x>/<x= >/<0>   Usermacro x/assign x/list all
```

`;` — bits to read/write, e.g. `0x55;2`; `<x>` / `<x= >` / `<0>` —
user macro x / assign x / list all.

The `o` line is a firmware string misnomer: `o` selects the **display
format** (see §Display format), not the per-mode output type.

The v3 help screen differs: it has no `e`; `#  Reset the BP` takes the row
where v4 shows `o`, and `o` takes the row where v4 shows `e`; it prints the
AUX assignment as `aux/CS`; and its right-hand column is shorter (`123` and
`0x123` carry no annotation, and the binary line says `Send value`).  Compare
both screens.

### Mode menu — `m`

`m` with no argument lists the enabled modes and prompts for a number:

```
m
1. HiZ
2. 1WIRE
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

The list is dynamic — it follows the enabled modes, so it changes with the
build's feature set (LCD/SMPS are off in the default v4 build).

The firmware's mode string is `1WIRE`: the menu, the banner, the `[ ... ]`
list and the prompt all use it.  Bus messages about the same mode say
`1-WIRE speed: …`; this documentation writes `1-Wire` in prose.

| Input | Result |
|---|---|
| `m`, then a number | the new mode's setup prompts run, then `Clutch disengaged!!!` + (non-HiZ) `To finish setup, start up the power supplies with command 'W'` + `Ready` |
| `m<N>` (e.g. `m6`) | same setup prompts, straight to `Ready` — the `Clutch` line is printed only by the menu form |
| Enter | accepts the default `1` (HiZ) |
| `x` | leaves the current mode unchanged (`no mode change`) |
| out-of-range number | `Nonexistent protocol!` |

A direct `m<N>` also accepts the setup answers on the same line (`m6 2 1`,
`m3 5 1 1 1 1 2`): a complete answer set runs the setup non-interactively,
prints the mode's settings summary, then `Ready`; an incomplete or invalid
set falls back to the mode's prompts.  The answer order belongs to the mode's
page.

`m1` and the menu choice `1` return to HiZ.  `m1` prints `Ready`; the menu
form prints `Clutch disengaged!!!` then `Ready`.  Both run the board-state
reset described in [usage.md](usage.md#reset-and-the-idle-state).  `#` also
returns to HiZ on v4, but additionally restores the display format and leaves
BASIC.  "Clutch disengaged" is explained in
[README.md](README.md#glossary).

### Terminal baud — `b`

On v3, `b` changes the physical UART rate.  `b<N>` with `N` = 1…10 selects an
option directly; a bare or invalid `b` prints `Set serial port speed: (bps)`
and prompts `(9)>`:

| Option | Selection |
|---|---|
| 1–9 | 300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200 bps |
| 10 | `BRG raw value` |

Option 10 asks for a raw BRG value (`Enter raw value for BRG`, default 34),
then applies it.  The default is 9 (115200), the rate the firmware boots at.
After selecting, the firmware prints `Adjust your terminal` + `Space to
continue` and waits for a space before re-initialising the port; if no space
arrives within five seconds it restores the previous rate and prints `Baud
confirmation timed out; previous rate restored.`

**v4:** the USB CDC link's rate belongs to the host.  `b` (with or without an
argument) prints `USB CDC baud rate is host-controlled.` and returns; there
is no rate menu and no `b<N>` selection.

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
