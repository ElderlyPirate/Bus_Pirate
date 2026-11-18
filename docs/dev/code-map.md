# Code map — cross-group index (maintainer)

**Status:** maintainer-facing.  This is the single cross-group symbol index for
the grouped firmware: one row per source group, two to four entry symbols, and
a pointer to the page that owns the detail.  It is not a user-documentation
companion — user pages carry no code references by policy; this page and
`docs/core/` are where the code lives.

Paths are relative to `Firmware/`.  Symbols only — **no line numbers**
(they rot with any edit).  A row is a starting point, not an inventory:
follow the group into `docs/core/` (core internals) or the matching function
set before changing behaviour.

## Core

| Area | Where |
|---|---|
| Terminal dispatcher | `core_system/proc_menu.c` — `serviceuser()` and the per-key `case` labels |
| Command-line editing | `core_system/proc_menu.c` — `handle_character()`, `handle_backspace()`, `handle_delete()`, `handle_escape_key()`, `handle_up_arrow()` / `handle_down_arrow()`, `load_user_macro()`, `update_binary_prefix()` |
| Value entry & parsing | `core_system/proc_menu.c` — `getint()`, `getint_with_spaces()`, `consumewhitechars()`, `getrepeat()`, `getnumbits()`, `getnumber()`, `getlong()`, `change_read_display()` |
| Mode switch & status | `core_system/proc_menu.c` — `changemode()`, `bp_change_mode()`, `set_display_mode()`, `set_baud_rate()`, `set_pullup_voltage()`, `convert_value()`, `switch_psu_on()` / `switch_psu_off()`, `print_version_info()`, `print_status_info()`, `print_pins_information()` |
| Numeric formatting & delays | `core_system/base.c` — `bp_write_formatted_integer()`, `bp_write_dec_word()` / `bp_write_dec_byte()`, `bp_write_dec_dword()`, `bp_write_voltage()`, `bp_delay_ms()` / `bp_delay_us()`, `bp_timer_microseconds()`, `bp_reset_board_state()`, the `user_serial_*` layer |
| Mode registry & callbacks | `core_system/core.c` — protocol table, `bp_current_protocol()`, `bp_protocol_by_index()`, null-callback defaults |
| USB CDC | `core_usb/cdc.c` — `initCDC()`, `getc_cdc()`, `putc_cdc()`, `CDC_Flush_In_Now()`, `getinbuf_cdc()` / `write_cdc()`; `core_usb/usb_stack.c` — `usb_init()`, `usb_handler()`, `usb_handle_setup()` |
| Bitbang engine | `core_bitbang/bitbang.c` — `bitbang_setup()`, `bitbang_write_value()`, `bitbang_read_value()`, waiting-I2C helpers |
| Onboard EEPROM | `core_eeprom/onboard_eeprom.c` — `eeprom_initialize()`, `eeprom_test()` |
| Strings & help | `core_messages/messages.c` — `bp_message_write_buffer()` / `_line()`, `print_help()`, `agree()`; `core_messages/messages_v4.s` / `.h`, `messages_v3.s` / `.h` |
| Limits & feature sets | `configuration.h` — `BUSPIRATEV3` / `BUSPIRATEV4` / `BP_CUSTOM_FEATURE_SET` blocks, `BP_ENABLE_*` switches, history/macro/buffer sizes; `core_system/main.c` — startup defaults |
| Board strings & VPU helpers | `core_system/base.h` — `BP_FIRMWARE_STRING`; `hardwarev4.h` / `hardwarev3.h` — `BP_VERSION_STRING`, VPU source helpers (`bp_enable_3v3_pullup` / `bp_enable_5v0_pullup`; empty no-ops on v3) |

The single-byte keys the dispatcher handles are:

```
h ? i m b o v t T f g c C k K L l p P e = | ~ # $
a A @ W w d D & % s S O < ( " [ { ] } r
```

## Capabilities

| Area | Where |
|---|---|
| ADC probe | `capability_adc/adc.c` — `bp_read_adc()`, `bp_adc_probe()`, `bp_adc_continuous_probe()` |
| AUX pin, PWM, servo, frequency | `capability_aux/aux_pin.c` — `bp_pwm_setup()`, `bp_servo_setup()`, `bp_frequency_counter_setup()`, `bp_aux_pin_set_low()` / `_high()`, `bp_aux_pin_read()` |
| Self-test | `capability_selftest/selftest.c` — `perform_selftest()` |
| SMPS | `capability_smps/smps.c` — `smps_start()`, `smps_stop()`, `smps_adc()`, `_ADC1Interrupt()`; reached from `protocol_binary/binary_io.c` (`0xF0`–`0xFF`) |
| LCD (HD44780) adapter | `capability_hd44780/hd44780.c` — `LCDsetup()`, `LCDsetup_exc()`, `LCDwrite()`, `LCDstart()` / `LCDstop()` (register select), `LCDmacro()`, `LCDpins()`, `HD44780_Reset()` / `_Init()` / `_WriteByte()` / `_WriteNibble()` / `_SPIwrite()`; selected by `BP_ENABLE_HD44780_SUPPORT`; registered in `core_system/core.c` (`"LCD"` entry, cleanup `spi_disable_interface`) |

