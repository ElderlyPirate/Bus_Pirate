# PC-AT keyboard (KEYB) — using the mode

Part of the KEYB documentation set (`docs/protocol/keyboard/`).  This page owns
the live monitor, reads, host writes, the mode macros, worked examples and the
limit list.  The wire format is in [protocol.md](protocol.md); scancode
meanings are in [scancodes.md](scancodes.md).

## Scope

What you type at `KEYB>` and what comes back.  Entry and `W` are in
[entry-and-setup.md](entry-and-setup.md); wiring is in [wiring.md](wiring.md).
Shared prompt grammar (`r:N`, `;N`, `o`, `l`/`L`, `(n)`, `W`, `P`) belongs to
[docs/generic/](../../generic/README.md) and is not repeated here.

## Before the first byte

1. Enter the mode ([entry-and-setup.md](entry-and-setup.md)).
2. Send `W` to switch on the supply and engage the interface.  A keyboard
   powered from `5.0V` needs this; an externally powered keyboard can clock
   without it, but `W` also engages the idle inhibit.
3. If the keyboard has no pull-ups, enable the Bus Pirate's (`e3`, then `P`)
   or fit external resistors ([wiring.md](wiring.md)).
4. Leave the display at HEX (`o1` if it was changed), the width at 8 and the
   bit order at `l`.  Width and bit order reset on entry; the display format
   is carried over from the previous mode.

## First bytes after power-up

A PC-AT keyboard runs a power-on self-test (BAT) when it receives power.  Its
first bytes after `W` can be the self-test result, not a key:

| Byte | Meaning |
|---|---|
| `0xAA` | BAT passed |
| `0xFC`, `0xFD` | BAT failed |
| `0xFA` | a command was acknowledged |

Allow the keyboard a moment after `W` before pressing keys.  The first `r` or
`(1)` may return `0xAA` (or `0xFC`/`0xFD`); that is not a scancode.  A reset
(`0xFF`) is usually answered by `0xFA` then `0xAA`.  The first read can also
be `NONE` while the keyboard boots.  See [scancodes.md](scancodes.md).

## The live monitor — `(1)`

`(1)` prints two header lines and then streams a value for every scancode the
keyboard sends:

```
KEYB>(1)
Input monitor, any key exits
Any key to exit
0x1C 0xF0 0x1C 0x32 0xF0 0x32

KEYB>
```

The example is a press and release of `A` (`1C`, then `F0 1C`) followed by a
press and release of `B` (`32`, then `F0 32`), using the standard Set 2 table
([scancodes.md](scancodes.md)).

- Both headers mean **a key on the terminal** (the serial connection), not a
  key on the keyboard being monitored.  Any terminal byte ends the monitor
  and returns to `KEYB>`.
- Each scancode is printed as it arrives, separated by spaces, with no line
  breaks.  The stream is live and unbuffered.
- A frame that fails its checks prints nothing in the monitor; only valid
  scancodes appear.  The error words belong to `r`.
- The monitor prints values in the active display format.  RAW (`o4`) sends
  the byte itself, which a terminal may not render as text.
- Typing on the keyboard keeps printing scancodes; the monitor exits only on
  a terminal key.  A terminal key that was already pending (type-ahead) ends
  the monitor at the first check.

**Spacing:** the monitor prints a space after every value, including the last
one before the exit.  The blank line before the prompt is the monitor's
closing line break plus the macro command's own line break.

The sample codes are the standard PC-AT Set 2 codes.  Which set a specific
keyboard sends is a property of that keyboard.

## Reading — `r` and `r:N`

`r` performs one read window and prints the value:

```
KEYB>r
READ: 0x1C
KEYB>
```

`r:N` performs exactly N read windows.  N independent attempts, one window
each; it does not stop early on an error.  The `READ: ` header is printed once
for the whole command.  Two buffered frames:

```
KEYB>r:2
READ: 0x1C 0xF0
KEYB>
```

After a press and release of `A`, the first read took the make code; the
second took the `F0` break prefix, so a third read would return `0x1C`.

A failed read prints the mode's error word **and then the value the read
returned**.  That value is always zero, so every failure prints `0x00` (HEX
default) after the error word — a sentinel, not a scancode.  All six error
words are line messages, so every failure is two lines:

```
KEYB>r
READ:  NONE
0x00
KEYB>
```

```
KEYB>r
READ:  *parity error
0x00
KEYB>
```

The two spaces after `READ:` are the header's trailing space plus the error
word's leading space.  With `r:N` the pairs repeat; the next error word
continues after the previous value's separator space:

