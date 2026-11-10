# PC keyboard (KEYB) — using the mode

Part of the KEYB documentation set (`docs/protocol/keyboard/`).  This page owns
the frame/handshake, the live input monitor, byte reads and writes, the macro
reference and the mode's limits.

## Scope

How to read a keyboard and send it host commands, using the live input monitor
(`(1)`), `r`, and a value write, plus the two mode macros.  Entry is in
[entry-and-setup.md](entry-and-setup.md); wiring is in [wiring.md](wiring.md).
Shared prompt grammar (`r:N`, `;N`, `o`, `W`, `P`, `(n)`) is owned by
`docs/generic/` and not repeated here.

## What the mode moves

KEYB is a manual interface: it reads and writes **raw 8-bit scancodes**.  It
does not decode scancodes into characters, and it does not distinguish make
(press) from break (release) — both are just bytes on the wire.  A PC/AT
keyboard typically sends the key's **make** code on press and the same code
with the high bit set on release (for example `1e` then `9e` for the `A` key).

The firmware prints byte values in the active display format — **HEX by
default**; `o` changes it ([commands.md](../../generic/commands.md)).

## Frame and handshake

The Bus Pirate is the host; the keyboard drives the interface in both
directions and generates the clock.  One byte is eleven bit slots, least
significant bit first, plus the keyboard's acknowledge slot on a host write:

| Slot | Device → host (a scancode) | Host → device (a command) |
|---|---|---|
| 1 | start bit — DATA low | start bit — DATA low |
| 2–9 | eight data bits, LSB first | eight data bits, LSB first |
| 10 | parity — odd | parity — odd, computed by the firmware |
| 11 | stop bit — DATA high | stop bit — DATA high (DATA released) |
| 12 | — | acknowledge, from the keyboard |

**Parity** is odd in both directions: the eight data bits plus the parity bit
contain an odd number of 1s.  On a read the mode checks the parity bit and
reports `*parity error` when the frame does not match.  On a write the firmware
works the parity bit out from the value and sends it; you do not supply it.

**Acknowledge.**  After a host byte the keyboard answers in a twelfth slot:
DATA low is `ACK`, DATA high is `NACK`.  The firmware samples the frame's
stop-bit slot and then the acknowledge slot; only the acknowledge slot decides
the reported result.  If the keyboard stops clocking before those slots, the
write reports `TIMEOUT`.

