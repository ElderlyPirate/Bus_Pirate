# UART — round-1 fixes record

Worktree: `C:\Users\jschl\dev\bpv4-docssweep`, branch `docs/combined-sweep`
(combined firmware `c57e223`; docs tip `bd8d2d6`).  Edits are confined to
`docs/protocol/uart/**`.  No firmware change, no WORKLOG change, no commit.

The current working-copy UART set (not the committed pre-sweep text) was
treated as the audit target.  Every claim below was checked against
`Firmware/protocol_uart/uart.c`, `uart2.c` / `uart2.h`,
`Firmware/protocol_binary/binary_io.c`, and the core/menu/message sources.

**Summary:** 15 findings — 15 fixed, 0 rejected, 0 code-owner decisions,
0 cross-set.  Two separate code-owner candidates are recorded below; no
firmware was changed.

## Findings

| # | Decision | Change (file + short quote) | Evidence |
|---|---|---|---|
| 1 | fixed | `terminal-usage.md` §Example session: "A full-duplex loopback … normal output (the looped line needs no pull-up)" and `HiZ>m3 5 1 1 1 2 1` → header `… 0 0 0 )` (`hiz 0`) | `uart_setup_prepare()` maps `output 2` to normal output; `UARTsettings()` prints the effective `hiz`.  Open drain with no pull-up leaves the looped line without a driven idle high, so the old `hiz 1` example had no start bit. |
| 2 | fixed | `binary.md` §Entry and exit: "on **v3** there is no feed command — feed `Vpu` externally and enable power + pull-ups, or select normal output"; new bring-up table in §Worked exchange (`0x51`+`0x4C` for v4; external `Vpu`+`0x4C`, or `0x90`, for v3); `edge-cases.md` binary bring-up row expanded | `binUART()` handles `0x5x` only under `BUSPIRATEV4`; on v3 the byte falls to `default: REPORT_IO_FAILURE()` (`0x00`).  `bp_binary_io_peripherals_set()`: bit 3 = regulator, bit 2 = pull-ups, so `0x4C` enables both.  `0x8x`/`0x9x` bit 4 selects normal output on both builds. |
| 3 | fixed | `bridge.md` §Exiting: "On **v4** the exit prints a blank line and the `UART>` prompt returns… On **v3** the reset reboots the board to `HiZ>`: the UART settings and any queued words are lost." | The v4 bridge loop breaks on `BP_BUTTON_ISDOWN()`; the v3 loop has no break.  The macro caller prints `bpBR` after `run_macro()`, hence the v4 blank line.  A v3 reset restarts the firmware at `HiZ>` with RAM state gone. |
| 4 | fixed | `errors.md` §Overruns: "**The count is internal: no command or status field prints it.**"; LED identified as "the LED the firmware turns on when a non-HiZ mode is entered (not the USB LED)"; v3 terminal-UART overrun added | `uart2_rx_overrun_count()` is used only for comparisons in `uart.c`; no message or status field prints it.  `hardware.h` drives `BP_LEDMODE`; `changemode()` turns it on for a non-HiZ mode.  The v3 bridge also clears `U1STAbits.OERR` and calls `bp_disable_mode_led()`. |
| 5 | fixed | `terminal-usage.md` §Sending: "A `;N` suffix after a value sets that value's width and stays in force for later values until changed — for example `0x1FF;9`" | The parser order is value, `:N`, `;N`; `docs/generic/values.md` §Width states the same rule and that the width persists.  The old wording described a prefix. |
| 6 | fixed | `bridge.md` §Host-side baud: "The Bus Pirate's side runs at its **terminal** rate… the host must open the virtual serial port at the same rate"; "`b` changes the **Bus Pirate** side, not the computer side"; link corrected to `navigation.md`; the README and bridge v3/v4 rows now say "`b` sets the terminal/Bus Pirate rate" | `set_baud_rate()` on v3 writes `U1BRG`, prints `Adjust your terminal` / `Space to continue`, then waits for a space at the new rate; on v4 it prints the CDC host-controlled notice.  `main.c` starts `terminal_speed = 8` (the 115200 preset, divider 34). |
| 7 | fixed | `README.md` status is now one line: "documented for v3 and v4; v3 hardware behaviour is unverified."  Removed the eight "v3 column is not verified" footers and `edge-cases.md` §Unverified paths; removed "build-only" from the build and auto-baud macro rows | `macros.md` and `uart.c` show the `(4)` macro is common code; only the entry-time `11` option and bps custom form are v4-only.  The template allows a short inline caveat; the `-p` rendering and 5.5 V caveats were kept where they change a step/limit. |
| 8 | fixed | `README.md` five-minute bullet now uses "`Live monitor`" and "`Bridge with flow control`"; names table adds `Live monitor` and `Bridge with flow control`; all "raw monitor" / "raw live monitor" prose replaced | Firmware macro strings are `2.Live monitor` and `3.Bridge with flow control` in both `messages_v3.s` and `messages_v4.s`. |
| 9 | fixed | `entry-and-setup.md` one-line form: "If any **required** field (through `output`) is missing or out of range…" | `uart_setup_prepare()` reads `hd = cmdbuf[cmdstart] == 0 ? 1 : getint()`, so an omitted `duplex` means full; an invalid `hd` sets `speed = 0` and falls back to the prompts. |
| 10 | fixed | `entry-and-setup.md` `spd` row: "`9` is custom baud or an auto-baud result"; added menu-number → one-line → header table (`1→1→0`, `5→5→4`, `9→9→8`, `10→10+brg→9`); removed `256000` from the `Calculated:` list | `UARTsettings()` prints the zero-based `mode_configuration.speed`; entry-time auto-baud sets `speed = 9` and `U2BRG`.  `uart_get_baud_rate()` prints `Estimated:`/`Calculated:` only when `detected <= 150000`, so `Calculated:` cannot print 256000. |
| 11 | fixed | `wiring.md`, `hduart.md`, `README.md`, `edge-cases.md`: "A pull-up to the target's logic voltage (3.3 V typical) is required"; removed "stated explicitly" and the fixed "1 kΩ to 3.3 V" wording | The output-type prompt defines open drain as `H=Hi-Z, L=GND`; the high comes from the pull-up reference, which the wiring page already says must be the target's logic supply.  A 3.3 V-only requirement is wrong for 5 V and 1.8 V targets. |
| 12 | fixed | `wiring.md` §Pick your wiring: new paragraph "A **non-self-powered target** also needs its supply: connect the target's **VCC** to the Bus Pirate's **3.3 V** or **5.0 V** header pin and share **GND**; `W` switches that rail on.  Vpullup is a separate reference and does not power the target."; "three required signal wires" | `switch_psu_on()` enables the regulator after checking both rails, then runs `setup_execute()`; Vpullup is a separate pull-up reference (`wiring.md` §Feeding the pull-up voltage). |
| 13 | fixed | `README.md` new §Reference index (deep links to speed menu, custom baud, one-line form, header, frame, failed reads, overruns, binary commands, `0x0F` trap) and §Worked sessions; Limits bullet "`0x0F` is a terminal return only in `BBIO1`…"; `binary.md` top warning + `### The 0x0F bridge trap`; Related-docs `usage.md` described as setup-only | All in-set anchors verified with a slug check.  `binUART()` case `0x0F` returns `0x01`, then loops as a byte bridge; v4 `BP_BUTTON_ISDOWN()` breaks to the `ART1` loop, v3 has no software exit.  `generic/usage.md` is an entry/`W`/`P`/`v` session with no bus traffic. |
| 14 | fixed | Full receive-ring description kept only in `terminal-usage.md` §Receive buffer and overruns; `errors.md` §Overruns now opens "The [receive ring](terminal-usage.md#receive-buffer-and-overruns) stores each word…"; `hduart.md` §Behaviour of this engine links to terminal-usage and errors | `uart2.c` defines one 128-slot ring with 127 usable entries, a four-deep hardware FIFO, and per-word flags; repeating the paragraph in three files risked drift. |
| 15 | fixed | `hduart.md` no-self-echo window converted to a Moment/What-happens table; `entry-and-setup.md` entry-time auto-baud caveats converted to a Condition/Result table; `terminal-usage.md` live-display bullets converted to a Point/Behaviour table | `uart2_tx()` drains and keeps pre-byte words, discards during the byte, drains again after `TRMT`; `uart_get_baud_rate()` handles the five-second window, early exit and timer-busy cases; `uart_start()`/`uart_stop()` and `uart_periodic_callback()` implement the display rules. |

