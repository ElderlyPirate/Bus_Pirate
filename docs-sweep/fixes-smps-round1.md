# SMPS docs - round 1 implementation record

**Scope:** `docs/capabilities/smps/` plus this record.  Worktree
`bpv4-docssweep`, branch `docs/combined-sweep`; the `Firmware/` tree is the
combined image.  No firmware changes, no commits, no WORKLOG edits.

**Result:** 17 findings.  All 17 are fixed in the SMPS set.  0 are rejected.
Two underlying design gaps are recorded as code-owner candidates (CO-1 and
CO-2) without touching firmware.  Three required correction items in three
other files are cross-set only; one optional consistency item is listed too.

The add-on was identified from the legacy hardware material in the repository
history: the schematic image and BOM at
`5b54791:hardware-other/SMPS/BusPirate-SMPS.sch.png` and
`5b54791:hardware-other/SMPS/BusPirate-SMPS.htm` (also
`hardware/BusPirate-SMPS.*` in earlier revisions).  The archived Dangerous
Prototypes wiki page linked from `Firmware/configuration.h` carries the same
part list and no rating text.  The schematic title is `BusPirate-SMPS`, the
board silk is `BUS PIRATE SMPS`, and the revision field is blank.

## Decisions

| # | Decision | What changed (file + short quote) | Evidence |
|---|---|---|---|
| 1 | fixed | `binary.md`: "The group is still routed.  `0xF2 <operand>` replies `0x00`, consumes the operand, and **stays in `RAW1`**."  The old `0xF2 0x00` exit example is gone; the page adds "The operand is consumed even when it is `0x00`; a zero operand is not an exit request on this path."  `edge-cases.md`: "this includes `0x00`: `0xF2 0x00` replies `0x00`, consumes the zero, and does not leave raw wire." | `Firmware/protocol_binary/binary_io.c` `handle_smps_command()` disabled branch: `if ((uint8_t)command >= SMPS_COMMAND_START) { user_serial_read_byte(); } REPORT_IO_FAILURE();`.  `Firmware/tests/test_binary_capabilities.py --case smps-frame` passes: every `0xF0`-`0xFF` opcode writes one failure byte and every start reads exactly one operand, including `0xF2`. |
| 2 | fixed | `wiring.md` new "Add-on identification" section: "The add-on named in this set is the legacy Dangerous Prototypes **Bus Pirate SMPS** board."  It lists the 2x5 connector pinout, the `JP1` feedback jumper (`1-2` = `V3ADC`, `2-3` = `V2G0ADC`), the `VPP`/`GND` output terminal, links the archived Dangerous Prototypes wiki page, and says the schematic gives no voltage/current rating and is not in this tree.  `README.md` names the board and links `wiring.md`. | Legacy schematic image at `5b54791:hardware-other/SMPS/BusPirate-SMPS.sch.png`: `IO` pins 1 VPPEN, 2 MCLR, 3 PGD, 4 PGC, 5 PWM, 6 VTARGET, 7 V3ADC, 8 5V, 9 V2G0ADC, 10 GND; `JP1` pins 1 V3ADC, 2 FB, 3 V2G0ADC; `X1-1` VPP, `X1-2` GND; converter fed from `VTARGET`; `5V`, `PGC`, `PGD`, `MCLR` and `VPPEN` have no drawn connection.  BOM `MA05-2`, `1X03`, `W237-102`; no rating column or value for the power parts.  `Firmware/configuration.h` points at the DP page and already warns compatibility is unclear. |
| 3 | fixed | `README.md` limits: "The firmware's cutoff can act only for requests `0x200`-`0x526`; whether the add-on can reach the target is unknown, and no request is a promise of a regulated output."  `binary.md` limits: "No request is a promise of a regulated output."  `feedback-loop.md` heading is "Where the firmware cutoff can act" and its lead says "This table describes the firmware comparison, not the add-on's regulated output range."  `edge-cases.md` heading is "A request above the firmware cutoff range". | `Firmware/capability_smps/smps.c` `smps_start()` computes `voltage_out = requested * 45 / 58`; `_ADC1Interrupt()` cuts duty only when `measurement > voltage_out`.  The firmware cannot see the add-on's output scaling or range. |
| 4 | fixed | `README.md` heading is now "Shortest byte sequence (not a safety claim)"; the warning immediately before the table says "Keep the load disconnected and verify the add-on output with your own measurement before connecting anything."  The first start is `0xF2 0x00` for `0x200`, "the lowest reachable value", not `0x0258`. | The reachable start range is `0x200`-`0xFFF` (the low nibbles `0` and `1` are read/stop).  `smps.c` has no over-voltage, over-current or temperature protection; the add-on rating is not documented. |
| 5 | fixed (+ CO-1) | `binary.md` worked session: "first read; discard it" then "read again after at least one conversion"; the prose says "There is no freshness flag; treat that first read as stale and use a later read."  `feedback-loop.md`: "There is no freshness flag.  Discard the first read after a start, or poll until the value changes..."  The reply-shape table adds a "no freshness flag" note. | `Firmware/capability_smps/smps.c` `smps_start()` never clears `smps_state.voltage_reading`; `smps_adc()` transmits that stored value.  See CO-1. |
| 6 | fixed (+ CO-2) | `README.md`: "No pull-up is required ... in the normal push-pull case" and "or the Bus Pirate needs a power cycle or hardware reset before the start."  `wiring.md` names the v4 OpenOCD open-drain path, says "the SMPS start does not clear the open-drain bit", "A v4 terminal reset (`#` or `0x0F`) does not clear it either; a power cycle or hardware reset does", and gives the pull-up alternative.  `edge-cases.md` adds "`AUX1` was left open drain". | `Firmware/tool_openocd/openocd.c` `binOpenOCDPinMode()` sets `OOCD_TRST_ODC` (= `BP_AUX1_ODC`) for `MODE_JTAG_OD`.  `Firmware/capability_smps/smps.c` `smps_start()` writes `BP_AUX1_DIR`, `BP_AUX1_LAT` and `BP_AUX1_RPOUT` but never `BP_AUX1_ODC`.  `Firmware/core_system/base.c` `bp_reset_board_state()` does not clear ODC bits.  See CO-2. |
| 7 | fixed | `feedback-loop.md` ownership row: "refusal count `0x01`; no check runs; the self-test then sits in its idle echo loop - send `0xFF` to leave (`BBIO1` is not re-printed)."  `edge-cases.md` new entry gives the echo example (`0x05` returns `0x06`) and the `0xFF` recovery. | `Firmware/capability_selftest/selftest.c` `perform_selftest()` returns 1 before any check when `bp_adc_foreground_available()` is false.  `Firmware/protocol_binary/binary_io.c` `binary_io_self_test()` sends the error count, then loops echoing `input_byte + errors` for every byte except `0xFF`; `0xFF` sends `0x01` and returns without printing `BBIO1`.  `--case selftest` passes the refusal path. |
| 8 | fixed | `feedback-loop.md`: "`target` is a raw comparison threshold on the same numeric scale as the 10-bit sample, but it is a 16-bit value.  It ranges from 397 at `0x200` to 3177 at `0xFFF`; a 10-bit sample can never reach a target above 1023."  Table header is "Target (threshold)".  `edge-cases.md` calls it "the comparison threshold". | `smps_start()` stores `(uint32_t)requested * 45 / 58` in a `uint16_t`; the ISR compares a 10-bit `ADC1BUF0` against it.  `--case smps` checks all 3584 targets. |
| 9 | fixed | `edge-cases.md`: "The duty follows the floating reading: it may sit at the computed duty or be cut to zero, and the output is uncontrolled."  Recovery adds "Remove the load and do not touch the output until the feedback path is verified; a zero sample is not proof of 0 V output."  `wiring.md` feedback section repeats the uncontrolled result.  `binary.md` limits adds the same no-presence-test warning. | `_ADC1Interrupt()` sets `OC5R = (measurement > voltage_out) ? 0 : pwm_duty_cycle`; a floating ADC can be on either side.  No fault or presence check exists. |
| 10 | fixed | `README.md` five-minute bullet: "**Check the build:** once in `RAW1`, send `0xF1`.  `0x01` means SMPS is built in; `0x00` means it is not.  It does not start the supply."  The shortest-session table also lists the optional check. | Enabled build: `0xF1` hits `SMPS_COMMAND_STOP` and replies `0x01` even before a start.  Disabled build: `handle_smps_command()` reports `0x00`.  Neither path starts the converter.  The "no prebuilt image" sentence was not added because only this tree was checked; the docs already say a build change is required. |
| 11 | fixed | `README.md` adds "Read in this order: wiring -> binary -> feedback-loop -> exit -> edge-cases.  Wire the Bus Pirate side before the first start."  The "Wiring headline" section now sits above "The five-minute version". | Presentation-only; the moved heading keeps its `#wiring-headline` anchor and the link check resolves it. |
| 12 | fixed | `README.md`: "The NULs need a host program or script; a terminal cannot send them (the Python example is in `binary.md#a-host-sequence`)." | The entry sequence is 20 raw NUL bytes; a normal terminal cannot emit them.  The Python example is at `binary.md` "A host sequence". |
| 13 | fixed | `README.md` and `wiring.md`: "No `k` selection is needed for SMPS: the start routes the PWM to `AUX1` regardless of the selected AUX target." | `smps_start()` writes the fixed `BP_AUX1_*` macros; it never reads `mode_configuration.alternate_aux`.  `k` only changes the terminal AUX target. |
| 14 | fixed | `feedback-loop.md` ownership table adds "`W` \| terminal \| refused with the same `ADC busy` line and syntax error; `w` still works".  The note says `W` checks the rails through the ADC, `w` only turns them off, and the binary `0x4x` supply bit is not blocked.  `wiring.md` repeats the distinction. | `Firmware/core_system/proc_menu.c` `case 'W'` calls `switch_psu_on()`, which calls `bp_adc_check_foreground()` first; `case 'w'` calls `switch_psu_off()` with no ADC check.  `Firmware/protocol_binary/binary_io.c` `handle_configure_peripherals()` calls `bp_binary_io_peripherals_set()` with no ADC guard, so the binary `0x4x` bit is not blocked. |
| 15 | fixed | `exit.md` table: `0x0F` = "yes on v4 (terminal reset)", `#` = "yes on v4 (terminal reset)".  The `0x0F` section defines the term once: "the **terminal reset** (the firmware's board reset)".  The `m` row says "board reset" because `m` runs only the reset step, not the terminal-state reset. | `Firmware/core_system/proc_menu.c` `case '#'` calls `reset_terminal_state()` -> `bp_reset_board_state()`; `0x0F` returns from the binary loop and the terminal path calls the same `reset_terminal_state()`; `bp_change_mode()` calls `bp_reset_board_state()` directly.  `bp_reset_board_state()` calls `smps_stop()` on an SMPS-enabled v4 build. |
| 16 | fixed | `binary.md` command table: "read the last feedback sample (raw counts)"; the same page adds "The sample is the ADC-node count, not a calibrated output voltage."  `README.md` read bullet uses "feedback sample".  No "output sample" remains in the set. | `smps_adc()` sends `smps_state.voltage_reading`, the ADC feedback-node count; it never converts to volts. |
| 17 | fixed | `binary.md` "Reply shapes" is now a table: request, reply, shape, notes.  It folds in the stale/zero cases and the disabled-build single byte. | Presentation-only; the reply shapes are unchanged and match `smps_adc()`, `handle_smps_command()` and `binary_io_self_test()` where relevant. |

