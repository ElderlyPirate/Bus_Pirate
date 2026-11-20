# SPI docs — round 1 fixes

Scope: `docs/protocol/spi/**` only. No firmware, binary-set, generic-set or
WORKLOG edits. No commit.

**Result:** 16 findings fixed in-set plus 1 cross-set finding (finding 4);
0 rejected. Finding 8 also raises one code-owner candidate (the `0x02`
CS-direction limitation). Extra turn: #8 propagation, the binary `0x5x`
safety warning and the #17 row widths are fixed.

## Per-finding record

| # | Decision | Change (file + short quote) | Evidence |
|---:|---|---|---|
| 1 | fixed | `entry-and-setup.md`: “on exit, runs the master pin/module setup … does not restore the input state”. `sniffer.md`: “CLK, MOSI and CS become outputs; it does not restore the previous input state”. `wiring.md` and `binary.md` say the same; `wiring.md` now says the master configuration **can** drive before the macro. | `Firmware/protocol_spi/spi.c`, `spi_sniffer()`: `spi_disable_interface()`/`spi_slave_enable()` on entry; `spi_slave_disable()` then `spi_setup(...)` on exit. `spi_setup()` sets `SPICS/CLK/MOSI_TRIS = OUTPUT` and enables the module; it does not touch the regulator. |
| 2 | fixed | `wiring.md`: bold “**Set Vpullup to the target's logic supply.** A 5 V pull-up on a 3.3 V target can damage it”; `e2` = 3.3 V, `e3` only for 5 V; target rails vs Vpullup reference stated. Added the same warning row in `wiring.md` and `edge-cases.md`. | `Firmware/hardwarev4.h`, `bp_enable_3v3_pullup()` / `bp_enable_5v0_pullup()`; `Firmware/core_system/proc_menu.c`, `set_pullup_voltage()` cases 2/3. No firmware over-voltage guard exists, so the warning is the only protection. |
| 3 | fixed (order corrected) | `wiring.md`: numbered “Bring-up order with the on-board pull-ups (v4)” = wire, open drain, `e2`/`e3`, `W`, `P`, `v`. `README.md` and `entry-and-setup.md` link it and state `e` before `W`, `P` after. | `set_pullup_voltage()` routes the selected rail but only allows open drain and no existing Vpullup voltage; `switch_psu_on()` is what enables the rails; `P` warns when the VPU ADC is below `0x50`. The auditor's `e → P → W` order can make `P` warn because the on-board rails are only switched on by `W`; `e → W → P` avoids that ordering. |
| 4 | cross-set | No SPI edit needed; `bus-operations.md` already says `;N` changes the displayed width and bit-order window, not the transfer width. Recorded the `docs/generic/values.md` change below. | `Firmware/core_system/proc_menu.c` number/read handlers set `mode_configuration.numbits` for display and `bp_reverse_integer`; `Firmware/protocol_spi/spi.c`, `spi_write()` calls `spi_write_byte()` (`uint8_t`), so only the low byte reaches the bus. |
| 5 | fixed | `sniffer.md` §Usage is now a five-step procedure: target disconnected, `(1)`/`(2)`, connect target, observe, stop while the master is quiet; example transcript and exit pin state follow. | `spi.c`, `spi_run_macro()` prints `Sniffer`/`Any key to exit` then calls `spi_sniffer()`; the interactive exit consumes one key and prints `bpBR`; `spi_sniffer()` then runs the master setup. |
| 6 | fixed | `entry-and-setup.md` table now separates “`m` [Enter], then the number at `(1)>`” from “`m5` or `m 5`” and “`m5 <six values>` or `m 5 <six values>`”; the “m alone” rule is next to the rows. | `Firmware/core_system/proc_menu.c`, `changemode()`: skips spaces, `has_argument` is true when a number follows `m`; only a missing argument opens the menu. `spi_setup_prepare()` reads the six values with `getint_with_spaces()`. |
| 7 | fixed | `README.md` `{` bullet now says “The flag is sticky: `]`/`}` and `w`/`W` do not clear it; `[` does.” | `spi.c`, `engage_spi_cs()` sets `write_with_read`; `spi_start()` clears it; `spi_stop()` does not. `spi_cleanup()` only disables the interface; `spi_setup_execute()` does not clear it. |
| 8 | fixed (recovery corrected and propagated); code-owner candidate noted | `binary.md` and `edge-cases.md` now describe the observable symptom (“transfer is ignored; CS never goes low”) and the real recovery: another `0x4x` with bit 0 clear, or `0x6x`/`0x8x`. They explicitly say “`0x02` writes only the latch” and cannot restore an input CS. Every remaining `0x02` instruction and the command-table entry now require CS to be an output. | `Firmware/protocol_spi/spi.c`, `SPI_BASE_COMMAND_CS_LOW`/`CS_HIGH` only write `IOLAT`; `Firmware/protocol_binary/binary_io.c`, `bp_binary_io_peripherals_set()` is the only place that changes CS `IODIR`. `spi_setup()` restores `SPICS_TRIS = OUTPUT`. |
| 9 | fixed | `binary.md` entry note adds “`0x06` is the `BBIO1` OpenOCD/JTAG entry but the extended AVR helper inside `SPI1`”; version reply explained as big-endian `0x0001` (version 1); zero-length bulk read returns the two acks and no data; availability now “present in the default v3 and v4 builds; a custom build without the extended AVR option answers `0x00`”. | `Firmware/protocol_binary/binary_io.c`, `BITBANG_COMMAND_OPENOCD = 0x06`; `Firmware/protocol_spi/spi.c`, `handle_extended_avr_command()` sends the `0x06` ack, then the sub-reply; version constant `0x0001`; length 0 passes bounds and the `while (length > 0)` loop does nothing. `Firmware/configuration.h` defines `BP_SPI_ENABLE_AVR_EXTENDED_COMMANDS` for both default builds. |
| 10 | fixed | `README.md` status is “v3 and v4 documented; the v3 hardware behaviour is not verified”; build row is “included in the default build”; one table caveat remains and the duplicate limit bullet is gone. `build-only` removed from the detail-page caveats. | Documentation status only; no behavior claim. The v3 build is present in the source tree but no v3 hardware verification exists for this set. |
| 11 | fixed | `README.md` defines the *terminal output buffer* (4096 B) and the *command buffer* (128/256 B) as separate buffers; availability rows and `sniffer.md`/`entry-and-setup.md`/`binary.md` use “terminal output buffer”. | `Firmware/configuration.h`: `BP_TERMINAL_BUFFER_SIZE 4096`, `BP_COMMAND_BUFFER_SIZE` 128 (v3) / 256 (v4). `user_serial_ringbuffer_setup()` uses the terminal buffer. |
| 12 | fixed | `macros.md`: “The v3 behaviour is **not verified**” (no invented v3 column). Added a `(11)` example: header changes from `( 4 0 1 0 1 0 )` to `( 4 1 1 0 1 0 )`. | `spi.c`, `spi_run_macro()` cases 10–15 set `spi_state`, write `SPI1CON1bits`, then `spi_print_settings()`. |
| 13 | fixed | `entry-and-setup.md`: heading “Menu-form example”; prose “The default v4 menu is shown”; closing note no longer says “capture”. | Firmware strings in `Firmware/core_messages/messages_v4.s`: menu list, speed menu and prompt text; `getnumber()` prints the blank line before `(n)>`. |
| 14 | fixed (template order kept) | `README.md` adds a “**Start here:** [The five-minute version]” link, keeps the map first per `docs/TEMPLATE.md`, completes the binary bullet (`0x02`/`0x03`, `0x4x`, `0x6x`) and adds `o`, `l`/`L` to the shared grammar. Every detail page now links `edge-cases.md`. | `docs/TEMPLATE.md` README skeleton puts the file table before the five-minute version. `Firmware/protocol_spi/spi.c` command enum confirms the binary groups; `proc_menu.c` handles `o` and `l`/`L`. |
| 15 | fixed | `binary.md`: “Every `0x4x` request also drives AUX as an output”; `0x44`/`0x4C`/`0x4D` comments now say `AUX low`; worked frame comment updated. | `Firmware/protocol_binary/binary_io.c`, `bp_binary_io_peripherals_set()`: `BP_AUX0_LAT = (inByte & 0x02) ? ON : OFF; BP_AUX0_DIR = OUTPUT;` runs for every `0x4x`. |
| 16 | fixed | `sniffer.md`: boundary wording is now concrete (“closing `]` can be delayed until the next `[` is pending … markers can print together before the next byte pair”); added “send `0x01` to stop and immediately get `SPI1` back; send `0x00` to exit to `BBIO1`”. Same example added to `edge-cases.md`. | `spi.c`, binary sniffer leaves the stopping byte pending; the SPI command loop then handles `0x01` as `SPI_BASE_COMMAND_SEND_IDENTIFIER` and `0x00` as exit. Marker ring drains before byte pairs in `spi_sniffer()`. |
| 17 | fixed (extra turn reflow) | Shortened the cited rows: `edge-cases.md` CS-direction rows, `binary.md` CS-direction rows and `bus-operations.md` no-CS row. Reflowed the remaining cited wide rows (`edge-cases.md` unpowered-target row and `sniffer.md` `0x01` stop row) below 130 characters. | Presentation change only; same facts as the code evidence above. |

