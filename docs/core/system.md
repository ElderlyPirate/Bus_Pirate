# System (`core_system/`)

**Status:** maintainer-facing.  Covers startup, the terminal dispatcher, the
mode registry, shared state, menus and the reset/exit paths.  The user-facing
prompt grammar and command reference live in `docs/generic/`; this page
explains the implementation behind them.

Source and test paths on this page are relative to `Firmware/`; tool/build and
documentation paths are relative to the repository root.  The Scope table
lists this page's group files by name (the group is named in the page title).
v3 behaviour here is established from source and native tests only (see the
README limits).

## Scope and files

| File | Responsibility |
|---|---|
| `main.c` | reset entry, persistent firmware signature, board bring-up, v4 USB wait, USB interrupt vector, `usb_suspend()` |
| `core.c` / `core.h` | mode-descriptor type, protocol enumeration, private registry table, live/index accessors, default callbacks, display-mode enum, mode-configuration type |
| `base.c` / `base.h` | board reset, serial abstraction, decimal/hex/bin output, voltage formatting, flash reads, delay/timer API, v3 UART ring and ISRs |
| `proc_menu.c` / `proc_menu.h` | `serviceuser()` loop, line editor, history, user macros, parsing helpers, mode/setup menus, status/pin printouts |

`core_system` does not contain a protocol engine.  It sequences protocol
callbacks and provides the helpers every mode prints and parses with.

## Boot and main flow

1. `main()` writes `firmware_signature` (constant `0x31415926`) to its
   persistent address (`0x47FC` on v4, `0x27FC` on v3).  The bootloader reads
   this value and the persistent `skip_pgc_pgd_check` flag to decide whether to
   stay in the bootloader.
2. `initialize_board()` configures the clock/PLL, analog pins, delay timer and
   board state.  On v4 it initialises the CDC layer and the USB stack
   (`initCDC()`, `usb_init()`, `usb_start()`); on v3 it starts the hardware UART
   at the default speed.
3. Global defaults are set: `terminal_speed = 8` (115200), `display_mode = HEX`,
   `quiet = OFF`, `mode_configuration.numbits = 8`.
4. `bp_reset_board_state()` parks the board in HiZ, clears the mode
   configuration, disables pull-ups/regulator, sets the ADC up and (on v4)
   reinitialises the onboard EEPROM pins.
5. On v4, `main()` enables `USB_TRN | USB_SOF | USB_UERR | USB_URST` interrupts
   and blocks until `usb_device_state >= CONFIGURED_STATE`, then registers
   `CDCFlushOnTimeout` as the SOF handler.  On v3 the banner is printed at the
   end of `initialize_board()`.
6. `serviceuser()` is entered and never returns.  `main()` ends in that call.

`_USB1Interrupt` calls `usb_handler()` and clears the USB1 flag.  Timer1's
overflow ISR (delay/timestamp counter) and, on v3, the UART1 RX/TX ISRs are the
other interrupt contexts in this module.

## Terminal state and data model

| Object | Where | Meaning |
|---|---|---|
| `bp_buffer` | `main.c` | `BP_TERMINAL_BUFFER_SIZE` bytes, 2-byte aligned; the terminal/sample buffer (the v3 serial ring also uses it) |
| `bus_pirate_configuration` | `main.c` | board/UI state: `terminal_input`, device type/revision, hardware version, terminal speed, display mode, `bus_mode`, `quiet`, `overflow`, `basic` |
| `mode_configuration` | `main.c` | per-mode state: `speed`, `numbits`, `alternate_aux` (2 bits), `periodicService`, `little_endian`, `high_impedance`, `int16`, `write_with_read`, `command_error` |
| `cmdbuf`, `cmdstart`, `cmdend` | `proc_menu.c` | command ring (`BP_COMMAND_BUFFER_SIZE`, mask `CMDLENMSK`); `cmdend` is the insertion point, `cmdstart` the parse cursor |
| `menu_state` | `proc_menu.c` | cursor position, history counter, binary-entry counter, escape state, `command_present` |
| `user_macros`, `user_macro` | `proc_menu.c` | `BP_USER_MACROS_COUNT` slots of `BP_USER_MACRO_MAX_LENGTH` bytes; `user_macro` is a pending recall |
| `setup_defaults_mode` | `proc_menu.c` | when set, setup prompts answer defaults without reading or printing (scripted mode change) |
| `last_command` | `main.c` | legacy `command_t`; declared and externed by several modules but not written or read by any C source (currently unused) |

