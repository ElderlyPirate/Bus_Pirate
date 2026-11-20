# UART — on the bus (sending, reading, the live display)

Part of the UART documentation set (`docs/protocol/uart/`).  How values go
out, how they are read, and how the buffered live display works.  The shared
prompt grammar for entering values, `r`, wrappers and formatting lives in
`docs/generic/` — see [commands.md](../../generic/commands.md) and
[values.md](../../generic/values.md) — and is not repeated here.  Error flags
are covered in [errors.md](errors.md).

## Sending

A value sent at the `UART>` prompt is echoed as `WRITE: <value>` and then
transmitted on TX.  Values are accepted in the same forms as everywhere:

| Form | Example | Sends |
|---|---|---|
| hex | `0x55` | one value |
| binary | `0b01010101` | one value |
| decimal | `85` | one value |
| text | `"OK"` | each character in turn |

Separate values with a space or comma.  A `;N` suffix after a value sets that
value's width and stays in force for later values until changed — for example
`0x1FF;9`; `:N` repeats the value.  See
[values.md](../../generic/values.md) for the delimiter and width rules.

A decimal value is masked to the current frame width: **0–255** in an 8-bit
frame, **0–511** in a 9-bit frame.  A wider number is displayed and sent with
only its low bits — `300` in an 8-bit frame becomes `0x2C` on the wire.  A
`;N` above nine widens only the displayed value; the UART frame still carries
eight or nine data bits.