```
KEYB>r:2
READ:  NONE
0x00  NONE
0x00
KEYB>
```

Both `NONE` words are failed reads; each `0x00` is that read's sentinel.

| Error word | Meaning |
|---|---|
| `NONE` | no frame started within the read window |
| `TIMEOUT` | the clock stopped changing while a frame or a bit was expected |
| `*startbit error` | the clock started a frame while DATA was high |
| `*parity error` | the eight data bits and the parity bit are not odd parity |
| `*stopbit error` | the frame did not end with the stop bit high |
| `UNKNOWN ERROR` | defensive default label; the current read path does not produce it |

A successful read prints `READ: ` and the value.  The dispatcher prints a
space after each value before the line break.

### Why a single `r` usually finds a key

After `W`, the mode holds CLOCK low between exchanges, so the keyboard cannot
send while the prompt is idle.  A key pressed while idle leaves a scancode in
the keyboard's output buffer; the next `r` releases CLOCK and the keyboard
clocks it promptly.  The first read after power-up may return a BAT byte
instead ([First bytes after power-up](#first-bytes-after-power-up)).  For a
burst of typing, read promptly or use `(1)`: how many scancodes a given
keyboard holds while inhibited is keyboard-specific; assume one.

## Writing to the keyboard

A value (`0xNN`, `0b…`, or decimal) sends one host byte.  The keyboard answers
in the frame's acknowledge slot:

| Result | Meaning |
|---|---|
| `ACK` | the keyboard accepted the byte |
| `NACK` | the keyboard released DATA (pulled high) in the acknowledge slot |
| `TIMEOUT` | a clock wait expired; the keyboard did not finish the exchange |

A one-byte command:

```
KEYB>0xEE
WRITE: 0xEEACK
KEYB>
```

A keyboard that rejects the byte releases DATA in the acknowledge slot:

```
KEYB>0xEE
WRITE: 0xEENACK
KEYB>
```

**Spacing:** `ACK` and `NACK` follow the value with no separating space, and
the line ends with a trailing space.  `TIMEOUT` is a line message with a
leading space, so the dispatcher's following space lands on the next line as
a near-blank line:

```
KEYB>0xEE
WRITE: 0xEE TIMEOUT

KEYB>
```

With `l` (the default), only the **low eight bits** of the typed value go on
the wire.  `l`/`L` is a value-level setting, not a wire-order change; the
frame is always LSB-first
([protocol.md §Two bit orders](protocol.md#two-bit-orders)).  Leave the width
at 8 and the bit order at `l` for host commands (see
[edge-cases.md §Shared settings](edge-cases.md#shared-settings)).

### Two-byte commands

Some commands need a data byte.  The keyboard acknowledges the command byte
and then waits for the data byte, so send it as the next value.  `0xED` (Set
LEDs) is the common case; the mask turns the lock LEDs on:

| Mask bit | LED |
|---|---|
| 0 | Scroll Lock |
| 1 | Num Lock |
| 2 | Caps Lock |

```
KEYB>0xED
WRITE: 0xEDACK
KEYB>0x01
WRITE: 0x01ACK
KEYB>
```

A lone `0xED` write leaves the command open: the next value is taken as the
mask byte, whatever it was meant to be.  In the example, `0x01` sets bit 0,
Scroll Lock.

If the command byte reports `NACK` or `TIMEOUT`, do not send the data byte:
the keyboard is not waiting for it.  Retry the command.

### Sending several raw bytes (a string)

A quoted string sends one **raw byte per character**: the character's ASCII
value, not text for the host and not a scancode.

```
KEYB>"abc"
WRITE: "aACKbACKcACK"
KEYB>
```

This sends `0x61`, `0x62`, `0x63` as three host-to-device command bytes.  It
does not type `abc` on the host; `a` is `0x61`, not the `A` key.  None of those
bytes is a standard keyboard command, so a keyboard may ignore them or `NACK`.
The `ACK` in the capture only means each frame was accepted, not that a
character was typed.  Use a string only for a sequence of raw host bytes; for
ordinary commands, send one value per command.

Each character is echoed and its ACK follows; after the last character the
closing quote and the line break are printed.  The string form is the shared
prompt's value syntax ([generic/values.md](../../generic/values.md)); it does
not add any framing of its own.

## Common host commands

These are the keyboard-side PC-AT command bytes.  They are **not** firmware
strings: the mode moves whatever byte you give it and reports the acknowledge
slot.  The byte values and responses are the keyboard's protocol; responses
and timing vary by keyboard.

| Byte | Command | Follow-up |
|---|---|---|
| `0xED` | Set LEDs | one mask byte (bit 0 Scroll, bit 1 Num, bit 2 Caps) |
| `0xEE` | Echo | the keyboard returns `0xEE`; use as a presence test |
| `0xF0` | Select scancode set | one byte: `0x01`, `0x02` or `0x03` |
| `0xF2` | Read keyboard ID | two ID bytes follow |
| `0xF3` | Set typematic rate/delay | one byte: bits 0–4 rate, bits 5–6 delay |
| `0xF4` | Enable scanning | scancodes resume |
| `0xF5` | Disable scanning | no scancodes until `0xF4` |
| `0xFE` | Resend | the keyboard repeats its last response |
| `0xFF` | Reset | self-test; usually `0xFA`, then `0xAA` |

`0xF3`'s delay field is bits 5–6: `00` is 250 ms, `01` is 500 ms, `10` is
750 ms and `11` is 1000 ms.  Bits 0–4 select the repeat rate, 0–31.

Two different acknowledgements are in play.  The frame's ACK/NACK slot is
reported on the `WRITE:` line.  The keyboard's `0xFA` response is a data byte
and is read with `r`.

A command's own response, if any, arrives as one or more normal
device-to-host frames; read them with `r`.  A keyboard often sends `0xFA`
(command acknowledged) as a data byte first, so a read can show `0xFA` before
the response you expect.

A reset (`0xFF`) can take time; the keyboard may not answer immediately.
Send `W` before any command on a keyboard fed from `5.0V`: an unpowered
keyboard cannot clock the frame and every write reports `TIMEOUT`.

## Macros — `(0)` and `(1)`

Mode macros are invoked as `(n)`; the grammar is in
[generic/macros.md](../../generic/macros.md).  KEYB has two:

| Macro | Action |
|---|---|
| `(0)` | macro list |
| `(1)` | live input monitor |

`(0)` prints the firmware's list:

```
KEYB>(0)
 0. Macro menu
 1. Live input monitor

KEYB>
```

The leading spaces are part of the firmware's strings; the blank line before
the prompt is the macro command's closing line break.  A number that is not a
macro prints:

```
KEYB>(2)
Unknown macro, try ? or (0) for help

KEYB>
```

## Worked examples

**Watch a key and its release.**  Enter, `W`, then `(1)`; press and release
`A`.  The monitor prints `0x1C 0xF0 0x1C`.  A terminal key ends it.

**Turn on Caps Lock.**  `W`, then `0xED`, then `0x04` (bit 2 set).  Both bytes
must acknowledge; the keyboard is asked to light Caps Lock.

**Stop the keyboard from sending.**  `W`, then `0xF5`.  Writes still answer
`ACK`; once any already-buffered scancode is read, `r` reports `NONE` and the
monitor stays silent until `0xF4`.

**Read a key without the monitor.**  Press and hold a key, then send `r`.  A
single scancode is returned.  Holding the key produces the keyboard's
typematic repeats; each repeat is another frame.

## Leaving

| Action | Effect |
|---|---|
| any terminal key during `(1)` | ends the monitor |
| `w` | supply off, mode idle |
| `m` / `m<N>` | switches mode; the board reset releases the pins and the supply |
| `#` | terminal reset to HiZ; restores HEX and releases the pins |

## Limits

- **Raw bytes only.**  No key-to-character decoding, no make/break
  interpretation, no modifier state and no typematic handling.
- **One byte per exchange.**  A release code that is two bytes needs two
  reads; the mode does not assemble sequences.
- **No monitor buffer.**  The monitor is a live stream; capture the terminal
  if you need a record.
- **No binary (BBIO) path.**  KEYB is prompt-only; the binary sub-protocol
  table has no keyboard entry.
- **No acknowledge on reads.**  The ACK slot exists only on host writes.
- **Host writes use timing outside the classic host-to-device window**; a
  strict keyboard may ignore the byte or answer `NACK`
  ([protocol.md](protocol.md#host-write-timing--unverified)).
- **v3 is not verified.**  The default v3 build does not include the mode.

## Related docs

- [protocol.md](protocol.md) — the frame, parity, ACK and timeout mechanics.
- [scancodes.md](scancodes.md) — make/break and the Set 2 roster.
- [edge-cases.md](edge-cases.md) — failure modes and formatting traps.
- [entry-and-setup.md](entry-and-setup.md) — mode entry and `W`.
- [generic/macros.md](../../generic/macros.md) — the `(n)` / `<n>` grammar.
