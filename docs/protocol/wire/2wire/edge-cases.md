# 2WIRE — edge cases and failure modes

Part of the 2WIRE documentation set (`docs/2wire/`).  Each entry is a concrete
trigger → symptom → behaviour → recovery.

## Floating lines — open drain without a pull-up

- **Trigger:** open-drain output selected, no pull-up (or no voltage on VPU).
- **Symptom:** reads return arbitrary values (`0x00`, `0xFF`, or noise), an ATR
  returns meaningless bytes, and nothing is flagged.
- **Behaviour:** the mode's start/stop do **not** check the released lines, so
  there is no short/pull-up warning (unlike I2C).  `P` may print
  `Warning: no voltage on Vpullup pin` when VPU is unpowered.
- **Recovery:** add a 2 kΩ–10 kΩ pull-up to the pull-up voltage and feed VPU
  (v4 `e2`/`e3`, v3 externally), or enter the mode with **normal** output if
  the target tolerates a driven line.

## `P` with normal (driven) output

- **Trigger:** the mode was entered with output type `2` (normal), then `P`.
- **Symptom:** `WARNING: pins not open drain (HiZ)` before
  `Pull-up resistors ON`.
- **Behaviour:** the pull-ups are switched on, but a driven-high pin ignores
  them, so they have no effect on the bus.
- **Recovery:** if a pull-up is actually needed, leave the mode and re-enter
  with output type `1` (open drain).  See
  [entry-and-setup.md](entry-and-setup.md).

## No voltage on VPU

- **Trigger:** `P` while nothing is on the Vpullup pin.
- **Symptom:** `Pull-up resistors ON` then
  `Warning: no voltage on Vpullup pin`; the open-drain bus has no idle-high.
- **Behaviour:** the pull-ups are enabled but reference a floating VPU pin.
- **Recovery:** on v4 select an on-board source (`e2` 3.3 V / `e3` 5.0 V) and
  re-run `P`; on v3 feed the Vpullup pin externally.  See
  [wiring.md](wiring.md).

## ATR with no card or no answer

- **Trigger:** `(1)` or `(2)` with no card, no power, or the reset line not
  wired.
- **Symptom:** four values print (often `0x00`/`0xFF` or noise) and the decode
  lines follow — **no error**.
- **Behaviour:** the mode does not sense a card; it just reads whatever the
  lines present, and the parse reads exactly four values.
- **Recovery:** confirm VCC (`W`), the pull-up (`P`), and the reset wiring
  (CS → card RST).  Use `.` to confirm the idle-high level before running the
  macro.  See [smartcard.md](smartcard.md).

## ATR reply length is not four values

- **Trigger:** the card's ATR is longer or shorter than the four values the
  macro reads.
- **Symptom:** the decoded fields look wrong and the bus is left mid-stream,
  so a following command starts from the wrong point.
- **Behaviour:** the parse always reads exactly four values and never drains a
  longer reply.
- **Recovery:** re-enter the mode (or reset) to realign the bus, then decide how
  to handle the extra bytes.

## ATR values bit-reversed

- **Trigger:** LSB-first order is selected (`L`) before the ATR macro.
- **Symptom:** the printed reply header notes
  `uses current LSB setting`, and each value is bit-reversed.
- **Behaviour:** the parse reverses each read value under LSB-first order.
- **Recovery:** select MSB-first (`l`) before running the ATR macro.

## Width changed before the ATR macro

- **Trigger:** `;N` set a width other than 8, then `(1)`/`(2)`.
- **Symptom:** each "value" the parse reads is N bits, so the decode is wrong.
- **Behaviour:** the parse assumes the default 8-bit width.
- **Recovery:** re-enter the mode (leaving resets the width to 8) or set the
  width back before running the macro.

## Silently wrong bus with no pull-up

- **Trigger:** open drain with no pull-up; a start/stop or read is sent.
- **Symptom:** no warning, but the condition or data is not what a real bus
  would show.
- **Behaviour:** 2WIRE does not validate the released lines (again unlike I2C).
- **Recovery:** check the data pin with `.` and the pin table with `v`; add the
  pull-up before trusting a read.

## `Maximum speed` is too fast for the target

- **Trigger:** the `Maximum speed` profile (`4`) against a slow device.
- **Symptom:** the target does not respond or returns wrong data.
- **Behaviour:** the top profile applies no deliberate delay, so it clocks as
  fast as the firmware can.
- **Recovery:** drop to a lower speed profile and re-enter the mode.

## Looking for a bus reset

- **Trigger:** expecting a 2WIRE `BUS RESET` macro.
- **Symptom:** `(0)` lists only the ATR macros.
- **Behaviour:** 2WIRE has no bus-reset macro; `BUS RESET` is a 1-Wire string.
- **Recovery:** use the base layer's `#` reset, or reset the board state by
  re-entering the mode.  See [macros.md](macros.md).