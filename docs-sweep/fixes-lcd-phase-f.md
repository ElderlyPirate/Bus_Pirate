# LCD Phase F delta repair — RA-17

Scope: `docs/capabilities/lcd/**` plus this record.  No firmware edits, no
other worktrees, no commit.  Verified against the final tree in this worktree
(`Firmware/capability_hd44780/`, `core_system/proc_menu.c`, `core_system/core.c`,
`core_messages/messages_v3.s` / `messages_v4.s`, `protocol_spi/spi.c`).

**Result:** 5 findings: **5 fixed**, **0 rejected**.  No cross-set change and
no code-owner candidate.

## Per-finding record

| # | Decision | Change (file + quote) | Evidence |
|---:|---|---|---|
| 1 | fixed | `entry-and-setup.md`: status capture is now `POWER SUPPLIES ON, Pull-up resistors OFF, Normal outputs (H=3.3v, L=GND)`; the paragraph says “entering the mode selects normal outputs, and the LCD setup writes the MOSI/CLK/CS open-drain bits off, so the SPI pins are driven push-pull”; the `W` bullet says “The setup configures the SPI pins, writes the MOSI/CLK/CS open-drain bits off, and selects the **data** register.” | `Firmware/capability_hd44780/hd44780.c`, `LCDsetup()` sets `mode_configuration.high_impedance=NO`; `LCDsetup_exc()` writes `BP_MOSI_ODC=OFF`, `BP_CLK_ODC=OFF`, `BP_CS_ODC=OFF`. `Firmware/core_system/core.c` registers `LCDsetup`/`LCDsetup_exc` as `setup_prepare`/`setup_execute`. `Firmware/core_system/proc_menu.c` selects `BPMSG1121` when `high_impedance != YES`; `messages_v3.s`/`messages_v4.s` define it as `Normal outputs (H=3.3v, L=GND)`. |
| 2 | fixed | `spi-link.md` §Output type is replaced with “The LCD mode uses normal push-pull outputs.  Entering the mode selects normal output, so `i` reports `Normal outputs (H=3.3v, L=GND)`.  The LCD setup writes the MOSI/CLK/CS open-drain bits off, so a previous binary SPI session's open-drain setting is cleared when `W` runs.  No pull-up or recovery step is needed.”  The obsolete `#open-drain-carry-over` link is gone. | Same `LCDsetup()` / `LCDsetup_exc()` evidence; the shared SPI path is not used for the carry-over claim.  `Firmware/protocol_spi/spi.c`, `spi_disable_interface()` still clears the SPI ODC bits, which supports the retained “leaving SPI … clears its open-drain setting” statement on the same page. |
| 3 | fixed | `README.md` pull-up bullet now says “**No pull-up is required**: the LCD setup selects normal push-pull outputs and clears the SPI open-drain bits; `i` reports `Normal outputs (H=3.3v, L=GND)`.”  The “Names used” table adds the status-line casing `3.3v` beside the prompt/capture forms. | Same firmware path; exact status string `Normal outputs (H=3.3v, L=GND)` in both message files. |
| 4 | fixed | `wiring.md` §Pull-up statement adds “The LCD setup selects normal push-pull outputs and writes the SPI open-drain bits off.”; “Two board-level and carry-over exceptions” becomes “One board-level requirement”; the binary-session carry-over bullet and its link are deleted. | Same `LCDsetup()` / `LCDsetup_exc()` evidence. |
| 5 | fixed | `edge-cases.md` deletes the “open-drain carry-over from a binary SPI session” failure-table row and the entire `## Open-drain carry-over` section; no replacement is needed because the positive statement is owned by `spi-link.md` and `entry-and-setup.md`.  No `#open-drain-carry-over` link remains. | Same firmware path; the failure no longer exists after RA-17.  All remaining local links and anchors resolve. |

## Verification

- Re-read `Firmware/capability_hd44780/hd44780.c`: `LCDsetup()` sets
  `high_impedance=NO` and `LCDsetup_exc()` writes the three SPI ODC bits off.
- Re-read `Firmware/core_system/core.c` (descriptor hooks),
  `Firmware/core_system/proc_menu.c` (status selection `BPMSG1121`), and the
  exact `Normal outputs (H=3.3v, L=GND)` strings in both message files.
- Checked `Firmware/protocol_spi/spi.c`: the retained SPI-mode cleanup sentence
  is still true (`spi_disable_interface()` sets the ODC bits to push-pull).
- Checked the LCD set for `#open-drain-carry-over` and `Open drain outputs`:
  none remain.  The only surviving open-drain wording is the positive
  “open-drain bits off” / “no open-drain bus” prose.
- Checked no TAB characters, no prose line over 80 columns, and no broken
  local links or heading anchors.
- No dates, bench prose, pass counts or resource percentages in the changed
  passages.

## Files changed

- `docs/capabilities/lcd/README.md`
- `docs/capabilities/lcd/entry-and-setup.md`
- `docs/capabilities/lcd/spi-link.md`
- `docs/capabilities/lcd/wiring.md`
- `docs/capabilities/lcd/edge-cases.md`
- `docs-sweep/fixes-lcd-phase-f.md` (this record)

No commit or push.
