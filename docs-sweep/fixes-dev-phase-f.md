# Dev docs — Phase F delta repair record

**Scope:** `docs/dev/**` only.  Firmware was not changed.
**Baseline:** `docs/combined-sweep` docs head `762eb8c5`; the `Firmware/` tree
is the final combined image `9b179cb5`.

**Verification performed:**

- Read `docs-sweep/feedback-dev-phase-f.md`, the dev row of
  `Bus_Pirate/PHASE-F-DOCS.md`, and the current `docs/dev/` pages.
- Verified the final `Firmware/Makefile`: with no `XC16_DIR` and no
  `xc16-gcc` on `PATH`, `make -C Firmware BP_VERSION=V4 -pn` resolves
  `XC16_DIR` to `C:/Program Files (x86)/Microchip/MPLABXC16/v2.10`.
- Verified the flag stamp in a temporary copy of `Firmware/`: a default v4
  build followed directly by `EXTRA_CFLAGS=-DBP_CUSTOM_FEATURE_SET`
  recompiled and linked the custom image; an unchanged rerun left the newest
  object, `.build-flags` and the ELF unchanged.  The worktree map, built with
  `-DVERSION=762eb8c5`, is 79,734 bytes program / 11,898 data and
  `.build-flags` contains `-DBP_CUSTOM_FEATURE_SET`.
- Built `Bootloaders/pirate-loader/` in a temporary copy with
  `cmake -G "MinGW Makefiles"` and `make`: `Built target pirate-loader`.
  Read the compiler-conditional static flags in `CMakeLists.txt` and the
  `#if defined(_WIN32)` path in `pirate-loader.c`.  MSVC itself was not
  available on this machine.
- Read the final `Bootloaders/BPv4-bootloader/firmware-v1/bootloader.c`
  (`VER_L 0x0e`), `main.c` (`APPLICATION_VECTOR_BYTE` check),
  `Bootloaders/BPv3-bootloader/firmware-v4.5/ds30loader.s`
  (`BLCHECKST = STARTADDR - (ROWSIZE*2)`) and
  `Firmware/protocol_dio/dio.c` (five-pin v3 / seven-pin v4 park mask).
- Link, anchor, TAB and prose-width checks pass on `docs/dev/`.  No board was
  attached; hardware behaviour remains source-derived.

## Findings

