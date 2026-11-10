# Core (firmware internals)

**Status:** maintainer-facing.  This set describes the firmware core: startup,
the terminal dispatcher, the mode registry, and the shared USB, bit-bang,
message and EEPROM modules.  It is not a user guide.  User-facing behaviour
(prompt grammar, commands, sessions) belongs to `docs/generic/` and the
per-function sets; this set explains how the firmware is put together.

Source and test paths are relative to `Firmware/`; tool/build and
documentation paths are relative to the repository root.  A page's Scope table
lists its own group's files by name (the group is named in the page title).
Maintainer pages may
cite files and symbols and carry **no line numbers** (they rot with every
edit); they carry no run logs or evidence narratives.

| File | Covers |
|---|---|
| [system.md](system.md) | boot/main flow, terminal dispatcher, command grammar, mode registry, HiZ, menus, shared state, serial abstraction, reset/exit paths |
| [usb.md](usb.md) | the v4 USB CDC device stack: descriptors, control transfers, endpoint/buffer model, CDC data path, AVRISP hook |
| [bitbang.md](bitbang.md) | the software GPIO/clock engine: profiles, pin semantics, waiting-I2C contract, stretch timeout |
| [messages.md](messages.md) | the packed string catalogs, the program-memory reader, help dispatch and `agree()` |
| [eeprom.md](eeprom.md) | the BPv4 onboard 24xx64 driver used by the self-test: bus model, read/write, ACK polling, fail-closed rollback |

## Orientation

The firmware is one cooperative foreground loop, with no scheduler, heap or
C++ runtime; mutable state lives in a few globals.  `main()` boots the board,
then `serviceuser()` loops forever: print the mode prompt, edit a command line,
dispatch it.  Boot, dispatcher and reset/exit paths are owned by
[system.md](system.md).

```
byte -> serviceuser() -> cmdbuf ring -> dispatcher (core_system/proc_menu.c)
     -> descriptor -> protocol_* / capability_* / tool_* callback
     -> binary or SUMP engine
```

## Layering and boundaries

`core_system` is the hub.  It owns the dispatcher, the mode registry and the
base helpers; every other module is reached from it, either directly or through
a mode descriptor.  The boundary is by ownership: protocols, capabilities,
tools and programmers call core helpers; core calls the capability helpers and
the binary/SUMP entries directly, while descriptors carry the mode callbacks.
**Core owns prompt-level gating and sequencing**
(refusing bus commands in HiZ, the pull-up/rail checks, the clutch order); the
capability modules own the hardware implementation and their own state.

| Layer | Owns | Depends on | Used by |
|---|---|---|---|
| `core_system/` | dispatcher, registry, parsing, formatting, delays, serial abstraction | descriptors and binary/SUMP entry points; capability helpers; message catalogs; USB CDC and onboard EEPROM (v4) | every other group (base/core helpers) |
| `core_usb/` | v4 USB CDC transport and device stack | `programmer_avrisp/` personality hook (when AVRISP is built) | `main.c` startup, `base.c` v4 serial wrappers |
| `core_bitbang/` | software pin/clock primitives and speed profiles | `core_system/` delay macros | `protocol_wire/` (2WIRE, 3WIRE), `protocol_i2c/` software engine |
| `core_messages/` | packed string catalogs and the table reader | `core_system/` serial writer | every module that prints |
| `core_eeprom/` | v4 onboard EEPROM driver | `core_system/` delay/timer helpers | `base.c` board reset, `capability_selftest/` |

Boundary rules that matter in practice:

- The mode-descriptor table is private to `core.c`; callers resolve the live
  mode through `bp_current_protocol()` and the menu through
  `bp_protocol_by_index()`.  Do not index the table directly.
- The dispatcher implements no bus timing.  Protocol I/O happens only inside
  descriptor callbacks or a binary/SUMP engine; the dispatcher drives the
  board-level elements (supplies, pull-ups, AUX, ADC, PWM/servo) through their
  capability helpers.
- Hardware capabilities keep their own state and accessors; core only sequences
  them (for example the AUX target selection and the power/pull-up checks).
- The v4 USB transport is the terminal for the whole firmware; protocol code
  treats it as an opaque serial port through `base.c`.
- Tools are not modes.  BASIC is entered from the dispatcher (`s`) and drives
  the current mode through the same callbacks; XSVF and OpenOCD are binary
  sub-modes reached from the JTAG binary entries.
- Programmer personalities differ: PIC is a registry mode, while AVRISP-MKII
  re-enumerates the USB device through the `core_usb` configuration hook.
- The onboard EEPROM self-test refuses to run while another owner has I2C1
  enabled; its only production callers are the board reset and the self-test.
  The interactive I2C mode keeps a separate onboard-EEPROM path.

## Where the other layers live

