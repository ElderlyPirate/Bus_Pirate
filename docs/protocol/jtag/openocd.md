# JTAG — OpenOCD path (`0x06`)

Part of the JTAG documentation set.  This page documents the binary bitbang
command `0x06`, a raw JTAG TAP shifter for an OpenOCD host (or any host that
speaks the same `OCD1` command set).  Entry, identifiers and defaults are in
[entry.md](entry.md); the TAP is in [tap.md](tap.md); clocking is in
[clock.md](clock.md); pins are in [wiring.md](wiring.md).

There is no terminal JTAG mode.  Host-side OpenOCD setup is a separate tool
set: [tools/openocd](../../tools/openocd/README.md).  This page owns the
in-mode byte protocol only.  OpenOCD performs the binary entry and the
`0x06` request itself; do not pre-enter the mode.

## Entry and exit

1. Enter `BBIO1` with 20 × `0x00`.
2. Send `0x06`; the path answers `OCD1` and enters its command loop.  The
   pins are high-Z inputs at this point.
3. Send `0x00` to leave; the bitbang layer resets the pins and regulator
   and prints `BBIO1`.
4. From `BBIO1`, `0x0F` replies `0x01` and returns to the terminal (v3: a
   hardware reset).
5. Inside the mode, `0x06` re-prints `OCD1`.

`OCD1` is the protocol identifier.  The full entry/exit table is in
[entry.md](entry.md).

## Commands

| Request | Followed by | Meaning | Reply |
|---|---|---|---|
| `0x00` | — | leave OpenOCD mode | `BBIO1` (from the bitbang layer) |
| `0x06` | — | report the identifier | `OCD1` |
| `0x01` | 1 byte mode | set the pin mode (see below) | — |
| `0x02` | feature, action | set a feature (see below) | — |
| `0x03` | — | read the four on-board ADC channels | 10 bytes |
| `0x05` | count (2 bytes) + TDI/TMS data | TAP shift, returning TDO | 3-byte echo, then TDO |
| `0x07` | speed, `0xAA`, `0x55` | set the host link speed | 2 bytes |
| `0x08` | delay (2 bytes) | set the TCK delay | — |
| other | — | unrecognised | single byte `0x00` |

### Pin mode (`0x01`)

| Mode byte | Setting |
|---|---|
| `0x00` | Hi-Z — all five lines are inputs; TDO is an input |
| `0x01` | JTAG — TDI/TMS/TCK/SRST/TRST are push-pull outputs; TDO input |
| `0x02` | JTAG open-drain — the same outputs are open-drain; pull-ups required |

There is no default output mode: entry is `0x00` Hi-Z.  Send `0x01 0x01`
(push-pull) or `0x01 0x02` (open-drain) before a shift.  Any other mode byte
behaves as `0x00` Hi-Z.  Host drivers choose the mode for you: OpenOCD's
Bus Pirate driver defaults to open-drain, so a stock setup needs pull-ups
unless push-pull is selected.  See
[tools/openocd](../../tools/openocd/README.md).

The mode byte drives the five outputs low before it applies the direction.
That is a real level change on SRST and TRST: selecting either JTAG mode
holds both lines low until a feature drives them high.  If they are wired
as active-low resets, release them after the pin mode:

| Line | Assert (usual active-low) | Release |
|---|---|---|
| TRST | `0x02 0x04 0x00` | `0x02 0x04 0x01` |
| SRST | `0x02 0x08 0x00` | `0x02 0x08 0x01` |

A later pin mode re-parks both lines low.  TDO is always an input.

### Features (`0x02`)

Two bytes follow: the feature, then the action (`0x00` off/low, non-zero
on/high).

| Feature byte | Meaning |
|---|---|
| `0x01` | mode LED |
| `0x02` | on-board voltage regulator (drives the `3.3V` and `5.0V` rails) |
| `0x04` | TRST — drive test reset to the action level |
| `0x08` | SRST — drive system reset to the action level |
| `0x10` | on-board pull-up resistors — needs a voltage on Vpullup |

The action is the pin level, not an "assert" flag.  TRST and SRST are
commonly active-low, so `0x00` asserts and `0x01` releases.  The reset
features only drive after `0x01` has made the pins outputs; before that
they are inputs.  Unknown features are ignored.

