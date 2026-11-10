# Bootloaders

**Status:** maintainer-facing.  Derived from `Bootloaders/BPv3-bootloader/firmware-v4.5/`
(ds30 loader assembly), `Bootloaders/BPv4-bootloader/firmware-v1/` (the v4 C
bootloader) and `Bootloaders/pirate-loader/`.  No bootloader write was run for
this page.

This page describes the resident bootloaders: what the folders contain, how
each decides to enter the bootloader, the host protocol, and when you must
update the bootloader rather than just flash firmware.  Flashing firmware is
[flashing.md](flashing.md).

## Scope

- **In:** the bootloader folder layout, the v3 and v4 entry rules, the
  bootloader host protocol as `pirate-loader` uses it, and the
  update-vs-flash decision including the route to the current v3 loader.
- **Out:** the application firmware build ([building.md](building.md)) and the
  normal firmware flash procedure ([flashing.md](flashing.md)).

## Folder map

| Path | Contents |
|---|---|
| `Bootloaders/pirate-loader/` | host-side loader source (`pirate-loader.c`), CMake build, `test.sh` / `test.bat` handshake smoke scripts |
| `Bootloaders/BPv3-bootloader/firmware-v4.5/` | **current** v3 bootloader: ds30 loader for the PIC24FJ64GA002, MPLAB-X project `ds30loader.X` |
| `Bootloaders/BPv3-bootloader/old/` | superseded v3 loaders (`firmware-v4.1`, `firmware-v4.3`) and older v2→v4 updaters (`upgrader-v2tov4/`, `v2tov4-bootloader-update/`) |
| `Bootloaders/BPv3-bootloader/v4xtov43-bootloader-update/` | Windows bootloader-update bundle: the 4.3-era upgrader image, the 4.5 **application** image, the ds30 GUI and prebuilt `pirate-loader` binaries (see §v3 update route) |
| `Bootloaders/BPv3-bootloader/upgrader-v4tov4/` | bootloader **v4.x → v4.x** upgrader for the BPv3 (legacy MPLAB project); erases the loader page and writes a replacement page — see §v3 update route |
| `Bootloaders/BPv3-bootloader/ds30_Loader_GUI/` | the ds30 loader Windows GUI used to flash v3 application images |
| `Bootloaders/BPv4-bootloader/firmware-v1/` | **current** v4 bootloader: USB CDC bootloader, MPLAB-X project `bpv4-bootloader.X` |

## v3 bootloader (ds30 loader)

Source: `Bootloaders/BPv3-bootloader/firmware-v4.5/ds30loader.s` plus
`settings.inc`, `devices.inc` and the device linker script.  It builds with
MPLAB-X (project `ds30loader.X`) and reports bootloader version **4.5**
(`BLVERSION = 0x0405`).  It occupies one program page at `0xA800` (the last
page of the 64 KB part) followed by the config words.

At reset the loader decides between staying in the bootloader and jumping to
the application:

1. If the application has run at least once it leaves its signature in flash.
   The loader compares `firmware_signature` against `0x31415926`.
2. If the signature matches **and** the `skip_pgc_pgd_check` flag (a byte at
   `0x27FA`, shared with the firmware's `$` command) is set, it goes straight
   to the bootloader and clears the flag.
3. Otherwise it runs the jumper test: drive **PGD** low, hold **PGC** as an
   input with a pull-up, and sample it.  A **PGC/PGD jumper** pulls PGC low
   and selects the bootloader; with no jumper PGC reads high and the loader
   selects the application.
4. In the bootloader it waits for host commands at 115200 8N1.  The receive
   wait is bounded (`BLTIME = 500 ms` per byte); if the host does not keep it
   busy the loader can return to the application, so start the host loader as
   soon as the board is in the bootloader.  A firmware `$` jump is recorded
   in the loader's state, so a timeout after `$` returns to the application
   rather than re-running the jumper test.

Consequences:

- The `$` command only works on this loader (and later): the older
  `old/firmware-v4.1` / `old/firmware-v4.3` sources have no
  `skip_pgc_pgd_check`, so `$` cannot hand off there.
- The jumper method always works, including with no application present.
- The running loader refuses to write its own page: any write at or past the
  last row before the loader is rejected with `'P'`.  Replacing the loader
  therefore needs the upgrader trampoline or a hardware programmer (§v3
  update route).

## v4 bootloader (USB)

Source: `Bootloaders/BPv4-bootloader/firmware-v1/` (`main.c`, `bootloader.c`,
`bootloader.h`, the USB stack and descriptors).  It builds with MPLAB-X
(project `bpv4-bootloader.X`) and reports bootloader version **4.14**
(`VER_H = 0x04`, `VER_L = 0x0e`).

At reset `main()` makes **one** decision: the entry block runs when the
signature is missing **or** the flag is clear **or** the NORMAL button is
held.

1. Entry block (signature mismatch `||` `skip_pgc_pgd_check` clear `||`
   button down): the loader lights the MODE and USB LEDs, enables the target
   supply for the entry window, and samples PGD.  If PGD reads high, the
   button is released **and** the application's reset vector is present (not
   blank), it jumps to the application.  Anything else stays in the
   bootloader: a low PGD (the PGC/PGD jumper), a held button, or an erased
   application.
