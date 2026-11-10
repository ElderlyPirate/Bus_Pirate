# Binary layer — edge cases and failure modes

Part of the binary-layer documentation set (`docs/binary/`).  Each entry is a
concrete trigger, the observable symptom, the current behaviour, and the
recovery.  Entry and framing are in
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
- **Behaviour:** a single accidental NUL no longer enters the layer; twenty
  are required.
- **Recovery:** none needed.

## Unknown or reserved command byte

- **Trigger:** the host sends a byte that is not a defined command (for
  example a reserved BBIO1 value, or a group a sub-protocol does not
  implement).
- **Symptom:** a single `0x00` failure byte appears in the reply stream.
- **Behaviour:** the command is refused; the layer stays in its current mode
  and keeps reading.  There is no other error signal and no framing marker.
- **Recovery:** the host must track how many reply bytes each command returns
  and drain exactly those; a lost byte count makes later replies align
  wrong — reset with `0x0F` and re-enter.

## Host and firmware drift to different layers

- **Trigger:** the host skips or miscounts a reply byte, or leaves a
  sub-protocol on one side only.
- **Symptom:** replies no longer match the expected command shapes; text from
  the terminal can appear mid-stream.
- **Behaviour:** each mode reads its own fixed request layout; a stray operand
  byte is interpreted as the next request.
- **Recovery:** send `0x00` to force a return to `BBIO1` (it re-prints
  `BBIO1`), then `0x0F` to reset to the terminal, and re-enter.

## PWM stays on after the layer is left

- **Trigger:** setting up PWM (`0x12`) and then leaving the binary layer
  without clearing it.
- **Symptom:** the target keeps being driven on the AUX pin after `0x0F` and
  the banner.
- **Behaviour:** PWM remains active across leaving the layer; it is not turned
  off on exit.
- **Recovery:** send `0x13` to clear PWM before leaving, or reset the board.

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

## Sub-protocol transfer exceeds the buffer

- **Trigger:** an I2C write-then-read whose write or read count is larger than
  the terminal buffer (4096 bytes on v4, 2048 on v3).
- **Symptom:** a single `0x00` failure with no bus traffic.
- **Behaviour:** the counts are checked at the start; an over-limit request is
  refused before any byte is consumed or any bus activity happens.  (The SPI
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
- **Behaviour:** the personality ends after a few seconds without host
  traffic, and a hardware reset also leaves it.
- **Recovery:** re-enter the personality from `BBIO1`.  See
  [../avrisp/README.md](../../programmers/avrisp/README.md).

## A command is refused on v3

- **Trigger:** sending an entry or group that the v3 build does not include
  (`0x08`/`0x09` AVRISP, `0x18` XSVF, or the `0x5x` pull-up feed).
- **Symptom:** a single `0x00` failure byte instead of the expected
  identifier or reply.
- **Behaviour:** the byte is routed but the feature is not compiled in, so it
  is refused.
- **Recovery:** none on v3; the feature requires a v4 board.  See the
  availability tables in [commands.md](commands.md) and
  [sub-protocols.md](sub-protocols.md).

## Garbage before the terminal banner

- **Trigger:** leaving the layer with `0x0F`.
- **Symptom:** stray bytes arrive between the `0x01` acknowledgement and the
  version banner.
- **Behaviour:** the serial link re-initialises during the reset; the host
  should wait for the banner before sending terminal commands.
- **Recovery:** discard bytes until the banner (or `HiZ>`) is seen.

## Related docs

- [entering-and-framing.md](entering-and-framing.md) — entry and framing.
- [commands.md](commands.md) — the BBIO1 command map.
- [sub-protocols.md](sub-protocols.md) — the sub-protocol entries.
- [../generic/edge-cases.md](../../generic/edge-cases.md) — terminal-side failure
  modes.