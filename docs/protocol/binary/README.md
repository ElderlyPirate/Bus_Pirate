# Binary bitbang layer (BBIO1)

**Status:** v4 documented; v3 is unverified (built for v3, hardware not
verified).

The binary bitbang layer is the Bus Pirate's byte-oriented control layer: a host
program sends single request bytes and receives raw bytes instead of typing
terminal commands. It is entered from the terminal prompt with twenty NUL
bytes, announces itself as `BBIO1`, and gives direct control of the five header
I/O lines, the target supply, the pull-up resistors and the in-layer capability
commands (PWM, ADC, frequency, self-test, edge wait). It is also the gateway to
every binary sub-protocol: `SPI1`, `I2C1`, `ART1`, `1W01`, `RAW1`, `OCD1`,
`PIC1`, `XSV1`, and the AVRISP-MKII personality.

This README is a map, not the manual; the details live in the files below.

| File | Covers |
|---|---|
| [entering-and-framing.md](entering-and-framing.md) | entry, wiring, replies, framing, leaving |
| [commands.md](commands.md) | the BBIO1 byte map and its capability commands |
| [sub-protocols.md](sub-protocols.md) | entries, identifiers, shared groups, mode matrix |
| [examples.md](examples.md) | worked BBIO1 and I2C exchanges, mode skeleton |
| [edge-cases.md](edge-cases.md) | failure modes: trigger → symptom → recovery |

## The five-minute version

- Enter from the terminal prompt with **20 consecutive `0x00`** bytes; the
  layer replies `BBIO1` (five bytes, no line ending). A host program sends
  them — a terminal cannot type NUL.
