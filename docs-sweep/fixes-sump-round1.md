# SUMP docs — round 1 fixes

Scope: `docs/protocol/sump/**` only.  No firmware, other doc sets or WORKLOG
edits.  No commit.  The parallel `docs/protocol/dio/**` changes were left
untouched.

**Result:** 19 findings fixed in-set, 0 rejected, 1 code-owner candidate
(finding 11 also fixed in-set), 2 cross-set edits queued (plus one terminology
note).  Six extra in-set corrections were made while verifying (see below).

## Per-finding record

| # | Decision | What changed (file + short quote) | Evidence |
|---:|---|---|---|
| 1 | fixed | `depth.md` tier ranges now end at expressible counts ("4096 to 8188", "8192 to 16 380", "16 384 to 32 764") and the cliff table uses "8188 / 4096 / 4092", "16 380 / 8192 / 8188", "32 764 / 16 384 / 16 380", "65 532 / 32 768 / 32 764". `readout.md`: "a count of 8188 ... pads the other 4092 bytes". `edge-cases.md`: "an 8188 count at 1 MHz ... 4092 padding bytes". `commands.md`: "There is no way to express 8191, 4095 or any other count that is not a multiple of four." | `Firmware/protocol_sump/sump.c` `sump_handle_command_byte()` `SUMP_CNT`: `read_count = ((((uint32_t)bytes[2] << 8) + bytes[1]) + 1) * 4;`; tiers at `>= 16 * BP_SUMP_STORAGE_WORDS` (32768), `>= 8 *` (16384), `>= 4 *` (8192), `>= BP_SUMP_SAMPLE_MEMORY_SIZE` (4096); `BP_SUMP_STORAGE_WORDS` is 2048. `Firmware/tests/test_sump.py` `counts(n)` encodes `n/4-1`; `count_matrix()` uses 32768, 65532, 65536, 65540, 131072, 262144. |
| 2 | fixed | `rates.md` default section: "**no divider is set**.  The default timer period is 1601 ticks" and "sending `0x80` with `x=1601` would select ... about 62 kHz". Table row added: "default period / 10 000 / timer / 9.994 kHz / 1601". `commands.md`: "divider: not set; the default timer period is 1601 ticks, about 9.994 kHz". `entry.md`: "1601-tick default timer period (about 9.994 kHz)". `edge-cases.md`: "every reset restores the 1601-tick default timer period; no divider is set". | `sump.c` `BP_DEFAULT_TIMER_PERIOD 0x00000640` (1600); `sump_reset()` sets `PR5 = HI16(BP_DEFAULT_TIMER_PERIOD); PR4 = LO16(BP_DEFAULT_TIMER_PERIOD);` and `sump_timer_ticks = BP_DEFAULT_TIMER_PERIOD + 1` (1601). `SUMP_DIV`: `period = x * 4 / 25`, `sump_timer_ticks = period + 1`. x=1601 gives 257 ticks (62.26 kHz); x=10000 gives 1601 ticks (9.9938 kHz). |
| 3 | fixed | `client.md`: "at a capture tier of 8192 or deeper, where **high** channel bits are dropped ... bits 0–3 (AUX2, MOSI, CLK, MISO) are kept and CS (bit 4) is dropped". `trigger.md`: "drops **high** channel bits ... CS (bit 4) is dropped; a CS trigger still fires, but CS is not in the reply". `depth.md`: "The dropped bits are the **high** channel bits, not the low ones". `examples.md`: "CS (bit 4) is dropped". | `Firmware/hardwarev4.h` `AUX2 = RD0`, `MOSI = RD1`, `CLK = RD2`, `MISO = RD3`, `CS = RD4`, `AUX0 = RD5`, `AUX1 = RD8`; `sump.c` `SUMP_CNT` mask `0x0F` at 8192, `0x03` at 16384, `0x01` at 32768. `Firmware/hardwarev3.h` `CS = RB6` (bit 0 after the `>>6 & 0x1F` transform). |
| 4 | fixed | `entry.md`: "Inside a five-byte command an exit byte is a parameter: the command completes and **takes effect** (unless its opcode is unknown)." Also says a byte during a timer capture or trigger wait aborts and is handled, with the fast post-trigger window linked as the exception. `commands.md` parser row: "the exit does not happen, and the command completes and takes effect (unless its opcode is unknown)". `edge-cases.md`: "no exit; the command completes and takes effect ... send the exit again after the command completes". | `sump.c` `sump_handle_command_byte()` `RX_COMMAND_PARAMETERS` stores every byte as `command_buffer.bytes[count]`; the full buffer falls through to `RX_COMMAND_PROCESS`, which executes `SUMP_DIV`, `SUMP_CNT`, `SUMP_TRIG` and ignores unknown opcodes. `Firmware/tests/test_sump.py` `parser_matrix()`: `command(0xc1,0x24230f02); require(!output_len,"exit-valued operands stay data");`. |
| 5 | fixed | `trigger.md` new table: 1 MHz / 4000 / 4 ms, 100 kHz / 400 / 4 ms, 10 kHz / 40 / 4 ms, 1 kHz / 16 / 16 ms, 100 Hz / 16 / 160 ms, 5.96 Hz / 16 / 2.7 s. Text: "The trigger arms at the end of the burst that completes the pre-fill ... Start the capture more than this long before the event". Missing-trigger row and `edge-cases.md` row rewritten; `examples.md` 100 kHz capture notes the first 4 ms burst. | `sump.c` `sump_acquire_samples()`: `fragment = (sump_timer_ticks == 0 OR sump_timer_ticks > 4000u) ? 16u : (64000u / sump_timer_ticks);`; trigger arming is checked only after a fragment: `if (written >= pre_trigger) { armed = true; sump_apply_trigger(); }`. 64000 ticks is 4 ms at 16 MHz; ticks > 4000 forces the 16-sample minimum. |
| 6 | fixed | `trigger.md`: "not polled until the window and the readout finish; the window is about 1 ms on v4 (2.05 ms for 4096 samples at 2 MHz on v3), and the readout follows it (about 0.36 s for 4096 bytes at 115200 baud on v3)". `edge-cases.md` row matches. `readout.md` adds "The fast post-trigger window is about 1 ms on v4 (2.05 ms ...); the readout that follows takes about 0.36 s ...". | `sump.c` fast path calls `sump_wait_for_fast_trigger()` then `sump_fast_capture_*()` then `sump_transmit_fast()` without a host read check in the capture or transmit loops. `sump_wait_for_fast_trigger()` polls only every 4096 passes. v3 2 MHz capacity 4096 samples -> 2.048 ms; 4096 bytes at 115200 8N1 -> 0.356 s. |
| 7 | fixed | `rates.md` heading sentence now "Common requests and the rate they achieve"; "the device then selects the timer period or engine band that `x` falls in, which is not always the nearest rate"; 8.33 MHz row marked "(nominal)" with a note; 300 kHz example now "at divider 332 (period 54 ticks)". | `sump.c` `SUMP_DIV` bands: `period < 1` 16 MHz, `< 3` 8 MHz, `< 7` 4 MHz, `< 15` 2 MHz, else timer. x=11 -> period 1 -> 8 MHz; x=43 -> period 6 -> 4 MHz; x=93 -> period 14 -> 2 MHz, so the selected band is not the nearest rate at the edges. |
| 8 | fixed | One form is used: `client.md` "ancillary version string `enh`, the enhanced-profile marker"; `commands.md` "ancillary version string `enh` (the enhanced-profile marker)"; `README.md` "ancillary version string; the marker the enhanced profiles match". | `sump.c` `SUMP_METADATA_ANCILLARY_VERSION` sends `'e','n','h','\0'`; the source comment calls `enh` the community marker for the enlarged buffers/rates, matched by enhanced profiles. |
| 9 | fixed | `entry.md` recovery table split: "the board answered `BBIO1` ... send `0x0F`" and "the board answered a sub-mode identifier (`I2C1`, `SPI1`, …) ... send `0x00`, read `BBIO1`, then send `0x0F`", with a link to `../binary/edge-cases.md#0x0f-inside-a-mode-does-not-return-to-the-terminal`. | `Firmware/protocol_binary/binary_io.c` `BITBANG_COMMAND_RETURN_TO_TERMINAL`: on v4 `reset_state(); return -1;`, on v3 `__asm volatile("RESET");`. The binary edge-case heading and anchor exist in `docs/protocol/binary/edge-cases.md`. |
| 10 | fixed | `edge-cases.md` Rates table adds a row for "a trigger wait longer than the 32-bit sample counter" with symptom "no visible symptom" and recovery "the counter is rebased at a whole number of ring revolutions; no action needed ([rates.md](rates.md#counter-rollover))". | `sump.c` `sump_acquire_samples()`: `if (trigger_armed && !triggered && written >= 0xffff0000UL) { written = ring_samples + written % ring_samples; }`. |
| 11 | fixed in-set; code-owner candidate | `readout.md`: "A long burst can overrun the v3 UART: bytes beyond the receiver are lost and the next command may be ignored. ... If the board stays silent, the receive path itself is stopped; power-cycle it." `edge-cases.md` row matches, with the parser-recovery link. | `Firmware/core_system/base.c` `user_serial_check_overflow()` returns `U1STAbits.OERR` on v3 and `user_serial_clear_overflow()` clears it; `Firmware/core_system/proc_menu.c` terminal loop clears it, but `Firmware/protocol_sump/sump.c` never calls either. The v3 UART receive FIFO is four bytes; an overrun must be cleared before new bytes are accepted. See code-owner section. |
| 12 | fixed | `client.md` v3 serial row: "the terminal's baud rate (default 115200), 8 data bits, no parity, one stop bit"; new paragraph: "On v3 the default is 115200 bps.  If `b` was used ... a mismatch gives no `1ALS` or a garbled one.  See [generic navigation.md](../../generic/navigation.md#terminal-baud--b)." | `Firmware/core_system/main.c`: "Set the UART port speed to 115200 bps."; `docs/generic/navigation.md` "Terminal baud — `b`" says option 9 (115200) is the boot rate. The anchor resolves. |
| 13 | fixed | `README.md` new "## First capture" section: install link, SUMP/OLS profile, 1 MHz, 4096 samples, no trigger, known square wave, common ground, "exact menu names vary by release", link to `examples.md`. Limits line now "No SUMP client ships with the firmware; [First capture](#first-capture) names the client and the first settings." | Usage-only; `https://sigrok.org/wiki/Downloads` was fetched and resolves to the sigrok download page listing PulseView and sigrok-cli. The firmware ships no client. |
| 14 | fixed | `commands.md`: "an exit additionally prints the terminal banner and the `HiZ>` prompt, which are terminal output rather than a SUMP reply." | `Firmware/core_system/proc_menu.c` `handle_left_arrow()`: after `enter_sump_mode()` returns, `reset_terminal_state()` then `bp_write_string("HiZ>")`; `enter_sump_mode()` calls `print_version_info()`. `Firmware/tests/test_core_sump_return.py` expects `"1ALSBus Pirate test banner\r\nHiZ>"`. |
| 15 | fixed | `client.md`: "A request above the advertised 16 MHz is not an error; it selects the 16 MHz engine.  The slow end is the exception: a request slower than about 6 Hz wraps the divider, so clamp it before computing the request." | `sump.c` `SUMP_DIV`: dividers 0–6 all select the 16 MHz engine; the divider field is 24 bits (`bytes[1..3]`), so a slower request wraps. `rates.md` §Divider limits documents the 5.96 Hz slowest timer rate. |
| 16 | fixed | `depth.md`: "A request below the tier size fills only that many slots; a request inside a tier fills the tier size and pads the rest." | `sump.c` `SUMP_CNT`: a read count below 4096 sets `samples_to_acquire = read_count`; 4096–8188 sets 4096; 8192–16380 sets 8192; `sump_transmit_*` pads `samples_requested - samples`. |
| 17 | fixed | `examples.md` 8192 example: "This block is for v4: `c0 01` selects AUX2.  On v3 the same byte selects CS (bit 0), which the 8192 tier keeps; change the mask deliberately there." | `Firmware/hardwarev3.h` `CS = RB6`; the v3 transform `(PORTB >> 6) & 0x1F` maps CS to bit 0. `sump_apply_trigger()` v3 `if (sump_trigger_mask & 0b00000001) ... // CS`. |
| 18 | fixed | `client.md`: "The `0x02` that enters SUMP is answered with the four bytes `1ALS`; each later `0x02` request returns it again." `readout.md` "Nothing unsolicited" row now says "a later `0x02` returns `1ALS`". | `Firmware/core_system/proc_menu.c` `handle_left_arrow()` calls `enter_sump_mode()`; `enter_sump_mode()` calls `sump_handle_command_byte(SUMP_ID)` once, which writes `SUMP_DEVICE_ID`. A later `0x02` is handled as `SUMP_ID` in `RX_COMMAND_IDLE`. |
| 19 | fixed | `README.md` status: "the v3 feature is present in the build but not verified on v3 hardware"; availability row "present in the build"; one note: "The v3 feature is present in the build and **not verified on v3 hardware**. Fast capacities, 4 MHz jitter and readout timing are the v3-specific caveats." `depth.md` row "present in the build, not verified". Removed `edge-cases.md` §Unverified paths and the trailing v3 caveats in `trigger.md` and `rates.md`; kept short inline caveats in the v3 fast/jitter/readout sections. | `Firmware/configuration.h` includes `BP_ENABLE_SUMP_SUPPORT` in both the v4 and v3 default feature blocks; no v3 hardware evidence exists in the tree. Wording only. |

