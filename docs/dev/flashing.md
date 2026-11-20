# Flashing the firmware

**Status:** maintainer-facing.  Derived from the firmware `$` entry, the
`Bootloaders/pirate-loader/` host source, the `Bootloaders/v3/` and
`Bootloaders/v4/` sources, and the device descriptors.

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
| bootloader | ds30 loader (`Bootloaders/v3/`) | USB bootloader (`Bootloaders/v4/`) |
| serial link | FTDI cable, `0403:6001` | USB CDC, firmware `04D8:FB00`; **bootloader** `04D8:FAFF` |
| `$` request entry | yes, with the 5.0 loader | yes, with the 5.0 loader |
| jumper entry | PGC/PGD jumper + power-cycle | NORMAL button (or the PGC/PGD jumper as a fallback) |
| post-flash restart | no restart command; the loader stays in its command loop after the host wait — power-cycle | loader sends `0xff`; the bootloader resets the board |
| host line speed | 115200 8N1 | host metadata only; USB CDC ignores it |

The two bootloaders are independent of the application revision: the current
application is v9.0 on both boards, and the current bootloader is 5.0 on both.
See [bootloaders.md](bootloaders.md) §Versions and the name map in
[README.md](README.md).

## Entering the bootloader

### Firmware `$` command

At the normal prompt (`HiZ>`), type `$`.  The firmware prints
`Are you sure? `, reads exactly one byte, echoes it, and accepts only `y` or
`Y`; any other byte cancels and the prompt returns.  A `y` prints
`BOOTLOADER`, waits 100 ms, runs the board reset, and resets the CPU.  On the
way in it sets a persistent flag so the bootloader skips its own entry test and
stays in the bootloader.

- **v4:** the flag lives at RAM `0x47FA`; the USB bootloader reads it in
  `main()`.  The application signature is at RAM `0x47FC`.
- **v3:** the flag lives at RAM `0x27FA`; the ds30 loader reads it at reset.
  The application signature is at RAM `0x27FC`.  The flag exists in the current
  **5.0** loader; older v3 4.1/4.3 loaders do not have it, so `$` cannot work
  there (use the jumper).

`$` is also the way out for a tool that holds the port: a SUMP session that
receives the `$` byte **leaves the analyzer and returns to the terminal in
HiZ** (the same exit as `0x0F` and `#`).  It does **not** enter the bootloader
from inside SUMP.  To continue into the bootloader, let the prompt come back
and type `$` there.

### Hardware entry

- **v3 (PGC/PGD jumper):** with the board unpowered, connect the **PGC and PGD
  pins** with a jumper, then apply power.  The ds30 loader drives PGD low, holds
  PGC as an input with its internal pull-up, and samples PGC; the jumper pulls
  PGC low and selects the bootloader.  **No external pull-up is required** —
  the loader enables the internal one.  Remove the jumper after flashing.
- **v4 (NORMAL button):** holding the **NORMAL button** while the board powers
  up enters the bootloader.  The v4 loader also honours the PGC/PGD jumper as
  a fallback.  **No external pull-up is required** for either entry: the loader
  configures the button/jumper inputs with its internal pull-ups, then releases
  them when it hands control to the application.
- **Blank v4 application:** the loader checks the low 16 bits and high byte of
  the first application instruction at `0x2000`.  An erased instruction keeps
  the board in the bootloader; a corrupt non-erased instruction still passes
  that presence check and can be jumped into.  Use the NORMAL button or the
  PGC/PGD jumper on a blank or suspect board.

Either way, **the terminal application must be closed before flashing** — the
bootloader and `pirate-loader` cannot share the port.  With `screen`, for
example, detach with `Ctrl+a d`.

**Power:** the Bus Pirate is powered normally over its own USB link.  Do not
power the board from an external supply and the USB cable at the same time,
and never drive the header rails from an external source while the on-board
supply is on.  The bootloader itself keeps the target supply enable (`RB9`)
off; it is not an activity LED.

