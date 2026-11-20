# LCD Phase F re-audit — RA-17

**Verdict: 5 of 5 filled in full. The set is clean for RA-17.**

## Per-item table

| # | Status | Evidence (current docs) | Remainder |
|---:|---|---|---|
| 1 | Filled in full | `entry-and-setup.md:137`: "POWER SUPPLIES ON, Pull-up resistors OFF, Normal outputs (H=3.3v, L=GND)". `entry-and-setup.md:142-146`: "Before `W`, the supply field reads `POWER SUPPLIES OFF`.  The `Normal outputs` text is the mode's output-type setting: entering the mode selects normal outputs, and the LCD setup writes the MOSI/CLK/CS open-drain bits off, so the SPI pins are driven push-pull". `entry-and-setup.md:187-189`: "The setup configures the SPI pins, writes the MOSI/CLK/CS open-drain bits off, and selects the **data** register." | None. The capture, the explanation and the `W` bullet are all updated. |
| 2 | Filled in full | `spi-link.md:114-118`: "The LCD mode uses normal push-pull outputs.  Entering the mode selects normal output, so `i` reports `Normal outputs (H=3.3v, L=GND)`.  The LCD setup writes the MOSI/CLK/CS open-drain bits off, so a previous binary SPI session's open-drain setting is cleared when `W` runs.  No pull-up or recovery step is needed." | None. The obsolete carry-over paragraph and `#open-drain-carry-over` link are gone. The optional fixed-settings row was not added; it was not required. |
| 3 | Filled in full | `README.md:54-56`: "**No pull-up is required**: the LCD setup selects normal push-pull outputs and clears the SPI open-drain bits; `i` reports `Normal outputs (H=3.3v, L=GND)`." `README.md:31` adds "`3.3v` (status line)" to the names table beside `3.3V` / `5.0V`. | None. The old binary-session exception and the "`i` still says `Open drain outputs`" sentence are gone. |
| 4 | Filled in full | `wiring.md:168-177`: "**No pull-up is required** on MOSI, CLK or CS in the normal case. ... The LCD setup selects normal push-pull outputs and writes the SPI open-drain bits off." and "One board-level requirement:" followed only by the output-enable/master-reset bullet. | None. The second carry-over bullet and its link are gone. |
| 5 | Filled in full | `edge-cases.md:31` is now the SMPS-feedback row; the "open-drain carry-over from a binary SPI session" row is absent. The `## Open-drain carry-over` section is absent; the text goes from "Text before initialisation" directly to "Leaving and power". | None. No `#open-drain-carry-over` link remains anywhere in the LCD set. |

## Checks

- **Firmware match:** the current `Firmware/capability_hd44780/hd44780.c:175-187`
  sets `mode_configuration.high_impedance=NO` in `LCDsetup()` and writes
  `BP_MOSI_ODC=OFF`, `BP_CLK_ODC=OFF`, `BP_CS_ODC=OFF` in `LCDsetup_exc()`.
  `core_system/proc_menu.c:1610-1615` selects `BPMSG1121` for normal outputs;
  `core_messages/messages_v4.s:389` / `messages_v3.s:359` is
  `Normal outputs (H=3.3v, L=GND)`. The docs match.
- **Dangling anchor:** no `#open-drain-carry-over` or `Open drain outputs`
  text remains in the LCD set. The only remaining open-drain wording is the
  positive "open-drain bits off" prose.
- **Links/anchors:** every local link and heading anchor in the eight LCD files
  resolves (checked with a link/anchor scan).
- **Presentation:** no TAB characters in the eight files; no prose line over
  80 columns (tables/code captures excluded); the changed status capture is a
  single code line under the limit; the failure table still has four columns
  after the row deletion; the README names table and the status capture are
  aligned; no dates, bench prose, pass counts or resource percentages in the
  changed passages.
- **Cross-set:** no change required. The previously checked
  `auxiliary/aux-pin.md` text is about the AUX pin, not MOSI/CLK/CS;
  `protocol/spi/binary.md` describes the binary SPI entry defaults, not the LCD
  terminal setup; `generic/navigation.md:104` is a generic `i` example. None
  contradicts this set.

## Notes

- The README names table now covers both firmware forms of the rail string:
  `3.3V` / `5.0V` for prompts/captures and `3.3v` for the status line. That
  resolves the casing note from the previous audit.
- The `spi-link.md` "leaving SPI ... clears its open-drain setting" sentence
  remains accurate for the terminal SPI cleanup; it is not a carry-over claim.
