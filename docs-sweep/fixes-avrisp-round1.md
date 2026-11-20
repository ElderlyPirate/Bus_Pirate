# AVRISP-MKII docs — round 1 implementation record

**Scope:** `docs/programmers/avrisp/` plus this record.  Worktree
`bpv4-docssweep`, branch `docs/combined-sweep`; the `Firmware/` tree is the
combined image.  No firmware changes, no commits, no WORKLOG edits.

**Result:** 22 findings.  All 22 are fixed in the AVRISP set.  0 are
rejected.  One design limitation is recorded as a code-owner candidate
(CO-1, tied to finding 1) without touching firmware.  No cross-set change
is required for correctness; optional consistency items are listed at the
end.

**Files edited:** `README.md`, `entry.md`, `host-tools.md`, `isp.md`,
`pdi.md`, `tpi.md`, `serial-route.md`, `protocol.md`, `command-model.md`,
`rescue-clock-osccal.md`, `sessions.md`, `edge-cases.md`, `wiring.md`.
`license.md` already had an uncommitted "Part of the set" paragraph in the
working tree and was not touched.

## Decisions

| # | Decision | What changed (file + short quote) | Evidence |
|---|---|---|---|
| 1 | fixed (+ CO-1) | `host-tools.md` new §Windows USB first use: "The mkII identity exists only while the personality is active, and the personality ends after about five seconds without a completed request."  The section gives four paths: serial route for ISP first (install/allow the normal CDC driver on first plug-in, before entry, where there is no five-second deadline), one-pass Zadig binding with re-entry/retry, INF pre-staging, or a Linux host.  "If the device disappears before the install finishes, run the entry script again on the same USB port and repeat from the selection.  Windows normally keeps a completed binding for the same device identity, so later entries are faster."  `edge-cases.md`: "re-enter with `0x08` and repeat the one-pass binding; a completed binding is normally kept."  `entry.md`: "Start `avrdude` as soon as the entry script returns.  It does not wait for a device that is not yet enumerated; if it reports that it cannot find `03EB:2104`, re-enter with `0x08` and start it again."  `README.md`: "A first-time Windows libusb/WinUSB binding may need more than one entry ... For ISP first use, prefer the serial route." | `Firmware/programmer_avrisp/avrisp_mkii.h` `AVRISP_IDLE_TICKS 500` with the 10 ms timer in `avrisp_mkii.c` = 5 s, set in `avrisp_mkii_enter()` before `avrisp_usb_run()`.  `avrisp_usb_run()` waits for `usb_device_state >= CONFIGURED_STATE` while `avrisp_activity_ticks`; it does not reset the timer on configuration; only `avrisp_response_end()` resets it after a completed request.  The descriptors in `avrisp_mkii.c` are VID/PID `03EB:2104`, one vendor interface, bulk OUT `0x02`/IN `0x82`, 64-byte packets, 100 mA.  `avrdude` `src/usb_libusb.c` `usbdev_open()` takes one device-list snapshot (no wait).  See CO-1. |
| 2 | fixed | `isp.md` fuse table: "`lfuse`, `hfuse`, `efuse` name the low/high/extended fuse byte; the value is the part's, not a universal one." | AVR fuse bytes are low, high and extended; `avrisp_isp.c` `avrisp_isp_read_fuse_lock_osccal()` / `avrisp_isp_write_fuse_lock()` just forward host bytes and have no fuse table. |
| 3 | fixed | `isp.md` §Speed: "The engine uses the AVR069 index numbering (0–163): seven hardware steps are tabulated below, and the remaining software steps are an approximate ladder.  You do not need AVR069 for normal use; `-B` selects a period."  The range row is "7 … 163 \| about 96 kHz down to about 51 Hz (approximate) \| software".  The "complete mapping" and "provenance" claims are gone. | `avrisp_isp.c`: `avrisp_isp_hardware_speeds[]` has 7 entries; `avrisp_soft_sck_half_us[]` has 157 entries (indices 7–163), first 5 µs, last 9785 µs; `avrisp_isp_enable_target()` uses hardware for `< 7` and software otherwise and clamps above 163.  `avrisp_test.c` checks the array size and endpoints.  The host ladder in `avrdude` `src/stk500v2.c` `avrispmkIIfreqs[]` is 164 entries, index 7 = 96386 Hz, last = ~51 Hz. |
| 4 | fixed | `rescue-clock-osccal.md` new §Worked calibration exchange: `host → 05`, `host ← 05 00` success, `host ← 05 80` 32 ms timeout, `host ← 05 C0` extra bytes rejected before any pin; "The reply never carries an OSCCAL value, and the personality never writes the OSCCAL fuse or register."  Adds `avrdude -c avrispmkII -P usb -p m328p -O` and the Microchip AVR053 application-note link for the target firmware.  `protocol.md`: "It replies `05 00` on success, `05 80` on the 32 ms timeout, and `05 C0` for a request with extra bytes." | `avrisp_v2.c` `avrisp_v2_valid_request()` sets `CMD_OSCCAL` expected length 1; the invalid branch writes the command + `STATUS_CMD_FAILED` (`05 C0`) before dispatch.  `avrisp_isp.c` `avrisp_isp_calibrate()` writes `CMD_OSCCAL` + `response_status`, where the status is `STATUS_CMD_OK` (`05 00`) or `STATUS_CMD_TOUT` (`05 80`); it counts 1024+25 cycles at 32786.9 Hz (~32 ms) and never writes an OSCCAL value.  `avrdude` `src/main.c` has `-O`/`--osccal`; `src/stk500v2.c` `stk500v2_perform_osccal()` sends `CMD_OSCCAL`. |
| 5 | fixed | `command-model.md` new §Changing target interface: "End the current session, let the mode end, rewire with the personality inactive, then re-enter and select the new interface."  A five-row table covers ISP→PDI, ISP→TPI, PDI→TPI, TPI→PDI and PDI/TPI→ISP with the matching `LEAVE`, wait/re-enter, rewire and SETMODE/enter bytes.  Links were added from `isp.md` §Limits, `pdi.md` §PDI wiring notes, `tpi.md` §TPI wiring notes and the failed-enable note, `wiring.md` §PDI/§TPI, `edge-cases.md` AUX/interface rows, and `sessions.md` §An ISP session at command level. | `avrisp_xprog.c` `avrisp_xprog_set_mode()` shuts down only an active XPROG clock and never checks or leaves an ISP session; `avrisp_v2.c` does not police interface switches.  `avrisp_mkii.c` `avrisp_mkii_enter()` calls `avrisp_xprog_shutdown()` and `avrisp_isp_shutdown()` only at mode end.  `avrisp_isp.c` `avrisp_isp_change_reset_line(true)` leaves `RESET` asserted when an enable loop exhausts.  The new sequence keeps `CS` live until the mode ends and uses the actual `LEAVE`/`SETMODE` bytes. |
| 6 | fixed | `README.md` table row: "device identity, `avrdude`, host drivers, Atmel Studio note"; limits: "Atmel Studio is **not a supported host path** for this port; use `avrdude`."  `host-tools.md` heading: "Atmel Studio (not a supported host path)"; "This port does not provide a supported Atmel Studio procedure ... Use `avrdude` for a supported path."  The `0x07` upgrade answer `0xC9` is retained as a warning. | `avrisp_v2.c` has no Atmel Studio procedure and answers unknown `0x07` with `STATUS_CMD_UNKNOWN`; the identity is the only compatibility mechanism.  No supported-host claim remains. |
| 7 | fixed | `host-tools.md` new §Prerequisites table: `avrdude` with USB/libusb support, `avrdude` for the serial route, Python 3 + pyserial or PowerShell, a terminal, and the OS driver (Windows mkII libusb/WinUSB, Windows serial CDC, Linux udev, macOS none); one check command per route; "a successful check prints the device signature and exits with status 0.  If `03EB:2104` cannot be opened, the problem is the driver or the `avrdude` build; if it opens but the signature fails, the problem is the target or its wiring." | Host-side requirement, not firmware.  `avrdude` `src/usb_libusb.c` reports "avrdude was compiled without usb support" when built without libusb; `src/stk500v2.c` `stk500v2_set_sck_period_mk2()` uses the mkII frequency table. |
| 8 | fixed | `wiring.md` header: "Shrouded 2×6 header, key = pin 1.  Pins are numbered in pairs from the key (1/2, 3/4, …)"; pins 9, 10 and 11 are separate rows: `Vpu` (external pull-up reference), `+3V3` (switched 3.3 V rail), `+5V` (switched 5 V rail).  The "published convention" and "order within 9–11 unconfirmed" caveats are gone.  The previous "pins 1–6 down one row and 7–12 down the other" sentence was inconsistent with the official pair numbering and was removed. | Official Bus Pirate v4 design overview I/O-header table (`dangerousprototypes.com/docs/Bus_Pirate_v4_design_overview`): 1 MOSI, 2 CLOCK, 3 MISO, 4 CS, 5 AUX0, 6 AUX1, 7 AUX2, 8 ADC, 9 VEXTERN, 10 SWV33, 11 SWV50, 12 GND.  The v4 rendering `docs/assets/art/BusPirate4c-SketchupModel-rendering.jpg` shows the silkscreen labels.  The firmware uses named pins only (`avrisp_mkii.h` `AVRISP_*_LAT` and `BP_*` macros). |
| 9 | fixed | `pdi.md` operational table: "The firmware keeps an idle keep-alive clock of about 25 kHz on the line between commands and during long reply transfers; data transfers run at about 125 kHz."  Limits: "Data transfers run at about 125 kHz; the idle keep-alive clock is about 25 kHz.  The host cannot change either rate." | `avrisp_xprog.c` `avrisp_xprog_start_clock()` sets `PR4 = 639` at FCY 16 MHz -> 25 kHz; `XPROG_BIT_HALF_US 4` -> 125 kHz data rate. |
| 10 | fixed | `tpi.md`: "The ATtiny4/5/9/10 datasheet gives VIH = 0.6 × VCC for the TPI pins at a 5 V supply, so 3.0 V is the minimum input high and 3.3 V is within the specified range with a small margin.  A level shifter is optional ... The target's `RESET` pin is held low for the whole session, so its higher 0.9 × VCC input-high threshold does not apply while TPI is active."  `wiring.md`: "The ATtiny4/5/9/10 VIH at 5 V is 3.0 V, so the 3.3 V push-pull high is within the specified range with a small margin; a level shifter is optional." | ATtiny4/5/9/10 datasheet DC characteristics: VIH = 0.6 VCC for VCC 2.4–5.5 V (except RESET); RESET VIH = 0.9 VCC.  The port's TPI path remains "not verified" on a physical target. |
| 11 | fixed | `edge-cases.md` §Unverified deleted.  Each caveat stays with its feature; the TPI table gains: "TPI target does not answer at all ... TPI is **not verified** on a physical target for this port.  Check the wiring and the 5 V supply first, then use ISP or a known-good programmer." | Inline caveats remain in `README.md` availability, `tpi.md` scope/limits, `pdi.md` scope/limits, `isp.md` §Speed, `host-tools.md` Studio note and `rescue-clock-osccal.md` limits. |
| 12 | fixed | `sessions.md`: "Target-dependent bytes are marked as such."  `host-tools.md`: "Output varies by `avrdude` release.  The command lines above are the standard mkII and STK500v2 forms."  `protocol.md`: "The ISP connection-status parameter (`0xA1`) reads `0x00` when ISP is ready.  There are no host-visible connection-fault bits."  CRC wording is now "High byte, low byte, middle byte; not a normalised order." and "not a normalised order."  "deliberately shaped", "deliberately contains", "not a claim" and "preserved deliberately" are gone. | Wording-only; the behaviour is unchanged and matches `avrisp_v2.c` (`PARAM_STATUS_TGT_CONN` is `STATUS_ISP_READY`), `avrisp_xprog.c` CRC byte order and the descriptors. |
| 13 | fixed | `pdi.md`: "The erase command is loaded, then triggered by a PDI write to the address in the request; page erases use the same address field.  The host supplies the address; the firmware does not add a mode switch or reorder the sections."  "LUFA/Table" is gone. | `avrisp_nvm_pdi.c` `avrisp_xmega_erase_memory()`: chip erase loads `CHIPERASE` then `CMDEX`; APP/BOOT/page/EEPROM/user-signature erases load the command then trigger with a PDI write to the request address; EEPROM page/section erase tags the page first.  The exact silicon result remains target-dependent. |
| 14 | fixed | `protocol.md`: "PDI supports erase types 1–8 and rejects type 9.  TPI accepts types 1–9; type 1 is chip erase, and types 2–9 use the section-erase command.  Any other erase type is rejected before the target is touched."  `tpi.md` repeats the TPI range and rejection. | `avrisp_xprog.c` `avrisp_xprog_erase()` rejects any memory type outside 1–9; the TPI branch maps only type 1 to `TINY_NVM_CMD_CHIPERASE`, all others to `TINY_NVM_CMD_SECTIONERASE`; the PDI switch has no case for type 9, so it returns FAILED. |
| 15 | fixed | `sessions.md` ISP table: enter row is the concrete request `10 C8 64 20 03 02 53 03 AC 53 00 00` with reply `10 00`/`10 C0` and a note "timeout 200, stabilisation 100 ms, execution 32 ms, 3 sync loops, 2 ms byte delay".  The prose says the values are a valid set and a shorter set works if the target is ready. | `avrisp_isp.c` `avrisp_isp_enter_mode()` reads timeout, stabilisation, execution, sync loops, byte delay, poll value, poll index and four enable bytes; `avrisp_v2.c` `avrisp_v2_valid_request()` requires size 12 and poll index <= 4.  The native test uses the same field order. |
| 16 | fixed | `README.md`, `serial-route.md`, `entry.md` and `host-tools.md`: "`COM5` is a Windows example; use `/dev/ttyACM0` on Linux or the `/dev/cu.usbmodem…` name on macOS."  The prerequisites table also says to use the host's port name. | Host port naming; no firmware behaviour. |
| 17 | fixed | `entry.md` related docs: "[entering-and-framing.md](../../protocol/binary/entering-and-framing.md) — the shared binary layer (`BBIO1`, result bytes, `0x0F`)." | The target file exists; the relative-link check passes. |
| 18 | fixed | `entry.md` and `sessions.md` end-of-mode diagrams became `Actor \| Bytes \| Meaning` tables: personality prints `BBIO1`; host sends `0F`; personality replies `01`; banner and `HiZ>` follow. | Presentation only; bytes unchanged and match `binary_io.c` `BITBANG_COMMAND_RETURN_TO_TERMINAL` and `send_binary_io_mode_identifier()`. |
| 19 | fixed | `pdi.md` §Operational notes and `protocol.md` §XPROG properties are now `Topic \| Behaviour` tables (PDI clock, memory errors, failed enable, EEPROM page erase, APP/BOOT erase, CRC; CRC order, TPI CRC, unknown subcommand, erase length, address width, page mode, page boundaries). | Presentation only; facts unchanged and re-checked against `avrisp_nvm_pdi.c` and `avrisp_xprog.c`. |
| 20 | fixed | `entry.md`: "The custom-build switch is `BP_ENABLE_AVRISP_MKII_SUPPORT`."  `README.md`: "Add `BP_ENABLE_AVRISP_MKII_SUPPORT` to the custom feature list."  `edge-cases.md`: "add `BP_ENABLE_AVRISP_MKII_SUPPORT` to the feature list and rebuild." | `Firmware/configuration.h`: the v4 default block defines `BP_ENABLE_AVRISP_MKII_SUPPORT`; the `BP_CUSTOM_FEATURE_SET` block does not; the guard `#if defined(BP_ENABLE_AVRISP_MKII_SUPPORT) && !defined(BUSPIRATEV4)` `#error`s. |
| 21 | fixed | `rescue-clock-osccal.md` step 3: "**Optional — reference wiring only.** ... Leaving `SCK` connected does not affect the command: the personality stops driving `CLK` and makes it an input."  The wiring table row says "optional; the AVR053 reference wiring leaves it off (the command does not drive `CLK`)". | `avrisp_isp.c` `avrisp_isp_calibrate()` sets `SPI1STATbits.SPIEN = OFF`, `BP_CLK_RPOUT = 0`, `BP_CLK_DIR = INPUT` and never drives `CLK`; the AVR053 clock is on `MOSI`. |
| 22 | fixed | `serial-route.md`: "For PDI/TPI with `avrdude`, use the USB route: the entry byte and deadline are in [entry.md](entry.md), and the target paths are in [pdi.md](pdi.md) and [tpi.md](tpi.md)."  The old `isp.md §Entry` loop is gone. | Entry/deadline are owned by `entry.md`; PDI/TPI paths are owned by `pdi.md`/`tpi.md`; the link check passes. |

