# LCD — the SPI link and the SPI-mode boundary

Part of the LCD documentation set (`docs/capabilities/lcd/`).  This page owns
how the LCD mode uses the Bus Pirate's SPI hardware, the transfer sequence,
the fixed settings, and the differences from the interactive **SPI** mode.
The electrical wiring is in [wiring.md](wiring.md); the display protocol is in
[display.md](display.md).

## Shared hardware, separate driver

The LCD mode and the SPI mode use the same SPI hardware and the same four
header pins: **MOSI**, **MISO**, **CLK** and **CS**.  They do not share a
driver:

- The SPI mode has its own setup prompts, speed table, macros and binary
  sub-protocol ([SPI set](../../protocol/spi/README.md)).
- The LCD mode configures the SPI hardware itself when `W` runs.  It has no
  speed or polarity prompts, no SPI macros and no binary path.
- Only one of the two modes owns the pins at a time.  A mode change runs the
  old mode's cleanup: leaving SPI releases the pads and clears its open-drain
  setting; leaving LCD releases the pads and disables the SPI module.

The LCD mode is therefore **not** a layer on top of the SPI mode.  You cannot
use SPI mode's CS control, reads or sniffer while in LCD, and the LCD mode
does not appear in the SPI binary protocol
([binary sub-protocols](../../protocol/binary/sub-protocols.md)).

## Fixed settings

`W` runs the LCD setup.  Its SPI configuration is fixed; nothing in the mode
changes it:

| Setting | Value | Effect |
|---|---|---|
| role | master | the Bus Pirate drives CLK |
| word | 8 bits | one byte per shift |
| clock idle | low | CLK rests low between shifts |
| clock edge | active to idle | data changes on the active-to-idle edge |
| sample | middle | fixed; MISO is not used, so this only completes the SPI configuration |
| secondary prescaler | 1:1 | — |
| primary prescaler | 16:1 | 1 MHz nominal from the 16 MHz system clock |
| buffer | standard | one byte at a time, polled; no FIFO |
| CS | GPIO, idle low | latch pulse generated per byte |

The nominal 1 MHz shift clock matches the SPI mode's `1MHz` speed entry, but
LCD has no way to choose another speed.  If a display or adapter needs a
different clock, the LCD mode cannot provide it.

## The transfer sequence

The display is in 4-bit mode, so one display byte is two nibbles.  Each nibble
is three latched shift-register bytes:

1. **Data, EN low.**  The byte is `(nibble << 4)`, with the activity-LED bit
   set and RS set for data or clear for a command.
2. **Data, EN high.**  The same byte with EN set.  This is the display's
   enable rise, with RS and the data already stable.
3. **Data, EN low.**  The same byte with EN clear.  This is the enable fall.

Each step shifts eight bits on MOSI/CLK and then pulses CS to latch the byte
into the shift register.  The high nibble is sent first, then the low nibble.
One display byte is therefore six shift-and-latch cycles, or 48 SPI clocks.

The latch is the important boundary: the shift register's outputs do not
change while bits are being shifted, because CS is low.  They change only on
the CS pulse, after all eight bits are in.  The display's EN is one of those
latched outputs, not the CS line
([wiring.md](wiring.md#the-eight-shift-register-outputs)).

## What the mode does not do

| SPI-mode feature | In LCD mode |
|---|---|
| speed, polarity, edge, sample, CS polarity prompts | none; fixed configuration |
| `[` / `]` as CS assert/release | remapped to the display's command/data register select |
| `{` / `}` as read-framed start/stop | the same register select, no read |
| `r` / `!` reads on MISO | not implemented; MISO is unused and reads report no effect |
| `/`, `\`, `-`, `_`, `^`, `.` clock/data operations | no pin action; the shared prefix prints, then the mode reports no effect |
| SPI macros (sniffer, clock tweaks) | none; `(n)` is the LCD macro set |
| SPI binary sub-protocol | none |

## Shared settings that still affect a write

The prompt's shared width, bit order and display format are not LCD settings,
but the dispatcher applies them before calling the write handler
([generic/values.md](../../generic/values.md)):

| Setting | Effect on an LCD write |
|---|---|
| display format `o` | changes only the `WRITE:` echo, not the byte shifted |
| bit order `l` / `L` | `L` reverses the value before the shift, which mirrors the eight adapter outputs; leave it at `l` |
| width `;N` | changes the echo width and, with `L`, the reversal width; the shift is still one 8-bit byte |
| repeat `:N` | repeats a numeric write; a quoted string is sent once, character by character |

With the defaults (8 bits, MSB first) a byte value is sent unchanged.  A
reversed value or a narrowed echo does not mean the link changed; it means the
dispatcher transformed the value before the shift.

## Write failure and timeout

The mode polls the SPI receive buffer after each byte with a short bounded
wait.  If the byte does not complete, it releases the SPI setup (the same
state as after `w`), sets the command error, and the remaining shifts fail
immediately.  The dispatcher then prints one syntax-error line.  `W` re-runs
the setup and the link works again.

A write attempted **before** `W` takes the same error path on a clean entry
because the SPI port is disabled; the `WRITE:` echo still prints.  That is the
normal "not powered" case, not a fault
([edge-cases.md](edge-cases.md#writes-before-w)).

## Output type

The LCD mode uses normal push-pull outputs.  Entering the mode selects normal
output, so `i` reports `Normal outputs (H=3.3v, L=GND)`.  The LCD setup writes
the MOSI/CLK/CS open-drain bits off, so a previous binary SPI session's
open-drain setting is cleared when `W` runs.  No pull-up or recovery step is
needed.

## SPI mode compared

| Aspect | SPI mode | LCD mode |
|---|---|---|
| build | default | optional build |
| entry | mode 5 in the default v4 image | 9 in the all-features v4 build |
| setup | six prompts | none |
| SPI buffer | enhanced | standard |
| clock | 12 speeds, user polarity/edge/sample | fixed 1 MHz equivalent |
| CS | user controlled, optional polarity | internal latch, idles low |
| reads | full duplex on MISO | none |
| wrapper keys | CS and read framing | display register select |
| binary | `SPI1` sub-protocol | none |
| purpose | generic SPI target | one HD44780 adapter |

The SPI set owns the shared bus behaviour
([entry and setup](../../protocol/spi/entry-and-setup.md),
[bus operations](../../protocol/spi/bus-operations.md),
[binary](../../protocol/spi/binary.md)); this page only records where LCD
diverges.