## Extra fix turn (re-audit)

| Item | Change | Evidence |
|---|---|---|
| #8 propagation | `binary.md` command table now says “CS latch low (asserts only while CS is an output)”; the bulk (`0x1x`) and `0x06` prose and the edge-case rows in `binary.md` and `edge-cases.md` now qualify `0x02` with “while CS is an output”; `README.md` says “`0x02`/`0x03` write the CS latch low/high (assert/release while CS is an output)”. The `0x4x` bit-0-clear and `0x6x`/`0x8x` recovery remains consistent. | Same code evidence as finding 8: `spi.c` CS handlers write `IOLAT` only; `bp_binary_io_peripherals_set()` changes `IODIR`; `spi_setup()` restores `SPICS_TRIS = OUTPUT`. |
| New safety issue: binary `0x5x` | `binary.md` §Electrical setup and §Pull-up feed now say: “**Match the Vpullup source to the target's logic supply:** use `0x51` for a 3.3 V target and `0x52` only for a 5 V target. A 5 V pull-up can damage a 3.3 V target.” Both link `wiring.md` §The pull-up reference; `binary.md` also lists `wiring.md` in Related docs. | `Firmware/hardwarev4.h`, `bp_enable_3v3_pullup()` / `bp_enable_5v0_pullup()`; `Firmware/core_system/proc_menu.c`, `set_pullup_voltage()` cases 2/3. No firmware over-voltage guard. |
| #17 row reflow | `binary.md` CS-direction rows are now 108 and 122 characters; `edge-cases.md` CS-direction rows are 114 and 109 characters. The previously widest `edge-cases.md` unpowered-target row and `sniffer.md` `0x01` stop row are also shortened. | Presentation change only. |