A quoted string sends the characters literally; it has **no escape syntax**.
`"OK"` sends `O` then `K`.  `"AT\r"` sends the four characters `A`, `T`, `\`
and `r` — use `0x0D` to send a carriage return.

`WRITE:` means the value was handed to the UART transmitter.  UART has **no
wire-level acknowledgement**, so it does not mean the target received it.

In 9-bit mode the echo carries the width suffix, for example
`WRITE: 0x01FF;9`.

## Reading

`r` reads one value and prints `READ: <value>`:

```
UART>r
READ: 0x55
```

`r:N` reads N values.  A counted read prints **one** `READ:` line with the
values space-separated:

```
UART>r:4
READ: 0x42 0x43 0x44 0x45
```

Only the live display repeats the `READ:` prefix per word.  `r;bits` sets the
display/bit-order width; the width stays in force until changed (see
[values.md](../../generic/values.md)).

A read with nothing waiting is not a bus error, but the firmware's rendering
is not a clean sentence:

```
UART>r
READ: FAILED, NO DATA0x00
```

The no-data token `FAILED, NO DATA` is printed without a trailing newline,
and the prompt then prints the zero the read returns.  The result is the
glued `0x00` above.  In decimal display the tail is `0`, in binary
`0b00000000`, and in RAW display it is an invisible NUL byte.  A counted read
that runs out of words mixes values and tokens:

```
UART>r:4
READ: 0x41 FAILED, NO DATA0x00 FAILED, NO DATA0x00 FAILED, NO DATA0x00
```

A byte with a parity or framing error is still delivered; `r` prints the flag
**before** the value (`READ: -p 0x40`, `READ: -p -f 0x40`).  See
[errors.md](errors.md).

## The live display

`[` or `{` starts a continuous display: every incoming word is printed as
`READ: <value>` as it arrives.  `]` or `}` stops it.

```
UART>{
UART LIVE DISPLAY, } TO STOP
UART>0x41
WRITE: 0x41
UART>READ: 0x41
UART>}
LIVE DISPLAY STOPPED
UART>
```

| Point | Behaviour |
|---|---|
| `[` and `{` | start the same display |
| `]` and `}` | stop it |
| wire effect | none; neither character is sent on the UART wire |
| stopping | only the display stops; queued words stay in the ring, so a word received after the stop is shown by the next `r` or display (until the ring fills) |
| after `W` | the display does not start by itself; open it with `[`/`{` |
| prompt | reprinted after each burst of displayed words; several `READ:` lines appear together only when several words were already queued |
| format | follows `o1` HEX, `o2` DEC, `o3` BIN, `o4` RAW and the current width |
| half duplex | the Bus Pirate's own transmitted words do not appear as reads — see [hduart.md](hduart.md) |

In the display a flag is printed **immediately after** the value, with no
space between them, for example `READ: 0x40-f` and `READ: 0x41-p -f`.  This
is the opposite placement from `r`; the flag strings still end with a space.

The high-throughput alternative to the display is the live monitor macro
`(2)`, which forwards raw bytes with no `READ:` formatting — see
[macros.md](macros.md).

## Receive buffer and overruns

A four-word hardware receive FIFO is drained by an interrupt into a
**127-usable-word software ring** (128 slots, one kept free).  Each entry
keeps the word's **ninth data bit** and its **parity/framing flags**, so
9-bit frames and per-word errors survive the buffering.

This absorbs short bursts at any speed.  Sustained traffic faster than the
host drains it eventually fills the ring; the next word is read and dropped,
and the overrun counter increments.  Outside a bridge/monitor nothing is
printed and no LED changes; no `*Bytes dropped*` message is produced.  See
[errors.md §Overruns](errors.md#overruns) for the counter and the MODE LED.

To move a lot of data, use the transparent bridge `(1)` or the live monitor
`(2)`, which hand bytes straight to the host in a 1:1 stream.  The
bridge and monitor check that the host link can accept a byte before
dequeuing the next word (v4 CDC readiness; v3 terminal transmitter space).
That is a **liveness aid, not flow control**: a stalled host can still fill
and overrun the finite ring.  Pace the traffic.

Two buffer sizes are often confused:

| Object | Size | Meaning |
|---|---|---|
| UART receive ring | 127 usable words | the per-mode receive queue described above |
| terminal output buffer | 4096 bytes on both builds | the firmware's shared terminal output buffer, separate from the UART ring |

The 4096-byte terminal output buffer is not extra receive capacity for UART.

The ring is reset when the port is enabled — after `W`, after a binary
speed/frame change, and when `(4)` re-runs the setup — and it is also cleared
when the port is disabled (`w`, a mode change, a reset).  Words queued before
one of those steps are lost.  The overrun counter is different: a disable
leaves it intact, and the next enable resets it.

## Example session

A full-duplex loopback (TX tied to RX) at 9600 bps, 8/NONE, normal output
(the looped line needs no pull-up).  The one-line entry stores the settings;
`W` applies them:

```
HiZ>m3 5 1 1 1 2 1
UART (spd brg dbp sb rxp hiz dup)=( 4 416 0 0 0 0 0 )
Ready
UART>W
POWER SUPPLIES ON
Clutch engaged!!!
UART>0x41
WRITE: 0x41
UART>r
READ: 0x41
UART>0x42:4
WRITE: 0x42 0x42 0x42 0x42
UART>r:4
READ: 0x42 0x42 0x42 0x42
UART>r
READ: FAILED, NO DATA0x00
```

With the display open, a write and its loopback appear as separate lines:

```
UART>{
UART LIVE DISPLAY, } TO STOP
UART>0x41
WRITE: 0x41
UART>READ: 0x41
UART>}
LIVE DISPLAY STOPPED
UART>
```

The prompt text, the `WRITE:` echo, the `READ:` lines and the `FAILED, NO
DATA` token are the firmware's wording.  The exact values depend on the peer;
a loopback shows the same values without a second device.

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| `WRITE:` / `READ:` echo | same | same |
| `r:N` one line | same | same |
| `[`/`{` start, `]`/`}` stop | same | same |
| live-display wording | `UART LIVE DISPLAY, } TO STOP` | same |
| receive ring | 127 usable words | 127 usable words |
| terminal output buffer | 4096 bytes | 4096 bytes |
| no-data token | `FAILED, NO DATA` + zero | same |
| 9-bit width suffix | same | same |

## Related docs

- [entry-and-setup.md](entry-and-setup.md) — the settings and `W`.
- [errors.md](errors.md) — flags, overruns and failed reads.
- [macros.md](macros.md) — the live monitor `(2)` and the macros.
- [bridge.md](bridge.md) — the transparent bridge.
