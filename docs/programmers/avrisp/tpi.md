# AVRISP-MKII — TPI path (tinyAVR)

Part of the AVRISP-MKII documentation set (`docs/programmers/avrisp/`).  This
page covers the **TPI** target interface (tinyAVR: ATtiny4/5/9/10); wiring is
in [wiring.md](wiring.md), the command set in [protocol.md](protocol.md), and
the entry/exit lifecycle in [entry.md](entry.md).

## Scope

- Covers: the `avrdude` TPI flow, TPI wiring and the 5 V supply requirement,
  the word rules, and TPI-specific limits.
- Does not cover: ISP ([isp.md](isp.md)), PDI ([pdi.md](pdi.md)), or the
  CDC serial route ([serial-route.md](serial-route.md)).
- **TPI is reachable only over the USB route** (`-c avrispmkII -P usb`).
  The serial route is ISP-only.
- **TPI is not verified.**  The protocol engine is complete, but there is
  no confirmed physical TPI target result for this set.

## Entry

Send the USB entry byte `0x08` from `BBIO1` ([entry.md](entry.md)); the
device re-enumerates as `AVRISP mkII`.  Then program the tinyAVR:

```
avrdude -c avrispmkII -P usb -p t10 -U flash:w:app.hex:i
```

`-p t10` is an ATtiny10; use the host tool's name for the target.

`avrdude` selects TPI with the `XPROG_SETMODE` command before
`XPROG ENTER_PROGMODE`; the Bus Pirate implements that sequence directly.

Erase (`-e`), read-back (`-U flash:r:file.hex:i`) and the automatic verify of
a `:w:` write all work through the same `-U` mechanism as on the ISP path;
fuse and lock names come from the host's part definition, not from this
personality.  See [isp.md](isp.md) §Fuses, lock bits, erase and verify.

## TPI wiring notes

- Clock → TPI_CLK; `/RESET` → AUX; data → the joined MOSI+MISO node →
  TPIDATA.  The Bus Pirate drives `/RESET` low to put the target into TPI
  mode.
- Data is half duplex, joined through resistors exactly as for PDI — see
  [wiring.md](wiring.md).
- **No pull-up is required** by the Bus Pirate.

## The 5 V supply requirement

NVM programming of the ATtiny4/5/9/10 requires a **5 V** target supply
(the NVM controller does not enable programming below that).  The Bus
Pirate drives 3.3 V logic; a 5 V target supply is not a 5 V Bus Pirate
output.  Power the target at 5 V from its own supply and use suitable
level translation where the logic levels demand it — **never feed 5 V or
12 V into a Bus Pirate pin**, and never enable the Bus Pirate's own power
outputs into an already-powered target.

## Word rules

TPI memory is word-oriented.  The personality accepts an odd byte count and
pads the last byte with `0xFF`, so a one-byte or odd-length write is a
normal request, not an error.

A write is rejected only when:

| Request | Result |
|---|---|
| a byte count of zero | rejected (`XPROG_ERR_FAILED`) |
| an odd start address | rejected — the address must be even |
| an address or length past the 16-bit space | rejected |

A rejected write touches no target memory.  A read has the same 16-bit
address limit but is byte-oriented and does not require an even address.

## Operational notes

- **Erase** is triggered by writing the high byte of the target address,
  per the TPI algorithm.
- **No memory CRC.**  TPI does not support the XPROG CRC command; a CRC
  request returns a failure status rather than data.
- **NVMEN cleanup.**  Leaving the mode clears the NVMEN bit before
  releasing `/RESET`.  If the TPI link is damaged, the bounded cleanup may
  not be able to clear NVMEN; power-cycling the target may then be
  required (see [edge-cases.md](edge-cases.md)).

## Limits

- **5 V NVM supply required** for ATtiny4/5/9/10 programming.
- **HT/RSTDISBL and high-voltage-reset parts are unsupported** — they need
  a 12 V reset the Bus Pirate cannot generate.
- **256 data bytes per request**, as for the other interfaces.
- **A target with a floating or unpowered NVM rail** answers nothing and
  the request ends as a timeout, not as a retry.

## Related docs

- [entry.md](entry.md) — entry, deadline and the return to `BBIO1`.
- [wiring.md](wiring.md) — the TPI table and the DATA join.
- [protocol.md](protocol.md) — XPROG subcommands and the TPI limits.
- [isp.md](isp.md) / [pdi.md](pdi.md) — the other target interfaces.
- [edge-cases.md](edge-cases.md) — failure modes.