Size effects (from `configuration.h`):

| Setting | v3 | v4 |
|---|---|---|
| command buffer | 128 B | 256 B |
| terminal buffer | 2048 B | 4096 B |
| command history | 7 entries | 15 entries |
| user macros | 3 slots × 24 B | 5 slots × 32 B |

`clear_mode_configuration()` (called from `bp_reset_board_state()`) sets
`numbits = 8`, clears `int16`, `write_with_read`, `command_error`,
`high_impedance`, `speed`, `periodicService`, `alternate_aux` and
`little_endian`.  A protocol's setup may override these afterwards.

## Serial abstraction

`base.c` presents one byte-stream interface; `main.c`, the dispatcher, the
protocols and the binary/SUMP engines all use it.  No module writes the v4 CDC
registers; on v3 the UART bridge/bulk paths and the OpenOCD fast path do touch
U1 directly.  v3 is a hardware UART with a
terminal output ring; v4 is the USB CDC stack in `core_usb`.

| Call | v3 (`base.c`) | v4 (`base.c` → `cdc.c`) |
|---|---|---|
| `user_serial_initialise()` | programs UART1 (`U1MODE`/`U1STA`, baud from `terminal_speed`) | no-op (the CDC stack is initialised by `initCDC()`/`usb_init()`) |
| `user_serial_set_baud_rate(rate)` | writes `U1BRG` | no-op; CDC line coding is host metadata |
| `user_serial_ready_to_read()` | `U1STAbits.URXDA`, non-blocking | `cdc_Out_len \|\| getOutReady()`, non-blocking (recycles ZLPs) |
| `user_serial_read_byte()` | **blocks** until `URXDA`, returns `U1RXREG` | **blocks** in `getc_cdc()` until a packet arrives |
| `user_serial_transmit_character(c)` | `quiet` suppresses; else **blocks** while `UTXBF`, writes `U1TXREG` | `quiet` suppresses; else `putc_cdc(c)` buffers, then **blocks** in `WaitInReady()` when a 64-byte packet fills and the IN BD is not yet free |
| `user_serial_wait_transmission_done()` | **blocks** until `TRMT` | **blocks** in `WaitInReady()` for the IN BD (only while configured) |
| `user_serial_transmit_done()` | `TRMT` | always `YES` |
| `user_serial_check_overflow()` / `clear_overflow()` | `OERR` flag / clear it | always `NO` / no-op |
| `user_serial_ringbuffer_append(c)` | appends to the output ring in `bp_buffer`, sets `overflow` when full | `putc_cdc(c)` |
| `user_serial_ringbuffer_process()` / `flush()` | drains one byte / drains fully to `U1TXREG` | no-op / `CDC_Flush_In_Now()` |
| `user_serial_ringbuffer_setup()` | resets the ring indices and `overflow` | no-op |
| `user_serial_read_big_endian_word()` / `_long_word()` | two / four `user_serial_read_byte()` calls | same |

Contract notes:

- **Reads block.** A caller that must not stall polls `ready_to_read()` first,
  as `serviceuser()` and the SUMP loop do.  There is no timeout.
- **`quiet` is output-only.** Both builds drop characters inside
  `user_serial_transmit_character()`, so formatting and bus work still happen;
  `reset_terminal_state()` clears it.
- **Overflow is a v3 concept.** Only the v3 UART exposes `OERR` and the ring's
  full flag.  v4 has no application-visible overflow: the USB stack keeps
  ownership of a busy OUT buffer and the host retries.
- **Both builds share the word/long readers**, so protocol code can read
  multi-byte values without knowing the transport.

## `serviceuser()` lifecycle

The outer loop is: print `<mode>`, optionally `(BASIC)`, then `>`, and collect
one command.

1. **Input wait.** While no byte is ready, a mode with
   `mode_configuration.periodicService` set has `periodic_update()` polled.  If
   it reports output, the current prompt and the partially typed command are
   reprinted so asynchronous mode output does not swallow the line.
2. **Overflow.** A v3 UART receive overflow is cleared and the loop restarts.
3. **Byte handling.** The byte is offered to the escape-sequence state machine
   (if one is in progress), then to the key map below.  Printable bytes go to
   the line editor; the dispatcher parses the line only after Enter.
4. **Enter.** `handle_enter_key()` sets `command_present`, NUL-terminates the
   line, advances `cmdend` and prints CRLF.
