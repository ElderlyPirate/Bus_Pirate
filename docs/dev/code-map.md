# Code map — cross-group index (maintainer)

**Status:** maintainer-facing.  This is the single cross-group symbol index for
the grouped firmware: one row per source group, the main entry symbols for
that group, and a pointer to the page that owns the detail.  User pages carry
no code references by policy; this page and `docs/core/` are where the code
lives.

Paths are relative to `Firmware/`.  Symbols only — **no line numbers** (they
rot with any edit).  A row is a starting point, not an inventory: follow the
group into `docs/core/` (core internals) or the matching function set before
changing behaviour.

## Core

| Area | Where |
|---|---|
| Terminal dispatcher | `core_system/proc_menu.c` — `serviceuser()` and the per-key `case` labels |
| Command-line editing | `core_system/proc_menu.c` — `handle_character()`, `handle_backspace()`, `handle_delete()`, `handle_escape_key()`, `handle_up_arrow()` / `handle_down_arrow()`, `load_user_macro()`, `update_binary_prefix()` |
| Value entry & parsing | `core_system/proc_menu.c` — `getint()`, `getint_with_spaces()`, `consumewhitechars()`, `getrepeat()`, `getnumbits()`, `getnumber()`, `getlong()`, `change_read_display()` |
| Mode switch & status | `core_system/proc_menu.c` — `changemode()`, `bp_change_mode()`, `set_display_mode()`, `set_baud_rate()`, `set_pullup_voltage()`, `convert_value()`, `switch_psu_on()` / `switch_psu_off()`, `print_version_info()`, `print_status_info()`, `print_pins_information()` |
| Numeric formatting & delays | `core_system/base.c` — `bp_write_formatted_integer()`, `bp_write_dec_word()` / `bp_write_dec_byte()`, `bp_write_dec_dword()`, `bp_write_voltage()`, `bp_delay_ms()` / `bp_delay_us()`, `bp_timer_microseconds()`, `bp_reset_board_state()`; `core_system/base.h` — `user_serial_transmit_character()`, `user_serial_read_byte()`, `user_serial_ready_to_read()`, `user_serial_wait_transmission_done()` |
| Mode registry & callbacks | `core_system/core.c` — protocol table, `bp_current_protocol()`, `bp_protocol_by_index()`, null-callback defaults |
| USB CDC | `core_usb/cdc.c` — `initCDC()`, `getc_cdc()`, `putc_cdc()`, `CDC_Flush_In_Now()`, `getinbuf_cdc()` / `write_cdc()`; `core_usb/usb_stack.c` — `usb_init()`, `usb_handler()`, `usb_handle_setup()` |
| Bitbang engine | `core_bitbang/bitbang.c` — `bitbang_setup()`, `bitbang_write_value()`, `bitbang_read_value()`, waiting-I2C helpers |
| Onboard EEPROM | `core_eeprom/onboard_eeprom.c` — `eeprom_initialize()`, `eeprom_test()` |
| Strings & help | `core_messages/messages.c` — `bp_message_write_buffer()`, `bp_message_write_line()`, `print_help()`, `agree()`; `core_messages/messages_v4.s`, `messages_v4.h`, `messages_v3.s`, `messages_v3.h` |
| Limits & feature sets | `configuration.h` — `BUSPIRATEV3` / `BUSPIRATEV4` / `BP_CUSTOM_FEATURE_SET` blocks, `BP_ENABLE_*` switches, history/macro/buffer sizes; `core_system/main.c` — startup defaults |
| Board strings & VPU helpers | `core_system/base.h` — `BP_FIRMWARE_STRING`; `hardwarev4.h` / `hardwarev3.h` — `BP_VERSION_STRING`, VPU source helpers (`bp_enable_3v3_pullup` / `bp_enable_5v0_pullup`; empty no-ops on v3) |

The dispatcher's letter and symbol keys (digits select a mode; spaces and
commas are separators):

```
h ? i m b o v t T f g c C k K L l p P e = | ~ # $
a A @ W w d D & % s S O < ( " [ { ] } r
/ \ - _ . ^ !
```

## Capabilities

