# UART — on the bus (sending, reading, the live display)

Part of the UART documentation set (`docs/uart/`).  How bytes go out, how
they are read, and how the buffered live display works.  The shared prompt
grammar for entering values, `r`, wrappers and formatting lives in
`docs/generic/` — see [commands.md](../../generic/commands.md) — and is not
repeated here.  Error flags are covered in [errors.md](errors.md).

## Sending

A number sent at the `UART>` prompt is echoed as `WRITE: <value>` and then
transmitted on TX.  Values are accepted in the same forms as everywhere:

| Form | Example |
|---|---|
| hex | `0x55` |
| binary | `0b01010101` |
| decimal | `85` |
| text | `"AT\r"` (each character is sent in turn) |

Separate values with a space or comma.  The `;` suffix sets the bit width
for the following value, for example `0x55;7`.  See
[commands.md](../../generic/commands.md) for the full delimiter and width
rules.

## Reading

`r` reads one byte and prints `READ: <value>`:

```
UART>r
READ: 0x55
```

`r:N` reads N bytes; `r;bits` sets the width for the read.  A read with
nothing waiting is not an error condition on the bus — it prints:

```
UART>r
READ: FAILED, NO DATA
```

The value is formatted according to the global display mode (`o`): hex by
default, decimal or binary on request.

## The live display

`[` or `{` opens a continuous display: every incoming byte is printed as
`READ: <value>` as it arrives.  `]` or `}` stops it.

```
UART>{
UART LIVE DISPLAY, } TO STOP
READ: 0x41
READ: 0x42
READ: 0x43

UART>}
LIVE DISPLAY STOPPED
```

- On this firmware **`[` and `{` behave identically** — both open the
  display; `]` and `}` both stop it.  The older split ("`[` discards,
  `{` displays") does not describe the current behaviour.
- The display shows the bytes the Bus Pirate receives.  It does not
  start automatically after `W`; open it with `[`/`{`.
- A byte received while the display is closed stays unread and is shown
  when a read is next issued (or by the next display).
- In half-duplex mode the Bus Pirate's own transmitted bytes do not appear
  as reads — see [hduart.md](hduart.md).

The high-speed alternative to the display is the live monitor macro `(2)`,
which forwards raw bytes with no `READ:` formatting — see
[macros.md](macros.md).

## Receive buffer and overruns

The Bus Pirate has a small hardware receive FIFO backed by a **128-byte
software buffer** that an interrupt keeps filled.  This absorbs short bursts
at any speed; sustained traffic faster than the host drains it eventually
fills the buffer, and further bytes are dropped silently (the firmware
counts the drops but does not print a message — see [errors.md](errors.md)).

To move a lot of data without loss, use a transparent bridge (`(1)`) or the
raw live monitor (`(2)`), which hand bytes straight to the host in a 1:1
stream — see [macros.md](macros.md).

## Worked session

Full duplex at 9600 bps against a peer that echoes what it receives.  The
setup prompts are abridged (`…`); the terminal is 8N1.

```
HiZ>m
… (select 3, speed 5, data 1, stop 1, polarity 1, output 1, duplex 1)
Ready
UART>W
POWER SUPPLIES ON
Clutch engaged!!!
UART>{
UART LIVE DISPLAY, } TO STOP
READ: 0x41
READ: 0x42
READ: 0x43

UART>}
LIVE DISPLAY STOPPED
UART>0x41
WRITE: 0x41
UART>r
READ: 0x41
UART>r:4
READ: 0x42
READ: 0x43
READ: 0x44
READ: 0x45

UART>
```

The exact byte values depend on the peer; the prompt text, the `WRITE:`
echo and the `READ:` lines are the firmware's wording.  A loopback
(TX and RX tied) is the simplest way to see the same bytes come back.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| `WRITE:` / `READ:` echo | same | same |
| `[`/`{` open, `]`/`}` stop | same | same |
| live-display wording | `UART LIVE DISPLAY, } TO STOP` | same |
| receive buffer | 128-byte software buffer | 128-byte software buffer |

The v3 column is **not verified**.