5. **Execution.** The dispatcher walks from `cmdstart` to `cmdend`.  Each key is
   a case in the switch in `serviceuser()`; the cursor advances after each
   case.  A handler may consume extra characters (numbers, repeats, widths).
6. **Error.** `mode_configuration.command_error` prints
   `Syntax error at char <n>` (distance from the command start) and ends the
   command.  The next loop iteration starts a fresh command.
7. **BASIC.** If `bus_pirate_configuration.basic` is set when a line is
   entered, the dispatcher calls `bp_basic_enter_interactive_interpreter()`
   instead of running that line, prints `Ready`, and returns to the prompt.
   Every typed line is intercepted before the command switch, so ordinary
   prompt keys (`#`, `$`, `m`, `w`, …) are not dispatched while BASIC is
   active; `EXIT`, handled inside the interpreter, clears the flag.  The
   prompt is the mode name plus `(BASIC)`: `HiZ(BASIC)>`, `SPI(BASIC)>`.  `s`
   changes the prompt immediately and the interpreter starts on the next
   Enter.  User-level detail: `docs/tools/basic/entry-and-prompt.md`.

The prompt string is the mode descriptor's `name`, so `HiZ>` is simply the HiZ
descriptor.  The same names drive the mode menu and the `[ ... ]` list in the
version banner.

## Line editor

| Input | Effect |
|---|---|
| printable `0x20`..`0x7E` | insert at cursor; append at end otherwise |
| Backspace | delete left of cursor; bell at line start |
| Delete / EOT | delete at cursor; bell at end |
| Left / right arrow, Home, End | move the cursor (ANSI `ESC [ …` sequences are decoded, not passed through) |
| Up / down arrow | recall older/newer history entry |
| Escape | starts escape-sequence decoding |
| Enter | finish the line |

The buffer is a ring; all indices are masked with `CMDLENMSK`.  Insertion and
removal shift the ring in place and echo the moved text with ANSI cursor
commands.  Empty slots and the active command are not listed as history
entries.

## Command dispatcher

These are the keys handled by `serviceuser()`.  Protocol-level semantics belong
to the mode page; the table records what the dispatcher does and which helper it
calls.

| Key | Handler / call | Notes |
|---|---|---|
| `h` | `cmdhistory()` | lists prior commands from the ring, then recalls a selected one (history build option) |
| `?` | `print_help()` | prints the fixed `HLP1000`..`HLP1022` block |
| `i` | `print_version_info()`, `print_status_info()` | status block only outside HiZ |
| `m` | `changemode()` | mode menu when no argument; `m<n>` switches directly |
| `b` | `set_baud_rate()` | v3 terminal baud; v4 prints that CDC baud is host-controlled |
| `o` | `set_display_mode()` | HEX / DEC / BIN / RAW |
| `v` | `print_pins_information()` | pin directions/states, rail voltages, mode pin table |
| `t` / `T` | `bp_timer_print()` / `bp_timer_reset()` | 32-bit microsecond timestamp |
| `f` | `bp_frequency_counter_setup()` | frequency counter on AUX |
| `g` | `bp_pwm_setup()` | refused in HiZ |
| `S` | `bp_servo_setup()` | refused in HiZ |
| `c` / `C` | `alternate_aux = 0` / `1` | AUX target AUX / CS |
| `k` / `K` | `alternate_aux = 2` / `3` | v4 only; AUX1 / AUX2 |
| `l` / `L` | `little_endian = NO` / `YES` | MSB-first / LSB-first |
| `p` / `P` | `bp_disable_pullup()` / `bp_enable_pullup()` | refused in HiZ; `P` warns when pins are push-pull and samples VPU for a missing supply |
| `e` | `set_pullup_voltage()` | v4 only; external / on-board 3.3 V / on-board 5.0 V |
| `=` / \| | `convert_value(NO / YES)` | print HEX=DEC=BIN; the pipe form reverses within `numbits` first |
| `~` | `perform_selftest(true, true)` | HiZ only |
| `#` | reset path | v4: soft terminal/board reset; v3: hardware `RESET` |
| `$` | bootloader path | `agree()`, sets `skip_pgc_pgd_check`, resets |
| `a` / `A` / `@` | AUX low / high / read | `@` accepts a repeat count |
| `W` / `w` | `switch_psu_on()` / `switch_psu_off()` | refused in HiZ |
| `d` / `D` | `bp_adc_probe()` / `bp_adc_continuous_probe()` | `d:N` repeats the probe |
| `&` / `%` | `bp_delay_us()` / `bp_delay_ms()` | repeat field is the delay value |
| `s` | `bus_pirate_configuration.basic = ON` | prompt becomes `<mode>(BASIC)>`; the interpreter starts on the next Enter |
| `O` | 1-Wire speed toggle | `1WIRE` mode only; swaps standard/overdrive in place |
| `<n>=text>` | define user macro | `<0>` lists, `<n>` recalls; recall replays through the editor |
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
| NUL / CR / LF / space / comma | ignored in the command loop | NUL also drives the binary-entry counter |

