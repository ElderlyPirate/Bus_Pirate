# 3WIRE — edge cases and failure modes

Part of the 3WIRE documentation set (`docs/3wire/`).  Each entry is a concrete
trigger → symptom → behaviour → recovery.

## CS is driven opposite to its label

- **Trigger:** the default `/CS` polarity is left in place and the target needs
  CS **asserted low** during a transaction.
- **Symptom:** the target never selects; reads return idle/floating values and
  the device ignores every write.
- **Behaviour:** on the current firmware the CS levels are the reverse of the
  label — with `/CS`, `[`/`{` drive CS **high** and `]`/`}` drive it **low**
  (see [bus-operations.md](bus-operations.md) §Which level is driven).
- **Recovery:** re-enter the mode and choose option **`CS`** (menu `1`) so that
  `[`/`{` drive CS low; or drive CS by hand over the binary `RAW1` path
  (`0x04`/`0x05`), which does not have this mismatch.  **(not verified.)**

## Misreading 3WIRE as SPI

- **Trigger:** a real SPI target is wired up and the 3WIRE mode is used.
- **Symptom:** no clock-polarity/phase control, no byte command set, no
  sniffer, and reads do not line up with the target's SPI framing.
- **Behaviour:** 3WIRE is a bare bit bus; it has no SPI engine and adds no
  framing on top of the four lines.
- **Recovery:** use the **SPI** mode for SPI targets; 3WIRE is for
  non-standard, hand-timed buses.

## Floating lines — open drain without a pull-up

- **Trigger:** open-drain output selected, no pull-up (or no voltage on VPU).
- **Symptom:** reads return arbitrary values (`0x00`, `0xFF`, or noise) and
  nothing is flagged.
- **Behaviour:** the mode's CS control does **not** check the released lines,
  so there is no short/pull-up warning (unlike I2C).  `P` may print
  `Warning: no voltage on Vpullup pin` when VPU is unpowered.
- **Recovery:** add a 2 kΩ–10 kΩ pull-up to the pull-up voltage and feed VPU
  (v4 `e2`/`e3`, v3 externally), or enter the mode with **normal** output if
  the target tolerates driven lines.

## `P` with normal (driven) output

- **Trigger:** the mode was entered with output type `2` (normal), then `P`.
- **Symptom:** `WARNING: pins not open drain (HiZ)` before
  `Pull-up resistors ON`.
- **Behaviour:** the pull-ups are switched on, but driven-high pins ignore
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

## `-` / `.` do not show the same line

- **Trigger:** `-` (data high) is issued and then `.` is expected to report
  "1".
- **Symptom:** `.` reports `DATA STATE: 0` (or a floating level) even though the
  data-out line was just driven high.
- **Behaviour:** the data-out line is MOSI and the data-in line is MISO — they
  are separate wires, so `.` samples MISO and never reflects what `-`/`_` drove
  on MOSI.
- **Recovery:** to read back your own output, wire MOSI to MISO externally (a
  loopback) or use `!`, which samples MISO; otherwise check the target's
  output on MISO.

## Expecting macros

- **Trigger:** `(0)` is issued expecting a macro menu.
- **Symptom:** `ERROR: command has no effect here` followed by a
  `Syntax error at char <n>` line, instead of a menu.
- **Behaviour:** the mode has **no macros by design**; its macro handler is
  intentionally empty.
- **Recovery:** build the sequence from base-layer **user macros** `<n>`, drive
  it from a host script (terminal or binary `RAW1`), or use SPI for SPI
  targets.  See [bus-operations.md](bus-operations.md) §No macros.

## Looking for a bus reset

- **Trigger:** expecting a `BUS RESET` command (as in 1-Wire).
- **Symptom:** no such command exists — there is no reset output for it.
- **Behaviour:** `BUS RESET` is a 1-Wire string; 3WIRE has no bus-reset or
  device-search command.
- **Recovery:** use the base layer's `#` reset, or re-enter the mode; to signal
  a target reset, drive CS or a data line by hand through a `[`/`{` … `]`/`}`
  wrapper or the bitwise controls.

## Width above 16

- **Trigger:** `;N` with N greater than 16 (`0x55;17`).
- **Symptom:** `Syntax error at char <n>`; the command does not run.
- **Behaviour:** the value width is limited to 16 bits; a larger width is
  rejected (the hardware shift would silently zero every bit).
- **Recovery:** use a width of 1–16, or split the transfer into multiple
  values.

## `Maximum speed` is too fast for the target

- **Trigger:** the `Maximum speed` profile (`4`) against a slow device.
- **Symptom:** the target does not respond or returns wrong data.
- **Behaviour:** the top profile applies no deliberate delay, so it clocks as
  fast as the firmware can; `/`, `\`, `-` and `_` then carry almost no settle
  time.
- **Recovery:** drop to a lower speed profile and re-enter the mode.

## Read returns a floating value

- **Trigger:** `r` with nothing (or nothing driving MISO).
- **Symptom:** `READ: 0x00` (or `0xFF`, or noise) with no error.
- **Behaviour:** the read clocks the bus and captures whatever MISO presents;
  the mode does not validate the line.
- **Recovery:** confirm the target is powered (`W`), selected (CS) and actually
  driving MISO; check the idle level with `.` and the pin table with `v`.