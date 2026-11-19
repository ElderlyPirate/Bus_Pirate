# AVRISP-MKII — getting in and out

Part of the AVRISP-MKII documentation set.  This page owns the build
availability, the two entry bytes, the host-side sequence, the idle deadline
and the way back out.  The path pages link here instead of repeating it.

## Build availability

The personality is compiled into the **default v4 image**.  It is not a
terminal mode: there is no `m` menu entry, no `AVRISP>` prompt and no `?`
entry.

| Build | State | What the entry bytes do |
|---|---|---|
| v4 default | included | `0x08` and `0x09` enter the personality |
| v4 all-features custom set | **not included** | both answer `0x00`; the binary layer reprints `BBIO1` |
| v3, any build | not available | the build stops if the switch is forced; a prebuilt image answers `0x00`, then `BBIO1` |

The all-features custom block is a fixed v4 list that leaves AVRISP-MKII
out.  The custom-build switch is `BP_ENABLE_AVRISP_MKII_SUPPORT`; a build
that adds it to the feature list has the personality.  The build route is
[dev/building.md](../../dev/building.md).  On v3 the personality cannot be
built at all — see the guard in the build page rather than trying to define
the switch.

If `0x08` or `0x09` is answered with `0x00` and `BBIO1`, the image in front of
you does not include the personality.  No wiring or host change fixes that.

## The two entry bytes

There is no terminal menu entry and no `AVRISP>` prompt.  The personality is
entered from the binary bitbang layer (`BBIO1`) with one byte:

| Byte | Transport | Host command | Reachable targets |
|---|---|---|---|
| `0x08` | USB mkII personality (the device re-enumerates) | `avrdude -c avrispmkII -P usb` | ISP, PDI, TPI |
| `0x09` | framed STK500v2 over the CDC port | `avrdude -c stk500v2 -P COM5` | ISP with `avrdude`; the engine also accepts XPROG |

`COM5` is a Windows example; use `/dev/ttyACM0` on Linux or the
`/dev/cu.usbmodem…` name on macOS.

Both bytes are accepted only from `BBIO1`.  A byte sent to the terminal is
terminal input; it does not enter the personality.

## Entering from a host

A terminal emulator normally cannot send NUL bytes, so the entry is done by
a small host program or script.  Reaching `BBIO1` is the shared binary-layer
step (20 × `0x00`); this page owns the personality entry that follows it.

**Python / pyserial** (the same shape works on Windows, Linux and macOS):

```python
import serial

ser = serial.Serial("COM5", 115200, timeout=0.2)
for _ in range(20):
    ser.write(b"\x00")
print(ser.read(5))          # BBIO1

ser.write(b"\x08")           # 0x09 for the serial route
ser.close()                  # release the port for avrdude
```

Close the terminal program first: only one program can hold the port, and the
20 NULs are sent by the script, not typed.  `COM5` is the Windows port name;
use the port the host lists (`/dev/ttyACM0` on Linux, a `/dev/cu.usbmodem`
name on macOS).

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

## What entering changes

The binary entry reset runs before the personality starts, then the
personality sets its own defaults:

| Item | State after entry |
|---|---|
| on-board target supplies (`+3V3`, `+5V`) | off |
| on-board pull-up sources | off |
| header signals `MOSI`, `CLK`, `MISO`, `AUX` | released (inputs, latches low) |
| `CS` | becomes the 4 MHz rescue clock output |
| V2 session parameters | back to defaults |
| V2 address pointer | cleared |
| target interface (ISP / PDI / TPI) | disabled until the host enables one |
| VTARGET | read from the `ADC` pin before each command |

The personality never switches a target supply on.  If the target is powered
from the Bus Pirate's own rails, it loses power at entry; use an external
target supply for programming.

## The idle deadline

- The personality ends after about **five seconds** without host traffic.
  The clock starts **when the mode is entered**, so USB enumeration and
  `avrdude` start-up are inside the same window.  Do not enter the mode and
  then look up a command.
- On the USB route only a completed, non-empty request renews the window.
  Empty OUT packets and an open-but-idle handle do not.
- On the serial route any received byte renews it.
- A request restarts the window when it is read and again when its reply is
  sent, so the deadline governs the gaps between requests, not the length of
  a command in progress.

## The USB route while entering

After `0x08` the firmware detaches the CDC device, re-initialises the USB
stack with the mkII descriptors and re-attaches.  The host sees the device
disappear and return as `AVRISP mkII`; the CDC serial port is gone for the
duration.

- The host must enumerate and configure the new interface inside the same
  five-second window.  A slow first-time driver binding can exceed it; the
  firmware then releases the target and restores the CDC device instead of
  waiting forever.  A binding that did not finish is retried by re-entering
  with `0x08`; the serial route avoids the problem for ISP.  The Windows
  procedure is in [host-tools.md](host-tools.md) §Windows USB first use.
- Start `avrdude` as soon as the entry script returns.  It does not wait for
  a device that is not yet enumerated; if it reports that it cannot find
  `03EB:2104`, re-enter with `0x08` and start it again.
- The mkII personality is bus-powered and requests 100 mA.
- A host reset or an interface deconfiguration ends the personality; it does
  not resume after the interface is configured again.  Re-enter with `0x08`.
- The serial route never re-enumerates: the same CDC port changes protocol
  and stays available to the host.  See [serial-route.md](serial-route.md).

## Getting out

The personality has no exit command and no prompt.  It ends when:

1. the idle deadline expires,
2. the USB link stalls, is reset or is deconfigured, or
3. the board is hardware-reset.

On the way out the target is released (PDI/TPI cleanup included), the
binary entry reset runs again (supplies and pull-ups off, signals released)
and **the binary layer prints `BBIO1` on the CDC port**.  On the USB route
the port has to re-enumerate and be configured first; if the host does not
configure it, the firmware detaches and retries rather than waiting forever.
That is the state a host sees after a failed or slow `avrdude` run — the
personality is gone, but the board is not back at the terminal yet.

From `BBIO1`, send byte **`0x0F`**: the board replies `0x01` and, on v4, runs
the terminal reset (pins to inputs, pull-ups released, supplies off, AUX
assignment back, HEX display, BASIC left) and prints the banner and the
`HiZ>` prompt.

| Actor | Bytes | Meaning |
|---|---|---|
| personality | — | the mode ends; the binary layer prints `BBIO1` |
| host | `0F` | return to the terminal |
| personality | `01` | accepted |
| personality | — | the banner and `HiZ>` follow |

## Entering again

After the return to `BBIO1`, send `0x08` or `0x09` again for another session,
or `0x0F` to leave for the terminal.

Within the idle window the personality **can** still serve a second
`avrdude` run without re-entry, provided the USB link stayed configured and
the request is valid.  This is not a general re-run or hot-swap path: a host
reset or a deconfiguration of the interface ends the mode, and no target
should be swapped or rewired while the personality is active.  Once the
deadline has passed, re-enter as above.  The entry byte is never queued for
a later session.

## Related docs

- [protocol.md](protocol.md) — the command set carried over either transport.
- [host-tools.md](host-tools.md) — the identity the host sees and the drivers.
- [isp.md](isp.md) / [pdi.md](pdi.md) / [tpi.md](tpi.md) — the target paths.
- [serial-route.md](serial-route.md) — the CDC transport in detail.
- [entering-and-framing.md](../../protocol/binary/entering-and-framing.md)
  — the shared binary layer (`BBIO1`, result bytes, `0x0F`).