## Code-owner candidates

**CO-1 - no sample-freshness indication (finding 5).**
`Firmware/capability_smps/smps.c`: `smps_start()` leaves
`smps_state.voltage_reading` unchanged; `smps_adc()` transmits the stored
value; `_ADC1Interrupt()` overwrites it only on a new conversion.  There is no
valid/fresh flag.  A host cannot distinguish the previous run's sample from a
new one without a host-side discard/poll rule.  Candidate fix: clear
`voltage_reading` at start, or add a validity bit that `0xF0` can report.  The
docs now state the discard/poll rule; firmware is unchanged.

**CO-2 - `AUX1` can remain open drain into an SMPS start (finding 6).**
`Firmware/tool_openocd/openocd.c` `binOpenOCDPinMode()` sets
`OOCD_TRST_ODC`, which is `BP_AUX1_ODC` on v4, for `MODE_JTAG_OD`.
`Firmware/capability_smps/smps.c` `smps_start()` sets `BP_AUX1_DIR`,
`BP_AUX1_LAT` and `BP_AUX1_RPOUT` but never clears `BP_AUX1_ODC`; the PWM high
stays released.  `Firmware/core_system/base.c` `bp_reset_board_state()` also
does not clear ODC bits, so a v4 terminal reset does not recover it; only a
power cycle or hardware reset does.  Candidate fix: set
`BP_AUX1_ODC = PUSH_PULL` in `smps_start()` (and consider clearing the ODC
bits in `bp_reset_board_state()`).  The docs now state the precondition and
the recovery; firmware is unchanged.

