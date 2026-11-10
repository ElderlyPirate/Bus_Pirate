# Flashing the firmware

**Status:** maintainer-facing.  Derived from `Firmware/core_system/proc_menu.c` (the `$`
entry), the `Bootloaders/pirate-loader/` sources, the two bootloader sources
and the device descriptors.  No flash was run for this page.

This page covers getting a built image onto a real Bus Pirate v3 or v4:
entering the bootloader, finding the right port, running `pirate-loader`, and
checking the result.  How to build the image is [building.md](building.md); the
bootloaders themselves are [bootloaders.md](bootloaders.md).

## Scope

- **In:** bootloader entry on v3 and v4; port identification; building and
  running `pirate-loader` and its options; the Pickit2 full-chip route; the
  post-flash check; drivers and port permissions.
- **Out:** the bootloader's own update story ([bootloaders.md](bootloaders.md))
  and the terminal prompt ([docs/generic/](../generic/README.md)).

## The flashing model

Both boards run a small resident bootloader that receives an Intel HEX image
over the serial link and writes the application flash.  The application
firmware can hand control to the bootloader on request (the `$` command), and
the board also enters the bootloader by hardware means.  `pirate-loader` is
the host program that talks to that bootloader.

| | v3 | v4 |
|---|---|---|
| bootloader | ds30 loader (`Bootloaders/BPv3-bootloader/firmware-v4.5/`) | USB bootloader (`Bootloaders/BPv4-bootloader/firmware-v1/`) |
| serial link | FTDI cable, `0403:6001` | USB CDC, firmware `04d8:fb00`; **bootloader** `04d8:FAFF` |
| `$` request entry | yes, with the v4.5 loader | yes, current loader |
| jumper entry | PGC/PGD jumper + power-cycle | NORMAL button (or the PGC/PGD jumper as a fallback) |
| post-flash restart | bootloader timeout returns to the app | loader asks the bootloader to restart |

## Entering the bootloader

### Firmware `$` command

At the normal prompt (`HiZ>`), type `$` and confirm the `y` prompt.  The
firmware prints `BOOTLOADER`, parks the board, and resets.  On the way in it
sets a persistent flag (`skip_pgc_pgd_check`) so the bootloader skips its own
entry test and stays in the bootloader.

- **v4:** the flag lives at `0x47FA`; the USB bootloader reads it in `main()`.
- **v3:** the flag lives at `0x27FA`; the ds30 loader reads it at reset.
  This flag exists only in the current v4.5 loader — older v3 loaders (`old/`)
  do not have it, so `$` cannot work there (use the jumper).

`$` is also the way out for a tool that holds the port: a SUMP session that
receives the `$` byte **leaves the analyzer and returns to the terminal in
HiZ** (the same exit as `0x0F` and `#`).  It does **not** enter the bootloader
from inside SUMP.  To continue into the bootloader, let the prompt come back
and type `$` there.

### Hardware entry

- **v3 (PGC/PGD jumper):** with the board unpowered, connect the **PGC and PGD
  pins** with a jumper, then apply power.  The ds30 loader detects the jumper
  at reset and stays in the bootloader.  Remove the jumper after flashing.
- **v4 (NORMAL button):** holding the **NORMAL button** while the board powers
  up enters the bootloader.  The v4 loader also honours the PGC/PGD jumper as
  a fallback.  During the entry window the MODE and USB LEDs light and the
  target supply is enabled; when the bootloader takes over the LEDs go out.

Either way, **the terminal application must be closed before flashing** — the
bootloader and `pirate-loader` cannot share the port.  With `screen`, for
example, detach with `Ctrl+a Ctrl+k`.

## Building pirate-loader

`pirate-loader` is a host tool (it does not run on the Bus Pirate).  Its
source is `Bootloaders/pirate-loader/pirate-loader.c` with a CMake build; it
needs CMake 3.5 or later.

Unix-like systems:

```
cd Bootloaders/pirate-loader
mkdir build
cd build
cmake ..
make
```

Windows: run the same `cmake ..` in the build directory, then open the
generated `pirate-loader.sln` in Visual Studio and build.

Prebuilt binaries from an older bootloader-update bundle are also present under
`Bootloaders/BPv3-bootloader/v4xtov43-bootloader-update/utilities/`
(`pirate-loader.exe`, `pirate-loader_lnx`, `pirate-loader_mac`).  Building from
`Bootloaders/pirate-loader/` is the current source of truth; the prebuilt
Windows binary's port speed was not verified.

- There is no `package/` directory in this repository; older instructions that
  pointed at `package/BPv3-firmware` are stale.

## Using pirate-loader

```
pirate-loader --dev=<port> --hex=<image.hex> [ --verbose ]
pirate-loader --dev=<port> --hello
pirate-loader --simulate --hex=<image.hex> [ --verbose ]
```

| Option | Effect |
|---|---|
| `--dev=<path>` | serial port: `COM<n>` on Windows, `/dev/ttyUSB*` / `/dev/ttyACM*` / `/dev/cu.*` on Unix.  For v4 in bootloader mode this is the **bootloader's** port, not the firmware's |
| `--hex=<file>` | the Intel HEX image to write (usually `Firmware/bpv4.hex` or `bpv3.hex`) |
| `--verbose` | dump each command frame and `Skipping page …` lines |
| `--hello` | handshake only — verify the bootloader answers; no file needed |
| `--simulate` | parse and walk the HEX without opening a device |
| `--help` | usage text |

