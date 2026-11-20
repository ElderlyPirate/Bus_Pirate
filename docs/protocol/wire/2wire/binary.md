# 2-Wire — binary `RAW1` (2-Wire specifics)

Part of the [2-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  This page covers the 2-Wire side of the
scriptable binary interface.  The shared raw-wire framing — entry/exit, the
command groups, generic actions, peripherals, configuration bits and the
speed/pull-up commands — is owned by [the shared `RAW1` page](../binary.md).

## Entry and exit

1. Enter the binary bitbang layer from the terminal by sending 20 × `0x00`;
   it prints `BBIO1`.
2. Send `0x05` (enter raw wire mode).  The mode prints `RAW1` and starts with
   **2 wires** configured; for 2-Wire nothing needs to change.
3. Send `0x00` to leave raw wire (back to `BBIO1`); from BBIO, `0x0F` returns
   to the interactive terminal.

Entry applies open drain, 2 wires, 8-bit, MSB-first and the maximum timing
profile (while recording speed 1).  There is no width command.

## 2-Wire specifics

| Request | Meaning | Host sends | Reply | Terminal equivalent |
|---|---|---|---|---|
| `0x06` | read one byte from SDA | — | one data byte | `r` |
| `0x1n` | bulk byte, `n + 1` bytes | `n + 1` data bytes | `0x01`, then `0x01` per data byte | a value (`0xNN`, `0b…`, decimal, `"text"`) |
| `0x8x` | configuration: bit 3 open drain, bit 2 wire count, bit 1 LSB-first | — | `0x01` | — (see mode entry and `l` / `L`) |
| `0x6x` | speed profile 0–3 | — | `0x01` | speed menu |
| `0x04` / `0x05` | CS low / high (the card reset in 2-Wire) | — | `0x01` | `C`, then `A` / `a` |
| `0x02` / `0x03` | I2C-style start / stop | — | `0x01` | `[` and `]` |
| `0x5x` | pull-up feed: `0x51` 3.3 V, `0x52` 5.0 V, other off | — | `0x01` applied / `0x00` refused | `e2` / `e3` / `e1` |

Notes:

- Each byte is clocked out on SDA (MOSI); the trailing `0x01` is the **Bus
  Pirate's own result byte**, not a slave ACK.  There is no slave-ACK byte in
  2-Wire (compare I2C's binary mode, which returns the slave ACK per byte).
- For 2-Wire leave configuration bit 2 clear (`0x88` = open drain + 2 wires +
  MSB-first).  `0x8x` and `0x6x` both set CS high — driven high in normal
  output, released in open drain.
- The pull-up feed reply is `0x00` when it is not applied: in normal output or
  when VPU already has a voltage.  On v3 the request returns `0x00`.
- `0x1n` consumes exactly `n + 1` bytes.  A command byte sent early becomes
  bus data.
- In 2-Wire the CS pin is only the smart-card reset line; the bus start/stop
  do not use it.

## Start and stop

`0x02` sends an I2C-style start condition and `0x03` an I2C-style stop; both
reply `0x01`.  In 2-Wire these mirror the terminal `[` and `]`.  As in the
terminal, a start is skipped when a line already reads low, but the reply is
still `0x01`.

## Differences from 3-Wire

| Item | 2-Wire | 3-Wire |
|---|---|---|
| bulk byte reply | one `0x01` per byte (Bus Pirate ack) | one sampled MISO byte per byte |
| start/stop (`0x02`/`0x03`) | the terminal `[`/`]` | I2C-style waveform only; not the CS wrappers |
| CS wrappers | `0x04`/`0x05` are the card reset | `0x04`/`0x05` are the terminal `[`/`]` |
| read path | clocks SCL and samples SDA | clocks CLK and samples MISO |
| data direction | one bidirectional line | separate MOSI and MISO |

## Worked exchange

The shared `RAW1` exchange — enter bitbang, select raw wire, configure, set
speed, feed the pull-up, write, read, exit — is in
[the shared raw-wire page](../binary.md#worked-exchange).  The reply byte
after the write is the Bus Pirate's own result byte, not a slave ACK.

## Limits

- The `RAW1` binary path starts as 2 wires until reconfigured; the same mode
  also serves 3 wires, so a host must set the wire count deliberately.
- The width is fixed at 8 bits; there is no `;N` equivalent.
- The bulk bit-transfer group (`0x3x`) writes only low bits; see the shared
  page.
- The `0x5x` pull-up feed group is v4 only; on v3 it returns `0x00`.
- There is no ATR macro, no settings read-back and no macro command set.
- The full PIC and SMPS sub-protocols are not covered here.
- The binary path is **unverified**; the terminal path in the other topic files
  is the described reference.