| # | Decision | File + quote | Evidence |
|---|---|---|---|
| 1 | fixed | `building.md`: "The Makefile auto-detects the installer defaults `/opt/microchip/xc16/v2.10` and `C:/Program Files (x86)/Microchip/MPLABXC16/v2.10`, then falls back to `PATH`.  Set `XC16_DIR` only for a non-default install root."  The Windows example drops `XC16_DIR`; the error row now reads "XC16 not on `PATH` and not at the installer default".  `README.md`: "from the auto-detected installer default or `PATH`". | Final `Firmware/Makefile` uses `$(wildcard C:/Program?Files?(x86)/Microchip/MPLABXC16/v2.10/bin)` and assigns the real path; `make -p` resolves `XC16_DIR` with no override. |
| 2 | fixed | `building.md`: "The per-target `build/<target>/.build-flags` stamp records the effective flag set: a flag-only change (`EXTRA_CFLAGS`, `OPT`, `SAVE_TEMPS`) rewrites the stamp and recompiles every object, while a rebuild with unchanged flags does not recompile or relink.  No `clean` is needed"; the custom command has no `clean`; the all-features result is **79,734 bytes program (31%)** / **11,898 bytes data (72%)**. | Final `Firmware/Makefile` defines `FLAG_STAMP`, makes every object depend on it, and rewrites it only when `CFLAGS` changes; the temporary build reproduced the recompile and the unchanged no-op; the worktree map confirms 79,734 / 11,898. |
| 3 | fixed | `flashing.md`: "Windows: from the same `build` directory, use the MinGW Makefiles generator (`cmake -G "MinGW Makefiles" ..`, then `make`), or run the Visual Studio generator and open the generated `pirate-loader.sln` to build.  CMake selects compiler-appropriate static flags: `-static -static-libgcc` for GCC, `-static` for Clang, and `/MT` for MSVC."  `README.md` step 3 names the MinGW Makefiles or Visual Studio route.  The prebuilt binaries are labelled as release artifacts from the older bundle. | Final `Bootloaders/pirate-loader/CMakeLists.txt` selects `/MT` for MSVC, `-static -static-libgcc` for GNU and `-static` for Clang; `pirate-loader.c` uses `#if defined(_WIN32)`; the MinGW build completed. |
| 4 | fixed | `bootloaders.md`: "**4.14** (`VER_H = 0x04`, `VER_L = 0x0e`)"; the entry block requires "the application reset vector word is not erased"; the blank warning says the loader "checks only the first word of the application reset vector at `0x2000`", an erased `0xFFFF` word stays in the bootloader, and a corrupt non-erased vector can still be jumped into; the "4.14 erase-guard" name follows.  `flashing.md`: `Bootloader version: 4,14`; the blank-v4 warning matches. | Final `bootloader.c` has `VER_L 0x0e`; final `main.c` defines `APPLICATION_VECTOR_ADDRESS 0x2000` and `APPLICATION_VECTOR_BYTE`, and checks `__builtin_tblrdl(APPLICATION_VECTOR_BYTE) != 0xFFFF` before the jump. |
| 5 | fixed | `bootloaders.md`: "the last safe row starts at `0xA780`; a row whose write reaches `0xA800` or beyond is rejected with `'P'`." | Final `ds30loader.s` has `BLCHECKST, ( STARTADDR - (ROWSIZE*2) )` with `STARTADDR = 0xA800` and `ROWSIZE = 64`; this closes the earlier CO-4 guard gap. |
| 6 | fixed | `verification.md`: "the safe-park contract covers five pins on v3 and seven on v4 (`AUX1`/`AUX2` included); v3 unverified". | Final `dio.c` builds `park_mask = AUX | MOSI | CLK | MISO | CS` and adds `AUX1 | AUX2` under `BUSPIRATEV4`. |

No finding was rejected.

**Prior code-owner items:** CO-1 (flag stamp), CO-2 (Windows XC16
wildcard), CO-3 (pirate-loader CMake flags) and CO-4 (v3 guard threshold)
from `fixes-dev-round1.md` are resolved in the final combined tree; findings
1, 2, 3 and 5 record the doc-side confirmation.

## Cross-set items

1. **DIO safe-park scope — resolved in `docs/dev/verification.md`.**  The
   audit's cross-set row owns this in the dev verification record, not in a
   user page.  `docs/protocol/dio/edge-cases.md` already states "`w` parks the
   five signal pins (v3) or seven pins (v4)", so no edit to
   `docs/protocol/dio/` is required.
2. **No other cross-set change.**  The audit states `tooling.md` and
   `code-map.md` contain no #203/#214 build or bootloader claim that needs
   changing.  Their current text is left untouched.

## Remaining questions

- **MSVC build.**  The CMake project selects `/MT` for MSVC and the audit
  records the MSVC route as verified; no Visual Studio was available here to
  repeat the link locally.
- **Hardware.**  No board was attached; the v4 4.14 vector guard, the v3
  `0xA780` guard and the bootloader entry procedures are source-derived.
- **All-features size.**  79,734 bytes is the worktree result with the
  `VERSION=762eb8c5` banner; a different banner hash shifts the program figure
  slightly, but the map/compile-line check remains the reliable confirmation.

## Files changed

- `docs/dev/README.md`
- `docs/dev/building.md`
- `docs/dev/flashing.md`
- `docs/dev/bootloaders.md`
- `docs/dev/verification.md`
- `docs-sweep/fixes-dev-phase-f.md` (this record)

## Publication credit

- Added `## Credits` to `docs/dev/flashing.md` for the #215 reuse decision:
  the bootloader-entry correction and the `$`/bootloader-version caveat came
  from Tom Jennings (`tomwj`, commit `e6eaaf29` on his fork).  The rewritten
  flashing and bootloader pages retain the correction.  No code change.
