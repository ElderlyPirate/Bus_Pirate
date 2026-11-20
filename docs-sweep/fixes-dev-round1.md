# Dev docs — round 1 implementation record

**Scope:** `docs/dev/**` only.  Firmware was not changed.
**Baseline:** `docs/combined-sweep` at `97619c9`; the `Firmware/` tree is the
combined firmware image (`c57e223`).

**Verification performed:**

- Read `docs/dev/README.md`, `building.md`, `flashing.md`, `bootloaders.md`,
  `tooling.md`, `code-map.md`, `shared-machinery.md`,
  `avrisp-design-notes.md` and `verification.md`, plus the repository
  `TEMPLATE.md` and the documentation guidelines.
- Read `Firmware/Makefile`, `Firmware/configuration.h`, the MPLAB-X project,
  `Bootloaders/BPv3-bootloader/firmware-v4.5/` (loader and settings),
  `Bootloaders/BPv4-bootloader/firmware-v1/` (`main.c`, `bootloader.c`,
  `bootloader.h`, `boot_config.h`), `Bootloaders/pirate-loader/` and the
  `upgrader-v4tov4/` source.
- Reproduced the Windows build failure with GNU Make 4.4.1 (MinGW): the
  documented `SHELL=` assignment failed at `$(shell date ...)`; the Git
  `usr\bin` + explicit `XC16_DIR` recipe built the v4 image and drove the v3
  build to its expected SMPS guards.
- Built in a temporary copy of `Firmware/`: default v4 at 90,144 bytes
  program / 12,626 bytes data; all-features v4 after `clean` at 79,485 /
  11,898; v3 custom build stopped at both SMPS `#error`s.  The all-features
  map contains `LCDsetup` and `smps_start`; the default v4 map contains
  neither.  A seven-character `VERSION` hash was used for the size check.
- Confirmed the make wildcard split with a minimal makefile; confirmed
  `xc16-gcc` is not on `PATH` while the installer-default XC16 exists.
- No board was attached, so bootloader entry, LED/VREG observation, flashing
  and recovery remain source-derived.

## Findings