## Code-owner candidate

**`0x02` cannot assert CS after a `0x4x` released the pin as an input.**
`Firmware/protocol_spi/spi.c`, `SPI_BASE_COMMAND_CS_LOW` does only
`IOLAT &= ~CS;`; `SPI_BASE_COMMAND_CS_HIGH` does only `IOLAT |= CS;`. Neither
changes `IODIR`. The only CS direction changes are in
`Firmware/protocol_binary/binary_io.c`, `bp_binary_io_peripherals_set()`
(`0x4x`), and `Firmware/protocol_spi/spi.c`, `spi_setup()` (`0x6x`/`0x8x`).
Consequence: after an open-drain `0x4x` with bit 0 set, `0x02` writes the latch
but leaves CS an input, so the documented “assert” action fails. `0x04` shares
the same latch-only path (`spi_read_write_io()` writes `SPICS` without changing
`TRIS`). The docs now describe the actual behavior and recommend `0x4x` bit 0
clear or `0x6x`/`0x8x`. Code owner to decide whether the CS commands should
also set `CS` as an output.

## Cross-set changes for the main session

Required:

1. `docs/generic/values.md` §Width — remove the absolute “`;N` sets the
   read/write width in bits” wording and the `0x55;4` / `r;16` transfer
   promises. Add: in a byte-oriented mode (SPI), `;N` changes only the displayed
   width and bit-reversal window; the mode page owns the transfer width, so
   `r;16` in SPI clocks one byte. Link
   `docs/protocol/spi/bus-operations.md` §Bit and word transfers.
2. `docs/generic/values.md` is also cited from `docs/generic/commands.md` line
   74 (“`;N` set the read/write width in bits”); apply the same byte-oriented
   caveat there.
3. `docs/generic/commands.md` §Feeding the Vpullup pin — the current order
   “`e2`/`e3`, then `P`” can make `P` warn because the on-board rails are not
   switched on until `W`. State the working order: `e2`/`e3` → `W` → `P` (or
   explain that the on-board Vpullup source is fed by the rails `W` switches
   on).

Consistency follow-ups (from the same findings):

4. `docs/generic/navigation.md` §Mode menu — the same `m`-alone ambiguity:
   change “`m`, then a number” to “`m` [Enter], then the number at the `(1)>`
   prompt” and add `m <N>` / spaces-allowed to the direct-form row.
5. `docs/generic/README.md` availability — rename “terminal command buffer” to
   “command buffer (input line)” or add a separate “terminal output buffer”
   row, matching the SPI set's new buffer definitions.

## Remaining questions

- `0x02`/`0x03` CS-direction intent is a firmware-owner decision; the docs now
  describe the current latch-only behavior.
- The auditor's structural proposals (split `binary.md`, add a separate
  quickstart page, collapse the duplicated edge-case tables) were not applied.
  The eight-file split and the per-page tables remain; the README start-here
  link and `edge-cases.md` links on every detail page remove the worst
  navigation dead ends without a larger restructure.
- v3 hardware behavior remains unverified; no v3 board was available and no
  hardware tests were run.
- The sniffer's maximum usable rate and the exact marker/byte split at very high
  rates remain unspecified beyond the documented “not guaranteed lossless”
  limit.

## Verification

- Re-read and cross-checked the firmware paths for entry, `W`/`w`, `e`/`P`,
  the sniffer, the `0x4x`/`0x6x`/`0x8x` groups, `0x06`, and the terminal
  `;N` handling.
- Checked all local links in `docs/protocol/spi/`; no broken file targets.
- Checked for TABs and trailing whitespace in the SPI set; none.
- No commit; no WORKLOG edits; no files outside `docs/protocol/spi/` and this
  record were changed.

## Files changed

- `docs/protocol/spi/README.md`
- `docs/protocol/spi/entry-and-setup.md`
- `docs/protocol/spi/wiring.md`
- `docs/protocol/spi/bus-operations.md`
- `docs/protocol/spi/macros.md`
- `docs/protocol/spi/sniffer.md`
- `docs/protocol/spi/binary.md`
- `docs/protocol/spi/edge-cases.md`
- `docs-sweep/fixes-spi-round1.md` (this record)
