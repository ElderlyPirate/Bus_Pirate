# Core firmware internals

**Status:** current · maintainer-facing.  This set describes the firmware core:
startup, the terminal dispatcher, the mode registry and the shared USB,
bit-bang, message and EEPROM modules.  It is not a user guide.  Prompt grammar,
commands and sessions belong to `docs/generic/` and the per-function sets.

This README is a map, not the manual.  The details live in the files below.
New to the core: read `system.md` first, then the module you are changing;
each page ends with its suite and related pages.

Source and test paths are relative to `Firmware/`; tool, build and
documentation paths are relative to the repository root.  Each page's Scope
section names its group files.  Maintainer pages cite files and symbols but
carry **no line numbers**.

## Maintainer index

| If you need to... | Start at |
|---|---|
| add or renumber a mode | [system.md §Adding a mode](system.md#adding-a-mode) |
| add or change a dispatcher key | [system.md §Command dispatcher](system.md#command-dispatcher) |
| change the board-reset order | [system.md §Board-state reset](system.md#board-state-reset-bp_reset_board_state) |
| add or regenerate a string | [messages.md §Generation and catalog contracts](messages.md#generation-and-catalog-contracts) |
| find a missing message identifier | [messages.md §Failure modes](messages.md#failure-modes-and-edge-cases) |
| diagnose a hung terminal | [usb.md §Failure modes](usb.md#failure-modes-and-edge-cases) |
| diagnose a mode that will not engage | [system.md §Failure modes](system.md#failure-modes-and-edge-cases) |
| diagnose an I2C clock timeout | [bitbang.md §Failure modes](bitbang.md#failure-modes-and-edge-cases) |
| diagnose an EEPROM self-test failure | [eeprom.md §Failure modes](eeprom.md#failure-modes-and-edge-cases) |

## The five-minute version

- One cooperative foreground loop: `main()` boots the board, `serviceuser()`
  edits one line, then the dispatcher calls the current mode's descriptor
  callback.  No scheduler, heap or C++ runtime; the core interrupt handlers
  cover USB (v4), Timer1 and the v3 UART.
- `core_system` is the hub; every interactive mode is one
  `bus_pirate_protocol_t` descriptor in a private table whose `name` drives the
  prompt, menu and banner.  The v4 terminal is the USB CDC stack; v3 is a
  hardware UART with an output ring behind the same `user_serial_*` calls.
- Most user-visible text comes from generated packed catalogs.  Edit
  `tools/packstrings/*.txt` and regenerate; never patch one `.s`/`.h` alone.

```
byte -> serviceuser() -> cmdbuf ring -> dispatcher
     -> mode descriptor -> protocol_* / capability_* / tool_* callback
     -> binary or SUMP engine
```

## Layering and boundaries

| Layer | Owns | Depends on | Used by |
|---|---|---|---|
| `core_system/` | dispatcher, registry, parsing, formatting, delays, serial abstraction, persistent flags | descriptors and binary/SUMP entries; capability helpers; message catalogs; USB CDC and onboard EEPROM (v4) | every other group |
| `core_usb/` | v4 USB CDC transport and device stack | `programmer_avrisp/` personality hook (when AVRISP is built) | `main.c` startup, `base.c` v4 wrappers |
| `core_bitbang/` | software pin/clock primitives and speed profiles | `core_system/` delay helpers | 2WIRE, 3WIRE, software I2C, binary raw entry, PIC |
| `core_messages/` | packed string catalogs and the table reader | `core_system/` serial writer | every module that prints catalog text |
| `core_eeprom/` | v4 onboard EEPROM driver | `core_system/` delay/timer helpers | `base.c` board reset, `capability_selftest/` |

- The mode-descriptor table is private to `core.c`; use
  `bp_current_protocol()` and `bp_protocol_by_index()`, never the table.
- The dispatcher implements no bus timing.  Protocol I/O happens only inside
  descriptor callbacks or a binary/SUMP engine.  Core owns prompt-level gating
  and sequencing and calls capability helpers and the binary/SUMP entries
  directly; descriptors carry the mode callbacks, and the capability modules
  own the hardware and their state.
- The v4 USB transport is the terminal for the whole firmware; protocol code
  treats it as an opaque serial port through `base.c`.  Tools are not modes:
  BASIC is entered by `s`; XSVF and OpenOCD are binary sub-modes.
- PIC is a registry mode; AVRISP is a USB personality.

## Module map

| Group | Source files | Detail |
|---|---|---|
| `core_system/` | `main.c`, `core.c/h`, `base.c/h`, `proc_menu.c/h` | [system.md](system.md) — boot, dispatcher, registry, resets |
| `core_usb/` | `cdc.c/h`, `usb_stack.c/h`, `picusb.h`, `descriptors.h`, `prj_usb_config.h`, `usb_stack_globals.h` | [usb.md](usb.md) — v4 CDC stack, control transfers, data path |
| `core_bitbang/` | `bitbang.c/h` | [bitbang.md](bitbang.md) — GPIO/clock primitives, profiles, I2C |
| `core_messages/` | `messages.c/h`, `messages_v3.s/h`, `messages_v4.s/h` | [messages.md](messages.md) — catalogs, reader, help, `agree()` |
| `core_eeprom/` | `onboard_eeprom.c/h` | [eeprom.md](eeprom.md) — v4 24xx64 driver, ACK poll, rollback |

Each detail page names that group's public entry points in its Scope/API
sections.  `eeprom_i2c_setup`, `eeprom_read_byte`, `eeprom_write_byte`,
`eeprom_ready` and `eeprom_bus_clear` are static; the full cross-group symbol
index is [the maintainer code map](../dev/code-map.md).

## Naming and availability

The firmware's displayed strings are the name authority; [system.md](system.md)
lists the registry strings and owns the one `1WIRE` / `1-Wire` /
`1-WIRE speed:` mapping.

| | v3 | v4 |
|---|---|---|
| firmware build | yes | yes |
| terminal transport | hardware UART1 through the FTDI link | USB CDC (`core_usb`) |
| onboard EEPROM driver | not compiled | compiled |
| documentation basis | build-only; **not verified on v3 hardware** | primary target |

## Board and wiring headline

Core adds no target wiring and no pull-ups.  v4 is USB bus-powered; the
waiting-I2C contract needs open-drain outputs plus external pull-ups on SDA/SCL
(see [i2c wiring](../protocol/i2c/wiring.md)); the onboard EEPROM is an internal
board device on I2C1.  Mode wiring lives in each user set.

## Native test coverage

Each page names its standalone host suite; the only runner is that per-suite
script, run from the repository root.  There is no aggregate runner and no
`test_native.py` in this tree.  The authoritative table with coverage and
compiler needs is [dev/tooling.md](../dev/tooling.md#test-suites); these are
host models, not a PIC24, SIE or electrical simulator.

## Where the other layers live

| Source group | User-facing sets |
|---|---|
| `protocol_*` wire | [index](../protocol/wire/README.md), [1wire](../protocol/wire/1wire/README.md), [2wire](../protocol/wire/2wire/README.md), [3wire](../protocol/wire/3wire/README.md) |
| `protocol_*` bus | [spi](../protocol/spi/README.md), [i2c](../protocol/i2c/README.md), [uart](../protocol/uart/README.md), [jtag](../protocol/jtag/README.md) |
| `protocol_*` bit/stream | [dio](../protocol/dio/README.md), [keyboard](../protocol/keyboard/README.md), [binary](../protocol/binary/README.md), [sump](../protocol/sump/README.md) |
| `capability_*` front ends | [auxiliary](../capabilities/auxiliary/README.md), [adc](../capabilities/adc/README.md) |
| `capability_*` test/power | [selftest](../capabilities/selftest/README.md), [smps](../capabilities/smps/README.md), [lcd](../capabilities/lcd/README.md) |
| `tool_*` | [basic](../tools/basic/README.md), [xsvf](../tools/xsvf/README.md), [openocd](../tools/openocd/README.md) |
| `programmer_*` | [avrisp](../programmers/avrisp/README.md), [pic](../programmers/pic/README.md) |
| base prompt layer | [generic](../generic/README.md) |

The cross-group symbol index and the return path to the code are
[dev/code-map.md](../dev/code-map.md).

## Limits and open items

- **v3 is build-only here; not verified on v3 hardware.**  v3 timing, UART and
  reset claims are source-derived.
- **No user settings persistence.**  The configuration structures are RAM and
  reset with the board; only the bootloader signature and `$` handshake byte
  persist.  The onboard EEPROM is not a settings store and has two access
  paths: this driver and the interactive I2C mode's macros.
- **USB is modelled, not bus-analysed.**  The host suite is not an SIE model;
  physical unplug and timing races are outside it.

## Related docs

- [../generic/README.md](../generic/README.md) (including
  [Idle mode (HiZ)](../generic/README.md#idle-mode-hiz)),
  [../SPEC.md](../SPEC.md), [../dev/building.md](../dev/building.md),
  [../dev/bootloaders.md](../dev/bootloaders.md),
  [../dev/shared-machinery.md](../dev/shared-machinery.md),
  [../dev/code-map.md](../dev/code-map.md),
  [../dev/verification.md](../dev/verification.md).