The dispatcher stops a command when the cursor reaches `cmdend`, when a handler
sets `command_error`, or when a handler deliberately moves `cmdstart` to the
end (the mode and baud handlers do this).

## Parsing helpers

| Helper | Grammar | Limits and failure |
|---|---|---|
| `getint()` | decimal, `0x`/`0X` hex, `0b`/`0B` binary | 16-bit value; leading digits required; overflow/too-long rejected; advances to the first non-digit; does not mutate the buffer |
| `getrepeat()` | `:n` | 1..32767; absent field returns 1; zero rejected |
| `getnumbits()` | `;n` | 1..16; absent field returns 0 (width unchanged) |
| `getnumber(def,min,max,x)` | blocking dialog | shows `(def)>`; blank = default; `-` allowed at the start; `x` aborts only when enabled; out-of-range or overlong input prints `Invalid choice, try again` and re-prompts |
| `getlong(def,min,max,x)` | v4 blocking dialog | up to 10 digits with explicit overflow check |
| `getint_with_spaces()` | consumes spaces, then `getint()` | the single shared setup-prompt path |
| `consumewhitechars()` | spaces only | no-op in scripted setup mode |

`getnumber()` and `getlong()` read bytes directly (not through `cmdbuf`) and
echo what they accept.  `setup_defaults_mode` short-circuits `getint()` to 0 and
`getnumber()` to the default, so a script can run a protocol's setup without
touching the terminal buffer.

## Mode registry

`core.c` owns a private `enabled_protocols[]` array of `bus_pirate_protocol_t`
descriptors.  `core.h` defines the positional index enum; conditional feature
macros remove entries, so indices are positional, not fixed numbers.

| Order | Enum | Displayed `name` | Build |
|---|---|---|---|
| 0 | `BP_HIZ` | `HiZ` | both |
| 1 | `BP_1WIRE` | `1WIRE` | both |
| 2 | `BP_UART` | `UART` | both |
| 3 | `BP_I2C` | `I2C` | both |
| 4 | `BP_SPI` | `SPI` | both |
| 5 | `BP_RAW2WIRE` | `2WIRE` | both |
| 6 | `BP_RAW3WIRE` | `3WIRE` | both |
| 7 | `BP_PCATKBD` | `KEYB` | v4 default; v3 optional |
| 8 | `BP_HD44780` | `LCD` | optional both; off in the default image |
| 9 | `BP_PICPROG` | `PIC` | both |
| 10 | `BP_DIO` | `DIO` | both |

JTAG is **not** in this registry: there is no interactive JTAG prompt mode.  It
is entered from the binary layer (OpenOCD `0x06`, XSVF `0x18`).  AVRISP is a USB
personality, also not a mode entry.