## Counts

- fixed in the AVRISP set: 22 (findings 1–22).
- rejected / not a defect: 0.
- code-owner candidates: 1 (CO-1, tied to finding 1).
- required cross-set changes: 0; optional consistency items: 2.

## Code-owner candidate

**CO-1 — initial USB configuration shares the five-second idle timer
(finding 1).**

`Firmware/programmer_avrisp/avrisp_mkii.h` defines `AVRISP_IDLE_TICKS 500`;
the Timer 3 tick is 10 ms (`avrisp_mkii.c` `_T3Interrupt()` and
`avrisp_timer_start()`), so the idle deadline is five seconds.
`avrisp_mkii.c` `avrisp_mkii_enter()` sets
`avrisp_activity_ticks = AVRISP_IDLE_TICKS` before `avrisp_usb_run()`.
`avrisp_usb_run()` waits for `usb_device_state >= CONFIGURED_STATE` while
`avrisp_activity_ticks` and does not reset that timer when configuration
completes.  Only `avrisp_response_end()` resets it, and only after a
completed request.  A first-time Windows driver binding that takes longer
than five seconds therefore ends the mode before the host can send the
first request.

The docs now work around this with the serial route for ISP and a
re-enter/retry binding procedure for the USB route.  Candidate firmware
fix: use a separate initial enumeration/configuration grace (for example
30 s), then fall back to the five-second inter-request idle.  Resetting the
idle timer on configuration alone does not help a bind that exceeds five
seconds, but it would protect the first request when configuration is fast.
Firmware was not changed.

