# Getting around — prompt, editing, navigation

Part of the generic-layer documentation set (`docs/generic/`).  This page owns
the prompt lifecycle: line editing, history, the version and help screens, the
mode menu, mode entry and exit, and the terminal baud rate.  The command tables
are in [commands.md](commands.md); value formats are in [values.md](values.md).

## The prompt

After the serial port is up the firmware prints the current mode name, an
optional `(BASIC)` marker, and `>`:

```
HiZ>
```

`HiZ` is the default mode.  Printable input is echoed; a command line ends with
Enter (`CR` or `LF`).  After each command the prompt is reprinted.

## Line editing

The firmware edits the command line in place and uses ANSI cursor sequences to
move the terminal cursor:

| Input | Action |
|---|---|
| printable ASCII `0x20`–`0x7E` | insert at the cursor |
| Backspace `0x08` | delete the character left of the cursor |
| `0x04` (EOT) or Delete `0x7F` | delete the character under the cursor |
| Left / Right `0x02` / `0x06`, or `ESC [ D` / `ESC [ C` | move the cursor |
| Up / Down `0x10` / `0x0E`, or `ESC [ A` / `ESC [ B` | recall newer / older history |
| Home / End `0x01` / `0x05`, or `ESC [ H` / `ESC [ F`, or `ESC [ 1 ~` / `ESC [ 4 ~` | jump to line start / end |
| `ESC [ 3 ~` | delete the character under the cursor |
| `ESC O A`–`ESC O D` | arrow keys in application-cursor mode |
| any other control byte | beep (`BEL`) |

There is no kill-line, no word delete and no tab completion.  When the command
buffer is full, further printable characters beep and are discarded; the part
that fits still runs on Enter.  A raw `0x02` is the left-arrow byte, but when
5–19 NUL bytes precede it, it enters SUMP instead (see
[commands.md](commands.md), §Leaving the CLI).

## Command history — `h`

`h` lists the previous command lines, newest first, numbered from 1.  It lists
up to 7 entries on v3 and 15 on v4, also bounded by the command buffer.  The
list ends with an `x. exit` line and the prompt `(0)>`:

```
h
1. v
2. i
x. exit

(0)>
```

| Reply | Effect |
|---|---|
| an entry number | re-runs that command immediately |
| `x`, `0`, or Enter | returns to the prompt, no change |

With no history, `h` prints only a blank line.  Type `h` alone: text after the
`h` is parsed as further commands after the history prompt finishes.

