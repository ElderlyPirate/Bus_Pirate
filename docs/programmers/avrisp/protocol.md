# AVRISP-MKII — transports and protocol reference

Part of the AVRISP-MKII documentation set (`docs/programmers/avrisp/`).  This
page owns both transports and the STK500v2 / XPROG command, status and
parameter catalogue.  The entry/exit lifecycle is in [entry.md](entry.md).

## Entry bytes (from binary bitbang mode)

| Byte | Transport | Host command | Reachable targets |
|---|---|---|---|
| `0x08` | USB mkII personality | `avrdude -c avrispmkII -P usb` | ISP, PDI, TPI |
| `0x09` | framed STK500v2 over CDC | `avrdude -c stk500v2 -P COMx` | ISP only |

Both bytes are accepted only on v4; on a v3 build both answer the failure
byte `0x00`.  How a host sends them, and how the mode ends, is
[entry.md](entry.md).

## Identity naming

The firmware presents two identity strings; this is the mapping used
everywhere in this set:

| Transport | SIGN_ON identifier | USB product string |
|---|---|---|
| USB (`0x08`) | `AVRISP_MK2` | `AVRISP mkII` (VID/PID `03EB:2104`) |
| serial (`0x09`) | `STK500_2` | the normal Bus Pirate serial device |

The USB identity must report as a mkII so the host enables the XPROG
(PDI/TPI) path; the serial identity must report as `STK500_2` so the host
keeps the framed serial path.

## USB transport

The USB route is a vendor-class device: one interface, two bulk endpoints,
64-byte packets.

| Property | Value |
|---|---|
| vendor / product | `03EB:2104` (Atmel AVRISP mkII) |
| device class | vendor specific (`0xFF`) |
| interface | 1, vendor specific, 2 endpoints |
| OUT endpoint | bulk, address `0x02`, 64-byte packets |
| IN endpoint | bulk, address `0x82`, 64-byte packets |
| request framing | the **raw V2 payload**, no AVR068 start/size/token/checksum header |
| request end | a packet shorter than 64 bytes, or a zero-length packet |
| reply end | a short packet, or a zero-length packet when the length is an exact multiple of 64 |
| maximum payload | 256 memory data bytes |

- A request may span several packets.  The personality buffers the whole
  transfer before dispatching it, so a command split across packets is
  handled as one request.
- An empty OUT packet does not start a request and does not renew the idle
  deadline.
- A reply whose length is an exact multiple of 64 bytes is terminated with
  a zero-length packet; without it the host would wait for the next packet.

### Worked exchange (USB)

`SIGN_ON` (`0x01`) is one byte in and thirteen bytes out:

```
host -> 01
host <- 01 00 0A 41 56 52 49 53 50 5F 4D 4B 32
```

| Byte(s) | Meaning |
|---|---|
| `01` (from host) | `SIGN_ON` |
| `01` | command echoed |
| `00` | `CMD_OK` |
| `0A` | identifier length = 10 |
| `41 56 52 49 53 50 5F 4D 4B 32` | `AVRISP_MK2` |

Thirteen bytes fit in one 64-byte packet, so the reply is a single short
packet.  On the serial route the same command is wrapped in an AVR068
frame — worked example in [serial-route.md](serial-route.md) §Worked
frame.

## STK500v2 commands handled

The numbering follows Atmel's AVR068 (STK500 protocol) and AVR069 (AVRISP
mkII protocol) specifications.  The tables on this page are complete for
this implementation, so those documents are provenance for the numbering,
not a required lookup.

| Code | Command | Notes |
|---|---|---|
| `0x01` | SIGN_ON | replies with the identity string above |
| `0x02` | SET_PARAMETER | writes a writable parameter |
| `0x03` | GET_PARAMETER | reads a readable parameter |
| `0x05` | OSCCAL | AVR053 calibration — see [rescue-clock-osccal.md](rescue-clock-osccal.md) |
| `0x06` | LOAD_ADDRESS | 4-byte address, big-endian |
| `0x0A` | RESET_PROTECTION | dummy acknowledgement |
| `0x10` | ENTER_PROGMODE_ISP | ISP enable sequence |
| `0x11` | LEAVE_PROGMODE_ISP | ISP exit sequence |
| `0x12` | CHIP_ERASE_ISP | chip erase |
| `0x13` | PROGRAM_FLASH_ISP | flash write |
| `0x14` | READ_FLASH_ISP | flash read |
| `0x15` | PROGRAM_EEPROM_ISP | EEPROM write |
| `0x16` | READ_EEPROM_ISP | EEPROM read |
| `0x17` | PROGRAM_FUSE_ISP | fuse write |
| `0x18` | READ_FUSE_ISP | fuse read |
| `0x19` | PROGRAM_LOCK_ISP | lock write |
| `0x1A` | READ_LOCK_ISP | lock read |
| `0x1B` | READ_SIGNATURE_ISP | signature read |
| `0x1C` | READ_OSCCAL_ISP | calibration-byte read |
| `0x1D` | SPI_MULTI | raw ISP byte exchange |
| `0x50` | XPROG | XPROG subcommand wrapper (PDI/TPI) |
| `0x51` | XPROG_SETMODE | select PDI or TPI |

