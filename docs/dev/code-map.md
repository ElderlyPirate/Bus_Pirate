# Code map — generic layer (maintainer notes)

**Status:** draft, 2026-11-01.  Developer-side companion to `docs/generic/`
(the user documentation carries no code references by policy).  Symbols only —
**no line numbers** (they rot with any edit).

| Area | Where |
|---|---|
| Terminal dispatcher & commands | `Firmware/proc_menu.c` — `serviceuser()` and the per-key cases (`h ? i m b o v t T f g c C k K L l p P e = \| ~ # $ a A @ W w d D & % s S O < ( " [ { ] } r` …); helpers `getint()`, `getrepeat()`, `getnumbits()`, `change_read_display()`, `changemode()`, `bp_change_mode()`, `cmdhistory()`, `getnumber()`, `print_version_info()`, `print_status_info()`, `print_pins_information()`, `set_display_mode()`, `set_baud_rate()`, `set_pullup_voltage()`, `convert_value()`, `switch_psu_on()`, `switch_psu_off()` |
| Numeric formatting, ADC, delays, timer | `Firmware/base.c` — `bp_adc_probe()`, `bp_adc_continuous_probe()`, `bp_write_formatted_integer()`, `bp_write_voltage()`, `bp_delay_ms()`, `bp_delay_us()`, `bp_timer_microseconds()`, `bp_timer_print()`, `bp_timer_reset()` |
| AUX pin, PWM, servo, frequency | `Firmware/aux_pin.c` — `bp_pwm_setup()`, `bp_servo_setup()`, `bp_frequency_counter_setup()`, `bp_aux_pin_set_low()` / `_high()`, `bp_aux_pin_read()` |
| Self-test | `Firmware/selftest.c` — `perform_selftest()` |
| Mode registry & callbacks | `Firmware/core.c` — protocol table, `bp_current_protocol()`, null-callback defaults |
| Binary layer & SUMP entry | `Firmware/binary_io.c` — `enter_binary_bitbang_mode()`, return `0x0F`; `Firmware/sump.c` — `enter_sump_mode()` |
| Strings & help | `Firmware/messages_v4.s` / `.h`, `Firmware/messages_v3.s` |
| Limits & defaults | `Firmware/configuration.h` — history length, macro slots, buffer size, default feature set; `Firmware/main.c` — startup defaults, v3 banner |
| Board strings & VPU helpers | `Firmware/base.h` — `BP_FIRMWARE_STRING`; `hardwarev4.h` / `hardwarev3.h` — `BP_VERSION_STRING`, VPU source helpers (`bp_enable_3v3_pullup` / `bp_enable_5v0_pullup`; empty no-ops on v3) |
| Build version | `Firmware/Makefile` — `VERSION` (git short hash baked into the banner) |

Related: `docs/SPEC.md` (capability matrix); the board reference card under
`Documentation/documents/` (for the `Vpu` pin label; moves under `docs/` with
the restructure).