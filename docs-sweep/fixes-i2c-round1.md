# I2C docs — round 1 fixes

Scope: `docs/protocol/i2c/**` only. No firmware, binary-set, generic-set,
`docs/dev/`, `docs/SPEC.md` or WORKLOG edits. No commit.

**Result:** 23 findings: 22 fixed in-set, 0 rejected, 1 cross-set-only
(finding 5). Finding 2 also needs one cross-set change (`docs/SPEC.md`), so
there are two cross-set changes in total. Finding 13 also raises one
code-owner candidate in the host tool (`scripts/I2CEEPROMWIN.c`); it is not a
firmware defect.

## Per-finding record

| # | Decision | Change (file + short quote) | Evidence |
|---:|---|---|---|
| 1 | fixed | `README.md` hardware paragraph: “Both boards have switched on-board pull-ups … v3 needs an external 0–5 V feed on Vpullup; an external pull-up is optional.” `wiring.md` heading is now “On-board pull-ups” and the v3/v4 row says “switched, value not covered”. | `Firmware/hardwarev3.h` defines `BP_PULLUP`, `BP_PULLUP_LAT` and `BP_PULLUP_DIR`; `Firmware/hardware.h` `bp_enable_pullup()` / `bp_disable_pullup()` drive `BP_PULLUP`; `Firmware/core_system/proc_menu.c` case `'P'` calls `bp_enable_pullup()` in any non-HiZ mode. `docs/generic/README.md:151` already says `P` uses the on-board resistors. |
| 2 | fixed + cross-set | `README.md`: “standard on v4, optional on v3 and not in the default v3 build”; status: “v3 is included in the build and its behaviour is **not verified**”. `entry-and-setup.md`: “on-board EEPROM macros 3/4 (v4 only)”. | `Firmware/configuration.h:412` defines `BP_I2C_USE_HW_BUS` only for `BUSPIRATEV4`. `Firmware/protocol_i2c/i2c.c` keeps v3 hardware code under `#ifdef BP_I2C_USE_HW_BUS` and the `PIC_REV_A3` warning in `i2c_setup_prepare()`, so a v3 build can compile it in; the default v3 build does not. `docs/SPEC.md` still marks the v3 cell absent; cross-set change below. |
| 3 | fixed | `binary.md` worked exchange now has `host <- 0x01` immediately after `host -> 0x60            speed = software profile 0`. | `Firmware/protocol_i2c/i2c.c`, `binary_io_enter_i2c_mode()` case `0b0110`: `bitbang_setup(2, inByte); REPORT_IO_SUCCESS();` — one `0x01` reply. |
| 4 | fixed | `entry-and-setup.md` row is now “park and remove the target rails”; the note says “`w` removes the target rails only: it leaves `P` and the VPU source as they were. Use `p` and `e1` to switch them off before rewiring.” | `Firmware/core_system/proc_menu.c`, `switch_psu_off()` calls `cleanup()`, prints `Clutch disengaged!!!` / `POWER SUPPLIES OFF`, and disables only the voltage regulator. It never calls `bp_disable_pullup()` or `bp_disable_3v3_pullup()` / `bp_disable_5v0_pullup()`. `i2c_cleanup()` clears the pending ACK and disables the I2C modules. |
| 5 | cross-set | No I2C edit needed. The I2C page already says “I2C has **no `Select output type:` prompt**”, which matches the mode. | `Firmware/protocol_i2c/i2c.c`, `i2c_setup_prepare()` sets `mode_configuration.high_impedance = ON` without an output-type prompt. `Firmware/core_system/proc_menu.c`, `set_pullup_voltage()` only requires `high_impedance == ON`, so the generic page's prompt requirement cannot be met in I2C. Recorded below. |
| 6 | fixed | `README.md` bring-up: “skip it for a self-powered target, where `e`/`P` still work”. | `Firmware/core_system/proc_menu.c`, `switch_psu_on()` enables the regulator and calls `setup_execute()`; `W` is the target-rail switch, not a precondition for `e`/`P`. The detail page already carries the safety warning. |
| 7 | fixed | `entry-and-setup.md`: “Typing the header as the arguments to `m4` fails … the prompts then run. Typing it at the `I2C>` prompt is a syntax error.” The entry edge table and `edge-cases.md` now make the same distinction. | `Firmware/core_system/proc_menu.c`, `changemode()`: `m4 <text>` parses `4`, then `i2c_setup_prepare()` reads non-numeric HW/speed as 0 and runs the prompts. An unrecognised first character at the mode prompt sets `command_error` and prints `Syntax error at char N`. |
| 8 | fixed | `entry-and-setup.md`: “Only `m` with no number on the same line (or `m0`, which counts as no number) opens the menu.” | `Firmware/core_system/proc_menu.c`, `changemode()`: `busmode = getint()`; when `!busmode` it prints the menu, so `m0` opens it. |
| 9 | fixed | `bus-operations.md`: “Set the pointer again before each read transaction; bytes within one transaction auto-increment. A new read address does not continue the previous write's pointer.” | `Firmware/protocol_i2c/i2c.c`, `i2c_read()` reads successive bytes without resending the memory pointer; the page's worked session sets `0x00 0x20` once and then reads two bytes. |
| 10 | fixed | `clock-stretching.md`: heading is now “Two outcomes”; the follow-on row is gone; “**After a timeout:** … does not retry the abandoned command”; the recovery bullet now says “starts from the current line state; it does not retry the abandoned command”. `edge-cases.md` timeout row aligned. | `Firmware/core_bitbang/bitbang.c`, `bitbang_wait_for_clock_release()` returns false after `BB_CLK_STRETCH_TIMEOUT`; `bitbang_i2c_abort()` sets the sticky error and drives the pins. The next `i2c_read()` / `i2c_write()` / `i2c_start()` / `i2c_stop()` calls `bitbang_i2c_clear_error()` and performs its own operation; there is no retry of the abandoned command. |
| 11 | fixed | `binary.md` AUX section: byte 1 is “the command was accepted”, not success; “it is `0x01` even when the sub-command is unknown”; the sub-result `0x00` means not recognised (or a valid low on `0x03`). The worked-exchange comment is now “command accepted”; `edge-cases.md` aligned. | `Firmware/protocol_i2c/i2c.c`, `binary_io_enter_i2c_mode()` case `9`: `REPORT_IO_SUCCESS()` runs before the sub-command is read; `fr = 1` for the defined sub-commands; `default: fr = 0`; both bytes are sent before the loop continues. |
| 12 | fixed | `eeprom.md`: “`0x4C` (supply, pull-ups and AUX low) … `0x4C` drives AUX low and the tool never sends `0x09 0x02`, so leave AUX unjumpered when using it; a jumpered AUX clamps SDA/SCL.” | `scripts/I2CEEPROMWIN.c:101` writes `\x02\x51\x4C` and never sends `0x09`. `Firmware/protocol_binary/binary_io.c`, `bp_binary_io_peripherals_set()` runs `BP_AUX0_LAT = (inByte & 0b00000010) ? ON : OFF; BP_AUX0_DIR = OUTPUT;` for every `0x4x`; `0x4C` has bit 1 clear, so AUX is low. |
| 13 | fixed + code-owner (host tool) | `eeprom.md` Limits: “The write retry loop has no limit; a missing, unpowered or miswired chip leaves the tool retrying until it is interrupted.” | `scripts/I2CEEPROMWIN.c:222-226`: `do { failcount++; WriteFile(...); ReadFile(...); } while (receive_buf[2] || receive_buf[3] || receive_buf[4] || ...);` — no counter or timeout. Code-owner candidate below. |
| 14 | fixed | `wiring.md`: “The `v` table labels that column `VPU` and the reference card marks the pin `Vpu`; this set uses Vpullup in prose and quotes the other forms where they appear.” | `Firmware/core_messages/messages_v3.s` / `messages_v4.s` pin-table row uses `VPU`; `set_pullup_voltage()` prints “Select Vpu (Pullup) Source”; the warnings say “Vpullup pin”. The README names table already made Vpullup primary. |
| 15 | fixed | `edge-cases.md` “## Unverified and caveats” deleted. `clock-stretching.md` now says “On v4”; `speeds.md` says “at the default build”. | The removed bullets repeated the per-table “v3 column is not verified” caveat and gave no user action. `docs/TEMPLATE.md` §10/§12 puts maintainer evidence in `docs/dev/` and allows only short inline caveats. |
| 16 | fixed | `README.md` Traffic is split into four bullets: start/stop, write, read, repeated start. | Documentation structure; the command set behind it is unchanged (`i2c_start()` / `i2c_stop()` / `i2c_write()` / `i2c_read()`). |
| 17 | fixed | `edge-cases.md` W row: “the driver is re-initialised; the pending bit stays set”. `clock-stretching.md`: “`W` re-runs the driver setup but leaves the pending bit set; resolve the read with `]` first.” | `Firmware/core_system/proc_menu.c`, `switch_psu_on()` calls `setup_execute()`. `i2c_setup_execute()` (software) and `hardware_i2c_setup()` (hardware) do not touch `i2c_state.acknowledgment_pending`; only `i2c_cleanup()` and the normal stop/timeout paths clear it. |
| 18 | fixed | `edge-cases.md` width row now says “send multiple bytes; change the width only for a bit-level transfer”. | `Firmware/core_system/proc_menu.c`, `getnumbits()` sets `mode_configuration.numbits`; `bitbang_write_value()` uses that width for software transfers, while `hardware_i2c_write()` always sends 8 bits. The two adjacent rows now agree on the default-8 recommendation. |
| 19 | fixed | `README.md` macros bullet: “`(3)` and `(4)` drive the on-board EEPROM (v4); run `(3)` before `(4)`. Both require **2 Hardware**; from Software `(3)` prints … and `(4)` is silent.” | `Firmware/protocol_i2c/i2c.c`, `i2c_macro()` case 3 refuses when `mode != I2C_TYPE_HARDWARE` and sets `command_error`; case 4 does nothing unless `i2c_state.to_eeprom`, which only case 3 sets. |
| 20 | fixed | `bus-operations.md` scanner paragraph: “with a start, the address byte, an ACK read, and a stop”; new bullet “Every probe ends with a stop, hit or no hit, so the bus is idle between addresses.” | `Firmware/protocol_i2c/i2c.c`, `i2c_macro()` case 1: after the ACK test (and the optional read-address byte) it always calls `bitbang_i2c_stop_wait()` / `hardware_i2c_stop()` before the next address. |
| 21 | fixed | `README.md` limits: “No generic device layer … The only chip-specific support is the on-board EEPROM macros 3/4 (v4) and the external EEPROM tool.” | The mode has no device layer in `Firmware/protocol_i2c/i2c.c`; macros 3/4 are `i2c_macro()` cases 3/4 and the tool is `scripts/I2CEEPROMWIN.c`. |
| 22 | fixed | `README.md` bring-up adds “`P` is enough at the slow profiles; for 400 kHz/1 MHz add a 1.5–2.2 kΩ external pull-up on SCL.” | `wiring.md` documents the v4 on-board SCL resistor as 10 kΩ; the fast profiles need a stronger pull-up. No code guard exists, so the note is guidance. |
| 23 | fixed | `bus-operations.md` repeated-start example now ends `I2C>r` → `READ: 0xAA` → `I2C>]` → `NACK` → `I2C STOP BIT`. | `Firmware/protocol_i2c/i2c.c`, `i2c_read()` reads a byte and leaves the ninth bit pending; `i2c_stop()` prints `NACK`, sends it, then prints `I2C STOP BIT`. |

