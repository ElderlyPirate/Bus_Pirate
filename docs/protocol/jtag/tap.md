# JTAG — the TAP, registers and scans

Part of the JTAG documentation set.  This page explains the TAP state
machine, how the two Bus Pirate paths drive it, and what happens during
instruction-register (IR) and data-register (DR) shifts, chain scans,
IDCODE reads, BYPASS scans and resets.  Byte framing is in
[openocd.md](openocd.md) and [xsvf.md](xsvf.md); pins are in
[wiring.md](wiring.md).

## Scope

- The 16-state TAP and its TMS transitions.
- How the OpenOCD path and the XSVF player drive the TAP.
- IR/DR shifts, chain scan/IDCODE, BYPASS and the reset variants.
- Not the host-tool setup (tools/openocd, tools/xsvf) and not the byte
  framing of either path.

## The TAP in one paragraph

Every JTAG device has a test access port (TAP) controller: a 16-state
machine clocked by TCK.  TMS selects the next state on each rising edge;
TDI supplies serial data and TDO returns it.  The machine has two parallel
paths — one for the instruction register (IR) and one for data registers
(DR) — joined at Run-Test/Idle.  A reset puts every TAP in the chain into
Test-Logic-Reset and selects the device's reset instruction (IDCODE on most
devices that have one).

## The 16 states

| State | Path | Purpose |
|---|---|---|
| Test-Logic-Reset | reset | all test logic reset; reset instruction selected |
| Run-Test/Idle | common | idle; some instructions run tests here |
| Select-DR-Scan | DR | entry to the DR path |
| Capture-DR | DR | parallel-load the selected DR |
| Shift-DR | DR | shift the DR between TDI and TDO |
| Exit1-DR | DR | first exit from Shift-DR |
| Pause-DR | DR | pause the DR shift |
| Exit2-DR | DR | second exit from Pause-DR |
| Update-DR | DR | latch the shifted DR |
| Select-IR-Scan | IR | entry to the IR path |
| Capture-IR | IR | load status bits into the IR shift path |
| Shift-IR | IR | shift the IR between TDI and TDO |
| Exit1-IR | IR | first exit from Shift-IR |
| Pause-IR | IR | pause the IR shift |
| Exit2-IR | IR | second exit from Pause-IR |
| Update-IR | IR | latch the new instruction |

## TMS transitions

TMS is sampled on every rising TCK edge.  `0` selects the left column and
`1` the right column.

| Current state | TMS = 0 → | TMS = 1 → |
|---|---|---|
| Test-Logic-Reset | Run-Test/Idle | Test-Logic-Reset |
| Run-Test/Idle | Run-Test/Idle | Select-DR-Scan |
| Select-DR-Scan | Capture-DR | Select-IR-Scan |
| Capture-DR | Shift-DR | Exit1-DR |
| Shift-DR | Shift-DR | Exit1-DR |
| Exit1-DR | Pause-DR | Update-DR |
| Pause-DR | Pause-DR | Exit2-DR |
| Exit2-DR | Shift-DR | Update-DR |
| Update-DR | Run-Test/Idle | Select-DR-Scan |
| Select-IR-Scan | Capture-IR | Test-Logic-Reset |
| Capture-IR | Shift-IR | Exit1-IR |
| Shift-IR | Shift-IR | Exit1-IR |
| Exit1-IR | Pause-IR | Update-IR |
| Pause-IR | Pause-IR | Exit2-IR |
| Exit2-IR | Shift-IR | Update-IR |
| Update-IR | Run-Test/Idle | Select-DR-Scan |

## Reset and idle

- Test-Logic-Reset is reached from any state by holding TMS high for at
  least five TCK rising edges.  The TAP stays there while TMS stays high.
- One TCK with TMS low moves from Reset to Run-Test/Idle.
- The XSVF player's `0x01` reset holds TMS high for ten clocks.  Its
  internal `XSTATE RESET` uses one transition plus five clocks.  The
  OpenOCD path leaves the reset sequence to the host.
