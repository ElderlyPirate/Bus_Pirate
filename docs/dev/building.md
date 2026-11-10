# Building the firmware

**Status:** maintainer-facing.  Derived from the tracked `Firmware/Makefile`, the MPLAB-X
project files, `configuration.h`, the message tooling and the native tests in
the repository tip.  No build was run for this page.

This page covers how to compile the Bus Pirate v3 / v4 community firmware,
which feature set goes into the image, and which artifacts are produced.
Flashing the result is [flashing.md](flashing.md).

## Scope

- **In:** toolchain prerequisites, the command-line `make` build, the feature
  sets in `Firmware/configuration.h`, the MPLAB-X route, artifact locations,
  message-string generation, and the common build errors that follow from these
  files.
- **Out:** what each function does with its feature switch (the per-function
  pages), and the test inventory — the authoritative suite table is
  [tooling.md](tooling.md) §Test suites.

## Prerequisites

| Requirement | Detail |
|---|---|
| Microchip **XC16** | The PIC24 C compiler and linker.  The Makefile falls back to the installer defaults `/opt/microchip/xc16/v2.10` and `C:/Program Files (x86)/Microchip/MPLABXC16/v2.10`; otherwise the toolchain must be on `PATH` or named with `XC16_DIR`. |
| GNU **make** + a POSIX `sh` | The Makefile is a standard GNU makefile.  On Windows, point `SHELL` at a `sh.exe` (for example the one shipped with Git for Windows). |
| **Git** | The Makefile bakes `git rev-parse --short HEAD` into the banner as `VERSION`.  A non-git source copy builds with an empty version component. |
| (optional) **MPLAB-X** | The graphical IDE route; requires MPLAB-X and the XC16 plugin.  Download from Microchip's [MPLAB-X page](http://www.microchip.com/mplab/mplab-x-ide). |
| (optional) **Python 3 + host GCC** | Only for regenerating message strings and for the hardware-free native tests. |

Obtaining XC16 is the one step that happens outside this repository:

1. Download **XC16** (not XC8 or XC32) for your platform from Microchip's
   [compiler page](http://www.microchip.com/mplab/compilers).
2. On Linux, make the installer executable and run it:
   `chmod +x xc16-*.run && ./xc16-*.run`.
3. At the edition prompt select the **free** edition.
4. Let the installer add XC16 to `PATH`; if you skip that option, either
   select the toolchain in MPLAB-X (Embedded → Build Tools → Scan for Build
   Tools, or Add → the XC16 install root) or pass `XC16_DIR` to the make
   command below.

The XC16 version is pinned in two places: the MPLAB-X project
(`Firmware/busPirate.X/nbproject/configurations.xml`, both configurations) and
the Makefile's installer-default fallback both name **v2.10**; the release
notes (`CHANGELOG.md`) still state the minimum as XC16 **1.35 or later** and
MPLAB-X **5.00 or later**.  That the current tree builds and links with 2.10
was not exercised for this page — **not verified** on a build.

## Command-line build

The Makefile lives at `Firmware/Makefile` and is the primary, IDE-free build.
Run it from the repository root with `-C Firmware`, or from inside `Firmware/`.

```
# v4 image
make -C Firmware BP_VERSION=V4          # -> Firmware/bpv4.hex

# v3 image
make -C Firmware BP_VERSION=V3          # -> Firmware/bpv3.hex

# clean: remove the whole build/ tree (both targets) plus this target's
# .elf/.hex/.map
make -C Firmware BP_VERSION=V4 clean
```

`BP_VERSION` defaults to `V4`.  Any value other than `V3` / `V4` stops the
build with `BP_VERSION must be V3 or V4`.  `clean` removes `build/` entirely
(so a later build of the other board recompiles everything) and deletes only
the selected target's `bpvN.elf`, `bpvN.hex` and `bpvN.map`; `distclean` is
defined as the same recipe.

### Selecting the toolchain and options

| Variable | Meaning | Default |
|---|---|---|
| `BP_VERSION` | `V3` or `V4` board build | `V4` |
| `XC16_DIR` | XC16 install root; used as `$(XC16_DIR)/bin/xc16-` | installer defaults, else `PATH` |
| `OPT` | optimisation level | `-O1` |
| `EXTRA_CFLAGS` | extra compiler flags (also the way to pass a preprocessor define) | empty |
| `SAVE_TEMPS` | when set, add `-save-temps=obj` (keeps `.i`/`.s` under `build/`) | unset |
| `HEAP_SIZE` / `STACK_SIZE` | override the linker heap/stack | unset |
| `SHELL` | the shell make uses for recipes | environment default |

Windows example (Git for Windows `sh`, explicit XC16 path):

```powershell
make -C Firmware BP_VERSION=V4 'SHELL=C:/Program Files/Git/usr/bin/sh.exe' 'XC16_DIR=C:/Program Files (x86)/Microchip/MPLABXC16/v2.10'
make -C Firmware BP_VERSION=V3 'SHELL=C:/Program Files/Git/usr/bin/sh.exe' 'XC16_DIR=C:/Program Files (x86)/Microchip/MPLABXC16/v2.10'
```

## Feature sets

Which functions are compiled in is decided in `Firmware/configuration.h`, not
by a make variable.  The chain is: `BP_VERSION` picks the device, `-mcpu`
defines the PIC macro, `configuration.h` turns that into `BUSPIRATEV3` or
`BUSPIRATEV4`, and one of three macro blocks then selects the feature switches.

| Block | Selected when | Effect |
|---|---|---|
| `BUSPIRATEV4` default | v4 build, `BP_CUSTOM_FEATURE_SET` not defined | the shipped v4 set (see table) |
| `BUSPIRATEV3` default | v3 build, `BP_CUSTOM_FEATURE_SET` not defined | the shipped v3 set: no AVRISP, no keyboard, smaller buffers |
| `BP_CUSTOM_FEATURE_SET` | that macro is defined | one all-features list, written for v4 |

The custom block is not a menu — it is a single fixed list that enables every
`BP_ENABLE_*` module **except AVRISP-MKII**, including HD44780 and SMPS.  It is
only usable on v4: the SMPS module raises a compile-time error on v3, so a v3
build with the custom block stops at the `#error` in `configuration.h`.

Per-module switches and their default state:

| Switch | v3 default | v4 default |
|---|---|---|
| `BP_ENABLE_1WIRE_SUPPORT` | on | on |
| `BP_ENABLE_BASIC_SUPPORT` | on | on |
| `BP_ENABLE_DIO_SUPPORT` | on | on |
| `BP_ENABLE_HD44780_SUPPORT` | off (optional) | off (optional) |
| `BP_ENABLE_I2C_SUPPORT` | on | on (and `BP_I2C_USE_HW_BUS`) |
| `BP_ENABLE_JTAG_SUPPORT` | on | on (XSVF sub-mode on v4 only) |
| `BP_ENABLE_PIC_SUPPORT` | on | on |
| `BP_ENABLE_PC_AT_KEYBOARD_SUPPORT` | off (optional) | on |
| `BP_ENABLE_RAW_2WIRE_SUPPORT` | on | on |
| `BP_ENABLE_RAW_3WIRE_SUPPORT` | on | on |
| `BP_ENABLE_SMPS_SUPPORT` | off (not available) | off (optional) |
| `BP_ENABLE_SPI_SUPPORT` | on | on |
| `BP_ENABLE_SUMP_SUPPORT` | on | on |
| `BP_ENABLE_UART_SUPPORT` | on | on |
| `BP_ENABLE_AVRISP_MKII_SUPPORT` | not available | on |

Guards that reject a wrong-board combination:

- AVRISP-MKII: `#error` unless the build is v4.
- SMPS: `#error` on v3.
- The XSVF sub-mode is enabled with JTAG on v4 only (memory); the OpenOCD
  adapter is enabled on both builds.

### Worked custom build: SMPS on v4

1. Edit `Firmware/configuration.h` in the `BUSPIRATEV4` default block: change
   `#undef BP_ENABLE_SMPS_SUPPORT` to `#define BP_ENABLE_SMPS_SUPPORT`.
2. Build the normal v4 image: `make -C Firmware BP_VERSION=V4`.
3. Flash `Firmware/bpv4.hex` ([flashing.md](flashing.md)); the SMPS binary
   commands are then compiled in.

For the all-features list instead of one switch, define `BP_CUSTOM_FEATURE_SET`
without editing the header — the Makefile appends `EXTRA_CFLAGS` to the
compiler flags:

```
make -C Firmware BP_VERSION=V4 EXTRA_CFLAGS=-DBP_CUSTOM_FEATURE_SET
```

- The all-features v4 list also enables the HD44780 adapter.  Its SPI output
  helper has no external definition when SPI support is also enabled (the
  `spi.c` implementation is `static`), so that combination is expected to fail
  at link time; `unverified` — see
  [the LCD set](../capabilities/lcd/README.md).
- A custom build changes the mode numbering in the `m` menu.  The firmware's
  own menu is authoritative, not the default table.

## What the build does

1. Selects the device, target name and linker script from `BP_VERSION`:
   v4 → `24FJ256GB106` / `bpv4` / `p24FJ256GB106.gld`; v3 → `24FJ64GA002` /
   `bpv3` / `p24FJ64GA002.gld`.
2. Compiles the shared source list plus the per-board set (`core_usb/cdc.c` +
   `core_usb/usb_stack.c` + `core_eeprom/onboard_eeprom.c` +
   `core_messages/messages_v4.s` for v4; `core_messages/messages_v3.s` +
   `tool_openocd/openocd_asm.s` for v3).
3. Archives objects under `build/<target>/` so v3 and v4 objects never mix.
4. Links with `--gc-sections` (unused sections and strings are dropped) and
   `--report-mem`, writing the map file.
5. Converts the ELF to Intel HEX with `xc16-bin2hex -a`.

## Artifacts

All paths are relative to the repository root; the Makefile runs in
`Firmware/`, so its products land there.

| Artifact | v4 | v3 |
|---|---|---|
| Intel HEX (flash this) | `Firmware/bpv4.hex` | `Firmware/bpv3.hex` |
| ELF | `Firmware/bpv4.elf` | `Firmware/bpv3.elf` |
| Linker map | `Firmware/bpv4.map` | `Firmware/bpv3.map` |
| Objects / intermediates | `Firmware/build/bpv4/` | `Firmware/build/bpv3/` |

`Firmware/.gitignore` ignores `build/`, `bpv3.*` and `bpv4.*`, so a clean
checkout has none of these present.

## MPLAB-X route

The IDE project is `Firmware/busPirate.X/` (NetBeans-style project `busPirate`
with two configurations).

1. File → Open Project → `Firmware/busPirate.X`.
2. Pick `BusPirate_v3` or `BusPirate_v4` in the configuration selector.
3. Build (the hammer).  A *production* (clean build) is what release artifacts
   come from.

The IDE writes its products under the project's distribution directory, by the
MPLAB-X / NetBeans convention
`Firmware/busPirate.X/dist/<config>/production/busPirate.X.production.hex`.
The project requests `output-file-format = elf`, so an `.elf` and map are
produced alongside.

- The project files do not override the IDE's `dist/` default, but the exact
  directory name is generated by MPLAB-X, not tracked here — **not verified**
  on a build.  The command-line Makefile is the path tracked in the
  repository.

The Makefile deliberately mirrors the compiler and linker options stored in
`Firmware/busPirate.X/nbproject/configurations.xml` (`-finline`, `-std=gnu99`,
the small scalar model, `--gc-sections`, memory report, `-save-temps`) so both
routes produce equivalent images.

## Message strings

User-visible terminal strings are authored as tab-separated rows in
`tools/packstrings/`:

| Source | Content |
|---|---|
| `bus_pirate_common_strings.txt` | strings shared by v3 and v4 |
| `bus_pirate_v3_strings.txt` | v3-only strings (includes the common file) |
| `bus_pirate_v4_strings.txt` | v4-only strings (includes the common file) |

`packstrings.py` turns a source file into a generated `.s` (the packed data)
and `.h` (one `void <id>_str(void)` plus a `<id>` macro per string).  The
generated files live in `Firmware/core_messages/` and are checked in;
regeneration is only needed after editing a `*_strings.txt`.  The Makefile
compiles `core_messages/messages_v3.s` or `core_messages/messages_v4.s`
(one per board) and `core_messages/messages.h` includes the matching header.

```
cd tools/packstrings
python packstrings.py bus_pirate_v3_strings.txt ../../Firmware/core_messages/messages_v3 v3
python packstrings.py bus_pirate_v4_strings.txt ../../Firmware/core_messages/messages_v4 v4
```

The third argument is the header include-guard tag
(`v3` → `BP_MESSAGES_V3_H`).  Run the generator **from `tools/packstrings/`**
because the output prefixes above are relative to that directory.  The
`#include bus_pirate_common_strings.txt` line inside the version files is
resolved relative to the including input file (not the process working
directory), so the version file itself may be given by any path.

- If the output base names the wrong directory, the generator writes copies
  that the Makefile never compiles: the edit appears to succeed and the image
  does not change.  Keep the `core_messages/` prefix.

## Native (hardware-free) tests

`Firmware/tests/` compiles real production functions against small mocks with
a host compiler; nothing here touches a board or emulates USB/timing.

```
python Firmware/tests/test_native.py
```

`test_native.py` is **only** the three shared-machinery harnesses:
`test_protocol_access.py` (mode-descriptor accessors), `test_shared_format.py`
(the decimal formatter and parser) and `test_messages.py` (the packed-message
reader on both address models).  Each needs Python 3 and `gcc` on `PATH`;
generated test files stay under `Firmware/build/`.

The module suites (core system, USB, bitbang, message catalog, EEPROM, PIC,
AVRISP and the two XC16 gates) are separate scripts run directly.  The
authoritative table, with coverage and runner per suite, is
[tooling.md](tooling.md) §Test suites.

## Common build errors

| Symptom | Cause | Fix |
|---|---|---|
| `BP_VERSION must be V3 or V4 (got '...')` | misspelled `BP_VERSION` | use `V3` or `V4` |
| `xc16-gcc: command not found` (or `.../bin/xc16-gcc` missing) | XC16 not on `PATH` and not found at the installer defaults | install XC16 or pass `XC16_DIR=/path/to/xc16/vX.Y` |
| `make: .../sh: No such file or directory` (Windows) | GNU make has no POSIX shell | pass `'SHELL=C:/Program Files/Git/usr/bin/sh.exe'` |
| `The AVRISP-MKII personality requires Bus Pirate v4 hardware.` | AVRISP enabled in a v3 build | remove `BP_ENABLE_AVRISP_MKII_SUPPORT` for v3 |
| `SMPS support is not available on Bus Pirate v3` | SMPS enabled in a v3 build (the custom block does this) | use a v4 build for SMPS |
| Empty `<VERSION>` in the `i` banner | built from a non-git copy | build from a git checkout, or accept an empty hash |
| v3 link fails to fit / region overflow | v3 is a 64 KB part | build v3 at the default `-O1`; the default feature set is chosen to fit |

## Verification

- Not run for this page; see [verification.md](verification.md) for the
  maintainer record.

## Limits & out of scope

- The build produces no signed or packaged release; see
  [tooling.md](tooling.md) §Release assembly.
- The exact MPLAB-X output directory is IDE-generated (see above); only the
  command-line artifacts are tracked.
