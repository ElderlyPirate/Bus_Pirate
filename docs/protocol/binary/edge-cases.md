# Binary layer — edge cases and failure modes

Part of the binary-layer documentation set (`docs/protocol/binary/`).  Each
entry is a concrete trigger, the observable symptom, the current behaviour, and
the recovery.  Entry and framing are in
[entering-and-framing.md](entering-and-framing.md).

## Entry does not happen — fewer than 20 NUL bytes

- **Trigger:** the host sends fewer than twenty `0x00` bytes (or the terminal
  is in a menu that consumes them differently).
- **Symptom:** no `BBIO1` reply; the terminal is unchanged.
- **Behaviour:** the layer is entered only after twenty NULs; there is no
  partial-entry state and no error reply.
- **Recovery:** press Enter a few times, send `#` to reset to the command
  line, then send exactly twenty or more NUL bytes and wait for `BBIO1`.

## A stray NUL at power-up

- **Trigger:** some terminals emit a NUL byte when the port opens.
- **Symptom:** none — the terminal is not interrupted.
- **Behaviour:** a single accidental NUL does not enter the layer; twenty are
  required.
- **Recovery:** none needed.

## Unknown or reserved command byte

- **Trigger:** the host sends a byte that is not a defined command (for
  example a reserved BBIO1 value, or a group a sub-protocol does not
  implement).
- **Symptom:** a single `0x00` failure byte appears in the reply stream.
- **Behaviour:** the command is refused; the layer stays in its current mode
  and keeps reading.  There is no other error signal and no framing marker.
- **Recovery:** drain exactly the reply bytes each command returns.  If a lost
  byte count makes later replies align wrong, send `0x00` to return to
  `BBIO1` (repeat until `BBIO1` appears), then send `0x0F` to reach the
  terminal and re-enter.  Do **not** send `0x0F` from inside UART: in that
  mode `0x0F` enters the transparent bridge instead of resetting.  Leave UART
  with `0x00` first.

## Host and firmware drift to different layers

- **Trigger:** the host skips or miscounts a reply byte, or leaves a
  sub-protocol on one side only.
- **Symptom:** replies no longer match the expected command shapes; text from
  the terminal can appear mid-stream.
- **Behaviour:** each mode reads its own fixed request layout; a stray operand
  byte is interpreted as the next request.
- **Recovery:** send `0x00` to force a return to `BBIO1` (it re-prints
  `BBIO1`), then `0x0F` to reset to the terminal, and re-enter.  Inside UART,
  `0x0F` is the bridge command — send `0x00` first to leave UART, and only
  then `0x0F`.

## A `0x4x` / `0x5x` byte sent in BBIO1 changes pin directions

- **Trigger:** a host sends a sub-protocol configure byte (for example `0x4C`
  or `0x51`) while still in BBIO1.
- **Symptom:** the reply is a pin-level byte (`0x00`–`0x1F`), not `0x01`, and
  the bus lines change direction.
- **Behaviour:** in BBIO1 the whole `0x40`–`0x5F` range is the pin direction
  byte; the `0x4x` and `0x5x` groups exist only inside a sub-protocol.
- **Recovery:** enter the sub-protocol first, then send the configure byte.
  Re-assert the intended directions if the bus is now misconfigured.

## A command waits for payload bytes that never arrive

- **Trigger:** a payload command is cut short — for example `0x12` (PWM)
  without all five bytes, or `0x70` without the selector and two timeout
  bytes.
- **Symptom:** the Bus Pirate does not reply and does not process later bytes
  as commands.
- **Behaviour:** the handler reads the full payload before it can reply or
  validate anything; the missing bytes block the loop.
- **Recovery:** send the missing bytes to complete the frame, or reset the
  board and re-enter.

## PWM after the layer is left

- **Trigger:** setting up PWM (`0x12`) and then leaving the binary layer with
  `0x0F` without clearing it.
- **Symptom:** on v4 the target stops being driven at the reset; the pin is no
  longer a PWM output.
- **Behaviour:** `0x0F` does not clear the PWM timer setup, and the firmware
  does not explicitly stop the timer on this path.  On v4 the terminal reset
  detaches the AUX output source, so the target stops.  On v3 `0x0F` is a
  hardware reset.
- **Recovery:** send `0x13` before leaving if you want the PWM module idle;
  otherwise the detached output stays inactive.

## Frequency count stops a running PWM