- Replies are minimal, but **`0x01` is not always success and `0x00` is not
  always failure**: a reply can be a result byte, a data word, a count, a pin
  level or a bus ACK. Use the command's reply shape
  ([commands.md](commands.md#reply-shapes)). There are no line ends, length
  bytes or unsolicited messages; the host must know every reply length.
- In BBIO1 the request byte's top bits decide its class: `1xxxxxxx` is a
  **pin state** byte, `010xxxxx` a **pin direction** byte, everything else is
  a **command**.
- `0x00` re-prints `BBIO1` without resetting anything. `0x0F` replies `0x01`
  and returns to the terminal through a reset.
- Entry bytes: `0x01` SPI, `0x02` I2C, `0x03` UART, `0x04` 1-Wire, `0x05`
  raw wire, `0x06` OpenOCD, `0x07` PIC, `0x08`/`0x09` AVRISP-MKII (v4),
  `0x18` XSVF (v4). Every entry prints an identifier except `0x08`/`0x09`,
  which have none and return to `BBIO1` after the startup budget (about
  **120 s** from attach to the first complete non-empty request on USB, about
  **5 s** on serial) or after about **5 s** of idle once a complete non-empty
  request has arrived.  Identifiers and owners are in
  [sub-protocols.md](sub-protocols.md#entry-bytes).
- Wiring headline: **AUX, MOSI, CLK, MISO, CS and ground**, plus the **ADC**
  pin for the probe and **Vpullup** (`Vpu`) for the pull-up resistors. Entry
  leaves every line an input with the supply and the pull-ups off, and it also
  switches the on-board Vpullup source off. An open-drain bus needs the
  pull-up resistors **on** with a voltage on Vpullup; push-pull output needs
  neither.

## Names used in this set

| Firmware form | Meaning |
|---|---|
| `BBIO1` | binary bitbang identifier; the layer's name |
| `SPI1` `I2C1` `ART1` `1W01` `RAW1` `OCD1` `PIC1` `XSV1` | binary sub-protocol identifiers |
| `HiZ>` | terminal prompt after leaving the layer |
| `AUX` `CS` `AUX1` `AUX2` | header pins; the printed names used in this set |
| `Vpullup` (`Vpu`, `VPU`) | the pull-up reference pin; the `v` table calls the column `VPU` |
| `VREG` | the on-board regulator that feeds the target supply; terminal `W` switches the same element |
| terminal `SPI` `I2C` … | modes entered with `m`; different strings |

The terminal's selection tokens are `A0`, `CS`, `A1` and `A2` (v3 prints `aux`
for `A0`): `A0` is the pin printed `AUX`, `A1` is `AUX1`, `A2` is `AUX2` (v4
for the last two).  This set uses the printed pin names; the token form appears
only where the terminal selection itself is meant.

## Availability (summary)

| Group | v3 | v4 |
|---|---|---|
| layer entry, pin I/O, `0x00`, `0x0F` | present, unverified | present |
| SPI / I2C / UART / 1-Wire / raw wire / OCD / PIC entries | present, unverified | present |
| AVRISP-MKII entries (`0x08`, `0x09`) | — (`0x00`, then `BBIO1`) | present |
| XSVF player (`0x18`) | — (`0x00`, stays in BBIO1) | present |
| pull-up feed (`0x5x`, sub-protocol modes only) | — (`0x00`) | present |
| PWM, ADC, frequency, self-test, wait | present, unverified | present |
| SMPS group (`0xFx`, raw wire only) | — (`0x00`) | optional build; off in the default image |

The v3 features are in the build; the v3 path is source-derived and not
verified on hardware.

## Limits & out of scope

- This set owns the **byte layer**: entry, framing, reply and ACK rules, the
  BBIO1 command map and the cross-mode command groups. The capability sets own
  the units, ranges, wiring and limits of PWM, ADC, frequency, self-test and
  SMPS; each mode's `binary.md` owns that mode's command set.
- **No read-back of the configuration.** There is no "report settings" command;
  a host must track the pin directions, speed, output type and wire count it
  set.  There is also **no read-only five-pin read**: every state or direction
  byte is a write, so reading the levels means rewriting all five direction bits
  or all five latches (and, for a state byte, the supply and pull-up bits).  See
  [commands.md](commands.md#reading-pins).
- **No length, checksum or framing marker.** Lost or extra bytes silently shift
  every later reply; recovery is to return to `BBIO1` and re-enter.
- **One host at a time.** The binary layer takes over the link; terminal
  commands are not interpreted until `0x0F`.
- Building and flashing the firmware is a developer topic:
  [docs/dev](../../dev/README.md).
- The SUMP logic analyzer has its own entry sequence and is a separate set:
  [docs/protocol/sump](../sump/README.md).

## Related docs

- [generic/README.md](../../generic/README.md) — terminal prompt, mode menu,
  `W`/`P`/`e`, reset and the NUL/STX entry boundary.
- [generic/commands.md](../../generic/commands.md#leaving-the-cli--binary-and-sump-entry)
  §Leaving the CLI — binary and SUMP entry — the terminal side of the entry
  sequence.
- [SPEC.md](../../SPEC.md) — the function and capability matrix.

| Mode set | Binary page |
|---|---|
| SPI | [SPI1](../spi/binary.md) |
| I2C | [I2C1](../i2c/binary.md) |
| UART | [ART1](../uart/binary.md) |
| 1-Wire | [1W01](../wire/1wire/binary.md) |
| 2-Wire | [RAW1 specifics](../wire/2wire/binary.md) |
| 3-Wire | [RAW1 specifics](../wire/3wire/binary.md) |
| raw wire | [shared `RAW1`](../wire/binary.md) |
| JTAG (OpenOCD, XSVF) | [OCD1 and XSV1](../jtag/README.md) |
| PIC | [PIC1](../../programmers/pic/binary.md) |
| AVRISP-MKII | [0x08 / 0x09](../../programmers/avrisp/entry.md) |
| self-test | [0x10 / 0x11](../../capabilities/selftest/binary.md) |
| ADC | [0x14 / 0x15](../../capabilities/adc/binary.md) |
| PWM, frequency, edge wait | [AUX capability](../../capabilities/auxiliary/README.md) |
| SMPS | [0xFx](../../capabilities/smps/binary.md) |