## Extra in-set corrections (not in the finding list)

| Item | Change | Evidence |
|---|---|---|
| "one of five storage formats" | `depth.md` now says "selects the storage format and the capture size"; the table's five rows share four packings (1 byte, 4/word, 8/word, 16/word). | `sump.c` `SUMP_CNT` sets `sump_samples_per_word` to 1, 4, 8 or 16 only. |
| Concrete unrecorded-vs-padding byte example | `readout.md`: at 8192 a real sample reads `0x5?` or `0xA?` with the high nibble alternating; a padding byte is exactly `0x55` or `0xAA`; a matching data nibble can coincide. | `sump_transmit_packed()`: `(pattern & ~mask) OR ((packed >> bitpos) & mask)` with `mask = 0x0F` at 8192. |
| Slow-rate abort latency | `readout.md` and `edge-cases.md` now say a timer-capture abort waits for the next burst, "up to 2.7 s at the slowest rate". | `sump_acquire_samples()` fragment is 16 samples when `sump_timer_ticks > 4000`; 16 samples at 5.96 Hz is 2.684 s. |
| Entry/ID row in "Nothing unsolicited" | `readout.md` row "`0x02`" changed to "a later `0x02`" so the entry `0x02` is not read as a second reply (same ambiguity as finding 18). | `enter_sump_mode()` sends one `1ALS` per entry; a later `0x02` is a separate ID request. |
| First example mask is v4-specific | `examples.md` now says the 1 MHz block is v4 (`c0 10` = CS bit 4) and that v3 uses `c0 01` for CS. | `Firmware/hardwarev3.h` `CS = RB6` -> bit 0 after the `>>6 & 0x1F` transform; `Firmware/hardwarev4.h` `CS = RD4` -> bit 4. |
| Remaining register name in prose | `rates.md` timer bullet no longer says `PR + 1`; it says "the programmed timer value plus one tick". | User-doc rule: no firmware symbol names; the formula block already gives the exact arithmetic. |