## Code-owner candidate (host tool, not firmware)

**`scripts/I2CEEPROMWIN.c` write retry has no limit.** In write mode the tool
loops over `do { failcount++; WriteFile(...); ReadFile(...); } while` until
every ACK byte in the reply is zero (`scripts/I2CEEPROMWIN.c`, `main()`,
write branch, lines 222-226). A missing, unpowered or miswired EEPROM therefore
leaves the tool retrying indefinitely and no summary line is printed. The docs
now state the limit; the tool owner may want a retry count or timeout and a
reported failure.

Related, not counted separately: the same tool would be more robust if it sent
`0x09 0x02` after `0x4C` to release AUX when the pin is jumpered. The docs
describe the current behaviour instead.

## Cross-set changes for the main session

Required:

1. `docs/SPEC.md:59` — change the I2C hardware-engine v3 cell from `-` to
   `off`:
   `| · hardware engine | - | x | 100k/400k/1M | ✓ |` →
   `| · hardware engine | off | x | 100k/400k/1M | ✓ |`.
   The SPEC legend defines `off` as “optional feature, not in the default
   build”; this matches the I2C README/entry page and `configuration.h`, which
   only defines `BP_I2C_USE_HW_BUS` for v4 by default while `i2c.c` keeps the
   v3 hardware code under that switch.
