# PC keyboard (KEYB) — using the mode

Part of the KEYB documentation set (`docs/keyboard/`).  This page owns the live
input monitor, byte reads and writes, and the macro reference.

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
default**; `o` changes it ([commands.md](../generic/commands.md)).

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
- Typing on the keyboard being monitored keeps printing scancodes; a single
  character on the serial/terminal connection ends the monitor and returns to
  `KEYB>`.
- The sample above is a press and release of `A` (`1e`, `9e`) then `B`
  (`30`, `b0`).

**Unverified:** the sample scancodes are standard PC/AT Set-1 codes.

## Reading with `r`

`r` reads one scancode; `r:N` reads up to N.  The read waits for the keyboard's
clock; each attempt scans briefly for activity and gives up if the keyboard is
idle.  A successful read prints the value:

```
KEYB>r
READ: 0x1e
```

A read that finds a malformed frame prints one of the mode's error words:

| Error word | Meaning |
|---|---|
| `NONE` | no keyboard activity within the read window |
| `TIMEOUT` | the clock line did not change when expected |
| `*startbit error` | the data line was not low at the start of the frame |
| `*parity error` | the frame's parity bit did not match the data |
| `*stopbit error` | the frame did not end with the stop bit high |
| `UNKNOWN ERROR` | any other read result |

**Note:** `TIMEOUT` is reported for both reads and writes; its meaning depends
on direction — on a read the clock line did not change when expected, on a
write the keyboard did not answer.  See [edge-cases.md](edge-cases.md) for the
failure modes behind each error word.

**Unverified:** the exact placement of the error word relative to the printed
value.

## Writing to the keyboard

A value (`0xNN`, `0b…`, or decimal) sends one byte to the keyboard — a host
command.  The keyboard answers with one bit, reported as:

| Result | Meaning |
|---|---|
| `ACK` | the keyboard accepted the byte |
| `NACK` | the keyboard rejected the byte |
| `TIMEOUT` | the keyboard did not answer |

```
KEYB>0xED
WRITE: 0xEDACK
```

**Footnote:** the response word follows the value with no separating space
(`WRITE: 0xEDACK`) — a firmware formatting quirk; read it as the value
followed by `ACK` / `NACK` / `TIMEOUT`.

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

Mode macros are invoked as `(n)` ([commands.md](../generic/commands.md)).
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

- No key-to-character decoding and no make/break interpretation.
- The monitor is a live stream with no buffer; if you need to keep it, capture
  the terminal.
- The interface always moves one 8-bit byte per exchange; the base `;N` width
  and `l` / `L` bit-order settings are generic prompt features and do not change
  the keyboard's two-wire framing.