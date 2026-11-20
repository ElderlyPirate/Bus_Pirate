# LCD docs — round 1 fixes

Scope: `docs/capabilities/lcd/**` plus this record.  No firmware, WORKLOG or
other set edits.  No commit.

**Result:** 13 findings processed: **13 fixed in-set** (11 direct, 2 with
follow-ups), **0 rejected**, **1 code-owner candidate** (finding 6, status
flag source) and **3 cross-set files** for the main session (finding 1
`dev/building.md` with two edits, plus `dev/code-map.md` and
`dev/verification.md` consistency).

## Blocker note (finding 1)

The auditor's link-failure premise is **obsolete at this tree**.  The v4
all-features build was run:

```
make -C Firmware BP_VERSION=V4 EXTRA_CFLAGS=-DBP_CUSTOM_FEATURE_SET
```

with XC16 2.10.  It compiled and linked `Firmware/bpv4.hex`; the map contained
`_LCDwrite`, `_LCDsetup`, `_LCDsetup_exc` and `_LCDmacro`.  Program use was
`0x1367d` (79,485 bytes, 31%); data use was `0x2e7a` (11,898 bytes, 72%).
No compiler warnings or link errors were reported.  The LCD driver has its
own bounded byte helper (`hd44780_spi_write_byte`) and the SPI family also
exports `spi_write_byte`, so LCD and SPI are no longer a link-time conflict.

The contradiction came from `docs/dev/building.md:153-157`, which still says
the combination is expected to fail.  The LCD set never repeated that claim;
it now states the working build explicitly.  The stale build-page text is a
cross-set correction below.

## Per-finding record

