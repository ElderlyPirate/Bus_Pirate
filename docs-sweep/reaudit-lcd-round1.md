# LCD re-audit — round 1

Scope: the 13 findings in `feedback-lcd-round1.md`, re-read against the current
`docs/capabilities/lcd/` files. I also checked the three cross-set files and the
code-owner candidate. I did not run a fresh full review.

**Verdict: 13 of 13 items filled in full.** No original finding remains open.
Three minor new/cross-set items are listed at the end.

## Per-item table

| # | Status | Evidence (current docs) | Remainder |
|---:|---|---|---|
| 1 | Filled in full | `entry-and-setup.md:8-10`: "**Build status:** the v4 all-features build includes the LCD and SPI modules together and builds; the LCD path is self-contained, so SPI support does not block it." `README.md:37`: "on v4 the all-features build includes it and builds with SPI enabled." `entry-and-setup.md:24`: "**includes LCD and SPI; builds**." | None in the LCD set. Cross-set `dev/building.md:154-159` now records the measured link; its Verification section at line 289 still says "Not run for this page" (main-session cleanup, not an LCD-page gap). The v3 fit is still "check the link result" — honest, not measured. |
| 2 | Filled in full | `wiring.md:124-130` names "display VDD (pin 2)" = "adapter VCC" and "display VSS (pin 1)" = "GND"; `wiring.md:132-135`: "Most HD44780 glasses are 5 V parts; check the datasheet. ... A 3.3 V glass on 5.0 V, or a 5 V glass on 3.3 V, can stay blank or be damaged." `wiring.md:140-141`: "**`W` is still required with a separate feed.**" `entry-and-setup.md:229` retitles the leaving table "Display VDD source"; `wiring.md:161-164`: "The display supply decides what happens when the mode is left. A display whose VDD is on the adapter's Bus Pirate rail loses power and clears; a display with its own VDD feed keeps its content." | None. |
| 3 | Filled in full | `wiring.md:22`: the v3-era line now names "v2, v2a, v2b, v2c, v3, v3a"; `wiring.md:23`: the v4-era line names "v4 adapter v3 (2011), v4b"; `wiring.md:29-35` embeds the v2 and v3a photos with captions; `wiring.md:37` links the v3a zip; `wiring.md:25-27` names the passive v4 breakout. `README.md:81-84`: "No firmware backlight or contrast setting. ... The v4b can be jumpered for fixed on or AUX0 control." | The v3a caption names Contrast/Backlight but not the Power jumper (small polish). The v4b AUX0 wording at `display.md:164` is a new minor issue — see below. |
| 4 | Filled in full | `README.md:44-45`: "On a stock image `m9` is `PIC` on v4 and `DIO` on v3, not LCD; if `LCD` is absent from the `m` list, the image does not have it." `README.md:43`: "in a build that includes it, the number is positional". | None. |
| 5 | Filled in full | `usage.md:82-83`: "The glass should show `Hello` from column 1, with the cursor after the last character." `usage.md:103-104`: "The glass should now show `Bus Pirate` on row 1; `Hello` has been overwritten from the home position. Row 2 is still blank." `usage.md:147`: "`(2)` should leave the glass cleared at home; `(3)` clears it again." `usage.md:158-159` gives the 20x2/16x2 result for `(6) 20`; `usage.md:224-238` gives the 5x8 bitmap and "The glass should show this outline at the home position". `display.md:84-90` adds the 16x2/20x2/20x4 expected-result table. | The glyph intro says "the 5x7 glyph plus its cursor row" while the 8th row is drawn as part of the shape — see below (minor). |
| 6 | Filled in full | `entry-and-setup.md:142-146`: "The `Open drain outputs` text is the mode's output-type flag: entering the mode sets that flag, but the LCD setup does not write the per-pin open-drain bits. On a clean entry those bits are clear, so the SPI pins are driven normally." `README.md:55-56`: "`i` still says `Open drain outputs`; that is the mode's flag, not the pad state." | None. The code-owner candidate remains open by design; the docs describe the current behavior and the power-cycle recovery. |
| 7 | Filled in full | `entry-and-setup.md:26`: "the v3 part is the smaller device; check the link result" and "the all-features build leaves room". `entry-and-setup.md:40-41`: "In the all-features v4 build, LCD is entry **9**." | None. No percentages in the LCD pages; the measured numbers stay in `dev/building.md` (maintainer page). |
| 8 | Filled in full | `display.md:37`: "Without an argument, with `0`, or with an argument outside `1`-`2`, the macro prompts". `commands.md:127`: "If the argument is missing, `0`, or outside `1`-`2`". `edge-cases.md:21`: "a missing, `0`, or outside `1`-`2` argument is treated as not given". | None. |
| 9 | Filled in full | `entry-and-setup.md:114`: the defaults row now reads "chosen by `(2)`; default `2` (Multiple)". | None. |
| 10 | Filled in full | `README.md:24-32`: the "Names used in this set" table maps "`RW` (pin table) / `R/W` (prose)", "`Vpullup` (warning) / `VPU` (`v` table) / `Vpu` (v4 select prompt)", "`3.3V` / `5.0V` (captures)" vs "`3.3 V` / `5.0 V`", "`Multiple`", and "`unverified` / `not verified`". | None. |
| 11 | Filled in full | `README.md:53-56`: "**No pull-up is required** in the normal case; a binary SPI session that left the outputs open drain is the exception ... `i` still says `Open drain outputs`; that is the mode's flag, not the pad state." | None. |
| 12 | Filled in full | `entry-and-setup.md:70-72`: "The direct form takes no setup answers: in a build that has LCD at entry 9, `m9 2` enters the mode and ignores the trailing `2`; select the line mode with `(2) 1` or `(2) 2` after `W`." | None. |
| 13 | Filled in full | `README.md:105-106`: separate related-docs rows: `docs/dev/building.md` "select the features and build the custom image" and `docs/dev/flashing.md` "write the built image to the Bus Pirate". | None. |

