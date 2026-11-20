# Bootloaders

**Status:** maintainer-facing.  Sources: `Bootloaders/v3/` (ds30 loader),
`Bootloaders/v4/` (USB CDC loader), `Bootloaders/pirate-loader/` and
`Bootloaders/tests/`.

This page describes the resident loaders: the folder layout, versions, entry
rules, host protocol, and when to update the loader rather than just flash an
application.  Flashing an application is [flashing.md](flashing.md).

## Scope

- **In:** the bootloader tree, the v3 and v4 entry rules, the host frames and
  status bytes, the build/test route, and the decision to update a loader.
- **Out:** the application build ([building.md](building.md)) and the normal
  application flash procedure ([flashing.md](flashing.md)).

## Versions and folder map

| Path | Contents |
|---|---|
| `Bootloaders/v3/` | v3 ds30 loader for the PIC24FJ64GA002; one source set |
| `Bootloaders/v4/` | v4 USB CDC loader for the PIC24FJ256GB106; one source set |
| `Bootloaders/pirate-loader/` | shared host tool, version **1.0.7** |
| `Bootloaders/tests/` | native, image and opt-in bench probes |

The **application revision is v9.0 on both boards**.  The **bootloader revision
is 5.0 on both boards**; it is independent of the application version.  The v4
hello replies **`F1 05 00 4B`** (device ID `F1`, version **5.0**, `K`).  The v3
hello reports ds30 protocol 1.0.2; its separate stored word at `0xABFA` reports
loader **5.0** (`0x0500`).  Protocol device IDs 212/241 are not the silicon
DEVID values.

The old v3/v4 loader revisions, self-upgraders, alternate applications, GUI
binaries and duplicate host binaries are removed; they remain available in Git
history.  Bootloader images are built from source, not downloaded, and no
prebuilt loader binary is shipped.

## Address and protocol contract

An address in a command is a PIC program/table address, not an instruction
index, packed byte offset or Intel HEX byte offset.  A 24-bit instruction
occupies two program addresses and four Intel HEX bytes (including a phantom
byte).  A row is 64 instructions = 128 program addresses = 192 payload bytes.
An erase page is eight rows = 1024 program addresses.

The host sends `C1`; the loader replies with device ID, protocol major,
protocol minor and `K`.  Subsequent commands are:

```
address[23:16], address[15:8], address[7:0], command, size, payload..., checksum
```

`size` includes the checksum.  The entire packet sums to zero modulo 256.
Command `01` has size `01` and arms a deferred page erase.  Command `02` has
size `C1` (193) and supplies exactly one complete row.  The deferred erase
belongs to the specified page and is consumed by one subsequent write in that
page; an error cancels it.  Row zero's reset GOTO is rewritten to the loader.
Replies: `K` success, `N` checksum, `P` protected/invalid address, `U` invalid
command/length, `V` programming or readback failure.  Malformed lengths are
drained without overflowing the row buffer.  No incomplete command starts NVM.

The v4 `FF`, size `01`, requests restart; its arbitrary address bytes are
ignored for compatibility with the existing host tool.

## v3 bootloader (ds30 loader)

Source: `Bootloaders/v3/ds30loader.s` plus `settings.inc`, `devices.inc` and the
device linker script.  It is built by the directory Makefile, not by a
prebuilt GUI.  The loader occupies one program page at `0xA800`:

| Item | Address |
|---|---|
| Hardware reset GOTO | `0x0000` -> `0xA800` |
| IVT / AIVT | `0x0004` / `0x0104` (application-owned in normal operation) |
| Relocated application reset GOTO | `0xA7FC`, `0xA7FE` |
| Loader | `0xA800` through `0xABF7` |
| Legacy firmware entry | `0xABF8` |
| Version | `0xABFA` = `0x0500` (5.0) |
| CONFIG2 / CONFIG1 | `0xABFC` / `0xABFE` = `0x799F` / `0x3F7F` |
| Persistent entry flag / signature | RAM `0x27FA` / `0x27FC` |

The last valid aligned row starts at `0xA780` and includes the relocated GOTO.
Only `TBLPAG=0` is allowed.  The entire last erase page is protected, including
the configuration words.  ICSP, not this loader, installs or updates the loader.

### Entry

At reset the loader decides between staying in the bootloader and jumping to
the application:

1. Compare `firmware_signature` against `0x31415926`.  A mismatch goes to the
   jumper test.
2. If the signature matches, read `skip_pgc_pgd_check` at `0x27FA` (the flag
   the firmware `$` command sets).  If it is set, clear it and enter the loader
   directly.
