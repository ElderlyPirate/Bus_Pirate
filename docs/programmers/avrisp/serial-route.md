# AVRISP-MKII — serial route (framed STK500v2 over CDC)

Part of the AVRISP-MKII documentation set.  This page owns the **serial**
transport: the same STK500v2 engine driven as AVR068 frames over the Bus
Pirate's USB CDC port.  The entry/exit lifecycle is in [entry.md](entry.md);
the command catalogue is in [protocol.md](protocol.md).

## What it is

The v4 Bus Pirate already exposes a USB CDC virtual serial port (the
terminal port).  The serial entry byte makes that same port carry the
framed STK500v2 protocol instead of the terminal, so a stock `avrdude` can
program an ISP target with **no libusb driver**.  It identifies itself to
the host as `STK500_2` so `avrdude` keeps the framed serial path.

## When to use it

- You do not want to bind a WinUSB/libusb driver to the mkII USB device.
- You only need **ISP** programming with `avrdude` (the standard host path
  does not enable XPROG over this route).
- You are already using the Bus Pirate's serial port.

For PDI/TPI with `avrdude`, use the USB route: the entry byte and deadline
are in [entry.md](entry.md), and the target paths are in [pdi.md](pdi.md)
and [tpi.md](tpi.md).  For the mkII USB identity, see
[host-tools.md](host-tools.md).

## Entry

1. Open the Bus Pirate's serial port at its normal settings — **115200
   baud, 8N1**.  The CDC link does not depend on the baud rate; the setting
   is the terminal convention, and the target SCK is selected separately.
2. Send 20 × `0x00` from the host (not a terminal — see
   [entry.md](entry.md)), read `BBIO1`, then send byte **`0x09`**.
3. **Close the terminal / release the port**, then run `avrdude` against
   the same COM port (the port stays enumerated; only the protocol
   changes).

The host-side snippet and the five-second idle deadline are in
[entry.md](entry.md).

```
avrdude -c stk500v2 -P COM5 -p m328p -U flash:w:app.hex:i
```

`COM5` and `m328p` are examples — use the port and the part name the host
tool knows.  `COM5` is a Windows example; use `/dev/ttyACM0` on Linux or the
`/dev/cu.usbmodem…` name on macOS.

A read-only signature check:

```
avrdude -c stk500v2 -P COM5 -p m328p -n -U signature:r:-:h
```

## Framing (AVR068)

Each frame is:

| Field | Size | Value |
|---|---|---|
| start | 1 | `0x1B` |
| sequence | 1 | echoed back in the reply |
| size | 2 | payload length, big-endian |
| token | 1 | `0x0E` |
| payload | n | the STK500v2 command and its data |
| checksum | 1 | XOR of all preceding bytes |

### Worked frame

A `SIGN_ON` request (`0x01`) on sequence `0x01`:

```
host -> 1B 01 00 01 0E 01 14
host <- 1B 01 00 0B 0E 01 00 08 53 54 4B 35 30 30 5F 32 02
```

| Byte(s) | Meaning |
|---|---|
| `1B` | start |
| `01` | sequence, echoed |
| `00 01` | payload length = 1 |
| `0E` | token |
| `01` | payload: `SIGN_ON` |
| `14` | checksum |

The reply payload is `01 00 08` followed by the 8-byte identity
`STK500_2` (`53 54 4B 35 30 30 5F 32`): command, `CMD_OK`, string length,
identity.  Eleven payload bytes make the size `00 0B`; the checksum is
`02`.

A parameter read on sequence `0x02`:

```
host -> 1B 02 00 02 0E 03 91 87
host <- 1B 02 00 03 0E 03 00 01 16
```

`03 91` is `GET_PARAMETER` for software-major; the reply payload is
`03 00 01`.

- On a **checksum error** the Bus Pirate replies with the AVR068 error
  pair `0xB0`, `0xC1` rather than dropping the frame silently.  The
  sequence is echoed so the host can match the reply.
- **Malformed or oversized frames are drained and resynchronised** with
  bounded waits, so a stray byte does not wedge the link.  A frame whose
  payload is larger than the request buffer, or whose size is zero, is
  drained to its end and then discarded; the host sees no reply for that
  frame.
- A truncated frame is abandoned after the bounded per-byte wait and the
  parser resynchronises on the next `0x1B`.
- The payload is exactly the same STK500v2 command set used on the USB
  route — see [protocol.md](protocol.md).

## Behaviour and limits

- **ISP with `avrdude`.**  PDI and TPI require the USB route because
  `avrdude` only enables the XPROG protocol for mkII-class host identities.
  The firmware engine behind this route also accepts XPROG, so a custom
  host can drive PDI or TPI over the serial framing; that combination is
  **not verified**.
- This is **not** a hardware-UART personality and **not** a baud-switching
  mode.  Host baud settings do not set an electrical target clock; the
  target SCK is selected by the `SCK_DURATION` parameter, as on the USB
  route.
- The route runs on the same engine as the USB personality: same speed
  ladder, same 256-byte request limit, same reset behaviour and the same
  idle exit.  When the mode ends the binary layer prints `BBIO1` on the
  same port; `0x0F` then returns to the terminal ([entry.md](entry.md)).
- The Bus Pirate does not power the target; wiring is in
  [wiring.md](wiring.md).
- Only one program can hold the CDC port.  The entry script and `avrdude`
  must run in sequence, not at the same time.

## Relationship to the USB route

Both routes share one protocol engine.  The only differences are the
transport framing, the host driver requirement, and the reachable target
interfaces with `avrdude` (the USB route adds PDI/TPI).  Do not run both at
once: the serial route occupies the same CDC port the terminal uses.

The USB route carries the raw payload with no AVR068 header, in 64-byte
bulk packets, and is documented in [protocol.md](protocol.md) §USB
transport.

## Related docs

- [entry.md](entry.md) — how the route is entered and left.
- [protocol.md](protocol.md) — command catalogue and the USB transport.
- [command-model.md](command-model.md) — the session model.
- [sessions.md](sessions.md) — worked serial frames.
- [isp.md](isp.md) — the ISP flow and the host commands.
- [wiring.md](wiring.md) — the ISP wiring this route uses.
