# Dev docs — re-audit round 1

**Scope:** the 12 findings in `feedback-dev-round1.md` against the current
`docs/dev/**`; no fresh full review. I also read `fixes-dev-round1.md` and the
source it cites where the fix depends on source behaviour.

**Verdict: 11 of 12 items filled in full.** The one remaining item is the
post-write exit from the v3 upgrader route (finding 5, minor).

## Per-item table

| # | Status | Current evidence | Remainder |
|---|---|---|---|
| 1 | **Filled in full** | `docs/dev/README.md:65-72`: "On v4 `--hello` sends the restart byte and leaves the bootloader; re-enter it before step 6 if you used `--hello`." Step 6 then says: "Run it directly after entering the bootloader; do not put `--hello` between the entry and this step." | None. The shortest path now has the re-entry step and the warning in `flashing.md:118` still matches `pirate-loader.c` (the `0xff` send is outside `if (!g_hello_only)` on v4). |
| 2 | **Filled in full** | `building.md:25-26`: "On Windows, use a native MinGW GNU make … put Git for Windows' `usr\bin` first on `PATH`. A command-line `SHELL=` assignment is not honoured by that make; `sh.exe` on `PATH` is what it uses." The recipe is `building.md:87-90` (`$env:Path = …` plus explicit `XC16_DIR`). | None. I re-ran the documented recipe with GNU Make 4.4.1: clean + default v4 built 90,144 / 12,626, and clean + all-features built 79,485 / 11,898. The old `SHELL=` row and the misleading Windows error row are gone. |
| 3 | **Filled in full** | `building.md:25`: "The Windows fallback under `C:/Program Files (x86)` is not matched: the Makefile's `$(wildcard ...)` path is unquoted and splits at the space. Pass `XC16_DIR` explicitly on Windows (or put the compiler's `bin` directory on `PATH`)." | None for the docs. The Makefile still has the unquoted wildcard; that is correctly recorded as code-owner **CO-2**, not a documentation claim. |
| 4 | **Filled in full** | `building.md:157-167`: "Changing only `EXTRA_CFLAGS` does not invalidate the existing objects … Clean first, then build", followed by `make … clean` and the custom build. `building.md:176-182` gives the reproduced 79,485 / 11,898 result and the map/compile-line check. | None. The recipe now matches the observed Makefile behaviour; code-owner **CO-1** remains the hardening fix. |
| 5 | **Partial** | `bootloaders.md:180-187` now says the programmer writes "the loader only", and step 3 says to write `Firmware/bpv3.hex` "with the ds30 GUI … or `pirate-loader`"; step 4 explains the `STARTADDR-4` / `0x0000` jump pair and the direct-programmer failure. The table at `bootloaders.md:191-192` is now tool-explicit. | After the **upgrader** route, the application write is the last step described ("then remove the jumper"), but that route has no jumper. The v3 loader's upgrader path enters through `firmwarejump`, so `WFWJUMP` is set and the loader stays in its command loop after the application write; there is no `0xff` restart on v3. Add "power-cycle after the application write" (for both routes, or link the `flashing.md` post-flash restart rule) so the route ends with the application running. This is a minor completion gap, not the old tool ambiguity. |
| 6 | **Filled in full** | `flashing.md:65-68`: "On the bootloader path, `startpoint()` turns MODE/USB off and VREG on, turns VREG off for the USB enumeration wait, then turns it on again before the command loop; the command loop runs with VREG lit." `bootloaders.md:99-101` repeats the corrected sequence. | None. This matches `main.c:140-161` (`vLedOn` → `vLedOff` before the wait → `vLedOn` before `bootloader()`). |
| 7 | **Filled in full** | `README.md:38-41`: "run `pirate-loader` with `Firmware/bpv3.hex` or `Firmware/bpv4.hex`; the exact order and the v4 port rule are in flashing.md." Step 6 (`README.md:70-72`) uses `Firmware/bpv4.hex` and adds "use `Firmware/bpv3.hex` on v3." | None. |
| 8 | **Filled in full** | `flashing.md:74-76`: "With `screen`, for example, detach with `Ctrl+a d`." | None. This is the correct detach binding; `Ctrl+a k` is no longer called detach. |
| 9 | **Filled in full** | `building.md:313`: the table symptom is now "`SMPS support is not available on Bus Pirate v3 boards.`"; `building.md:317-318` notes that the second guard prints the shorter string without `boards.` | None. The table now quotes the first error the compiler actually prints; both guards are explained. |
| 10 | **Filled in full** | `code-map.md:31-38`: "digits select a mode; spaces and commas are separators", with `/ \ - _ . ^ !` added to the key list. | None. The list is complete against the non-digit `serviceuser()` cases I checked; digits and separators are described separately. |
| 11 | **Filled in full** | `README.md:96-97`: "**v3 hardware is unverified.** v3 builds are supported; the v3 hardware behaviour in this set is source-derived and not verified on hardware." `verification.md:19-20` now says "v3 hardware is unverified throughout. No v3 hardware basis is claimed by any set." | None for the quoted finding. The verification matrix rows still use `v3 build-only` as a basis shorthand (`verification.md:33,48,49`); that is not the old "no behaviour claimed" denial, but it reads slightly abbreviated next to the reworded top. |
| 12 | **Filled in full** | `bootloaders.md:116` and `:150`: both `cmd = 0x02` rows now say "the next status is `'K'`, `'V'` or `'P'`." | None. `WritePage()` sets `'P'` for protected ranges; the status table at `bootloaders.md:122-128` lists it. |

