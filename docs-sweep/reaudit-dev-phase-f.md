# Dev docs — Phase F re-audit

**Scope:** the six findings in `feedback-dev-phase-f.md`, checked against the
current `docs/dev/**` and the final tree in `bpv4-docssweep/Firmware/`,
`Bootloaders/` and `Firmware/Makefile` (`9b179cb5`).

**Verdict: 6 of 6 filled in full.** The set is clean for the audited delta.

## Per-item table

| # | Status | Current evidence | Remainder |
|---|---|---|---|
| 1 | **Filled in full** | `building.md:25`: "The Makefile auto-detects the installer defaults `/opt/microchip/xc16/v2.10` and `C:/Program Files (x86)/Microchip/MPLABXC16/v2.10`, then falls back to `PATH`.  Set `XC16_DIR` only for a non-default install root." `README.md:34-35` and `:48-52` now use the auto-detected default; the Windows example at `building.md:83-90` omits `XC16_DIR`. | None. This matches `Firmware/Makefile:43-53` and the `make -p` result with no override (`XC16_DIR := C:/Program Files (x86)/Microchip/MPLABXC16/v2.10`). |
| 2 | **Filled in full** | `building.md:157-166`: "The per-target `build/<target>/.build-flags` stamp records the effective flag set: a flag-only change (`EXTRA_CFLAGS`, `OPT`, `SAVE_TEMPS`) rewrites the stamp and recompiles every object, while a rebuild with unchanged flags does not recompile or relink.  No `clean` is needed." The custom command has no `clean`; `building.md:172-173` records **79,734 bytes program / 11,898 bytes data**. | None. This matches the `FLAG_STAMP` rules in `Firmware/Makefile:130-131,207-228`; a default build followed by `EXTRA_CFLAGS=-DBP_CUSTOM_FEATURE_SET` recompiled, and an unchanged rerun did no work. |
| 3 | **Filled in full** | `flashing.md:95-99`: "Windows: … use the MinGW Makefiles generator (`cmake -G "MinGW Makefiles" ..`, then `make`), or run the Visual Studio generator and open the generated `pirate-loader.sln` to build.  CMake selects compiler-appropriate static flags: `-static -static-libgcc` for GCC, `-static` for Clang, and `/MT` for MSVC." `README.md:54-56` names the MinGW Makefiles or Visual Studio route. | None for the docs. This matches `Bootloaders/pirate-loader/CMakeLists.txt:21-36` and the `#if defined(_WIN32)` path in `pirate-loader.c`. The prebuilt bundle remains correctly labelled as old release artifacts with an unverified port speed. |
| 4 | **Filled in full** | `bootloaders.md:82-83`: "**4.14** (`VER_H = 0x04`, `VER_L = 0x0e`)"; `bootloaders.md:90-96` now requires "the application reset vector word is not erased"; `bootloaders.md:105-109` says the loader "checks only the first word of the application reset vector at `0x2000`", an erased `0xFFFF` word stays in the loader, and a corrupt non-erased vector can still be entered. `bootloaders.md:138-139` calls it the "4.14 erase-guard" loader; `flashing.md:69-73` and `:161` agree and expect `Bootloader version: 4,14`. | None. This matches `Bootloaders/BPv4-bootloader/firmware-v1/bootloader.c:22-23` (`VER_L 0x0e`) and `main.c:127-138` (`APPLICATION_VECTOR_BYTE` check). |
| 5 | **Filled in full** | `bootloaders.md:73-76`: "The running loader refuses writes that would reach its own page: the last safe row starts at `0xA780`; a row whose write reaches `0xA800` or beyond is rejected with `'P'`." | None. This matches `Bootloaders/BPv3-bootloader/firmware-v4.5/ds30loader.s:146-149` (`BLCHECKST = STARTADDR - (ROWSIZE*2)`, `STARTADDR = 0xA800`). |
| 6 | **Filled in full** | `verification.md:42`: "the safe-park contract covers five pins on v3 and seven on v4 (`AUX1`/`AUX2` included); v3 unverified". | None. This matches `Firmware/protocol_dio/dio.c:45-57` (the base five-pin mask plus `AUX1 | AUX2` under `BUSPIRATEV4`). |

## Presentation and links

- No TAB characters in `docs/dev/`.
- No non-table prose line over 88 columns; the changed table rows are table
  cells, not broken wraps.
- No dates, bench narratives, pass counts, capture provenance or internal log
  names in the changed passages. The all-features byte result is a build fact,
  not a pass count.
- All relative links in `docs/dev/` resolve, and all file anchors resolve.
- Tables are intact; no misaligned capture or option-list problem.

## Notes

The only residual is the already-recorded MSVC environment caveat: the CMake
selects `/MT` and the Visual Studio route is documented, but no Visual Studio
was available here to repeat the link locally. That is a verification
limitation, not a stale docs claim. No hardware was attached; the v4 4.14
vector guard and the v3 `0xA780` guard remain source-derived.
