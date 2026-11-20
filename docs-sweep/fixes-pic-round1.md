# PIC (ICSP) docs — round 1 implementation record

**Scope:** `docs/programmers/pic/**` only.  Firmware was not changed.
**Baseline:** `docs/combined-sweep` at `fc81a44` (rebased on the combined
firmware `c57e223`).

**Verification performed:**

- Read `Firmware/programmer_pic/pic.c` / `.h`,
  `Firmware/protocol_binary/binary_io.c`, `Firmware/core_system/proc_menu.c`,
  `Firmware/core_messages/messages_v3.s` / `messages_v4.s`,
  `Firmware/tests/test_pic.py`, `Firmware/tests/test_pic_setup.py`.
- Read the in-repo host sample under `scripts/BusPirateRawCS/`, especially
  `PIC16ProgrammerHelper.cs` and `BusPirateRawWire.cs`.
- Read the external `picprog` source
  (`https://github.com/DangerousPrototypes/PiratePICprog`, `software/
  buspirate.c`, `main.c`, `common.h`) and the published adapter schematic
  (`https://github.com/DangerousPrototypes/Buspirate_Adapter_Pic_Programmer`).
- Ran `python Firmware/tests/test_pic.py` (44 checks) and
  `python Firmware/tests/test_pic_setup.py` (112 cases): both pass.  The
  tests pin LSB-first reads, the 16-bit write mask, state reset and display
  width; they do not cover the inline-parse-error behaviour below.

## Findings