- **Trigger:** sending `0x16` while PWM is running.
- **Symptom:** the PWM output disappears and `A0` becomes an input.
- **Behaviour:** `0x16` does not refuse; it shuts the timers down, forces `A0`
  to an input and measures.  The terminal `f` command refuses while PWM is
  active instead.
- **Recovery:** send `0x13` to clear PWM deliberately, then measure; re-send
  `0x12` to restart PWM afterwards.

## Continuous ADC does not stop on its own

- **Trigger:** starting continuous readings (`0x15`) and waiting for it to
  finish.
- **Symptom:** an open-ended stream of 2-byte samples.
- **Behaviour:** the stream runs until the host sends a byte; that byte is
  consumed to stop it (it is not a command).
- **Recovery:** send one byte to stop; then continue with the next real
  command.

## Wait-for-edge returns "no edge" immediately

- **Trigger:** a wait with a timeout of `0` and a pin that is already at rest.
- **Symptom:** result byte `0x00` (timeout) with the current level, even with
  no edge expected.
- **Behaviour:** a `0` timeout checks the pin once and returns without
  waiting; this is the documented way to sample the AUX level.
- **Recovery:** none — use a non-zero timeout to actually wait.

## Invalid wait selector with no timeout

- **Trigger:** sending `0x70 0x00` and stopping; the selector is invalid and
  the two timeout bytes are missing.
- **Symptom:** no reply; the layer waits.
- **Behaviour:** the selector **and** the 16-bit timeout are always consumed
  before the selector is validated.  The single `0x00` failure therefore comes
  only after the full frame.
- **Recovery:** send the two timeout bytes (any value) to complete the frame,
  or reset and re-enter.

## Sub-protocol transfer exceeds the buffer

- **Trigger:** an I2C write-then-read whose write or read count is larger than
  the terminal buffer (4096 bytes on v4, 2048 on v3).
- **Symptom:** a single `0x00` failure with no bus traffic.
- **Behaviour:** the two 16-bit counts have already been consumed when the
  check runs; only the **payload** bytes are untouched.  (The SPI
  write-then-read path does not apply this terminal-buffer check.)
- **Recovery:** split the transfer into chunks within the limit.

## XSVF player blocks with no timeout

- **Trigger:** starting an XSVF run (`0x18`, then `0x03`) and stopping the
  host mid-stream.
- **Symptom:** the Bus Pirate waits for the next length-prefixed chunk and
  does not answer.
- **Behaviour:** the player has no timeout; it blocks until the host answers
  its ready byte or the board is reset.
- **Recovery:** reset the board, then re-enter the binary layer.  See
  [../jtag/xsvf.md](../jtag/xsvf.md).

## AVRISP mode ends by itself

- **Trigger:** entering an AVRISP personality (`0x08` or `0x09`) and then
  leaving the host idle.
- **Symptom:** the link changes back — a USB serial port re-appears, or the
  framed route stops answering.
- **Behaviour:** the personality ends after about **five seconds** without
  host traffic, and a hardware reset also leaves it.
- **Recovery:** re-enter the personality from `BBIO1`.  See
  [the AVRISP-MKII set](../../programmers/avrisp/README.md).

## A command is refused on v3

- **Trigger:** sending an entry or group that the v3 build does not include
  (`0x08`/`0x09` AVRISP, `0x18` XSVF, or the `0x5x` pull-up feed **inside a
  sub-protocol**).
- **Symptom:** a single `0x00` failure byte instead of the expected
  identifier or reply; for the AVRISP entries the `0x00` is followed by
  `BBIO1`.
- **Behaviour:** the byte is routed but the feature is not compiled in, so it
  is refused.  In BBIO1, `0x5x` is a direction byte, not a refused command.
- **Recovery:** none on v3; the feature requires a v4 board.  See the
  availability tables in [commands.md](commands.md) and
  [sub-protocols.md](sub-protocols.md).

## Garbage before the terminal banner

- **Trigger:** leaving the layer with `0x0F` on v3.
- **Symptom:** stray bytes arrive between the `0x01` acknowledgement and the
  version banner.
- **Behaviour:** the v3 hardware reset re-initialises the serial link; the
  host should wait for the banner before sending terminal commands.  On v4 the
  terminal reset prints the banner directly on the same link.
- **Recovery:** discard bytes until the banner (then the prompt) is seen.

## Related docs

- [entering-and-framing.md](entering-and-framing.md) — entry and framing.
- [commands.md](commands.md) — the BBIO1 command map.
- [sub-protocols.md](sub-protocols.md) — the sub-protocol entries.
- [generic/edge-cases.md](../../generic/edge-cases.md) — terminal-side
  failure modes.