| # | Decision | What changed (file + short quote) | Evidence |
|---|---|---|---|
| 1 | fixed | `README.md`: step 5 says "On v4 `--hello` sends the restart byte and leaves the bootloader; re-enter it before step 6 if you used `--hello`"; step 6 says "Run it directly after entering the bootloader; do not put `--hello` between the entry and this step". | `pirate-loader.c` `main()`: the `0xff` send is outside `if (!g_hello_only)` and runs when `flashsize > BPV3_FLASHSIZE`; the v4 device table sets `0x2AC00`. |
| 2 | fixed | `building.md`: Windows recipe is now `$env:Path = 'C:\Program Files\Git\usr\bin;' + $env:Path` plus explicit `XC16_DIR`; the `SHELL` variable row is gone; the error table points at `PATH`. | GNU Make 4.4.1 (MinGW) ignored the command-line `SHELL=`; the exact documented command failed at `process_begin: CreateProcess(NULL, date ...)`.  `Firmware/Makefile` never references `SHELL`.  The PATH recipe built the v4 image and drove the v3 build to its expected SMPS guards. |
| 3 | fixed + code-owner CO-2 | `building.md` Prerequisites: "The Windows fallback under `C:/Program Files (x86)` is not matched: the Makefile's `$(wildcard ...)` path is unquoted and splits at the space.  Pass `XC16_DIR` explicitly on Windows". | `Firmware/Makefile` `$(wildcard C:/Program Files (x86)/Microchip/MPLABXC16/v2.10/bin)`; a minimal makefile gives an empty match for the unquoted path and a match for the escaped path; `xc16-gcc` is absent from `PATH` while the default install exists. |
| 4 | fixed + code-owner CO-1 | `building.md`: "Changing only `EXTRA_CFLAGS` does not invalidate the existing objects ... Clean first"; the recipe is `make ... clean` then the custom build; added the expected 79,485 / 11,898 result and the map/compile-line check. | `Firmware/Makefile` `CFLAGS += $(EXTRA_CFLAGS)`, object paths `build/$(TARGET)/...`; after a default v4 build, the custom command printed `make: Nothing to be done for 'all'.`; after `clean`, the all-features build linked 79,485 program / 11,898 data. |
| 5 | fixed | `bootloaders.md`: hardware-programmer row now says "erase the chip and write the loader only"; step 3 writes the application with the ds30 GUI or `pirate-loader`; step 4 explains the jump pair and says a direct programmer write restores `0x0000` so `$` and the jumper can no longer enter. | `ds30loader.s` `usrapp` at `STARTADDR-4` and its row-0 rewrite; `pirate-loader.c` `fixJumps()` (v3-sized parts only); `upgrader-v4tov4/main.c` erases/rewrites the loader page, points `0x0000` at it and jumps to `BLJUMPADDRESS` (`0xABF8` -> `firmwarejump` -> `setup`). |
| 6 | fixed | `flashing.md` and `bootloaders.md`: "turns VREG off for the USB enumeration wait, then turns it on again before the command loop; the command loop runs with VREG lit." | `main.c` `startpoint()`: `vLedOn()` before `usb_init`, `vLedOff()` before the `usb_device_state` wait, `vLedOn()` before `bootloader()`. |
| 7 | fixed | `README.md`: the five-minute flash bullet names both images and links to `flashing.md`; shortest-path step 6 uses `Firmware/bpv4.hex` and says "use `Firmware/bpv3.hex` on v3". | The Makefile produces `Firmware/bpv3.hex` / `bpv4.hex`; the page covers both boards. |
| 8 | fixed | `flashing.md`: "detach with `Ctrl+a d`." | `screen`'s default bindings: `Ctrl+a d` detaches; `Ctrl+a k` kills the window. |
| 9 | fixed | `building.md`: table symptom is now "`SMPS support is not available on Bus Pirate v3 boards.`"; a note says the second guard prints the shorter string without `boards.`. | `configuration.h` has two `#ifdef BP_ENABLE_SMPS_SUPPORT` guards; a v3 custom build printed both `#error`s. |
| 10 | fixed | `code-map.md`: added the missing `/ \ - _ . ^ !` line; the sentence now says digits select a mode and spaces/commas are separators. | `proc_menu.c` `serviceuser()` cases `/`, `\`, `-`, `_`, `.`, `^`, `!`; space and comma are ignored. |
| 11 | fixed | `README.md`: "**v3 hardware is unverified.** v3 builds are supported; the v3 hardware behaviour in this set is source-derived and not verified on hardware."  `verification.md` uses the same wording. | The set documents v3 `$`, PGC/PGD, FTDI, ds30 and recovery; the old sentence denied v3 hardware behaviour. |
| 12 | fixed | `bootloaders.md`: both `cmd = 0x02` rows now say "the next status is `'K'`, `'V'` or `'P'`." | `bootloader.c` `WritePage()` sets `'P'` for `PROT_BL` / `PROT_CONFIG`; `ds30loader.s` `bladdrerror` sends `BLPROT` and `vfail` sends `'V'`. |

No numbered finding was rejected.

### Additional fixes found while verifying

- **A1 — v3 loader address wording.**  `bootloaders.md` now says the loader
  occupies "one program page at `0xA800`, with the version word and the config
  words at its end"; the stored-version row says "last word" instead of "last
  instruction".  Evidence: `devices.inc` `FLASHSIZE 0xAC00`, `settings.inc`
  `PAGESIZE 512`, `ds30loader.s` `STARTADDR`; `hardwarev3.h`
  `BL_ADDR_VER 0xABFA`; `blver` is at `0xABFA`, before the config words at
  `0xABFC` / `0xABFE`.
- **A2 — pirate-loader Windows route.**  `flashing.md` marks the Visual Studio
  route **unverified** and says a MinGW build of the checked-in project does
  not compile its Windows serial wrappers; `README.md` points at that caveat
  instead of presenting the `.sln` route as the Windows recipe.  This is also
  code-owner candidate CO-3.
- **A3 — v3 loader guard wording.**  `bootloaders.md` now says "a write that
  starts in the loader page is rejected with `'P'`" instead of the broader
  "any write at or past the last row before the loader".  The old wording
  would also deny the `usrapp` row at `0xA780`, which the application write
  must be allowed to reach.  The remaining threshold gap is CO-4.

## Code-owner candidates

### CO-1 — changing `EXTRA_CFLAGS` / `OPT` does not rebuild

- **File / symbol:** `Firmware/Makefile`, `CFLAGS`, `EXTRA_CFLAGS`,
  `OBJECTS`, `build/$(TARGET)/%.o`.
- **Behaviour:** `EXTRA_CFLAGS` is appended to `CFLAGS`, but the object path
  depends only on `TARGET` and the source name.  `-MMD -MP` tracks headers,
  not compiler flags.  A flag-only change leaves every object up to date and
  the old image is linked.
- **Why it matters:** the documented all-features build silently links the
  default image.  `building.md` now requires `clean`, but the Makefile still
  permits the trap.
- **Suggested fix:** a flag stamp dependency (a file whose content is the
  flag set) or per-feature object directories.

### CO-2 — Windows XC16 installer-default path is never matched

- **File / symbol:** `Firmware/Makefile`, `ifeq ($(XC16_DIR),)`,
  `$(wildcard C:/Program Files (x86)/Microchip/MPLABXC16/v2.10/bin)`.
- **Behaviour:** the unquoted `$(wildcard ...)` splits at the spaces, so the
  default Windows install is not found; the build falls through to bare
  `xc16-` and fails with `xc16-gcc: command not found` even though the
  compiler is installed at the default path.
- **Why it matters:** the documented fallback is not a fallback on Windows;
  `building.md` now says to pass `XC16_DIR` explicitly.
- **Suggested fix:** quote/escape the path in the `wildcard` call (for example
  `$(wildcard C:/Program\ Files\ (x86)/...)`) or test `XC16_DIR` first and
  make the Windows default explicit.

### CO-3 — pirate-loader CMake project is GCC-only on Windows

- **File / symbol:** `Bootloaders/pirate-loader/CMakeLists.txt`,
  `CMAKE_EXE_LINKER_FLAGS`; `Bootloaders/pirate-loader/pirate-loader.c`,
  `#ifdef WIN32`, `write()`, `read()`, `close()`, `open()`.