**Bus state.**  Between exchanges the mode holds CLOCK low (the host inhibit);
each read or write releases the line for the keyboard's clocks.  A read checks
the frame's start, parity and stop bits and prints the error word for the first
check that fails — the words are listed under
[Reading with `r`](#reading-with-r).

## The live input monitor — `(1)`

`(1)` runs the monitor.  It prints two header lines and then streams a value
for every scancode the keyboard sends:

```
KEYB>(1)
Input monitor, any key exits
Any key to exit
1e 9e 30 b0
KEYB>
```

- Header line 1 is `Input monitor, any key exits`; header line 2 is
  `Any key to exit`.  **Both headers mean a key on the terminal** (the
  serial connection), not a key on the keyboard being monitored.
- Each scancode is printed as it arrives, separated by spaces, with no line
  breaks — the stream is live and unbuffered.
- A frame that fails its checks prints nothing in the monitor; only successful
  scancodes appear (the error words belong to `r`).
- Typing on the keyboard being monitored keeps printing scancodes; a single
  character on the serial/terminal connection ends the monitor and returns to
  `KEYB>`.
- The sample above is a press and release of `A` (`1e`, `9e`) then `B`
  (`30`, `b0`).

**Unverified:** the sample scancodes are standard PC/AT Set-1 codes.

## Reading with `r`

`r` reads one scancode.  `r:N` performs exactly N reads — N independent
attempts, one read window each; it does not stop early on an error.  The
`READ: ` header is printed once for the whole command.  Each attempt waits
briefly for the keyboard to start a frame and gives up if it stays idle.

A successful read prints the value:

```
KEYB>r
READ: 0x1e
```

A failed read prints the mode's error word **and then the value the read
returned**.  That value is always zero, so every failure prints `0x00` (HEX
default) after the error word — it is a sentinel, not a scancode.  The
line-printing errors put the value on the next line:

```
KEYB>r
READ:  *parity error
0x00
```

`TIMEOUT` is the exception: the firmware prints it without a newline, so the
sentinel is appended to the same line:

```
KEYB>r
READ:  TIMEOUT0x00
```

**Footnote:** `READ:  TIMEOUT0x00` is the firmware's own formatting — read it
as the `TIMEOUT` error word followed by the `0x00` value.  The other five
error words are line messages and put `0x00` on its own line.  With `r:N` the
error/value pairs repeat, separated by the dispatcher's space:
`TIMEOUT0x00 TIMEOUT0x00 …`.

| Error word | Meaning |
|---|---|
| `NONE` | no frame started within the read window |
| `TIMEOUT` | the clock stopped changing while a frame or a bit was expected |
| `*startbit error` | the clock line showed a frame start but DATA was not low — no start bit |
| `*parity error` | the eight data bits and the parity bit are not odd parity — a bit was misread |
| `*stopbit error` | the frame did not end with the stop bit high |
| `UNKNOWN ERROR` | defensive default label; the current read path does not produce it |

**Note:** `TIMEOUT` is also a write result, with a different cause — see
[Writing to the keyboard](#writing-to-the-keyboard).  The failure modes behind
each word are in [edge-cases.md](edge-cases.md).

## Writing to the keyboard

A value (`0xNN`, `0b…`, or decimal) sends one byte to the keyboard — a host
command.  The keyboard answers with the acknowledge bit, reported as:

| Result | Meaning |
|---|---|
| `ACK` | the keyboard accepted the byte |
| `NACK` | the keyboard rejected the byte |
| `TIMEOUT` | the keyboard did not clock the byte or its answer |

A one-byte command:

```
KEYB>0xEE
WRITE: 0xEEACK
```

A two-byte command — `0xED` (Set LEDs) needs a bit-mask byte after it:

```
KEYB>0xED
WRITE: 0xEDACK
KEYB>0x01
WRITE: 0x01ACK
```

**Footnote (spacing):** `ACK` and `NACK` follow the value with no separating
space (`WRITE: 0xEDACK`); `TIMEOUT` carries a leading space
(`WRITE: 0xED TIMEOUT`).  Every write line ends with a space before the
newline.  This is firmware formatting, not part of the value.

**Two-byte commands:** the keyboard acknowledges the command byte and then
waits for the data byte, so a lone `KEYB>0xED` leaves the command open.  Send
the mask byte as the next value.

## Background (keyboard protocol — unverified)

The keyboard-side protocol behind the bytes you send.  These are common PC/AT
host commands, **not** firmware strings, and are unverified against this mode:

| Byte | Command |
|---|---|
| `0xED` | Set LEDs — follow with a bit-mask byte |
| `0xEE` | Echo |
| `0xF4` | Enable scanning |
| `0xF5` | Disable scanning |
| `0xFF` | Reset |

## Macros — `(0)` and `(1)`

Mode macros are invoked as `(n)` ([commands.md](../../generic/commands.md)).
KEYB has two:

| Macro | Action |
|---|---|
| `(0)` | macro menu — lists the available macros (printed label `0. Macro menu`) |
| `(1)` | run the live input monitor |

`(0)` prints the firmware's list:

```
 0. Macro menu
 1. Live input monitor
```

A number that is not a macro prints `Unknown macro, try ? or (0) for help`.

## Leaving the mode

- End the monitor with any terminal key (returns to `KEYB>`).
- `w` powers the keyboard down and parks the bus; `W` powers it again.
- `m` switches to another mode, which powers the interface down.

## Limits

- No key-to-character decoding and no make/break interpretation: the mode
  moves raw scancodes only.
- The monitor is a live stream with no buffer; if you need to keep it, capture
  the terminal.
- No binary (BBIO) sub-protocol — KEYB is a prompt-only mode.
- The interface always moves one 8-bit byte per exchange with its own start,
  parity and stop bits; the base `;N` width and `l` / `L` bit-order settings do
  not change the keyboard framing.