The Up and Down arrows recall history while you are editing a line (see
[Line editing](#line-editing)); this is separate from the `h` list.

## Version and status — `i`

`i` prints the banner.  In HiZ that is all; in any other mode it appends a
status block.  The firmware line carries the short build hash baked in at
build time, so it differs per build (`<hash>` below stands for that value):

```
i
Bus Pirate v4
ElderlyPirate Firmware v9.0 - goo.gl/gCzQnW - <hash> 
[HiZ 1WIRE UART I2C SPI 2WIRE 3WIRE KEYB PIC DIO]
DEVID:0x1019 REVID:0x0004 (24FJ256GB106 A6)
http://github.com/ElderlyPirate
HiZ>
```

| Line | Meaning |
|---|---|
| `Bus Pirate v4` | board name.  On v3 the line is `Bus Pirate v3.<revision>` |
| `ElderlyPirate Firmware v9.0 - goo.gl/gCzQnW - <hash> ` | firmware string; the trailing space is part of it |
| `[ ... ]` | the modes compiled into this build, in menu order |
| `DEVID:` / `REVID:` | chip device-type and revision words (chip-specific) |
| ` (24FJ256GB106 A6)` | chip family and revision string |
| `http://github.com/ElderlyPirate` | project URL |

On v3 the mode list line continues with ` Bootloader v<major>.<minor>` and
then ends.  The v3 firmware also prints this banner at startup; v4 does not.

The non-HiZ status block follows the banner, in this order:

| Field | Content |
|---|---|
| config words | `CFG0:` on v4, then `CFG1:` and `CFG2:` on both |
| separator | `*----------*` |
| pin table | `Pinstates:` and the same four rows as `v` |
| supply / pull-up / VPU / output type | one line, e.g. `POWER SUPPLIES OFF, Pull-up resistors OFF, Open drain outputs (H=Hi-Z, L=GND)` |
| bit order and width | `MSB set: MOST sig bit first, Number of bits read/write: 8` |
| AUX assignment | `a/A/@ controls AUX pin` (or `CS` / `AUX1` / `AUX2`) |
| mode settings | the active mode's own settings summary |
| separator | `*----------*` |

The pin table and the supply/pull-up state are owned by
[usage.md](usage.md#v-in-hiz) and [commands.md](commands.md); the settings
summary is owned by the mode's page.

## Help — `?`

`?` prints 21 command rows in two columns.  The device emits them tab-separated,
so the on-screen column positions depend on the terminal's tab stops; the table
below transcribes the v4 screen.  The v3 screen has the same 21-row layout
with the differences listed after the table.

| Left | Meaning | Right | Meaning |
|---|---|---|---|
| `?` | This help | `(0)` | List current macros |
| `=X/\|X` | Converts X/reverse X | `(x)` | Macro x |
| `~` | Selftest | `[` | Start |
| `o` | Set output type | `]` | Stop |
| `$` | Jump to bootloader | `{` | Start with read |
| `&/%` | Delay 1 us/ms | `}` | Stop |
| `a/A/@` | AUXPIN (low/HI/READ) | `"abc"` | Send string |
| `b` | Set baudrate | `123` | Send integer value |
| `c/C/k/K` | AUX assignment (A0/CS/A1/A2) | `0x123` | Send hex value |
| `d/D` | Measure ADC (once/CONT.) | `0b110` | Send binary value |
| `f` | Measure frequency | `r` | Read |
| `g/S` | Generate PWM/Servo | `/` | CLK hi |
| `h` | Commandhistory | `\` | CLK lo |
| `i` | Versioninfo/statusinfo | `^` | CLK tick |
| `l/L` | Bitorder (msb/LSB) | `-` | DAT hi |
| `m` | Change mode | `_` | DAT lo |
| `e` | Set Pullup Method | `.` | DAT read |
| `p/P` | Pullup resistors (off/ON) | `!` | Bit read |
| `s` | Script engine | `:` | Repeat e.g. r:10 |
| `v` | Show volts/states | `;` | Bits to read/write e.g. 0x55;2 |
| `w/W` | PSU (off/ON) | `<x>/<x= >/<0>` | Usermacro x/assign x/list all |

Notes on the labels:

- `o Set output type` is a firmware misnomer: `o` selects the **display
  format**, not the per-mode output type.  The output type is a setup prompt
  (`Select output type:`).
- `Set Pullup Method` is `e`: it selects the source that feeds Vpullup.
- `PSU` in the help is the power supply; the messages say `POWER SUPPLIES`.
- `Script engine` is `s`, which enters BASIC; the prompt gains `(BASIC)`.
- `<x>` is a user-macro slot; `(x)` is a mode macro.
- `t`, `T` and `O` are working commands with no help row; on v4 so is `#`.
- **Case matters.**  `s` enters BASIC, while `S` starts the servo generator on
  AUX; `o` sets the display format, while `O` toggles the 1-Wire speed and is
  refused outside `1WIRE`.  `p` / `P`, `w` / `W`, `l` / `L`, `d` / `D` and
  `t` / `T` are related pairs (off/on, MSB/LSB, one-shot/continuous,
  read/reset).

The v3 help screen has the same row layout but differs:

| Help row | v3 | v4 |
|---|---|---|
| indentation | one leading space on every line | none |
| 4 | `#  Reset the BP` | `o  Set output type` |
| 8, right column | `123` (no description) | `123  Send integer value` |
| 9 | `c/C  AUX assignment (aux/CS)`; right column `0x123` with no description | `c/C/k/K  AUX assignment (A0/CS/A1/A2)`; right column `0x123  Send hex value` |
| 10, right column | `0b110  Send value` | `0b110  Send binary value` |
| 17 | `o  Set output type` | `e  Set Pullup Method` |

So `#` has a row only on v3 and `e` only on v4.  On v3 `o` is row 17; on v4
it is row 4.

## Mode menu — `m`

The menu lists the modes compiled into the build and asks for a number.  The
default v4 menu is:

| Number | Mode |
|---|---|
| 1 | `HiZ` |
| 2 | `1WIRE` |
| 3 | `UART` |
| 4 | `I2C` |
| 5 | `SPI` |
| 6 | `2WIRE` |
| 7 | `3WIRE` |
| 8 | `KEYB` |
| 9 | `PIC` |
| 10 | `DIO` |
| x | exit(without change) |

The default v3 build omits `KEYB` (PIC is 8, DIO is 9) and may have `LCD`
inserted before `PIC` in a custom build.  The list, the banner and the prompt
all use the same mode names.  The list ends with `x. exit(without change)`,
then a blank line, then the `(1)>` prompt.

| Input | Result |
|---|---|
| `m`, then a number | runs the new mode's setup prompts, then `Clutch disengaged!!!`; a non-HiZ mode adds `To finish setup, start up the power supplies with command 'W'`; then `Ready` |
| `m<N>` / `m N` (for example `m6` or `m 6`) | runs the same setup prompts and prints `Ready`; the `Clutch` line and the finish-setup prompt are printed only by the menu form |
| `m<N> <answers…>` | a complete answer set skips the prompts, prints the settings summary (when the mode has one), then `Ready`; an incomplete set falls back to the prompts |
| `m0` | the zero counts as "no argument", so the menu opens |
| Enter at the `(1)>` prompt | accepts the default `1` (HiZ) |
| `x` at the prompt | `no mode change` |
| out-of-range number at the prompt | `Invalid choice, try again`, then the prompt repeats |
| out-of-range direct form, e.g. `m11` | `Nonexistent protocol!` |
| non-numeric direct form, e.g. `mx` | the line aborts with a syntax error |

The setup answer order belongs to the mode's page.  A direct change prints no
`Clutch disengaged!!!` line, but the mode starts in the same parked state.
`W` starts the supplies and runs the mode's bus setup (the engage routine); it
does not repeat the setup prompts.  The mode's page states what it drives and
when.  In DIO the setup hook is empty, so `W` only switches the supplies.  The
menu form only announces the parked state.

`m1` returns to HiZ and runs the board-state reset; it prints `Ready`, not the
banner.  `#` also returns to HiZ and additionally restores HEX and prints the
banner.  Neither runs inside BASIC: the interpreter receives every line, so
both are syntax errors there; type `EXIT` first.  The reset detail is in
[usage.md](usage.md#reset-and-the-idle-state).

## Terminal baud — `b`

`b` changes the **terminal** link rate, not a target UART.  A mode's own target
baud is set in that mode's setup prompts.

On **v4** the USB CDC link's rate belongs to the host.  `b` (with or without
an argument) prints one line and returns:

```
USB CDC baud rate is host-controlled.
```

On **v3** `b` changes the physical UART.  A bare `b` prints
`Set serial port speed: (bps)` and prompts `(9)>`:

| Option | Selection |
|---|---|
| 1–9 | 300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200 bps |
| 10 | `BRG raw value` |

Option 10 asks `Enter raw value for BRG` and prompts `(34)>`.  `BRG` is the
baud-rate generator divider, not a rate: usable values are 1–32767, 34 is the
firmware's default (the 115200 bps preset) and 127 is 31250 bps (MIDI).  The
`(34)` is that default, not the rate currently in force.  A valid
`b10 <value>` (1–32767) skips the prompt; a missing, zero, malformed, or
over 32767 value reopens `Enter raw value for BRG` at the same `(34)>` prompt.
The menu default is 9 (115200), the rate the firmware boots at.

The UART page gives the formula and a rate table
([entry and setup](../protocol/uart/entry-and-setup.md#custom-baud-option-10)).

After the selection the firmware prints `Adjust your terminal` and
`Space to continue`, switches its own UART to the new rate, and waits about
five seconds for a space at that rate.  Switch the terminal and press Space.
If no space arrives, it restores the previous rate and prints
`Baud confirmation timed out; previous rate restored.`

## v3 / v4 delta

| Feature | v3 | v4 |
|---|---|---|
| startup banner | printed by the firmware | not printed; `i`, `#` or a layer exit shows it |
| command history `h` | 7 entries | 15 entries |
| user macros `<n>` | 3 slots × 24 B | 5 slots × 32 B |
| command buffer | 128 B | 256 B |
| VPU source `e` | — | present |
| AUX assignment `k` / `K` | — | present |
| reset `#` | hardware reset | terminal reset to HiZ + banner |
| `v` pin table | 10 columns | 12 columns |
| `b` | rate menu | one-line host-controlled notice |
| help screen | `#` row, `o` row, `aux/CS`, leading space | `o` row, `e` row, `A0/CS/A1/A2` |

The dispatcher is the same on both builds; these differences follow the board.

## Related docs

- [commands.md](commands.md) — command and wiring tables.
- [values.md](values.md) — numeric input, display formats, width.
- [macros.md](macros.md) — user and mode macros.
- [usage.md](usage.md) — example sessions.
- [edge-cases.md](edge-cases.md) — refusal and error behaviour.
- [SPEC.md](../SPEC.md) — the capability matrix.
