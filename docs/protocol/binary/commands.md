# BBIO1 command map

Part of the binary-layer documentation set (`docs/protocol/binary/`).  This
page is the single authority for the **bitbang layer itself** (BBIO1): the pin
state and direction bytes, the core command byte values, their reply shapes and
their v3/v4 availability.  The sub-protocol entries and their shared groups are
in [sub-protocols.md](sub-protocols.md); entry, replies and framing are in
[entering-and-framing.md](entering-and-framing.md).

## Same byte, different layer or mode

A byte value does not identify a command on its own — the current layer and
mode decide:

- In BBIO1, `0x40`–`0x5F` is a pin **direction** byte.  The `0x4x`, `0x5x`,
  `0xAx` and `0xFx` command groups exist only inside a sub-protocol.
- In BBIO1, every `0x80`–`0xFF` byte is a pin **state** byte.  That includes
  `0xA0`–`0xAF`, `0xF0`–`0xFF` and `0x88`: inside raw wire they are PIC,
  SMPS and configuration commands; in BBIO1 they switch power, pull-ups and
  pins.
- In BBIO1, `0x00` re-prints the identifier and `0x01` enters SPI.  Inside a
  sub-protocol, `0x00` is that mode's exit and `0x01` re-prints **its**
  identifier.
- `0x0F` is the terminal return in BBIO1 only.  In the nibble-group modes it
  is a mode-specific byte; in I2C it starts the sniffer and in UART it enters
  the bridge.  See [edge-cases.md](edge-cases.md#0x0f-inside-a-mode-does-not-return-to-the-terminal)
  §`0x0F` inside a mode.
- `0x8x` is per mode: `0x88` is normal output in SPI but open drain in
  `RAW1`.  Use the mode's own page.
- `PIC1`, `OCD1` and `XSV1` have their own flat layouts; for example `0x04`
  clocks a PIC opcode in `PIC1`, while in `OCD1` the same value selects TRST
  inside a feature request (`0x02 0x04 action`) and `0x05` is the TAP shift.

## Byte classes

| Byte | Class | Reply |
|---|---|---|
| `1xxxxxxx` (`0x80`–`0xFF`) | pin **state** byte | the five pin levels (`0x00`–`0x1F`) |
| `010xxxxx` (`0x40`–`0x5F`) | pin **direction** byte | the five pin levels (`0x00`–`0x1F`) |
| all other low-bit bytes | core **command** byte | per command |

Bytes that match neither a command nor a valid payload are answered with a
single `0x00` failure byte.

## Reply shapes

There is no frame marker, so the host must know each command's reply shape.
`0x01` is the usual success byte and `0x00` the usual failure, but a reply can
also be a value, a count, a level or a bus bit:

| Shape | Reply | Used by |
|---|---|---|
| result byte | `0x01` success, `0x00` failure | most commands |
| data word | the value, no result byte | `0x14` (2 bytes), `0x16` (4 bytes), `0xF0` (2 bytes) |
| count | number of failed checks (`0x00` = clean) | `0x10`, `0x11` |
| pin level | `0x00`–`0x1F`, five levels | state byte, direction byte |
| result + level | result byte (`0x01` arrived, `0x00` timeout) then the selected pin level | `0x70` wait-for-edge |
| leading `0x01`, then data or per-byte replies | one ack, then the read bytes or one reply per data byte | I2C `0x08`; `0x1x` groups |
| stream | samples until the host sends a byte | `0x15` |
| bus ACK bit | `0x00` = ACK, `0x01` = NACK | I2C `0x1x` per byte |

So `0x00` is a normal reply for a clean self-test, an all-low pin level and an
I2C ACK.  The sections below and the mode pages state the exact shape for each
command.

## Reading pins

There is **no read-only five-pin command**.  Every state byte writes all five
latches plus the supply and pull-up bits, then reports the levels; every
direction byte rewrites all five directions, then reports the levels.  Neither
can sample a pin without changing the outputs, and neither reports the
directions.

- Do **not** poll with `0x80`: it switches the target supply and the pull-up
  resistors off and drives every latch low.
- To sample the selected AUX pin without changing the pin state, send the wait
  with a zero timeout: `0x70 0x03 0x00 0x00`.  It always returns `0x00`
  (timeout) and then the current level, even if an edge is pending: expiry is
  evaluated before the change-notification flag.  Make the pin an input first
  if it is an output; see [Wait for an edge](#wait-for-an-edge-0x70).
- The other four header lines have no read-only sample in BBIO1.  A level byte
  from them always follows a write.

## Pin state byte (`1xxxxxxx`)

Bit 7 is fixed `1`.  The lower seven bits drive the pins and peripherals; the
reply is the five pin levels (`0x00`–`0x1F`) after the write.  Neither the
class bit nor the power and pull-up bits are echoed — only the five pin
levels are reported.

| Bit | Mask | Controls | State `1` / `0` |
|---|---|---|---|
| 7 | `0x80` | class marker | always `1` |
| 6 | `0x40` | target supply (VREG) | on / off |
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

The class bit is part of the byte, so a state request is always `0x80` or
higher.  `0xC0` is "supply on, all pin latches low"; `0x40` is **not** a state
byte — it is the direction byte "all outputs".  The pull-up bit switches the
resistor network only; the voltage still has to come from Vpullup (see
[entering-and-framing.md](entering-and-framing.md#wiring) §Wiring).

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

The whole `0x40`–`0x5F` range is this direction byte in BBIO1.  Set means
**input**, which is the opposite of the common "1 = output" convention: a host
that sends `0x40` makes all five lines outputs, and `0x5F` makes all five
inputs.  A host that sends `0x4C`, `0x51` or any other `0x4x`/`0x5x` byte here
changes pin directions and receives the pin levels — it does not switch on
power, select the pull-up feed or enter a mode.  Those groups exist only inside
a sub-protocol.

The direction byte has bits only for `AUX` and `CS` of the AUX targets; there
is no bit for `AUX1` or `AUX2`.

## The AUX pin

The state-byte and direction-byte AUX bit (bit 4) always drives or senses the
plain `AUX` pin, whatever AUX target the terminal has selected.  The terminal
can select among `AUX`, `CS`, `AUX1` and `AUX2` with `c` / `C` / `k` / `K`
(the selection tokens are `A0` / `CS` / `A1` / `A2`; v3 prints `aux` for `A0`,
and `A1` / `A2` are v4).

| Behaviour | Follows the terminal selection? |
|---|---|
| state-byte AUX level | no — always `AUX` |
| direction-byte AUX bit | no — always `AUX` |
| `0x70` / `0x7x` wait for an edge | yes |
| PWM (`0x12`) output | no — always `AUX` |
| frequency count (`0x16`) input | no — always `AUX` |
| I2C extended-AUX command (`0x09`) | sets its own AUX/CS choice |

Entering BBIO1 does not reset the selection.  A terminal selection made before
entering therefore applies to the wait.  Binary I2C is the one sub-protocol
whose entry forces the AUX target back to `AUX`; its `0x09 0x10` / `0x09 0x20`
sub-commands choose `AUX` or `CS` again.  If the wait follows the wrong pin,
check the selection in the terminal first, or use `AUX` / `CS` from BBIO1.

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
| `0x08` | enter AVRISP-MKII USB personality (**v4 only**) | none; v3: `0x00`, then `BBIO1` |
| `0x09` | enter AVRISP-MKII serial STK500v2 route (**v4 only**) | none; v3: `0x00`, then `BBIO1` |
| `0x0F` | return to the terminal through a reset | `0x01`, then reset |
| `0x10` | short self-test | error count |
| `0x11` | full self-test, jumpers required | error count |
| `0x12` | set up PWM (five bytes follow) | `0x01` |
| `0x13` | clear / disable PWM | `0x01` |
| `0x14` | ADC one-shot (voltage probe) | 2 bytes big-endian |
| `0x15` | ADC continuous | stream of 2-byte samples |
| `0x16` | frequency count on `AUX` | 4 bytes big-endian |
| `0x18` | XSVF player (**v4 only**) | `XSV1` (v3: `0x00`) |
| `0x70` | wait for an edge on the selected AUX pin | 2 bytes (see below) |

Reserved or unrecognised commands are answered with `0x00`; these include
`0x0A`–`0x0E`, `0x17`, `0x19`–`0x3F`, `0x60`–`0x6F` and `0x71`–`0x7F`.
The ranges `0x40`–`0x5F` and `0x80`–`0xFF` are not commands at all.

### Entering the sub-protocols (`0x01`–`0x09`, `0x18`)

Each entry switches the layer to that protocol's command loop and prints the
protocol's identifier.  Send `0x00` to the sub-protocol to return to `BBIO1`.
The XSVF player is the exception: it ignores `0x00`, so use `0x0F` (below).
The command sets and links are in [sub-protocols.md](sub-protocols.md); the
`0x08`/`0x09` personalities are in
[the AVRISP-MKII set](../../programmers/avrisp/entry.md).  Those two
entries have no command loop and no identifier: they hand the link to the
personality.  The USB route waits about two minutes for the first complete
non-empty request (the serial route about five seconds from entry), then both
end after about five seconds of idle once such a request has arrived; empty
packets extend neither window.  A reset also ends it, then it prints `BBIO1`
again.  On a v3 build both bytes are answered with
`0x00` and the layer re-announces itself with `BBIO1`, so read until `BBIO1`.

### Reset (`0x0F`)

The Bus Pirate replies `0x01`, then resets.  On v3 this is a real hardware
reset.  On v4 it runs the **terminal reset** — pins to inputs, pull-ups
released, supplies off, an SMPS feedback loop stopped, the AUX output source
detached and the AUX assignment back to `AUX`, display format back to HEX,
BASIC left — then prints the banner and the `HiZ>` prompt.  The reset also
clears the generator flag, stops Timer2 and clears the v4 `AUX1` open-drain
bit, but it does not disable the output-compare channel or clear its
period/duty setup; send `0x13` if the PWM module must be left fully idle.  See
[PWM](#pwm-0x12-0x13) and
[entering-and-framing.md](entering-and-framing.md#leaving) §Leaving.

### Self-test (`0x10`, `0x11`)

- `0x10` — **short self-test**: the pass without the external-jumper steps.
  The per-check `0x10` column in
  [the self-test check reference](../../capabilities/selftest/checks.md)
  defines exactly what it skips.
- `0x11` — **full self-test**: every check.  Disconnect the target first and
  fit the same jumpers as the interactive full run: `ADC`→`+3.3V` on both
  boards, plus `Vpu`→`+5V` on v3.

The reply is a single byte: the **number of failed checks** (`0x00` = no
errors).  If the SMPS owns the ADC, the reply is `0x01` and no check runs.
While the test is idle it reads input — any byte other than `0xFF`
is echoed back as `byte + errors` (one byte, wrapping), and `0xFF` leaves the
test with a reply of `0x01`.  Leaving the test returns to the bitbang layer
**without re-printing `BBIO1`**, so the host must track the current layer
itself.  The MODE LED blinks when there are no errors and is steady when there
are.  Full detail:
[the self-test binary page](../../capabilities/selftest/binary.md).

On a build with SMPS support, while the SMPS feedback loop owns the ADC, both
commands report a count of `0x01` (refusal count; no check ran) without
running the test, then enter the same idle loop.  See
[edge-cases.md](edge-cases.md#adc-is-busy--the-smps-feedback-owns-the-converter)
§ADC is busy.

### PWM (`0x12`, `0x13`)

- `0x12` — set up PWM.  Five bytes follow:

  | Byte order | Field |
  |---|---|
  | 1 | prescaler, low two bits: `0` = 1:1, `1` = 1:8, `2` = 1:64, `3` = 1:256 |
  | 2–3 | duty cycle, a raw 16-bit timer count, high byte first |
  | 4–5 | period, a raw 16-bit timer count, high byte first |

  The Bus Pirate replies `0x01` after the complete five-byte sequence.  The
  output appears on the plain `AUX` pin, which is forced to an output; it does
  not follow the selected AUX target.  Duty and period are **raw timer
  counts**, not percentages.  The timer tick is the 16 MHz clock divided by
  the prescaler, and the output frequency is approximately

  ```
  output frequency = timer tick rate / (period + 1)
  ```

  The duty count is compared against the period count, so a duty of about
  half the period is a 50 % duty cycle.

  Example: prescaler `2` (1:64, tick 250 kHz), duty `0x0063` (99), period
  `0x00C7` (199) gives about 1250 Hz at about 50 %:

  ```
  host -> 0x12 0x02 0x00 0x63 0x00 0xC7    set up PWM
  host <- 0x01                             success
  ```

- `0x13` — clear / disable PWM: stops the timer, disables the output-compare
  channel and detaches the AUX output; replies `0x01`.

There is **no binary servo command**.  The servo generator is terminal-only
(`S`); the binary PWM is a raw timer-count generator.  Units, ranges and the
servo details are in the
[AUX capability page](../../capabilities/auxiliary/README.md).

A PWM or servo left running in the terminal is not cleared by entering BBIO1:
entry makes the pins inputs, so the waveform no longer drives the header, but
the timer setup stays until `0x12`, `0x13` or a hardware reset.  The v4
terminal reset clears the generator flag, stops Timer2 and detaches the AUX
output source, but it does not disable the output-compare channel or clear its
period/duty setup; send `0x13` if the PWM module must be left fully idle.  On v3
the hardware reset stops everything.

On an SMPS-enabled build, `0x12` and `0x13` are refused with `0x00` while the
SMPS feedback loop owns the ADC; `0x12` consumes its five-byte frame first.

### ADC voltage probe (`0x14`, `0x15`)

- `0x14` — one-shot reading: 2 bytes, high byte first, the raw 10-bit count
  (`0x0000`–`0x03FF`).
- `0x15` — continuous readings: one sample per loop — sample, transmit; the
  stream pauses if the host falls behind.  There is **no leading `0x01`
  acknowledgement** — the first bytes on the wire are samples.  Any byte from
  the host stops the stream and is consumed; keep reading until no sample
  arrives within your timeout before sending the next command.

On a build with SMPS support, while the SMPS feedback loop owns the ADC, both
commands return the impossible sample `0xFF 0xFF` once; `0x15` does not stream.
See
[edge-cases.md](edge-cases.md#adc-is-busy--the-smps-feedback-owns-the-converter)
§ADC is busy.

The scale, limits and wiring are in
[the ADC capability page](../../capabilities/adc/README.md).

### Frequency count (`0x16`)

Takes a frequency measurement on the **`AUX`** pin and returns 4 bytes, most
significant byte first, in **Hz**.  The command forces `AUX` to an input and
does not follow the selected AUX target; it uses a **one-second gate** and
returns `0` when no edges are counted.  The first gate runs a 1:256 prescaler.
If its count is 255 or less, the measurement is repeated at the finer 1:1
prescaler.  Above about 65.5 kHz the first count is larger, so the single
prescaled gate is used and the result is a multiple of 256 Hz.  Allow about one
second above about 65.5 kHz and about two seconds below it.

```
host -> 0x16                            measure the frequency on AUX
host <- 0x00 0x00 0x27 0x10             example: 10000 Hz
```

`0x16` **stops the terminal generator** — a PWM or a servo: the generator
timers are shut down and `AUX` becomes an input.  The terminal `f` command
refuses while a generator is active instead, so send `0x13` first if you want
to clear a PWM deliberately.  Units and limits:
[the AUX capability page](../../capabilities/auxiliary/frequency.md).

### XSVF player (`0x18`, v4 only)

On v4 this switches on the on-board voltage regulator, prints `XSV1` and
enters the XSVF player loop; on v3 the byte is answered with a single `0x00`
and the layer stays in bitbang.  The player ignores `0x00`; send `0x0F` to
leave it, which returns to BBIO1 **silently** and does not reset the pins or
switch the regulator off.  Full detail: [the XSVF player page](../jtag/xsvf.md).

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

2. one **16-bit big-endian timeout** in milliseconds (`0` returns at once; see
   below).  The maximum is 65535 ms (about 65.5 s).

The reply is two bytes: a **result byte** (`0x01` when the requested edge
arrived, `0x00` on timeout) followed by the **selected AUX pin level** (`0` or
`1`) at the moment the wait finished.

A **zero timeout always returns `0x00`** (timeout) followed by the current
level, even if an edge is pending: the firmware evaluates expiry before the
change-notification flag.  Do not wait for `0x01` from a zero-timeout wait.

The wait starts when the frame is complete: an edge that arrives while the
selector and timeout bytes are still being received is not latched.

Two different invalid cases:

- An invalid **sub-command** (`0x71`–`0x7F` in a nibble group) is answered
  with a single `0x00` immediately, without reading a payload.
- A valid sub-command with an invalid **edge selector** still consumes the
  selector and the two timeout bytes before it replies `0x00`.  A host that
  sends just `0x70 0x00` leaves the layer waiting for the timeout bytes.

**Direction contract.**  The wait does **not** change the selected pin's
direction.  Configure it as an input first — a BBIO1 direction byte such as
`0x5F` (all inputs), or the mode's equivalent — then send the wait.  Of the
AUX targets, the BBIO1 direction byte has bits only for `AUX` and `CS`; if the
selected target is `AUX1`/`AUX2`, make it an input in the terminal before
entering (the selection persists), or use `AUX`/`CS`.  The firmware reads the
pin level without switching direction and arms change notification on the
selected pin; change notification on a pin the firmware is driving as an
output is not confirmed, so treat an output-pin wait as `unverified`.

The wait reports that an edge occurred, not a count or a pulse width.  Use
`0x16` for a rate.

## Availability

| Command group | v3 | v4 |
|---|---|---|
| state byte / direction byte | present, unverified | present |
| `0x00`, `0x01`–`0x07`, `0x0F` | present, unverified | present |
| `0x08` / `0x09` AVRISP-MKII | — (`0x00`, then `BBIO1`) | present |
| self-test `0x10` / `0x11` | present, unverified | present |
| PWM `0x12` / `0x13` | present, unverified | present |
| ADC `0x14` / `0x15`, frequency `0x16` | present, unverified | present |
| XSVF `0x18` | — (`0x00`) | present |
| wait-for-edge `0x70` | present, unverified | present |

The v3 column is **not verified** on hardware.

## Related docs

- [entering-and-framing.md](entering-and-framing.md) — entry, replies,
  framing.
- [sub-protocols.md](sub-protocols.md) — the protocol entries and shared
  groups.
- [examples.md](examples.md) — worked exchanges.
- [edge-cases.md](edge-cases.md) — failure modes.
- [the AUX capability page](../../capabilities/auxiliary/README.md) — PWM,
  frequency and edge-wait units and limits.
- [the ADC page](../../capabilities/adc/README.md) — probe and continuous
  sampling, scale, wiring and limits.
- [the self-test page](../../capabilities/selftest/binary.md) — the binary
  variants in full.