## Protocols

| Area | Where |
|---|---|
| 1-Wire | `protocol_wire/1wire.c` — `onewire_setup_prepare()` / `_execute()`, `onewire_reset()`, `perform_bus_reset()`, `onewire_run_macro()`, `perform_device_search()`, `lookup_device_model()`, `binary_io_enter_1wire_mode()` |
| 2-Wire | `protocol_wire/raw2wire.c` — `raw2wire_setup_prepare()` / `_execute()`, `raw2wire_start()` / `_stop()`, `trigger_atr()`, `read_atr_header()` |
| 3-Wire | `protocol_wire/raw3wire.c` — `raw3wire_setup_prepare()` / `_execute()`, `raw3wire_start()` / `_stop()`, `setup_raw3wire()` |
| Raw wire binary (`RAW1`) | `protocol_binary/binary_io.c` — `binary_io_raw_wire_mode_handler()`, `handle_wire_generic_command()`, `handle_bulk_byte_transfer()`, `handle_bulk_clock_ticks_advance()`, `handle_bulk_bit_transfer()`, `handle_configure_peripherals()`, `handle_set_pullup()`, `handle_set_speed()`, `handle_configuration()`, `handle_wait_command()` |
| SPI | `protocol_spi/spi.c` — `spi_setup()`, `spi_setup_prepare()` / `_execute()`, `spi_start()` / `_stop()`, `spi_write()` / `spi_read()`, `spi_run_macro()`, `spi_disable_interface()` |
| I2C | `protocol_i2c/i2c.c` — `i2c_setup_prepare()` / `_execute()`, `i2c_start()` / `i2c_stop()`, `i2c_write()` / `i2c_read()`, `i2c_macro()`, `hardware_i2c_abort()` |
| UART | `protocol_uart/uart.c` — `uart_setup_prepare()` / `_execute()`, `uart_get_baud_rate()`, `uart_write()` / `uart_read()`, `uart_run_macro()`, `binUART()`; `protocol_uart/uart2.c` — `uart2_enable()`, `uart2_tx()` / `uart2_rx()`, `uart2_rx_flags()` |
| DIO | `protocol_dio/dio.c` — `dio_setup_prepare()`, `dio_write()`, `dio_read()` |
| Keyboard | `protocol_keyboard/pc_at_keyboard.c` — `pc_at_keyboard_prepare()` / `_execute()`, `read_scancode()`, `write_byte()`, `handle_scancode()` |
| JTAG and OpenOCD | `protocol_jtag/jtag.c` — `jtag()` TAP engine, `jtag_setup()`, state helpers; `tool_openocd/openocd.c` — `binOpenOCD()` adapter |
| XSVF player | `tool_xsvf/micro.c` — `xsvfDoXSIR()`, `xsvfGotoTapState()`, `xsvfShiftOnly()`; `tool_xsvf/lenval.c` — `addVal()`, `readVal()`; `tool_xsvf/ports.c` — `xsvf_setup()`, `setPort()` |
| Binary layer & SUMP entry | `protocol_binary/binary_io.c` — `enter_binary_bitbang_mode()`, `handle_bitbang_command()`, `bitbang_pin_state_get()`, return `0x0F`; `protocol_sump/sump.c` — `enter_sump_mode()`, reset and exit bytes |

## Tools and programmers

| Area | Where |
|---|---|
| BASIC | `tool_basic/basic.c` — `interpreter()`, `assign()`, `handle_special_token()`, `get_token()`, `bp_basic_enter_interactive_interpreter()` |
| PIC (ICSP) | `programmer_pic/pic.c` — `pic_setup_prepare()`, `pic_setup_execute()`, `pic_write()`, `pic_read()`, `pic_start()` / `pic_stop()`, `pic_run_macro()`, `binary_io_enter_pic_mode()`; `protocol_binary/binary_io.c` — raw-wire `0xAx` handlers (`handle_pic_command*()`, `pic614_*()`, `pic416_*()`, `pic424_*()`) |
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
