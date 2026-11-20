# PC-AT keyboard (KEYB) — frame, parity and handshake

Part of the KEYB documentation set (`docs/protocol/keyboard/`).  This page owns
the wire format and the timing of one exchange.  What you type and see is in
[usage.md](usage.md); wiring is in [wiring.md](wiring.md).

## Scope

One byte per exchange, in either direction.  The mode does not frame or
assemble sequences: every `r` is one device-to-host frame, and every value is
one host-to-device frame.  A key's release code can therefore take two or
three separate reads.

## The frame

The interface is a synchronous, open-drain clock/data bus.  The keyboard
generates the clock; the host (the Bus Pirate) only pulls CLOCK low to inhibit
the keyboard or to request a send.

A device-to-host byte is 11 bit slots; a host-to-device byte adds a twelfth
acknowledge slot:

| Slot | Device to host (a scancode) | Host to device (a command) |
|---|---|---|
| 1 | start — DATA low | start — DATA low |
| 2–9 | eight data bits, LSB first | eight data bits, LSB first |
| 10 | parity — odd | parity — odd, computed by the firmware |
| 11 | stop — DATA released (pull-up high) | stop — DATA released (pull-up high) |
| 12 | — | acknowledge, from the keyboard |

The data bits are least-significant bit first.  **Parity is odd** in both
directions: the eight data bits plus the parity bit contain an odd number of
1s.  On a read the mode checks the parity bit; on a write it computes and
sends it.

After a host byte the keyboard answers in the twelfth slot: **DATA low is
`ACK`; DATA released (pulled high) is `NACK`**.  The mode samples the stop-bit
slot first and then the acknowledge slot; only the acknowledge slot decides
the reported result.  A low stop bit on a host write is not reported
separately.

## Two bit orders

On the wire the order is fixed: **least-significant bit first**, in both
directions.  The prompt's `l` / `L` setting is a value-level reversal, not a
wire-order change:

| Setting | Effect |
|---|---|
| `l` (default) | a typed value is sent as-is; a read is printed as received |
| `L` | the value is reversed before a write and after a read |

The `i` label `MSB set: MOST sig bit first` describes this value order, not
the wire.  At the default width of 8, `L` turns `0xED` into `0xB7` on the
wire, which is not the Set LEDs command.  Leave `l` set for host commands and
roster lookups.  The keyboard frame is always one 8-bit byte, LSB first.

## Who clocks

The keyboard clocks the bit slots in both directions; on a host write it also
clocks the acknowledge slot.

- **Read** — the Bus Pirate releases CLOCK and DATA.  If the keyboard has a
  byte to send, it pulls CLOCK low, clocks the frame, and the Bus Pirate
  samples DATA in the low phase of each bit.
- **Write** — the Bus Pirate holds CLOCK low briefly, pulls DATA low (start),
  and releases CLOCK.  The keyboard then clocks the bits; the Bus Pirate
  changes DATA while the clock is high and releases DATA for the stop bit.

Once `W` has engaged the mode, CLOCK is held low between exchanges.  That is
the host inhibit: it stops the keyboard from clocking while the prompt is
idle.  Each exchange releases it; a read re-asserts it, and a completed write
does too.  Before `W` the pins are inputs and the Bus Pirate's supply is off.

## Reading a frame

One `r` runs one read window:

1. Release CLOCK and DATA, then poll for CLOCK low.
2. If CLOCK never goes low in the window, the result is `NONE`.
3. When CLOCK is low, check DATA low for the start bit.  DATA high is a
   `*startbit error`.
4. Wait for the next CLOCK high, then read the eight data bits, least
   significant first.
5. Read the parity slot and check odd parity.
6. Read the stop slot; DATA must be high.
7. Re-assert CLOCK low for the idle inhibit.

The read window is short: 255 polls with a 5 µs delay between them, about
1.3 ms of delay time plus loop overhead.  A keyboard that is holding a byte
starts clocking as soon as CLOCK is released, so the window is normally enough
for one buffered scancode.

A clock that stops changing during the frame is a `TIMEOUT`, not `NONE`.
`NONE` means no frame started at all in the window.

## Writing a frame

One value write runs one host-to-device exchange:

1. Hold CLOCK low for about 60 µs (the host request).
2. Pull DATA low for the start bit.
3. Release CLOCK and wait for the keyboard to take over.
4. Send the eight data bits, least significant first, changing DATA while the
   clock is high and waiting for the low phase.
5. Send the parity bit.
6. Release DATA for the stop bit.
7. Sample the stop-bit slot, then the acknowledge slot.
8. Re-assert CLOCK low for the idle inhibit.

If the keyboard does not clock a bit or does not answer, the write reports
`TIMEOUT`.  If it releases DATA (pulled high), the write reports `NACK`.

## Host-write timing — unverified

**Unverified:** host writes against a real keyboard.  Two timing details are
outside the commonly cited PC-AT host-to-device window: the inhibit is about
60 µs rather than 100 µs or more, and DATA changes while CLOCK is high rather
than in the clock's low phase.  A strict keyboard may ignore the byte or
answer `NACK`; either shows up as `TIMEOUT` or `NACK` at the prompt.

Reads do not depend on either detail: the mode samples the keyboard's own
clocked frame.

## Bus state after a failure

- After `W`, a failed **read** re-asserts CLOCK low before returning, so the
  next exchange starts from the normal inhibit.
- A failed **write** can leave the keyboard mid-frame.  The next command
  starts a fresh exchange; if the keyboard stays unresponsive, cycle the
  supply with `w`, then `W`.
- Neither failure changes the mode, the width, the bit order or the display
  format.  Those are shared prompt settings.

## What the mode does not do

| Not done | Consequence |
|---|---|
| No retry after the command returns | the read window may re-sample after a frame error; issue `r` again for a fresh window |
| No sequence assembly | a release code arrives over two reads (`F0`, then the code) |
| No translation | no ASCII, no modifier state, no typematic suppression |
| No acknowledge on reads | the ACK slot exists only on host writes |
| No framing changes | the byte is always 8 bits with odd parity and a stop bit |
