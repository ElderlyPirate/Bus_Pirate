# AVRISP-MKII — TPI path (tinyAVR)

Part of the AVRISP-MKII documentation set.  This page covers the **TPI**
target interface (tinyAVR: ATtiny4/5/9/10); wiring is in
[wiring.md](wiring.md), the command set in [protocol.md](protocol.md), and
the entry/exit lifecycle in [entry.md](entry.md).

## Scope

- Covers: the `avrdude` TPI flow, TPI wiring and the 5 V supply requirement,
  the word rules, and TPI-specific limits.
- Does not cover: ISP ([isp.md](isp.md)), PDI ([pdi.md](pdi.md)), or the
  CDC serial route ([serial-route.md](serial-route.md)).
- With `avrdude`, **TPI is reachable only over the USB route**
  (`-c avrispmkII -P usb`): `avrdude` enables XPROG only for mkII-class
  hosts.  The firmware engine behind the serial route also accepts XPROG, so
  a custom host can drive TPI over the serial framing; that combination is
  **not verified**.
- **TPI is not verified.**  The protocol engine is complete; a physical TPI
  target has not been verified.

## Entry

Send the USB entry byte `0x08` from `BBIO1` ([entry.md](entry.md)); the
device re-enumerates as `AVRISP mkII`.  Then program the tinyAVR:

```
avrdude -c avrispmkII -P usb -p t10 -U flash:w:app.hex:i
```

`-p t10` is an ATtiny10; use the host tool's name for the target.

`avrdude` selects TPI with the `XPROG_SETMODE` command before
`XPROG ENTER_PROGMODE`; the Bus Pirate implements that sequence directly.

Erase (`-e`), read-back (`-U flash:r:readback.hex:i`) and the automatic
compare of a `:w:` write all work through the same `-U` mechanism as on the
ISP path; fuse and lock names come from the host's part definition, not from
this personality.  See [isp.md](isp.md) §Fuses, lock bits, erase and verify.

## TPI wiring notes

- Clock → TPI_CLK; `RESET` → `AUX`; data → the joined MOSI+MISO node →
  TPIDATA.  The Bus Pirate drives `RESET` low and **keeps it low for the
  entire TPI session**, as the tinyAVR TPI physical layer requires.
- Data is half duplex, joined through resistors exactly as for PDI — see
  [wiring.md](wiring.md).
- **No pull-up is fitted or enabled by the Bus Pirate.**
- The Bus Pirate drives 3.3 V push-pull.  The ATtiny4/5/9/10 datasheet gives
  VIH = 0.6 × VCC for the TPI pins at a 5 V supply, so 3.0 V is the minimum
  input high and 3.3 V is within the specified range with a small margin.
  A level shifter is optional; use one for a long or noisy connection.  The
  target's `RESET` pin is held low for the whole session, so its higher
  0.9 × VCC input-high threshold does not apply while TPI is active.
- The full ISP → TPI switch sequence is in
  [command-model.md](command-model.md) §Changing target interface.

## The 5 V supply requirement

NVM programming of the ATtiny4/5/9/10 requires a **5 V** target supply (the
target datasheet allows NVM programming at 5 V only).  The Bus Pirate drives
3.3 V logic; a 5 V target supply is not a 5 V Bus Pirate output.  Power the
target at 5 V from its own supply.  The target's 5 V logic signals are
within the header's 5 V input class, but a 5 V **supply rail** must never be
connected to a signal pin.  Never enable the Bus Pirate's own power outputs
into an already-powered target, and never allow a 12 V reset to reach a Bus
Pirate pin.

The tinyAVR TPI enable sequence also requires `RESET` low after power is
applied; the firmware asserts it when the XPROG interface is enabled and
releases it when the session ends.

## Word rules

TPI memory is word-oriented.  The personality accepts an odd byte count and
pads the last byte with `0xFF`, so a one-byte or odd-length write is a
normal request, not an error.

A write is rejected only when:

| Request | Result |
|---|---|
| a byte count of zero | rejected (FAILED, result `1`) |
| a byte count above 256 | rejected |
| an odd start address | rejected — the address must be even |
| an address or rounded length past the 16-bit space | rejected |
| a memory type outside 1–6 | rejected |

A rejected write touches no target memory.  A read has the same 16-bit
address limit but is byte-oriented: the address need not be even and an odd
length is normal.

A TPI fuse write is not subject to the PDI one-byte rule; a four-byte fuse
block is accepted.  The address and length are still checked before the
target is touched.

## Operational notes

- **Erase** is triggered by writing the high byte of the target address,
  per the TPI algorithm.  TPI accepts erase types 1–9; type `1` is a chip
  erase, and types 2–9 use the section-erase command.  Any other erase type
  is rejected before the target is touched.
- **No memory CRC.**  TPI does not support the XPROG CRC command; a CRC
  request returns a failure status rather than data.
- **NVMEN cleanup.**  Leaving the mode clears the NVMEN bit before
  releasing `RESET`.  If the TPI link is damaged, the bounded cleanup may
  not be able to clear NVMEN; power-cycling the target may then be
  required (see [edge-cases.md](edge-cases.md)).
- **A failed XPROG enable may leave the interface enabled** even though the
  reply is FAILED.  Send `XPROG LEAVE_PROGMODE`, or let the mode end, before
  rewiring; see [command-model.md](command-model.md) §Changing target
  interface.
- **`RSTDISBL` targets are unsupported.**  A part with the reset-disable
  fuse set needs 12 V on `RESET` for TPI; the Bus Pirate cannot generate it.

## Limits

- **5 V NVM supply required** for ATtiny4/5/9/10 programming; this is a
  target requirement, not a Bus Pirate output.
- **RSTDISBL and high-voltage-reset parts are unsupported** — they need
  a 12 V reset the Bus Pirate cannot generate.
- **256 data bytes per request**, as for the other interfaces.
- **16-bit address space.**  A read or write past `0xFFFF` is rejected.
- **No host speed setting.**  TPI data transfers run at about 125 kHz.
- **A target with a floating or unpowered NVM rail** answers nothing and
  the request ends as a timeout, not as a retry.
- **Not verified** on a physical TPI target.

## Related docs

- [entry.md](entry.md) — entry, deadline and the return to `BBIO1`.
- [wiring.md](wiring.md) — the TPI table and the DATA join.
- [protocol.md](protocol.md) — XPROG subcommands and the TPI limits.
- [command-model.md](command-model.md) — the XPROG session order.
- [isp.md](isp.md) / [pdi.md](pdi.md) — the other target interfaces.
- [edge-cases.md](edge-cases.md) — failure modes.
