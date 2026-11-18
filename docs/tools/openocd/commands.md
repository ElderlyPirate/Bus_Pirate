# `OCD1` commands — pin modes, features, TAP shift, ADC, speed, delay

Part of the OpenOCD adapter set.  This page is the byte-level reference for
the command loop that starts after `0x06` answers `OCD1`.  Entry and exit
are in [entry-and-exit.md](entry-and-exit.md); worked traces are in
[exchanges.md](exchanges.md); the TAP the bits belong to is
[protocol/jtag/tap.md](../../protocol/jtag/tap.md).

## Scope

- Every request the path accepts, its payload and its reply.
- The pin modes, the feature bits incl. VREG, the ADC read, the TAP shift,
  the link-speed handshake and the TCK delay.
- **Not covered:** the host program's options, the wiring, and the failure
  handling; each has its own page in this set.

## Framing rules

- A request is one command byte followed by a fixed payload.  There is no
  length prefix and no checksum.
- Configuration requests (`0x01`, `0x02`, `0x08`) have **no reply**.  Do not
  wait for one; the next byte you read would belong to a later request.
- Multi-byte values are **big-endian**: the shift count, the TCK delay, the
  ADC readings.
- An unrecognised command byte is answered with a single `0x00`; the loop
  stays in `OCD1`.
- The two identifiers are text, not status codes: `0x06` answers `OCD1`,
  and leaving with `0x00` returns to `BBIO1`.

### Command table

| Request | Payload | Action | Reply |
|---|---|---|---|
| `0x00` | — | leave the path | `BBIO1` (5 bytes) |
| `0x01` | 1 byte: pin mode | set the pin mode | none |
| `0x02` | 1 byte: feature, 1 byte: action | set a feature | none |
| `0x03` | — | read the four ADC channels | 10 bytes |
| `0x04` | — | not a command in this path | `0x00` |
| `0x05` | 2-byte bit count, then TDI/TMS data | TAP shift | 3-byte echo, then TDO bytes |
| `0x06` | — | report the identifier | `OCD1` (4 bytes) |
| `0x07` | 1 byte: selector; then a separate `0xAA 0x55` write | set the host link speed | `0x07`, selector (or `0x07 0x00` after a bad confirmation) |
| `0x08` | 2 bytes: delay | set the TCK delay | none |
| `0x09`–`0x0F` | — | not commands in this path | `0x00` |

`0x04` is **not** the TAP shift.  It is the retired byte from the older
protocol version; a current host must use `0x05`.  Inside `OCD1` it is
answered like any other unknown command.

### Byte meanings are per mode

The same byte means different things in `BBIO1` and in `OCD1`.  Always read
the table for the mode you are in:

| Byte | In `BBIO1` | In `OCD1` |
|---|---|---|
| `0x04` | enter the 1-Wire sub-mode | unrecognised; `0x00` |
| `0x05` | enter the raw-wire sub-mode | TAP shift |
| `0x06` | enter this OpenOCD path | report `OCD1` |
| `0x0F` | return to the terminal | unrecognised; `0x00` |

## Pin modes (`0x01`)

One byte follows the command.  There is no reply.

| Mode byte | Setting |
|---|---|
| `0x00` | Hi-Z: all five JTAG lines are inputs; TDO is an input |
| `0x01` | JTAG: TDI, TMS, TCK, SRST and TRST are push-pull outputs; TDO input |
| `0x02` | JTAG open-drain: the same outputs are open-drain; pull-ups required |
| other | behaves as `0x00` Hi-Z |

- Entry leaves TDI, TMS, TCK and SRST Hi-Z; TRST is not touched.  A pin
  mode must be set before the first shift.
- The mode byte writes the five JTAG output latches (TDI, TMS, TCK, SRST,
  TRST) **low** before it applies the direction.  Selecting either JTAG mode
  therefore drives **SRST and TRST low**.
- A later pin mode re-parks all five outputs low, including the two reset
  lines.
- TDO is always an input.
- The host normally sends one pin mode per session:
  `0x01 0x01` for push-pull, `0x01 0x02` for open-drain.

### The pin mode asserts both resets

This is the most common surprise in a hand-driven session.  With active-low
reset inputs, `0x01 0x01` or `0x01 0x02` leaves both resets asserted.  The
release is a feature write with action **`1`**:

| Line | Assert (usual active-low) | Release |
|---|---|---|
| TRST | `0x02 0x04 0x00` | `0x02 0x04 0x01` |
| SRST | `0x02 0x08 0x00` | `0x02 0x08 0x01` |

