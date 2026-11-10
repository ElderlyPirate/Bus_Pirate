# BBIO1 command map

Part of the binary-layer documentation set (`docs/protocol/binary/`).  This
page is the single authority for the **bitbang layer itself** (BBIO1): the pin
state and direction bytes, the core command byte values, their reply shapes and
their v3/v4 availability.  The sub-protocol entries and their shared groups are
in [sub-protocols.md](sub-protocols.md); entry, replies and framing are in
[entering-and-framing.md](entering-and-framing.md).

## Same byte, different layer or mode

A byte value does not identify a command on its own — the current layer
decides:

- In BBIO1, `0x40`–`0x5F` is a pin **direction** byte.  The `0x4x` configure
  and `0x5x` pull-up-feed groups exist only after entering a sub-protocol.
- In a sub-protocol, `0x4x` configures the board and `0x5x` sets the pull-up
  feed; see [sub-protocols.md](sub-protocols.md) §Shared command groups.
- `0x8x` is per mode: `0x88` is normal output in SPI but open drain in `RAW1`.
  Use the mode's own page for its `0x8x` meaning.

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
[generic/README.md](../../generic/README.md)).  The `0x70` wait-for-edge
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

The whole `0x40`–`0x5F` range is this direction byte in BBIO1.  A host that
sends `0x4C`, `0x51` or any other `0x4x`/`0x5x` byte here changes pin
directions and receives the pin levels — it does not switch on power or the
pull-up feed.  Those groups exist only inside a sub-protocol (see the callout
above).

## Core command table

| Request | Action | Reply |
|---|---|---|
| `0x00` | re-print `BBIO1` (no state change) | `BBIO1` |
| `0x01` | enter binary SPI | `SPI1` |
| `0x02` | enter binary I2C | `I2C1` |
| `0x03` | enter binary UART | `ART1` |
| `0x04` | enter binary 1-Wire | `1W01` |
| `0x05` | enter raw wire | `RAW1` |
| `0x06` | enter OpenOCD JTAG | `OCD1` |
| `0x07` | enter PIC | `PIC1` |
| `0x08` | enter AVRISP-MKII USB personality (**v4 only**) | none (v3: `0x00`, then `BBIO1`) |
| `0x09` | enter AVRISP-MKII serial STK500v2 route (**v4 only**) | none (v3: `0x00`, then `BBIO1`) |
| `0x0F` | return to the terminal through a reset | `0x01`, then reset |
| `0x10` | short self-test | error count (see below) |
| `0x11` | full self-test, jumpers required | error count (see below) |
| `0x12` | set up PWM (five bytes follow) | `0x01` |
| `0x13` | clear / disable PWM | `0x01` |
| `0x14` | ADC one-shot (voltage probe) | 2 bytes big-endian |
| `0x15` | ADC continuous | stream of 2-byte samples |
| `0x16` | frequency count on `A0` | 4 bytes big-endian |
| `0x18` | XSVF player (**v4 only**) | `XSV1` (v3: `0x00`) |
| `0x70` | wait for an edge on the AUX pin | 2 bytes (see below) |

Reserved or unrecognised commands are answered with `0x00`; these include
`0x0A`–`0x0E`, `0x17`, `0x19`–`0x3F`, `0x60`–`0x6F` and `0x71`–`0x7F`.

### Entering the sub-protocols (`0x01`–`0x09`)

Each of these switches the layer to a protocol command loop and prints the
protocol's identifier.  Send `0x00` to the sub-protocol to return to `BBIO1`.
The command sets and links are in [sub-protocols.md](sub-protocols.md); the
`0x08`/`0x09` AVRISP personalities are in
[the AVRISP-MKII set](../../programmers/avrisp/README.md).  The AVRISP entries
have no command loop: they hand the link to the personality, which ends after
about five seconds without host traffic or on a reset.  On a v3 build both
bytes are answered with `0x00` and the layer then re-announces itself with
`BBIO1`, so read until `BBIO1`.

### Reset (`0x0F`)

The Bus Pirate replies `0x01`, then resets.  On v3 this is a real hardware
reset.  On v4 it runs the **terminal reset** — pins to inputs, pull-ups
released, supplies off, the AUX output source detached and the AUX assignment
back to `A0`, display format back to HEX, BASIC left — then prints the banner
and the `HiZ>` prompt.  See
[entering-and-framing.md](entering-and-framing.md) §Leaving.

### Self-test (`0x10`, `0x11`)

- `0x10` — **short self-test**: every check except those needing an external
  jumper.
- `0x11` — **full self-test**: every check.  Disconnect the target first and
  fit the same jumpers as the interactive full run: `ADC`→`+3.3 V` on both
  boards, plus `Vpu`→`+5 V` on v3.

The reply is a single byte: the **number of failed checks** (`0x00` = no
errors).  While the test is idle it reads input — any byte other than `0xFF`
is echoed back as `byte + errors` (one byte), and `0xFF` leaves the test with a
reply of `0x01`.  Leaving the test returns to the bitbang layer **without
re-printing `BBIO1`**, so the host must track the current layer itself.  The
MODE LED blinks when there are no errors and is steady when there are.  Full
detail: [the self-test binary page](../../capabilities/selftest/binary.md).

