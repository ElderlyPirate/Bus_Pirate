# Self-test

**Scope:** the Bus Pirate's own board check, run from HiZ with `~` or from the
binary bitbang layer with `0x10` (short) or `0x11` (full).  v4 is documented;
v3 is covered but not verified on v3 hardware.  The reader is a Bus Pirate
user running the check or a host author scripting it.

The self-test drives the board's own control lines, the `AUX` pin, on-board
EEPROM (v4), supply rails, ADC input and four I/O pins, and prints `OK` or
`FAIL` for each check.  It is a bring-up and fault-finding check for the Bus
Pirate itself, not a test of a target or of a bus protocol.  It is a
capability, not a mode or a tool: it takes over the pins and rails for the
run and returns the board to HiZ at the end.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [wiring.md](wiring.md) | the one or two jumpers, pull-up and supply needs, target disconnect, post-run state |
| [terminal.md](terminal.md) | `~`: entry, the setup prompt, the v4 full run and v3 order, reading a failure |
| [checks.md](checks.md) | every check, the expected pin and rail, the `0x10` split, v3/v4, the SMPS refusal |
| [binary.md](binary.md) | `0x10` / `0x11`: entry, the count byte, the idle loop, worked exchanges |
| [edge-cases.md](edge-cases.md) | concrete failure and recovery cases |

## The five-minute version

- Terminal: at `HiZ>` type `~`.  It always runs the full test.  In any other
  mode it prints `Self-test in HiZ mode only` and does nothing.
- Disconnect every target.  Fit `ADC`→`+3.3V` on v4; on v3 fit both that and
  `Vpu`→`+5V`.  No external pull-up is needed.  Send any byte (the prompt
  says Space).
- Every check line ends `OK` or `FAIL`; a measurement check adds its reading
  as `name(v.vv)`.  The run ends `Found <n> errors.` — `0` is clean.
- Binary: enter `BBIO1` with 20 × `0x00`, then send `0x10` (short, no
  jumpers) or `0x11` (full, jumpers).  `0x11` needs the jumpers above; both
  need every target disconnected and the board powered.
- Binary reply: one byte, the error count; `0x00` is clean.  Send `0xFF` to
  leave the test (reply `0x01`); the host is still in `BBIO1`.  Send `0x0F`
  to return to the terminal (reply `0x01`, then the banner and `HiZ>`).
- The run switches the board's rails and pull-ups itself and needs no
  external supply.  It leaves the pins in HiZ; the terminal run returns to
  `HiZ>`, while a binary session stays in `BBIO1` until `0x0F`.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| terminal `~` | present, not verified on v3 | present |
| binary `0x10` / `0x11` | present, not verified on v3 | present |
| on-board EEPROM checks | — | yes |
| `Vusb`, `5V VPU`, `3.3V VPU` | — | yes |
| `VPU` jumper check | yes | — |
| setup jumpers | `Vpu`→`+5V`, `ADC`→`+3.3V` | `ADC`→`+3.3V` |
| checks, full / short | 21 / 15 | 27 / 22 |
| SMPS-owned ADC | — | optional build; refuses the run |

The per-check table is in [checks.md](checks.md); the wiring table is in
[wiring.md](wiring.md).

## Limits & out of scope

- Board only: it does not test a target, a bus protocol, or any mode's
  command set.  A clean run does not prove that SPI, I2C or another mode
  works with a device.
- It does not test the USB data path, the bootloader, the v4 NORMAL
  (bootloader) button, or the `AUX1` / `AUX2` pins, and it does not test the
  whole on-board EEPROM: `READ&WRITE` uses one byte.
- The voltage checks are coarse in-band tests (about ±0.5 V), not
  calibration; there is no timing, current or signal-integrity test.
- The `MODE LED` check tests the drive line.  The end-of-run LED prompt is a
  visual inspection and adds nothing to the count.
- `~` is HiZ-only.  The binary bytes run from `BBIO1` and do not check the
  terminal mode; enter from HiZ for a clean baseline.
- On a v4 build with SMPS support, a running SMPS owns the ADC and both
  binary bytes return `0x01` without running; a terminal `~` does not
  normally meet it because reaching `HiZ>` resets the board.  See
  [checks.md](checks.md#when-the-run-does-not-start).
- The v3 column is **not verified on v3**.

## Related docs

| Page | Why |
|---|---|
| [generic/commands.md](../../generic/commands.md) | `~` in the command tables and the 20-NUL binary entry |
| [generic/navigation.md](../../generic/navigation.md) | where `~` sits in the prompt |
| [protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md) | `BBIO1` entry and the `0x0F` leave, including the v3 reset |
| [protocol/binary/commands.md](../../protocol/binary/commands.md) | the `0x10` / `0x11` rows and the ADC-busy rule |
| [capabilities/adc/probe.md](../adc/probe.md) | the ADC scale and the SMPS feedback ownership |
| [capabilities/smps/README.md](../smps/README.md) | the optional build that can refuse the run |
| [capabilities/auxiliary/README.md](../auxiliary/README.md) | `AUX` pin and target naming |
| [SPEC.md](../../SPEC.md) | the capability index |
