# DIO — edge cases & failure modes

Part of the DIO documentation set (`docs/protocol/dio/`).  Each entry gives
the trigger, the observable symptom, what the firmware does, and the recovery.

## Supply and pull-ups

### `w` does not park the pins

- Trigger: `w` in DIO.
- Symptom: `Clutch disengaged!!!` + `POWER SUPPLIES OFF`; the prompt stays
  `DIO>`; a pin that was an output stays an output.
- Behaviour: `w` only switches the supply off and restores the 8-bit
  read/write width.  Directions, levels and pull-ups are unchanged.  `#` is
  the command that parks all pins as inputs and returns to `HiZ` (on v3 `#`
  reboots the firmware).
- Recovery: send a direction value with the pins you need as inputs (bit = 1;
  `0x1F` makes all five inputs), or use `#`.  No re-entry is needed.

### A state value clears POWER or PULLUP

- Trigger: after `W`, send a state value with bit 6 clear — for example
  `0x80` or `0x87`.
- Symptom: the next `v` shows the rails at `0.00V`; `POWER SUPPLIES ON` is
  not printed again.
- Behaviour: every state value carries POWER (bit 6) and PULLUP (bit 5); the
  whole byte is applied, so a clear bit 6 disables the supply and a clear
  bit 5 releases the pull-ups.
- Recovery: include the bits you want.  `0xC0 | pins` keeps power on and
  leaves the pull-ups off; `0xE0 | pins` keeps power on and enables the
  pull-ups.  Or send `W` again.

### `0xE0` also enables the pull-ups

- Trigger: using `0xE0 | pins` to keep power on while changing a pin.
- Symptom: the pull-ups turn on as well; they may have no high reference.
- Behaviour: bit 5 adds the pull-ups.  Keeping power on needs only bit 6, so
  `0xC0 | pins` is the power-only recipe.
- Recovery: use `0xC0 | pins` when the pull-ups must stay off; feed VPU if
  you want them on.

### Pull-ups on with no VPU reference

- Trigger: `P` when nothing is on the VPU pin, or a state value with bit 5
  set.
- Symptom: `P` prints `WARNING: pins not open drain (HiZ)`, then
  `Pull-up resistors ON`, then `Warning: no voltage on Vpullup pin`.  A state
  value enables the resistors with no warning.
- Behaviour: `P` samples VPU once after enabling and warns if it is low.  The
  state bit only switches the resistors.  Either way the lines have no high
  reference until VPU is fed.
- Recovery: feed VPU externally (both boards), then enable the pull-ups.  A
  pull-up is optional for DIO.

### `e` is refused in DIO

- Trigger: `e`, `e2` or `e3` in DIO on v4.
- Symptom: `WARNING: pins not open drain (HiZ)`; nothing changes.
- Behaviour: DIO drives push-pull only.  `e` is accepted only in a mode whose
  output type is open drain, and entering DIO clears any source selected
  earlier.
- Recovery: feed VPU from an external supply.  There is no on-board VPU route
  from DIO on either board.

### `W` with a short or no supply

- Trigger: `W` with a shorted rail or no supply.
- Symptom: `VREG too low, is there a short?` followed by
  `POWER SUPPLIES OFF`.
- Behaviour: the supply is switched off; the pin state is unchanged.
- Recovery: clear the short (or connect the supply), then `W` again.

## Pin writes

### A level is written while the pin is still an input

- Trigger: a state write for a pin whose direction is still input.
- Symptom: the pin does not change on `v`; the level appears only after the
  pin becomes an output.
- Behaviour: a level is driven only when the direction bit is 0 (output).
  The state bit is stored but the pin stays Hi-Z.
- Recovery: send the direction value first, then the state value.  For
  example `0x00` makes all five outputs.

### A value larger than a byte

- Trigger: a value above `0xFF`, for example `0x1FF`.
- Symptom: only the low byte is applied and echoed.
- Behaviour: only the low byte reaches the pin-control function, and only
  bit 7 of that byte selects direction versus state.
- Recovery: send a value in `0x00`–`0xFF`.

### A binary command code is applied as a pin value

- Trigger: a binary-layer command code such as `0x01` (SPI) or `0x0F`
  (return to terminal).
- Symptom: no mode change; a pin value is applied instead.
- Behaviour: the binary command codes are interpreted by the binary layer,
  not by DIO.  In DIO every value is a direction (`0x00`–`0x7F`) or state
  (`0x80`–`0xFF`) write, so `0x01` sets `CS` as an input.
- Recovery: none needed; use the numeric direction/state values.

### An output fights an external driver

- Trigger: a pin is an output while an external driver holds the same net at
  the opposite level.
- Symptom: excessive current; the target or the Bus Pirate pin may be
  damaged.  The firmware gives no warning.
- Behaviour: DIO does not detect contention; an output drives the pin
  regardless of what is on the net.
- Recovery: set the pin to input (direction bit = 1) before connecting an
  external driver; never drive a pin against a supply.

### The v4 `AUX1` / `AUX2` pins do not respond

- Trigger: driving the v4 `AUX1` or `AUX2` header pin with a DIO value.
- Symptom: no bit in the value changes those pins; they stay inputs.
- Behaviour: a DIO value only addresses `AUX` (`AUX0`), `MOSI`, `CLK`,
  `MISO` and `CS`.
- Recovery: on v4, reach those pins through the AUX route — select the target
  with `c` / `C` / `k` / `K`, then drive it with `a` / `A` (see
  [commands.md](../../generic/commands.md) §Elements).

## Reads and commands

### `r` does not show direction or rails

- Trigger: expecting `r` to show a pin's direction, the supply or the
  pull-ups.
- Symptom: `r` returns only the five levels; the value looks like a write
  byte but bits 7–5 are clear.
- Behaviour: `r` is a level read in the compact layout (bits 4–0, 1 = high).
  Direction, power and pull-up are not part of it.
- Recovery: use `v` for directions and rails; see
  [read-back.md](read-back.md).

### Wrappers or mode macros report an error

- Trigger: `[`, `{`, `]`, `}`, `/`, `\`, `-`, `_`, `.`, `^`, `!`, or `(n)`.
- Symptom: `ERROR: command has no effect here` followed by
  `Syntax error at char N`.
- Behaviour: DIO has no start/stop, clock, data, bit-read or macro callbacks,
  so these shared commands do nothing and flag an error.  Shared user macros
  `<n>` still run.
- Recovery: none needed; use the value commands, `r`/`v`, and `<n>`.