Naming: the mode and its descriptor string are `1WIRE`; prose for the
technology is `1-Wire`; the firmware's status messages print `1-WIRE speed:
standard/overdrive`.  Use `1WIRE` for the mode everywhere in this set.

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

Callbacks that should do nothing are assigned the null defaults in `core.c`;
`clock_pulses` is intentionally left NULL where the protocol cannot batch
ticks.  Each null callback prints `ERROR: command has no effect here` and sets
`command_error`; DIO's silent setup callback is the one exception.
`setup_prepare`/`setup_execute`/`cleanup` use `reset_mode_to_8_bits()` where no
protocol work is needed; it only resets `numbits`/`int16` (the full reset is
central).

### Mode-change lifecycle

Interactive `m`/`m<n>` and scripted `bp_change_mode()` share steps 1–3 and 5;
step 4 answers the defaults without prompts in the scripted path, and step 6
differs (the scripted path calls `setup_execute()` immediately):

1. `cleanup()` on the current mode.
2. `bp_reset_board_state()`: pins to input, mode LED off, USB LED on, board
   state, EEPROM driver/bus (v4), AUX reset, `bus_mode = BP_HIZ`, mode
   configuration cleared, pull-ups/regulator off, ADC configured.
3. Set `bus_mode` to the new index.
4. `setup_prepare()`: the mode's setup prompts; the pins are prepared but the
   bus is not driven.
5. Mode LED on for a non-HiZ mode.
6. `setup_execute()` runs when the supplies are started (`W`), not at mode
   entry.  `bp_change_mode()` calls it immediately because a script never issues
   `W`.

Selecting a non-HiZ mode from the `m` menu prints `Clutch disengaged!!!`, the
finish-setup prompt and `Ready`; the direct `m<n>` form skips the first two and
prints only `Ready`.  Both enable the mode LED for a non-HiZ mode.  `W` prints
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
   descriptor members that apply; callbacks
   that should do nothing can use the null defaults in `core.c`.  Width is not
   an argument: read `mode_configuration.numbits`.  A combined setup split is
   `setup_prepare()` (pins prepared, bus not driven) and `setup_execute()`
   (interactive: called when the supplies engage; the scripted
   `bp_change_mode()` calls it immediately).
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
   in `core.c` at the same guarded position, fill the callbacks and set
   `name` (at most 7 characters; the field is `char name[8]`).  This table is
   private; nothing indexes it directly.
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

`m` with no argument enumerates the registry:

```
1. HiZ
2. 1WIRE
3. UART
...
x. exit(without change)
```

The listing is produced with `bp_protocol_by_index()`; the displayed labels are
the descriptor names, so the menu and the prompt cannot drift.

| Menu | Keys / defaults | Notes |
|---|---|---|
| display mode `o` | 1 HEX, 2 DEC, 3 BIN, 4 RAW | `o0` or an invalid value falls back to the blocking menu with default 1 |
| terminal baud `b` (v3) | 1..9 fixed (300…115200), 10 custom BRG | value 10 asks for the raw `U1BRG` (default 34); the `b` path validates 1..10 |
| VPU source `e` (v4) | 1 external/none, 2 on-board 3.3 V, 3 on-board 5.0 V | requires a non-HiZ mode, open-drain pins, and no external voltage already on VPU |
| history `h` | entry number, `x` exit | only initialized entries are listed |
| mode menu `m` | 1..count, `x` exit | `m0` behaves as the menu |

v4 `set_baud_rate()` is intentionally inert: CDC line coding is host metadata
and the firmware does not reconfigure a physical UART.  On v3, after a change
the firmware prints `Adjust your terminal`, then `Space to continue`, and waits
up to five seconds (Timer1 deadline, or 5000 ms without the hardware timer) for
a space; a timeout restores the previous speed and BRG.

## Reset and exit paths

| Path | Behaviour |
|---|---|
| `#` (v4) | prints `RESET`, runs `reset_terminal_state()` (cleanup, board reset, USB LED off, `quiet = NO`, display = HEX, BASIC off, editor state cleared), rewinds the command cursor, reprints the version banner; it does not re-enumerate USB |
| `#` (v3) | prints `RESET`, waits for transmission to drain, executes the hardware `RESET` |
| `$` | `agree()`, sets the persistent `skip_pgc_pgd_check`, 100 ms delay, board reset, waits for the UART/USB queue, executes `RESET`; the bootloader sees the flag and stays in the loader |
| binary entry | 20 NUL bytes → `enter_binary_bitbang_mode()`.  v4: the `0x0F` return resets terminal state and reprints the banner and `HiZ>`.  v3: `0x0F` executes a hardware `RESET`; the board reboots into `HiZ`, so there is no soft return and no "previous mode" prompt |
| SUMP entry | at least 5 NUL bytes followed by the raw left-arrow byte (`0x02`, STX) → `enter_sump_mode()`.  `0x00` inside SUMP resets the analyzer and stays in SUMP; the exit bytes are `0x0F`, `#` and `$` (any of them parks in HiZ and prints the banner, then the dispatcher applies the terminal state reset) |
| BASIC | `s` sets the basic flag; the prompt becomes `<mode>(BASIC)>` and the next Enter runs the interpreter, which prints `Ready`.  `EXIT` clears the flag; `#` and `$` are unreachable while it is set |

The binary-entry counter is cleared by any byte that is not NUL or STX, so an
ANSI cursor key or ordinary typing cancels a partial entry sequence.  The SUMP
test is `binary_mode_counter >= 5` at the left-arrow handler.

## Failure modes and edge cases

- **Command ring full.** The editor refuses a printable byte with a bell rather
  than overwriting unread history.
