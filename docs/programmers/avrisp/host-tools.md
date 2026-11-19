# AVRISP-MKII — host identity and tools

Part of the AVRISP-MKII documentation set.  This page covers what a host sees
on the USB link, the `avrdude` identity, the driver or permission step, and
the Atmel Studio compatibility note.  The transport framing is in
[protocol.md](protocol.md) and [serial-route.md](serial-route.md).

## Prerequisites

| Item | Needed for | Check |
|---|---|---|
| `avrdude` with USB (libusb) support | the USB mkII route | `avrdude -c avrispmkII -P usb -p m328p -n` with a target wired and powered |
| `avrdude` (any build) | the serial route | `avrdude -c stk500v2 -P COM5 -p m328p -n` with a target wired and powered; use the host's port name instead of `COM5` |
| Python 3 with pyserial, or PowerShell | sending the entry byte | the snippets in [entry.md](entry.md) §Entering from a host |
| a terminal program | optional terminal use and recovery | only one program can hold the CDC port |
| the OS driver | Windows USB: libusb/WinUSB; Windows serial: the normal CDC driver; Linux: udev permission; macOS: none | see §Drivers and permissions |
| Zadig or a WinUSB/libusb INF | the Windows USB driver binding | see §Drivers and permissions |

Both `avrdude` checks require the personality to be entered first (`0x08` for
USB, `0x09` for the serial route); run the entry snippet before the check.

With a target wired and powered, a successful check prints the device
signature and exits with status 0.  If `03EB:2104` cannot be opened, the
problem is the driver or the `avrdude` build; if it opens but the signature
fails, the problem is the target or its wiring.  An `avrdude` build without
USB support reports `avrdude was compiled without usb support`.

## What the host sees

On the USB route the device presents a single vendor-specific interface that
is shaped like an Atmel AVRISP mkII:

| Descriptor field | Value |
|---|---|
| vendor / product id | `03EB:2104` |
| USB version | 1.10 |
| device class | vendor specific (`0xFF`) |
| device version | 2.00 |
| manufacturer string | `ATMEL` |
| product string | `AVRISP mkII` |
| serial string | `000200012345` followed by an embedded NUL |
| interfaces | 1, vendor specific, no CDC |
| endpoints | bulk OUT `0x02`, bulk IN `0x82`, 64-byte packets |
| power | bus powered, 100 mA requested |
| reported firmware | version 1.24 (parameters `0x91`/`0x92`) |
| protocol identifier | `AVRISP_MK2` (`SIGN_ON`) |

The serial string contains the embedded NUL the real mkII descriptor
carries.  The firmware version is the value hosts expect from an mkII; it
is not the Bus Pirate firmware version.

On the serial route there is no new USB identity: the normal Bus Pirate CDC
device (VID/PID `04d8:fb00` on v4) stays enumerated and answers `SIGN_ON`
with `STK500_2`.  Do not look for a second device in a device list.

## avrdude

| Route | Command |
|---|---|
| USB mkII | `avrdude -c avrispmkII -P usb -p m328p -U flash:w:app.hex:i` |
| USB mkII, signature only | `avrdude -c avrispmkII -P usb -p m328p -n -U signature:r:-:h` |
| serial route | `avrdude -c stk500v2 -P COM5 -p m328p -U flash:w:app.hex:i` |

`COM5` is a Windows example; use `/dev/ttyACM0` on Linux or the
`/dev/cu.usbmodem…` name on macOS.

- `-p m328p`, `x128a3u` and `t10` are examples; use the host tool's own name
  for the part.  PDI and TPI are reached with `-c avrispmkII -P usb` because
  `avrdude` enables XPROG only for mkII-class devices.
- `-B` is a **period in microseconds**, not a frequency: `-B 8` is 8 µs =
  125 kHz, `-B 125` is 125 µs = 8 kHz.  The speed ladder is in
  [isp.md](isp.md) §Speed.
- A `:w:` write is read back and compared unless `-V` is given.  `-V` only
  skips the comparison; it does not change how the data is written.
- The personality implements the mkII command surface.  Whether `avrdude`
  knows a particular part is the host's device database, not this firmware;
  no minimum `avrdude` version is claimed.
- `-O` / `--osccal` performs the AVR053 calibration handshake.  The target
  must already run the calibration firmware; see
  [rescue-clock-osccal.md](rescue-clock-osccal.md) §Worked calibration
  exchange.  Older `avrdude` builds may not have the option.
- The serial route is the ISP route with `avrdude`.  The engine behind it
  also accepts XPROG, so a custom host can drive PDI or TPI over the serial
  framing; that combination is **not verified** with a real target.
- Output varies by `avrdude` release.  The command lines above are the
  standard mkII and STK500v2 forms.

## Drivers and permissions

| Host | USB mkII route | Serial route |
|---|---|---|
| Windows | a libusb/WinUSB-style driver bound to `03EB:2104`; if the first binding resets the device or exceeds the two-minute startup budget, re-enter and retry — see §Windows USB first use | the normal Bus Pirate serial driver; install/allow it on first plug-in, before entering the personality; no libusb driver |
| Linux | a udev rule for `03EB:2104`, or permission to open the device | the normal CDC device permissions |
| macOS | nothing extra — the system USB stack opens it | nothing extra |

A udev rule that grants access to the mkII identity:

```
# /etc/udev/rules.d/99-avrisp-mkii.rules
SUBSYSTEM=="usb", ATTR{idVendor}=="03eb", ATTR{idProduct}=="2104", MODE="0666"
```

