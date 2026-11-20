# Binary docs — round 1 fixes

Scope: `docs/protocol/binary/**` only.  No firmware, other doc sets or WORKLOG
edits.  No commit.

**Result:** 16 findings fixed in-set, 0 rejected, 0 code-owner.  Three findings
(1, 2, 3) also require cross-set edits; finding 2 had no in-set defect.  One
extra in-set error was found and corrected while verifying (the `OCD1` reply to
an unknown byte), and one extra cross-set error is recorded (the I2C `0x09`
sub-result table).

## Per-finding record

| # | Decision | What changed (file + short quote) | Evidence |
|---:|---|---|---|
| 1 | fixed in-set; cross-set | `edge-cases.md`: "the 4096-byte terminal output buffer" and "This is the terminal output buffer, not the 128/256-byte command line." The number itself was already 4096 on both boards. | `configuration.h` `BP_TERMINAL_BUFFER_SIZE 4096` has no board split. `main.c` `bp_buffer[BP_TERMINAL_BUFFER_SIZE]` is `terminal_input`; `proc_menu.h` `cmdbuf[BP_COMMAND_BUFFER_SIZE]` is the separate 128/256 B command ring. `i2c.c` `i2c_write_then_read()` bounds both counts with `BP_TERMINAL_BUFFER_SIZE`. |
| 2 | no in-set defect; edge case added; cross-set | `sub-protocols.md` already said "a start still consumes its operand byte so the following request stays aligned". Added `edge-cases.md` §SMPS start on a build without SMPS: "the no-SMPS start path still reads its operand before it reports the refusal". | `binary_io.c` `handle_smps_command()`, `#else` branch: `if ((uint8_t)command >= SMPS_COMMAND_START) user_serial_read_byte();` then `REPORT_IO_FAILURE()`. |
| 3 | fixed in-set; cross-set | `entering-and-framing.md` §Leaving: "On an SMPS-enabled build it also stops the SMPS feedback loop". `commands.md` §Reset: "an SMPS feedback loop stopped". `sub-protocols.md`: "on v4 the terminal reset stops the loop". | `proc_menu.c`, NUL entry: `enter_binary_bitbang_mode()` then `reset_terminal_state()` → `bp_reset_board_state()`; `base.c` `bp_reset_board_state()` calls `smps_stop()` under `BUSPIRATEV4` + `BP_ENABLE_SMPS_SUPPORT`. |
| 4 | fixed | `README.md`: "`0x01` is not always success and `0x00` is not always failure … Use the command's reply shape". New `commands.md` §Reply shapes table (result byte, data word, count, level, leading `0x01` plus per-byte, stream, bus ACK). `entering-and-framing.md` §Reply and ACK rules: "The table is the usual case, not a universal rule". | `binary_io.h` result constants; `binary_io.c` `binary_io_self_test()` returns the error count, `bitbang_pin_state_get()` returns a level, `read_and_transmit_adc_measurement()` returns a word; `i2c.c` case `0b0001` transmits `bitbang_read_bit(true)` per byte, where 0 = ACK. |
| 5 | fixed | `examples.md` new §I2C byte write in `I2C1`: `0x02` start, `0x11 0xAA 0xBB` → `0x01 0x00 0x00`, `0x03` stop. `README.md` map row is now "worked BBIO1 and I2C exchanges, mode skeleton". `sub-protocols.md` §`0x3x`: "The reply is two bytes: `0x01` before the host's data byte is read and `0x01` after the bits are written." | `i2c.c` case `0b0001`: `REPORT_IO_SUCCESS()` then one transmitted ACK bit per written byte; `binary_io.c` `handle_bulk_byte_transfer()` has the same leading `0x01`. `handle_bulk_bit_transfer()` calls `REPORT_IO_SUCCESS()` before reading the byte and again after the bits. |
| 6 | fixed | `commands.md` new §Reading pins: "There is **no read-only five-pin command**", "Do **not** poll with `0x80`", and the `0x70 0x03 0x00 0x00` sample. `README.md` limits: "There is also **no read-only five-pin read**". `entering-and-framing.md` framing bullet now says the levels come back only from a write. | `bitbang_pin_state_set()` always writes the regulator, the pull-up latch and all five `IOLAT` bits, then reads the pins; `bitbang_pin_direction_set()` always writes all five `IODIR` bits. `handle_wait_command()` reads `bp_aux_pin_state_get()` without touching direction. |
| 7 | fixed | `README.md` status defines the v3 term; every availability table now uses "present, unverified" instead of "present (build-only)". | `configuration.h` default feature blocks include the protocol and capability features for v3; only AVRISP-MKII is restricted to v4 (`#error` guard). Status wording only. |
| 8 | fixed | `edge-cases.md` `0x40` recovery: "`0x0F` resets the pins, the supply and the pull-ups; send `0x13` first if a PWM timer must stop." `entering-and-framing.md` §Leaving: "It is not a substitute for `0x13`: the PWM timer is left running". `commands.md` §PWM: "`0x0F` does not stop the PWM timer either". | `bp_reset_board_state()` never writes `T2CON`, so the timer keeps running; it does not clear the PWM timer setup in the default build. (`smps_stop()` in an SMPS build also clears the shared OC5 channel, but still not `T2CON`.) `bp_aux_pin_reset_state()` only does `AUXPIN_RPOUT = OFF`. `handle_clear_pwm()` is the deliberate binary stop path. |
| 9 | fixed | `sub-protocols.md` §`0xFx`: "presets the **AUX and CS latches high** (MOSI, CLK and MISO latch low)". | `0xF1` = `1 111 0001`: bit 6 power, bit 5 pull-up, bit 4 AUX, bit 0 CS; `bitbang_pin_state_set()` writes each latch. |
| 10 | fixed | `README.md`: "v3 prints `aux` for `A0`". `commands.md` §The AUX pin uses the same mapping. | `messages_v3.s`: "c/C AUX assignment (aux/CS)"; `messages_v4.s`: "c/C/k/K AUX assignment (A0/CS/A1/A2)". |
| 11 | fixed | `README.md` names table adds `VREG`: "the on-board regulator that feeds the target supply; terminal `W` switches the same element". `entering-and-framing.md` §Wiring: "the same element as terminal `W`, which puts the 3.3 V and 5.0 V rails on their header pins"; the binary bit "runs no rail check and no bus setup". Removed "VCC pin". | `hardware.h` `bp_set_voltage_regulator_state()` and `bp_enable_voltage_regulator()` both drive `BP_VREGEN`; `proc_menu.c` `switch_psu_on()` enables it, checks `BP_ADC_3V3` and `BP_ADC_5V0`, then calls `setup_execute()`; `bp_binary_io_peripherals_set()` sets only the regulator. |
| 12 | fixed (threshold corrected) | `commands.md` §Frequency count: a `0x16` → `0x00 0x00 0x27 0x10` example (10000 Hz) and "Above about 65.5 kHz the first count is larger, so the single prescaled gate is used and the result is a multiple of 256 Hz." | `aux_pin.c` `bp_measure_frequency()`: first gate at 1:256; `if (frequency > 0xFF) frequency *= 256; else` rerun at 1:1. The branch needs a count of 256, i.e. 65536 edges/s, not 65280. `poll_frequency_counter_value()` uses `PR5:PR4 = 0x00F42400` = 16,000,000 ticks = 1 s. |
| 13 | fixed | `README.md` entry bullet: "Every entry prints an identifier except `0x08`/`0x09`, which have none and return to `BBIO1` after about five seconds of host silence." | `binary_io.c` cases `0x08`/`0x09` call `avrisp_mkii_enter()`; `avrisp_mkii.c` prints no identifier; `avrisp_mkii.h` `AVRISP_IDLE_TICKS 500` × 10 ms = 5 s; the caller then calls `send_binary_io_mode_identifier()`. |
| 14 | fixed | `examples.md`: "## Reading the exchanges"; "`x20` is this page's shorthand for twenty copies, not bytes to send"; "This is a template, not a literal exchange." | Presentation only; the byte blocks were already labelled as format illustrations. |
| 15 | fixed | Anchored the section pointers: `README.md` `sub-protocols.md#entry-bytes` and `generic/commands.md#leaving-the-cli--binary-and-sump-entry`; the same generic anchor in `entering-and-framing.md` and `edge-cases.md`; `commands.md` to `#wiring`, `#leaving`, `#0x0f-inside-a-mode-does-not-return-to-the-terminal`; `sub-protocols.md` to `#the-aux-pin`, `#wait-for-an-edge`, `#same-byte-different-layer-or-mode`; `examples.md` to `#self-test-0x10-0x11`. | Heading slugs re-derived from the current files; every in-repo link and anchor was checked and resolves. |
| 16 | fixed | `sub-protocols.md`: "persists into BBIO1 and into every sub-protocol except binary I2C, which forces it back to `AUX`". Removed the 1-Wire row from the `0x6x` table. | `i2c.c` `binary_io_enter_i2c_mode()` sets `mode_configuration.alternate_aux = 0`; it is the only protocol-entry assignment. `1wire.c` handles speed in `BINARY_IO_ONEWIRE_COMMAND_CONFIGURE` (`0x2x`) and has no `0x6x` case. |
| 17 | fixed | `examples.md` §Enter, identify, leave adds a five-step host procedure (open port, twenty NULs, read five bytes and compare, read each reply length, `0x0F` then read to the prompt). | `proc_menu.c` ASCII_NUL path calls `enter_binary_bitbang_mode()` on the 20th NUL; `binary_io.c` prints the identifier and returns on `0x0F`; the v4 caller prints the banner and `HiZ>`. |

