# UART

**Status:** documented for v3 and v4; v3 hardware behaviour is unverified.

UART is asynchronous serial: start/stop-framed data on a data line.  This
mode is a full-duplex UART (**TX** + **RX** + ground), a half-duplex
**single-wire** UART (HDUART — the firmware's `Half (single wire)` duplex
option), a transparent USB↔serial bridge, a live monitor, and a binary
(`ART1`) sub-protocol.  One engine covers all of them; the terminal modes
share one command set, and the binary layer has its own.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | entering UART, the speed menu, custom baud, auto-baud, bits/parity/stop/polarity, output type, duplex, the settings header, the one-line form, `W` |
| [wiring.md](wiring.md) | full-duplex pins, the HDUART single wire and its **required** pull-up, open drain, VPU feeding, `v` pin names, voltage limit, v3/v4 |
| [terminal-usage.md](terminal-usage.md) | sending, `r` / `r:N` / `r;bits`, the live display `[`/`{` and `]`/`}`, the 127-word receive ring, examples |
| [hduart.md](hduart.md) | half-duplex single wire: selection, forced open drain, idle-high, no self-echo, speeds, auto-baud |
| [macros.md](macros.md) | the `(0)` list, the live monitor `(2)`, auto-baud `(4)` |
| [bridge.md](bridge.md) | the transparent bridge `(1)` and the flow-control bridge `(3)` |
| [binary.md](binary.md) | the `ART1` binary sub-protocol and a worked exchange |
| [errors.md](errors.md) | parity, framing and overrun reporting, failed reads |
| [edge-cases.md](edge-cases.md) | failure modes and known quirks |

## Reference index

