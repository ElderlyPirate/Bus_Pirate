# Final cross-set cleanup — record

Worktree `bpv4-docssweep`, branch `docs/combined-sweep`, tip `1b37007`
(rebased onto combined `c57e223`).  No commit or push.  Each item was checked
against the accepted set and, where behaviour was in question, against the
combined firmware.

| # | Item | File | Fix |
|---:|---|---|---|
| 1 | `0x5x` refusal reasons | `protocol/wire/binary.md` | Added SMPS feedback-loop ADC ownership to the refusal list, matching `bp_binary_io_pullup_control()` / `bp_adc_foreground_available()`. |
| 2 | `0x6x` CS level | `protocol/wire/binary.md` | "drives CS high" → sets CS high, driven high in normal output and released in open drain (`bitbang_set_cs(HIGH)`). |
| 3 | `0x8x` CS level | `protocol/wire/binary.md` | Same qualification for the configuration command. |
| 4 | RAW1 edge-case CS claim | `protocol/wire/binary.md` | `0x8x`/`0x6x` "drive CS high" → set CS high with the open-drain release noted. |
| 5 | 2-Wire binary CS claim | `protocol/wire/2wire/binary.md` | Same correction; prevents the per-wire page contradicting the shared page. |
| 6 | 3-Wire binary CS claim | `protocol/wire/3wire/binary.md` | Same correction. |
| 7 | OCD1 byte collision | `protocol/binary/commands.md` | `0x04` is the TRST feature byte (`0x02 0x04 action`), not the TAP shift; `0x05` is the shift. |
| 8 | `0x16` generator wording | `protocol/binary/commands.md` | Stops the terminal generator (PWM or servo), not only a PWM. |
| 9 | `0x16` generator wording | `protocol/binary/edge-cases.md` | Heading and body now say PWM or servo; recovery names `S` for a servo. |
| 10 | XSVF stall landings | `protocol/binary/edge-cases.md` | Added the ordered results: `0x01` boundary, `0x04` command byte, `0x05`/`0x06` state/size. |
| 11 | XSVF entry wording | `protocol/binary/entering-and-framing.md` | Entry skips the shared reset, configures only TDI/TDO/TCK/TMS, switches VREG on; other pins and pull-ups untouched. |
| 12 | XSVF entry wording | `protocol/binary/sub-protocols.md` | Same correction in the entry notes. |
| 13 | OpenOCD link/label | `protocol/binary/sub-protocols.md` | `0x06` row now labels the firmware path and links the host adapter (`tools/openocd/`). |
| 14 | XSVF result order | `protocol/jtag/xsvf.md` | `0x05` (TAP state) before `0x06` (shift size), matching the result table and tool set. |
| 15 | `0x01` result parenthetical | `protocol/jtag/xsvf.md` | "stall inside a command" → stall seen at the next command boundary. |
| 16 | `AUX0` in JTAG entry | `protocol/jtag/entry.md` | SRST (`AUX0`) → SRST (`AUX`); `AUX0` is the firmware symbol, not the terminal pin name. |
| 17 | AUX-target reset sources | `protocol/dio/aux-pin.md` | Added the binary return (`0x0F`) to the resets that restore `A0`. |
| 18 | `i` at HiZ | `protocol/dio/aux-pin.md` | Noted that `i` prints only the banner in HiZ; enter DIO to see the selection line. |
| 19 | SMPS start example | `capabilities/selftest/checks.md` | `0xF2 0x58` → `0xF2 0x00` (lowest reachable request, `0x200`), matching the SMPS encoding. |
| 20 | SMPS start example | `capabilities/selftest/binary.md` | Same example correction. |
| 21 | XSVF firmware link | `generic/README.md` | XSVF row now links the JTAG player page as well as the tool set. |
| 22 | Width example caveat | `generic/values.md` | `0x55;4` is width-aware only where the mode clocks the width; byte modes send the low 8 bits. |
| 23 | Terminal buffer naming | `core/system.md` | Size table row → terminal output buffer. |
| 24 | Setup input wording | `core/system.md` | `setup_defaults_mode` avoids consuming terminal input, not "touching the terminal buffer". |
| 25 | `AUX0` in core | `core/system.md` | Bare `AUX0` → `AUX` (`BP_AUX0`) in the board-reset step. |
| 26 | DIO cleanup exception | `core/system.md` | Noted strong `dio_cleanup()` (five pins, bias, latches, 8-bit width) as the exception to "cleanup = 8-bit reset". |
| 27 | Terminal buffer naming | `protocol/uart/terminal-usage.md` | Two mentions → terminal output buffer. |
| 28 | XSVF verification basis | `dev/verification.md` | Added bounded chain scan and standalone `-x`/`-r` to the source-only note. |
| 29 | Terminal buffer naming | `protocol/i2c/README.md` | "terminal buffer" → terminal output buffer. |
| 30 | LCD `AUX0` mapping | `capabilities/lcd/README.md` | Defined `AUX0` as the v4b adapter jumper label for the Bus Pirate `AUX` pin (board label kept). |
| 31 | `P` push-pull advice | `generic/edge-cases.md` | Noted DIO has no output-type choice (push-pull only), so "choose open drain" does not apply there. |
| 32 | `e` push-pull advice | `generic/edge-cases.md` | Same DIO qualifier on the `e` row. |
| 33 | v3 `v` default names | `generic/usage.md` | Named the v3 HiZ signal order (`CLK`, `MOSI`, `CS`, `MISO`) as well as the v4 order, matching the v3 column table and the 3-Wire/LCD/PIC pages. |

## Verified, no change

- **SUMP exit and board reset** — the §2F note said SUMP returns without the
  board reset.  The combined tree calls `reset_terminal_state()` after
  `enter_sump_mode()` returns (`handle_left_arrow()`), so the existing reset
  claims in `generic/commands.md`, `sump/entry.md` and the AUX set are correct.
- **One-line `g` range** — the §2F note said 1–3999 kHz; `bp_pwm_setup()` and
  the prompt both accept 1–4000, and the docs already say 1–4000.
- **I2C `0x09` text injection** — already documented in `i2c/binary.md`
  (`AUX LOW`/`AUX HIGH`/`AUX INPUT/HI-Z` between the ack and sub-result), and
  the combined handler now sets the unknown sub-result to `0x00`.
- **Pending working-tree edits** — `protocol/jtag/edge-cases.md` entry state,
  `protocol/jtag/xsvf.md` stock-host exit/regulator wording, and the
  `programmers/avrisp/license.md` set intro were checked against the firmware
  and the tool set; kept.
- **Mode numbers** — default v4 8 KEYB / 9 PIC / 10 DIO and default v3
  8 PIC / 9 DIO are consistent across SPEC, generic, DIO, KEYB, PIC, BASIC
  and the dev pages.
- **Code map** — the JTAG/XSVF, keyboard, AUX, DIO and UART symbol additions
  from §2F are already present; no further file/symbol drift found.

## Result

**Consistent.**  33 fixes across 23 files.  Every relative link in `docs/`
resolves (199 pages), no TABs.  No unexplained `AUX0` remains: the only user
form left is the LCD adapter's v4b jumper label, now defined against `AUX`; no
bare "terminal buffer" remains.  No unresolved contradiction; the stale §2F
entries above are recorded rather than applied.