2. No entry block (valid signature **and** flag set **and** button released):
   it goes straight to the bootloader.  This is the `$` path.
3. The `skip_pgc_pgd_check` flag at `0x47FA` is cleared in both paths.
4. It enumerates as a USB device (`04d8:FAFF` in bootloader mode) and waits
   for the host.

A missing signature alone therefore does **not** keep the board in the
bootloader: a blank-signature board with a valid application and PGD high jumps
to that application.

The host protocol (`bootloader.c`):

| Step | Bytes |
|---|---|
| Hello | host sends `0xC1`; bootloader answers `[DEVICEID][VER_H][VER_L]`, then the first status byte (`'K'`) |
| Command frame | `[addrU][addrH][addrL][cmd][len][payload…][checksum]`; bytes must sum to zero |
| `cmd = 0x01` | enable erase for the next write |
| `cmd = 0x02` | erase-if-enabled then write one row, verify, answer `'K'` |
| `cmd = 0xff` | answer `'K'`, disable USB and `RESET` |

Status bytes: the bootloader sends one status byte **before** it reads each
command (the result of the previous operation), not only after a write.

| Byte | Meaning |
|---|---|
| `'K'` | ok |
| `'V'` | verify failed |
| `'P'` | write would touch a protected region |
| `'N'` | checksum error |
| `'U'` | unknown command |

A verbose flash that stalls is usually waiting on one of these bytes; `'N'` and
`'U'` mean the host framing is wrong, `'P'` means the address range is
protected.

The loader protects the bootloader region (`PROT_BL`), the config page
(`PROT_CONFIG`) and the reset-vector jump (`PROT_GOTO`), so an application
write cannot overwrite the bootloader.  This last point is why the current v4
loader is described as the "4.14 erase-guard" loader.

## When to update the bootloader

Firmware flashes are routine; bootloader writes are rare.  Update the
bootloader only when:

- the board has an **old loader without `$` support** (`old/firmware-v4.1`,
  `old/firmware-v4.3`) and you want the software entry to work;
- a bootloader defect fix is specifically required (for example the v4
  erase-guard / LED behavior in the current v4.14 source);
- the bootloader region is **blank or corrupted** (a blank chip, or a failed
  bootloader write) and a programmer must rewrite it.

Otherwise, flashing a new application image with `pirate-loader` is all that
is needed — it never rewrites the bootloader region.

### v3 update route — to the current 4.5 loader

The 4.5 loader is the one that adds `$`.  It is built, not downloaded:

1. Build `Bootloaders/BPv3-bootloader/firmware-v4.5/` with MPLAB-X/XC16
   (project `ds30loader.X`).  The product is the loader page plus config words;
   `settings.inc` already fixes 115200 8N1 and `BLTIME`.
2. Write it by one of the two routes in the table below.
3. Re-flash the application afterwards: the upgrader rewrites the reset vector
   at `0x0000` to jump into the loader, and the programmer route erases the
   chip, so either way the board boots into the loader until a new application
   is written with the ds30 GUI (configured for PIC24FJ64GA002 at 115200).

| Route | How | Limit |
|---|---|---|
| Hardware programmer | erase the chip, write the loader, then write the application ([Pickit2 route](flashing.md#the-pickit2-route-full-chip-programming)) | always works, including blank/bricked chips |
| Upgrader trampoline | rebuild `upgrader-v4tov4/` with its `firmware.h` array replaced by the built 4.5 page, flash that image with `pirate-loader`, then re-flash the application | needs the HEX→array conversion (`convert.html` in the same folder) and a working current loader |

What the update **bundle** installs: `v4xtov43-bootloader-update/` carries
`BPv3-bootloader-upgrade-v4xtov4.3.hex` (the upgrader as compiled for the
older release), `BPv3-Firmware-v4.5.hex` (the 4.5 **application** image), the
ds30 GUI and prebuilt `pirate-loader` binaries.  The as-checked-in upgrader
payload is the older loader (the version bytes it prints read 4.3), which has
no `skip_pgc_pgd_check`; running the bundle therefore does **not** produce a
loader that accepts `$`.  It is a firmware-update helper, not a route to the
current loader.

- The ds30 GUI alone cannot replace the loader either: it ships with the
  bootloader-overwrite guard enabled and the running loader enforces the same
  guard.
- Bootloader images are not prebuilt in this repository: the v3 one is built
  from `firmware-v4.5/`, the v4 one from `firmware-v1/`.

### v4 update route

Rewrite the loader with a hardware programmer (Pickit2 route in
[flashing.md](flashing.md)) using the image built from
`Bootloaders/BPv4-bootloader/firmware-v1/`.  A blank/bricked device always
needs the hardware-programmer route, because `pirate-loader` needs a running
bootloader to talk to.

## Verification

- Not run for this page; see [verification.md](verification.md) for the
  maintainer record.

## Limits & out of scope

- The ds30 loader GUI and the Windows update bundles are third-party tooling
  and are not re-documented here.
- Exact bootloader version bounds for `$` persistence on every historical v3
  loader are **not** enumerated — only the sources present in this tree.
