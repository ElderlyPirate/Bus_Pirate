# Bus Pirate Bootloaders

Only two board revisions are maintained here:

| Directory | Board / device | Transport | Loader version |
| --- | --- | --- | --- |
| `v3/` | Bus Pirate v3, PIC24FJ64GA002 | UART1, 115200 8N1 | 4.6 |
| `v4/` | Bus Pirate v4, PIC24FJ256GB106 | Full-speed USB CDC | 4.15 |
| `pirate-loader/` | Shared host tool, including GA004 v3 clones | Serial port | 1.0.7 |

Historical v4.1/v4.3 ds30 sources, v1/v2 board pin maps, self-upgraders,
alternate applications, GUI binaries and duplicate host binaries are removed.
They remain available in Git history. Build bootloader images from source;
do not distribute an old HEX under a new version's name.

## Build And Test

Requires Microchip XC16 (verified with 2.10), GNU Make, and CMake. Python 3
and a native GCC-compatible C compiler run the host tests. No build/test
target opens a serial port or flashes a board.

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
`v4/build/bpv4-bootloader.{hex,map}`. The loaders and host tool treat compiler
warnings as errors. Retained MPLAB X project metadata lists the same sources;
the reproducible, qualified build path is the directory Makefile. IDE builds
are not independently qualified.

## Address And Protocol Contract

An address in a command is a PIC program/table address, not an instruction
index, packed byte offset, or Intel HEX byte offset. A 24-bit instruction
occupies two program addresses and four Intel HEX bytes (including a phantom
byte). A row is 64 instructions = 128 program addresses = 192 payload bytes.
An erase page is eight rows = 1024 program addresses.

The host sends `C1`; the loader replies with device ID, protocol major,
protocol minor and `K`. Subsequent commands are:

```
address[23:16], address[15:8], address[7:0], command, size, payload..., checksum
```

`size` includes the checksum. The entire packet sums to zero modulo 256.
Command `01` has size `01` and arms a deferred page erase. Command `02`
has size `C1` (193) and supplies exactly one complete row. The deferred erase
belongs to the specified page and is consumed by one subsequent write in that
page; an error cancels it. Row zero's reset GOTO is rewritten to the loader.
Replies: `K` success, `N` checksum, `P` protected/invalid address, `U` invalid
command/length, `V` programming or readback failure. Malformed lengths are
drained without overflowing the row buffer. No incomplete command starts NVM.

The v3 hello reports ds30 protocol 1.0.2; the separate word at `0xABFA` reports
loader version 4.6. The v4 hello reports 4.15, also stored at `0x1FF6`.
These protocol device IDs (212/241) are not the silicon DEVID register values.
The v4 `FF`, size `01`, requests restart; its arbitrary address bytes are
ignored for compatibility with the existing host tool.

## Manual Audit And Evidence

The retained sources, protocol conversions and linker maps were reviewed
against the following, not assumed correct because they were retained:

- DS70000157G: `TBLRDL/H`, `TBLWTL/H`, `GOTO`, `DISI`, compares and skips.
- DS39897C (the PIC24FJ256GB110 family **data sheet**, covering GB106):
  4.3.2/table 4-31, 5.2/5.6.1, reset/vectors, 8.5, 18.2/18.3.1/18.7 and
  configuration words. It is not itself the separate Family Reference Manual.
- DS39881E (PIC24FJ64GA004 family, covering GA002): 4.3, 5.2/5.5, 7.2,
  PPS and UART chapters, configuration words and the existing A3 UART workaround.
- The normal firmware USB implementation was read as guidance; it was not
  copied wholesale or modified. Its seven-byte line-coding representation and
  ownership discipline inform the loader's independent, polled implementation.

Native tests execute the actual v4 parser, CDC and EP0 state machines with
SFR stand-ins. A limited interpreter exercises the actual v3 assembly command
path, including both address bytes, row latches and readback. Host-tool tests
exercise HEX parsing, bounds, checksums, clone handshake, relocated GOTO and
preflight. Linked-image tests independently check reset words, layout,
configuration and version words. These are not a PIC or USB electrical model.

**Bench gate:** v3 is build/host-evidence only (no v3 board). The rebuilt v4
image is NOT bench-qualified until explicitly flashed and tested. Earlier
4.14 bench claims do not qualify this new 4.15 image. ICSP installation erases
the application: retain an approved application image for restoration. Owner
wiring confirmation is required before any programming operation.
