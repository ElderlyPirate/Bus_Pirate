# AVRISP-MKII — PDI path (XMEGA)

Part of the AVRISP-MKII documentation set (`docs/programmers/avrisp/`).  This
page covers the **PDI** target interface (XMEGA devices); wiring is in
[wiring.md](wiring.md), the command set in [protocol.md](protocol.md), and the
entry/exit lifecycle in [entry.md](entry.md).

## Scope

- Covers: the `avrdude` PDI flow, PDI wiring notes, fuses/lock bits, and
  PDI-specific limits.
- Does not cover: ISP ([isp.md](isp.md)), TPI ([tpi.md](tpi.md)), or the
  CDC serial route ([serial-route.md](serial-route.md)).
- **PDI is reachable only over the USB route** (`-c avrispmkII -P usb`).
  `avrdude` enables the XPROG protocol only for mkII-class hosts, so the
  [serial route](serial-route.md) is ISP-only.

## How the route is selected

`avrdude` drives a real mkII by first sending the `XPROG_SETMODE` command
to choose the target interface (PDI or TPI), then `XPROG ENTER_PROGMODE`,
and then issues `XPROG` memory/erase/CRC commands.  The Bus Pirate
implements that sequence directly; there is nothing to configure in the
firmware.  See [protocol.md](protocol.md) for the command numbering.

## Entry

Send the USB entry byte `0x08` from `BBIO1` ([entry.md](entry.md)); the
device re-enumerates as `AVRISP mkII`.  Then program the XMEGA:

```
avrdude -c avrispmkII -P usb -p x128a3u -U flash:w:app.hex:i
```

`-p x128a3u` is an XMEGA128A3U; use the host tool's name for the target.
A read-only signature check:

```
avrdude -c avrispmkII -P usb -p x128a3u -n -U signature:r:-:h
```

## PDI wiring notes

- The PDI clock is the XMEGA's **RESET/PDI_CLK** pin.  Do not wire `AUX`
  and do not press the target's reset button while programming.
- Data is a single half-duplex node formed by joining MOSI and MISO
  through resistors — see [wiring.md](wiring.md).
- All XMEGA logic is 3.3 V; the programmer does not power the target.

## Fuses, lock bits and erase

Fuses and lock bits are ordinary `-U` operations with the XMEGA memory
names (`fuse0` … `fuse5`, `lock`); they go through the same XPROG path as
flash.  Read before writing, because the values are part-specific:

```
avrdude -c avrispmkII -P usb -p x128a3u -U fuse0:r:-:h -U lock:r:-:h
avrdude -c avrispmkII -P usb -p x128a3u -e
```

`-e` is a chip erase.  A `:w:` flash write is verified after writing unless
`-V` is given.  Protected or locked devices behave per the target's own
protection settings; the personality adds no shortcut and no bypass.

## Operational notes

- **PDI needs a running clock.**  The XMEGA PDI interface times out if
  PDI_CLK is idle for too long, so the firmware keeps a clock running on
  the clock line between commands and during long reply transfers.  The
  operator does not need to do anything for this.
- **Memory errors surface as XPROG status codes** (OK / FAILED / TIMEOUT),
  not as a dropped connection.
- If a PDI link is electrically damaged mid-session, the firmware's
  bounded cleanup may not be able to reset the target interface;
  power-cycling the target may then be necessary (see
  [edge-cases.md](edge-cases.md)).

## Limits

- **256 data bytes per request.**  An XMEGA flash page is larger than 256
  bytes; `avrdude` splits it into 256-byte `WRITE_MEM` chunks.
- **Section and page erases follow the LUFA/Table behaviour.**  The XMEGA
  AU manual contradicts itself for application/boot section erase; this
  personality keeps the sequence LUFA uses and does not add a mode switch.
  This is not a claim about every XMEGA silicon revision.
- **EEPROM page erase** tags the selected page before erasing, so only the
  intended page is cleared and its neighbours are preserved.
- **CRC reply byte order** is high byte, low byte, middle byte — LUFA's
  order preserved deliberately, not a normalised ordering.
- **Not verified for every XMEGA part.**

## Related docs

- [entry.md](entry.md) — entry, deadline and the return to `BBIO1`.
- [wiring.md](wiring.md) — the PDI table and the DATA join.
- [protocol.md](protocol.md) — XPROG subcommands and parameter ids.
- [isp.md](isp.md) / [tpi.md](tpi.md) — the other target interfaces.
- [edge-cases.md](edge-cases.md) — failure modes.
