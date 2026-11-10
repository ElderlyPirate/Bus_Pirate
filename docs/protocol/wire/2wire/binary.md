# 2WIRE — binary `RAW1` (2-wire specifics)

Part of the [2-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  This page covers the 2-wire side of the
scriptable binary interface.  The shared raw-wire framing — entry/exit, the
command groups, generic actions, peripherals, configuration bits and the
speed/pull-up commands — is owned by [the shared `RAW1` page](../binary.md).

## Entry and exit

1. Enter the binary bitbang layer from the terminal by sending 20 × `0x00`;
   it prints `BBIO1`.
2. Send `0x05` (enter raw wire mode).  The mode prints `RAW1` and starts in
   the **2-wire** configuration; for 2-Wire nothing needs to change.
3. Send `0x00` to leave raw wire (back to `BBIO1`); from BBIO, `0x0F` returns
   to the interactive terminal.

## 2-wire specifics

- **Read byte (`0x06`)** clocks a full value in from SDA (MOSI), MSB-first
  unless LSB-first is configured.
- **Bulk byte (`0x1x`)** — the low nibble is `n`; the host sends `n + 1` data
  bytes.  The per-byte sequence is:

  1. host → `0x1n` (the request),
  2. host ← `0x01` (the command is accepted),
  3. for each of the `n + 1` bytes: host → data byte, host ← `0x01`.

  Each byte is clocked out on SDA (MOSI); the trailing `0x01` is the **Bus
  Pirate's own result byte**, not a slave ACK.  There is no slave-ACK byte in
  2-wire (compare I2C's binary mode, which returns the slave ACK per byte).
- **Configuration (`0x8x`)** sets the mode flags from the low bits: bit 3 =
  high-impedance/open drain, bit 2 = 3-wire, bit 1 = LSB-first.  On entry the
  handler starts open-drain, 8-bit, MSB-first and in 2-wire.  For 2-Wire leave
  bit 2 clear (`0x88` = open drain + 2 wires + MSB-first).
- **Set speed (`0x6x`)** selects the same four bit-bang profiles as the
  terminal (`0` ≈ 6 kHz … `3` maximum).
- **Pull-up feed (`0x5x`)** reproduces the v4 `e` choices: `0x51` = 3.3 V,
  `0x52` = 5.0 V, any other value disables both.  The reply is `0x01` when the
  source is applied and `0x00` when it is not, i.e. in non-open-drain mode or
  when VPU already has a voltage.  On v3 the request returns `0x00`.

## Worked exchange

The shared `RAW1` exchange — enter bitbang, select raw wire, configure, set
speed, feed the pull-up, write, read, exit — is in
[the shared raw-wire page](../binary.md#worked-exchange).  The reply byte
after the write is the Bus Pirate's own result byte, not a slave ACK.

## Limits

- The `RAW1` binary path is 2-wire until reconfigured; the same mode also
  serves 3-wire, so a host must set the wire count deliberately.
- The bulk bit-transfer command takes its data level from bit 7 of the command
  byte; the host byte it reads is consumed but not used.
- The `0x5x` pull-up feed group is v4 only; on v3 it returns `0x00`.
- The full PIC and SMPS sub-protocols are not covered here.
- The binary path is unverified; the terminal path in the other topic files is
  the described reference.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| `RAW1` raw-wire mode | present | present |
| generic / bulk / config / speed groups | present | present |
| pull-up feed (`0x5x`) | — (request returns `0x00`) | present |

The v3 column is **not verified**.
