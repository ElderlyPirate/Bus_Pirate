# BBIO1 command map

Part of the binary-layer documentation set (`docs/binary/`).  This page is the
single command-table authority for the **bitbang layer itself** (BBIO1): the
pin state and direction bytes and the core command byte table.  The
sub-protocol entries and their shared groups are in
[sub-protocols.md](sub-protocols.md); entry, replies and framing are in
[entering-and-framing.md](entering-and-framing.md).

## Byte classes

A request byte falls into one of three classes:

| Byte | Class | Reply |
|---|---|---|
| `1xxxxxxx` (`0x80`–`0xFF`) | pin **state** byte | the five pin levels (`0x00`–`0x1F`) |
| `010xxxxx` (`0x40`–`0x5F`) | pin **direction** byte | the five pin levels (`0x00`–`0x1F`) |
| all other low-bit bytes | core **command** byte | per command |

Bytes that match neither a command nor a valid payload are answered with a
single `0x00` failure byte.

## Pin state byte (`1xxxxxxx`)

Bit 7 is fixed `1`.  The lower seven bits drive the pins and peripherals; the
reply is the five pin levels (`0x00`–`0x1F`) showing the state after the write
(the class bit is not echoed).

| Bit | Mask | Controls | State `1` / `0` |
|---|---|---|---|
| 7 | `0x80` | class marker | always `1` |
| 6 | `0x40` | power supply (VREG) | on / off |
| 5 | `0x20` | pull-up resistors | on / off |
| 4 | `0x10` | AUX | high / low |
| 3 | `0x08` | MOSI | high / low |
| 2 | `0x04` | CLK | high / low |
| 1 | `0x02` | MISO | high / low |
| 0 | `0x01` | CS | high / low |

State byte layout, most significant bit first:

```
1|POWER|PULLUP|AUX|MOSI|CLK|MISO|CS
```

## The AUX pin

The state-byte and direction-byte AUX bit (bit 4) always drives the plain
`AUX` (`A0`) header line.  The terminal layer can reassign which pin it treats
as "AUX" with `c` / `C` / `k` / `K` among `A0` / `CS` / `A1` / `A2` (printed
`AUX` / `CS` / `AUX1` / `AUX2`; see
[../generic/README.md](../../generic/README.md)).  The `0x70` wait-for-edge
command follows that selected pin, while the state and direction bits stay on
`A0`.  Entering the binary layer does not reset the selection.

## Pin direction byte (`010xxxxx`)

Bits 7–5 are fixed `010`.  Bits 4–0 set each pin to **input (`1`)** or
**output (`0`)**; the reply is the five pin levels (`0x00`–`0x1F`).

| Bit | Mask | Pin | `1` | `0` |
|---|---|---|---|---|
| 4 | `0x10` | AUX | input | output |
| 3 | `0x08` | MOSI | input | output |
| 2 | `0x04` | CLK | input | output |
| 1 | `0x02` | MISO | input | output |
| 0 | `0x01` | CS | input | output |

Direction byte layout, most significant bit first:

```
010|AUX|MOSI|CLK|MISO|CS
```

## Core command table

| Request | Action | Reply |
|---|---|---|
| `0x00` | reset to bitbang / report version | `BBIO1` |
| `0x01` | enter binary SPI | `SPI1` |
| `0x02` | enter binary I2C | `I2C1` |
| `0x03` | enter binary UART | `ART1` |
| `0x04` | enter binary 1-Wire | `1W01` |
| `0x05` | enter raw wire | `RAW1` |
| `0x06` | enter OpenOCD JTAG | `OCD1` |
| `0x07` | enter PIC | `PIC1` |
| `0x08` | enter AVRISP-MKII USB personality (**v4 only**) | none (v3: `0x00`) |
| `0x09` | enter AVRISP-MKII serial STK500v2 route (**v4 only**) | none (v3: `0x00`) |
| `0x0F` | reset / return to the terminal | `0x01`, then reset |
| `0x10` | short self-test | error count (see below) |
| `0x11` | full self-test, jumpers required | error count (see below) |
| `0x12` | set up PWM (five bytes follow) | `0x01` |
| `0x13` | clear / disable PWM | `0x01` |
| `0x14` | ADC one-shot (voltage probe) | 2 bytes big-endian |
| `0x15` | ADC continuous | stream of 2-byte samples |
| `0x16` | frequency count on the AUX pin | 4 bytes big-endian |
| `0x18` | XSVF player (**v4 only**) | `XSV1` (v3: `0x00`) |
| `0x70` | wait for an edge on the AUX pin | 2 bytes (see below) |

Reserved or unrecognised commands are answered with `0x00`; these include
`0x0A`–`0x0E`, `0x17`, `0x19`–`0x3F`, `0x60`–`0x6F` and `0x71`–`0x7F`.