2. `docs/generic/commands.md` §Feeding the Vpullup pin, lines 37-40 — replace
   “Enter a non-HiZ mode whose setup answers `Select output type:` with open
   drain (most bus modes default to it; DIO has no such prompt)” with a form
   that includes I2C, for example: “Enter a non-HiZ mode that is open drain
   (I2C is always open drain and asks no output-type prompt; most bus modes
   default to it; DIO has no such prompt)”. The I2C page links this section,
   and `set_pullup_voltage()` only requires `high_impedance == ON`, which I2C
   sets in `i2c_setup_prepare()`.

Consistency follow-up (not required by a finding):

3. `docs/dev/building.md` currently lists `BP_I2C_USE_HW_BUS` only on the v4
   side. If the v3 optional hardware build is to remain documented in the I2C
   set, add a note that it is enabled by defining `BP_I2C_USE_HW_BUS` for a v3
   build (the Makefile accepts `EXTRA_CFLAGS`); otherwise remove “optional
   build” from the I2C set and keep v3 software-only.

## Remaining questions

- The v3 hardware-engine build is source-supported but not exposed by the
  default `configuration.h` feature blocks. The set says “optional build” on
  the strength of the `#ifdef BP_I2C_USE_HW_BUS` v3 code; a v3 board was not
  available and no build was run. The main session should confirm whether this
  optional build is a supported configuration or should be described as
  unsupported.
