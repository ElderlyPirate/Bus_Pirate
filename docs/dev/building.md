# Building the firmware

**Status:** draft.  Confirmed from the tracked `Firmware/Makefile`, the
MPLAB-X project files, the message tooling and the native tests in the
repository tip.

This page covers how to compile the Bus Pirate v3 / v4 community firmware and
which artifacts are produced.  Flashing the result is
[flashing.md](flashing.md).

## Scope

- **In:** toolchain prerequisites, the command-line `make` build, the MPLAB-X
  route, artifact locations, message-string generation, the hardware-free
  native tests, and the common build errors that follow from these files.
- **Out:** the option-by-option meaning of the firmware itself, and the
  configuration flags in `Firmware/configuration.h` (see the per-function
  pages and code map).

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

The XC16 version named by the Makefile's installer-default fallback is
**v2.10**.  The release notes (`CHANGELOG.md`) state XC16 **1.35 or later**
and MPLAB-X **5.00 or later**.  The exact XC16 version that builds the
current tree is **Requires verification** — no build was run for this page.

## Command-line build

The Makefile lives at `Firmware/Makefile` and is the primary, IDE-free build.
Run it from the repository root with `-C Firmware`, or from inside `Firmware/`.

```
# v4 image
make -C Firmware BP_VERSION=V4          # -> Firmware/bpv4.hex

# v3 image
make -C Firmware BP_VERSION=V3          # -> Firmware/bpv3.hex

# remove build products for the selected version
make -C Firmware BP_VERSION=V4 clean
```

`BP_VERSION` defaults to `V4`.  Any value other than `V3` / `V4` stops the
build with `BP_VERSION must be V3 or V4`.

### Selecting the toolchain and options

| Variable | Meaning | Default |
|---|---|---|
| `BP_VERSION` | `V3` or `V4` board build | `V4` |
| `XC16_DIR` | XC16 install root; used as `$(XC16_DIR)/bin/xc16-` | installer defaults, else `PATH` |
| `OPT` | optimisation level | `-O1` |
| `EXTRA_CFLAGS` | extra compiler flags | empty |
| `SAVE_TEMPS` | when set, add `-save-temps=obj` (keeps `.i`/`.s` under `build/`) | unset |
| `HEAP_SIZE` / `STACK_SIZE` | override the linker heap/stack | unset |
| `SHELL` | the shell make uses for recipes | environment default |

Windows example (Git for Windows `sh`, explicit XC16 path):

```powershell
make -C Firmware BP_VERSION=V4 'SHELL=C:/Program Files/Git/usr/bin/sh.exe' 'XC16_DIR=C:/Program Files (x86)/Microchip/MPLABXC16/v2.10'
make -C Firmware BP_VERSION=V3 'SHELL=C:/Program Files/Git/usr/bin/sh.exe' 'XC16_DIR=C:/Program Files (x86)/Microchip/MPLABXC16/v2.10'
```

### What the build does

1. Selects the device, target name and linker script from `BP_VERSION`:
   v4 → `24FJ256GB106` / `bpv4` / `p24FJ256GB106.gld`; v3 → `24FJ64GA002` /
   `bpv3` / `p24FJ64GA002.gld`.
2. Compiles the shared source list plus the per-board set (`dp_usb/` + the USB
   stack + `onboard_eeprom.c` + `messages_v4.s` for v4; `messages_v3.s` +
   `openocd_asm.s` for v3).
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
MPLAB-X convention
`Firmware/busPirate.X/dist/<config>/production/busPirate.X.production.hex`.
The project requests `output-file-format = elf`, so an `.elf` and map are
produced alongside.

- **Requires verification:** the exact IDE output filename/location.  The
  command-line Makefile is the path verified from the repository; the MPLAB-X
  directory layout follows the IDE convention, not a file tracked here.

The Makefile deliberately mirrors the compiler and linker options stored in
`Firmware/busPirate.X/nbproject/configurations.xml` so both routes produce
equivalent images.

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
generated files `Firmware/messages_v3.{s,h}` and `Firmware/messages_v4.{s,h}`
are checked in; regeneration is only needed after editing a `*_strings.txt`.

```
cd tools/packstrings
python packstrings.py bus_pirate_v3_strings.txt ../../Firmware/messages_v3 v3
python packstrings.py bus_pirate_v4_strings.txt ../../Firmware/messages_v4 v4
```

The third argument is the header include-guard tag
(`v3` → `BP_MESSAGES_V3_H`).  Run the generator **from `tools/packstrings/`**
because the output prefixes above are relative to that directory.  The
`#include bus_pirate_common_strings.txt` line inside the version files is
resolved relative to the including input file (not the process working
directory), so the version file itself may be given by any path.

- **Requires verification:** the exact command lines above are derived from
  `packstrings.py`'s argument parsing and the `#include` lines; they are not
  recorded in a make target (the Makefile does not regenerate strings).

## Native (hardware-free) tests

`Firmware/tests/` compiles real production functions against small mocks with
a host compiler; nothing here touches a board or emulates USB/timing.

```
python Firmware/tests/test_native.py
```

`test_native.py` runs three harnesses in sequence:
`test_protocol_access.py` (mode-descriptor accessors), `test_shared_format.py`
(the decimal formatter and parser) and `test_messages.py` (the packed-message
reader on both address models).  Each needs Python 3 and `gcc` on `PATH`;
generated test files stay under `Firmware/build/`.

`Firmware/tests/` also contains AVRISP-specific harnesses (`test_avrisp.py`,
`test_avrisp_board_gate.py`, `test_avrisp_cdc_gate.py`, plus
`avrisp_mock.h` / `avrisp_test.c`) which are run by their own gate scripts, not
by `test_native.py`.

## Common build errors

| Symptom | Cause | Fix |
|---|---|---|
| `BP_VERSION must be V3 or V4 (got '...')` | misspelled `BP_VERSION` | use `V3` or `V4` |
| `xc16-gcc: command not found` (or `.../bin/xc16-gcc` missing) | XC16 not on `PATH` and not found at the installer defaults | install XC16 or pass `XC16_DIR=/path/to/xc16/vX.Y` |
| `make: .../sh: No such file or directory` (Windows) | GNU make has no POSIX shell | pass `'SHELL=C:/Program Files/Git/usr/bin/sh.exe'` |
| Empty `<VERSION>` in the `i` banner | built from a non-git copy | build from a git checkout, or accept an empty hash |
| v3 link fails to fit / region overflow | v3 is a 64 KB part | build v3 at the default `-O1`; the default feature set is chosen to fit |

## Verification

- Not run for this page; see [verification.md](verification.md) for the
  maintainer record.

## Limits & out of scope

- The per-module `BP_ENABLE_*_SUPPORT` switches in
  `Firmware/configuration.h` and their effect on the image are not documented
  here.
- The build produces no signed or packaged release; see
  [tooling.md](tooling.md) §Packaging.