## Extra in-set correction (not in the finding list)

| Item | Change | Evidence |
|---|---|---|
| `OCD1` does not ignore unknown bytes | `entering-and-framing.md`: "`OCD1` also answers a single `0x00`. Only the `XSV1` loop ignores unknown bytes." `edge-cases.md` §Unknown command and the `0x0F` table say the same; the `0x0F` recovery says "`OCD1` answers `0x00`". | `openocd.c` `binOpenOCD()` `default:` fills `buf[0] = 0x00` and calls `bp_write_buffer(buf, 1)`. `jtag.c` `jtag()` has `default: break;`, so `XSV1` really ignores unknown bytes. |
| SPI section pointer | `examples.md` now points at §Worked frames, the current heading of `docs/protocol/spi/binary.md`. | `docs/protocol/spi/binary.md` heading "## Worked frames" (line 365). |

## Cross-set changes for the main session

Required by findings (do not edit from this set):

1. **`docs/protocol/i2c/binary.md` §Write-then-read (line 109)** — replace
   "4096 bytes on v4, 2048 on v3" with "4096 bytes on both v3 and v4".  Use the
   same name as this set ("terminal output buffer") and keep the contrast with
   the 128/256-byte command line.  Evidence:
   `Firmware/configuration.h` `BP_TERMINAL_BUFFER_SIZE 4096` (unconditional),
   `Firmware/core_system/main.c` `bp_buffer[BP_TERMINAL_BUFFER_SIZE]`,
   `Firmware/protocol_i2c/i2c.c` `i2c_write_then_read()` bounds both counts by
   that constant.  No 2048-byte terminal buffer exists in the tree.
