# I2C

**Status:** v3 and v4 documented; v3 is included in the build and its
behaviour is **not verified**.

I2C is a two-wire bus (SDA data, SCL clock) plus ground. This mode is a manual
master: it drives start/stop conditions, writes and reads bytes, manages the
ACK/NACK bit, scans for device addresses, sniffs traffic, and reaches the v4
on-board EEPROM. Two drivers are available: a software (bit-banged) driver on
both boards, and the on-chip hardware engine — standard on v4, optional on v3
and not in the default v3 build. A binary `I2C1` sub-protocol exposes the same
bus to a host program.

The set is written for someone wiring a target and driving it from the terminal
or a host program; it is not an I2C tutorial. It covers the two boards' deltas
and the failure modes, not device-specific protocols.

This README is a map, not the manual; the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | entry, driver and speed prompts, the one-line form, the header, bring-up with `W`/`e`/`P` |
| [speeds.md](speeds.md) | the software profiles and hardware rates, labels vs actual, binary operands |
| [wiring.md](wiring.md) | SDA/SCL pins, the mandatory pull-up, on-board values, Vpullup, voltage limit |
| [bus-operations.md](bus-operations.md) | start/stop, repeated start, write/read, the ACK/NACK model, the scanner, a worked session |
| [clock-stretching.md](clock-stretching.md) | the per-clock wait, the bound, `I2C bus error`, recovery, binary `0x0A` |
| [macros.md](macros.md) | the `(0)` list and macros 1–4; scanner format in bus-operations, sniffer in `sniffer.md` |
| [sniffer.md](sniffer.md) | macro `(2)`, output format, buffering and limits |
| [eeprom.md](eeprom.md) | the on-board 24xx64 geometry, write protect, and the Windows `I2CEEPROMWIN` tool |
| [binary.md](binary.md) | the `I2C1` sub-protocol, entry defaults, configuration, bulk, `0x08`, a worked exchange |
| [edge-cases.md](edge-cases.md) | trigger/symptom/recovery tables for every topic |

## The five-minute version

- **Enter:** `m4` (or `m` then `4`). On v4 choose **1 Software** or
  **2 Hardware**, then a speed; pressing Enter twice selects
  Software `~6kHz`. A default v3 build skips the driver prompt.
- **Wiring:** SDA on **MOSI**, SCL on **CLK**, common ground. **Pull-ups on
  both lines are mandatory — 2 kΩ to 10 kΩ to Vpullup.** Without them the
  scanner reports a device at every address.
- **Bring-up:** `W` powers the target rails; skip it for a self-powered target,
  where `e`/`P` still work. On v4 select the Vpullup source first (`e2` 3.3 V or
  `e3` 5 V), then enable the pull-ups with `P`. Match VPU to the target's logic
  level. `P` is enough at the slow profiles; for 400 kHz/1 MHz add a
  1.5–2.2 kΩ external pull-up on SCL.
- **Start/stop:** `[` starts, `]` stops; `{`/`}` are the same.
- **Write:** `0xNN`, `0b…` or a decimal writes a byte and prints the slave's
  ACK.
- **Read:** `r` reads a byte; its ninth bit is deferred to the next bus
  operation — a read/write sends ACK, a start/stop sends NACK.
- **Repeated start:** a second `[` before the stop.
- **Macros:** `(1)` scans addresses, `(2)` sniffs, `(0)` lists them. `(3)` and
  `(4)` drive the on-board EEPROM (v4); run `(3)` before `(4)`. Both require
  **2 Hardware**; from Software `(3)` prints
  `Onboard EEPROM requires hardware I2C` and `(4)` is silent.
- **Limits:** clock stretching is supported in both drivers with a bounded wait
  (about 25 ms); a timeout prints `I2C bus error`, releases both pins and
  abandons the command. The binary layer is software-only; `0x0A` reports the
  last bus status.

## Availability (summary)

| Item | v3 | v4 |
|---|---|---|
| build | included | included |
| terminal mode (`m4`) | yes | yes |
| software driver | yes | yes |
| hardware engine | optional build, not the default | yes, always offered |
| clock stretching | yes | yes |
| address scanner / sniffer | yes | yes |
| on-board EEPROM macros 3/4 | no | yes (**not verified**) |
| binary `I2C1` | yes | yes |
| binary uses the hardware engine | no | no |
| terminal output buffer | 4096 B | 4096 B |

**Hardware:** pull-ups on SDA and SCL. Both boards have switched on-board
pull-ups; v4 can also feed Vpullup from its on-board 3.3 V or 5 V source. v3
needs an external 0–5 V feed on Vpullup; an external pull-up is optional. The
on-board EEPROM macros need a v4 with a populated chip.

## Names used in this set

| Firmware form | Meaning |
|---|---|
| `I2C` / `I2C>` | terminal mode name / prompt |
| `I2C1` | binary sub-protocol identifier returned by `0x02` / `0x01` |
| `Software` / `Hardware` | the bit-banged driver / the on-chip engine |
| `mod` / `spd` | settings-header fields; both print **zero-based** |
| `Vpullup` (`Vpu`, `VPU`) | pull-up reference: the `Vpu` header pin, the `VPU` column in `v` |
| `SDA` / `SCL` | bus line names; the header pins are MOSI / CLK |
| `Enable Writing the on-board EEPROM` | macro 4's menu text; the effect is clearing the write protect |

The settings header is `I2C (mod spd)=( 0 0 )`; the fields are explained in
[entry-and-setup.md](entry-and-setup.md#the-settings-header).

## Buffers

The *terminal output buffer* is the 4096-byte buffer used for output and for
the binary write-then-read payload; it is the same on v3 and v4. The *command
buffer* is the input line, 128 bytes on v3 and 256 on v4. They are separate
buffers; the binary counts are limited by the terminal output buffer, not the
command line.

## Limits & out of scope

- **No generic device layer** (no EEPROM page-write helper, no register
  decode). The only chip-specific support is the on-board EEPROM macros 3/4
  (v4) and the external EEPROM tool.
- **Not a multi-master arbitrator.** The hardware bus-clear is a single-master
  recovery sequence; it cannot resolve two masters driving the bus.
- The speed labels are **nominal**. The actual SCL is lower, increasingly so at
  the fast settings, and depends on the bus rise time and load. Exact rates on a
  given board are **unverified** — see [speeds.md](speeds.md).
- The on-board EEPROM macros (v4) are **not verified**; they need a populated
  board.
- The sniffer's maximum rate depends on the traffic, the build and the host
  link, and is **unverified** — see [sniffer.md](sniffer.md).
- Shared prompt grammar (`r`, `:N`, `;N`, `W`/`w`, `P`/`p`, `e`, `v`, `o`,
  `l`/`L`, `(n)`, wrappers, `#`, `$`) lives in
  [docs/generic/](../../generic/README.md) and is not repeated here.

## Related docs

- [generic/](../../generic/README.md) — prompt grammar, `W`/`w`, `P`/`p`, `e`,
  `v`, reset and bootloader.
- [binary/](../binary/README.md) — BBIO1 entry, framing and the shared groups.
- [SPEC.md](../../SPEC.md) — the function and capability index.
- [BASIC bus access](../../tools/basic/bus-access.md) — the `MODE` statement,
  which applies a mode's defaults without the terminal `W` step.
