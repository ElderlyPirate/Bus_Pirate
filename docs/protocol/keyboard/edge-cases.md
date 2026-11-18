# PC-AT keyboard (KEYB) — edge cases and failure modes

Part of the KEYB documentation set (`docs/protocol/keyboard/`).  Each table
gives one condition per row and the symptom it produces; recovery steps follow
the table.

When the monitor is silent or reads fail, check in this order: **power**
(`W` for a keyboard on `5.0V`, VCC, GND), then **DATA** (it must idle high
through a pull-up; CLOCK is driven low by the mode between exchanges), then
**clock/data wiring** (DATA on MOSI, CLOCK on CLK).  Every failed read prints
its error word and then `0x00`; that value is a sentinel, not a scancode
([usage.md §Reading](usage.md#reading--r-and-rn)).

## Reads and the monitor

| Trigger | Symptom |
|---|---|
| keyboard unpowered: no supply (`W` off and no external supply), or VCC/GND open, lines floating low | `TIMEOUT` on every read; monitor silent |
| keyboard unpowered but the lines are pulled high | `NONE` on every read; monitor silent |
| keyboard powered and idle | `NONE`; monitor silent |
| first read after power-up | `AA`, `FC` or `FD` (BAT), or `NONE` while the keyboard boots |
| scanning disabled with `0xF5` | `NONE`; writes still answer |
| no keyboard attached, pull-ups on | `NONE` |
| no keyboard attached, lines floating low | `TIMEOUT` or `*startbit error` |
| typing on the keyboard does not end the monitor | scancodes keep printing |
| a terminal key was already pending before `(1)` | the monitor exits at the first check |
| a key pressed while the prompt is idle | the next `r` normally returns it; a burst can lose later codes |

- `TIMEOUT` — send `W` and check VCC and GND; then check the DATA pull-up.
  CLOCK is held low by the mode between exchanges, so an idle low CLK is
  normal, not a missing pull-up.
- `NONE` — normal at idle.  Press a key.  If nothing arrives, run the Echo
  presence test below; after `0xF5`, send `0xF4` to re-enable scanning.
- The monitor exit key is on the terminal, not the keyboard.  Clear any
  type-ahead before starting `(1)`.
- A burst of typing while the mode is idle is limited by the keyboard's own
  output buffer.  Read promptly or use the monitor; the buffer depth is
  keyboard-specific.

## Is a keyboard there?

`NONE` alone cannot separate an idle keyboard from an absent or unpowered one.
Use Echo as a presence test, with no keys pressed:

```
KEYB>0xEE
WRITE: 0xEEACK
KEYB>r
READ: 0xAA
KEYB>r
READ: 0xEE
KEYB>
```

The first read after power-up may be a BAT byte (`0xAA`, `0xFC` or `0xFD`).
Read again; a BAT byte means the keyboard is alive, but it is not the echo.
The returned `0xEE` is the positive evidence.  An `ACK` alone is weak: a DATA
line stuck low acknowledges every write.  If the write reports `NACK` or
`TIMEOUT`, check power and wiring; if reads keep returning `NONE` or `TIMEOUT`
and never `0xEE`, check the wiring too ([wiring.md](wiring.md)).

## Frame errors

| Trigger | Symptom |
|---|---|
| no pull-up; the clock never reaches its high phase | `TIMEOUT` |
| weak pull-up; the 1 bits or the stop bit read low | `*parity error` or `*stopbit error` |
| DATA on the wrong pin, or DATA held high | `*startbit error` |
| CLOCK held low by a short | `*startbit error` or `TIMEOUT` |
| noise, long wires, no common ground | `*parity error` |
| the frame ends without a high stop bit | `*stopbit error` |
| the clock stalls in the middle of a frame or a bit | `TIMEOUT` |
| any other result | `UNKNOWN ERROR` — a defensive label the read path does not produce |

- `TIMEOUT`, `*parity error`, `*stopbit error` — DATA must idle high through a
  pull-up.  CLOCK is driven low by the mode between exchanges; check its high
  phase during a frame, not at the idle prompt.  Then check the common ground
  and shorten the wiring.
- `*startbit error` — DATA must be on MOSI and driven by the keyboard; CLOCK
  must not be held low by a short.  A single one that clears on retry can be
  the keyboard's own clock-low window before its frame.
- A disconnected DATA line reads as a fixed level: high fails the start-bit
  check; low fails parity or times out.  Check DATA is on MOSI.
- `UNKNOWN ERROR` is not expected.  Retry; if it repeats, treat it as a
  wiring fault.

## Host writes

| Trigger | Symptom |
|---|---|
| write to an unpowered keyboard (no supply) | `TIMEOUT` |
| CLOCK stuck or shorted, or the keyboard not clocking | `TIMEOUT` |
| keyboard rejects the byte | `NACK` |
| DATA low at the acknowledge slot (stuck or shorted) | `ACK` for every write, even nonsense |
| keyboard clocks but leaves DATA released at the acknowledge slot | `NACK` |
| DATA stuck high or on the wrong pin | `TIMEOUT` — the keyboard sees no start |
| a two-byte command sent without its data byte | the command byte ACKs; the next value is taken as the data byte |
| `L` set (at 8 bits) | the write sends a bit-reversed byte while the reply shows the typed value |
| `;N` set | the reply is masked and suffixed `;N`; the frame stays 8 bits — reset to 8 for host commands |
| a strict keyboard and the mode's host timing | `TIMEOUT` or `NACK` ([protocol.md](protocol.md#host-write-timing--unverified)) |

- Check `W`, VCC, GND and the CLOCK/DATA wiring first.
- Check the shared settings: `l` to leave values alone, and a width of 8 (`w`
  then `W` resets it, or `r;8` sets it).
- If the keyboard stays unresponsive, send `0xFF` (reset) or cycle the supply
  with `w`, then `W`.
- A `0xFF` reset can take time before the keyboard answers again.

## Power and pull-up

| Trigger | Symptom |
|---|---|
| short on the 5 V rail | `VREG too low, is there a short?` + `POWER SUPPLIES OFF` |
| keyboard over-loads the 5 V rail | the rail sags, or `W` refuses |
| `P` sent before `W` or before `e3` | `Pull-up resistors ON` + `Warning: no voltage on Vpullup pin` |
| Bus Pirate pull-ups on with no Vpullup reference | they do nothing; reads rely on the keyboard's own |
| `w` sent with the pull-ups on | the rail is off but `P` stays set (and `e3`, if used) |

- Short: clear it and retry `W`.
- Over-load: use an external 5 V supply with a common ground.  Power the
  keyboard from that supply directly; never connect it to the `5.0V` pin
  ([wiring.md §Supply](wiring.md#supply)).
- `P` warning: the pull-up source had no voltage at that moment.  Run `W` and
  `e3`, then `P` again, or select `e1`, connect the external feed, then `P`.
  The warning is about the order, not a failed pull-up.
- After `w`, turn the pull-ups off with `p` and the source off with `e1`
  before rewiring.

## Shared settings

| Trigger | Symptom |
|---|---|
| `[`, `{`, `]`, `}`, `/`, `\`, `^`, `-`, `_`, `.` or `!` | `ERROR: command has no effect here` + `Syntax error at char 1` |
| `L` set | read values print bit-reversed; writes send a reversed byte |
| `;N` set | printed values are masked and suffixed `;N`; the frame stays 8 bits |
| RAW display (`o4`) during `(1)` | the monitor emits the byte itself, not readable text |
| `(2)` or any number other than 0 / 1 | `Unknown macro, try ? or (0) for help` |
| `m8` on a default v3 build | selects PIC, not KEYB; the mode is absent |

- The wrappers and bit operations belong to bus modes.  KEYB moves whole
  keyboard frames, so those commands are refused.  Use `r` and a value write.
- Reset the shared settings: `l` for bit order, `o1` for HEX, and a width of 8
  (`r;8`, or `w` then `W`).
- The mode has no binary path; do not look for a `KEYB` entry in the binary
  sub-protocol table ([binary/sub-protocols.md](../binary/sub-protocols.md)).

## What is not established

These are properties of the keyboard or the optional build, not of the mode's
frame handling:

| Item | Why it matters |
|---|---|
| keyboard's internal DATA/CLOCK pull-ups | if absent, fit 1–10 kΩ or enable `P` |
| keyboard's 5 V current draw | the rail can sag or `W` can refuse |
| host-write timing against a real keyboard | a strict keyboard may `TIMEOUT` or `NACK` ([protocol.md](protocol.md#host-write-timing--unverified)) |
| scancodes a keyboard buffers while inhibited | a burst can lose later codes |
| scancode set and BAT behaviour | the roster or first bytes may differ |
| optional v3 build | the mode is unverified on v3 |

## Related docs

- [usage.md](usage.md) — the monitor, reads and writes.
- [protocol.md](protocol.md) — the frame and timeout mechanics.
- [wiring.md](wiring.md) — the pull-up and supply checks.
- [generic/edge-cases.md](../../generic/edge-cases.md) — shared prompt
  failures.