2. **`docs/capabilities/smps/binary.md` §Entry and exit (lines 21-24)** —
   remove the `unverified` caveat: the v4 terminal reset does stop the loop.
   Keep "only `0xF1` while still in raw wire" as the direct stop.  Evidence:
   `Firmware/core_system/proc_menu.c` NUL-entry branch →
   `reset_terminal_state()`; `Firmware/core_system/base.c`
   `bp_reset_board_state()` → `smps_stop()` under `BUSPIRATEV4` +
   `BP_ENABLE_SMPS_SUPPORT`.
3. **`docs/capabilities/smps/binary.md` §Entry and exit (lines 31-35), the
   table row "any, when the feature is not built" (line 48) and §Limits
   (lines 149-150)** — a start's operand **is** consumed.  Evidence:
   `Firmware/protocol_binary/binary_io.c` `handle_smps_command()`, `#else`:
   `if ((uint8_t)command >= SMPS_COMMAND_START) user_serial_read_byte();`.
4. **`docs/capabilities/smps/edge-cases.md` §The SMPS feature is not built in
   (lines 8-23) and §Leaving raw wire (lines 63-68)** — rewrite the
   `0xF2 0x00` / `0xF2 0x58` symptoms and the "does not consume a start's
   operand" behaviour.  With the operand consumed, `0xF2 0x00` consumes `0x00`
   as the operand, answers `0x00`, and stays in raw wire; the byte after the
   operand is the next request.  In the second entry, remove the `unverified`
   note: the v4 terminal reset does stop the board (item 2).  The §A requested
   value below `0x200` entry is correct: `0xF0`/`0xF1` take no operand.