- TRST, where fitted, resets the test logic independently of TCK.  It is
  separate from the TMS sequence.

## Reaching a shift

| Target | TMS sequence from Run-Test/Idle |
|---|---|
| Shift-DR | `1, 0, 0` |
| Shift-IR | `1, 1, 0, 0` |
| Update-DR from Shift-DR | `1, 1` |
| Update-IR from Shift-IR | `1, 1` |
| Run-Test/Idle from Update | `0` |

## Shift rules

- TDI and TMS are presented before the rising TCK edge; the target samples
  them on that edge.  TDO is updated by the target on the falling edge and
  is stable until the next falling edge; the host reads it between edges.
- Data is shifted **least-significant bit first**.
- The IR length is the sum of the IR lengths of every device in the chain.
  The DR length is set by the current instruction and is also the sum
  across the chain.
- A shift stays in Shift-IR or Shift-DR while TMS is low.  The last bit
  must be clocked with TMS high to enter Exit1; otherwise the TAP keeps
  shifting and the bit count is wrong.
- Update-IR makes the new instruction active.  Update-DR latches the data.
- Capture-DR loads the selected register.  Capture-IR loads device status
  bits; their meaning is device-specific.

## How each path drives the TAP

| Job | OpenOCD path | XSVF player |
|---|---|---|
| TCK, TMS, TDI | host supplies every bit through `0x05` | player supplies them from the file |
| TAP state | host tracks and walks it | player tracks all 16 states |
| IR data | host target configuration | `XSIR` / `XSIR2` in the file |
| DR length | host target configuration | `XSDRSIZE` in the file |
| Reset | host TMS sequence; TRST feature optional | `0x01`, `XSTATE RESET`, or run start |
| Chain discovery | host scan | `0x02` probe |
| Clock | `0x08` delay | no setting |

## Chain scan and IDCODE

A JTAG chain connects the Bus Pirate's TDI to the first device, each
device's TDO to the next device's TDI, and the last TDO back to the Bus
Pirate's MISO.  The IR and DR of all devices form one shift register.  The
device nearest TDO is the first to drive TDO and the last to receive TDI.

To read IDCODEs:

1. Reset the TAP.
2. Load the IDCODE instruction, or rely on the reset default.  Most devices
   with an IDCODE select it after Test-Logic-Reset.
3. Shift 32 bits per device in the DR path.  The first 32 bits out are the
   IDCODE of the device nearest TDO; the next 32 bits are the next device,
   and so on.

This 32-bits-per-device rule assumes every device in the chain has a
32-bit IDCODE.  A device without one contributes its bypass bit instead;
check the chain's data sheets.

IDCODE is 32 bits, least-significant bit first:

| Bits | Field |
|---|---|
| 0 | always 1 |
| 1–11 | manufacturer identity |
| 12–27 | part number |
| 28–31 | version |

A device without an IDCODE may return zeros or a vendor value.  A chain
with TDO stuck low returns all zeros; stuck high returns all ones.  Neither
identifies a real device.

### The XSVF chain probe (`0x02`)

The player's probe is a convenience scan.  It assumes the reset default is
IDCODE on every device, and that the chain's total IR length is at most
510 bits.

1. Reset the TAP, then hold TDI high.
2. Shift 510 ones into the IR path.  All-ones selects BYPASS on IEEE 1149.1
   devices, so every device selects its one-bit bypass register.
3. Shift the DR path and count leading zeros until TDO goes high.  Each
   device contributes one bypass bit, so the count is the number of
   devices.
4. Reset the TAP, then shift the DR path and read 32 bits per device.
5. Reply: one count byte equal to 4 × devices, then 4 bytes per device,
   least-significant byte first, nearest-TDO device first.

The probe leaves the TAP in Run-Test/Idle.  Limits: the count loop stops
after about 250 devices; the reply is clamped to 63 devices because the
count is one byte (4 × 63 = 252 = `0xFC`).  A longer chain is truncated,
not wrapped.  With TDO stuck low the probe reaches the cap and reports 63
devices of garbage IDCODEs; with TDO stuck high it reports zero devices and
no IDCODE bytes.  The OpenOCD path has no equivalent command; the host
performs its own scan.

