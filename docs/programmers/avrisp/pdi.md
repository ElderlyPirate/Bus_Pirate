# AVRISP-MKII — PDI path (XMEGA)

Part of the AVRISP-MKII documentation set.  This page covers the **PDI**
target interface (XMEGA devices); wiring is in [wiring.md](wiring.md), the
command set in [protocol.md](protocol.md), and the entry/exit lifecycle in
[entry.md](entry.md).

## Scope

- Covers: the `avrdude` PDI flow, PDI wiring notes, fuses/lock bits, and
  PDI-specific limits.
- Does not cover: ISP ([isp.md](isp.md)), TPI ([tpi.md](tpi.md)), or the
  CDC serial route ([serial-route.md](serial-route.md)).
- With `avrdude`, **PDI is reachable only over the USB route**
  (`-c avrispmkII -P usb`): `avrdude` enables the XPROG protocol only for
  mkII-class hosts.  The firmware engine behind the serial route also
  accepts XPROG, so a custom host can drive PDI over the serial framing;
  that combination is **not verified**.
- **PDI is not verified for every XMEGA part.**

## How the route is selected

`avrdude` drives a real mkII by first sending `XPROG_SETMODE` to choose the
target interface (PDI or TPI), then `XPROG ENTER_PROGMODE`, and then issues
`XPROG` memory/erase/CRC commands.  The Bus Pirate implements that sequence
directly; there is nothing to configure in the firmware.  See
[protocol.md](protocol.md) §XPROG subcommands for the numbering.

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

Host drivers and permissions are in [host-tools.md](host-tools.md).

## PDI wiring notes

- The PDI clock is the XMEGA's **RESET/PDI_CLK** pin.  Do not wire `AUX`
  and do not press the target's reset button while programming.  The full
  ISP → PDI switch sequence is in [command-model.md](command-model.md)
  §Changing target interface.
- Data is a single half-duplex node formed by joining MOSI and MISO
  through resistors — see [wiring.md](wiring.md).
- The Bus Pirate drives 3.3 V push-pull.  An XMEGA running at 3.3 V needs no
  level shifting; a target running below 3.3 V does.  The programmer does
  not power the target.

## Fuses, lock bits and erase

Fuses and lock bits are ordinary `-U` operations with the XMEGA memory names
(`fuse0` … `fuse5`, `lock`); they go through the same XPROG path as flash.
Read before writing, because the values are part-specific:

```
avrdude -c avrispmkII -P usb -p x128a3u -U fuse0:r:-:h -U lock:r:-:h
avrdude -c avrispmkII -P usb -p x128a3u -e
```

`-e` is a chip erase.  A `:w:` flash write is read back and compared unless
`-V` is given.  Protected or locked devices behave per the target's own
protection settings; the personality adds no shortcut and no bypass.

## Operational notes

| Topic | Behaviour | Limit or caveat |
|---|---|---|
| PDI clock | The XMEGA PDI interface times out if PDI_CLK is idle.  The firmware keeps an idle keep-alive clock of about 25 kHz on the line between commands and during long reply transfers; data transfers run at about 125 kHz. | No host setting. |
| Memory errors | Returned as XPROG status codes (OK / FAILED / TIMEOUT), not as a dropped connection. | — |
| Failed enable | The interface may stay enabled even though the reply is FAILED. | Send `XPROG LEAVE_PROGMODE`, or let the mode end, before rewiring. |
| EEPROM page erase | The selected page is tagged before erasing, so only the intended page is cleared and its neighbours are preserved. | — |
| APP/BOOT section erase | The erase command is loaded, then triggered by a PDI write to the address in the request; page erases use the same address field.  The host supplies the address; the firmware does not add a mode switch or reorder the sections. | The exact result is target-silicon dependent and unverified on every part. |
| CRC reply order | High byte, low byte, middle byte. | Not a normalised order. |

## Limits

- **256 data bytes per request.**  An XMEGA flash page is larger than 256
  bytes; `avrdude` splits it into 256-byte `WRITE_MEM` chunks.
- **PDI addresses are 32-bit.**  The request carries the four address bytes;
  there is no 16-bit truncation.
- **A fuse or lockbits write must be exactly one byte.**  A longer write is
  rejected with FAILED before the target is touched.
- **No memory CRC on TPI** (a PDI CRC is supported; see [tpi.md](tpi.md)).
- **No host speed setting.**  Data transfers run at about 125 kHz; the idle
  keep-alive clock is about 25 kHz.  The host cannot change either rate.
- **Not verified for every XMEGA part**, and the exact section/page erase
  behaviour is target-silicon dependent.

## Related docs

- [entry.md](entry.md) — entry, deadline and the return to `BBIO1`.
- [wiring.md](wiring.md) — the PDI table and the DATA join.
- [protocol.md](protocol.md) — XPROG subcommands and parameter ids.
- [command-model.md](command-model.md) — the XPROG session order.
- [isp.md](isp.md) / [tpi.md](tpi.md) — the other target interfaces.
- [edge-cases.md](edge-cases.md) — failure modes.