| # | Decision | What changed (file + short quote) | Evidence |
|---|---|---|---|
| 1 | fixed | `config-words.md`: replaced the table with the verified six-bit rows (`0x00`, `0x02`, `0x03`, `0x04`, `0x05`, `0x06`, `0x08`, `0x09`, `0x0B`); removed the wrong `0x0A`/`0x0C`/`0x0E` rows and the `0x18` claim; added "the six-bit map used by the common PIC16F midrange parts; other six-bit families can differ." | PIC16F627A/628A/648A programming specification, Table 2-3 (command mapping); `scripts/BusPirateRawCS/BusPiratePICProgrammer/PIC16ProgrammerHelper.cs` (`IncAddress 0x06`, `readProgramWord 0x04`, `loadCodeWord 0x02`, `CommandBeginCodeProg 0x08`, `CommandErase 0x09`, `CommandEepromErase 0x0B`); `pic.c` `pic_run_macro()` uses `0x00`, `0x06`, `0x04`. |
| 2 | fixed + code-owner CO-1 | `README.md`: "Start here" now reads "apply the programming-mode condition (VPP for HVPP, MCLR at VDD for LVPP), then run `(1)`"; quick-start step 4 adds "do this before `(1)`"; `sessions.md`/`wiring.md`/`macros.md` repeat the same precondition. | `pic.c` `pic_run_macro()` never touches CS/MCLR; `pic.c` `pic_setup_prepare()`; `macros.md` existing "does not drive MCLR" statement. |
| 3 | fixed | `wiring.md` bring-up table is `e2`/`e3` → `W` → `P`; `README.md`, `sessions.md` and `edge-cases.md` use the same order; a note says "`e` may be issued before or after `W` ... Keep `P` last." | `proc_menu.c` `set_pullup_voltage()` only requires non-HiZ and open-drain and refuses when external VPU voltage is present; it does not depend on `W`.  `docs/generic/commands.md` and `docs/protocol/spi/wiring.md` already use `e` → `W` → `P`. |
| 4 | fixed | `wiring.md`: split the adapter table into "control inputs and reference connections" (`CS` → adapter `MCLR` input, `AUX` → `VPPEN`, `MISO` → target-VDD switch, `VPU` → `VTARGET`) and "programming outputs" (`VPP` → target `MCLR`/`VPP` only); added "The adapter's VPP output goes only to the target's MCLR.  **No Bus Pirate pin is VPP-tolerant**". | Published adapter schematic `bp_pic_programmer_v1.sch.png`: JP1's MCLR/VPPEN/MISO nets drive the switch transistors; the switched 13 V net is the separate `UPP`/`VPP` output.  `picprog` source `buspirate.c` uses `CS` for MCLR and `AUX` for VPP.  Adapter repo: `DangerousPrototypes/Buspirate_Adapter_Pic_Programmer`. |
| 5 | fixed | `picprog.md`: new "The route" section — `BBIO1` → `0x05` (`RAW1`) → `0x4x` power/AUX/CS → `0xA0` engine → `0xAx`, chiefly `0xA7`; source link; "does not use the terminal PIC mode or `PIC1`"; "a build without terminal `PIC` still answers this path." | `software/buspirate.c`: `BP_EnableRaw2Wire()` sends `0x05`; `BP_SetPicMode()` sends `0xA0`; `BP_BufferReset()` starts an `0xA7` script; `BP_Init()` uses `0x40`.  `binary_io.c` `handle_pic_command()` is unconditional. |
| 6 | fixed | `picprog.md`: removed the part-support result table and the "ID/erase only", "read valid; verify can fail" and HEX-parser diagnosis; replaced with "The tool ships a small part table.  That table is the authority ... this set does not re-verify it." | The removed statements were tool-result claims with no repository evidence; the current source has a part table but no bench results. |
| 7 | fixed | `README.md`: "Start here" at line 16, file map at line 22, quick start at line 36, naming table at line 72; limits and quick start are tables; 112 lines. | Structural review; template target ≤ ~120 lines. |
| 8 | fixed (PIC) + cross-set | `bus-operations.md`: "The mode shifts **LSB-first** on the wire.  The generic bit-order setting is a separate layer ... `L` reverses the value within the display width before the mode sees it, so the wire order becomes MSB-first ... the label describes the generic transformation, not the wire order in this mode."  `edge-cases.md` says "leave the generic bit order at `l` (no reversal)." | `proc_menu.c` applies `little_endian` reversal before `protocol->send()`; `pic.c` `write_bits()` shifts LSB-first; `test_pic.py` checks LSB-first assembly. |
| 9 | fixed | `wiring.md`: added the inline v4 2x6 header table (12 pins, key = pin 1) and the v3 2x5 cable order (`GND`, `3.3V`, `5.0V`, `ADC`, `VPU`, `AUX`, `CLK`, `MOSI`, `CS`, `MISO`); removed the AVRISP-only pointer. | `docs/programmers/avrisp/wiring.md` v4 header table; `docs/generic/usage.md` v3 column order; v3 cable order from the published v3 header/pinout. |
| 10 | fixed | `README.md` limits row: "the terminal can drive CS after selecting it with `C` (the command family is called AUX; the pin is CS)". | `proc_menu.c`: `C` sets `alternate_aux = 1` (CS); `A`/`a` drive the selected AUX target. |
| 11 | fixed | `wiring.md`: replaced the ambiguous four-column table with code blocks per board: `v4: CS MISO CLK MOSI / - - PGC PGD`; `v3: CLK MOSI CS MISO / PGC PGD - -`; then states PGC = CLK and PGD = MOSI. | `messages_v4.s` `MSG_PIC_PINS_STATE` = `-\t-\tPGC\tPGD`; `messages_v3.s` = `PGC\tPGD\t-\t-`; `docs/generic/usage.md` column order. |
| 12 | fixed | `binary.md`: split `0xA4` into 416 (3-byte records) and 424 (4-byte records), and `0xA5` into 416 (count + command; one byte per count) and 424 (count only; six bytes per count); both `0xA5` rows say "no result byte". | `binary_io.c` `handle_pic_command_write()` (`commands_count = count * (416 ? 3 : 4)`); `handle_pic_command_write_and_read_bits()` (416 command byte and one byte per count; 424 fixed sequence and two bytes per read, three reads). |
| 13 | fixed | Removed the per-page v3 closers from `entry-and-setup.md`, `wiring.md` (kept one procedural note), `bus-operations.md`, `macros.md`, `config-words.md`, `binary.md`, `picprog.md`, `edge-cases.md`; kept the README status line and one v3 warning note under `wiring.md`. | Grep of the set; the remaining `not verified` hits are the README status, the binary caveat and the one wiring note. |
| 14 | fixed | `README.md`/`picprog.md`: banner corrected to the current source's `(Bus) Pirate Pic Programmer` (two m's; the old `Programer`/v0.1 output was stale); `e1` row is "external/none"; `v3a+` became "v3a and later". | `software/main.c` prints `"(Bus) Pirate Pic Programmer " PICPROG_VER`; `software/common.h` defaults `PICPROG_VER` to `"v0.3"`; `proc_menu.c` `set_pullup_voltage()` case 1 disables the source and prints the external/none message. |
| 15 | fixed | `README.md`: the seven quick-start bullets are a six-row table; the eight limits bullets are a six-row table plus one shared-grammar sentence. | Structural review. |
| 16 | fixed | `picprog.md`: "The tool prints its own progress; an example:" (no "trimmed").  `config-words.md`: the duplicate `0x00` → `0x02` → `0x08` → `0x04` transcript was removed; the page now links to `sessions.md` and states the two configuration-specific differences. | `sessions.md` remains the one canonical write/read session. |
| 17 | fixed | `bus-operations.md`: "A 14-bit payload is 16 clocks including the leading and trailing framing zeros, about 1.6 ms, plus the command delay." | `pic.c` `pic_write()` 6-bit data path: `write_zero(); write_bits(value, 14); write_zero();`; `PIC_PIN_DELAY = 100` µs. |