## Building pirate-loader

`pirate-loader` is a host tool (it does not run on the Bus Pirate).  Its
source is `Bootloaders/pirate-loader/pirate-loader.c` with a CMake build; it
needs CMake 3.15 or later.

Unix-like systems:

```
cd Bootloaders/pirate-loader
mkdir build
cd build
cmake ..
cmake --build .
```

Windows: from the same `build` directory, use the MinGW Makefiles generator
(`cmake -G "MinGW Makefiles" ..`, then `cmake --build .`), or run the Visual
Studio generator and open the generated `pirate-loader.sln` to build.  CMake
selects compiler-appropriate static flags: `-static -static-libgcc` for GCC,
`-static` for Clang, and `/MT` for MSVC.  The loader is built with warnings as
errors (`/W4 /WX` for MSVC, `-Wall -Wextra -Werror` otherwise).

No prebuilt `pirate-loader` binaries are shipped in the current tree; build it
from source.  The old bootloader-update bundle, its GUI and its prebuilt
`utilities/` binaries are removed.  There is also no `package/` directory in
this repository; older instructions that pointed at `package/BPv3-firmware`
are stale.

## Using pirate-loader

```
pirate-loader --dev=<port> --hex=<image.hex> [ --verbose ]
pirate-loader --dev=<port> --hello
pirate-loader --simulate --hex=<image.hex> [ --verbose ]
```

| Option | Effect |
|---|---|
| `--dev=<path>` | serial port: `COM<n>` on Windows, `/dev/ttyUSB*` / `/dev/ttyACM*` / `/dev/cu.*` on Unix.  For v4 in bootloader mode this is the **bootloader's** port, not the firmware's |
| `--hex=<file>` | the Intel HEX image to write (usually `Firmware/bpv4.hex` or `Firmware/bpv3.hex`) |
| `--verbose` | dump each command frame and `Skipping page …` lines |
| `--hello` | handshake only — verify the bootloader answers; no file needed.  On v4 it still sends the `0xff` restart after the handshake, so the board leaves the bootloader |
| `--simulate` | parse and walk the HEX without opening a device |
| `--help` | usage text |

