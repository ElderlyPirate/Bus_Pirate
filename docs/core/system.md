# System (`core_system/`)

**Status:** current · maintainer-facing.  Covers startup, the terminal
dispatcher, the mode registry, shared state, menus and the reset/exit paths.
The user-facing prompt grammar and command reference live in `docs/generic/`;
this page explains the implementation behind them.

Source and test paths on this page are relative to `Firmware/`; tool, build and
documentation paths are relative to the repository root.  The Scope table
lists this page's group files by name.  v3 is build-only: v3-specific paths
here are source-derived and **not verified on v3 hardware**.

## Contents

- **Startup and state:** [Boot and main flow](#boot-and-main-flow),
  [Terminal state and data model](#terminal-state-and-data-model),
  [Persistent state and configuration](#persistent-state-and-configuration)
- **Runtime:** [Base helpers](#base-helpers),
  [Serial abstraction](#serial-abstraction),
  [`serviceuser()` lifecycle](#serviceuser-lifecycle),
  [Line editor](#line-editor), [Command dispatcher](#command-dispatcher),
  [Parsing helpers](#parsing-helpers)
- **Modes:** [Mode registry](#mode-registry),
  [HiZ (the parked mode)](#hiz-the-parked-mode),
  [Mode-change lifecycle](#mode-change-lifecycle),
  [Adding a mode](#adding-a-mode), [Menus](#menus)
- **Recovery:** [Terminal reset and exit paths](#terminal-reset-and-exit-paths),
  [Failure modes and edge cases](#failure-modes-and-edge-cases),
  [Invariants and contracts](#invariants-and-contracts),
  [Native test coverage](#native-test-coverage)

## Scope and files

| File | Responsibility |
|---|---|
| `main.c` | reset entry, persistent firmware signature, board bring-up, v4 USB wait, USB interrupt vector, `usb_suspend()` |
| `core.c` / `core.h` | mode-descriptor type, protocol enumeration, private registry table, live/index accessors, default callbacks, display-mode enum, mode-configuration type |
| `base.c` / `base.h` | board reset, serial abstraction, formatting, voltage, flash reads, delay/timer API, ADC ownership guard, v3 UART ring and ISRs |
| `proc_menu.c` / `proc_menu.h` | `serviceuser()` loop, line editor, history, user macros, parsing helpers, mode/setup menus, status/pin printouts |

`core_system` contains no protocol engine.  It sequences protocol callbacks and
provides the helpers every mode prints and parses with.  Build-time feature
selection lives in `configuration.h`; board pin maps live in `hardwarev3.h` and
`hardwarev4.h`.

## Boot and main flow

1. `main()` writes `firmware_signature` (constant `0x31415926`) to its
   persistent location (`0x47FC` on v4, `0x27FC` on v3).  At reset the
   bootloader compares this value and the persistent `skip_pgc_pgd_check` flag
   to choose between the bootloader and the application.
2. `initialize_board()`:
   - configures the clock/PLL and analog pins (v3: clock divider, all pins
     digital; v4: nested interrupts off, PLL prescaler, PSV, analog pin masks),
   - disables the secondary oscillator, waits, and starts the delay timer,
   - sets the v3 UART PPS mapping and the defaults `terminal_speed = 8`
     (115200) and `display_mode = HEX`,
   - calls `bp_reset_board_state()` to park the board,
   - starts the transport: v3 `user_serial_initialise()`, or v4 `initCDC()`,
     `usb_init()` and `usb_start()`,
   - reads the device type and revision from flash, sets `quiet = OFF` and
     `mode_configuration.numbits = 8`, initialises BASIC when built, and clears
     `TBLPAG`; on v3 it prints the banner at the end.
3. On v4, `main()` enables the USB LED, enables the USB interrupt sources
   (`USB_TRN | USB_SOF | USB_UERR | USB_URST`), blocks until
   `usb_device_state >= CONFIGURED_STATE`, registers `CDCFlushOnTimeout` as the
   SOF handler, disables the USB LED, and enters `serviceuser()`.  On v3 the
   LED helpers are no-ops and `main()` enters the loop directly.
4. `serviceuser()` never returns.  `main()` ends in that call.
5. The interrupt contexts in this group are `_USB1Interrupt` (calls
   `usb_handler()` and clears the USB1 flag), Timer1's overflow ISR (delay and
   timestamp counter) and, on v3, the UART1 RX/TX ISRs.

On v4 `usb_start()` waits indefinitely for the bus to leave SE0 before
`DEFAULT_STATE`; the AVRISP personality uses its own bounded attach for the
same wait.  The core startup path has no timeout.

## Terminal state and data model

| Object | Where | Meaning |
|---|---|---|
| `bp_buffer` | `main.c` | `BP_TERMINAL_BUFFER_SIZE` bytes, 2-byte aligned; the terminal/sample buffer (the v3 serial ring uses it) |
| `bus_pirate_configuration` | `main.c` | board/UI state: `terminal_input`, device type/revision, hardware version, terminal speed, display mode, `bus_mode`, `quiet`, `overflow`, `basic` |
| `mode_configuration` | `main.c` | per-mode state: `speed`, `numbits`, `alternate_aux` (2 bits), `periodicService`, `little_endian`, `high_impedance`, `int16`, `write_with_read`, `command_error` |
| `cmdbuf`, `cmdstart`, `cmdend` | `proc_menu.c` | command ring (`BP_COMMAND_BUFFER_SIZE`, mask `CMDLENMSK`); `cmdend` is the insertion point, `cmdstart` the parse cursor |
| `menu_state` | `proc_menu.c` | cursor position, history counter, binary-entry counter, escape state, `command_present` |
| `user_macros`, `user_macro` | `proc_menu.c` | `BP_USER_MACROS_COUNT` slots of `BP_USER_MACRO_MAX_LENGTH` bytes; `user_macro` is a pending recall |
| `setup_defaults_mode` | `proc_menu.c` | when set, setup prompts answer defaults without reading or printing (scripted mode change) |

Size effects (from `configuration.h`):

| Setting | v3 | v4 |
|---|---|---|
| command buffer | 128 B | 256 B |
| terminal buffer | 4096 B | 4096 B |
| command history | 7 entries | 15 entries |
| user macros | 3 slots × 24 B | 5 slots × 32 B |

`clear_mode_configuration()` (called from `bp_reset_board_state()`) sets
`numbits = 8`, clears `int16`, `write_with_read`, `command_error`,
`high_impedance`, `speed`, `periodicService`, `alternate_aux` and
`little_endian`.  A protocol's setup may override these afterwards.

## Persistent state and configuration

Runtime settings are **not** persisted.  The two configuration structures and
the command/macro buffers are RAM and are rebuilt or cleared on every board
reset.  The only persistent values written by core are the bootloader
handshake:

| Object | Location | Contract |
|---|---|---|
| `firmware_signature` | `0x47FC` (v4), `0x27FC` (v3) | 32-bit persistent word, written to `0x31415926` on every boot; the loader treats it as the "application has run" marker |
| `skip_pgc_pgd_check` | `0x47FA` (v4), `0x27FA` (v3) | persistent byte set by `$`; with a matching signature it forces the bootloader path, and the loader clears it |

`bus_pirate_configuration.device_type` and `.device_revision` are read from
the device ID/revision words in the config-memory region (`0x00FF0000` and
`0x00FF0002`), not from the user `CFG0`..`CFG2` words that the status block
prints.  `hardware_version` comes from the v3 port strap; it is `0` on v4.
The onboard EEPROM in `core_eeprom` is opaque storage used by the self-test,
not a settings store: no versions, slots, CRCs or migration logic are written
by core.

Feature selection is build-time only: `configuration.h` defines the
`BP_ENABLE_*` set per board, and the size/history/macro limits are compile-time
constants.  A custom build must carry its own feature set; there is no runtime
configuration file.

## Base helpers

| Helper group | Contract |
|---|---|
| formatted integers | `bp_write_formatted_integer()` masks the value to `mode_configuration.numbits` (1..16), then prints in the selected `display_mode`; `int16` selects byte/word width, and RAW emits the high byte before the low byte for 16-bit values |
| decimal output | `bp_write_dec_byte()` / `bp_write_dec_word()` cover 8/16-bit values; `bp_write_dec_dword()` covers all of `uint32_t` with ten digits; `bp_write_dec_dword_friendly()` adds thousands separators |
| hex/binary output | `bp_write_hex_byte()` / `bp_write_hex_word()` print the `0x` prefix; `bp_write_bin_byte()` prints `0b` plus eight bits |
| voltage | `bp_write_voltage()` converts a 10-bit ADC reading through the board divider and prints centivolts as `V.cc`; callers add the unit (`V`) |
| delays and timestamp | Timer1 free-runs at two ticks per microsecond; `bp_timer_microseconds()` returns a 31-bit microsecond count (the final right shift drops the half-microsecond tick, leaving a 31-bit count) that wraps every 35.8 minutes; `bp_delay_us()` uses the short path below `0x3FFF` and the long path otherwise; `bp_delay_ms()` multiplies in 32 bits |
| flash reads | `bp_read_from_flash(page, address)` returns the low 16 bits of the program word and restores `TBLPAG` |
| bit order | `bp_reverse_integer(value, bits)` reverses within the requested width; v4 uses a 256-byte table, v3 uses mask/shift stages |
| bulk output | `bp_write_buffer()` uses the CDC span/lock API on v4 and per-byte writes on v3 |
| ADC ownership | `bp_adc_check_foreground()` refuses foreground ADC/VPU/PSU work while an SMPS feedback owner holds `AD1IE`; it prints the busy line and sets `command_error` |

`delay_short_inline()` is the inlined hot-path version used by bit-bang; it
accepts 0..16384 microseconds.  Unlike a generic timer delay, it never routes
to a long path.

## Serial abstraction

`base.c` presents one byte-stream interface; `main.c`, the dispatcher, the
protocols and the binary/SUMP engines all use it.  No module writes the v4 CDC
registers; on v3 the UART bridge/bulk paths and the OpenOCD fast path do touch
U1 directly.  v3 is a hardware UART with a terminal output ring; v4 is the USB
CDC stack in `core_usb`.

| Call | v3 (`base.c`) | v4 (`base.c` → `cdc.c`) |
|---|---|---|
| `user_serial_initialise()` | programs UART1 (`U1MODE`/`U1STA`, baud from `terminal_speed`) | no-op (the CDC stack is initialised by `initCDC()`/`usb_init()`) |
| `user_serial_set_baud_rate(rate)` | writes `U1BRG` | no-op; CDC line coding is host metadata |
| `user_serial_ready_to_read()` | `U1STAbits.URXDA`, non-blocking | `cdc_Out_len` or `getOutReady()`, non-blocking (recycles ZLPs) |
| `user_serial_read_byte()` | **blocks** until `URXDA`, returns `U1RXREG` | **blocks** in `getc_cdc()` until a packet arrives |
| `user_serial_transmit_character(c)` | `quiet` suppresses; else **blocks** while `UTXBF`, writes `U1TXREG` | `quiet` suppresses; else `putc_cdc(c)` buffers, then **blocks** in `WaitInReady()` when a 64-byte packet fills and the IN BD is not yet free |
| `user_serial_wait_transmission_done()` | **blocks** until `TRMT` | **blocks** in `WaitInReady()` for the IN BD (only while configured) |
| `user_serial_transmit_done()` | `TRMT` | always `YES` |
| `user_serial_check_overflow()` / `clear_overflow()` | `OERR` flag / clear it | always `NO` / no-op |
| `user_serial_ringbuffer_append(c)` | appends to the output ring in `bp_buffer`, sets `overflow` and clears the mode LED when full | `putc_cdc(c)` |
| `user_serial_ringbuffer_process()` / `flush()` | drains one byte / drains fully to `U1TXREG` | no-op / `CDC_Flush_In_Now()` |
| `user_serial_ringbuffer_setup()` | resets the ring indices and `overflow` | no-op |
| `user_serial_read_big_endian_word()` / `_long_word()` | two / four `user_serial_read_byte()` calls | same |

Contract notes:

- **Reads block.**  A caller that must not stall polls `ready_to_read()` first,
  as `serviceuser()` and the SUMP loop do.  There is no timeout.
- **`quiet` is output-only.**  Both builds drop characters inside
  `user_serial_transmit_character()`, so formatting and bus work still happen;
  `reset_terminal_state()` clears it.
- **Overflow is a v3 concept.**  Only the v3 UART exposes `OERR` and the ring's
  full flag.  v4 has no application-visible overflow: the USB stack keeps
  ownership of a busy OUT buffer and the host retries.
- **The v3 flush has no escape.**  `user_serial_ringbuffer_flush()` loops until
  the ring is empty and waits on `UTXBF`; a UART that never drains spins there.
- **Both builds share the word/long readers**, so protocol code can read
  multi-byte values without knowing the transport.

## `serviceuser()` lifecycle

The outer loop is: print `<mode>`, optionally `(BASIC)`, then `>`, and collect
one command.

1. **Input wait.**  While no byte is ready, a mode with
   `mode_configuration.periodicService` set has `periodic_update()` polled.  If
   it reports output, the current prompt and the partially typed command are
   reprinted so asynchronous mode output does not swallow the line.
2. **Overflow.**  A v3 UART receive overflow is cleared and the loop restarts.
3. **Byte handling.**  The byte is offered to the escape-sequence state machine
   (if one is in progress), then to the input key map.  Printable bytes go to
   the line editor; the dispatcher parses the line only after Enter.
4. **Enter.**  `handle_enter_key()` sets `command_present`, NUL-terminates the
   line, advances `cmdend` and prints CRLF.
5. **Execution.**  The dispatcher walks from `cmdstart` to `cmdend`.  Each key
   is a case in the switch in `serviceuser()`; the cursor advances after each
   case.  A handler may consume extra characters (numbers, repeats, widths).
6. **Error.**  `mode_configuration.command_error` prints
   `Syntax error at char <n>` (one-based distance from the command start) and
   ends the command.  The next loop iteration starts a fresh command.
7. **BASIC.**  If `bus_pirate_configuration.basic` is set when a line is
   entered, the dispatcher calls `bp_basic_enter_interactive_interpreter()`
   instead of running that line, prints `Ready`, and returns to the prompt.
   Every typed line is intercepted before the command switch, so ordinary
   prompt keys (`#`, `$`, `m`, `w`, …) are not dispatched while BASIC is
   active; `EXIT`, handled inside the interpreter, clears the flag.  The prompt
   is the mode name plus `(BASIC)`: `HiZ(BASIC)>`, `SPI(BASIC)>`.  `s` changes
   the prompt immediately and the interpreter handles the next Enter.  User
   detail: [BASIC entry and prompt](../tools/basic/entry-and-prompt.md).

The prompt string is the mode descriptor's `name`, so `HiZ>` is simply the HiZ
descriptor.  The same names drive the mode menu and the bracketed mode list in
the version banner.

## Line editor

| Input | Effect |
|---|---|
| printable `0x20`..`0x7E` | insert at cursor; append at end otherwise |
| Backspace | delete left of cursor; bell at line start |
| Delete / EOT (`0x7F` / `0x04`) | delete at cursor; bell at end |
| Left / right arrow, Home, End | move the cursor (ANSI `ESC [` / `ESC O` sequences are decoded, not forwarded) |
| raw control aliases | `0x02` left, `0x06` right, `0x10` up, `0x0E` down, `0x01` home, `0x05` end |
| Up / down arrow | recall older/newer history entry |
| Escape | starts escape-sequence decoding |
| Enter (CR / LF) | finish the line |

The buffer is a ring; all indices are masked with `CMDLENMSK`.  Insertion and
removal shift the ring in place and echo the moved text with ANSI cursor
commands.  Empty slots and the active command are not listed as history
entries.  A full ring refuses a printable byte with a bell rather than
overwriting unread history.

## Command dispatcher

These are the keys handled after Enter.  Protocol-level semantics belong to the
mode page; the table records what the dispatcher does and which helper it
calls.

| Key | Handler / call | Notes |
|---|---|---|
| `h` | `cmdhistory()` | lists prior commands from the ring, then recalls a selected one; build option `BP_ENABLE_COMMAND_HISTORY` |
| `?` | `print_help()` | prints the fixed `HLP1000`..`HLP1022` block |
| `i` | `print_version_info()`, `print_status_info()` | status block only outside HiZ |
| `m` | `changemode()` | mode menu when no argument; `m<n>` switches directly |
| `b` | `set_baud_rate()` | v3 terminal baud; v4 prints that CDC baud is host-controlled |
| `o` | `set_display_mode()` | HEX / DEC / BIN / RAW |
| `v` | `print_pins_information()` | pin directions/states, rail voltages, mode pin table |
| `t` / `T` | `bp_timer_print()` / `bp_timer_reset()` | 31-bit microsecond timestamp |
| `f` | `bp_frequency_counter_setup()` | frequency counter on AUX |
| `g` | `bp_pwm_setup()` | refused in HiZ |
| `S` | `bp_servo_setup()` | refused in HiZ |
| `c` / `C` | `alternate_aux = 0` / `1` | AUX target AUX / CS |
| `k` / `K` | `alternate_aux = 2` / `3` | v4 only; AUX1 / AUX2 |
| `l` / `L` | `little_endian = NO` / `YES` | MSB-first / LSB-first |
| `p` / `P` | `bp_disable_pullup()` / `bp_enable_pullup()` | refused in HiZ; `P` warns when pins are push-pull and samples VPU for a missing supply |
| `e` | `set_pullup_voltage()` | v4 only; external / on-board 3.3 V / on-board 5.0 V |
| `=` / `\|` | `convert_value(NO / YES)` | print HEX=DEC=BIN; the pipe form reverses within `numbits` first |
| `~` | `perform_selftest(true, true)` | HiZ only |
| `#` | reset path | v4: soft terminal/board reset; v3: hardware `RESET` |
| `$` | bootloader path | `agree()`, sets `skip_pgc_pgd_check`, resets |
| `a` / `A` / `@` | AUX low / high / read | `@` accepts a repeat count |
| `W` / `w` | `switch_psu_on()` / `switch_psu_off()` | refused in HiZ |
| `d` / `D` | `bp_adc_probe()` / `bp_adc_continuous_probe()` | `d:N` repeats the probe |
| `&` / `%` | `bp_delay_us()` / `bp_delay_ms()` | the repeat field is the delay value |
| `s` | `bus_pirate_configuration.basic = ON` | prompt becomes `<mode>(BASIC)>`; the interpreter handles the next Enter |
| `O` | 1-Wire speed toggle | `1WIRE` mode only; swaps standard/overdrive in place |
| `<n=text>` | define a user macro | the first `>` ends the body; `<0>` lists, `<n>` recalls; recall replays through the editor; see [macros](../generic/macros.md) |
| `(n)` | `run_macro(n)` | validates the trailing `)` |
| `"text"` | send each byte via `send()` | applies `little_endian` reversal |
| `[` / `{` | `start()` / `start_with_read()` | 2WIRE/3WIRE differ in what "start" means |
| `]` / `}` | `stop()` / `stop_from_read()` | — |
| digits | send value | repeat `:n`, width `;bits`; prints the sent value and, when `write_with_read` is set, `READ:` plus the reply |
| `r` | read value | optional display letter `x`/`d`/`b`/`w`, repeat `:n`, width `;bits` |
| `/` / `\` | clock high / low | — |
| `-` / `_` | data high / low | — |
| `.` | data state | echoes `0`/`1` |
| `^` | clock pulse(s) | uses `clock_pulses()` when the descriptor provides it, otherwise repeats `clock_pulse()` |
| `!` | read bit(s) | repeats and prints each bit |
| NUL / CR / LF / space / comma | ignored in the command loop | NUL also drives the binary-entry counter in the input loop |

The dispatcher stops a command when the cursor reaches `cmdend`, when a handler
sets `command_error`, or when a handler deliberately moves `cmdstart` to the
end (the mode and baud handlers do this).  For the transfer commands (`d`,
`@`, `!`, digits and `r`), the `:n` repeat field means *n* transfers: an absent
field returns 1 from `getrepeat()`, and those call sites add one and
pre-decrement.  `&` and `%` use the field directly as the delay value.

The semantics behind these keys are owned elsewhere: the capability keys by
[auxiliary](../capabilities/auxiliary/README.md) and
[adc](../capabilities/adc/README.md), the self-test by
[selftest](../capabilities/selftest/README.md), the prompt layer by
[generic](../generic/README.md), BASIC by
[tools/basic](../tools/basic/README.md), and the bus keys by the active
protocol set.  This page owns the dispatch and the core-level gating only.

## Parsing helpers

| Helper | Grammar | Limits and failure |
|---|---|---|
| `getint()` | decimal, `0x`/`0X` hex, `0b`/`0B` binary | 16-bit value; at least one digit; overflow or a token that fills the ring is rejected; advances to the first non-digit; never mutates the buffer; leading zeroes are decimal, not octal |
| `getrepeat()` | `:n` | 1..32767; absent field returns 1; zero rejected |
| `getnumbits()` | `;n` | 1..16; absent field returns 0 (width unchanged) |
| `getnumber(def,min,max,x)` | blocking dialog | shows `(def)>`; blank = default; `-` allowed at the start; lowercase `x` aborts only when enabled; non-digits bell; overlong or out-of-range input prints `Invalid choice, try again` and re-prompts |
| `getlong(def,min,max,x)` | v4 blocking dialog | up to 10 digits with explicit overflow check; same prompt/retry rules |
| `getint_with_spaces()` | consumes spaces, then `getint()` | the shared setup-prompt path used by the mode and tool setups |
| `consumewhitechars()` | spaces only | no-op in scripted setup mode |

`getnumber()` and `getlong()` read bytes directly (not through `cmdbuf`) and
echo what they accept.  `getnumber()` uses a five-digit buffer and accepts the
full signed 16-bit range; `getlong()` accepts the signed 32-bit range.
`getint()` returns the 16-bit pattern in an `int`; a caller that uses it as a
count or index must range-check it.  `setup_defaults_mode` short-circuits
`getint()` to 0 and `getnumber()` to the default, so a script can run a
protocol's setup without touching the terminal buffer.

## Mode registry

`core.c` owns a private `enabled_protocols[]` array of `bus_pirate_protocol_t`
descriptors.  `core.h` defines the positional index enum; conditional feature
macros remove entries, so indices are positional, not fixed numbers.
`BP_HIZ` is index 0 and is always present.

Default v4 image (KEYB on, LCD off):

| Menu | Enum | Displayed `name` |
|---|---|---|
| 1 | `BP_HIZ` | `HiZ` |
| 2 | `BP_1WIRE` | `1WIRE` |
| 3 | `BP_UART` | `UART` |
| 4 | `BP_I2C` | `I2C` |
| 5 | `BP_SPI` | `SPI` |
| 6 | `BP_RAW2WIRE` | `2WIRE` |
| 7 | `BP_RAW3WIRE` | `3WIRE` |
| 8 | `BP_PCATKBD` | `KEYB` |
| 9 | `BP_PICPROG` | `PIC` |
| 10 | `BP_DIO` | `DIO` |

`BP_HD44780` (`LCD`) sits between `BP_PCATKBD` and `BP_PICPROG`; it is off in
both default images.  Menu numbers for the four KEYB/LCD combinations:

| KEYB built | LCD built | `KEYB` menu | `LCD` menu | `PIC` menu | `DIO` menu |
|---|---|---|---|---|---|
| off | off | — | — | 8 | 9 |
| on | off | 8 | — | 9 | 10 |
| off | on | — | 8 | 9 | 10 |
| on | on | 8 | 9 | 10 | 11 |

The enum value is the menu number minus one in each combination.  The default
v3 image is KEYB off, LCD off.  Inserting anywhere but the end renumbers every
later entry for users and scripts.

JTAG is **not** in this registry: there is no interactive JTAG prompt mode.  It
is entered from the binary layer (OpenOCD `0x06`, XSVF `0x18`).  AVRISP is a USB
personality, also not a mode entry.  SMPS is a binary capability, not a mode.

Naming: the mode and its descriptor string are `1WIRE`; prose for the
technology is `1-Wire`; the firmware's status messages print
`1-WIRE speed: standard` and `1-WIRE speed: overdrive`.  Use `1WIRE` for the
mode everywhere in this set.

Two other display forms recur here.  The pin called `Vpullup` in messages is
the `VPU` column of the `v` table and `Vpu (Pullup)` in the v4 source menu;
this set uses `VPU` for the pin and quotes the message text.  The AUX target
values of `alternate_aux` are 0=`AUX`, 1=`CS`, 2=`AUX1`, 3=`AUX2`; the `v`
table and the `c`/`C`/`k`/`K` replies name the same pins.

### HiZ (the parked mode)

HiZ is registry index 0 and the state every reset returns to.  Its descriptor is
deliberately inert: all bus operations, `send`/`read`, clock/data and macro
callbacks use the null handlers; `setup_prepare`/`setup_execute`/`cleanup` use
`reset_mode_to_8_bits()`; `print_pins_state` prints the generic pin header
(`MSG_SPI_PINS_STATE`) and `print_settings` only emits CRLF.

Consequences in the dispatcher:

| Area | HiZ behaviour |
|---|---|
| `i` | version banner only; `print_status_info()` is skipped |
| `g`, `S`, `p`, `P`, `W`, `w` | refused with `Command not used in this mode` |
| `~` | allowed here only; runs `perform_selftest(true, true)` |
| `O` | refused (`1WIRE` only) |
| AUX / pull-ups / regulator | reset to the parked state by `bp_reset_board_state()` |
| binary / SUMP entry | still available; these do not require an active mode |

HiZ is also the safe state for the self-test and for changing modes: a mode
change first calls the current mode's `cleanup()` and then parks the board here
before the new setup runs.

Descriptor fields (all callbacks, plus `const char name[8]`):

| Group | Fields | Contract |
|---|---|---|
| bus operations | `start`, `start_with_read`, `stop`, `stop_from_read` | called by the `[` `{` `]` `}` keys |
| data | `send`, `read`, `read_bit`, `data_state`, `data_high`, `data_low` | width is `mode_configuration.numbits`, not an argument |
| clock | `clock_high`, `clock_low`, `clock_pulse`, `clock_pulses` | `clock_pulses` nullable; `^` falls back to repeated `clock_pulse` |
| polling | `periodic_update` | only used when `periodicService` is set |
| macros | `run_macro` | `(n)` |
| setup | `setup_prepare`, `setup_execute`, `cleanup` | see lifecycle below |
| presentation | `print_pins_state`, `print_settings`, `name` | `v`, `i`, prompt |

If a field is omitted from a descriptor initializer it stays NULL (the table
is static); it is not replaced by a default.  A descriptor the dispatcher can
reach must assign every required field.  The reusable bodies are:

| Field group | Fields | Required? | Reusable body | Observable behaviour |
|---|---|---|---|---|
| bus/clock operations | `start`, `start_with_read`, `stop`, `stop_from_read`, `clock_high`, `clock_low`, `data_high`, `data_low`, `clock_pulse` | yes | `null_operation_callback` | prints `ERROR: command has no effect here`, sets `command_error` |
| data operations | `send`; `read`, `data_state`; `read_bit`, `periodic_update` | yes; `periodic_update` only when `periodicService` is set | `null_send_callback`; `null_data_read_callback`; `null_bit_read_callback` | prints the same error and sets `command_error`; returns `0x100`, `0`, or `OFF` |
| macros | `run_macro` | yes | `null_macro_callback` | prints the same error and sets `command_error` |
| setup/cleanup | `setup_prepare`, `setup_execute`, `cleanup` | yes | `reset_mode_to_8_bits` where the mode needs no bus work | sets `numbits = 8` and clears `int16`; the full reset is central |
| batched clock | `clock_pulses` | no | NULL is supported | `^` repeats `clock_pulse()` instead |
| presentation | `print_pins_state`; `print_settings`; `name` | yes; `name` non-empty | `hiz_print_pins_state`; `empty_print_settings_implementation`; no default | generic pin header; CRLF only; `name` drives the prompt, menu and banner |

DIO's `setup_execute` is `silent_null_operation_callback`, which clears
`command_error` and prints nothing.  A NULL pointer in a field the dispatcher
calls is not a fallback; it faults.  `clock_pulses` is the only field the
dispatcher tests for NULL.

### Mode-change lifecycle

Interactive `m`/`m<n>` and scripted `bp_change_mode()` share steps 1–3 and 5;
step 4 answers the defaults without prompts in the scripted path, and step 6
differs (the scripted path calls `setup_execute()` immediately):

1. `cleanup()` on the current mode.
2. `bp_reset_board_state()`: the full ordered sequence is under
   [Board-state reset](#board-state-reset-bp_reset_board_state).  It releases
   the bus and AUX pins, stops SMPS, parks the onboard EEPROM, clears the mode
   configuration, disables the pull-ups and regulator, and reconfigures the
   ADC; `bus_mode` ends at `BP_HIZ`.
3. Set `bus_mode` to the new index.
4. `setup_prepare()`: the mode's setup prompts; the pins are prepared but the
   bus is not driven.
5. Mode LED on for a non-HiZ mode.
6. `setup_execute()` runs when the supplies are started (`W`), not at mode
   entry.  `bp_change_mode()` calls it immediately because a script never issues
   `W`.

Selecting a non-HiZ mode from the `m` menu prints `Clutch disengaged!!!`, the
finish-setup prompt and `Ready`; selecting HiZ omits the finish-setup line.
The direct `m<n>` form skips both announcements and prints only `Ready`.  Both
run the setup prompts and enable the mode LED for a non-HiZ mode.  `W` prints
`POWER SUPPLIES ON` and `Clutch engaged!!!` after `setup_execute()`; `w` calls
`cleanup()` and disables the regulator.

`switch_psu_on()` reads `BP_ADC_3V3` and `BP_ADC_5V0` after a 2 ms settle and
requires both above `V33L`/`V5L`; otherwise it disables the regulator and
reports a too-low VREG.  `P` additionally samples `BP_ADC_VPU` and warns when
the pull-up pin has no voltage.

### Adding a mode

The ordered work for a new registry mode.  It touches the registry enum and
table, the feature set, the build lists and the docs; the menu and prompt then
follow automatically.

1. **Implement the callbacks.**  Put the mode in its own source group
   (`protocol_<x>/`, `capability_<x>/` or `programmer_<x>/`) and provide the
   descriptor members that apply; callbacks that should do nothing can use the
   null defaults in `core.c`.  Width is not an argument: read
   `mode_configuration.numbits`.  A combined setup split is `setup_prepare()`
   (pins prepared, bus not driven) and `setup_execute()` (interactive: called
   when the supplies engage; the scripted `bp_change_mode()` calls it
   immediately).
2. **Extend the enum.**  Add `BP_<MODE>` to
   `bus_pirate_available_protocols_t` in `core.h`, inside its
   `#ifdef BP_ENABLE_<X>_SUPPORT` guard.  Order is the menu order and the
   indices are positional: append by default.  Inserting in the middle
   renumbers every later `m<n>` and BASIC `MODE n`, which is user-visible and
   script-breaking.
3. **Add the build flag.**  Define `BP_ENABLE_<X>_SUPPORT` in
   `configuration.h` for each board profile that ships the mode; the flag is
   what removes the enum entry, the descriptor and the code from other builds.
4. **Add the descriptor.**  Append the matching entry to `enabled_protocols[]`
   in `core.c` at the same guarded position, fill the callbacks and set `name`
   (at most 7 characters; the field is `char name[8]`).  This table is private;
   nothing indexes it directly.
5. **Wire the build.**  Add the source to `SOURCES` in `Makefile` and to the
   MPLAB-X project list (`busPirate.X/nbproject/configurations.xml`).  If the
   mode prints, add its strings to `tools/packstrings/` and regenerate the
   catalogs.  Check the path-list assertions in the native tests after moving
   files.
6. **Document it.**  Add the set under `docs/protocol/<x>/`,
   `docs/capabilities/<x>/` or `docs/programmers/<x>/` (README plus topic
   files), its row in `docs/SPEC.md`, and a row in the registry table above.
   BASIC's `MODE n` uses the same one-based menu numbers, so note the new index
   there too.

## Menus

`m` with no argument enumerates the registry as `<n>. <name>`, one line per
entry, then prints `x. exit(without change)` and reads a selection with
`getnumber`.  The displayed labels are the descriptor names, so the menu and
the prompt cannot drift.

| Menu | Keys / defaults | Notes |
|---|---|---|
| display mode `o` | 1 HEX, 2 DEC, 3 BIN, 4 RAW | `o1`..`o4` apply directly; `o0`, an invalid value or no argument prints the menu and uses `getnumber` default 1 |
| terminal baud `b` (v3) | 1..9 fixed (300…115200), 10 custom BRG | invalid or no argument prints the menu; the blocking menu default is 9 (115200); entry 10 asks for the raw `U1BRG` (blank default 34) |
| VPU source `e` (v4) | `External (or None)`, `Onboard 3.3v`, `Onboard 5.0v` | requires a non-HiZ mode, open-drain pins, and no external voltage already on VPU; invalid or no argument prints the menu, default 1 |
| history `h` | entry number, `x` exit | only initialized entries are listed; default 0 means no recall |
| mode menu `m` | 1..count, `x` exit | default 1 (HiZ); `x` prints `no mode change`; `m0` opens the menu; an invalid direct `m<n>` prints `Nonexistent protocol!` |

v4 `set_baud_rate()` is intentionally inert: CDC line coding is host metadata
and the firmware does not reconfigure a physical UART.  On v3, after a change
the firmware prints `Adjust your terminal`, then `Space to continue`, and waits
up to five seconds (Timer1 deadline, or 5000 ms without the hardware timer) for
a space; a timeout restores the previous speed and BRG.

## Terminal reset and exit paths

`reset_terminal_state()` is the shared terminal soft-reset helper, used by the
v4 `#` path, the SUMP exit and the v4 binary return.  It calls the current
mode's `cleanup()`, runs `bp_reset_board_state()`, disables the USB LED, clears
`quiet`, restores HEX, clears the BASIC flag, and resets the editor/prefix
state.  It does not clear the user macro table; `serviceuser()` clears the
macros at entry.

### Board-state reset (`bp_reset_board_state()`)

`bp_reset_board_state()` parks the board and always ends in `BP_HIZ` with the
mode configuration cleared.  Order:

1. Release the bus pins (MOSI, CLK, MISO, CS) and AUX0: direction input.
2. v4: release AUX1/AUX2; on SMPS builds run `smps_stop()`; detach the
   AUX1/AUX2 peripheral outputs and clear their latches.
3. Disable the mode LED and enable the USB LED.
4. v4: set up the secondary hardware I2C/SPI pins (input, low) and the
   button, disable the onboard 3.3 V and 5 V VPU sources, and call
   `eeprom_initialize()` (see [eeprom.md](eeprom.md)).
5. Detach the source from the currently selected AUX pin.
6. Set `bus_mode = BP_HIZ` and call `clear_mode_configuration()`.
7. Call `bp_disable_pullup()`, `bp_disable_voltage_regulator()` and
   `bp_adc_pin_setup()`.
8. Reconfigure the ADC: automatic sample trigger, no scan, 3*Tcy conversion
   clock, one 16-bit result buffer.

Callers:

| Caller | Path |
|---|---|
| `initialize_board()` | boot bring-up |
| `changemode()` | `m` menu and direct `m<n>` |
| `bp_change_mode()` | scripted/binary mode change |
| `reset_terminal_state()` | v4 `#`, SUMP exit, v4 binary return |
| `$` | bootloader handshake before the hardware reset |
| `perform_selftest()` | end of the `~` self-test |

The onboard-EEPROM and SMPS details are owned by [eeprom.md](eeprom.md) and
[smps](../capabilities/smps/README.md).

The corresponding command paths:

| Path | Behaviour |
|---|---|
| `#` (v4) | prints `RESET`, runs `reset_terminal_state()`, rewinds the command cursor so the rest of the line is discarded, reprints the version banner; it does not re-enumerate USB |
| `#` (v3) | prints `RESET`, waits for transmission to drain, executes the hardware `RESET` |
| `$` | `agree()`, sets the persistent `skip_pgc_pgd_check`, prints `BOOTLOADER`, 100 ms delay, board reset, waits for the UART/USB queue, executes `RESET`; with the matching signature the loader takes the bootloader path and clears the flag.  On v3 only the 4.5 loader understands the flag; older loaders ignore it and take the jumper path, so the PGC/PGD jumper is the fallback |
| `m1` / `bp_change_mode(1)` | returns to HiZ and runs the board-state reset but leaves `quiet` and the display mode alone, and prints `Ready` instead of the banner |
| binary entry | 20 NUL bytes call `enter_binary_bitbang_mode()`.  v4: the `0x0F` return runs the terminal reset, reprints the banner and `HiZ>`.  v3: `0x0F` executes a hardware `RESET`; the board reboots into `HiZ`, so there is no soft return and no "previous mode" prompt |
| SUMP entry | 5–19 NUL bytes followed by the raw left-arrow byte (`0x02`, STX) call `enter_sump_mode()` (20 NULs enter the binary layer instead).  `0x00` inside SUMP resets the analyzer and stays in SUMP; the exit bytes are `0x0F`, `#` and `$` (any of them parks in HiZ and prints the banner, then the input-loop handler applies the terminal state reset).  A `$` inside SUMP is an exit, not a bootloader request |
| BASIC | `s` sets the basic flag; the prompt becomes `<mode>(BASIC)>` and the interpreter handles each Enter, printing `Ready` when the line returns.  `EXIT` clears the flag; `#` and `$` are unreachable while it is set |

The binary-entry counter is cleared by any byte that is not NUL or STX, so an
ANSI cursor key or ordinary typing cancels a partial entry sequence.  The SUMP
test is `binary_mode_counter >= 5` at the left-arrow handler.

## Failure modes and edge cases

| Trigger | Symptom | Behaviour | Recovery |
|---|---|---|---|
| Command ring full | bell; byte refused | printable byte is not inserted; unread history is not overwritten | Backspace or Enter to free space |
| Syntax error | `Syntax error at char <n>` | command stops at the offending position; `<n>` is one-based from the command start | retype the command |
| Invalid menu input | bell; overlong or out-of-range input re-prompts | non-digits ignored; blank takes the default; `x` aborts only when enabled; uppercase `X` is ignored | answer again or `x` |
| Direct mode index out of range | `Nonexistent protocol!` | current mode untouched; the menu path cannot produce an out-of-range value | choose `1..count` |
| Power-up failure | `W` reports a too-low VREG | regulator disabled; `setup_execute()` not run; mode stays unengaged | fix the supply and reissue `W` |
| Pull-up without source | no-voltage warning after `P` | pull-up element is enabled anyway; the command is not refused | provide VPU or an external pull-up, or use `p` |
| ADC busy | `ADC busy: SMPS feedback active; command rejected.` | `command_error` set; `P`, `d`, `D`, `e`, `W` and `v` do not proceed | wait for the feedback owner; `smps_stop()` releases the ADC on reset |
| Baud change unconfirmed (v3) | `Baud confirmation timed out; previous rate restored.` | previous speed and BRG restored | reissue `b` and press space in time |
| Binary return (v3) | hardware `RESET` | reboots through bring-up into `HiZ`; nothing from the pre-entry mode is preserved | re-enter the mode |
| Stale macros | slot text survives `reset_terminal_state()` | editor/history/prefix state is cleared; the macro table is not | overwrite the slot; the table is cleared only when `serviceuser()` is entered again (a reboot), not by the soft reset |
| Truncated escape sequence | no block | the final byte of an unrecognized sequence is processed as ordinary input | none |
| v3 output ring stopped | flush waits on `UTXBF` forever | no timeout | restore the terminal or UART |
| `$` inside SUMP | SUMP treats it as an exit | parks in `HiZ`, not the bootloader | send a separate `$` at the terminal prompt |

## Invariants and contracts

- **One foreground context.**  The dispatcher, protocol callbacks and the
  binary/SUMP engines run in the same loop.  Values shared with an ISR (USB
  buffer descriptors, the v3 UART transfer globals) are `volatile` and are
  updated under interrupt exclusion where the protocol requires it.
- The mode accessors do not range-check: `bp_current_protocol()` trusts
  `bus_mode`, and `bp_protocol_by_index()` trusts its index.  Callers validate
  against `ENABLED_PROTOCOLS_COUNT` before indexing.
- The descriptor table is `static`; access is only through the two accessors.
- The dispatcher owns insertion/removal in `cmdbuf` and the `cmdstart`/`cmdend`
  cursors.  Helpers may inspect the ring at `cmdstart`; BASIC may uppercase the
  parsed line in place, but nothing else adds or removes characters.
- The parser never carries state across the end of a line: each Enter starts a
  fresh parse at `cmdstart`.  Sticky mode settings (width, byte order) live in
  `mode_configuration`, not in the parser.
- For the value/read commands, the dispatcher takes the transfer width from
  `mode_configuration.numbits`; an explicit `;bits` suffix is validated 1..16
  before the mode call.
- `bp_reset_board_state()` always ends in `BP_HIZ` with the mode configuration
  cleared, whatever path called it.
- Setup prompts share `getint_with_spaces()`/`getnumber()` so grammar, echo and
  error behaviour are identical across modes.
- `bp_change_mode()` does not prompt: `setup_defaults_mode` makes the setup
  helpers return defaults.  Any setup text a protocol prints itself is outside
  that guarantee.
- No core path persists user settings; only the bootloader signature and
  `skip_pgc_pgd_check` survive a reset.

## Native test coverage

`tests/test_core_system.py` compiles production functions with deterministic
serial/MMIO mocks.  It covers formatting/width masking, mode-state reset, every
ring seam, uint16 parsing/non-mutation, dialogs, baud/Timer1 behaviour,
escape/prefix/reset, latches, full uint32 output, macros, history, v3 ISR
bounds and read display selectors.  It also asserts that the MPLAB-X project
lists all seven `core_system` files at existing paths.

The reset and policy seams have separate host suites:

| Suite | Models |
|---|---|
| `tests/test_core_reset.py` | `bp_reset_board_state()` and the SMPS-stop order, with register fixtures |
| `tests/test_core_cleanup.py` | retired core declarations and stable USB encodings |
| `tests/test_core_config_words.py` | flash-config addresses and the status-report read helper |
| `tests/test_core_adc_ownership.py` | the foreground ADC rejection policy, with SMPS bodies as an owner fixture |
| `tests/test_core_dio_cleanup.py` | the core DIO fallback, park behaviour and cleanup dispatch fixture |
| `tests/test_core_lcd_cleanup.py` | the core LCD fallback, park behaviour and cleanup dispatch fixture |
| `tests/test_core_sump_return.py` | the binary/SUMP return output and state |

Run a suite directly from the repository root, for example
`python Firmware/tests/test_core_system.py`.  There is no aggregate runner in
this tree.  These are host-side tests, not a PIC simulator; the weak-linkage
fixtures note that MinGW does not model XC16 weak resolution.

## Related docs

- [navigation](../generic/navigation.md), [commands](../generic/commands.md),
  [values](../generic/values.md), [macros](../generic/macros.md),
  [usage](../generic/usage.md), [edge-cases](../generic/edge-cases.md) —
  the user-facing prompt layer.
- [Idle mode (HiZ)](../generic/README.md#idle-mode-hiz) — HiZ behaviour.
- [binary](../protocol/binary/README.md), [sump](../protocol/sump/README.md) —
  the two non-CLI entry paths and their reset/exit bytes.
- [auxiliary](../capabilities/auxiliary/README.md) — PWM, servo, frequency,
  AUX.
- [adc](../capabilities/adc/README.md) — `d`/`D` probe front ends.
- [selftest](../capabilities/selftest/README.md) — the `~` self-test.
- [smps](../capabilities/smps/README.md), [lcd](../capabilities/lcd/README.md)
  — the remaining capability fronts.
- [basic](../tools/basic/README.md) — the `(BASIC)` prompt and `EXIT`.
- [shared-machinery](../dev/shared-machinery.md) — the shared accessor
  contract.
- [bootloaders](../dev/bootloaders.md) — the persistent-flag handoff.
- [usb.md](usb.md), [bitbang.md](bitbang.md), [messages.md](messages.md),
  [eeprom.md](eeprom.md) — the sibling modules.