## Cross-set changes for the main session

1. **`docs/protocol/binary/sub-protocols.md` (findings 3 and 16).**
   Line 289 says `0xF0` returns the "last output sample"; change to "last
   feedback sample (ADC-node count, not a calibrated voltage)".  Line 295
   says "The regulation loop, the useful range and the electrical caveats";
   change "useful range" to "the range where the firmware cutoff can act" and
   add that whether the add-on reaches a target is unknown.
2. **`docs/generic/edge-cases.md` (finding 14).**
   The SMPS refusal row lists `d`, `D`, `P`, `e`, `g`, `S`, `v` or `~`;
   add `W` (the terminal rail-on command).  Optionally note that `w` and `p`
   are not blocked.  The code path is `switch_psu_on()` ->
   `bp_adc_check_foreground()`, while `switch_psu_off()` has no guard.
3. **`docs/dev/verification.md` (finding 2).**
   The SMPS row says "no SMPS board available; every electrical value is
   code-derived".  A legacy schematic and BOM are available in the repository
   history (`5b54791:hardware-other/SMPS/`).  Change to: board not available
   for electrical verification; connector/pinout and jumper from the legacy
   schematic; input/output ratings still undocumented; firmware behaviour
   code-derived.
4. **`docs/capabilities/selftest/binary.md` and `checks.md` (optional, not a
   defect).**  Both use `0xF2 0x58` as the "SMPS-specific" start example.
   It is not a safety example, so no change is required.  If the main session
   wants one convention across the tree, switch them to `0xF2 0x00` / `0x200`.