Reload the rules (`sudo udevadm control -R`) and re-plug the board.  The
repository's Bus Pirate rules cover the terminal device (`04d8:fb00`, plus
the v3 FTDI `0403:6001`), so they do not apply to `03EB:2104`.

## Windows USB first use

The mkII identity exists only while the personality is active.  It is kept
for about **120 seconds** from attach until the first complete request — long
enough to enumerate, configure and finish a normal binding; empty packets do
not extend it.  A binding that resets the device, or takes longer than two
minutes, still needs another entry.  Use one of these paths:

1. **ISP first: use the serial route.**  Install or allow the normal CDC
   serial driver on first plug-in, before entering the personality; the CDC
   install is not time-limited.  Once bound, the port stays enumerated and no
   libusb binding is involved.  See [serial-route.md](serial-route.md).
2. **Bind the mkII driver with retries.**  Open Zadig with
   *Options → List All Devices* enabled.  Enter with `0x08`, refresh the
   list if the device does not appear, select `AVRISP mkII`
   (`03EB:2104`), choose WinUSB or libusb, and install.  If the device
   disappears before the install finishes, run the entry script again on
   the same USB port and repeat from the selection.  Windows normally keeps
   a completed binding for the same device identity, so later entries are
   faster.  Do not try to continue an install for a device that is no
   longer present.
3. **Pre-stage the driver.**  For repeat deployments, add a WinUSB/libusb
   INF for `03EB:2104` to the Windows driver store before the first entry
   (`pnputil /add-driver` with the INF file).  This removes the interactive
   step; the first enumeration still has to finish inside the 120-second
   budget.
4. **Use a Linux host.**  No driver binding is needed; a udev rule or
   permission to open the device is enough.

After the binding is in place, enter with `0x08`, wait for `03EB:2104` to
appear, then start `avrdude` (it does not wait for a device that is not yet
enumerated).  If `avrdude` reports that it cannot find `03EB:2104`, it started
before the device appeared or after the mode ended: re-enter with `0x08` and
run it again.  The startup budget is not renewable — only the first complete
request switches the mode to the five-second idle — but it is two minutes, so
a normal binding has room.  After a request completes, the session stays alive
while the host keeps sending.

Only one driver can own the interface at a time.  If the device was bound to
an Atmel/Studio driver, a libusb-based `avrdude` may not be able to open it;
bind the driver that matches the host tool you use.  This is a host-driver
choice, not a firmware change.

## Atmel Studio (not a supported host path)

Atmel Studio selects an AVRISP mkII by its USB identity and protocol, so the
personality presents the same VID/PID, strings, interface and `SIGN_ON`
identifier.  The reported firmware version is 1.24.

This port does not provide a supported Atmel Studio procedure: tool
discovery and programming are **not verified** for this port, and whether a
particular Studio release accepts the clone is outside the firmware.  Use
`avrdude` for a supported path.

- The mkII firmware-upgrade command (`0x07`) is **not** implemented; it is
  answered with `0xC9`.  Do not accept a Studio firmware-upgrade prompt:
  the step cannot complete.
- On Windows, the Atmel driver and a libusb/WinUSB driver compete for the
  same interface.  Pick the one that matches the tool you are about to use.

## Checking the identity

While the personality is active on the USB route:

- Linux: `lsusb` lists `03eb:2104`; the CDC serial device is absent.
- Windows: the device list shows `AVRISP mkII` (or the bound libusb device);
  the Bus Pirate COM port is absent until the mode ends.
- macOS: the device appears in the USB tree; no COM port is created by the
  mkII personality.

After the mode ends, the vendor device disappears and the Bus Pirate CDC
port returns.  If you see the CDC port and no `03EB:2104`, the personality
is not active — it timed out, was reset, or is not in the build.

## Host-side failure map

| Symptom | Likely cause | Action |
|---|---|---|
| `03EB:2104` never appears | entry byte not sent, mode already ended, or personality not in the build | enter again and start the host tool once the device appears; check the build ([entry.md](entry.md) §Build availability) |
| device appears but `avrdude` cannot open it | no driver, wrong driver, or missing udev permission | bind WinUSB/libusb (on Windows, re-enter and retry if the binding resets the device or exceeds the two-minute startup budget; see §Windows USB first use) or add the udev rule, then re-enter |
| `avrdude -c avrispmkII -P usb` cannot find the tool | the device is not in the mkII personality (for example still CDC), or another driver owns it | re-enter with `0x08`; check the driver |
| `avrdude` reports no device right after entry | it started before the device appeared, or the mode ended (host reset/deconfiguration, or the startup budget expired) | re-enter with `0x08` and run it again; see §Windows USB first use |
| `avrdude -c stk500v2 -P COM5` cannot open the port | the terminal or another program still holds the port | close it and retry |
| Atmel Studio offers a firmware upgrade | the upgrade command is not implemented | do not accept it; use `avrdude` |
| host sees the device only briefly | the host reset/deconfigured the interface, or the binding exceeded the 120-second budget | re-enter and repeat the binding; for ISP first use, use the serial route.  See §Windows USB first use |

## Related docs

- [entry.md](entry.md) — entry bytes and the startup and idle deadlines.
- [protocol.md](protocol.md) — USB transport and `SIGN_ON`.
- [serial-route.md](serial-route.md) — the CDC route.
- [isp.md](isp.md) — `avrdude` flows and the speed ladder.
- [edge-cases.md](edge-cases.md) — more failure and recovery entries.