| Source group | Documentation sets |
|---|---|
| `protocol_*` | [wire](../protocol/wire/README.md) ([1wire](../protocol/wire/1wire/README.md), [2wire](../protocol/wire/2wire/README.md), [3wire](../protocol/wire/3wire/README.md)), [spi](../protocol/spi/README.md), [i2c](../protocol/i2c/README.md), [uart](../protocol/uart/README.md), [jtag](../protocol/jtag/README.md), [dio](../protocol/dio/README.md), [keyboard](../protocol/keyboard/README.md), [binary](../protocol/binary/README.md), [sump](../protocol/sump/README.md) |
| `capability_*` | [aux](../capabilities/auxiliary/README.md), [adc](../capabilities/adc/README.md), [selftest](../capabilities/selftest/README.md), [smps](../capabilities/smps/README.md), [lcd](../capabilities/lcd/README.md) |
| `tool_*` | [basic](../tools/basic/README.md), [xsvf](../tools/xsvf/README.md), [openocd](../tools/openocd/README.md) |
| `programmer_*` | [avrisp](../programmers/avrisp/README.md), [pic](../programmers/pic/README.md) |

## Module map

| Page | Source group | Key symbols |
|---|---|---|
| [system.md](system.md) | `core_system/` (`main.c`, `core.c/h`, `base.c/h`, `proc_menu.c/h`) | `main`, `initialize_board`, `serviceuser`, `changemode`, `bp_change_mode`, `getint`, `getrepeat`, `getnumbits`, `getnumber`, `getlong`, `cmdhistory`, `bus_pirate_protocol_t`, `bp_current_protocol`, `bp_protocol_by_index`, `bp_reset_board_state`, `bp_write_formatted_integer`, `bp_delay_ms`, `bp_delay_us`, `bp_timer_microseconds`, the `user_serial_*` layer |
| [usb.md](usb.md) | `core_usb/` (`cdc.c/h`, `usb_stack.c/h`, `descriptors.h`, `picusb.h`, `prj_usb_config.h`) | `initCDC`, `usb_init`, `usb_start`, `usb_handler`, `usb_handle_setup`, `user_configured_init`, `getinbuf_cdc`, `write_cdc`, `putc_cdc`, `getc_cdc`, `CDC_Flush_In_Now`, `CDCFlushOnTimeout`, `usb_device_state` |
| [bitbang.md](bitbang.md) | `core_bitbang/` (`bitbang.c/h`) | public: `bitbang_setup`, `bitbang_write_value`, `bitbang_read_value`, `bitbang_read_with_write`, `bitbang_i2c_start_wait`, `bitbang_i2c_stop_wait`, `bitbang_i2c_write_bit`, `bitbang_clock_stretch_timeouts`; static: `bitbang_wait_for_clock_release`, `BITBANG_DELAYS` |
| [messages.md](messages.md) | `core_messages/` (`messages.c/h`, `messages_v3/v4.h/.s`) | `bp_message_write_buffer`, `bp_message_write_line`, `print_help`, `agree`, `BPMSGxxxx`, `MSG_*`, `HLP10xx` |
| [eeprom.md](eeprom.md) | `core_eeprom/` (`onboard_eeprom.c/h`) | public: `eeprom_initialize`, `eeprom_test`; static: `eeprom_i2c_setup`, `eeprom_read_byte`, `eeprom_write_byte`, `eeprom_ready`, `eeprom_bus_clear` |

## Native test coverage

Six standalone host suites cover this group (`test_core_system.py`,
`test_usb.py`, `test_bitbang.py`, `test_messages.py`,
`test_message_catalog.py`, `test_core_eeprom.py`); run one with
`python Firmware/tests/<suite>` from the repository root.
`tests/test_native.py` runs only `test_protocol_access.py`,
`test_shared_format.py` and `test_messages.py`, so its pass does not cover the
other five.  Each module page states what its suite models.

## Limits and open items

- **v3 is established from source and native tests only.**  No physical v3
  verification is claimed for UART/timer behaviour.
- The USB stack is not exercised by a bus analyzer; control transfers, buffer
  ownership and reset interleavings are covered by source-level tests.  See
  [usb.md](usb.md).
- The EEPROM driver is not a settings store, and the interactive I2C mode and
  the core driver remain two implementations of onboard-EEPROM access.
  Consolidating them is an architectural follow-up.  See [eeprom.md](eeprom.md).

## Related docs

- [../generic/README.md](../generic/README.md) — using the prompt.
- [../SPEC.md](../SPEC.md) — function and capability index.
- [../dev/building.md](../dev/building.md),
  [../dev/bootloaders.md](../dev/bootloaders.md),
  [../dev/shared-machinery.md](../dev/shared-machinery.md),
  [../dev/code-map.md](../dev/code-map.md) — build/tests, the `$` handoff,
  shared helpers, maintainer code map.
