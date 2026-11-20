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

**Bench gate:** v3 is build/host-evidence only (no v3 board). V4's
software-controlled bench legs and normal cold power-up below pass;
NORMAL-held/jumper power-up cases remain pending owner action. Earlier 4.14
bench claims do not qualify 4.15. ICSP
installation erases the application: retain an approved application image.
Owner wiring confirmation is required before programming.

## V4 Bench, 2026-11-20

- Owner-confirmed XM64 PA4 -> ICSP 5/PGC, PA5 -> 4/PGD, PA6 -> 1/MCLR,
  GND -> 3/VSS; 2/VDD and 6 open, separate USB power, Vpu out.
- Existing XM64 `icsp-usb` programmer on COM9 (`1209:0002`), target ID
  `0x001019`, revision 4. Full erase/config erase and 31-chunk bootloader write
  pass independent verify. CONFIG1/2 remain `0x3E7F`/`0x239E`.
- Qualified image for these legs: SHA-256
  `B10BF69E7C7B68E3EA36CE249CD6FCB2994912E236122BC8DA46808068304698`.
  Blank entry enumerates as COM11 (`04D8:FAFF`), hello `F1 04 0F 4B`.
- 464 live framing/protection/recovery checks plus four scratch erase/write
  checks pass. ICSP independently confirms the scratch pattern, all eight
  erased page rows, unchanged reset/vectors and all 56 loader-region rows.
- Approved application SHA-256
  `EA4CC34245C7710576D2140F48DBB31FD687481F0BF53F631BBA8C92E349AA44`
  installs through pirate-loader and restarts without replug to COM6
  (`04D8:FB00`), firmware `a318d309`, `HiZ>`. All 472 application rows,
  application vectors, protected reset GOTO, 56 loader rows and CONFIG1/2
  independently pass ICSP readback on the final installed image.
- Firmware `$`/`y` -> loader hello -> acknowledged restart -> application
  passes three consecutive cycles without physical action.
- Owner-performed normal cold power-up passes with PA4/PA5/PA6 disconnected,
  ground connected, no jumper and NORMAL released: COM6 `04D8:FB00`, firmware
  v8.0 `a318d309`, `HiZ>`. Two info queries three seconds apart remain
  responsive; no resident `04D8:FAFF` device or other observed anomaly.
  Observation used only CR/`i`, not reset, ICSP or bootloader-entry commands.
- Physical NORMAL-held/jumper cases are **not yet run**. Raw
  malformed EP0 control requests remain native-test evidence, not a live
  USB-control-transfer claim. No v3 or Windows 11 result is claimed.

The opt-in `tests/bench_v4_protocol.py`, `bench_v4_entry.py` and
`bench_v4_readback.py` reproduce these legs; they are never run by `make test`.
The protocol probe's scratch option deliberately erases application page
`0x2400` and must not run on an application that needs preserving. ICSP uses
the existing XM64 `picprog.py`, not a replacement programming engine.

Fixture release behavior must be checked against the installed XM64 image,
not inferred from source or carried forward from an earlier run. The initial
bench used explicit `linecheck` release; the readback probe no longer applies
it by default. Also, the application's entry confirmation needs the CDC reader
kept open through detach, as in the existing `enter_bootloader.py` helper.
An early-close probe stranded that
application path; corrected-reader tests pass on the original image. The
temporary startup hypotheses were removed, not shipped as unproven fixes.
The actual code regression found by this bench is in host HEX validation:
XC16 records may cross a 64 KiB boundary; the host now accepts those while
retaining complete destination bounds. Detailed local transcripts are in
ignored `tests/build/bench-20261120/`.

On the subsequent fixture recheck, the XM64 USB CDC programmer image
`icsp-programmer/firmware-usb/icsp-usb.hex` (SHA-256
`6B6A0DCEB0B0147D79F079151DD037519A65AF89A5C19968FFF0BF4CF64E5742`)
was reflashed through its own USB bootloader: 16 pages verified, full-image
CRC `0xBDBB`. A pre-erase backup matched the selected image's program bytes.
After reflash, ping, PIC ID/revision, CONFIG1/2 and reset/version/application
entry rows all passed without a release workaround. On that specific run,
the application COM6 did not return after the reads; no `linecheck` was then
issued. These are current observations for that image, not a blanket finding
about other XM64 fixes. The owner subsequently performed the separate normal
cold-power test recorded above; neither the reflash nor an ICSP reset was
counted as that physical test.