Do not "release" a reset by re-sending action `0x00`, by re-sending the pin
mode, or by re-entering `0x06`: `0x00` is the asserted level, the pin mode
drives low again, and `0x06` inside the loop only prints the identifier.
Send the feature with action `1`.

## Features (`0x02`)

Two bytes follow: the feature, then the action.

| Feature | Meaning | Action |
|---|---|---|
| `0x01` | mode LED | `0x00` off; any non-zero on |
| `0x02` | regulator (VREG), driving the `3.3V` and `5.0V` rails | `0x00` off; any non-zero on |
| `0x04` | TRST | `0x00` low; `0x01` high |
| `0x08` | SRST | `0x00` low; `0x01` high |
| `0x10` | on-board pull-up resistors, tied to Vpullup | `0x00` off; any non-zero on |
| other | ignored | — |

- For TRST and SRST the action is the **pin level**, not an "assert" flag.
  Both are commonly active-low, so `0x00` asserts and `0x01` releases.
- Use exactly `0x00` or `0x01` for the two reset lines.  The firmware takes
  the low bit of the action byte there, so an even value such as `0x02`
  behaves as `0x00`; the LED, VREG and pull-up features treat any non-zero
  value as on.
- The reset features only drive after a pin mode has made the pins outputs.
  Before that they change a latch that is not connected to the pin.
- Unknown features are ignored, with no reply.
- **VREG** switches the board's fixed supply.  It is the same on-board
  regulator that the terminal `W` command switches (`POWER SUPPLIES ON`).
  It drives both the `3.3V` and the `5.0V` rail; the target's VCC goes to
  exactly one of them.  The path cannot select a voltage, so leave VREG off
  for a target on its own supply and for every 1.8 V or 2.5 V target.
- **Pull-ups** (`0x10`) tie the lines to the **Vpullup** pin, not to a
  per-line rail.  A voltage must be present on Vpullup; feed it externally.
  The v4 on-board routing to Vpullup is a terminal command (`e`) and is not
  reachable from the binary path; binary entry also switches that source off.
  On the JTAG lines, per-line pull-ups to the target rail are the usual
  choice for open-drain level shifting.

## ADC read (`0x03`)

No payload.  The reply is 10 bytes:

| Bytes | Meaning |
|---|---|
| 1 | `0x03` — the command |
| 2 | `0x08` — the number of data bytes that follow |
| 3–4 | ADC/probe pin, big-endian |
| 5–6 | Vpullup |
| 7–8 | `3.3V` rail |
| 9–10 | `5.0V` rail |

- The values are 10-bit ADC counts, not millivolts.  One count is about
  6.45 mV (the converter is scaled for a 0–6.6 V input behind a divider).
- The firmware switches the ADC on for the read and off afterwards, so read
  it on demand; there is no continuous mode through this command.
- OpenOCD's `buspirate adc` sends this command and prints the four values in
  volts, for example
  `ADC: ADC_Pin = 0.00 VPullup = 5.00 V33 = 3.30 V50 = 0.00`.  It is a runtime
  command; before the adapter is initialised it sends nothing.

## TAP shift (`0x05`)

This is the only command that moves the TAP.  The host supplies TDI and TMS
for every bit and reads TDO back.

| Part | Format |
|---|---|
| count | 2 bytes, big-endian, in **bits** |
| limit | 8192 bits per `0x05` request (`0x2000`), including TAP moves |
| echo | 3 bytes: `0x05`, count high, count low |
| payload | groups of up to 16 bits, low byte first |
| reply | one TDO byte per 1–8-bit group, two per 9–16-bit group |

1. Send `0x05`, then the count high byte, then the low byte.
2. The firmware echoes the three header bytes.  The echo repeats the
   **requested** count even when the count is clamped.
3. Send the payload in groups of up to 16 bits:
   - 9–16 bits: TDI low, TMS low, TDI high, TMS high;
   - 1–8 bits: TDI low, TMS low.
4. Each bit sets TDI and TMS while TCK is low, pulses TCK high, and samples
   TDO.  The reply carries the sampled bits, low byte first, in the same
   group order.
5. After the last bit TCK is left high and TDI/TMS hold their last values.
   In a final 9–16-bit group the unused high bits are ignored; still send
   all four payload bytes.  A final 1–8-bit group takes two payload bytes.

There are no state shortcuts.  The host walks the TAP, including any reset
sequence, and keeps track of the current state across commands; the firmware
keeps no TAP state beyond the pin levels.

### Bit order and grouping