## Focus checks requested

- **Shortest flash path:** Correct now. The `--hello` reset is stated before
  step 6, step 6 says to re-enter and not to put `--hello` between entry and
  flashing, and both image names are given. Executable as written.
- **Windows build recipe:** Correct and executable. The documented
  `$env:Path = 'C:\Program Files\Git\usr\bin;' + $env:Path` plus explicit
  `XC16_DIR` works with the installed GNU Make 4.4.1; the page also explains
  why the old `SHELL=` form failed and why the installer-default path is not
  auto-detected.
- **Custom-feature clean/build:** Correct. The page now warns that changing
  only `EXTRA_CFLAGS` leaves objects up to date and puts `make … clean` before
  the custom build. The reproduced 79,485 / 11,898 result and the map /
  compile-line check are present.
- **v3 bootloader write route:** Mostly correct; the tool ambiguity and direct
  programmer failure are fixed. The remaining gap is the upgrader route's exit
  after the application write (finding 5 above).
- **v4 VREG order:** Correct. Both pages now match `main.c`.

## Presentation check

- **TAB characters:** none in `docs/dev` or the root `README.md`.
- **Captures:** no raw terminal captures in this set, so no alignment problem.
  The expected-output material is in tables.
- **Menus / option lists:** `pirate-loader` options, make variables, bootloader
  frames/statuses and test suites are tables. The only compact key sequence is
  `code-map.md:34-38`; it is a dispatcher-key sequence, not a menu.
- **~80-column wraps:** all `docs/dev` prose is at or below 88 columns; no
  fenced command line is over the page width by accident. The root `README.md`
  has one pre-existing 93-column blockquote at line 58; it is outside the
  changed dev set.
- **Bench/date/test-result prose:** no dates, pass counts, run logs, bench/rig
  narratives, capture provenance or internal test-file names in `docs/dev`.
  The exact `79,485` / `11,898` all-features build result remains in
  `building.md:176-179`; `verification.md:61` explicitly records it as the
  set's build basis, so I treated it as the documented build result rather
  than a new bench claim. If the rule is meant to apply literally to
  maintainer pages too, that is the only measured-result passage to move or
  remove.

## Code-owner candidates and cross-set

The four code-owner candidates are accurately scoped and do not block the
docs:

- **CO-1** (Makefile flag stamp): docs now require `clean`; Makefile still
  permits the stale-object trap.
- **CO-2** (Windows XC16 wildcard): docs now state the fallback is not matched;
  Makefile still has the unquoted wildcard.
- **CO-3** (pirate-loader Windows CMake): docs now mark the Visual Studio
  route unverified and the MinGW wrapper compile failure is stated; there is
  still no verified Windows source build. This is an honest limitation, not a
  regression.
- **CO-4** (v3 ds30 guard arithmetic): `bootloaders.md:73-75` now says "a write
  that starts in the loader page" rather than the old over-broad threshold.
  The 64-byte guard gap remains a code hardening item.

The optional root `README.md` maintainer pointer was not added. The existing
bottom "Firmware" section still reaches `docs/dev/README.md`; this is a
navigation improvement, not a required fix.

## Still open

- **#5 (minor):** add the v3 post-application-write power-cycle (or link the
  post-flash restart rule) to the upgrader route in `bootloaders.md:174-179`;
  otherwise the route leaves the board in the loader and the "then remove the
  jumper" line does not apply to that route.
