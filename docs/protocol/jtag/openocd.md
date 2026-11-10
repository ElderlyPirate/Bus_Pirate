# JTAG — OpenOCD path (`0x06`)

Part of the JTAG documentation set (`docs/jtag/`).  This page documents the
binary bitbang command `0x06`, a raw JTAG TAP shifter intended to be driven by
an OpenOCD host (or any host that speaks the same binary command set).  The
physical pin map is in [wiring.md](wiring.md); the binary-layer basics are in
[binary/README.md](../binary/README.md).

There is no terminal JTAG mode — this byte is the only way to reach the JTAG
hardware for the debug path.

## Entry and exit

1. Enter the binary bitbang layer from the terminal by sending 20 × `0x00`;
   it prints `BBIO1`.
2. Send `0x06`; the mode prints `OCD1` and the OpenOCD command loop starts.
   On entry the pins are **high-impedance inputs** — send a pin mode (below)
   before expecting any output.
3. Send `0x00` to leave the mode; the bitbang layer prints `BBIO1` again.
   From BBIO, `0x0F` returns to the interactive terminal.
4. Inside the mode, `0x06` re-prints the identifier `OCD1`.

The identifier `OCD1` stands for OpenOCD protocol version 1.

## Commands

| Request | Followed by | Meaning | Reply |
|---|---|---|---|
| `0x00` | — | leave OpenOCD mode | `BBIO1` (from the bitbang layer) |
| `0x06` | — | report the identifier | `OCD1` |
| `0x01` | 1 byte mode | set the pin mode (see below) | — |
| `0x02` | feature, action | set a feature (see below) | — |
| `0x03` | — | read the four on-board ADC channels | 10 bytes |
| `0x05` | count (2 bytes) + TDI/TMS data | TAP shift, returning TDO | 3-byte echo, then TDO |
| `0x07` | speed, `0xAA`, `0x55` | set the UART link speed | 2 bytes |
| `0x08` | delay (2 bytes) | set the JTAG clock delay | — |
| other | — | unrecognised | single byte `0x00` |

### Pin mode (`0x01`)

| Mode byte | Setting |
|---|---|
| `0x00` | Hi-Z — all pins are inputs |
| `0x01` | JTAG — TDI/TMS/TCK/SRST/TRST are push-pull outputs, TDO is an input |
| `0x02` | JTAG open-drain — the same outputs become open-drain; **pull-ups required** (see [wiring.md](wiring.md)) |

The mode byte resets the five output pins low before applying the direction,
so a mode change parks the chain cleanly.

### Features (`0x02`)

Two bytes follow: the feature, then the action (`0x00` = off/low, non-zero =
on/high).

| Feature byte | Meaning |
|---|---|
| `0x01` | mode LED |
| `0x02` | on-board voltage regulator |
| `0x04` | TRST — drive the test reset line to the action level |
| `0x08` | SRST — drive the system reset line to the action level |
| `0x10` | on-board pull-up resistors |

### ADC read (`0x03`)

Replies with ten bytes: `0x03`, then `8`, then four 16-bit big-endian
readings in the order **probe, VPU, 3.3 V, 5.0 V**.

### TAP shift (`0x05`)

This is the core JTAG operation.  It shifts a block of bits through the chain
with the host supplying both TDI and TMS per bit and reading TDO back.

1. The count is a **16-bit big-endian** number of bit sequences; it is clamped
   to `0x2000` (8192) bits per command.
2. The Bus Pirate echoes the three header bytes (`0x05`, count-high,
   count-low).
3. The host then sends the bit data in groups of up to 16 bits, **low byte
   first**:
   - for a group of **9–16** bits: one TDI byte, one TMS byte, then a second
     TDI byte and a second TMS byte (the high 8 bits);
   - for a group of **≤ 8** bits: one TDI byte and one TMS byte.
4. For each bit the Bus Pirate sets TDI and TMS, pulses TCK, and samples TDO.
   It replies with one TDO byte, or **two** when the group is 9–16 bits.

There are no state-transition shortcuts: the host is responsible for walking
the TAP (including any TMS reset pattern).  `0x05` carries no persistent state
between commands beyond the current pin levels.

### Link speed (`0x07`)

One speed byte follows (`0x00` = normal, other = fast), then the host must send
`0xAA` and `0x55`.  The reply is `0x07` plus a result byte.  If the two confirm
bytes are not `0xAA 0x55`, the firmware reverts the link to normal speed.
A host uses this when it needs to raise the CDC/serial rate for a faster run.

### JTAG clock delay (`0x08`)

Two bytes set the inter-bit delay as a 16-bit big-endian value; a larger value
means a **slower** TCK.  There is no reply.  The default at entry is the
smallest delay.

## Worked exchange

A format illustration of a 16-bit TAP shift (byte values are illustrative).  The
`0x05` echo is sent before the TDO bytes.

```
host -> 0x00 x20          enter binary bitbang
host <- "BBIO1"
host -> 0x06              select OpenOCD path
host <- "OCD1"
host -> 0x01 0x01         pin mode = JTAG (push-pull)
host -> 0x02 0x02 0x01    feature VREG = on
host -> 0x05 0x00 0x10    TAP shift, 16 bit sequences
host -> 0x12 0xAB         TDI low byte, TMS low byte
host -> 0x34 0xCD         TDI high byte, TMS high byte
host <- 0x05 0x00 0x10    command echo
host <- 0x3C 0x81         TDO (two bytes for a 9-16 bit group)
host -> 0x00              leave OpenOCD
host <- "BBIO1"
host -> 0x0F              leave bitbang
host <- "HiZ>"
```

## Limits and notes

- **TAP shifter, not a JTAG engine.**  No IR/DR descriptions, no scan-chain or
  IDCODE handling, no automatic TMS sequences.  The host does all of it.
- Shift count is 16-bit and clamped to 8192 bits per command.
- The OpenOCD path is present on **both v3 and v4**.  On v3 the shift is
  interrupt-driven; on v4 it is a polling loop with the `0x08`-tunable delay.
  The wire protocol is the same.  (The v3 behaviours here are **not
  verified** — build-only.)
- The pin mode must be set before a shift; until then the pins are inputs.
- A host driver that speaks this `OCD1` command set is required — the shifter
  is not standard JTAG-over-serial by itself.

## Related

- [wiring.md](wiring.md) — pins, levels and the pull-up requirement for the
  open-drain mode.
- [xsvf.md](xsvf.md) — the file-driven XSVF path.
- [edge-cases.md](edge-cases.md) — failure modes and recovery.