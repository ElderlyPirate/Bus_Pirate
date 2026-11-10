# AVRISP-MKII — serial route (framed STK500v2 over CDC)

Part of the AVRISP-MKII documentation set (`docs/avrisp/`).  This page owns
the **serial** transport: the same STK500v2 engine driven as AVR068 frames
over the Bus Pirate's USB CDC port.

## What it is

The v4 Bus Pirate already exposes a USB CDC virtual serial port (the
terminal port).  The serial entry byte makes that same port carry the
framed STK500v2 protocol instead of the terminal, so a stock `avrdude` can
program an ISP target with **no extra USB driver**.  It identifies itself
to the host as `STK500_2` so `avrdude` keeps the framed serial path.

## When to use it

- You do not want to bind a WinUSB/libusb driver to the mkII USB device.
- You only need **ISP** programming (the route cannot reach PDI or TPI).
- You are already using the Bus Pirate's serial port.

For PDI/TPI and for the mkII USB identity, use the USB route
([isp.md](isp.md) §Entry).

## Entry

1. Open the Bus Pirate's serial port at its normal settings — **115200
   baud, 8N1**, default terminal settings.
2. Enter binary bitbang mode (BBIO1) and send byte **`0x09`**.
3. **Close the terminal / release the port**, then run `avrdude` against
   the same COM port (the port stays enumerated; only the protocol
   changes).

```
avrdude -c stk500v2 -P COMx -p <part> -U flash:w:<file>.hex:i
```

A read-only signature check:

```
avrdude -c stk500v2 -P COMx -p <part> -n -U signature:r:-:h
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

- On a **checksum error** the Bus Pirate replies with the AVR068 error
  pair `0xB0`, `0xC1` rather than dropping the frame silently.
- **Malformed or oversized frames are drained and resynchronised** with
  bounded waits, so a stray byte does not wedge the link.
- The payload is exactly the same STK500v2 command set used on the USB
  route — see [protocol.md](protocol.md).

## Behaviour and limits

- **ISP only.**  PDI and TPI require the USB route because `avrdude` only
  enables the XPROG protocol for mkII-class host identities.
- This is **not** a hardware-UART personality and **not** a baud-switching
  mode.  Host baud settings do not set an electrical target clock; the
  target SCK is selected by the `SCK_DURATION` parameter, as on the USB
  route.
- The route runs on the same engine as the USB personality: same speed
  ladder, same 256-byte request limit, same reset behaviour and the same
  idle exit.  At idle the port returns to the terminal prompt.
- The Bus Pirate does not power the target; wiring is in
  [wiring.md](wiring.md).

## Relationship to the USB route

Both routes share one protocol engine.  The only differences are the
transport framing, the host driver requirement, and the reachable target
interfaces (the USB route adds PDI/TPI).  Do not run both at once: the
serial route occupies the same CDC port the terminal uses.