`0x07` (FIRMWARE_UPGRADE) is **not** handled: unknown commands are
answered with `0xC9`, not acted on.

## Status codes

| Code | Status | Emitted | Meaning |
|---|---|---|---|
| `0x00` | CMD_OK | yes | success |
| `0x80` | CMD_TOUT | yes | command timed out |
| `0x81` | RDY_BSY_TOUT | yes | target stayed busy to timeout |
| `0x82` | SET_PARAM_MISSING | **no** | defined, not emitted by this implementation |
| `0xC0` | CMD_FAILED | yes | command failed, including an invalid request |
| `0xC9` | CMD_UNKNOWN | yes | command not recognised |
| `0xCA` | CMD_ILLEGAL_PARAM | **no** | defined, not emitted by this implementation |

An invalid parameter value is answered with `0xC0`; a request of the wrong
length or with an out-of-range payload is also answered with `0xC0` before
any target pin is touched.  The exception is a malformed `XPROG` request:
it is answered in the XPROG shape, with result `1` (FAILED).

For the ISP connection-status parameter the value `0x00` means
*ISP ready*; there are no host-visible connection-fault bits, and this is
deliberate.

## Parameters

| Id | Parameter | Access | Default |
|---|---|---|---|
| `0x80` | build number low | read | 0 |
| `0x81` | build number high | read | 0 |
| `0x90` | hardware version | read | 0 |
| `0x91` | software major | read | 1 |
| `0x92` | software minor | read | `0x18` |
| `0x94` | VTARGET | read | ADC-derived, 0.1 V units |
| `0x98` | SCK duration | read/write | 6 (125 kHz) |
| `0x9E` | reset polarity | read/write | `0x01` (active-low) |
| `0xA1` | target connection status | read | `0x00` (ready) |
| `0xA4` | discharge delay | read/write | 0 |

`0xA4` is accepted and stored; **no discharge delay is applied** — the
value has no effect on the pin sequence.  Parameters are set by the host
each session and are **not persisted** to EEPROM.  VTARGET is
informational and does not interlock programming.

## XPROG subcommands (PDI / TPI)

The `XPROG` command (`0x50`) wraps these subcommands; the transport must
first be selected with `XPROG_SETMODE` (`0x51`).

| Code | Subcommand |
|---|---|
| `0x01` | ENTER_PROGMODE |
| `0x02` | LEAVE_PROGMODE |
| `0x03` | ERASE |
| `0x04` | WRITE_MEM |
| `0x05` | READ_MEM |
| `0x06` | CRC |
| `0x07` | SET_PARAM |

`XPROG_SETMODE` takes `0x00` = PDI or `0x02` = TPI; any other value is
rejected.  Memory types are `1` application, `2` boot, `3` EEPROM,
`4` fuse, `5` lockbits, `6` user signature, `7` factory calibration.
Erase types are `1` chip, `2` app, `3` boot, `4` EEPROM, `5` app page,
`6` boot page, `7` EEPROM page, `8` user signature, `9` config.
XPROG results use `0` OK, `1` FAILED, `2` collision, `3` timeout.

### SET_PARAM ids

`XPROG SET_PARAM` (`0x07`) carries the parameter id followed by its
value:

| Id | Parameter | Value |
|---|---|---|
| `0x01` | NVMBASE | 4 bytes, big-endian (default `0x010001C0`) |
| `0x02` | EEPROM page size | 2 bytes; accepted 1–256, else `FAILED` (default 32) |
| `0x03` | NVMCMD register address | 1 byte (default `0x33`) |
| `0x04` | NVMCSR register address | 1 byte (default `0x32`) |
| `0x05` | unknown, host compatibility | 2 bytes, accepted and discarded |

An unknown id is answered `FAILED`.  A host sets these before an XMEGA or
TPI write; the defaults are inherited from LUFA and are what `avrdude`
expects to override from the part definition.

## Request handling

- Every complete request is validated for its exact expected length (and
  for payload bounds) **before** any target pin is touched.  A request
  that does not match is answered with a failure/unknown status; the
  personality does **not** stall the endpoint or leave it unanswered.
- Requests and replies carry at most **256 memory data bytes**.
- A TPI write is the one place where a length is rewritten rather than
  rejected: an odd byte count is padded with `0xFF` (see
  [tpi.md](tpi.md) §Word rules).

## Related docs

- [entry.md](entry.md) — entry bytes in practice, the idle deadline, the
  return to `BBIO1`.
- [isp.md](isp.md) / [pdi.md](pdi.md) / [tpi.md](tpi.md) — the target
  paths and their host commands.
- [serial-route.md](serial-route.md) — the AVR068 frame and a worked
  serial exchange.
- [edge-cases.md](edge-cases.md) — failure modes.
- [rescue-clock-osccal.md](rescue-clock-osccal.md) — the `0x05` command.
