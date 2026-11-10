# Code map — firmware (maintainer notes)

**Status:** draft.  Developer-side companion to `docs/generic/` and
`docs/core/` (the user documentation carries no code references by policy).
Paths are relative to `Firmware/`.  Symbols only — **no line numbers** (they
rot with any edit).

| Area | Where |
|---|---|
| Terminal dispatcher & commands | `core_system/proc_menu.c` — `serviceuser()` and the per-key cases (`h ? i m b o v t T f g c C k K L l p P e = \| ~ # $ a A @ W w d D & % s S O < ( " [ { ] } r` …); helpers `getint()`, `getrepeat()`, `getnumbits()`, `change_read_display()`, `changemode()`, `bp_change_mode()`, `cmdhistory()`, `getnumber()`, `print_version_info()`, `print_status_info()`, `print_pins_information()`, `set_display_mode()`, `set_baud_rate()`, `set_pullup_voltage()`, `convert_value()`, `switch_psu_on()`, `switch_psu_off()` |
| Numeric formatting, delays, timer | `core_system/base.c` — `bp_write_formatted_integer()`, `bp_write_voltage()`, `bp_delay_ms()`, `bp_delay_us()`, `bp_timer_microseconds()`, `bp_timer_print()`, `bp_timer_reset()`, `bp_reset_board_state()`, the `user_serial_*` layer |
| ADC probe | `capability_adc/adc.c` — `bp_read_adc()`, `bp_adc_probe()`, `bp_adc_continuous_probe()` |
| AUX pin, PWM, servo, frequency | `capability_aux/aux_pin.c` — `bp_pwm_setup()`, `bp_servo_setup()`, `bp_frequency_counter_setup()`, `bp_aux_pin_set_low()` / `_high()`, `bp_aux_pin_read()` |
| Self-test | `capability_selftest/selftest.c` — `perform_selftest()` |
| LCD (HD44780) adapter | `capability_hd44780/hd44780.c` — `LCDsetup()`, `LCDsetup_exc()`, `LCDwrite()`, `LCDstart()` / `LCDstop()` (register select), `LCDmacro()`, `LCDpins()`, `HD44780_Reset()` / `_Init()` / `_WriteByte()` / `_WriteNibble()` / `_SPIwrite()`; selected by `BP_ENABLE_HD44780_SUPPORT`; registered in `core_system/core.c` (`"LCD"` entry, cleanup `spi_disable_interface`) |
| Mode registry & callbacks | `core_system/core.c` — protocol table, `bp_current_protocol()`, `bp_protocol_by_index()`, null-callback defaults |
| Binary layer & SUMP entry | `protocol_binary/binary_io.c` — `enter_binary_bitbang_mode()`, return `0x0F`; `protocol_sump/sump.c` — `enter_sump_mode()`, reset and exit bytes |
| JTAG, XSVF and OpenOCD | `protocol_jtag/jtag.c` — `jtag()` TAP engine, `jtag_setup()`, state helpers; `tool_xsvf/micro.c` — XSVF player (`xsvfDoXSIR()`, …); `tool_openocd/openocd.c` — `binOpenOCD()` adapter |
| USB CDC | `core_usb/cdc.c` — `initCDC()`, `getc_cdc()`, `putc_cdc()`, `CDC_Flush_In_Now()`, `getinbuf_cdc()`/`write_cdc()`; `core_usb/usb_stack.c` — `usb_init()`, `usb_handler()`, `usb_handle_setup()` |
| Bitbang engine | `core_bitbang/bitbang.c` — `bitbang_setup()`, value transfer, waiting-I2C helpers |
| Onboard EEPROM | `core_eeprom/onboard_eeprom.c` — `eeprom_initialize()`, `eeprom_test()` |
| Strings & help | `core_messages/messages.c` — `bp_message_write_buffer()`/`_line()`, `print_help()`, `agree()`; `core_messages/messages_v4.s` / `.h`, `messages_v3.s` / `.h` |
| Limits & defaults | `configuration.h` — history length, macro slots, buffer size, default feature set; `core_system/main.c` — startup defaults, v3 banner |
| Board strings & VPU helpers | `core_system/base.h` — `BP_FIRMWARE_STRING`; `hardwarev4.h` / `hardwarev3.h` — `BP_VERSION_STRING`, VPU source helpers (`bp_enable_3v3_pullup` / `bp_enable_5v0_pullup`; empty no-ops on v3) |
| Build version | `Makefile` — `VERSION` (git short hash baked into the banner) |

Related: `docs/SPEC.md` (capability matrix), `docs/core/README.md` (module map
and boundaries), `docs/dev/shared-machinery.md` (shared helper contract).