The loader sets the port speed itself, so a terminal configured differently
does not matter.  `pirate-loader` 1.0.7 opens **115200 8N1** on both Windows
and POSIX; the older Windows build incorrectly programmed 921600 8N1, and that
bug is fixed.  The v4 CDC link ignores the host-side line speed; the v3 link
runs through an FTDI cable into a loader whose UART divider is a build
constant, so the v3 speed matters there — see [v3 serial
speed](#v3-serial-speed) below.

### Find the port after entry

| | v3 | v4 |
|---|---|---|
| port across entry | the FTDI cable keeps its port | firmware and bootloader are different USB devices, so the OS enumerates the bootloader as a **new port** |
| Windows | same `COM<n>` | check Device Manager → Ports (COM & LPT), or `mode`, for the new `COM<n>` |
| Linux | same `/dev/ttyUSB*` / `/dev/ttyACM*` | the ACM node can renumber (`ttyACM0` → `ttyACM1`); check `dmesg` or `/dev/serial/by-id/` |

On v4, the `--dev` that worked for the terminal is usually wrong for the
flash.  Point `--dev` at the bootloader port; `--hello` is a safe way to test a
candidate port before writing anything.  On v4 `--hello` still sends the
`0xff` restart after the handshake, so re-enter the bootloader if you intend
to flash next.

The host frontend can list candidates without opening them:

```
$ bp ports
```

| Port | VID:PID | Description | Marker |
|---|---|---|---|
| COM1 | `----:----` | Communications Port (COM1) | — |
| COM6 | `04D8:FB00` | USB Serial Device (COM6) | `[Bus Pirate v4 application]` |
| COM10 | `04D8:FAFF` | USB Serial Device (COM10) | `[Bus Pirate v4 bootloader]` |

`bp ports` only lists; it does not select or open a port.  See
[tooling.md](tooling.md) §Host frontend.

### A normal run

```
pirate-loader --dev=COM10 --hex=Firmware/bpv4.hex
```

Expected milestones:

| Step | v4 | v3 |
|---|---|---|
| hello | `Sending Hello to the Bootloader...OK` | same |
| hello reply field | `Bootloader version: 5,00` (loader 5.0) | `Bootloader version: 1,02` (ds30 hello protocol 1.0.2) |
| device | `Device ID [0xf1]:PIC24FJ256GB106` | `Device ID [0xd4]:PIC24FJ64GA002` |
| pages | `Erasing page … OK` / `Writing page … OK` | same |
| final | `Firmware updated successfully! :)` | same |
| restart | `Asked the bootloader to restart.` | no restart line; power-cycle |

The hello reply is the bootloader's device byte, its protocol/version bytes,
and then the status byte `'K'`; the loader then checks the device table, which
selects the flash size and, for a v3-sized part, fixes the bootloader/user jump
addresses before writing.  A v3 clone that answers device ID `0x02` is
accepted as `PIC24FJ64GA004 (BPv3 clone)`; the loader uses the same v3 flash
size and jump fixing.  The clone hello status `0x13` is also accepted.

From the source:

- The loader accepts the normal bootloader `'K'` status and treats the
  protection status `'P'` as `(SKIPPED by bootloader)`; a checksum error `'N'`
  or any other byte fails the command.  Both loaders also return `'U'` for an
  invalid command or length and `'V'` for a programming/readback failure.
- On v4 it also sends a restart request (`0xff`) after the flash; on v3 it
  does not.  The restart request is fire-and-forget: some loaders acknowledge
  it, some act silently, so the host prints `Asked the bootloader to restart.`
  rather than waiting for a reply.
- `Error updating firmware :(` means `sendFirmware()` returned failure, so the
  image may be partially written.  Power-cycle and check the version (below)
  before retrying.
- A port that is "in use, or permission denied" almost always means a terminal
  or another loader still holds it.

The loader also prints the parsed size and one line per page/row.  The v4
default image from [building.md](building.md) parses as:

```
Parsing HEX file [Firmware/bpv4.hex]
Found 30335 words (91005 bytes)
Erasing page 0, 0000...OK
Writing page 0 row 0, 0000...OK
...
Writing page 58 row 471, eb80...OK
Firmware updated successfully! :)
Asked the bootloader to restart.
```

The exact page/row count depends on the image and feature set.  Protected
configuration rows are reported as `(SKIPPED by bootloader)` and do not fail
the flash.

### v3 serial speed

The v3 5.0 ds30 loader fixes its UART divider at build time (115200 8N1 from a
16 MHz instruction clock) and has no autobaud.  `pirate-loader` 1.0.7 opens
115200 8N1 on both Windows and POSIX, so the handshake works on both.  The old
Windows build opened 921600 and produced `No reply from the bootloader, or
invalid reply received` or an `unexpected status` byte; rebuild with 1.0.7 if
you still have that binary.  This only affects v3; the v4 CDC link ignores the
host-side line speed.

### Post-flash restart

| Entry route | v3 | v4 |
|---|---|---|
| `$` | no restart command; the loader stays in its command loop after the host wait — power-cycle | `0xff` restart; the board resets and re-enumerates as the application |
| PGC/PGD jumper | remove the jumper, then power-cycle | remove the jumper if fitted; release NORMAL; `0xff` restarts the board |
| NORMAL button | n/a | release the button; `0xff` restarts the board |

- **v4:** if the device does not re-appear as a serial port, power-cycle it
  (after removing any jumper and releasing the button).
- **v3:** if the board does not come back after a power-cycle, remove the
  jumper and check the image again.

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

- The exact `pk2cmd` switch set is **unverified** here.  Erase the device
  before writing a **bootloader** or recovering a bricked board; `pk2cmd -?`
  lists the erase option.
- The Pickit2 powers the target, so the Bus Pirate must be unplugged, and the
  programming header is keyed (triangle to triangle; one pin may be unused).
  Do not power the board from USB while the programmer supplies it.
- The bootloader images come from the [bootloader sources](bootloaders.md).

## Verify the flash

1. Power-cycle the board (remove any bootloader jumper first).
2. Open the port at **115200 8N1** and press Enter; the prompt appears
   (default `HiZ>`).  On v4 this is the firmware's port again, which may be a
   different number from the bootloader's.
3. Type `i`: the banner shows the hardware version, firmware version and
   build hash, the mode list, and the device ID/rev.  On v3 it also prints the
   stored bootloader version (`Bootloader v5.0`); v4 does not print a
   bootloader version in `i`.

A stale or mismatched image shows the wrong firmware version in step 3.  No
separate checksum tool is shipped for the application image.

## Drivers and permissions

- **v4** enumerates as a USB CDC ACM device (firmware `04D8:FB00`, bootloader
  `04D8:FAFF`); **v3** goes through its FTDI cable (`0403:6001`).
- The project ships a Linux udev rule for the Bus Pirate ports:
  `contrib/98-buspirate.rules`.  It matches the v3 FTDI PID (`0403:6001`) and
  the v4 firmware PID (`04D8:FB00`) and creates `/dev/buspirate`, so the tools
  need not run as root.  It does **not** match the v4 bootloader PID
  (`04D8:FAFF`), so no `/dev/buspirate` appears while the board is in
  bootloader mode; point `pirate-loader` at the raw `ttyACM*` node instead.
  The rule is a repository asset, not part of the firmware build.
- **Unverified / out of scope:** per-operating-system driver installation
  steps are not documented in this repository.  On Windows the v3 FTDI link
  may need the FTDI virtual-COM-port driver; the v4 CDC link is normally served
  by the in-box USB serial driver.  Confirm against the current OS.

## Edge cases and failure modes

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `$` answered with anything but `y`/`Y` | prompt returns, no reset | the byte is echoed; run `$` again with `y` |
| v3 board has an older 4.1/4.3 loader | `$` resets back to the application | the old loader does not understand the flag; use the PGC/PGD jumper |
| v4 bootloader entered from the terminal | the terminal port disappears | the bootloader is a different USB device; find the new port and re-enter after any `--hello` |
| terminal or another loader holds the port | `in use, or permission denied` | close the terminal; detach `screen` with `Ctrl+a d` |
| v3 flash at 921600 | `No reply …` or an unexpected status | rebuild/use `pirate-loader` 1.0.7, which opens 115200 |
| v4 application blank or corrupt | board stays in the bootloader | use NORMAL or the PGC/PGD jumper; write a valid image |
| v4 packet interrupted | loader waits, then requires a new hello after about 1 s | re-enter the bootloader and start the flash again |
| row reaches the running v3 loader | loader returns `P` | the last safe row starts at `0xA780`; use a smaller image |
| v4 `--hello` used immediately before flashing | board leaves the bootloader | re-enter the bootloader, then run `--hex` without `--hello` |
| flash reports `Error updating firmware :(` | partial write possible | power-cycle, check `i`, then retry the whole flash |

## Verification

The page is derived from the sources named above; the per-set basis is in
[verification.md](verification.md).  The `pk2cmd` switch set remains
**unverified**.

## Limits & out of scope

- No bootloader *update* instructions here — see
  [bootloaders.md](bootloaders.md).
- Only `pirate-loader` and `pk2cmd` are covered; other programmers are out of
  scope.
- v3 hardware behaviour is source-derived; v3 is not verified on hardware.

## Credits

The original bootloader-entry correction — quit the terminal before starting
`pirate-loader`, and the `$`/bootloader-version caveat — came from Tom
Jennings (`tomwj`, commit `e6eaaf29` on his fork).  The rewritten flashing and
bootloader pages retain that correction.
