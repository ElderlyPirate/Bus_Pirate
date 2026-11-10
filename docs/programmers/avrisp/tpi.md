# AVRISP-MKII — TPI path (tinyAVR)

Part of the AVRISP-MKII documentation set (`docs/avrisp/`).  This page
covers the **TPI** target interface (tinyAVR: ATtiny4/5/9/10); wiring is in
[wiring.md](wiring.md), the command set in [protocol.md](protocol.md).

## Scope

- Covers: entering the programmer, the `avrdude` TPI flow, TPI wiring and
  the 5 V supply requirement, and TPI-specific limits.
- Does not cover: ISP ([isp.md](isp.md)), PDI ([pdi.md](pdi.md)), or the
  CDC serial route ([serial-route.md](serial-route.md)).
- **TPI is reachable only over the USB route** (`-c avrispmkII -P usb`).
  The serial route is ISP-only.
- **TPI is not verified.**  The protocol engine is complete, but there is
  no confirmed physical TPI target result here.

## Entry

1. Enter binary bitbang mode (BBIO1) and send byte **`0x08`**
   ([isp.md](isp.md) §Entry) — same entry as ISP.
2. The device re-enumerates as `AVRISP mkII`.
3. Program the tinyAVR:

```
avrdude -c avrispmkII -P usb -p <part> -U flash:w:<file>.hex:i
```

For an ATtiny10 use `-p t10`.

`avrdude` selects TPI with the `XPROG_SETMODE` command before
`XPROG ENTER_PROGMODE`; the Bus Pirate implements that sequence directly.

## TPI wiring notes

- Clock → TPI_CLK; `/RESET` → AUX0; data → the joined MOSI+MISO node →
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

## Operational notes

- **Word-oriented NVM.**  TPI writes are word-based: an odd byte count is
  padded with `0xFF`, and the target address must be even.
- **Erase** is triggered by writing the high byte of the target address,
  per the TPI algorithm.
- **No memory CRC.**  TPI does not support the XPROG CRC command; a CRC
  request returns a failure status rather than data.
- **NVMEN cleanup.**  Leaving the mode clears the NVMEN bit before
  releasing `/RESET`.  If the TPI link is damaged, the bounded cleanup may
  not be able to clear NVMEN; power-cycling the target may then be
  required (see [edge-cases.md](edge-cases.md)).

## Limits

- **Not verified.**  No physical TPI target result is confirmed here.
- **5 V NVM supply required** for ATtiny4/5/9/10 programming.
- **HT/RSTDISBL and high-voltage-reset parts are unsupported** — they need
  a 12 V reset the Bus Pirate cannot generate.
- **256 data bytes per request**, as for the other interfaces.