3. Otherwise run the jumper test: drive **PGD** low, hold **PGC** as an input
   with a pull-up, and sample it.  A **PGC/PGD jumper** pulls PGC low and
   selects the bootloader; with no jumper PGC reads high and the loader selects
   the application.
4. In the bootloader it waits for host commands at **115200 8N1**.  The receive
   wait is bounded (`BLTIME = 500 ms` per byte).

The current v3 **5.0** loader understands the `$` flag.  Older v3 4.1/4.3
loaders do not; use the PGC/PGD jumper with those.  The jumper method always
works, including with no application present.  A host-wait timeout clears a
pending erase and either remains resident or returns to the application.  An
erased application always stays recoverable.

### Errors and limits

Strict command/length, table-page and alignment checks precede all NVM access.
The NVM unlock is guarded by `DISI`, followed by the two required NOPs; `WREN` is
cleared afterward and `WRERR`/readback failures return `V`.  UART receive
handles overrun, framing and parity faults; transmit observes FIFO capacity and
drains before exit.  The running loader rejects writes that would reach its own
page: the last safe row starts at `0xA780`; a row whose write reaches `0xA800`
or beyond is rejected with `P`.  Replacing the loader therefore requires a
hardware programmer.

### Host framing

The v3 ds30 loader uses the same address/command/length/checksum shape and the
same `K` / `N` / `P` / `V` status bytes; it has no `U` status.  The hello reply
is `[DEVICEID][VERMAJ][VERMIN*16+VERREV]` followed by `K`.  `pirate-loader`
speaks this framing for the v3 part.

## v4 bootloader (USB CDC)

Source: `Bootloaders/v4/` (`bootloader.c`, `bootloader.h`, the polled USB stack
and descriptors).  It builds by the directory Makefile.  The loader enumerates
as **`04D8:FAFF`** and reports version **5.0** (`VER_H = 5`, `VER_L = 0`).

| Item | Address |
|---|---|
| Hardware reset GOTO | `0x0000` -> `0x0400` |
| IVT / AIVT | `0x0004` / `0x0104` |
| Loader allocation | `0x0400` through `0x1FFF` |
| Version | `0x1FF6` = `0x0500` (5.0) |
| Application entry | `0x2000` |
| Protected configuration erase page | `0x2A800` through `0x2ABFF` |
| CONFIG3 / CONFIG2 / CONFIG1 | `0x2ABFA` / `0x2ABFC` / `0x2ABFE` |
| Persistent entry flag / signature | RAM `0x47FA` / `0x47FC` |

CONFIG1/2 remain `0x3E7F` / `0x239E`; the 12 MHz primary crystal and DIV3 PLL
settings yield the USB 48 MHz clock and FCY=16 MHz.  Full rows are staged,
programmed and verified, with `DISI` protecting the key sequence.  Erases are
bound to an accepted page; rows are aligned and checked against the complete
loader/configuration windows before any table write.  Page zero is allowed for
application vectors but its reset GOTO always points to `0x0400`.

### Entry and exit

NORMAL held, a PGC/PGD jumper, or the existing firmware signature plus entry
flag forces residency.  Otherwise the loader reads the first application
instruction at **`0x2000`, not `0x4000`**, and remains resident if it is erased.
Both its low 16 bits and high byte are checked.  This is a presence guard, not
an image checksum or a guarantee against interrupted application programming.
The flag is consumed once; application handoff releases jumper/button
pull-ups, outputs and the loader timer.  RB9 is the target supply enable,
never an activity LED, and is held off throughout loader execution.

Restart acknowledges the command without overwriting an SIE-owned buffer,
waits for IN completion (bounded to 250 ms), detaches for 100 ms, then resets.
A blank application returns to the loader.  Interrupted packets require a new
hello after a 1 s idle timeout.  USB reset, reconfiguration and endpoint halt
invalidate partial protocol state and pending erase; no stale packet may write.

### USB implementation

The stack is deliberately polled: no fast path drops EP0 transactions and no
ISR shares C state with the parser.  Hardware-updated BDs are volatile,
512-byte aligned, and use the PIC24 count/status/address layout.  EP0 is 8
bytes; EP1 IN advertises a ten-byte notification endpoint; EP2 IN/OUT are
64-byte bulk endpoints.  Boot replies are only 1/4 bytes, so each is already a
short packet; the firmware's streaming/ZLP machinery is unnecessary here.

EP0 tracks data/status phases, DATA1 status OUT, descriptor termination,
deferred `SET_ADDRESS`, stalls and SETUP cancellation.  Requests validate type,
direction, recipient, interface, endpoint, index, selector and length.  Invalid
descriptors/endpoints stall before any array traversal.  `GET_STATUS` does not
modify a BD.  Configuration zero disables all non-control endpoints.  Halt
changes stop that DMA direction before reclaiming its BD and reset toggles.
Line coding is exactly seven wire bytes, never a native enum/struct ABI;
unsupported requests stall.  OUT ZLPs are recycled, not delivered as bytes.