| # | Decision | Change (file + short quote) | Evidence |
|---:|---|---|---|
| 1 | fixed (+ cross-set) | `entry-and-setup.md` adds “**Build status:** the v4 all-features build includes the LCD and SPI modules together and builds”; the availability table drops the 95%/30% cells and says “includes LCD and SPI; builds”; the v3 cell now says “cannot build: SMPS is v4-only”. `README.md` says the v4 all-features build “includes it and builds with SPI enabled”. | `Firmware/configuration.h` `BP_CUSTOM_FEATURE_SET` defines both `BP_ENABLE_HD44780_SUPPORT` and `BP_ENABLE_SPI_SUPPORT`; `Firmware/capability_hd44780/hd44780.c` `hd44780_spi_write_byte()`; `Firmware/protocol_spi/spi.c` / `spi.h` export `spi_write_byte()`. Build result above. `Firmware/configuration.h` SMPS `#error` for v3. |
| 2 | fixed | `wiring.md` replaces “Power the adapter” with “Display power, contrast and backlight”: a table naming adapter VCC, **display VDD (pin 2)**, **display VSS (pin 1)**, V0 (pin 3) and backlight A/K; “The v3-era board supplies 5.0 V to the glass”; “set the voltage selector to the glass's VDD”; “`W` is still required with a separate feed”. `entry-and-setup.md`, `usage.md` and `edge-cases.md` now key the clear/keep result to the **display VDD source**, not the adapter supply. | Legacy schematics: v2 `hardware-other/BPv3-HD44780_LCDv2/BP.LCDadapter.v2b.sch.png` (LCD pin 1 GND, pin 2 VCC, pin 3 V0 via R1 10K; IO header pin 8 VCC, pin 9 3V3, pin 10 GND); v4b `hardware-other/BPv4-HD44780_LCD/BPv4LCDadapter.v4b.sch.png` (J3 selects SWU33/SWU50 to VCC; LCD pin 2 VCC, pin 3 CONTRAST, pins 15/16 A/K). `hd44780_cleanup()` releases the pads; `switch_psu_off()` cuts the rails. |
| 3 | fixed | `wiring.md` now names the actual revisions: "v3-era LCD adapter: v2, v2a, v2b, v2c, v3, v3a" and "v4-era LCD adapter: v4 adapter v3 (2011), v4b"; embeds `BP-LCDv2-250px.jpg` and `BP-LCD-v3a-600.jpg`; links `Bus Pirate Lcd Adapter v3a.zip`; notes the passive v4 board is a breakout, not the 595 adapter. `README.md` changes "No backlight or contrast control" to "No firmware backlight or contrast setting" and names the v3a pots and v4b AUX0 jumper. | `git ls-tree d12478f~1 hardware-other/BPv3-HD44780_LCDv2` and `.../BPv4-HD44780_LCD`; `BP.LCDadapter-history.txt` (v2b added the adjustable backlight resistor; v3 part reduction); `docs/assets/art/BP-LCD-v3a-600.jpg` (Contrast and Backlight pots); v2/v4b schematics for the 595 and 10K pot. |
| 4 | fixed | `README.md` five-minute version now says “On a stock image `m9` is `PIC` on v4 and `DIO` on v3, not LCD; if `LCD` is absent from the `m` list, the image does not have it”; “other builds number it differently” is replaced by “in a build that includes it, the number is positional”. | `Firmware/configuration.h` both default blocks `#undef BP_ENABLE_HD44780_SUPPORT`; `Firmware/core_system/core.c` mode table order (LCD after KEYB, before PIC, DIO last). The existing build-dependent menu table gives v4 stock PIC 9/DIO 10 and v3 stock PIC 8/DIO 9. |
| 5 | fixed | `usage.md` adds the expected glass after `"Hello"` (“should show `Hello` from column 1”), after `"Bus Pirate"` (“should now show `Bus Pirate` on row 1; `Hello` has been overwritten”), after `(2)`/`(3)` (“should leave the glass cleared at home”), for `(6) 20` (20x2 visible / 16x2 last four off-screen), and a 5x8 custom-glyph bitmap `.###.` … `.###.`. `display.md` adds an “Expected result” table for 16x2, 20x2 and 20x4. | `Firmware/capability_hd44780/hd44780.c` `HD44780_WriteByte()` high/low nibble order, `HD44780_Init()` clear, `LCDmacro()` `(4)`/`(6)` raw addresses; the glyph bytes in `usage.md` decode to the shown 5-bit rows. The result is derived from the HD44780 map, not a hardware observation. |
| 6 | fixed (+ code-owner) | `entry-and-setup.md` replaces “the LCD setup does not select open drain” with “entering the mode sets that flag, but the LCD setup does not write the per-pin open-drain bits. On a clean entry those bits are clear, so the SPI pins are driven normally”. `README.md` adds “`i` still says `Open drain outputs`; that is the mode's flag, not the pad state.” | `Firmware/capability_hd44780/hd44780.c`, `LCDsetup()` sets `mode_configuration.high_impedance = 1`; `LCDsetup_exc()` does not write `BP_MOSI_ODC`/`BP_CLK_ODC`/`BP_CS_ODC`; `Firmware/core_system/proc_menu.c`, `print_status_info()` prints `Open drain outputs` when `high_impedance == YES`. Code-owner candidate below. |
| 7 | fixed | `entry-and-setup.md` removes the “about 95% used” / “about 30% used” row and the “used as this set's reference” phrase; the menu paragraph now reads “In the all-features v4 build, LCD is entry **9**”; the v3/v4 table row is “menu number in the documented build”. | Presentation change; the measured build numbers stay in this record and the cross-set note, not in the user page. |
| 8 | fixed | The `(2)` rule is now identical in all three places: `commands.md` “missing, `0`, or outside `1`-`2`”; `display.md` “Without an argument, with `0`, or with an argument outside `1`-`2`”; `edge-cases.md` “a missing, `0`, or outside `1`-`2` argument is treated as not given”. | `Firmware/capability_hd44780/hd44780.c`, `LCDmacro()` case 2: `if(!((input>=1)&&(input<=2)))` prints the menu and calls `getnumber(2,1,2,0)`. |
| 9 | fixed | `entry-and-setup.md` defaults row is now "display lines: chosen by `(2)`; default `2` (Multiple)". | Firmware string `"Display lines:\r\n 1. 1 \r\n 2. Multiple"` in `messages_v3.s` / `messages_v4.s`; `getnumber(2,1,2,0)` default 2. |
| 10 | fixed | `README.md` adds a “Names used in this set” table: `RW`/`R/W`, `Vpullup`/`VPU`/`Vpu`, `3.3V`/`5.0V` vs `3.3 V`/`5.0 V`, `Multiple`, `unverified`/`not verified`. Prose now uses `R/W` and `3.3 V`/`5.0 V`; captures keep the firmware forms. | `Firmware/core_messages/messages_v4.s`: `"VPU"`, `"Vpullup"` warnings, `"Vpu=3V3"`/`"Vpu=5V"` markers, `"Display lines:... 2. Multiple"`, `"Open drain outputs..."`. |
| 11 | fixed | `README.md` no-pull-up bullet now says “**No pull-up is required** in the normal case; a binary SPI session that left the outputs open drain is the exception” and links the open-drain section. | `Firmware/capability_hd44780/hd44780.c`, `LCDsetup_exc()` does not clear the ODC bits; `Firmware/protocol_binary/binary_io.c` can set them; `Firmware/protocol_spi/spi.c`, `spi_cleanup()` clears them only for the terminal SPI path. |
| 12 | fixed | `entry-and-setup.md` adds “The direct form takes no setup answers: in a build that has LCD at entry 9, `m9 2` enters the mode and ignores the trailing `2`; select the line mode with `(2) 1` or `(2) 2` after `W`.” | `Firmware/core_system/proc_menu.c`, `changemode()`: `getint()` reads the mode number, the LCD `setup_prepare()` reads no answers, then `cmdstart = (cmdend - 1) & CMDLENMSK` discards the rest of the line. |
| 13 | fixed | `README.md` splits the build/flash row: building says “select the features and build the custom image”; a new row links `docs/dev/flashing.md` for “write the built image to the Bus Pirate”. | `docs/dev/flashing.md` exists and is the flash page; `entry-and-setup.md` already linked it. |

