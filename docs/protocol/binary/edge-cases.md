# Binary layer — edge cases and failure modes

Part of the binary-layer documentation set (`docs/protocol/binary/`).  Each
entry is a concrete trigger, the observable symptom, the current behaviour, and
the recovery.  Entry and framing are in
[entering-and-framing.md](entering-and-framing.md); the byte maps are in
[commands.md](commands.md) and [sub-protocols.md](sub-protocols.md).

## Entry does not happen — fewer than 20 NUL bytes

- **Trigger:** the host sends fewer than twenty `0x00` bytes, or another byte
  arrives between them.
- **Symptom:** no reply; the terminal is unchanged.
- **Behaviour:** the count is consecutive and lives in the terminal loop.  Any
  byte other than NUL (or the raw STX used by SUMP) resets it.  A short run
  gets no reply and no error.
- **Recovery:** finish any waiting settings prompt, send `#` to return to the
  idle prompt, then send twenty NULs and wait for `BBIO1`.

## 20 NULs then `0x02` enters BBIO1 and selects I2C

- **Trigger:** a host aims for SUMP by sending NULs followed by `0x02`.
- **Symptom:** `BBIO1`, then `I2C1` — not the SUMP entry.
- **Behaviour:** the binary entry fires on the 20th NUL and wins over the SUMP
  prefix.  The `0x02` is then read by the binary loop as "enter I2C".
