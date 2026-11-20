# Dev docs — Phase F delta audit

**Verdict: No.** The set still describes the pre-repair Makefile, the old
Windows XC16 requirement and loader-build caveat, the old ds30 guard, and the
4.12 v4 loader; the DIO verification row still has the five-pin scope.

Verified against the worktree's final tree (`Firmware/` and `Bootloaders/`,
combined `9b179cb5`), not external PR worktrees. No board was used.

## Findings

### 1. [major] Windows XC16 is documented as not auto-detected and as requiring `XC16_DIR`

`docs/dev/building.md:25`:

> The Windows fallback under `C:/Program Files (x86)` is not matched: the
> Makefile's `$(wildcard ...)` path is unquoted and splits at the space.  Pass
> `XC16_DIR` explicitly on Windows.

The same stale statement is repeated at `building.md:45-46` ("the Windows path
is not auto-detected"), `:78` ("pass it explicitly on Windows"), `:84-90`
(the Windows example says "XC16 path given explicitly"), and `:310` ("the
Windows installer default is not auto-detected"). `docs/dev/README.md:47-50`
also tells the shortest-path reader to "pass `XC16_DIR` explicitly."

The final `Firmware/Makefile:43-53` now probes the Windows default with the
space-safe pattern `C:/Program?Files?(x86)/Microchip/MPLABXC16/v2.10/bin` and
sets `XC16_DIR` to the real path. With no `XC16_DIR`, `make -p` resolves to
`C:/Program Files (x86)/Microchip/MPLABXC16/v2.10`, and a clean v4 build
completed at 90,381 bytes program / 12,626 data.

**Fix:** remove the "not matched/not auto-detected/pass explicitly" text.
State that `/opt/...` and the Windows installer default are auto-detected,
that `PATH` is the fallback, and that `XC16_DIR` is only an optional override
for a non-default install. Update the Windows example to omit it (or label it
optional), and change the error-table row to "XC16 not on PATH and not at the
installer default; pass `XC16_DIR` if installed elsewhere."

### 2. [major] The flag-stamp behaviour is documented backwards

`docs/dev/building.md:157-170`:

> Changing only `EXTRA_CFLAGS` does not invalidate the existing objects: the
> Makefile has no dependency on it, so after a default build the custom
> command prints `Nothing to be done` and leaves the old image.  Clean first,
> then build:
>
> `make -C Firmware BP_VERSION=V4 clean`
> `make -C Firmware BP_VERSION=V4 EXTRA_CFLAGS=-DBP_CUSTOM_FEATURE_SET`
>
> The same applies to any change that only affects compiler flags (`OPT`,
> `SAVE_TEMPS`).

The final `Firmware/Makefile:130-131,207-228` stores the effective `CFLAGS`
in `build/<target>/.build-flags`, makes every object depend on it, and rewrites
the stamp only when the flags change. A default build followed directly by
`EXTRA_CFLAGS=-DBP_CUSTOM_FEATURE_SET` recompiled and linked the custom image
(79,734 bytes program / 11,898 data); an unchanged second run did no work. No
`clean` is needed.

**Fix:** replace this passage with: the flag stamp makes a flag-only change
recompile all objects, while an unchanged rebuild stays a no-op; show the
custom build without `clean` and retain only the map/compile-line check.

### 3. [major] The Windows `pirate-loader` source build is still documented as unverified and broken

`docs/dev/flashing.md:94-98`:

> Windows: run the same `cmake ..` in the build directory, then open the
> generated `pirate-loader.sln` in Visual Studio and build.  That route is
> **unverified**: the CMake project sets GCC-style linker flags
> (`-static -static-libgcc`), so confirm the link.  A MinGW build of the
> checked-in project does not compile its Windows serial wrappers.

`docs/dev/README.md:52-53` repeats: "on Windows the Visual Studio route is
unverified."

The final `Bootloaders/pirate-loader/CMakeLists.txt:21-36` selects `/MT` for
MSVC and `-static`/`-static-libgcc` only for GCC/Clang; `pirate-loader.c` uses
`#if defined(_WIN32)` for the Windows wrappers. MinGW and MSVC source builds
are the verified builds called out by the #214 repair.

**Fix:** replace the Windows paragraph with the verified source route (CMake
with MinGW Makefiles or Visual Studio; compiler-appropriate static flags), and
remove the "unverified/GCC-only/MinGW does not compile" sentences. Keep the
prebuilt binary's port-speed caveat unless that binary is independently
verified.

### 4. [major] The v4 loader version and erased-application behaviour are stale

`docs/dev/bootloaders.md:82`:

> **4.12** (`VER_H = 0x04`, `VER_L = 0x0c`).

`docs/dev/flashing.md:160` expects:

> `Bootloader version: 4,12`

The final `Bootloaders/BPv4-bootloader/firmware-v1/bootloader.c:22-23` has
`VER_L 0x0e`, so the hello reply is **4,14**. The final `main.c:127-138` also
checks the application reset vector before jumping:

> `if (__builtin_tblrdl(APPLICATION_VECTOR_BYTE) != 0xFFFF) { ... goto 0x2000 ... }`

`docs/dev/bootloaders.md:103-107` still says the opposite:

> The loader does not validate the application image. ... a blank-signature
> board with PGD high and the button released jumps to `0x2000` even if that
> region is erased.

The loader now stays in the bootloader when the reset vector is erased; it
only skips the "fully validate" claim, not the reset-vector check. Line 137's
"4.12 erase-guard" name is also stale.

**Fix:** change the version to **4.14** (`VER_L = 0x0e`) in
`bootloaders.md:82`, `bootloaders.md:137`, and `flashing.md:160`; change the
blank-application warning to say the loader checks only the application reset
vector (an erased `0xFFFF` does not jump; a corrupt non-empty vector still
can), and keep the NORMAL-button/jumper advice for a suspect image.

### 5. [minor] The v3 ds30 guard is not described at the new threshold

`docs/dev/bootloaders.md:73-75`:

> The running loader refuses to write its own page: a write that starts in
> the loader page is rejected with `'P'`.

The final `Bootloaders/BPv3-bootloader/firmware-v4.5/ds30loader.s:146-149`
uses:

> `BLCHECKST, ( STARTADDR - (ROWSIZE*2) )`

with `STARTADDR = 0xA800`, so `BLCHECKST` is `0xA780`. The last safe row
starts at `0xA780`; a row starting above it (including `0xA7C0`) ends at or
past `0xA800` and is refused with `'P'`. The current sentence covers only
writes that start at `0xA800` or above and misses the 64-word gap this repair
closed.

**Fix:** state the exact boundary: "the last safe row starts at `0xA780`; a
row whose write reaches `0xA800` or beyond is rejected with `'P'`."

### 6. [minor] The DIO verification row still claims a five-pin park for both boards

`docs/dev/verification.md:42`:

> read-back (`r` bits 4–0); the safe-park contract covers the five signal
> pins, not `AUX1`/`AUX2`; v3 unverified

The final `Firmware/protocol_dio/dio.c:45-57` builds `AUX | MOSI | CLK | MISO
| CS` and adds `AUX1 | AUX2` on v4; `docs/dev/verification.md` therefore has
the v4 scope wrong.

**Fix:** "the safe-park contract covers five pins on v3 and seven on v4
(`AUX1`/`AUX2` included); v3 unverified."

## Cross-set list

| Item | Page / owner | Action |
|---|---|---|
| DIO safe-park scope | `docs/dev/verification.md:42` (dev owns; describes `docs/protocol/dio/`) | Change to five pins v3 / seven pins v4 including `AUX1`/`AUX2`; v3 unverified. No user-page edit is required. |

`tooling.md` and `code-map.md` contain no #203/#214 build or bootloader claim
that needs changing; the changed claims are in `building.md`, `flashing.md`,
`bootloaders.md`, `README.md` and `verification.md`.

## Presentation

- No TAB characters in the changed `docs/dev` passages.
- Tables render as tables; the changed long rows are table cells, not prose
  wraps. All non-table prose in `docs/dev` remains at or below 88 columns.
- No dates, bench narratives, pass counts, evidence-section provenance or
  internal log names in the changed passages.
- The v4 version table row and the DIO verification row are the only changed
  table content that need the fixes above; no alignment problem.