| Area | Where |
|---|---|
| ADC | `capability_adc/adc.c` — `bp_read_adc()`, `bp_adc_probe()`, `bp_adc_continuous_probe()`; ownership/refusal gate in `core_system/base.c` — `bp_adc_foreground_available()`, `bp_adc_check_foreground()` |
| AUX pin, PWM, servo, frequency | `capability_aux/aux_pin.c` — `bp_pwm_setup()`, `bp_servo_setup()`, `bp_frequency_counter_setup()`, `bp_update_pwm()`, `bp_update_duty_cycle()`, `bp_measure_frequency()`, `bp_aux_pin_set_low()`, `bp_aux_pin_set_high()`, `bp_aux_pin_read()`, `bp_aux_pin_state_get()`, `bp_aux_pin_set_change_notification()`, `bp_aux_pin_reset_state()` |
| Self-test | `capability_selftest/selftest.c` — `perform_selftest()`; `protocol_binary/binary_io.c` — `binary_io_self_test()`; ADC ownership gate in `core_system/base.c` — `bp_adc_foreground_available()`, `bp_adc_check_foreground()` |
| SMPS | `capability_smps/smps.c` — `smps_start()`, `smps_stop()`, `smps_adc()`, `_ADC1Interrupt()`; `smps_start()` calls `bp_aux_pin_reset_state()` (`capability_aux/aux_pin.c`); reached from `protocol_binary/binary_io.c` (`0xF0`–`0xFF`) |
| LCD (HD44780) adapter | `capability_hd44780/hd44780.c` — `LCDsetup()`, `LCDsetup_exc()`, `LCDwrite()`, `LCDstart()`, `LCDstop()` (register select), `LCDmacro()`, `LCDpins()`, `hd44780_cleanup()`, `HD44780_Reset()`, `HD44780_Init()`, `HD44780_WriteByte()`, `HD44780_WriteNibble()`, `HD44780_SPIwrite()`, local transfer helper `hd44780_spi_write_byte()`; selected by `BP_ENABLE_HD44780_SUPPORT`; registered in `core_system/core.c` (`"LCD"` entry, cleanup `hd44780_cleanup`, weak fallback `spi_disable_interface`) |

## Protocols

| Area | Where |
|---|---|
| 1-Wire | `protocol_wire/1wire.c` — `onewire_setup_prepare()`, `onewire_setup_execute()`, `onewire_reset()`, `perform_bus_reset()`, `onewire_run_macro()`, `perform_device_search()`, `lookup_device_model()`, `binary_io_enter_1wire_mode()` |
| 2-Wire | `protocol_wire/raw2wire.c` — `raw2wire_setup_prepare()`, `raw2wire_setup_execute()`, `raw2wire_start()`, `raw2wire_stop()`, `trigger_atr()`, `read_atr_header()` |
| 3-Wire | `protocol_wire/raw3wire.c` — `raw3wire_setup_prepare()`, `raw3wire_setup_execute()`, `raw3wire_start()`, `raw3wire_stop()`, `setup_raw3wire()` |
| Raw wire binary (`RAW1`) | `protocol_binary/binary_io.c` — `binary_io_raw_wire_mode_handler()`, `handle_wire_generic_command()`, `handle_bulk_byte_transfer()`, `handle_bulk_clock_ticks_advance()`, `handle_bulk_bit_transfer()`, `handle_configure_peripherals()`, `handle_set_pullup()`, `handle_set_speed()`, `handle_configuration()`, `handle_wait_command()` |
| SPI | `protocol_spi/spi.c` — `spi_setup()`, `spi_setup_prepare()`, `spi_setup_execute()`, `spi_start()`, `spi_stop()`, `spi_write()`, `spi_read()`, `spi_write_byte()`, `spi_run_macro()`, `spi_disable_interface()` |
| I2C | `protocol_i2c/i2c.c` — `i2c_setup_prepare()`, `i2c_setup_execute()`, `i2c_start()`, `i2c_stop()`, `i2c_write()`, `i2c_read()`, `i2c_macro()`, `hardware_i2c_abort()` |
| UART | `protocol_uart/uart.c` — `uart_setup_prepare()`, `uart_setup_execute()`, `uart_get_baud_rate()`, `uart_write()`, `uart_read()`, `uart_run_macro()`, `binUART()`; `protocol_uart/uart2.c` — `uart2_enable()`, `uart2_tx()`, `uart2_rx()`, `uart2_set_single_wire()`, `uart2_rx_pending()`, `uart2_rx_flags()`, `uart2_rx_overrun_count()` |
| DIO | `protocol_dio/dio.c` — `dio_setup_prepare()`, `dio_write()`, `dio_read()`, `dio_cleanup()` (strong hook; weak core fallback in `core_system/core.c`); shared pin helpers in `protocol_binary/binary_io.c` — `bitbang_pin_state_get()`, `bitbang_pin_direction_set()`, `bitbang_pin_state_set()` |
| Keyboard | `protocol_keyboard/pc_at_keyboard.c` — `pc_at_keyboard_prepare()`, `pc_at_keyboard_execute()`, `read_scancode()`, `read_byte()`, `read_bit()`, `write_bit()`, `write_byte()`, `keyboard_wait_clock_change()`, `handle_scancode()`, `pc_at_keyboard_run_macro()` |
| JTAG and OpenOCD | `protocol_jtag/jtag.c` — `jtag()` TAP engine, `jtag_setup()`, `jtag_reset()`, `jtag_set_state()`, `jtag_leave_state()`; `tool_openocd/openocd.c` — `binOpenOCD()`, `binOpenOCDPinMode()`, `binOpenOCDHandleFeature()` |
| XSVF player | `tool_xsvf/micro.c` — `xsvfRun()`, `xsvfDoXCOMMENT()`, `xsvfDoXSIR()`, `xsvfGotoTapState()`, `xsvfShiftOnly()`; `tool_xsvf/lenval.c` — `addVal()`, `readVal()`; `tool_xsvf/ports.c` — `xsvf_setup()`, `setPort()`, `readByte()`, `xsvf_wait_byte()`, `waitTime()` |
| Binary layer & SUMP entry | `protocol_binary/binary_io.c` — `enter_binary_bitbang_mode()`, `handle_bitbang_command()`, return `0x0F`; `protocol_sump/sump.c` — `enter_sump_mode()`, reset and exit bytes |