- **Syntax error.** A key with no handler, an invalid numeric field, an
  out-of-range repeat/width or a malformed macro/wrapper stops the command at
  the offending position and reports the character offset.
- **Menu input.** `getnumber()` ignores non-digits with a bell; blank input
  takes the default; `x` is only accepted when the caller enables it; overlong
  input and out-of-range values re-prompt.
- **Mode index.** A mode index outside the registry prints
  `Nonexistent protocol!` and leaves the current mode untouched.
- **Power-up failure.** If the regulator does not reach the expected rails,
  `W` disables it again and does not run `setup_execute()`; the mode stays
  unengaged.
- **Pull-up without source.** `P` still enables the pull-up element, then reads
  VPU and prints the no-voltage warning; it does not refuse the command.
- **BAUD change abandonment.** A v3 baud change that is not confirmed within the
  deadline restores the previous speed/BRG and says so.
- **Binary return (v3).** The `0x0F` return executes a hardware `RESET`; the
  board reboots through the normal bring-up into `HiZ`.  Nothing from the
  pre-entry mode is preserved.  v4 returns to `HiZ>` through the soft terminal
  reset instead.
- **Stale state.** `reset_terminal_state()` clears editor/history/prefix state
  but not the user macro table at runtime; macros are cleared at
  `serviceuser()` entry.

## Invariants and contracts

- **One foreground context.**  The dispatcher, protocol callbacks and the
  binary/SUMP engines run in the same loop.  Values shared with an ISR (USB
  buffer descriptors, the v3 UART transfer globals) are `volatile` and are
  updated under interrupt exclusion where the protocol requires it.
- The mode accessors do not range-check: `bp_current_protocol()` trusts
  `bus_mode`, and `bp_protocol_by_index()` trusts its index.  Callers validate
  against `ENABLED_PROTOCOLS_COUNT` before indexing.
- The dispatcher owns insertion/removal in `cmdbuf` and the `cmdstart`/`cmdend`
  cursors.  Helpers may inspect the ring at `cmdstart`; BASIC may uppercase the
  parsed line in place, but nothing else adds or removes characters.
- The parser never carries state across the end of a line: each Enter starts a
  fresh parse at `cmdstart`.  Sticky mode settings (width, byte order) live in
  `mode_configuration`, not in the parser.
- For the value/read commands, the dispatcher takes the transfer width from
  `mode_configuration.numbits` and validates 1..16 before calling the mode.
- `bp_reset_board_state()` always ends in `BP_HIZ` with the mode configuration
  cleared, whatever path called it.
- Setup prompts share `getint_with_spaces()`/`getnumber()` so grammar, echo and
  error behaviour are identical across modes.
- `bp_change_mode()` does not prompt: `setup_defaults_mode` makes the setup
  helpers return defaults.  Any setup text a protocol prints itself is outside
  that guarantee.

## Native test coverage

`tests/test_core_system.py` (run from the repository root:
`python Firmware/tests/test_core_system.py`) compiles the production functions
with deterministic serial/MMIO mocks and covers formatting/width masking, state
reset, every ring seam, uint16 parsing/non-mutation, dialogs, baud/Timer1
wraps, escape/prefix/reset, latches, full uint32 output, macros, history, v3
ISR bounds and read display selectors.  It is a standalone harness;
`tests/test_native.py` runs only the shared-machinery battery
(`test_protocol_access.py`, `test_shared_format.py`, `test_messages.py`).
These are host-side tests, not a PIC simulator.

## Related docs

- `docs/generic/navigation.md`, `commands.md`, `usage.md`, `edge-cases.md` —
  the user-facing prompt layer.
- `docs/generic/README.md#idle-mode-hiz` — HiZ behaviour.
- `docs/protocol/binary/README.md`, `docs/protocol/sump/README.md` — the two
  non-CLI entry paths and their reset/exit bytes.
- `docs/capabilities/auxiliary/README.md` — PWM, servo, frequency, AUX.
- `docs/capabilities/adc/README.md` — `d`/`D` probe front ends.
- `docs/capabilities/selftest/README.md` — the `~` self-test.
- `docs/capabilities/smps/README.md`, `docs/capabilities/lcd/README.md` —
  the remaining capability fronts.
- `docs/tools/basic/README.md` — the `(BASIC)` prompt and `EXIT`.
- `docs/dev/shared-machinery.md` — the shared accessor contract.
- [usb.md](usb.md), [bitbang.md](bitbang.md), [messages.md](messages.md),
  [eeprom.md](eeprom.md) — the sibling modules.