## Code-owner candidate

**v3 UART overrun is not cleared in the SUMP loop.**

- File/symbol: `Firmware/protocol_sump/sump.c`, `enter_sump_mode()` and
  `sump_handle_command_byte()`; `Firmware/core_system/base.c`,
  `user_serial_check_overflow()` / `user_serial_clear_overflow()`;
  `Firmware/core_system/proc_menu.c`, terminal input loop.
- Evidence: the terminal loop checks `user_serial_check_overflow()` and clears
  `U1STAbits.OERR`; the SUMP loop never calls either helper. The v3 UART
  receive FIFO is four bytes, and a PIC24 UART stops accepting new bytes while
  OERR is set. A burst during a SUMP readout can therefore lose the bytes and
  leave the receiver stopped until a reset/power-cycle.
- Docs consequence: `readout.md` and `edge-cases.md` now say the overrun can
  lose bytes and that a silent board needs a power-cycle. No firmware change
  was made. The code owner should decide whether to clear the flag in the SUMP
  command loop (as the terminal loop does).
- Confidence: the OERR-needs-clearing behaviour is from the PIC24 UART
  contract; it was not measured on a v3 board. The docs recovery (power-cycle)
  is safe either way.

## Cross-set changes for the main session

Required by the findings (do not edit from this set):