The loader sets the port speed itself, so a terminal configured differently
does not matter.  The speed is **not** the same on all hosts: on Unix the port
is opened at **115200 8N1** (the termios `B115200` constant), but the Windows
build defines `B115200` as **921600**, so `configurePort()` programs
`SetCommState` with 921600 8N1.  The v4 CDC link ignores the host-side speed;
the v3 link runs through an FTDI cable into a loader whose UART divider is a
build constant, so the Windows speed matters there — see
[the v3 speed trap](#v3-on-windows-the-speed-mismatch) below.

### Find the port after entry

| | v3 | v4 |
|---|---|---|
| port across entry | the FTDI cable keeps its port | firmware and bootloader are different USB devices, so the OS enumerates the bootloader as a **new port** |
| Windows | same `COM<n>` | check Device Manager → Ports (COM & LPT), or `mode`, for the new `COM<n>` |
| Linux | same `/dev/ttyUSB*` / `/dev/ttyACM*` | the ACM node can renumber (`ttyACM0` → `ttyACM1`); check `dmesg` or `/dev/serial/by-id/` |

On v4, the `--dev` that worked for the terminal is usually wrong for the
flash.  Point `--dev` at the bootloader port; `--hello` is a safe way to test a
candidate port before writing anything.

### A normal run

```
pirate-loader --dev=COM10 --hex=Firmware/bpv4.hex
```

Expected milestones: `Sending Hello to the Bootloader...OK`, `Bootloader
version: 4,14`, `Device ID [0x…]: PIC24FJ256GB106`, per-page
`Erasing page … OK` / `Writing page … OK`, then
`Firmware updated successfully! :)`.

- The hello reply is the bootloader's device byte, its version, and then the
  status byte `'K'`; the loader then checks the device table (from
  `PIC24FJ16GA002` up to `PIC24FJ256DA210`), which fixes the flash size and the
  v3 jump fix.

From the source:

- The loader accepts the normal bootloader `'K'` status and the Sandbox
  Electric BPv3.b clone's `0x13`.
- On v3-sized flash the loader fixes the bootloader/user jump addresses before
  writing.  On v4 it also sends a restart request after the flash.
- An `Error updating firmware :(` reported late in the write does not always
  mean the image failed; power-cycle the board and check the version (below)
  before retrying.  Maintainer provenance for this note is in
  [verification.md](verification.md).
- A port that is "in use, or permission denied" almost always means a terminal
  or another loader still holds it.

### v3 on Windows: the speed mismatch

The v4.5 ds30 loader fixes its UART divider at build time (about 115200 8N1
from a 16 MHz instruction clock) and has no autobaud.  The Windows build of
`pirate-loader` opens the port at 921600, so the handshake fails — the typical
symptom is `No reply from the bootloader, or invalid reply received` or an
`unexpected status` byte instead of `OK`.

Workarounds:

- run `pirate-loader` on a Unix host (it opens 115200 there), or
- rebuild `pirate-loader` for Windows with the `#define B115200 921600` line
  changed to `115200`.

This only affects v3; the v4 CDC link ignores the host-side line speed.

### Post-flash restart

- **v4:** `pirate-loader` sends the bootloader's `0xff` restart request; the
  bootloader acknowledges and resets itself.  If the device does not
  re-appear as a serial port, power-cycle it (after removing any jumper).
- **v3:** `pirate-loader` does not send a restart command.  The ds30 loader
  returns to the application when its host wait ends; if the board does not
  come back, remove the jumper and power-cycle it.

## The Pickit2 route (full-chip programming)

`pirate-loader` writes only the application flash and needs a working
bootloader.  To program a blank chip, restore a bricked bootloader, or write a
bootloader for the first time, use a hardware programmer such as a Pickit2
with `pk2cmd`:

```
# program one image (programmer powers the target); confirm the switch shape
# with pk2cmd -?
pk2cmd -P -M -F <image.hex>
```

- The exact `pk2cmd` switch set is **not verified** here (no programmer was
  used).  Erase the device before writing a **bootloader** or recovering a
  bricked board; `pk2cmd -?` lists the erase option.
- The Pickit2 powers the target, so the Bus Pirate must be unplugged, and the
  programming header is keyed (triangle to triangle; one pin may be unused).
- The bootloader images come from the [bootloader sources](bootloaders.md).

## Verify the flash

1. Power-cycle the board (remove any bootloader jumper first).
2. Open the port at **115200 8N1** and press Enter; the prompt appears
   (default `HiZ>`).  On v4 this is the firmware's port again, which may be a
   different number from the bootloader's.
3. Type `i`: the banner shows the hardware version, firmware version and
   build hash, the bootloader version, and the device ID/rev.

A stale or mismatched image shows the wrong firmware version in step 3.  No
separate checksum tool is shipped for the application image.

## Drivers and permissions

- **v4** enumerates as a USB CDC ACM device (firmware `04d8:fb00`, bootloader
  `04d8:FAFF`); **v3** goes through its FTDI cable (`0403:6001`).
- The project ships a Linux udev rule for the Bus Pirate ports:
  `contrib/98-buspirate.rules` (moved there by the documentation migration).
  It matches the
  v3 FTDI PID (`0403:6001`) and the v4 firmware PID (`04d8:fb00`) and creates
  `/dev/buspirate`, so the tools need not run as root.  It does **not** match
  the v4 bootloader PID (`04d8:FAFF`), so no `/dev/buspirate` appears while
  the board is in bootloader mode; point `pirate-loader` at the raw `ttyACM*`
  node instead.  The rule is a documentation asset, not part of the firmware
  build.
- **Unverified / out of scope:** per-operating-system driver installation
  steps are not documented in this repository.  On Windows the v3 FTDI link
  may need the FTDI virtual-COM-port driver; the v4 CDC link is normally served
  by the in-box USB serial driver.  Confirm against the current OS.

## Verification

- Not run for this page; see [verification.md](verification.md) for the
  maintainer record.

## Limits & out of scope

- No bootloader *update* instructions here — see
  [bootloaders.md](bootloaders.md).
- Only `pirate-loader` and `pk2cmd` are covered; other programmers are out of
  scope.