Feature `0x10` ties the lines to Vpullup, not to a per-line rail.  A
voltage must be present on Vpullup: feed it externally, because the v4
on-board source is not selectable from the JTAG path.  See
[wiring.md](wiring.md#pull-ups).

Feature `0x02` switches the same regulator that the XSVF entry turns on.
It drives the `3.3V` and `5.0V` rails; leave it off when the target has its
own supply.

### ADC read (`0x03`)

Replies with ten bytes: `0x03`, `8`, then four 16-bit big-endian readings
in the order **probe, VPU, 3.3 V, 5.0 V**.

### TAP shift (`0x05`)

This is the only operation that moves the TAP.  The host supplies TDI and
TMS for every bit and reads TDO back.

1. The count is a **16-bit big-endian** number of bits.  It is
   clamped to `0x2000` (8192) bits per command.
2. The Bus Pirate echoes the three header bytes (`0x05`, count high, count
   low).  The echo repeats the **requested** count even when it is clamped.
3. The host sends the payload in groups of up to 16 bits, low byte first:
   - 9–16 bits: TDI low, TMS low, TDI high, TMS high;
   - 1–8 bits: TDI low, TMS low.
4. Each bit sets TDI and TMS, pulses TCK, and samples TDO.  The reply is one
   TDO byte, or two for a 9–16-bit group, low byte first.

There are no state shortcuts: the host walks the TAP, including any reset
sequence.  `0x05` keeps no state between commands beyond the pin levels.
The host must account for the 3-byte echo before the TDO bytes.

On v4 a **zero count** consumes no payload and returns no TDO byte.  On v3
the zero-count path is unverified.

**Clamp.**  A count above 8192 is clamped.  If the host sends payload for
the full requested count, the firmware consumes only 8192 bits; the
surplus is read as command bytes and desyncs the mode.  Keep every shift
at or below 8192 bits and split longer scans.  OpenOCD does not split a
longer scan for you; the target configuration must keep each scan within
the limit.

### Link speed (`0x07`)

One selector byte follows: `0x01` selects fast, any other value normal.
The host then sends `0xAA` and `0x55`; a wrong confirm forces normal.  The
reply is `0x07` plus the selector byte, not a success code.  A selector
other than `0x01` still selects normal and is echoed unchanged.  On v3 the
rate changes before the confirm bytes; on v4 the CDC rate is
host-controlled and the command is a no-op.  Details:
[clock.md](clock.md).

### TCK delay (`0x08`)

Two bytes set the inter-bit delay as a 16-bit big-endian value; larger is
slower.  There is no reply and no read-back.  Default `1`; `0` is
accepted.  Both builds apply it.  Details: [clock.md](clock.md).

## Worked exchange

A format illustration of a 16-bit TAP shift; the byte values are not a
valid TAP sequence.  The echo is sent after the count and before the TDO
bytes.

```
host -> 0x00 x20          enter binary bitbang
host <- "BBIO1"
host -> 0x06              select OpenOCD path
host <- "OCD1"
host -> 0x01 0x01         pin mode = JTAG (push-pull)
host -> 0x02 0x02 0x01    VREG on only if the board powers the target
host -> 0x05 0x00 0x10    TAP shift, 16 bits
host <- 0x05 0x00 0x10    command echo
host -> 0x12 0xAB         TDI low byte, TMS low byte
host -> 0x34 0xCD         TDI high byte, TMS high byte
host <- 0x3C 0x81         TDO (two bytes for a 9-16 bit group)
host -> 0x00              leave OpenOCD
host <- "BBIO1"
host -> 0x0F              leave bitbang
host <- 0x01              return-to-terminal ack
host <- "HiZ>"            (version banner omitted)
```

`0x01` is a raw byte, not the text "1".  The v4 terminal reset prints the
version banner before `HiZ>`; it is omitted above so the exchange stays
aligned.

## Limits and notes

- **TAP shifter, not a JTAG engine.**  No IR/DR descriptions, no
  scan-chain or IDCODE handling, no automatic TMS sequences.  The host
  does all of it.
- Shift count is 16-bit and clamped to 8192 bits per command; the echo
  reports the requested count.  A clamped shift with a full payload leaves
  surplus bytes that desync the next command.
- The OpenOCD path is present on **both v3 and v4**.  On v3 the shift is
  interrupt-driven; on v4 it is a polling loop.  Both apply the `0x08`
  delay; the wire protocol is the same.  The v3 behaviours are **not
  verified** (build-only).
- The pin mode must be set before a shift; until then the pins are inputs.
  A mode change drives SRST and TRST low.
- A host driver that speaks this `OCD1` command set is required — the
  shifter is not standard JTAG-over-serial by itself.
- Leaving with `0x00` does not reset the target TAP; the target only
  changes state if it sees TCK/TMS or TRST.  Reset the chain before leaving
  if the target must be left in a known state.

## Related

- [entry.md](entry.md) — entry, settings and exit.
- [tap.md](tap.md) — TAP states, shifts, IDCODE and BYPASS.
- [clock.md](clock.md) — TCK delay and link speed.
- [wiring.md](wiring.md) — pins, levels and pull-ups.
- [xsvf.md](xsvf.md) — the file-driven XSVF path.
- [edge-cases.md](edge-cases.md) — failure modes and recovery.
- [tools/openocd](../../tools/openocd/README.md) — host-side setup and
  configuration.