## Code-owner candidate

**The LCD status line reports open drain from a mode flag, not from the pad
state.**  `Firmware/capability_hd44780/hd44780.c`, `LCDsetup()` sets
`mode_configuration.high_impedance = 1` (comment “yes, always HiZ”).
`LCDsetup_exc()` configures PPS, TRIS, SPI1CON1/CON2 and CS, but never writes
`BP_MOSI_ODC`, `BP_CLK_ODC` or `BP_CS_ODC`.
`Firmware/core_system/proc_menu.c`, `print_status_info()` prints
`Open drain outputs (H=Hi-Z, L=GND)` whenever `high_impedance == YES`.
Consequence: on a clean entry `i` says open drain while the SPI pads are
push-pull; after a binary SPI session left the ODC bits set, the same text is
correct but the mode cannot distinguish the two cases.  Code owner to decide
whether `LCDsetup_exc()` should clear the ODC bits (and lose the carry-over) or
whether the status should be derived from the per-pin ODC state.  The docs now
describe the current behaviour and the power-cycle recovery.

## Cross-set changes for the main session

Required (finding 1):

1. `docs/dev/building.md:153-157` — replace the stale link-failure bullet:
   “The all-features v4 list also enables the HD44780 adapter.  Its SPI output
   helper has no external definition when SPI support is also enabled (the
   `spi.c` implementation is `static`), so that combination is expected to
   fail at link time; `unverified`” with the current fact: the v4 all-features
   build includes LCD and SPI and links; the LCD driver uses its own bounded
   byte helper and `spi_write_byte()` is exported.  On v3 the custom set still
   stops at the SMPS `#error`.
2. `docs/dev/building.md` status line — “No build was run for this page” and
   “not verified on a build” are no longer accurate for the LCD combination:
   the v4 custom build was run and linked.  If the page keeps a build-status
   note, record the measured v4 all-features result: 79,485 bytes program
   (31%) and 11,898 bytes data (72%), XC16 2.10, no warnings.

Consistency follow-ups from the same evidence:

3. `docs/dev/code-map.md:46` — the LCD row says the registered cleanup is
   `spi_disable_interface`; the strong hook is now `hd44780_cleanup` (the
   standalone core keeps `spi_disable_interface` as a weak fallback).  Suggested
   text: “registered in `core_system/core.c` (`"LCD"` entry, cleanup
   `hd44780_cleanup`; weak fallback `spi_disable_interface`), local transfer
   helper `hd44780_spi_write_byte()`”.
4. `docs/dev/verification.md:46` — the LCD row says “pin-out, glyphs and the
   SPI link are code-derived”.  The pin-out/connector/supply facts now come
   from the legacy design files in `hardware-other` (v2/v2b/v3/v4-v4b
   schematics and the v3a photo/model); the on-glass glyphs and timing remain
   code-derived and unverified.  Suggested replacement: “no adapter available
   for an electrical check; board revisions, connector and supply from the
   legacy design files; on-glass glyphs and timing code-derived”.

No `docs/protocol/spi/` change is needed: the SPI set does not mention LCD.

## Remaining questions

- **v3 + LCD build fit** — not measured.  The old 95% figure was removed; the
  page now says the v3 part is the smaller device and to check the link result.
  Building a v3 image with `BP_ENABLE_HD44780_SUPPORT` added to the default set
  would settle it.
- **v3 hardware** — no v3 board or adapter was available; the v3 column remains
  build-only and unverified.
- **v4b logic margin** — a 74HC595 at 5 V has a CMOS input threshold above the
  Bus Pirate's 3.3 V high.  The docs recommend the 3.3 V setting when the glass
  allows it and say to confirm the populated part; no board was measured.
- **v3a identity** — the repository has the v3/v3a 3D asset and photo, not a
  separate v3a schematic/BOM.  The v3a pots and jumpers are documented from the
  photo; the underlying design line is v2b/v3.
- **Passive v4 board** — the repository's `BPv4-PassiveLCD` board is a pin
  breakout without a 595.  If the public “Bus Pirate v4 LCD adapter” page
  describes that board rather than the v4b, the external link is ambiguous; the
  local design files are the safer reference.
- **External Dangerous Prototypes pages** — not fetched (403); the local
  schematics and assets were used instead.
- **Activity LED sense** — high = LED on in the published schematics; a clone
  may invert it.  The table says “published designs”.
- **Capture blank lines** — the W/w and syntax-error captures keep the set's
  existing transcription style, which omits some empty lines produced by
  `bpBR`.  If byte-exact transcripts are required, add the blank line after
  `POWER SUPPLIES ON` and before the next prompt.

## Verification

- Read `feedback-lcd-round1.md`, all eight LCD files, `docs/TEMPLATE.md`,
  `docs/SPEC.md` where linked, and the documentation guidelines.
- Re-read the firmware paths: `capability_hd44780/hd44780.c/.h`,
  `protocol_spi/spi.c/.h`, `core_system/core.c`, `proc_menu.c`, `base.c`,
  `configuration.h`, and the LCD message strings in `messages_v3.s` /
  `messages_v4.s`.
- Ran the v4 all-features build (command above).  It linked; map symbols and
  memory use are recorded in the blocker note.  Build products were removed
  afterwards.
- Recovered the generated `Firmware/build/message-tests` and
  `Firmware/build/shared-format-tests` fixtures by rerunning
  `Firmware/tests/test_messages.py` and `test_shared_format.py` after the
  build clean.
- Checked every local link and heading anchor in the LCD set; 0 broken.
- Checked for TABs, prose lines over 80 columns and inconsistent table column
  counts; none.
- No commit; no WORKLOG edits; no files outside `docs/capabilities/lcd/` and
  this record were changed.  Pre-existing JTAG worktree changes were not
  touched.

## Files changed

- `docs/capabilities/lcd/README.md`
- `docs/capabilities/lcd/entry-and-setup.md`
- `docs/capabilities/lcd/wiring.md`
- `docs/capabilities/lcd/display.md`
- `docs/capabilities/lcd/commands.md`
- `docs/capabilities/lcd/usage.md`
- `docs/capabilities/lcd/edge-cases.md`
- `docs-sweep/fixes-lcd-round1.md` (this record)

`spi-link.md` was reviewed and needed no change.