## BYPASS

BYPASS is the all-ones instruction.  It selects a one-bit shift register in
each device, so a chain of N devices has an N-bit DR.  Data shifted in
appears at TDO N clocks later; the register normally captures 0, and the
rest of the stream is the TDI data.

| Use | What it shows |
|---|---|
| chain length | N bypass bits = N devices |
| isolation | one device can be reached while the others are transparent |
| wiring check | TDI/TDO and TCK work without a device-specific instruction |

Apart from the `0x02` probe, the Bus Pirate does not select BYPASS by
itself.  The OpenOCD host loads it with an IR scan; an XSVF file does it
with `XSIR` followed by `XSDR`.

## Reset kinds

| Reset | What it resets | How it is applied |
|---|---|---|
| TAP reset (TMS) | TAP state machine and IR | TMS high for at least 5 TCK |
| TRST | test logic | optional pin; OpenOCD feature `0x04`; XSVF does not use it |
| SRST | system/CPU, not the TAP | optional pin; OpenOCD feature `0x08` |
| Bus Pirate board reset | pins, supplies, pull-ups | `#`, reset button, or second `0x0F` |
| XSVF `0x01` | TAP reset, then idle | player command |
| XSVF `0x0F` | TAP reset, then leave | player command |

TRST and SRST are driven to the level in the feature action, not inverted.
Both are commonly active-low, so write `0` to assert and `1` to release.
Selecting the OpenOCD pin mode drives both low; release them after the pin
mode if they are wired.

The OpenOCD path does not reset the target just by leaving.  `0x00` returns
to `BBIO1` and makes the pins inputs, but the target TAP only changes if it
sees TCK/TMS or TRST.  The XSVF `0x0F` does perform a TMS reset before it
returns.

## Worked TAP walk — one IDCODE read

A single-device chain on the OpenOCD path.  The sequence is 43 bits in one
`0x05` block.  Send the header, wait for the 3-byte echo, then send the
payload as three 4-byte groups.  Each group is TDI low byte, TMS low byte,
TDI high byte, TMS high byte; every TDI byte is zero:

```
host -> 0x05 0x00 0x2B    TAP shift, 43 bits
host <- 0x05 0x00 0x2B    3-byte echo
host -> 00 5F 00 00       bits 1-16:  TDI low, TMS low, TDI high, TMS high
host -> 00 00 00 00       bits 17-32
host -> 00 00 00 03       bits 33-43
host <- (6 TDO bytes)     two per group, low byte first
```

The TMS bytes, in transmission order, are `5F 00 00 00 00 03` (each byte
least-significant bit first).

| Bits | TMS | Purpose |
|---|---|---|
| 1–5 | 1 | Test-Logic-Reset |
| 6 | 0 | Run-Test/Idle |
| 7 | 1 | Select-DR-Scan |
| 8 | 0 | Capture-DR |
| 9 | 0 | Shift-DR |
| 10–41 | 0 ×31, then 1 | 32-bit IDCODE shift; the last bit exits to Exit1-DR |
| 42 | 1 | Update-DR |
| 43 | 0 | Run-Test/Idle |

The 32 TDO bits sampled during bits 10–41 are the IDCODE,
least-significant bit first.  TDO before the shift is not part of the
IDCODE.  The `0x05` reply returns TDO in 16-bit groups, low byte first, so
reassemble the stream before extracting the IDCODE.  For a multi-device
chain, shift 32 bits per device when every device has a 32-bit IDCODE, and
expect the nearest-TDO device first.

## Related

- [README.md](README.md) — overview and index.
- [entry.md](entry.md) — binary entry, settings and exit.
- [wiring.md](wiring.md) — pins, power, levels and pull-ups.
- [clock.md](clock.md) — TCK and link rates.
- [openocd.md](openocd.md) — the `0x06` command set.
- [xsvf.md](xsvf.md) — the `0x18` player.
- [edge-cases.md](edge-cases.md) — failure modes and recovery.