- **Recovery:** send `0x00` to return to `BBIO1`, `0x0F` to reach the terminal,
  then send 5–19 NULs followed by `0x02`.  See
  [generic/commands.md](../../generic/commands.md#leaving-the-cli--binary-and-sump-entry)
  §Leaving the CLI — binary and SUMP entry.

## Unknown or reserved command byte

- **Trigger:** the host sends a byte that is not a defined command or a valid
  payload for the current mode.
- **Symptom:** a single `0x00` failure byte appears in the reply stream.
- **Behaviour:** in BBIO1, `PIC1` and the nibble-group modes the byte is
  refused and the layer stays in its current mode.  There is no other error
  signal and no framing marker.  `OCD1` also answers one `0x00`; only `XSV1`
  ignores unknown bytes without a reply.
- **Recovery:** drain exactly the bytes each command returns.  If a lost byte
  count has shifted the replies, see the next entry.

## Host and firmware drift to different layers

- **Trigger:** the host skips or miscounts a reply byte, or one side leaves a
  sub-protocol and the other does not.
- **Symptom:** replies no longer match the expected command shapes; terminal
  text can appear mid-stream after a reset.
- **Behaviour:** every mode reads its own fixed request layout.  A stray
  operand or data byte is interpreted as the next request, and in a nibble
  group its high nibble selects a command group.
- **Recovery:** send `0x00` to the active mode and repeat until `BBIO1`
  appears, then send `0x0F`.  Do **not** start with `0x0F`: inside I2C it
  starts the sniffer, inside UART it enters the bridge, inside SPI/1-Wire/raw
  wire/`PIC1` it is refused, and `OCD1` answers `0x00`.  The XSVF player is the
  exception: it ignores `0x00`, so leave it with `0x0F`.

## A `0x4x` or `0x5x` byte sent in BBIO1 changes pin directions

- **Trigger:** a host sends a sub-protocol configure byte (for example `0x4C`
  or `0x51`) while still in BBIO1.
- **Symptom:** the reply is a pin-level byte (`0x00`–`0x1F`), not `0x01`, and
  the bus lines change direction.
- **Behaviour:** in BBIO1 the whole `0x40`–`0x5F` range is the pin direction
  byte.  `0x4C` makes AUX, MISO and CS outputs and MOSI/CLK inputs; `0x51`
  makes MOSI, CLK and MISO outputs and AUX/CS inputs.  No power, pull-up feed
  or mode change happens.
- **Recovery:** enter the sub-protocol first, then send the configure byte.
  Re-assert the intended directions if the bus is now misconfigured.

## Power-on attempt with `0x40`

- **Trigger:** a host sends `0x40` believing the state-byte power bit (bit 6)
  is being set.
- **Symptom:** the reply is a level byte, and all five lines become outputs.
- **Behaviour:** `0x40` is `010 00000` — the direction byte "all outputs".
  A state byte must have bit 7 set: `0xC0` is "supply on, all pin latches
  low".
- **Recovery:** send `0xC0` for the supply, or `0x5F` to return all lines to
  inputs.  `0x0F` resets the pins, the supply and the pull-ups; send `0x13`
  first if a PWM timer must stop.

## `0x0F` inside a mode does not return to the terminal

- **Trigger:** the host sends `0x0F` while a sub-protocol is active.
- **Symptom and behaviour:** the byte has a different meaning in every mode.

  | Mode | What `0x0F` does | Reply |
  |---|---|---|
  | SPI, 1-Wire, raw wire, `PIC1` | refused | `0x00` |
  | I2C | starts the sniffer; next byte ends it | `0x01` after the exit |
  | UART | enters the transparent bridge | `0x01`, then the bridge |
  | `OCD1` | refused | `0x00` |
  | `XSV1` | leaves the player, back to BBIO1 | none |

- **Recovery:** send `0x00` to return to BBIO1, then `0x0F` from there.  In
  UART the bridge can be left with the NORMAL button on v4, which returns to
  the `ART1` command loop; on v3 the only way out is a reset.  In `XSV1`,
  `0x0F` already returned to BBIO1, so send `0x0F` again.

## A command waits for payload bytes that never arrive

- **Trigger:** a payload command is cut short — for example `0x12` (PWM)
  without all five bytes, `0x70` without the selector and two timeout bytes, or
  a `0x1x` bulk transfer with fewer than `n + 1` data bytes.
- **Symptom:** the Bus Pirate does not reply, and later bytes are not treated
  as commands.
- **Behaviour:** the handler reads the complete payload before it can reply or
  validate anything; the missing bytes block the loop.  This includes the
  five-byte PWM frame when the request is going to be refused.
- **Recovery:** send the missing bytes to complete the frame, or reset the
  board and re-enter.

## PWM after the layer is left

- **Trigger:** setting up PWM (`0x12`) and then leaving the layer with `0x0F`
  without clearing it.
- **Symptom:** on v4 the target stops being driven at the reset; the timer may
  still run but no waveform reaches the header.
- **Behaviour:** `0x0F` does not stop the PWM timer, and no code path stops it
  on this exit.  The v4 terminal reset detaches the AUX output source, so the
  pin stops.  On v3 `0x0F` is a hardware reset, which stops everything.
- **Recovery:** send `0x13` before leaving if you want the module idle.
  Entering BBIO1 from a terminal PWM session has the same effect: the pins
  become inputs, but the timer setup is not cleared.

## Frequency count stops a running PWM

- **Trigger:** sending `0x16` while PWM is running.
- **Symptom:** the PWM output disappears and `AUX` becomes an input.
- **Behaviour:** `0x16` does not refuse; it shuts the timers down, forces
  `AUX` to an input and measures.  The terminal `f` command refuses while PWM
  is active instead.
- **Recovery:** send `0x13` to clear PWM deliberately, then measure; re-send
  `0x12` to restart PWM afterwards.

## Continuous ADC does not stop on its own

- **Trigger:** starting continuous readings (`0x15`) and waiting for it to
  finish.
- **Symptom:** an open-ended stream of 2-byte samples.
- **Behaviour:** the stream runs until the host sends a byte; that byte is
  consumed to stop it (it is not a command).  Samples already generated and
  buffered are still delivered, and the board checks for the stop only at the
  loop boundary.
- **Recovery:** send one byte, keep reading until no sample arrives within
  your timeout, then send the next real command.  See
  [the ADC continuous section](../../capabilities/adc/binary.md).

## ADC is busy — the SMPS feedback owns the converter

- **Trigger:** an SMPS-enabled build is running its feedback loop (started with
  `0xF2`–`0xFF` in raw wire), and the host sends an ADC, PWM, pull-up-feed or
  self-test request.
- **Symptom:** the request does not do its normal job:

  | Request | Result while the SMPS feedback runs |
  |---|---|
  | `0x14`, `0x15` | `0xFF 0xFF` — impossible sample; `0x15` does not stream |
  | `0x12` | the five-byte frame is consumed, then `0x00` |
  | `0x13` | `0x00`; nothing is changed |
  | `0x5x` | `0x00`; nothing is changed |
  | `0x10`, `0x11` | error count `0x01`; test skipped, then idle loop |

- **Behaviour:** the SMPS regulation loop owns the ADC and the output-compare
  channel.  Returning to BBIO1 with `0x00` does **not** stop it, so the
  refusals continue until it is stopped.
- **Recovery:** enter raw wire and send `0xF1`, or leave the binary layer with
  `0x0F` (the v4 terminal reset runs the SMPS stop path).  The group is absent
  from the default image, so this only applies to an SMPS-enabled build.

## SMPS start on a build without SMPS

- **Trigger:** sending `0xF2`–`0xFF` in `RAW1` on a build without SMPS
  support.
- **Symptom:** `0x00`, with the operand byte consumed rather than read as the
  next request.
- **Behaviour:** the no-SMPS start path still reads its operand before it
  reports the refusal, so the following byte is the next command.  `0xF0` and
  `0xF1` are fixed requests and take no operand.
- **Recovery:** none; send the operand with every start, including on a build
  without SMPS.

## Pull-up resistors on with nothing on Vpullup

- **Trigger:** setting the pull-up bit in BBIO1 (`0xE0`) or in a sub-protocol
  (`0x4x` bit 2) without a voltage on the Vpullup pin.
- **Symptom:** the I/O lines are not pulled to a defined high level; reads can
  follow leakage or a nearby signal.
- **Behaviour:** the bit switches the resistor network only.  Entry switches
  the on-board Vpullup source off, and in BBIO1 the `0x5x` group is not
  available, so a BBIO1-only session needs an external voltage on Vpullup.  The
  terminal `P` command warns about this; the binary layer does not.
- **Recovery:** feed Vpullup with 0–5 V, or in a sub-protocol send `0x51` or
  `0x52` first (v4).  Turn the resistors off if no source is available.

## Wait-for-edge returns "no edge" immediately

- **Trigger:** a wait with a timeout of `0` and a pin that is already at rest.
- **Symptom:** result byte `0x00` (timeout) with the current level.
- **Behaviour:** a `0` timeout checks the pin once and returns without waiting;
  this is the documented way to sample the selected pin with this command.
- **Recovery:** none — use a non-zero timeout to actually wait.

## Invalid wait frame — two different cases

- **Trigger:** a malformed wait request.

  | Frame | Result |
  |---|---|
  | `0x71`–`0x7F` (invalid sub-command) | `0x00` at once; no payload |
  | `0x70 0x00 0x00 0x64` (invalid selector) | `0x00` after the payload |
  | `0x70 0x00` and stop (no timeout) | no reply; the layer waits |

- **Symptom:** either an immediate failure byte or a blocked layer.
- **Behaviour:** the sub-command nibble is validated before the payload, but
  the edge selector is validated only after the selector and the 16-bit
  timeout have been consumed.  The layer cannot tell a host that stopped from a
  host that is slow.
- **Recovery:** complete the frame with two timeout bytes (any value) to get
  the `0x00`, or reset and re-enter.

## Wait on AUX1 or AUX2 after entering from the terminal

- **Trigger:** the terminal selected `AUX1` or `AUX2` and left it as an output;
  the host then sends a wait from BBIO1.
- **Symptom:** the wait may never see an edge, or may report an odd level.
- **Behaviour:** the wait does not change the pin's direction, and the BBIO1
  direction byte has no bit for `AUX1`/`AUX2`.  Change notification on a pin
  the firmware drives is not confirmed.
- **Recovery:** before entering the binary layer, select the pin and release it
  with `@` in the terminal (the selection persists), or use `AUX`/`CS`, whose
  direction the BBIO1 direction byte can set.

## Sub-protocol transfer exceeds the buffer

- **Trigger:** an I2C write-then-read whose write or read count is larger than
  the 4096-byte terminal output buffer.
- **Symptom:** a single `0x00` failure with no bus traffic.
- **Behaviour:** both 16-bit counts have already been consumed when the check
  runs; only the **payload** bytes are untouched.  A zero write count is also
  refused, because the write payload must include the address byte.  The limit
  is 4096 bytes on both v3 and v4.  This is the terminal output buffer, not the
  128/256-byte command line.
- **Recovery:** split the transfer into chunks within the limit.

## A bulk count larger than sixteen

- **Trigger:** a host tries to send more than sixteen bytes in one `0x1x`
  request, or pads the payload to a round number.
- **Symptom:** the extra bytes are read as the next request, often as
  configure or speed commands.
- **Behaviour:** the low nibble is the count minus one; the mode consumes
  exactly `n + 1` bytes and then returns to its command loop.
- **Recovery:** send one `0x1x` per group of up to sixteen bytes and keep the
  payload exactly `n + 1` bytes.

## An SMPS stop byte sent after leaving raw wire

- **Trigger:** the host sends `0xF1` to stop the supply after leaving `RAW1`
  for `BBIO1`.
- **Symptom:** the target supply and pull-ups may switch **on**, and the pin
  latches change.
- **Behaviour:** in BBIO1, `0xF1` is a pin state byte: target supply on,
  pull-ups on, AUX and CS latched high, MOSI/CLK/MISO latched low.  It is not
  an SMPS command, the SMPS keeps running, and the pins stay inputs until a
  direction byte makes them outputs.
- **Recovery:** return to `RAW1` and send `0xF1` there, or send `0x0F` from
  BBIO1 for the terminal reset, which stops the SMPS on an SMPS-enabled build.

## XSVF player aborts a stalled stream

- **Trigger:** starting an XSVF run (`0x18`, then `0x03`) and stopping the host
  mid-stream.
- **Symptom:** the Bus Pirate waits about five seconds during a chunk wait,
  then sends result byte `0x04` (illegal command — the forced `0xFF` is read
  as the next command) and returns to the player loop.
- **Behaviour:** the player aborts the run instead of parking the port.  A byte
  sent while it is in a timed wait (`XWAIT`/`XRUNTEST`) aborts with `0x01`; a
  zero-length chunk is re-requested.
- **Recovery:** send `0x0F` to leave the player (back to BBIO1, silently), then
  `0x0F` again for the terminal.  See [../jtag/xsvf.md](../jtag/xsvf.md).

## AVRISP-MKII mode ends by itself

- **Trigger:** entering an AVRISP-MKII personality (`0x08` or `0x09`) and then
  leaving the host idle.
- **Symptom:** the link changes back — a USB serial port re-appears, or the
  framed route stops answering; `BBIO1` is printed on the way out.
- **Behaviour:** the personality ends after about **five seconds** without host
  traffic, and a hardware reset also leaves it.  It is accepted only from
  BBIO1 and only on v4.
- **Recovery:** from the returned `BBIO1`, send `0x08` or `0x09` again, or
  `0x0F` for the terminal.  See
  [the AVRISP-MKII entry page](../../programmers/avrisp/entry.md).

## A command is refused on v3

- **Trigger:** sending an entry or group that the v3 build does not include
  (`0x08`/`0x09` AVRISP-MKII, `0x18` XSVF, or the `0x5x` pull-up feed **inside
  a sub-protocol**).
- **Symptom:** a single `0x00` failure byte instead of the expected identifier
  or reply; for the AVRISP entries the `0x00` is followed by `BBIO1`; for XSVF
  the layer simply stays in BBIO1.
- **Behaviour:** the byte is routed but the feature is not compiled in, so it
  is refused.  In BBIO1, `0x5x` is a direction byte, not a refused command.
- **Recovery:** none on v3; the feature requires a v4 board.  See the
  availability tables in [commands.md](commands.md) and
  [sub-protocols.md](sub-protocols.md).

## Garbage before the terminal banner

- **Trigger:** leaving the layer with `0x0F` on v3.
- **Symptom:** stray bytes arrive between the `0x01` acknowledgement and the
  version banner.
- **Behaviour:** the v3 hardware reset re-initialises the serial link; the host
  should wait for the banner before sending terminal commands.  On v4 the
  terminal reset prints the banner directly on the same link.
- **Recovery:** discard bytes until the banner (then the prompt) is seen.

## Related docs

- [entering-and-framing.md](entering-and-framing.md) — entry and framing.
- [commands.md](commands.md) — the BBIO1 command map.
- [sub-protocols.md](sub-protocols.md) — the entries and shared groups.
- [generic/edge-cases.md](../../generic/edge-cases.md) — terminal-side failure
  modes.