No finding was rejected: all 17 findings were reproduced against the
firmware, the host sample or the public tool source.

### Additional fix found while verifying

- `binary.md` worked `PIC1` exchange: the `Load Configuration` command needs
  a 16-bit zero payload, so the example now uses `0x80` with `0x00 0x00`
  instead of a bare `0x40` opcode.  Evidence: `binary_io.c` `case 0x80` reads
  two host bytes and calls `pic_write(temp)`; the 6-bit data path frames 14
  bits with a leading and trailing zero.

## Code-owner candidates

### CO-1 — partial inline settings are applied despite a parse error

- **File / symbol:** `Firmware/programmer_pic/pic.c`,
  `pic_setup_prepare()`; `Firmware/core_system/proc_menu.c`,
  `getint_with_spaces()` / `getint()`.
- **Behaviour:** `m 9 2` (missing delay) makes the second
  `getint_with_spaces()` return `0` and set `command_error`.  The
  `interactive` test in `pic_setup_prepare()` checks only the value ranges,
  so `0` is accepted, the mode is applied as 4b/16b at 0 ms and the header
  `PIC(mod dly)=(4 0)` is printed; the generic parser then reports
  `Syntax error at char 6`.  The mode change has already happened.
- **Why it matters:** a malformed one-line command half-succeeds instead of
  falling back to the prompts or refusing the change.
- **Docs status:** documented as a concrete trap in `edge-cases.md`; firmware
  was not changed.

## Cross-set changes for the main session

1. **Required — `docs/generic/values.md`, `l`/`L` section.**  Add one
   sentence reconciling the generic reversal with a native LSB-first mode:
   "In a mode with a native LSB-first shifter (PIC), selecting `L` reverses
   the value, so the wire order becomes MSB-first; the label describes the
   generic transformation, not the wire."  This is the other half of finding
   8; the PIC page now explains its side.
2. **Optional — root `README.md`, "Application support".**  The `picprog`
   bullet is still unlinked.  Link it to
   `docs/programmers/pic/picprog.md` or to
   `https://github.com/DangerousPrototypes/PiratePICprog`.  The PIC page now
   has the source link, so this is a convenience, not a blocker.
3. **No change required — `docs/generic/commands.md` and
   `docs/protocol/spi/wiring.md`.**  They already use `e` → `W` → `P`, which
   is now the PIC canonical order.  `docs/programmers/avrisp/wiring.md`
   already owns the v4 header table; the PIC page now inlines a copy.
   `docs/SPEC.md`'s PIC row remains accurate.

## Remaining questions

- **Adapter control polarity and VPP sequencing.**  The published schematic
  is the authority; the docs say not to assume active-high.  This is a
  hardware/adapter question, not a firmware one.
- **`picprog` availability and maintenance.**  The source is public; the
  project's current status is unknown.  The page says so.
- **`(1)` ID address.**  The macro hardcodes six configuration-space
  increments; this is part-specific.  The page says so.
- **Raw-wire `0xA4`/`0xA5`/`0xA7` script semantics.**  These remain
  unverified on hardware; the page keeps that caveat.
- **v3.**  The mode number (8) and VPU route are build-derived; one README
  status line and one wiring note remain.

## Files changed

- `docs/programmers/pic/README.md`
- `docs/programmers/pic/entry-and-setup.md`
- `docs/programmers/pic/wiring.md`
- `docs/programmers/pic/bus-operations.md`
- `docs/programmers/pic/macros.md`
- `docs/programmers/pic/config-words.md`
- `docs/programmers/pic/sessions.md`
- `docs/programmers/pic/binary.md`
- `docs/programmers/pic/picprog.md`
- `docs/programmers/pic/edge-cases.md`
- `docs-sweep/fixes-pic-round1.md` (this record)
