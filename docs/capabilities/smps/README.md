# SMPS (switched-mode power supply board)

**Availability:** optional build — v4 only, off in the default image.  The
board and its legacy design are not available, so the electrical behaviour is
`unverified`.

SMPS drives a legacy Dangerous Prototypes switched-mode power-supply add-on
board over the binary raw-wire command group `0b1111xxxx` (`0xF0`–`0xFF`).
The firmware sets a PWM duty cycle and reads the output back through the ADC,
reporting the last output sample on request.  There is no terminal menu: it is
reached only through the binary bitbang layer.  In one line: a programmable DC
output for that add-on board.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [binary.md](binary.md) | entry, the command set, the requested-value encoding, the PWM/ADC mechanism, a worked exchange |
| [edge-cases.md](edge-cases.md) | failure modes and known limits |

## Safe session

Keep the order: enter, start, read, stop, leave.

1. Enter binary bitbang: 20 × `0x00` → `BBIO1`.
2. Select raw wire: `0x05` → `RAW1`.
3. Start: `0xF2`–`0xFF` plus one operand byte → `0x01`.
4. Read: `0xF0` → two sample bytes (repeat as needed).
5. Stop: `0xF1` → `0x01`.
6. Leave raw wire: `0x00` → `BBIO1`; then `0x0F` → `0x01` and `HiZ>`.

Stopping is a separate request.  Leaving raw wire does not stop the board;
only `0xF1` sent while still in raw wire is confirmed to stop it (a hardware
reset also stops it; the v4 terminal reset is not confirmed to).  See
[edge-cases.md](edge-cases.md).

## The five-minute version

- **v4 only, and off in the default images.**  Enabling it is a source build
  change, not a runtime setting; the build process is in
  [dev/building.md](../../dev/building.md), and the per-module switch itself
  is outside these user pages.  On v3 it cannot be enabled at all.
- No terminal command: send 20 × `0x00` to enter binary bitbang (`BBIO1`),
  then `0x05` for raw wire (`RAW1`).  The SMPS command group lives there.
- `0xF2`–`0xFF` **start**: the command byte's low nibble is the top nibble of
  the requested value, and one operand byte follows with its low byte —
  `0xF2 0x58` requests `0x0258`.  Reply `0x01`.
- `0xF1` **stop** (reply `0x01`); `0xF0` returns the last output sample
  (raw counts) as two big-endian bytes, with no result byte.
- The PWM comes out the pin the firmware labels `AUX1`; the feedback is read
  on the pin it labels `ADC`.  Treat every value as raw and `unverified` — the
  firmware never converts to volts.  The useful request range is bounded by
  the 10-bit ADC; see [binary.md](binary.md) §The PWM / ADC mechanism.
- Wrong build, an unreachable request, or no board → see
  [edge-cases.md](edge-cases.md).

## Sending it from a host

There is no menu to type into: the control path is a raw byte stream.  Open
the serial port and write these bytes in order.

| Step | Bytes | Reply | Meaning |
|---|---|---|---|
| enter bitbang | `0x00` × 20 | `BBIO1` | leave the terminal for the binary layer |
| raw wire | `0x05` | `RAW1` | select raw-wire mode; the SMPS group is now live |
| start | `0xF2` then `0x58` | `0x01` | one operand byte follows the command (value `0x0258`) |
| read | `0xF0` | two bytes, big-endian | last output sample (raw counts); no result byte |
| stop | `0xF1` | `0x01` | stop switching and clean up |
| leave raw wire | `0x00` | `BBIO1` | leave raw wire; this does **not** stop the board |
| leave binary | `0x0F` | `0x01`, then `HiZ>` | return to the terminal |

The byte stream for the minimal safe session is the entry bytes, one request,
a stop and the two leaves: 20 × `0x00`, then `0x05`, `0xF2 0x58`, `0xF1`,
`0x00`, `0x0F`.  Any serial tool that can emit raw bytes (NUL included) works;
a Python `serial` write of
`b"\x00" * 20 + b"\x05\xf2\x58\xf1\x00\x0f"` is one option.  Shared binary
framing (`BBIO1`, protocol selection, leaving) lives in
[protocol/binary/entering-and-framing.md][framing]; this recipe is
SMPS-specific.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | not available (compile-time error if enabled) | optional build; **off** in the default image |
| terminal menu | — | — (binary only) |
| hardware | — | legacy Dangerous Prototypes SMPS add-on board |

**Command group `0xF0`–`0xFF` (both builds):** routed on every build; refused
with `0x00` when SMPS is not built in.

Full detail and the exact framing are in [binary.md](binary.md).

## Related capabilities

| Capability | Why it matters here |
|---|---|
| [ADC probe](../adc/README.md) | while SMPS runs, the probe channel is the SMPS feedback, so it is not a free external input |
| [AUX assignment and pin names](../../generic/commands.md#aux-targets-and-pin-names) | the PWM appears on the pin labelled `AUX1`, the `A1` target selected with `k` |

## Wiring headline

The firmware routes the PWM to the pin it labels **`AUX1`** (the `A1` target
of the AUX assignment, selected with `k`) and reads the feedback on the pin it
labels **`ADC`**.  The board's own connector, pinout and supply routing are
**not verified**; the legacy board design may not match the current firmware.
Do not derive a connector map from these documents.

## Limits & out of scope

- No terminal user interface: the only control path is the binary command
  group.
- No unit conversion: the requested value and the reported sample are raw
  firmware numbers; the sample is an unfiltered ADC count, and the relation of
  either number to an actual output voltage is `unverified`.
- Enabling the feature is a source build change; it is not available at
  runtime and cannot be enabled on v3.
- The board is not available, so the whole electrical path is `unverified`.
- The regulation loop is only meaningful over part of the reachable request
  range; see [binary.md](binary.md) §The PWM / ADC mechanism.
- Shared binary-layer grammar (entering `BBIO1`, choosing a protocol, framing,
  leaving) lives in [protocol/binary/entering-and-framing.md][framing]; the
  recipe above is SMPS-specific.

[framing]: ../../protocol/binary/entering-and-framing.md