- **Behaviour:** the CMake project unconditionally sets
  `-static -static-libgcc`, which is a GCC-style link option.  A MinGW build
  compiles the `WIN32` serial wrappers and then fails on conflicting
  declarations for `write`, `read` and `open`; the Visual Studio link was not
  available to test.
- **Why it matters:** the repository has no verified Windows source build.
  `flashing.md` now marks the Visual Studio route unverified and points at the
  prebuilt binary; `README.md` follows that caveat.
- **Suggested fix:** make the linker flags compiler-conditional, select the
  Windows path with `_WIN32` (or a CMake option), and verify the MSVC link.

### CO-4 — v3 ds30 loader protection threshold is 64 bytes loose

- **File / symbol:** `Bootloaders/BPv3-bootloader/firmware-v4.5/ds30loader.s`,
  `BLCHECKST`, `ROWSIZE`, `bladdrchk`, `program`.
- **Behaviour:** `ROWSIZE` is 64 words, so one row write covers 128 bytes.
  `BLCHECKST = STARTADDR - ROWSIZE` evaluates to `0xA7C0`; a row starting at
  that highest allowed address writes through `0xA83F`, overlapping the
  loader at `0xA800`.  The last safe row starts at `0xA780`, which is also the
  row that carries `usrapp` at `0xA7FC`.
- **Why it matters:** normal host tools send row-aligned addresses
  (`0xA780`, then `0xA800`), so routine flashes do not hit the gap; a
  malformed or unaligned host can.  This is a hardening issue, not a
  routine-flash failure.
- **Suggested fix:** `BLCHECKST = STARTADDR - (ROWSIZE * 2)` or compare the
  row end address, not the start.

## Cross-set changes for the main session

1. **Optional — root `README.md`.**  Add a maintainer/build pointer near the
   top, before "Purchasing": "Building or flashing?  Start at
   `docs/dev/README.md`."  The existing "Firmware" section is at the bottom
   and is correct; this is navigation only, not a correctness fix.
2. **No change required — `docs/generic/`.**  It carries no `pirate-loader`
   flash sequence or `--hello` instruction; the `$`/bootloader entry wording
   does not conflict with the dev pages.
3. **No change required — `docs/core/` and `docs/SPEC.md`.**  Their "v3 is
   build-only" phrases are already qualified with "not verified on v3
   hardware"; `SPEC.md` says the v3 build is "documented but not verified on
   v3 hardware".  The dev README was the only place that denied v3 hardware
   behaviour outright.

## Remaining questions

- **Visual Studio link (CO-3).**  No Visual Studio was available; the `.sln`
  route stays unverified.  The prebuilt `pirate-loader.exe` is the practical
  Windows binary, with its port speed still unverified.
- **v3 guard arithmetic (CO-4).**  The gap is source-derived; no malformed
  host or unaligned image was exercised.  The code owner should confirm the
  intent before changing the guard.
- **`pk2cmd` switches.**  Still unverified, as the page says.
- **Code-map key list.**  The list is now complete against `serviceuser()`,
  but it remains hand-maintained; a generated check would prevent the next
  drift.
- **Hardware basis.**  No board was attached; bootloader entry, the VREG LED
  sequence, port re-enumeration, flashing and recovery remain source-derived.

## Files changed

- `docs/dev/README.md`
- `docs/dev/building.md`
- `docs/dev/flashing.md`
- `docs/dev/bootloaders.md`
- `docs/dev/code-map.md`
- `docs/dev/verification.md`
- `docs-sweep/fixes-dev-round1.md` (this record)
