# Bootloaders

**Status:** maintainer-facing.  Derived from
`Bootloaders/BPv3-bootloader/firmware-v4.5/` (ds30 loader assembly),
`Bootloaders/BPv4-bootloader/firmware-v1/` (the v4 C bootloader) and
`Bootloaders/pirate-loader/`.

This page describes the resident bootloaders: what the folders contain, how
each decides to enter the bootloader, the host protocol, and when you must
update the bootloader rather than just flash firmware.  Flashing firmware is
[flashing.md](flashing.md).

## Scope

- **In:** the bootloader folder layout, the v3 and v4 entry rules, the host
  frames and status bytes, and the update-vs-flash decision including the
  route to the current v3 loader.
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
MPLAB-X (project `ds30loader.X`).  It occupies one program page at `0xA800`,
with the version word and the config words at its end.

Version numbers are split:

| Form | Value | Source |
|---|---|---|
| stored `BLVERSION` | **4.5** (`0x0405`) | last word before the config words; `i` reads it through `BL_ADDR_VER` |
| hello reply version | **1.02** | `VERMAJ = 1`, `VERMIN = 0`, `VERREV = 2`; the third byte is `VERMIN * 16 + VERREV` |

At reset the loader decides between staying in the bootloader and jumping to
the application:

1. Compare `firmware_signature` against `0x31415926`.  A mismatch goes to the
   jumper test.
2. If the signature matches, read `skip_pgc_pgd_check` at `0x27FA` (the flag
   the firmware `$` command sets).  If it is set, clear it and enter the
   loader directly.
3. Otherwise run the jumper test: drive **PGD** low, hold **PGC** as an input
   with a pull-up, and sample it.  A **PGC/PGD jumper** pulls PGC low and
   selects the bootloader; with no jumper PGC reads high and the loader
   selects the application.
4. In the bootloader it waits for host commands at 115200 8N1.  The receive
   wait is bounded (`BLTIME = 500 ms` per byte).

Consequences:

- The `$` command only works on this loader (and later): the older
  `old/firmware-v4.1` / `old/firmware-v4.3` sources have no
  `skip_pgc_pgd_check`, so `$` cannot hand off there.
- The jumper method always works, including with no application present.
- After a `$` hand-off, a host-wait timeout returns to the loader command
  loop, not to the application; the `WFWJUMP` state records the firmware jump.
  A timeout with the PGC/PGD jumper still fitted also stays in the loader.
  A reset-time entry with no jumper and no `$` flag returns to the
  application.
- The running loader refuses writes that would reach its own page: the last
  safe row starts at `0xA780`; a row whose write reaches `0xA800` or beyond is
  rejected with `'P'`.  Replacing the loader therefore needs the upgrader
  trampoline or a hardware programmer (§v3 update route).

## v4 bootloader (USB)

Source: `Bootloaders/BPv4-bootloader/firmware-v1/` (`main.c`, `bootloader.c`,
`bootloader.h`, the USB stack and descriptors).  It builds with MPLAB-X
(project `bpv4-bootloader.X`); its hello reply reports bootloader version
**4.14** (`VER_H = 0x04`, `VER_L = 0x0e`).  The USB bootloader enumerates as
`04d8:FAFF`.

At reset `main()` makes **one** decision: the entry block runs when the
signature is missing **or** the flag is clear **or** the NORMAL button is
held.

1. Entry block (signature mismatch `||` `skip_pgc_pgd_check` clear `||`
   button down): disable the flag, wait out a 35 × 65,000 loop, light the
   MODE/USB/VREG LEDs, and sample PGD twenty times.  If PGD reads high, the
   button is released, and the application reset vector word is not erased, it
   jumps to the application at `0x2000`.  Anything else stays in the
   bootloader: a low PGD (the PGC/PGD jumper), a held button, an erased
   application vector, or a jump that was not taken.
2. No entry block (valid signature **and** flag set **and** button released):
   it goes straight to `startpoint()` and the bootloader.  This is the `$`
   path.
3. The `skip_pgc_pgd_check` flag at `0x47FA` is cleared in both paths.
4. `startpoint()` turns MODE/USB off and the VREG LED on, starts USB, turns
   the VREG LED off while it waits for enumeration, then turns it on again
   before `bootloader()`; the command loop runs with VREG lit.

The loader does **not** fully validate the application image: it checks only
the first word of the application reset vector at `0x2000`.  An erased
`0xFFFF` word keeps the board in the bootloader; a corrupt non-erased vector
still passes that check and can be jumped into.  Use the NORMAL button or the
PGC/PGD jumper on a suspect board.

The host protocol (`bootloader.c`):

| Step | Bytes |
|---|---|
| Hello | host sends `0xC1`; bootloader answers `[DEVICEID][VER_H][VER_L]`, then the first status byte (`'K'`) |
| Command frame | `[addrU][addrH][addrL][cmd][len][payload…][checksum]`; bytes must sum to zero |
| `cmd = 0x01` | enable erase for the next write |
| `cmd = 0x02` | erase-if-enabled then write one row, verify; the next status is `'K'`, `'V'` or `'P'` |
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

A verbose flash that stalls is usually waiting on one of these bytes; `'N'`
and `'U'` mean the host framing is wrong, `'P'` means the address range is
protected.