## Cross-set changes

**Required: none.**  The pin numbering remains the same; the ADC/AUX/SMPS
pages continue to cite the AVRISP page and use pins 4–8 and 12 as before.
`docs/SPEC.md` needs no change.  The user set no longer uses "LUFA/Table"
or "preserved deliberately", but no other file must change for the set to
be correct.

**Optional consistency (not required):**

- `docs/dev/avrisp-design-notes.md` still says "LUFA/Table sequence
  retained" and "same (preserved deliberately)" for the CRC order.  The
  maintainer note is accurate as an internal note; aligning it with the new
  observable wording in `pdi.md`/`protocol.md` would remove the last
  occurrences of the phrase.
- `docs/capabilities/adc/wiring.md` calls the v4 numbering "the convention
  published in the AVRISP wiring table".  The AVRISP page now states the
  pinout as fact; changing "convention" to "pin numbering" would be a
  wording-only alignment.

## Verification performed

- Read `Firmware/programmer_avrisp/avrisp_v2.c`, `avrisp_v2.h`,
  `avrisp_isp.c`, `avrisp_mkii.c`, `avrisp_mkii.h`, `avrisp_xprog.c`,
  `avrisp_xprog.h`, `avrisp_nvm_pdi.c`, `avrisp_nvm_tpi.c`,
  `Firmware/protocol_binary/binary_io.c`, `Firmware/configuration.h`,
  `Firmware/core_system/proc_menu.c`, `Firmware/core_usb/prj_usb_config.h`
  and `Firmware/tests/avrisp_test.c` / `test_avrisp.py`.