### Entering the sub-protocols (`0x01`–`0x09`)

Each of these switches the layer to a protocol command loop and prints the
protocol's identifier.  Send `0x00` to the sub-protocol to return to `BBIO1`.
The command sets and links are in [sub-protocols.md](sub-protocols.md); the
`0x08`/`0x09` AVRISP personalities are in
[../avrisp/README.md](../../programmers/avrisp/README.md).

### Reset (`0x0F`)

The Bus Pirate replies `0x01`, then resets.  On v3 this is a real hardware
reset; on v4 it returns to the terminal with a simulated reset.  See
[entering-and-framing.md](entering-and-framing.md) §Leaving.

### Self-test (`0x10`, `0x11`)

- `0x10` — **short self-test**: every check except those needing an external
  jumper.
- `0x11` — **full self-test**: every check; the jumpers must be in place.

The reply is a single byte: the **number of failed checks** (`0x00` = no
errors).  While the test is idle it reads input — any byte other than `0xFF`
is echoed back as `byte + errors` (one byte), and `0xFF` leaves the test with a
reply of `0x01`.  Leaving the test returns to the bitbang layer **without
re-printing `BBIO1`**, so the host must track the current layer itself.  The
MODE LED blinks when there are no errors and is steady when there are.  Full
detail: [../self-test/binary.md](../../capabilities/selftest/binary.md).

### PWM (`0x12`, `0x13`)

- `0x12` — set up PWM.  Five bytes follow:

  | Byte order | Field |
  |---|---|
  | 1 | prescaler (low two bits) |
  | 2–3 | duty-cycle value, high byte first |
  | 4–5 | period value, high byte first |

  The Bus Pirate replies `0x01` after the complete five-byte sequence; the
  PWM output appears on the AUX pin.
- `0x13` — clear / disable PWM; replies `0x01`.

**PWM remains active after the layer is left** — clear it with `0x13` or reset
the board before returning to the terminal.

### ADC voltage probe (`0x14`, `0x15`)

- `0x14` — one-shot reading: 2 bytes, high byte first.  The probe voltage is
  `(ADC / 1024) × 6.6 V`.
- `0x15` — continuous readings: sends 2-byte samples as fast as the link
  allows.  There is **no leading `0x01` acknowledgement** — the first bytes on
  the wire are samples.  A new reading is not started until the previous one
  has been sent, to keep the sample spacing even.  The stream stops when the
  host sends any byte (that byte is consumed).

### Frequency count (`0x16`)

Takes a frequency measurement on the AUX pin and returns 4 bytes, most
significant byte first.

### XSVF player (`0x18`, v4 only)

On v4 this switches on the on-board voltage regulator, prints `XSV1` and
enters the XSVF player loop; on v3 the byte is answered with a single `0x00`
and the layer stays in bitbang.  Full detail:
[../jtag/xsvf.md](../jtag/xsvf.md).

### Wait for an edge (`0x70`)

Waits for an edge on the AUX pin, or until a timeout expires.  The command is
shared by all nibble-group binary modes (see
[sub-protocols.md](sub-protocols.md)); in BBIO1 it is exactly `0x70`.

Payload and reply:

1. one **edge selector** byte:

   | Value | Edge |
   |---|---|
   | `0x01` | rising (low → high) |
   | `0x02` | falling (high → low) |
   | `0x03` | either edge |

2. one **16-bit big-endian timeout** in milliseconds (`0` checks the pin once
   and returns immediately).

The reply is two bytes: a **result byte** (`0x01` when the requested edge
arrived, `0x00` on timeout) followed by the **AUX pin level** (`0` or `1`) at
the moment the wait finished.  An invalid edge selector is answered with a
single failure byte.  The pin is read as an input while waiting, and a change
is latched by the hardware so an edge is not missed between checks.

## Availability

| Command group | v3 | v4 |
|---|---|---|
| state byte / direction byte / `0x00` / `0x01`–`0x07` / `0x0F` | present (build-only) | present |
| `0x08` / `0x09` AVRISP | — (answered `0x00`) | present |
| self-test `0x10` / `0x11` | present (build-only) | present |
| PWM `0x12` / `0x13` | present (build-only) | present |
| ADC `0x14` / `0x15`, frequency `0x16` | present (build-only) | present |
| XSVF `0x18` | — (answered `0x00`) | present |
| wait-for-edge `0x70` | present (build-only) | present |

The v3 column is **not verified** (v3 is build-only here).

## Related docs

- [entering-and-framing.md](entering-and-framing.md) — entry, replies,
  framing.
- [sub-protocols.md](sub-protocols.md) — the protocol entries and shared
  groups.
- [examples.md](examples.md) — worked exchanges.
- [edge-cases.md](edge-cases.md) — failure modes.