The loader protects the bootloader region (`PROT_BL`), the config page
(`PROT_CONFIG`) and the reset-vector jump (`PROT_GOTO`), so an application
write cannot overwrite the bootloader.  This last point is why the current v4
loader is described as the "4.14 erase-guard" loader.

### v3 host framing

The v3 ds30 loader uses the same address/command/length/checksum shape and
the same `'K'` / `'N'` / `'P'` / `'V'` status bytes; it has no `'U'` status.
`pirate-loader` speaks this framing for the v3 part.

| Step | Bytes |
|---|---|
| Hello | host sends `0xC1`; loader answers `[DEVICEID][VERMAJ][VERMIN*16+VERREV]`, then `'K'` |
| Command frame | `[addrU][addrH][addrL][cmd][len][payload…][checksum]`; bytes must sum to zero |
| `cmd = 0x01` | enable erase for the next write |
| `cmd = 0x02` | erase-if-enabled then write one row, verify; the next status is `'K'`, `'V'` or `'P'` |

## When to update the bootloader

Firmware flashes are routine; bootloader writes are rare.  Update the
bootloader only when:

- the board has an **old loader without `$` support** (`old/firmware-v4.1`,
  `old/firmware-v4.3`) and you want the software entry to work;
- a bootloader defect fix is specifically required;
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
3. Put the board into the loader: after the programmer route, fit the
   PGC/PGD jumper and power-cycle; the upgrader route jumps into the new
   loader directly.  Write `Firmware/bpv3.hex` with the ds30 GUI
   (PIC24FJ64GA002, 115200) or
   `pirate-loader --dev=<port> --hex=Firmware/bpv3.hex`.  Then finish the
   entry route: on the programmer route remove the PGC/PGD jumper; on the
   upgrader route power-cycle the board (the loader stays in its command loop
   after the application write).
4. Use one of those host tools, not the hardware programmer, for the
   application.  The host write places the application's own reset goto at
   `STARTADDR-4` (`0xA7FC`), and the loader rewrites the row at `0x0000` to a
   goto to the loader (`ds30loader.s` `usrapp` and its row-0 rewrite;
   `pirate-loader.c` `fixJumps`).  A direct programmer write of the
   application instead restores the application's reset vector at `0x0000`,
   so the loader never runs at reset and neither `$` nor the PGC/PGD jumper
   can enter it.

| Route | How | Limit |
|---|---|---|
| Hardware programmer | erase the chip and write the loader only ([Pickit2 route](flashing.md#the-pickit2-route-full-chip-programming)); then jumper into the loader and write the application with the ds30 GUI or `pirate-loader` | always works, including blank/bricked chips |
| Upgrader trampoline | rebuild `upgrader-v4tov4/` with its `firmware.h` array replaced by the built 4.5 page, flash that image with `pirate-loader`; it erases the page before the loader and the loader page, writes the new loader, and points `0x0000` at it; then write the application with the ds30 GUI or `pirate-loader` | needs the HEX→array conversion (`convert.html` in the same folder) and a working current loader |

What the update **bundle** installs: `v4xtov43-bootloader-update/` carries
`BPv3-bootloader-upgrade-v4xtov4.3.hex` (the upgrader as compiled for the
older release), `BPv3-Firmware-v4.5.hex` (the 4.5 **application** image), the
ds30 GUI and prebuilt `pirate-loader` binaries.  The as-checked-in upgrader
payload is the older loader (the version bytes it prints read 4.3), which has
no `skip_pgc_pgd_check`; running the bundle therefore does **not** produce a
loader that accepts `$`.  It is a firmware-update helper, not a route to the
current loader.

- The ds30 GUI alone cannot replace the loader either: its `settings.xml`
  ships `IgnoreBLOverwrite=false` with `IgnoreBLOverwriteAllow=false`, and
  the running loader enforces the same guard.
- Bootloader images are not prebuilt in this repository: the v3 one is built
  from `firmware-v4.5/`, the v4 one from `firmware-v1/`.

### v4 update route

Rewrite the loader with a hardware programmer (Pickit2 route in
[flashing.md](flashing.md)) using the image built from
`Bootloaders/BPv4-bootloader/firmware-v1/`.  A blank/bricked device always
needs the hardware-programmer route, because `pirate-loader` needs a running
bootloader to talk to.

After the loader write, hold the NORMAL button (or fit the PGC/PGD jumper)
while powering up, then write the application with
`pirate-loader --dev=<bootloader port> --hex=Firmware/bpv4.hex`.  That write
is what installs the loader's reset-vector jump at `0x0000`: the bootloader's
`WritePage()` replaces an application write to `0x0000` with a goto to
`BLSTARTADDR`.  A programmer write of the application would leave the
application's own reset vector there, so the loader would not run at reset and
the button and `$` entry would stop working.

## Verification

The entry rules and frames above come from the bootloader sources named in
each section.  The per-set basis is in [verification.md](verification.md).

## Limits & out of scope

- The ds30 loader GUI and the Windows update bundles are third-party tooling
  and are not re-documented here.
- Exact bootloader version bounds for `$` persistence on every historical v3
  loader are **not** enumerated — only the sources present in this tree.