## Files changed

- `docs/capabilities/smps/README.md`
- `docs/capabilities/smps/binary.md`
- `docs/capabilities/smps/feedback-loop.md`
- `docs/capabilities/smps/wiring.md`
- `docs/capabilities/smps/exit.md`
- `docs/capabilities/smps/edge-cases.md`
- `docs-sweep/fixes-smps-round1.md` (this record)

## Verification performed

- `python test_binary_capabilities.py --case smps-frame` -> PASS: all
  disabled SMPS opcodes consume exactly their declared operands (including
  `0xF2 0x00`).
- `python test_binary_capabilities.py --case smps` -> PASS: 3584 SMPS targets,
  zero-duty feedback ownership, single-buffer ISR reads and repeated stop.
- `python test_binary_capabilities.py --case selftest` -> PASS: self-test
  refusal precedes rails/pins/ADC/reset.
- Static read of `Firmware/capability_smps/smps.c`, `smps.h`,
  `Firmware/protocol_binary/binary_io.c`, `Firmware/capability_adc/adc.c`,
  `Firmware/core_system/base.c`, `proc_menu.c`, `Firmware/configuration.h`,
  `Firmware/core_messages/messages_v3.s` / `messages_v4.s`, and
  `Firmware/tool_openocd/openocd.c`.
- Legacy hardware: schematic PNG, BOM HTM and archived DP wiki page (BOM and
  schematic only; no ratings).
- Link and anchor check over the six SMPS files: all relative links resolve
  and all anchors exist; no TAB characters.
- No bench or electrical verification was performed.

## Remaining questions

- The legacy board has no revision letter.  Confirm that the physical board
  matches the schematics above and that `JP1` selects the ADC pin actually
  wired to the Bus Pirate.
- The input and output voltage/current ratings are not documented; obtain the
  full Dangerous Prototypes hardware archive and measure before applying a
  load.
- No v4 adapter cable or direct plug-in is documented.  Confirm continuity of
  the individually wired PWM, feedback, ground and supply wires.
- Whether a prebuilt SMPS-enabled image exists outside this tree was not
  checked.  The docs state only that enabling it is a build change.
- The terminal `d`/`D`/`W` refusal paths are defensive; only the code path was
  verified, not a reachable terminal state.  Normal routes to `HiZ>` run the
  terminal reset and stop the loop.
- The legacy v3.5 schematic labels its single AUX pin `AUX1`, while the v3
  firmware's pin map calls that pin `AUX`.  The SMPS set now says "no `AUX1`
  target in the v3 firmware" rather than "no `AUX1` pin".  The AUX set may
  want to reconcile the silkscreen label; this is a naming note, not an SMPS
  finding.
- The switching frequency (about 125 kHz) is code-derived from the `0x7F`
  period and the system clock, not measured.  The docs keep the `unverified`
  caveat.
- CO-1 and CO-2 are firmware owner decisions; the docs describe the current
  behaviour and the workarounds.
