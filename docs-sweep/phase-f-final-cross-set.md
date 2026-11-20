# Phase F — final cross-set consistency pass

Worktree `bpv4-docssweep`, branch `docs/combined-sweep`, docs head `84a97865`.
Checked against the final tree in the worktree (combined `9b179cb5`).
No firmware edits; no commit or push. Only the touched sets were reopened,
and only for the recorded cross-set facts.

## Applied changes

| Item | File | Change |
|---|---|---|
| V4 `AUX1` ODC clear | `protocol/binary/commands.md` | The `0x0F` terminal-reset paragraph now says the reset clears the v4 `AUX1` open-drain bit (with the Timer2/OC5 detail). |
| V4 `AUX1` ODC clear | `protocol/binary/entering-and-framing.md` | Same fact added to the v4 reset bullet. |
| V4 `AUX1` ODC clear | `protocol/binary/edge-cases.md` | Added to the `0x40` reset-recovery summary and the PWM-after-exit behaviour. |
| V4 `AUX1` ODC clear | `tools/openocd/entry-and-exit.md` | Added to the v4 terminal-reset summary. |
| V4 `AUX1` ODC clear | `capabilities/auxiliary/aux-pin.md` | Board-reset/release paragraph now names the `AUX1` ODC clear and the SMPS-start clear. |
| V4 `AUX1` ODC clear | `capabilities/auxiliary/wiring.md` | “Leaving the board” reset paragraph now names the `AUX1` ODC clear and the SMPS-start clear. |
| V4 `AUX1` ODC clear | `capabilities/auxiliary/pwm-servo.md` | Note after the reset table: every v4 board reset clears `AUX1` ODC; an SMPS start does too. |
| AVRISP timeouts | `protocol/binary/README.md` | Entry summary now splits the USB 120 s startup budget, the serial 5 s entry window and the 5 s idle. |
| AVRISP timeouts | `protocol/binary/sub-protocols.md` | Same split; adds that empty packets reset neither window. |
| AVRISP timeouts | `protocol/binary/entering-and-framing.md` | Leaving-table row now says startup budget (120 s USB / 5 s serial) or 5 s idle or reset. |
| AVRISP timeouts | `protocol/binary/examples.md` | AVRISP example summary now says startup budget / serial entry / idle timeout. |
| AVRISP timeouts | `protocol/binary/commands.md` | AVRISP entry paragraph now requires the first complete non-empty request and notes empty packets do not extend either window. |
| AVRISP timeouts | `protocol/binary/edge-cases.md` | AVRISP self-exit entry now says complete non-empty request and empty packets extend neither window. |
| AVRISP timeouts | `dev/avrisp-design-notes.md` | New startup-budget bullet (120 s USB, 5 s serial entry, 5 s idle); empty packets extend neither. |
| Stale exit link | `protocol/jtag/xsvf.md` | Exit mapping now points at `tools/xsvf/players.md#exit-status-and-the-next-run`, not the tool README map. |

## Evidence

- `Firmware/core_system/base.c` `bp_reset_board_state()`: releases the v4
  `AUX1`/`AUX2` directions, clears `BP_AUX1_ODC` and both latches, clears
  `T2CONbits.TON`, then detaches the AUX source.  There is no `AUX2` ODC bit,
  so the pages state only the `AUX1` clear.
- `Firmware/capability_smps/smps.c` `smps_start()`: clears `BP_AUX1_ODC` and
  `voltage_reading`; `smps_stop()` leaves the stored sample unchanged.
- `Firmware/programmer_avrisp/avrisp_mkii.h`: `AVRISP_USB_STARTUP_TICKS`
  (12000 x 10 ms = 120 s), `AVRISP_IDLE_TICKS` (500 x 10 ms = 5 s).
- `Firmware/programmer_avrisp/avrisp_mkii.c` `avrisp_usb_run()`: sets the
  startup budget, switches to idle only after a non-empty request, and ignores
  an empty packet without resetting either window; `avrisp_serial_run()` starts
  with the 5 s idle and refreshes it on a received byte.
- `Firmware/tool_xsvf/micro.c` / `ports.c`: the XCOMMENT loop exits on the
  stalled flag and the run returns `0x01`; the earlier cross-set correction is
  already present and consistent with `tools/xsvf`.
- `Firmware/tool_openocd/openocd.c`: the v3 tap shift is guarded with `if (j)`,
  so a zero count is a guarded no-op on both builds; the existing JTAG/tool
  pages already state this.

## Consistency sweep (item 3)

| Fact | Result |
|---|---|
| Timer2 + `AUX1` ODC on reset | Core and generic already had it; the seven list pages now match. No remaining “Timer2 still runs” or “does not stop Timer2” claim. |
| Silent I2C `0x09` | I2C binary says exactly two bytes, silent; the AUX binary/edge-case pages and generic agree. No stale “consume the text” wording. |
| `Baud>150k` | UART pages quote `Baud>150k: The BP cannot measure above 150000`; no old 16,000,000 text remains in generic/core. |
| 2-Wire ATR | `2wire/macros.md`, `smartcard.md`, `edge-cases.md` already have reset released before sampling and CS high; no cross-set page repeats the invalid-ATR claim. |
| SPI CS `0x02`/`0x03` | SPI binary already makes `0x02` assert and enable CS, `0x03` release it; no “cannot recover” wording remains. |
| XCOMMENT / v3 zero count | JTAG, XSVF and OpenOCD pages already state the `0x01` unwind and the guarded zero-count no-op; the binary page agrees. |
| SMPS start sample/ODC | SMPS binary/README/exit/feedback pages already state the clear-at-start rule; the AUX pages now carry the ODC half. |
| DIO five/seven-pin park | DIO pages and `dev/verification.md` already say five pins on v3, seven on v4; AUX wiring matches. |
| ADC two-rail | ADC and selftest already distinguish the switched 3.3 V branch from AVdd; core qualifies `BP_ADC_3V3`. |
| PIC `m 9 2` fallback | PIC entry/edge-case pages state nothing from the partial line is applied; `pic_setup_prepare()` matches. |
| Makefile/loader | Dev building/flashing/bootloader pages already carry the flag stamp, XC16 default, 4.14 loader and `0xA780` boundary; nothing outside `dev/` repeats the old claims. |

## Remaining item

- **`docs/SPEC.md` UART row** lacks the “v3 build-only” note recorded by the
  UART Phase F audit.  Not edited, per the instruction not to touch SPEC unless
  an accepted audit required it; recorded here for the publication decision.

No unresolved contradiction was found in the touched sets.