1. **`docs/core/system.md` line 568** — "SUMP entry | at least 5 NUL bytes
   followed by the raw left-arrow byte (`0x02`, STX) call
   `enter_sump_mode()`" should say **5–19 NUL bytes**.  At twenty NULs the
   terminal enters the binary layer instead.  Evidence:
   `Firmware/core_system/proc_menu.c` `ASCII_NUL` increments
   `binary_mode_counter` and calls `enter_binary_bitbang_mode()` at 20;
   `handle_left_arrow()` enters SUMP only when the counter is at least 5.
   `docs/generic/commands.md` and the SUMP set already say 5–19.

2. **5.5 V limit authority (`docs/protocol/uart/edge-cases.md` line 18)** —
   that page calls the board-level limit "(unverified)", while the SUMP set and
   most other protocol sets state "5 V safe, 5.5 V maximum" as fact.  The main
   session should pick one authority.  No firmware source in the tree defines
   the electrical limit.  If the "unverified" label wins, the SUMP pages (and
   the SPI, JTAG, 1-Wire, 2-Wire, 3-Wire, PIC, OpenOCD and XSVF pages) need the
   same caveat; if the fact wins, remove the UART caveat.  Do not change the
   SUMP wording until the decision is made.

Terminology note (not a correctness defect):

3. **"build-only"** remains in `docs/core/README.md`,
   `docs/dev/verification.md`, `docs/tools/*`, `docs/capabilities/*` and
   `docs/programmers/*`.  The SUMP set now uses "present in the build".  If the
   main session wants one term, apply it globally; finding 19 only required the
   SUMP set.

## Remaining questions

- The exact sigrok/PulseView profile label and whether a given client sends
  exactly five reset bytes are still unverified; `client.md` and `entry.md` say
  so.
- v3 hardware behaviour is still unverified: fast capacities, 4 MHz jitter and
  readout timing.  No v3 board evidence is in the tree.
- The 9.994 kHz default is arithmetic from the 1601-tick period; it was not
  measured on hardware.
- The OERR consequence is a code-owner question; the exact v3 recovery
  (reset button vs power-cycle) should be confirmed on hardware.
- The README "First capture" menu names vary by client release; the section
  says so and does not claim a specific label.
