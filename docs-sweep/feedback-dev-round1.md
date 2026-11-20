# Dev docs — maintainer reader, round 1

## Verdict

**Yes with fixes.** The set is unusually usable and most of its hard claims
match the tree, but the "shortest path" flash sequence and the Windows build
instructions fail as written, and the custom-feature recipe silently reuses
stale objects.

## Task log

1. **First impression (README).** Read root `README.md`, `docs/dev/README.md`,
   then `TEMPLATE.md`/`SPEC.md` for context. ~5 min. The set is clearly the
   legacy v3/v4 community firmware; `docs/dev/README.md` labels itself
   maintainer-facing and gives an obvious file map. The root README's
   "Firmware" pointer is at the bottom, after purchasing, so a clean-checkout
   maintainer has to scroll past most of the page or infer the `docs/dev` row
   in the cluster table. The later "v3 is build-only" claim then conflicts with
   the v3 flash instructions in the same set.

2. **Build.** Read `Firmware/Makefile`, `configuration.h`, the MPLAB-X
   configuration and the string tooling. Toolchain present: GNU Make 4.4.1,
   XC16 2.10 at the installer default, host GCC and Python 3. Tried the exact
   Windows command from `building.md`; it failed before compiling (see
   finding 2). Retried with Git's `usr\bin` on `PATH` and an explicit
   `XC16_DIR`: v4 default built to 90,144 bytes program / 12,626 data;
   all-features after `clean` built to 79,485 / 11,898 (exactly the documented
   result, zero warnings); v3 default built to 59,415 / 6,488. The v3 custom
   build stops at two SMPS `#error`s. The documented custom command after a
   normal v4 build prints `Nothing to be done` and leaves the default image.
   Yes, I can produce an image. Unclear: the Windows `SHELL` advice, the
   installer-default fallback, and the undocumented need for `clean` before
   changing `EXTRA_CFLAGS`.

3. **Flash.** Read `flashing.md`, `bootloaders.md`, `pirate-loader.c` and both
   bootloader sources; no board was attached. Desk check succeeded: options,
   `B115200`/921600 Windows trap, v4 `0xff` restart, v3/v4 status bytes, port
   re-enumeration and post-flash `i` check all match the source. Recovery is
   mostly clear through the Pickit2 route and the blank-v4 warning. The main
   procedural dead end is the README shortest path: it uses `--hello` and then
   flashes without re-entering the bootloader, although the detail page warns
   that `--hello` resets a v4. The v3 bootloader-update route is also
   ambiguous about which tool writes the application.

4. **Bootloaders.** Read the folder map and both loader sources. The version
   claims match: v3 stored `0x0405` / hello `1.02`, v4 hello `4.12`; the
   addresses (`0x27FA`, `0x27FC`, `0x47FA`, `0x47FC`), signature, entry rules
   and protection bits check out. The v4 VREG LED sequence is described
   backwards in two pages (finding 6). The v3 update-route table conflicts
   with its own step 3 (finding 5).

5. **Test and map.** Counted the 32 `test_*.py` files; the `tooling.md` table
   names all 32. Ran all 32 from the repository root: 32 passed. Ran a
   `packstrings.py` regeneration to a temp directory: content matched, but
   output line endings are LF while the tracked working files are CRLF on this
   checkout. The code-map's 222 function tokens all exist, and its file paths
   resolve. The verification matrix labels are consistent with the pages. The
   dispatcher key list is incomplete against `serviceuser()` (finding 10).

6. **Shared machinery and AVRISP notes.** Checked the helper table against
   `core.c`, `proc_menu.c` and `base.c`: the accessors, parser entry, decimal
   engine and masking claim are current. The AVRISP design notes link every
   current user page, record the custom-build omission and document AVR053 /
   AVR053 wiring. A custom v4 build with AVRISP added also links (91,854 bytes
   program / 12,684 data), so the suggested route is viable.

7. **Rendering.** Scanned the 9 dev pages plus root `README.md`,
   `TEMPLATE.md` and `SPEC.md` (12 files, 1,789 lines): zero TAB characters,
   zero non-command prose lines over 88 columns, no dates, pass counts, bench
   narratives, capture provenance or internal log names, and no dead links.
   There are no raw terminal captures to misalign. The presentation problems
   here are factual claims, not layout.