5. **`docs/capabilities/smps/README.md` lines 33-34** — "the v4 terminal reset
   is not confirmed to" → it does stop the board (same evidence as item 2).
6. **`docs/capabilities/auxiliary/frequency.md` §The binary command
   (lines 126-131)** — "counts 255 edges" is really a prescaled count; the
   multiple-of-256 branch starts at 65536 Hz, so "above about 65.28 kHz"
   should be "above about 65.5 kHz" (65.536 kHz).  Evidence:
   `Firmware/capability_aux/aux_pin.c` `bp_measure_frequency()`.
7. **`docs/capabilities/auxiliary/edge-cases.md` line 39** — same 65.28 →
   65.5 kHz correction.
8. **`docs/generic/README.md` line 138** — rename the size-row label
   "terminal command buffer" to "command buffer"; it is the 128/256 B input
   ring, not the 4096-byte terminal output buffer named in finding 1.
9. **`docs/core/system.md` line 95** — optional consistency rename:
   "terminal buffer" → "terminal output buffer" (the values are already
   correct).

Extra observation, not raised by the auditor:

10. **`docs/protocol/i2c/binary.md` §AUX (`0x09`), table row "other"
    (line 141)** — an unknown sub-command returns `0x01 0x00`, not
    "the result stays `0x01`".  Evidence: `Firmware/protocol_i2c/i2c.c`
    `binary_io_enter_i2c_mode()` case `9`: `fr = 1` then `default: fr = 0`.

## Remaining questions

- The v3 column stays unverified; there is no v3 hardware evidence in the
  tree.  No v3 claim was strengthened.
- The I2C page's 2048 figure has no code counterpart.  If it was meant as a
  v3-specific limit, no reason is visible in `configuration.h`; the main
  session should treat it as stale unless the I2C owner has evidence.
- The 65.5 kHz cutoff is derived from `> 0xFF` on the 1:256 gate.  The AUX
  set currently states 65.28 kHz; the two sets must agree.
- The SMPS unit, useful range and electrical behaviour remain owned and
  unverified in `docs/capabilities/smps/`.
- `docs/protocol/spi/binary.md` is being edited in parallel; this set points
  at its current heading "Worked frames".  Re-check that pointer if the SPI
  heading changes.
- Two structural suggestions were not adopted: the trigger index at the top of
  `edge-cases.md` (the headings already scan, and an index duplicates them) and
  a full move of the leaving semantics out of `commands.md`/`edge-cases.md`
  (the caveats are now in `entering-and-framing.md` §Leaving, and the other two
  pages stay the command-level detail).