Timer1 supplies deadlines independently of USB SOF, so unplug/suspend cannot
freeze protocol timeouts.  Idle enumeration is intentionally indefinite, but
never blocks USB servicing.  Full USB suspend-current compliance is not
claimed: this loader does not implement board-wide low-power sleep.

## Updating the bootloader

Application flashes are routine; bootloader writes are rare.  Update the
bootloader only when:

- the board has an old loader without `$` support (v3 4.1/4.3) and you want
  software entry;
- a bootloader defect fix is specifically required;
- the bootloader region is blank or corrupted, or an ICSP install is needed.

Both current loaders protect their own page, so **the running loader cannot
replace itself**.  Use a hardware programmer (ICSP/Pickit2 or equivalent) to
write the loader image.  Then enter the loader (NORMAL button or PGC/PGD jumper
on v4; PGC/PGD jumper on v3) and write the application with `pirate-loader`.
No software self-upgrader, GUI, or prebuilt loader bundle is shipped.

- **v3:** build `Bootloaders/v3/`; the image includes the loader page and the
  configuration words.  A blank/bricked chip needs the hardware programmer.
  After the loader write, jumper into the loader and write `Firmware/bpv3.hex`
  with `pirate-loader`; the host write places the relocated application GOTO at
  `0xA7FC` and the loader rewrites the row at `0x0000` to point at `0xA800`.
- **v4:** build `Bootloaders/v4/`; a blank/bricked chip needs the hardware
  programmer.  After the loader write, hold NORMAL (or fit the PGC/PGD jumper)
  while powering up, then write `Firmware/bpv4.hex` with
  `pirate-loader --dev=<bootloader port> --hex=Firmware/bpv4.hex`.  The write
  installs the loader's reset-vector jump at `0x0000`; a direct programmer write
  of the application would leave the application's own reset vector in place.

## Build and test

Requires Microchip XC16 (verified with 2.10), GNU Make and CMake.  Python 3 and
a native GCC-compatible C compiler run the host tests.  No build/test target
opens a serial port or flashes a board.

```sh
make -C Bootloaders XC16=/path/to/xc16/bin/xc16-
make -C Bootloaders test
cmake -S Bootloaders/pirate-loader -B Bootloaders/pirate-loader/build
cmake --build Bootloaders/pirate-loader/build --config Release
```

Windows example (PowerShell):

```powershell
mingw32-make -C Bootloaders "XC16=C:/Program Files (x86)/Microchip/MPLABXC16/v2.10/bin/xc16-"
mingw32-make -C Bootloaders test
```

Images and linker maps are `v3/build/bpv3-bootloader.{hex,map}` and
`v4/build/bpv4-bootloader.{hex,map}`.  The loaders and host tool treat compiler
warnings as errors.  Retained MPLAB X project metadata lists the same sources;
the reproducible, qualified build path is the directory Makefile.  IDE builds
are not independently qualified.

`pirate-loader` 1.0.7 sets **115200 8N1** on Windows and uses `CS8` on POSIX;
the older Windows 921600 bug is fixed.  There are no tracked prebuilt
`pirate-loader` binaries in the current tree.

## Bench status and limits

- **v4 bootloader 5.0:** loader-only bench PASS after fresh XM64 ICSP wiring:
  erase/write 31 chunks, CONFIG `0x3E7F`/`0x239E`, hello `F1 05 00 4B`,
  464 live protocol + 4 scratch checks, all 56 loader rows and stored
  version `0x0500` at `0x1FF6`.  The final combined **v9.0** application
  (`Firmware/bpv4.hex`, build `14256fd5`) was then uploaded through the 5.0
  loader: 472 application rows + 8 vector rows, post-flash restart to COM6,
  banner **v9.0**, `HiZ>`.  The NORMAL-held and jumper physical entry cases
  were exercised on 4.15; the 5.0 reflash was loader-only plus the app upload,
  not a full repeat of those physical cases.
- **v3 bootloader 5.0:** build and image tests verify `0x0500` at `0xABFA`;
  there is **no v3 hardware** available, so it is build/host-evidence only.
  The v3 `$` entry is source-derived and not bench-verified.

Raw malformed EP0 control requests remain native-test evidence, not a live
USB-control-transfer claim.  ICSP uses the existing XM64 `picprog.py`; it is
not a replacement programming engine.  Detailed local evidence is under the
ignored `Bootloaders/tests/build/` directories; the opt-in probes are never
run by `make test`.