- Ran `python Firmware/tests/test_avrisp.py`: `AVRISP native regression
  checks passed: 3012`.  This covers the seven hardware speeds, the 157
  software entries, ISP enable/poll/leave, XPROG enter/leave/erase/write/
  read/CRC/SET_PARAM, the TPI/PDI framing and the serial AVR068 framing.
- Checked `avrdude` `src/main.c` (`-O`/`--osccal`),
  `src/stk500v2.c` (`avrispmkIIfreqs[]`,
  `stk500v2_perform_osccal()`, `stk500v2_set_sck_period_mk2()`) and
  `src/usb_libusb.c` (`usbdev_open()`) for the host-side claims.
- Checked every relative Markdown link in the set: all resolve.
- Checked for TAB characters and table-column mismatches: none.  Only one
  non-table line is over 80 columns: the long AVR053 PDF URL in
  `rescue-clock-osccal.md`.
- Re-checked the four `0x05` replies against `avrisp_v2.c` /
  `avrisp_isp.c`; the `05 C0` path is the length-reject branch, not a
  target timeout.

## Remaining questions / assumptions

- The Windows first-use retry procedure is the documented workaround; a
  first-time Zadig/WinUSB bind inside five seconds was not verified on a
  physical Windows host.  The serial route is the deterministic ISP path.
- `avrdude -O` / `--osccal` exists in current builds; older builds may not
  have it, so the raw `05` exchange is documented for a custom host.
- The v4 pin map is confirmed from the official design overview and the
  board rendering, not from a physical board in this workspace.
- The ATtiny4/5/9/10 VIH figure is from the datasheet; TPI remains
  unverified on a physical target for this port.
- The software end of the ISP ladder is approximate; the docs no longer
  claim a complete frequency table.
