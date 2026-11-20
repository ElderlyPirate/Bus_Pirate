# I2C

**Status:** current · a manual two-wire master in the terminal, a binary
`I2C1` host interface, the v4 on-board EEPROM, and the `bp eeprom` host
frontend for external EEPROMs.

I2C is a two-wire bus (SDA data, SCL clock) plus ground. This mode is a manual
master: it drives start and stop conditions, writes and reads bytes, manages
the ACK/NACK bit, scans for addresses, sniffs traffic, and reaches the v4
on-board EEPROM. Two drivers are available: a software (bit-banged) driver on
both boards, and the on-chip hardware engine on v4. A binary `I2C1`
sub-protocol exposes the same bus to a host program.

The set is written for someone wiring a target and driving it from the
terminal or a host program; it is not an I2C tutorial. It covers the board
deltas and the failure modes, not device-specific protocols.

This README is a map, not the manual; the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | entry, driver and speed prompts, the one-line form, the header, bring-up |
| [wiring.md](wiring.md) | SDA/SCL pins, the mandatory pull-up, on-board values, Vpullup, voltage limits |
| [speeds.md](speeds.md) | software profiles, hardware rates, labels vs actual, binary operands |
| [bus-operations.md](bus-operations.md) | **command reference**, start/stop, repeated start, write/read, ACK model, scanner, worked session |
| [clock-stretching.md](clock-stretching.md) | the per-clock wait, the bound, `I2C bus error`, recovery, binary `0x0A` |
| [macros.md](macros.md) | the `(0)` list and macros 1–4; scanner and sniffer details are linked |
| [sniffer.md](sniffer.md) | macro `(2)`, output format, buffering and limits |
| [onboard-eeprom.md](onboard-eeprom.md) | the v4 24xx64, macros 3/4, write protect and the on-board bus |
| [host-eeprom.md](host-eeprom.md) | `bp eeprom read` / `write` / `verify` for an external EEPROM |
| [binary.md](binary.md) | the `I2C1` sub-protocol, command set, `0x08`, a worked exchange |
| [edge-cases.md](edge-cases.md) | one trigger/symptom/recovery table per topic |

## The five-minute version

- **Enter:** `m4` (or `m` then `4`). On v4 choose **1 Software** or
  **2 Hardware**, then a speed; pressing Enter twice selects Software
  `~6kHz`. A default v3 build skips the driver prompt.
- **Wiring:** SDA on **MOSI**, SCL on **CLK**, common ground. **Pull-up
  2 kΩ–10 kΩ to Vpullup is required on both lines.** Without it the address
  scanner reports a device at every address.
- **Power:** `W` puts the Bus Pirate's target rails on. For a self-powered
  target, skip `W`; the bus still needs pull-ups (the target's own may be
  enough). On v4 select the Vpullup source first (`e2` 3.3 V or `e3` 5 V), then
  `P` enables the on-board pull-up resistors. Match VPU to the target's logic
  level.
- **Drive:** `[` starts, `]` stops; a number writes one byte; `r` reads one
  byte. A read's ninth bit is deferred to the next bus operation.
- **Macros:** `(1)` scans addresses, `(2)` sniffs, `(0)` lists them. On v4
  `(3)` then `(4)` reach the on-board EEPROM; both require **2 Hardware**.
- **Host:** `bp eeprom read` / `write` / `verify` move an external EEPROM
  image as text hex. For on-board EEPROMs use the terminal macros.

## Availability (summary)

| Item | v3 | v4 |
|---|---|---|
| build | included | included |
| terminal mode (`m4`) | yes | yes |
| software driver | yes | yes |
| hardware engine | not in this tree | yes, always offered |
| on-board EEPROM macros 3/4 | no | yes |
| binary `I2C1` | yes | yes |
| binary uses the hardware engine | no | no |
| `bp eeprom` (external bus) | yes | yes |

**v3:** the mode and the software driver are in the default build. The
hardware engine is v4-only in this tree; v3 timing and the v3-only wiring
deltas are **not verified on v3**. If a v3 difference matters, use Software.

## Names used in this set

| Name / form | Meaning |
|---|---|
| `I2C` / `I2C>` | terminal mode name / prompt |
| `I2C1` | binary sub-protocol identifier returned by `0x02` / `0x01` |
| `Software` / `Hardware` | the bit-banged driver / the on-chip engine |
| `mod` / `spd` | settings-header fields; both print **zero-based** |
| `Vpullup` (`Vpu`, `VPU`) | pull-up reference: the `Vpu` header pin, the `VPU` column in `v` |
| `SDA` / `SCL` | bus line names; the header pins are MOSI / CLK |
| `1.7bit address search` | macro 1's menu text; it probes every **8-bit** address byte |
| `Enable Writing the on-board EEPROM` | macro 4's menu text; the effect is clearing write protect |
| `on-board EEPROM I2C interface` | the internal bus reached by macro 3 |
| `bp eeprom` | the host frontend for an **external** EEPROM |

The settings header is `I2C (mod spd)=( 0 0 )`; the fields are explained in
[entry-and-setup.md](entry-and-setup.md#the-settings-header).

## Limits & out of scope

- **No generic device layer** (no EEPROM page-write helper, no register
  decode). The only chip-specific support is the v4 on-board EEPROM macros and
  the `bp eeprom` host frontend.
- **Not a multi-master arbitrator.** The hardware bus-clear is a single-master
  recovery sequence; it cannot resolve two masters driving the bus.
- The speed labels are **nominal**. The actual SCL is lower, increasingly so at
  the fast settings, and depends on the bus rise time and load. Exact rates on a
  given board are **unverified** — see [speeds.md](speeds.md).
- Clock stretching has a bounded wait (a poll count, not a wall-clock
  guarantee). See [clock-stretching.md](clock-stretching.md).
- The sniffer's maximum rate depends on the traffic, the build and the host
  link, and is **unverified** — see [sniffer.md](sniffer.md).
- The on-board EEPROM is **v4 only**. Its 8 KB array is opaque storage; writing
  it does not change the firmware configuration.
- The `bp eeprom` frontend drives the **external** bus only; it cannot reach
  the on-board chip.
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