| Topic | Where |
|---|---|
| speed menu, custom baud, MIDI | [entry-and-setup.md §The speed menu](entry-and-setup.md#the-speed-menu), [§Custom baud](entry-and-setup.md#custom-baud-option-10), [§MIDI](entry-and-setup.md#midi-31250-bps) |
| one-line form, settings header | [entry-and-setup.md §The one-line settings form](entry-and-setup.md#the-one-line-settings-form), [§The settings header](entry-and-setup.md#the-settings-header) |
| frame, polarity, output, duplex | [entry-and-setup.md §Frame and line settings](entry-and-setup.md#frame-and-line-settings) |
| failed reads | [errors.md §Failed reads](errors.md#failed-reads) |
| overruns and the MODE LED | [errors.md §Overruns](errors.md#overruns) |
| binary commands | [binary.md §Commands](binary.md#commands) |
| `0x0F` bridge trap | [binary.md §The `0x0F` bridge trap](binary.md#the-0x0f-bridge-trap) |

## Worked sessions

| Session | Where |
|---|---|
| menu entry and setup prompts | [entry-and-setup.md §Menu-form example](entry-and-setup.md#menu-form-example) |
| full-duplex loopback read/write | [terminal-usage.md §Example session](terminal-usage.md#example-session) |
| live display | [terminal-usage.md §The live display](terminal-usage.md#the-live-display) |
| HDUART selection and no self-echo | [hduart.md §No self-echo](hduart.md#no-self-echo--what-is-sent-vs-what-comes-back) |
| bridge prompt | [bridge.md §Transparent bridge](bridge.md#1--transparent-bridge) |
| binary exchange | [binary.md §Worked exchange](binary.md#worked-exchange) |

## The five-minute version

- Enter: `m` then `3`, or `m3`; answer the six prompts (speed, data
  bits/parity, stop bits, receive polarity, output type, duplex).  The speed
  prompt defaults to `1` = 300 bps — pick the speed explicitly.  `W` applies
  the settings and starts the supplies; the terminal port is off until a `W`
  succeeds.
- Wiring: **full duplex — TX on MOSI, RX on MISO, plus ground; cross the two
  data lines.**  The default output is **open drain**, so the TX line needs a
  pull-up unless you choose `2. Normal`.  For **half-duplex single wire** use
  **MOSI only plus a 1 kΩ–10 kΩ pull-up to the target's logic voltage
  (3.3 V typical) — the pull-up is required**; MISO is unused (see
  [hduart.md](hduart.md)).
- `P` enables the on-board pull-ups and needs a voltage on Vpullup (`e2` on
  v4); `p` disables them.  `w` turns the supplies off and disables the port.
- Send with `0xNN`, `0b…`, a decimal (`0`–`255`, or `0`–`511` in 9-bit
  mode), or `"text"`; `r` reads one value; `r:N` reads N.  `[`/`{` open the
  live display, `]`/`}` stop it; the receive ring keeps filling while UART is
  engaged.
- `(0)` opens the macro menu — see [macros.md](macros.md).  `(1)` is the
  transparent bridge, `(2)` the `Live monitor`, `(3)` `Bridge with flow
  control` (handshake relay on v3), `(4)` auto-baud.
- Failures are reported inline: `-f ` (framing), `-p ` (parity), and
  `FAILED, NO DATA` (nothing to read) — see [errors.md](errors.md).

## Availability (summary)

| Feature | v3 | v4 |
|---|---|---|
| build | present | present (default image) |
| full duplex | yes | yes |
| half-duplex single wire (HDUART) | yes | yes |
| presets 300–115200 | yes | yes |
| MIDI 31250 | raw BRG `127` / binary `0x69` | custom `31250` / binary `0x69` |
| custom baud (prompt) | raw BRG (1–32767) | bps (62–999999) |
| entry-time auto-baud (option `11`) | — | yes |
| auto-baud macro `(4)` | yes | yes |
| bridge `(1)` / monitor `(2)` | yes | yes |
| flow-control bridge `(3)` | CTS/RTS on CS/CLK | no flow control (= `(1)`) |
| bridge exit | hardware reset | NORMAL button |
| host link | FTDI; `b` sets the terminal rate | CDC; host-controlled (`b` is a no-op) |
| binary `ART1` | yes | yes |
| `0x5x` VPU feed (binary) | — | yes |
| 9-bit frames | yes | yes |

## Names used in this set

| Firmware form | Meaning |
|---|---|
| `UART` / `UART>` | terminal mode name / prompt |
| `ART1` | binary sub-protocol identifier returned by `0x01` (the UART binary layer) |
| `TxD` / `RxD` | the `v` column labels for MOSI / MISO |
| `Half (single wire)` | the duplex option called **HDUART** in this set |
| `Live monitor` | the `(2)` macro; the raw byte monitor |
| `Bridge with flow control` | the `(3)` macro; the flow-control bridge |
| `spd` `brg` `dbp` `sb` `rxp` `hiz` `dup` | settings-header fields |
| `Vpullup` (`Vpu`, `VPU`) | the pull-up reference pin: `Vpullup` in messages, `Vpu` in the `e` menu, `VPU` in `v` |

## Limits & out of scope

- No protocol above the byte layer: no MODBUS/AT command parsing, no
  address/checksum handling.  UART is a manual byte bus and a bridge.
- The half-duplex variant has **no collision detection**: words the peer
  sends while the Bus Pirate is transmitting are not kept.
- The receive path is a **127-usable-word ring**.  Sustained traffic faster
  than the host drains it fills the ring and drops words; outside a
  bridge/monitor the drop is silent (the counter is not printed) — see
  [errors.md](errors.md).
- The target UART has **no flow control**.  Macro `(3)` only relays the host
  serial handshake lines on v3; on v4 it is the plain bridge.
- MIDI is not a menu entry; 31250 bps is reachable through the binary speed
  operand or a custom baud value — see
  [entry-and-setup.md](entry-and-setup.md).
- The binary layer is **full duplex only** and its bulk writes are 8-bit; it
  does not inherit the terminal UART settings.
- `0x0F` is a terminal return only in `BBIO1`; inside `ART1` it is a one-way
  bridge (v4: NORMAL button; v3: reset).
- Shared prompt grammar (`r`, `W`, `P`, `(n)`, wrappers, bit ops, safe reset
  `#`, bootloader `$`) lives in
  [docs/generic/](../../generic/README.md) and is not repeated here.

## Related docs

- [generic/](../../generic/README.md) — prompt grammar, mode menu, `W`/`w`,
  `P`/`p`, `e`, `v`, value forms and display formats.
- [generic/usage.md](../../generic/usage.md) — a default UART entry session
  (setup, `W`, `P`, `v`; no bus traffic).
- [binary/](../binary/README.md) — BBIO1 entry, framing and the shared
  command groups.
- [SPEC.md](../../SPEC.md) — the function and capability index.