### PWM (`0x12`, `0x13`)

- `0x12` — set up PWM.  Five bytes follow:

  | Byte order | Field |
  |---|---|
  | 1 | prescaler, low two bits: `0` = 1:1, `1` = 1:8, `2` = 1:64, `3` = 1:256 |
  | 2–3 | duty cycle, a raw 16-bit Timer2 count, high byte first |
  | 4–5 | period, a raw 16-bit Timer2 count, high byte first |

  The Bus Pirate replies `0x01` after the complete five-byte sequence; the
  PWM output appears on the AUX pin.  Duty and period are **raw timer
  counts**, not percentages: the timer tick is the 16 MHz Timer2 clock divided
  by the prescaler, and the output frequency is approximately

  ```
  output frequency = timer tick rate / (period + 1)
  ```

  The duty count is compared against the period count, so a duty of about
  half the period is a 50 % duty cycle.

  Example: prescaler `2` (1:64, tick 250 kHz), duty `0x0064` (100), period
  `0x00C7` (199) gives about 1250 Hz at about 50 %:

  ```
  host -> 0x12 0x02 0x00 0x64 0x00 0xC7    set up PWM
  host <- 0x01                             success
  ```


- `0x13` — clear / disable PWM: stops the timer, disables the output-compare
  channel and detaches the AUX output; replies `0x01`.

`0x0F` does not clear the PWM timer setup, but on v4 the terminal reset
detaches the AUX output source, so the target stops being driven.  On v3
`0x0F` is a hardware reset.  Send `0x13` before leaving if you want the module
idle.  The full PWM behaviour is a capability topic
([the AUX capability page](../../capabilities/auxiliary/README.md), to be
written).

### ADC voltage probe (`0x14`, `0x15`)

- `0x14` — one-shot reading: 2 bytes, high byte first.  The probe voltage is
  `(ADC / 1024) × 6.6 V`; the probe pin accepts 0–6.6 V through the on-board
  divider.
- `0x15` — continuous readings: sends 2-byte samples as fast as the link
  allows.  There is **no leading `0x01` acknowledgement** — the first bytes on
  the wire are samples.  A new reading is not started until the previous one
  has been sent, to keep the sample spacing even.  The stream stops when the
  host sends any byte (that byte is consumed).

ADC detail is a capability topic
([the ADC page](../../capabilities/adc/README.md), to be written).

### Frequency count (`0x16`)

Takes a frequency measurement on the **`A0`** pin and returns 4 bytes, most
significant byte first, in **Hz**.  The command forces `A0` to an input (it
does not follow the selected alternate AUX target) and uses a **one-second
gate**; it returns `0` when no edges are counted.  If the first gate counts
255 edges or fewer, the measurement is repeated at the finer 1:1 prescaler, so
allow about two seconds.

`0x16` **stops a running PWM**: the timers are shut down and `A0` becomes an
input.  The terminal `f` command refuses while PWM is active instead, so send
`0x13` first if you want to clear the PWM deliberately.

### XSVF player (`0x18`, v4 only)

On v4 this switches on the on-board voltage regulator, prints `XSV1` and
enters the XSVF player loop; on v3 the byte is answered with a single `0x00`
and the layer stays in bitbang.  Full detail:
[the XSVF player page](../jtag/xsvf.md).

### Wait for an edge (`0x70`)

Waits for an edge on the selected AUX pin, or until a timeout expires.  The
command is shared by all nibble-group binary modes (see
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

The edge byte and the 16-bit timeout are **always consumed before the selector
is validated**.  An invalid selector is answered with a single `0x00` failure
byte only after all three payload bytes have arrived; a host that sends just
the selector leaves the layer waiting for the timeout bytes.

The reply is two bytes: a **result byte** (`0x01` when the requested edge
arrived, `0x00` on timeout) followed by the **AUX pin level** (`0` or `1`) at
the moment the wait finished.

**Direction contract.**  The wait does **not** change the selected pin's
direction.  Configure it as an input first — a BBIO1 direction byte such as
`0x5B` (all inputs except CLK), or the mode's equivalent — then send the wait.
The BBIO1 direction bits cover `A0` and `CS` only; if the selected AUX is
`A1`/`A2`, set it input in the terminal before entering (the selection
persists), or use `A0`/`CS`.
The firmware reads the pin level without switching direction and arms change
notification on the selected pin; change notification on a pin the firmware is
driving as an output is not confirmed, so treat an output-pin wait as
unverified.

## Availability

| Command group | v3 | v4 |
|---|---|---|
| state byte / direction byte / `0x00` / `0x01`–`0x07` / `0x0F` | present (build-only) | present |
| `0x08` / `0x09` AVRISP | — (answered `0x00`, then `BBIO1`) | present |
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
- [the AUX capability page](../../capabilities/auxiliary/README.md) — PWM,
  frequency and edge-wait units and limits (stub; details to follow).
- [the ADC page](../../capabilities/adc/README.md) — probe and continuous
  sampling (stub; details to follow).