## Tools and programmers

| Area | Where |
|---|---|
| BASIC | `tool_basic/basic.c` — `interpreter()`, `assign()`, `handle_special_token()`, `get_token()`, `bp_basic_enter_interactive_interpreter()` |
| PIC (ICSP) | `programmer_pic/pic.c` — `pic_setup_prepare()`, `pic_setup_execute()`, `pic_write()`, `pic_read()`, `pic_start()` / `pic_stop()`, `pic_run_macro()`, `pic_cleanup()`, `pic_reset_mode_state()`, `pic_set_display_width()`, `binary_io_enter_pic_mode()`; `protocol_binary/binary_io.c` — raw-wire `0xAx` handlers (`handle_pic_command*()`, `pic614_*()`, `pic416_*()`, `pic424_*()`) |
| AVRISP-MKII | `programmer_avrisp/` — `avrisp_v2_process_command()` (`avrisp_v2.c`), `avrisp_usb_run()` / `avrisp_usb_configure_endpoints()` (`avrisp_mkii.c`), `avrisp_isp_transfer_byte()` (`avrisp_isp.c`), `avrisp_xprog_send_byte()`, `avrisp_xmega_enable_pdi()`, `avrisp_tiny_enable_tpi()` |
| Build version | `Makefile` — `VERSION` (git short hash baked into the banner) |

## Depth

| Page | Owns |
|---|---|
| [../core/system.md](../core/system.md) | boot/main flow, dispatcher, registry, shared state, reset paths |
| [../core/usb.md](../core/usb.md) | USB stack, control transfers, endpoint model, CDC data path |
| [../core/bitbang.md](../core/bitbang.md) | pin/clock primitives, profiles, stretch timeout |
| [../core/messages.md](../core/messages.md) | catalogs, program-memory reader, help, `agree()` |
| [../core/eeprom.md](../core/eeprom.md) | onboard EEPROM driver |
| [../core/README.md](../core/README.md) | layering and boundary rules for every group above |

Related: `docs/SPEC.md` (capability matrix),
[shared-machinery.md](shared-machinery.md) (shared helper contract),
[building.md](building.md) (build, feature sets, tests).