## Five focus checks

- **Real build path:** yes. `entry-and-setup.md:8-12` has a build-status box;
  `entry-and-setup.md:21-35` gives the v4 all-features and v3 own-set paths;
  `README.md:36-40` points to it. `dev/building.md:154-159` now records the
  measured v4 all-features link (79,485 bytes program / 31%, 11,898 bytes data
  / 72%, XC16 2.10, no warnings). The v3 fit is explicitly "check the link
  result", which is the honest state.
- **Display supply:** yes. `wiring.md:117-164` has the display VDD/VSS/V0/
  backlight table, the 5 V-glass warning, the separate-feed rule, and "`W` is
  still required with a separate feed." The `w` and leaving behaviour is keyed
  to the **display VDD source** in `entry-and-setup.md:229-232`,
  `usage.md:267-270`, and `edge-cases.md:117-119`.
- **Adapter revisions:** yes. `wiring.md:20-40` names the v3-era line
  (v2-v3a), the v4-era line (v4 adapter v3 2011, v4b), the passive v4
  breakout, embeds the two photos, and links the v3a design zip.
- **Stock `m9` trap:** yes. `README.md:44-45` states the v4 PIC / v3 DIO
  mapping and the "if `LCD` is absent" check in the five-minute version.
- **What the glass should show:** yes. The `usage.md` sessions now give the
  expected text, the `(6) 20` 20x2/16x2 result, the `(2)`/`(3)` clear result,
  the glyph bitmap, and `display.md:84-90` adds the geometry table.

## Cross-set status

- `dev/building.md:154-159`: updated. It now says the LCD and SPI combination
  links and records the measured v4 result. `dev/building.md:3-6` also says the
  measured link is recorded.
- `dev/building.md:289`: still says "Not run for this page"; `dev/building.md:49`
  still says "was not exercised for this page - **not verified** on a build."
  Those are stale for the measured v4 all-features combination. The main
  session should reconcile the Verification section with lines 154-159.
- `dev/code-map.md:46`: updated to `hd44780_cleanup` (weak fallback
  `spi_disable_interface`) and `hd44780_spi_write_byte()`.
- `dev/verification.md:46`: updated to "no adapter available for an electrical
  check; board revisions, connector and supply from the legacy design files;
  on-glass glyphs and timing code-derived."
- **Code-owner candidate:** the docs now describe the current behavior:
  `LCDsetup()` sets the mode's `high_impedance` flag, `LCDsetup_exc()` does not
  write the per-pin ODC bits, `i` reports the flag, and a binary carry-over is
  indistinguishable from the normal status. The code owner still decides
  whether to clear the ODC bits or derive the status from them; no docs change
  is needed for that decision.

## Presentation check

| Rule | Result | Evidence |
|---|---|---|
| No TAB characters | Pass | 0 tabs in all eight LCD files. |
| Aligned captures | Pass | `usage.md:51-54` token starts are 0,9,18,27,36,45,54,63,72,81,90,99 on every row; the glyph is 8 rows × 5 characters. |
| Menus / option lists as tables | Pass | The `m` menu, `(2)` prompt and `(0)` list are firmware code captures; the wiring, supply, confirm and geometry material is in tables. No bullet-chain option list. |
| ~80-column wraps | Pass | No prose line over 80 columns (tables and code captures excluded). |
| No bench prose, dates, tested/verified-result prose | Pass with one note | No dates, pass counts, evidence sections, provenance, internal log names, or resource percentages in the LCD pages. The only date-like string is "v4 adapter v3 (2011)", a board-revision label, not a run date. The build status says "builds" but carries no toolchain, memory, warning, or run narrative; the measured record is in `dev/building.md`. |
| No resource percentages in user docs | Pass | No `%` in the LCD pages. The 31% / 72% figures are in `dev/building.md` (maintainer page). |

## New problems (max 5)

1. **[minor]** `display.md:164`: "backlight control | none as a mode setting;
   board pots/jumpers only (the v4b can use AUX0)". "Board pots/jumpers only" is
   inaccurate for the v4b: `wiring.md:151-157` says the v4b AUX0 jumper lets the
   shared `A` / `a` command drive the backlight. The README's "No firmware
   backlight or contrast setting" is defensible as "no LCD-mode setting", but
   the display row should say "none as an LCD-mode setting; the v3a has a manual
   pot; the v4b can be jumpered to AUX0, which the shared `A` / `a` command
   drives (select AUX0 with `k` / `K`)."
2. **[minor]** `usage.md:224`: "The eight row bytes form this 5x8 cell (the 5x7
   glyph plus its cursor row)". The 8th row in the bitmap is drawn as the
   bottom of the glyph; the cursor is at the next position. The phrase can
   imply the 8th row is a cursor underline. Better: "this 5×8 cell; the 8th row
   is also where the cursor underline appears."
3. **[minor, cross-set]** `dev/building.md:289`: "Not run for this page" and
   `dev/building.md:49`: "was not exercised for this page - **not verified** on
   a build", while `dev/building.md:154-159` records the measured v4
   all-features link. The Verification/status lines should distinguish the
   general page from the measured LCD combination.