8. **README navigation.** Root → `docs/dev/README.md` → any dev page is two
   clicks; inside the dev README every page is one click. No link dead ends.
   Build, flash, bootloaders, tooling, code-map and verification are all
   reachable. The only dead end is operational (finding 1). The two quick
   starts ("five-minute version" and "shortest path") have already diverged
   and should not both carry command sequences.

## Findings

### 1. [major] The shortest path resets a v4 before it flashes

`docs/dev/README.md:62-65`:

> 5. Find the port: on v4 the bootloader enumerates as a new device, so use
>    `pirate-loader --dev=<port> --hello` to confirm — see
>    [flashing.md §Find the port](flashing.md#find-the-port-after-entry).
> 6. `pirate-loader --dev=<port> --hex=Firmware/bpv4.hex`.

But `flashing.md:118` says:

> On v4 it still sends the `0xff` restart after the handshake, so the board
> leaves the bootloader

`pirate-loader.c` sends that `0xff` even in `--hello` mode whenever
`flashsize > BPV3_FLASHSIZE` (the v4 case). A maintainer who follows the
shortest path finds the port, resets the board, and then step 6 fails because
the bootloader is gone and the port may re-enumerate. Add an explicit
"re-enter the bootloader after `--hello`" step, or state that `--hello` is
only for port identification and is never followed immediately by a flash.

### 2. [major] The documented Windows build command fails with native GNU make

`building.md:26`:

> On Windows, point `SHELL` at a `sh.exe` (for example the one shipped with
> Git for Windows).

and `building.md:87-88`:

> `make -C Firmware BP_VERSION=V4 'SHELL=C:/Program Files/Git/usr/bin/sh.exe' 'XC16_DIR=...'`

With GNU Make 4.4.1 on this machine, the `SHELL=` assignment was ignored. The
exact command failed before compiling:

```
process_begin: CreateProcess(NULL, date -u +%Y-%m-%d, ...) failed.
Makefile:133: pipe: Bad file descriptor
process_begin: CreateProcess(NULL, mkdir -p build/bpv4/protocol_wire/, ...) failed.
```

Putting `C:\Program Files\Git\usr\bin` on `PATH` and passing `XC16_DIR`
explicitly built both boards. The page should name the make flavour that
honours `SHELL` or replace this recipe with the PATH method; the Makefile's
`SHELL` variable-table entry has the same problem.

### 3. [minor] The XC16 installer-default fallback does not work on Windows

`building.md:25`:

> The Makefile falls back to the installer defaults `/opt/microchip/xc16/v2.10`
> and `C:/Program Files (x86)/Microchip/MPLABXC16/v2.10`

The Makefile uses `$(wildcard C:/Program Files (x86)/...)`. GNU make splits
that on spaces (a direct check gives `W=[]`), so without `XC16_DIR` the build
falls through to bare `xc16-` and fails with `xc16-gcc: command not found`
even though XC16 is installed at the default path. Quote the path in the
Makefile or make the Windows instructions always pass `XC16_DIR`.

### 4. [major] The custom-feature command silently links the old image

`building.md:150-156` presents this as the way to build the all-features list:

> For the all-features list instead of one switch, define
> `BP_CUSTOM_FEATURE_SET` without editing the header — the Makefile appends
> `EXTRA_CFLAGS` to the compiler flags:
>
> `make -C Firmware BP_VERSION=V4 EXTRA_CFLAGS=-DBP_CUSTOM_FEATURE_SET`

Changing only `EXTRA_CFLAGS` does not invalidate the existing objects. After a
normal v4 build (90,144 bytes) I ran the documented command and got:

```
make: Nothing to be done for 'all'.
```

The linked image stayed the default 90,144-byte build; the documented 79,485
all-features result only appeared after `make ... clean`. Add `clean` to the
recipe (or add a flag/configuration stamp to the Makefile) before this
example. The same trap applies to any change that only affects CFLAGS.

### 5. [major] The v3 update route does not say which tool writes the application

`bootloaders.md:180-183`:

> | Hardware programmer | erase the chip, write the loader, then write the application ([Pickit2 route](flashing.md#the-pickit2-route-full-chip-programming)) | always works, including blank/bricked chips |

and `bootloaders.md:174-178`:

> The upgrader rewrites the reset vector at `0x0000` to jump into the loader,
> and the programmer route erases the chip, so either way the board boots into
> the loader until a new application is written with the ds30 GUI

If "write the application" means the linked Pickit2 route, the route is wrong
for keeping the loader reachable. A direct programmer write of the
application restores the application's reset vector at `0x0000`; only the
ds30 GUI / `pirate-loader` path runs the jump fix in `pirate-loader.c`
(`fixJumps`, `pirate-loader.c:610-631`), copying the application reset vector
to `STARTADDR-4` and pointing `0x0000` at the loader. The result is a board
that boots the application but whose `$` flag and PGC/PGD jumper cannot enter
the loader. Make the sequence explicit: programmer writes the loader only;
power-cycle into the loader; then write the application with the ds30 GUI /
`pirate-loader`. If a programmer write of the application is intended, state
how the loader entry is restored.

### 6. [minor] The v4 VREG LED sequence is reversed

`flashing.md:65-67`:

> During the entry window the MODE, USB and VREG LEDs light; the loader then
> turns MODE/USB off, keeps VREG on until USB enumeration, and turns VREG off
> before the command loop.

`bootloaders.md:101-102` repeats it:

> `startpoint()` turns MODE/USB off, keeps VREG on until USB enumeration,
> then turns VREG off and enters the command loop.

`Bootloaders/BPv4-bootloader/firmware-v1/main.c:140-161` does the opposite:
`vLedOn()` at 142, `vLedOff()` at 148 before the enumeration wait, then
`vLedOn()` at 161 before `bootloader()`. The command loop runs with VREG on.
Correct both pages; the current wording will mislead anyone who uses the LED
or VREG rail to judge bootloader state.

### 7. [minor] `bpv4.hex` is hard-coded while the text covers v3 too

`docs/dev/README.md:38-40`:

> `pirate-loader --dev=<bootloader port> --hex=Firmware/bpv4.hex`.

and `docs/dev/README.md:65-66`:

> 6. `pirate-loader --dev=<port> --hex=Firmware/bpv4.hex`.

Both passages explicitly include v3, whose image is `Firmware/bpv3.hex`. A
maintainer following them on v3 will try to flash the v4 image. Use
`--hex=Firmware/bpv<3|4>.hex`, or show the v3 and v4 commands separately.

### 8. [minor] `Ctrl+a Ctrl+k` is "kill", not "detach"

`flashing.md:73-75`:

> With `screen`, for example, detach with `Ctrl+a Ctrl+k`.

In `screen`, `Ctrl+a d` detaches; `Ctrl+a k` kills the window. Killing does
release the port, but it destroys the session. Say "detach with `Ctrl+a d`"
(or call `Ctrl+a k` "kill the window").

### 9. [minor] The SMPS error quote is only half of what the build prints

`building.md:294`:

> | `SMPS support is not available on Bus Pirate v3` | SMPS enabled in a v3 build (the custom block does this) | use a v4 build for SMPS |

A v3 custom build actually stops at both guards in `configuration.h`:

```
./configuration.h:496:2: error: #error "SMPS support is not available on Bus Pirate v3 boards."
./configuration.h:545:2: error: #error "SMPS support is not available on Bus Pirate v3"
```

The table quotes the second, dead-looking message. Quote the first one (with
"boards.") or note that both duplicate guards fire, so a maintainer searching
for the table's exact string is not surprised.

### 10. [minor] The code-map dispatcher key list is incomplete

`code-map.md:31-35`:

> The dispatcher's letter and symbol keys (digits select a mode):
>
> ```
> h ? i m b o v t T f g c C k K L l p P e = | ~ # $ a A @ W w d D & % s S O < ( " [ { ] } r
> ```

`serviceuser()` in `Firmware/core_system/proc_menu.c` also handles `/`, `\`,
`-`, `_`, `.`, `^`, `!` and space. The list reads as authoritative but is a
partial list. Either add the remaining keys or label it "frequent keys" and
point at the dispatcher for the full set.

### 11. [minor] "v3 is build-only" contradicts the v3 hardware procedures

`docs/dev/README.md:89`:

> **v3 is build-only.**  No v3 hardware behaviour is claimed by this set.

The same set documents the v3 PGC/PGD jumper, FTDI `0403:6001` link, the v4.5
ds30 UART divider and the v3 Windows speed trap, the `$` flag at `0x27FA`,
post-flash bootloader-version printing, and v3 recovery. The intended meaning
is "no v3 hardware basis / not verified on hardware"; the current sentence
says the opposite of what the pages do. Rephrase to "v3 builds are supported;
v3 hardware behavior is source-derived and not verified on hardware."

### 12. [minor] The v3/v4 command table omits the `'P'` write status

`bootloaders.md:117`:

> | `cmd = 0x02` | erase-if-enabled then write one row, verify; the next status is `'K'` or `'V'` |

`bootloaders.md:127` then lists:

> | `'P'` | write would touch a protected region |

`WritePage()` in `Bootloaders/BPv4-bootloader/firmware-v1/bootloader.c` sets
`'P'` for the protected bootloader/config ranges. The v3 ds30 source does the
same. The command row should say `'K'`, `'V'` or `'P'` (and note `'N'`/`'U'`
for the framing errors) rather than imply a write can only return two bytes.

## What to keep

- The 100-line `docs/dev/README.md` map: file table, status line and
  "maintainer-facing" label are the right shape for this set.
- The build result is not decorative: after a clean build the all-features v4
  image reproduced at exactly 79,485 bytes program / 11,898 bytes data and
  zero warnings. The artifact table and `clean`/`distclean` behaviour also
  match the Makefile.
- `flashing.md`'s hardware-specific detail is the strongest part of the set:
  the v3 Windows 921600/115200 trap, the v4 new-port rule, the `--hello`
  restart warning, the status-byte handling and the post-flash `i` check are
  all directly code-verified.
- `bootloaders.md` keeps the two loader families and their version split
  straight: stored `4.5`/hello `1.02` for ds30, `4.12` for the v4 USB loader,
  with matching addresses and entry conditions.
- `tooling.md`'s 32-row suite table is complete; all 32 scripts passed from the
  repository root. The XSVF caveats are specific enough to act on.
- `code-map.md` is genuinely navigable: all 222 function tokens I extracted
  exist, and every path resolves. `verification.md` keeps its labels
  disciplined, and `avrisp-design-notes.md` links every current AVRISP page.

## Structural opinion

The layout is right for this set: one short map, then focused pages for build,
flash, bootloaders, tooling, code-map, shared helpers and AVRISP. Do not turn
the dev README into a manual.

Concrete changes:

- Keep **one** quick start. The five-minute summary and the seven-step
  "shortest path" already disagree; either move the operational sequence into
  `flashing.md` and leave only links in the README, or delete the five-minute
  duplicate.
- Add a maintainer start pointer near the top of the root README
  ("Building or flashing? Start at `docs/dev/README.md`"). The current
  "Firmware" section is at the bottom, after purchasing.
- Make the v3 update route a tool-explicit table with columns `Step | Tool |
  Effect | Resulting reset vector`, or split it into its own page. The
  `bootloaders.md` page currently mixes loader reference, host protocol and
  recovery procedure in one 218-line file.
- Either generate the dispatcher key list from `proc_menu.c` or remove it from
  `code-map.md`; a hand-maintained partial list will keep going stale.
- Add a "Windows quick start" block with the exact working command (Git
  `usr\bin` on `PATH`, explicit `XC16_DIR`) and a note that a custom feature
  build starts from `clean` or a flag stamp.

## Unknowns

- No v3 or v4 hardware was attached, so the actual bootloader entry, LED/VREG
  behavior, port re-enumeration, flashing, post-flash `i`, udev symlink and
  Pickit2 recovery are untested. `pk2cmd`'s exact switch set remains
  unverified, as the page says.
- MPLAB-X was not available: the IDE build and the
  `dist/<config>/production/` path are still unverified, as documented.
- The Windows `pirate-loader.sln` route was not built (no Visual Studio on
  this machine). `Bootloaders/pirate-loader/CMakeLists.txt:20` sets
  `-static -static-libgcc` unconditionally, which looks GCC-only; verify the
  Visual Studio link or document the MinGW/Unix Makefiles route.
- Only XC16 2.10 on Windows was used. The docs claim a 1.35 minimum but I did
  not build with other XC16 versions; the Linux `make` path was not exercised.
- The historical v3 upgrader/update bundle and the prebuilt Windows
  `pirate-loader.exe` were inspected, not run.
- I reviewed the working tree; the `docs/dev/` files are uncommitted in this
  checkout, so this is not a review of a fixed commit.
