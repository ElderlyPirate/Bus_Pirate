# SMPS (switched-mode power supply board)

**Status:** draft — v4-only and an optional build.  The board and its legacy
design are not available, so the electrical behaviour is `unverified`.

SMPS drives a legacy Dangerous Prototypes switched-mode power-supply add-on
board over the binary raw-wire command group `0b1111xxxx` (`0xF0`–`0xFF`).
The firmware sets a PWM duty cycle and reads the output back through the ADC,
reporting the last output sample on request.  There is no terminal menu: it is
reached only through the binary bitbang layer.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [binary.md](binary.md) | entry, the command set, the requested-value encoding, the PWM/ADC mechanism, a worked exchange |
| [edge-cases.md](edge-cases.md) | failure modes and known limits |

## The five-minute version

- **v4 only, and off in the default images.**  Enable it in a custom v4 build;
  on v3 enabling it is a compile-time error.
- No terminal command: send 20 × `0x00` to enter binary bitbang (`BBIO1`),
  then `0x05` for raw wire (`RAW1`).  The SMPS command group lives there.
- `0xF2`–`0xFF` **start**: the command byte's low nibble is the high nibble of
  the requested value, and one operand byte follows with its low byte —
  `0xF2 0x58` requests `0x0258`.  Reply `0x01`.
- `0xF1` **stop** (reply `0x01`); `0xF0` returns the last output sample
  (raw counts) as two big-endian bytes, with no result byte.
- The PWM comes out the pin the firmware labels `AUX1`; the feedback is read
  on the pin it labels `ADC`.  Treat every value as raw and `unverified` — the
  firmware never converts to volts.
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
| leave | `0x00` | `BBIO1` | leave raw wire; this does **not** stop the board |

The minimal send is the entry bytes plus one request, e.g. the byte stream
`00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 05 F2 58`.
Any serial tool that can emit raw bytes (NUL included) works; a Python
`serial` write of `b"\x00" * 20 + b"\x05\xf2\x58"` is one option.  Shared
binary framing (`BBIO1`, protocol selection, leaving) lives in
[../binary/entering-and-framing.md](../binary/entering-and-framing.md); this
recipe is SMPS-specific.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | not available (compile-time error if enabled) | optional build; **off** in the default image |
| terminal menu | — | — (binary only) |
| command group `0xF0`–`0xFF` | — | routed on all builds; replies `0x00` until enabled |
| hardware | — | legacy Dangerous Prototypes SMPS add-on board |

Full detail and the exact framing are in [binary.md](binary.md).

## Wiring headline

The firmware routes the PWM to the pin it labels **`AUX1`** (the `A1` target
of the `c`/`C` assignment) and reads the feedback on the pin it labels
**`ADC`**.  The board's own connector, pinout and supply routing are
**not verified**; the legacy board design may not match the current
firmware.  Do not derive a connector map from these documents.

## Limits & out of scope

- No terminal user interface: the only control path is the binary command
  group.
- No unit conversion: the requested value and the reported sample are raw
  firmware numbers; the sample is an unfiltered ADC count, and the relation of
  either number to an actual output voltage is `unverified`.
- The board is not available, so the whole electrical path is `unverified`.
- Shared binary-layer grammar (entering `BBIO1`, choosing a protocol, framing,
  leaving) lives in
  [../binary/entering-and-framing.md](../binary/entering-and-framing.md); the
  recipe above is SMPS-specific.