- The `I2CEEPROMWIN` AUX-release and retry-limit changes are host-tool work,
  not firmware. The docs describe the current behaviour.
- The auditor's larger structural proposals (a dedicated first-session page, a
  terminal-to-binary mapping table, a wiring sketch, and deduplicating the
  repeated v3/v4 and edge-case tables) were not applied. Two small navigation
  fixes were applied: the README `macros.md` row now says where the scanner and
  sniffer formats live, and “Buffers” is its own section instead of the tail of
  “Names used in this set”.
- No firmware, host tool or board was run for this pass; all behavior claims
  were checked against the source files listed in the table.

## Verification

- Read all ten `docs/protocol/i2c/*.md` files and checked every finding against
  `Firmware/protocol_i2c/i2c.c`, `Firmware/core_bitbang/bitbang.c`,
  `Firmware/protocol_binary/binary_io.c`, `Firmware/core_system/base.c`,
  `Firmware/core_system/proc_menu.c`, `Firmware/core_system/core.c`, the
  message tables, `configuration.h`, `hardwarev3.h` / `hardwarev4.h` and
  `scripts/I2CEEPROMWIN.c`.
- Checked all local links and heading anchors in `docs/protocol/i2c/`; no
  broken targets.
- Checked for TAB characters and prose lines over 82 columns outside tables and
  code blocks; none.
- No commit; no WORKLOG edits; no edits outside `docs/protocol/i2c/` and this
  record. The worktree also contains uncommitted changes from the parallel
  UART work and a pre-existing `docs/generic/values.md` change; those were not
  touched.

## Files changed

- `docs/protocol/i2c/README.md`
- `docs/protocol/i2c/entry-and-setup.md`
- `docs/protocol/i2c/wiring.md`
- `docs/protocol/i2c/bus-operations.md`
- `docs/protocol/i2c/clock-stretching.md`
- `docs/protocol/i2c/edge-cases.md`
- `docs/protocol/i2c/binary.md`
- `docs/protocol/i2c/eeprom.md`
- `docs/protocol/i2c/speeds.md`
- `docs-sweep/fixes-i2c-round1.md` (this record)
