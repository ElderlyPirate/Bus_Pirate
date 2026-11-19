# Edge wait (binary)

Part of the AUX capability set.  This page owns the wait-for-edge capability:
which pin it watches, how an edge and a timeout are selected, and what the
reply means.  The binary group tables and the entry framing are owned by
[protocol/binary/](../../protocol/binary/README.md); the byte-level command
map is in [protocol/binary/commands.md](../../protocol/binary/commands.md).
The other AUX binary commands are in [binary.md](binary.md).

## Scope

The wait is a **binary-layer command only**; there is no terminal command for
it.  This page covers the capability and its contracts.  It does not repeat
the BBIO1 entry sequence, the pin state/direction bytes or the sub-protocol
command groups.

## Which pin

The wait follows the AUX target selected with `c` / `C` / `k` / `K`:
`AUX` (`A0`), `CS`, and on v4 `AUX1` / `AUX2`.  Entering BBIO1 does not reset
the selection; BBIO1 leaves it alone, so a terminal selection made before
entering is still in force for the wait.

The I2C binary mode is different: its entry forces the target back to `AUX`.
Its `0x09` extended-AUX command takes sub-command `0x10` to use `AUX` and
`0x20` to use `CS` ([I2C binary](../../protocol/i2c/binary.md#aux-0x09)).
That selector can choose only `AUX` or `CS`, so an `AUX1` / `AUX2` wait is not
available inside the I2C binary mode; select it in the terminal and use BBIO1
or a nibble-group mode other than I2C instead.  The other nibble-group modes
do not change the terminal selection.  v3 offers only `AUX` and `CS`.

This is the shared binary command that follows the terminal selection.  The
state byte and the direction byte keep their AUX bit on `AUX` (`A0`): those
two bits always act on `AUX`, not `CS` or `AUX1` / `AUX2`
([protocol/binary/commands.md](../../protocol/binary/commands.md)).

## Payload and reply

In BBIO1 the command is the single byte `0x70`.  Three payload bytes follow:

| Byte | Meaning |
|---|---|
| 1 | edge selector: `0x01` rising, `0x02` falling, `0x03` either |
| 2–3 | timeout in milliseconds, 16-bit big-endian |

A normal reply is two bytes:

| Byte | Meaning |
|---|---|
| 1 | result: `0x01` the requested edge arrived, `0x00` timeout |
| 2 | the pin level at the moment the wait finished (`0` or `1`) |

A rising wait that succeeds therefore replies `0x01 0x01`; a falling wait
that succeeds replies `0x01 0x00`; an either-edge wait reports the new level.
A timeout returns `0x00` followed by the level the pin had at the timeout.

Worked exchange, waiting up to one second for a rising edge:

```
host -> 0x70 0x01 0x03 0xE8    wait for rising, 1000 ms
host <- 0x01 0x01              edge seen, pin is high
```

Waiting 100 ms for a falling edge that does not arrive:

```
host -> 0x70 0x02 0x00 0x64    wait for falling, 100 ms
host <- 0x00 0x01              timeout, pin is still high
```

## Timeout contract

- The timeout is a 16-bit count of milliseconds, 0 to 65535 (about 65.5 s).
- `0` does not wait: it always returns result `0x00` followed by the current
  level.  It is the documented way to read the selected pin with this command;
  use a non-zero timeout to wait for an edge.
- The command reports an edge only if the pin changes after the command is
  armed.  An edge that occurs while the three payload bytes are still being
  received is not latched.
- The pin is not changed by the wait: its direction and output type are left
  as they were.

## Direction contract

Configure the watched pin as an **input** before sending the wait.  The wait
does not switch direction; it samples the level and watches for a change.
A pin the firmware is driving as an output is not a reliable edge source, and
change notification on a driven output is not confirmed (unverified).

Entering BBIO1 sets `AUX`, `CS` and the bus pins to inputs, so they start in
the right direction; a direction byte can change that before the wait.
`AUX1` / `AUX2` are not part of the BBIO1 pin set and must be configured in
the terminal.

| Selected pin | How to make it an input before the wait |
|---|---|
| `AUX` (`A0`) | BBIO1 direction byte bit 4 = `1`, or `@` in the terminal |
| `CS` | BBIO1 direction byte bit 0 = `1`, or `@` in the terminal |
| `AUX1` / `AUX2` (v4) | select it (`k` / `K`), read it once with `@`, then enter BBIO1 or a nibble-group mode other than I2C |

Of the AUX targets, the BBIO1 direction byte has bits only for `AUX` and `CS`.
It has no bits for `AUX1` / `AUX2`, which is why those two must be configured
in the terminal first; the selection then persists into BBIO1.  Remember that
entering the I2C binary mode resets the selection to `AUX`.

The direction byte, its bit values and the shared sub-protocol groups are in
[protocol/binary/commands.md](../../protocol/binary/commands.md).

## Where it is available

- BBIO1: the command is exactly `0x70`.
- The nibble-group binary modes: the same wait is part of the shared `0x7x`
  group (see
  [protocol/binary/sub-protocols.md](../../protocol/binary/sub-protocols.md)).
  Only the low nibble `0` is the wait; `0x71`–`0x7F` are answered with a
  single `0x00` and consume no payload.

## Failure modes

The concrete failure cases are in [edge-cases.md](edge-cases.md); the
binary-layer recovery paths are in
[protocol/binary/edge-cases.md](../../protocol/binary/edge-cases.md).  Two
frame contracts are part of this command:

- An invalid selector is answered with one `0x00` byte only after both
  timeout bytes have arrived.
- A truncated payload blocks the layer until the missing bytes arrive; send
  the missing bytes to complete the frame, or reset the board.

## Limits

- Binary only; no terminal equivalent.
- One selected pin per wait.
- Millisecond timeout only; no microsecond form.
- `AUX1` / `AUX2` waits are v4-only and need the terminal pre-configuration;
  the I2C binary entry cannot select them.
- The wait reports "an edge occurred", not a transition count or a pulse
  width; use the frequency counter for a rate.

## Related

- [aux-pin.md](aux-pin.md) — the target selection the wait follows.
- [frequency.md](frequency.md) — counting edges instead of waiting for one.
- [binary.md](binary.md) — the other AUX binary commands.
- [wiring.md](wiring.md) — logic levels and input bias.
- [docs/protocol/binary/commands.md](../../protocol/binary/commands.md) — the
  `0x70` row, the payload and the direction bytes.
- [docs/protocol/binary/edge-cases.md](../../protocol/binary/edge-cases.md) —
  truncated frames and host/layer drift.
- [docs/SPEC.md](../../SPEC.md) — the binary-layer capability row.
