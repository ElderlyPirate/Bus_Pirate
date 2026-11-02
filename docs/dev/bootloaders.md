# Bootloaders

**Status:** draft.  Confirmed from `Bootloaders/BPv3-bootloader/firmware-v4.5/`
(ds30 loader assembly), `Bootloaders/BPv4-bootloader/firmware-v1/` (the v4 C
bootloader) and `Bootloaders/pirate-loader/`.

This page describes the resident bootloaders: what the folders contain, how
each decides to enter the bootloader, and when you must update the bootloader
rather than just flash firmware.  Flashing firmware is
[flashing.md](flashing.md).

## Scope

- **In:** the bootloader folder layout, the v3 and v4 entry rules, the
  bootloader host protocol as `pirate-loader` uses it, and the
  update-vs-flash decision.
- **Out:** the application firmware build ([building.md](building.md)) and the
  normal firmware flash procedure ([flashing.md](flashing.md)).

## Folder map

| Path | Contents |
|---|---|
| `Bootloaders/pirate-loader/` | host-side loader source (`pirate-loader.c`), CMake build, `test.sh` / `test.bat` handshake smoke scripts |
| `Bootloaders/BPv3-bootloader/firmware-v4.5/` | **current** v3 bootloader: ds30 loader for the PIC24FJ64GA002, MPLAB-X project `ds30loader.X` |
| `Bootloaders/BPv3-bootloader/old/` | superseded v3 loaders (`firmware-v4.1`, `firmware-v4.3`) and older v2→v4 updaters (`upgrader-v2tov4/`, `v2tov4-bootloader-update/`) |
| `Bootloaders/BPv3-bootloader/v4xtov43-bootloader-update/` | Windows bootloader-update bundle: upgrade HEX images and prebuilt `pirate-loader` binaries + ds30 GUI |
| `Bootloaders/BPv3-bootloader/upgrader-v4tov4/` | v4-to-v4 bootloader upgrader (legacy MPLAB project) |
| `Bootloaders/BPv3-bootloader/ds30_Loader_GUI/` | the ds30 loader Windows GUI used to flash v3 bootloaders |
| `Bootloaders/BPv4-bootloader/firmware-v1/` | **current** v4 bootloader: USB CDC bootloader, MPLAB-X project `bpv4-bootloader.X` |

## v3 bootloader (ds30 loader)

Source: `Bootloaders/BPv3-bootloader/firmware-v4.5/ds30loader.s` plus
`settings.inc`, `devices.inc` and the device linker script.  It builds with
MPLAB-X (project `ds30loader.X`) and reports bootloader version **4.5**
(`BLVERSION = 0x0405`).

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
   wait is bounded (`BLTIME = 500 ms`); if the host does not keep it busy the
   loader can return to the application, so start the host loader as soon as
   the board is in the bootloader.

Consequences:

- The `$` command only works on this loader (and later): the older
  `old/firmware-v4.1` / `old/firmware-v4.3` sources have no
  `skip_pgc_pgd_check`, so `$` cannot hand off there.
- The jumper method always works, including with no application present.

## v4 bootloader (USB)

Source: `Bootloaders/BPv4-bootloader/firmware-v1/` (`main.c`, `bootloader.c`,
`bootloader.h`, the USB stack and descriptors).  It builds with MPLAB-X
(project `bpv4-bootloader.X`) and reports bootloader version **4.14**
(`VER_H = 0x04`, `VER_L = 0x0e`).

At reset it uses the same two ideas as v3, plus the hardware button:

1. `firmware_signature` must match `0x31415926`, otherwise it enters the
   bootloader.
2. Otherwise, if `skip_pgc_pgd_check` (at `0x47FA`, set by the firmware's `$`)
   is clear or the **NORMAL button** is held, it samples PGD: a **PGC/PGD
   jumper** (or the held button) keeps it in the bootloader, while a high PGD
   with the button released jumps to the application — but only when the
   application's reset vector is present, so a blank application still falls
   through to the bootloader.  It clears the flag on the way.
3. It enumerates as a USB device (`04d8:FAFF` in bootloader mode) and waits
   for the host.

The host protocol (`bootloader.c`):

| Step | Bytes |
|---|---|
| Hello | host sends `0xC1`; bootloader answers `[DEVICEID][VER_H][VER_L]` then `'K'` |
| Command frame | `[addrU][addrH][addrL][cmd][len][payload…][checksum]`; bytes must sum to zero |
| `cmd = 0x01` | enable erase for the next write |
| `cmd = 0x02` | erase-if-enabled then write one row, verify, answer `'K'` (`'V'` on verify fail, `'P'` if the target is protected) |
| `cmd = 0xff` | acknowledge `'K'`, disable USB and `RESET` |

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

Update routes:

- **v3:** the Windows bundle `v4xtov43-bootloader-update/` (upgrade HEX +
  ds30 GUI) writes the v3 loader; the older `upgrader-v4tov4/` is the
  v4-to-v4 upgrader.
- **v4:** rewrite with a hardware programmer (Pickit2 route in
  [flashing.md](flashing.md)) using the bootloader image built from
  `Bootloaders/BPv4-bootloader/firmware-v1/`.
- A blank/bricked device always needs the hardware-programmer route, because
  `pirate-loader` needs a running bootloader to talk to.

## Verification

- Not run for this page; see [verification.md](verification.md) for the
  maintainer record.

## Limits & out of scope

- The ds30 loader GUI and the Windows update bundles are third-party tooling
  and are not re-documented here.
- Exact bootloader version bounds for `$` persistence on every historical v3
  loader are **not** enumerated — only the sources present in this tree.