- Data is shifted **least-significant bit first** in every byte.
- The count is the number of TCK pulses, not the number of bytes.
- A 9–16-bit group consumes four payload bytes and returns two TDO bytes.
- A 1–8-bit group consumes two payload bytes and returns one TDO byte.
- The last bit of a scan normally carries TMS high to leave the shift state;
  that bit is part of the count.

### Zero count

- **v4:** the echo is sent, no payload is read and no TDO byte follows.
- **v3:** a zero count is not supported; the v3 path has no zero-count guard.
  Use at least one bit; the v3 outcome is unverified.

OpenOCD never asks for zero bits.

### The 8192-bit clamp

A count above 8192 is clamped to 8192.  The echo still repeats the requested
count, so a host cannot see the clamp in the echo.  The TDO reply is sized
for the clamped count, so expect fewer TDO bytes than the requested count.

The clamp is **not** a clean truncation.  If the host sends payload for the
full requested count, the firmware consumes only the first 8192 bits; the
surplus bytes stay in the serial stream and are read as command bytes.  The
next command is then misread, which looks like a random mode failure.

Keep the whole `0x05` request at or below 8192 bits, including its TAP
moves, and split longer scans.  OpenOCD batches queued scans and their TAP
moves into one request; it flushes at the buffer boundary but does not split
one longer scan.  A scan of 8192 bits leaves no room for the TAP moves that
go into the same request; keep margin below the limit.  The driver has two
1024-byte lane buffers, TDI and TMS
([limits-and-errors.md](limits-and-errors.md)).

## Link speed (`0x07`)

One selector byte follows, then two confirmation bytes.

| Part | Value |
|---|---|
| selector `0x01` | fast |
| any other selector | normal (and echoed unchanged) |
| confirmation | `0xAA`, then `0x55` |
| reply | `0x07`, then the selector (or `0x00` after a bad confirmation) |

**v3 requires separate writes.**  The firmware changes its UART rate as soon
as it reads the selector, **before** the confirmation bytes.  The sequence is:

1. Send `0x07` and the selector.
2. Switch the host port to the new rate.
3. Send `0xAA 0x55` in a separate write.
4. Read `0x07 selector`, or `0x07 0x00` after a bad confirmation.

Do not put the selector and the confirmation in one write.  On v3 the
confirmation would arrive at the old rate and the handshake would fail.  A
failed confirmation reverts the firmware to normal, so the host must follow
it back before reading the reply.

- The reply is **not** a success code.  The second byte repeats the
  requested selector unless the confirmation was wrong, in which case it is
  `0x00` and the link is back at normal.
- On **v4** the CDC rate is host metadata; the firmware accepts the command
  but does not change its rate.  The host's port setting also has no effect
  on a CDC link, so the write split is not required.
- Entering the path does not reset the rate.  `buspirate speed normal` is
  the safe default and the value OpenOCD uses at exit.
- On v3 the fast rate is about 1 Mbaud (nominal); normal is 115200.

## TCK delay (`0x08`)

Two bytes set the inter-bit delay as a 16-bit big-endian value.

| Value | Effect |
|---|---|
| `0` | minimum delay; fastest loop the build allows |
| `1` | default at entry |
| larger | slower TCK |

- There is **no reply** and no read-back.
- The value is a loop count, not microseconds, and no TCK frequency is
  reported.  The actual rate depends on the build, the board clock and the
  loop.
- Both builds apply the delay to every bit of a `0x05` shift.
- OpenOCD never sends this command, so a stock session runs at the default
  `1`.  A custom host can set it.

## Unknown bytes

Any byte not in the table is answered with a single `0x00`, and the loop
keeps reading.  That includes `0x04`, `0x09`–`0x0F` and every byte above
`0x0F`.

`0x00` inside the loop is **not** an unknown byte: it is the leave command.
The single-`0x00` reply to an unknown byte is easy to confuse with that
exit, so match the request you sent to the reply you expect.

## Related

| Doc | Why |
|---|---|
| [entry-and-exit.md](entry-and-exit.md) | entry state and the two-step exit |
| [exchanges.md](exchanges.md) | worked byte traces |
| [wiring.md](wiring.md) | pins, rails, pull-ups |
| [limits-and-errors.md](limits-and-errors.md) | the clamp, zero counts, recovery |
| [../../protocol/jtag/tap.md](../../protocol/jtag/tap.md) | TAP states and scan structure |
| [../../protocol/jtag/clock.md](../../protocol/jtag/clock.md) | TCK and link rates |