### Extra corrections made while verifying

- `entry-and-setup.md` and `macros.md`: removed `256000` from the rates that
  `Calculated:` "can report".  The measurement routine prints the overflow
  message and suppresses both `Estimated:` and `Calculated:` when the result
  is above 150000.  The entry-time path can still *select* 128000 or 256000
  from the wider table; the `(4)` macro only reports and cannot print 256000.
- `edge-cases.md` full-duplex wiring row now says a loopback also needs
  normal output or a pull-up, matching finding 1.
- `bridge.md` and `errors.md` now say the v3 terminal bridge also turns the
  MODE LED off for its own `U1STAbits.OERR` overrun, not only for the UART
  receive counter.
- `wiring.md` §Voltage limit no longer says "The UART pins are 5 V logic"
  (readable as a 5 V output); it now says the data pins are 3.3 V outputs in
  Normal output and that the open-drain high is the pull-up voltage.

## Code-owner candidates

No finding required a code change.  The writer's candidate list is classified
below; the two entries marked "candidate" are the ones the findings touch and
the code owner may want to review.

| Candidate | Touched by | Verdict / evidence |
|---|---|---|
| binary `0x07` divider `0` | 13 (binary command index) | **Not a defect.**  `binUART()` accepts the raw divider as given; `U2BRG = 0` gives `FCY / 4 / (0 + 1)` = 4 Mbaud at 16 MHz.  `binary.md` and `edge-cases.md` document it as "used as given". |
| auto-baud threshold | 10, 15 | **Code-owner candidate (message text).**  `uart_get_baud_rate()` checks `detected > 150000` but prints `MSG_UART_BAUD_OVERFLOW`, whose text says "cannot measure above 16000000".  The docs explain the mismatch; the message is misleading and could be corrected by the code owner. |
| `READ: FAILED, NO DATA0x00` | none (audit "what to keep") | **Documented rendering artifact, not raised as a defect.**  `MSG_UART_FAILED_NO_DATA` is emitted with `bp_message_write_buffer` (no terminator) and `uart_read()` returns 0, so the caller's value prints against the token.  `errors.md` and `terminal-usage.md` explain it. |
| flag placement | none | **Not a defect.**  `uart_read()` prints the flags before returning the value; `uart_periodic_callback()` prints the value then the flags.  `errors.md` documents both placements. |
| stale ring after `w` | 3, 14 | **Code-owner candidate (stale RX).**  `uart2_disable()` stops the port but leaves `uart2_rx_head` / `uart2_rx_tail`; `uart2_enable()` resets them.  A word queued before `w` can therefore still be returned by `r` while the port is disabled.  `entry-and-setup.md` and `edge-cases.md` document the behaviour. |
| `ART1` vs `UART1` | 2, 13 | **Not a defect.**  `MSG_UART_MODE_IDENTIFIER` is `"ART1"` in both `messages_v3.s` and `messages_v4.s`; the binary layer uses `ART1` consistently. |
| v3 flow-control direction | 3, 6 | **Not a defect.**  `uart.c` sets `BP_CS_DIR = INPUT`, `BP_CLK_DIR = OUTPUT`, `FTDI_CTS_DIR = OUTPUT`, `FTDI_RTS_DIR = INPUT` and relays `FTDI_CTS = BP_CS`, `BP_CLK = FTDI_RTS`.  The bridge/wiring tables match: CS is the target-side CTS input, CLK is the host-side RTS output. |

## Cross-set changes

None required by these findings.  The generic `;N` grammar, the `b` command
section, and the binary shared-group anchor already exist and are correct.
One optional cleanup for the main session, not needed for this set:
`docs/SPEC.md` still carries a question mark in the auto-baud row ("settings
field + bps form v4-only ?"); the UART set resolves the behaviour (macro `(4)`
common, entry-time `11` and bps custom form v4-only).

## Remaining questions

- The v3 FTDI host-rate description is now consistent with the firmware side
  (`b` writes `U1BRG`; the host must match the terminal rate).  The FTDI
  EEPROM/host line-coding interaction is not in the repository; the wording
  avoids claiming a specific chip configuration.
- The `-p` parity-flag rendering remains marked "not verified" in
  `errors.md`; no code path was exercised to confirm it.
- The exact 5.5 V board rating remains marked unverified in `wiring.md`; no
  board-level source in the repository states it.
- The 115200 / 16 MHz-peer ~9 % mismatch claim is unchanged and remains a
  hardware-dependent statement.
- Not implemented: the optional symptom index at the top of `edge-cases.md`
  (structural proposal 6).  Its trigger → symptom → recovery tables already
  carry the same information, and finding 13's navigation request is covered
  by the README reference index and worked-sessions table.
