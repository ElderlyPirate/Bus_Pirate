# AVRISP-MKII — getting in and out

Part of the AVRISP-MKII documentation set (`docs/programmers/avrisp/`).  This
page owns the whole lifecycle: the two entry bytes, the host-side sequence,
the idle deadline and the way back out.  The path pages
([isp.md](isp.md), [pdi.md](pdi.md), [tpi.md](tpi.md),
[serial-route.md](serial-route.md)) link here instead of repeating it.

## The two entry bytes

There is no terminal menu entry and no `AVRISP>` prompt.  The personality is
entered from the binary bitbang layer (`BBIO1`) with one byte:

| Byte | Transport | Host command | Reachable targets |
|---|---|---|---|
| `0x08` | USB mkII personality (the device re-enumerates) | `avrdude -c avrispmkII -P usb` | ISP, PDI, TPI |
| `0x09` | framed STK500v2 over the CDC port | `avrdude -c stk500v2 -P COMx` | ISP only |

Both bytes are accepted only on v4.  A v3 build answers the failure byte
`0x00` and stays in the binary layer.

## Entering from a host

A terminal emulator cannot send NUL bytes, so the entry is done by a small
host program or script.  Reaching `BBIO1` is the shared binary-layer step
(20 × `0x00`); this page owns the personality entry that follows it.

**Python / pyserial** (the same shape works on Windows, Linux and macOS):

```python
import serial

ser = serial.Serial("COM5", 115200, timeout=0.2)
for _ in range(20):
    ser.write(b"\x00")
print(ser.read(16))          # BBIO1

ser.write(b"\x08")           # 0x09 for the serial route
ser.close()                  # release the port for avrdude
```

Close the terminal program first: only one program can hold the port, and the
20 NULs are sent by the script, not typed.

Then start `avrdude` immediately (see the deadline below).  On the USB route
the CDC port disappears when the mkII device enumerates; on the serial route
the same port stays enumerated and must be free for `avrdude`.

**PowerShell** — send bytes, not characters.  Writing the character `8`
sends `0x38`, not the byte `0x08`; use the byte-array form:

```powershell
$sp = New-Object System.IO.Ports.SerialPort COM5,115200,None,8,one
$sp.Open()
$nul = [byte[]]::new(20)
$sp.Write($nul, 0, 20)                  # 20 x 0x00
Start-Sleep -Milliseconds 100
$sp.ReadExisting()                      # BBIO1
$sp.Write([byte[]]@(0x08), 0, 1)        # 0x09 for the serial route
$sp.Close()
```

## The idle deadline

- The personality ends after about **five seconds** without host traffic.
  The clock starts **when the mode is entered**, so USB enumeration and
  `avrdude` start-up are inside the same window.  Do not enter the mode and
  then look up a command.
- On the USB route only a real request renews the window.  Empty OUT packets
  and an open-but-idle handle do not.
- On the serial route any received byte renews it.
- A request restarts the window when it arrives and again when its reply is
  sent, so the deadline governs the gaps between requests, not the length of
  a command in progress.

## Getting out

The personality has no exit command and no prompt.  It ends when:

1. the idle deadline expires,
2. the USB link stalls or is reset, or
3. the board is hardware-reset.

On the way out the target is released (PDI/TPI cleanup included), the
hardware state is reset and **the binary layer prints `BBIO1` again on the
CDC port** (on the USB route the port has to re-enumerate first).  That is
the state a host sees after a failed or slow `avrdude` run — the personality
is gone, but the board is not back at the terminal yet.

From `BBIO1`, send byte **`0x0F`**: the board replies `0x01` and, on v4, runs
the terminal reset (pins to inputs, pull-ups released, supplies off, `A0`
assignment back, HEX display, BASIC left) and prints the banner and the
`HiZ>` prompt.

```
personality ends -> "BBIO1"        (binary bitbang layer)
host  -> 0x0F
host  <- 0x01
                   banner + "HiZ>" (terminal)
```

## Entering again

After the return to `BBIO1`, send `0x08` or `0x09` again for another session,
or `0x0F` to leave for the terminal.

Within the idle window the personality is still alive: a second `avrdude`
run that starts before the deadline and reopens the same device is served
without sending the entry bytes again.  Once the deadline has passed,
re-enter as above.  The entry byte is never queued for a later session.

## Related docs

- [protocol.md](protocol.md) — the command set carried over either transport.
- [isp.md](isp.md) / [pdi.md](pdi.md) / [tpi.md](tpi.md) — the target paths.
- [serial-route.md](serial-route.md) — the CDC transport in detail.
- `docs/protocol/binary/entering-and-framing.md` — the shared binary layer
  (`BBIO1`, result bytes, `0x0F`).
