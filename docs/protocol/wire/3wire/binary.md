# 3-Wire — binary `RAW1` (3-Wire specifics)

Part of the [3-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  This page covers the 3-Wire side of the
scriptable binary interface.  The shared raw-wire framing — entry/exit, the
command groups, generic actions, peripherals, configuration bits and the
speed/pull-up commands — is owned by
[the shared `RAW1` page](../binary.md).

## Entry and exit

1. Enter the binary bitbang layer from the terminal by sending 20 × `0x00`;
   it prints `BBIO1`.
2. Send `0x05` (enter raw wire mode).  The mode prints `RAW1` and starts with
   **2 wires** configured, so a 3-Wire host must send a configuration byte:
   `0x8C` for open drain + 3 wires + MSB-first, or `0x84` for normal output +
   3 wires + MSB-first.
3. Send `0x00` to leave raw wire (back to `BBIO1`); from BBIO, `0x0F` returns
   to the interactive terminal.

Entry applies open drain, 2 wires, 8-bit, MSB-first and the maximum timing
profile (while recording speed 1).  There is no width command.

## 3-Wire specifics

| Request | Meaning | Host sends | Reply | Terminal equivalent |
|---|---|---|---|---|
| `0x06` | read one byte from MISO | — | one sampled byte | `r` |
| `0x1n` | bulk byte, `n + 1` bytes | `n + 1` data bytes | `0x01`, then one MISO byte per data byte | a value; after `{` the write also reports `READ:` |
| `0x8x` | configuration: bit 3 open drain, bit 2 wire count (1 = 3 wires), bit 1 LSB-first | — | `0x01` | — (binary-only setting) |
| `0x6x` | speed profile 0–3 | — | `0x01` | speed menu |
| `0x04` / `0x05` | CS low / high | — | `0x01` | `[` / `{` and `]` / `}` (with `/CS`) |
| `0x02` / `0x03` | I2C-style start / stop on CLK and MOSI | — | `0x01` | — (not the CS wrappers) |
| `0x5x` | pull-up feed: `0x51` 3.3 V, `0x52` 5.0 V, other off | — | `0x01` applied / `0x00` refused | `e2` / `e3` / `e1` |

Notes:

- The configuration starts at 2 wires; bit 2 set selects 3 wires.  `0x8C` is
  open drain + 3 wires + MSB-first; `0x84` is normal output + 3 wires +
  MSB-first.
- Bulk byte is full duplex: one `0x01` is sent before the data bytes, then
  each data byte sent returns the MISO byte sampled during that write.  There
  is no per-byte `0x01` (that is the 2-Wire reply).
- `0x8x` and `0x6x` both drive CS high, so a host re-asserts CS with `0x04`
  after configuring.
- `0x1n` consumes exactly `n + 1` data bytes.  A command byte sent early is
  written to the bus as data.
- The `0x5x` pull-up feed group is v4 only; on v3 it returns `0x00`.

## CS and the start/stop bytes

CS is driven explicitly with `0x04` (low) and `0x05` (high).  With the default
`/CS` polarity these are the binary counterparts of the terminal `[`/`{` and
`]`/`}`; with the `CS` choice the wrappers drive the opposite levels, so `0x05`
matches `[`/`{` and `0x04` matches `]`/`}`.  The binary bytes do not carry the
3-Wire read-back flag.

`0x02` and `0x03` still generate the I2C-style start and stop waveform on CLK
and MOSI; they are **not** the 3-Wire CS wrappers and do not move CS.

## Worked exchange

A minimal 3-Wire byte exchange.  The device bytes are illustrative; with open
drain a pull-up is still required, so the `0x51` / `0x4D` setup steps are
included:

```
host -> 0x00 x20     enter bitbang
host <- "BBIO1"
host -> 0x05         select raw wire
host <- "RAW1"
host -> 0x8C         configure: open drain, 3 wires, MSB-first
host <- 0x01
host -> 0x60         set speed = profile 0
host <- 0x01
host -> 0x51         v4: enable the 3.3 V pull-up feed
host <- 0x01
host -> 0x4D         peripherals: supply + pull-ups on, CS high
host <- 0x01
host -> 0x04         CS low
host <- 0x01
host -> 0x10         bulk-write 1 byte
host <- 0x01         accepted
host -> 0xA5         data byte
host <- (sampled byte)   the MISO byte read during the write
host -> 0x06         read one byte (clocks 0xFF, samples the next value)
host <- (one data byte)
host -> 0x05         CS high
host <- 0x01
host -> 0x00         exit -> BBIO1
host <- "BBIO1"
```

The write is full duplex — the byte is clocked out on MOSI while MISO is
sampled — and the read command returns the next value clocked in from MISO.

## Limits

- The `RAW1` binary path serves 3 wires only after the configuration byte
  changes the wire count; the mode starts with 2 wires.
- The width is fixed at 8 bits; there is no `;N` equivalent.
- The bulk bit-transfer group (`0x3x`) writes only low bits; see the shared
  page.
- The `0x5x` pull-up feed group is v4 only; on v3 it returns `0x00`.
- There are no ATR macros, no settings read-back and no mode macro command
  set.
- The binary path is **unverified**.
