# Building the firmware

**Status:** maintainer-facing.  Derived from the tracked `Firmware/Makefile`,
`Firmware/configuration.h`, the MPLAB-X project files, the message tooling and
the native suites.  The v4 all-features link result is recorded below.

This page covers how to compile the Bus Pirate v3 / v4 community firmware,
which feature set goes into the image, and which artifacts are produced.
Flashing the result is [flashing.md](flashing.md).

## Scope

- **In:** toolchain prerequisites, the command-line `make` build, the feature
  sets in `Firmware/configuration.h`, the MPLAB-X route, artifact locations,
  message-string generation, and the common build errors that follow from
  these files.
- **Out:** what each function does with its feature switch (the per-function
  pages), and the test inventory — the authoritative suite table is
  [tooling.md](tooling.md) §Test suites.

## Prerequisites

| Requirement | Detail |
|---|---|
| Microchip **XC16** | The PIC24 C compiler and linker.  The Makefile auto-detects the installer defaults `/opt/microchip/xc16/v2.10` and `C:/Program Files (x86)/Microchip/MPLABXC16/v2.10`, then falls back to `PATH`.  Set `XC16_DIR` only for a non-default install root. |
| GNU **make** + POSIX shell utilities | The Makefile is a standard GNU makefile; its recipes call `sh`, `date`, `mkdir` and `rm`.  On Windows, use a native MinGW GNU make (for example the scoop `mingw` package) and put Git for Windows' `usr\bin` first on `PATH`.  A command-line `SHELL=` assignment is not honoured by that make; `sh.exe` on `PATH` is what it uses. |
| **Git** | The Makefile bakes `git rev-parse --short HEAD` into the banner as `VERSION`.  A non-git source copy builds with an empty version component. |
| (optional) **MPLAB-X** | The graphical IDE route; requires MPLAB-X and the XC16 plugin.  Download from Microchip's [MPLAB-X page](http://www.microchip.com/mplab/mplab-x-ide). |
| (optional) **Python 3 + host GCC** | Only for regenerating message strings and for the hardware-free native tests. |

Obtaining XC16 is the one step that happens outside this repository:

1. Download **XC16** (not XC8 or XC32) for your platform from Microchip's
   [compiler page](http://www.microchip.com/mplab/compilers).
2. On Linux, make the installer executable and run it:
   `chmod +x xc16-*.run && ./xc16-*.run`.
3. At the edition prompt select the **free** edition.
4. The Makefile auto-detects the installer default.  For a different install
   root, either select it in MPLAB-X (Embedded → Build Tools → Scan for Build
   Tools, or Add → the XC16 install root) or pass `XC16_DIR` to the make
   command below.

The XC16 version is pinned in two places: the MPLAB-X project
(`Firmware/busPirate.X/nbproject/configurations.xml`, both configurations) and
the Makefile's auto-detected installer defaults both name **v2.10**.  The
release notes (`CHANGELOG.md`) still state the minimum as XC16 **1.35 or
later** and MPLAB-X **5.00 or later**.

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
| `XC16_DIR` | XC16 install root; used as `$(XC16_DIR)/bin/xc16-` | auto-detected `/opt/microchip/xc16/v2.10` or the Windows installer default, else `PATH`; override for a non-default install |
| `OPT` | optimisation level | `-O1` |
| `EXTRA_CFLAGS` | extra compiler flags (also the way to pass a preprocessor define) | empty |
| `SAVE_TEMPS` | when set, add `-save-temps=obj` (keeps `.i`/`.s` under `build/`) | unset |
| `HEAP_SIZE` / `STACK_SIZE` | override the linker heap/stack | unset |

Windows example (native MinGW `make`; Git for Windows `usr\bin` supplies the
POSIX utilities; the installer-default XC16 is found automatically):

```powershell
$env:Path = 'C:\Program Files\Git\usr\bin;' + $env:Path
make -C Firmware BP_VERSION=V4
make -C Firmware BP_VERSION=V3
```

Add `'XC16_DIR=C:/path/to/xc16'` only if XC16 is installed outside the
installer default.  Use the native MinGW GNU make (for example the scoop
`mingw` package); a Cygwin or MSYS make uses a different path model.  Do not
add a `SHELL=` assignment: this make ignores it, and `sh.exe` on `PATH` is
what the recipes use.

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

The omission of AVRISP is deliberate in the current tree: the custom list has
no `BP_ENABLE_AVRISP_MKII_SUPPORT`.  To include the programmer in a custom v4
build, add that switch to the `BP_CUSTOM_FEATURE_SET` block.  The v4
all-features link result below therefore excludes AVRISP-MKII.  See
[avrisp-design-notes.md](avrisp-design-notes.md) §Build selection.

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
compiler flags.  The per-target `build/<target>/.build-flags` stamp records
the effective flag set: a flag-only change (`EXTRA_CFLAGS`, `OPT`,
`SAVE_TEMPS`) rewrites the stamp and recompiles every object, while a rebuild
with unchanged flags does not recompile or relink.  No `clean` is needed:

```
make -C Firmware BP_VERSION=V4 EXTRA_CFLAGS=-DBP_CUSTOM_FEATURE_SET
```

- The all-features v4 list enables both the HD44780 adapter and SPI.  That
  combination links: the LCD driver uses its own bounded byte helper
  (`hd44780_spi_write_byte()` in `capability_hd44780/hd44780.c`) and the SPI
  family exports `spi_write_byte()`.
- The v4 all-features build with XC16 2.10 at `-O1` links at **79,734 bytes
  program (31%)** and **11,898 bytes data (72%)**, with no warnings.
- To confirm the feature set took effect, the compile lines show
  `-DBP_CUSTOM_FEATURE_SET` and `Firmware/bpv4.map` contains `LCDsetup` and
  `smps_start`; the default v4 map contains neither.
- On v3 the custom set still stops at the SMPS `#error`; a v3 LCD image needs
  the LCD switch added to the v3 default set instead.  See
  [the LCD set](../capabilities/lcd/README.md).  The adapter revisions and
  the connector/supply path there come from the legacy `hardware-other` design
  files; the on-glass glyph and timing behaviour is code-derived.
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

A clean checkout has none of these present.  The repository `.gitignore`
covers MPLAB-X generated directories; the Makefile's `bpv3.*`, `bpv4.*` and
`build/` products are not ignored, so they appear as untracked files after a
command-line build.

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
  directory name is generated by MPLAB-X, not tracked here — **unverified**
  on a build.  The command-line Makefile is the path tracked in the
  repository.

The Makefile deliberately mirrors the compiler and linker options stored in
`Firmware/busPirate.X/nbproject/configurations.xml` (`-finline`, `-std=gnu99`,
the small scalar model, `--gc-sections`, memory report); `SAVE_TEMPS` adds the
project's `-save-temps`.

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
a host compiler; nothing here touches a board or emulates USB/timing.  Every
suite is a standalone script run from the repository root — there is **no
aggregate runner**, so a pass in one says nothing about the others.  For
example:

```
python Firmware/tests/test_core_system.py
```

Most suites need Python 3 and `gcc` on `PATH`; the two AVRISP XC16 gates
(`test_avrisp_board_gate.py`, `test_avrisp_cdc_gate.py`) need `xc16-gcc` (or
its path as the first argument).  Several fixtures read a revision with
`git show`, so run them from a git checkout.  Generated test files stay under
`Firmware/build/` or a temporary directory and are never linked into the
image.

The authoritative table, with coverage and runner per suite, is
[tooling.md](tooling.md) §Test suites.

## Common build errors

| Symptom | Cause | Fix |
|---|---|---|
| `BP_VERSION must be V3 or V4 (got '...')` | misspelled `BP_VERSION` | use `V3` or `V4` |
| `xc16-gcc: command not found` (or `.../bin/xc16-gcc` missing) | XC16 not on `PATH` and not at the installer default | install XC16, put it on `PATH`, or pass `XC16_DIR=/path/to/xc16/vX.Y` if installed elsewhere |
| `process_begin: CreateProcess(NULL, date ...) failed` or `mkdir ... failed` (Windows) | make cannot find the POSIX utilities the recipes call | put `C:\Program Files\Git\usr\bin` first on `PATH`; keep `SHELL` unset (see §Command-line build) |
| `The AVRISP-MKII personality requires Bus Pirate v4 hardware.` | AVRISP enabled in a v3 build | remove `BP_ENABLE_AVRISP_MKII_SUPPORT` for v3 |
| `SMPS support is not available on Bus Pirate v3 boards.` | SMPS enabled in a v3 build (the custom block does this) | use a v4 build for SMPS |
| Empty `<VERSION>` in the `i` banner | built from a non-git copy | build from a git checkout, or accept an empty hash |
| v3 link fails to fit / region overflow | v3 is a 64 KB part | build v3 at the default `-O1`; the default feature set is chosen to fit |

A v3 custom build trips both SMPS guards in `configuration.h`; the second
prints `SMPS support is not available on Bus Pirate v3` (without `boards.`).

## Verification

The v4 all-features link result is recorded above.  The per-set documentation
basis, including the areas that remain `unverified`, is in
[verification.md](verification.md).

## Limits & out of scope

- The build produces no signed or packaged release; see
  [tooling.md](tooling.md) §Release assembly.
- The exact MPLAB-X output directory is IDE-generated (see above); only the
  command-line artifacts are tracked.
- The v3 part is close to its program-flash limit; adding features may require
  dropping another module.
