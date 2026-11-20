# AVRISP-MKII — transports and protocol reference

Part of the AVRISP-MKII documentation set.  This page owns both transports and
the STK500v2 / XPROG command, status and parameter catalogue.  The entry/exit
lifecycle is in [entry.md](entry.md); the session model is in
[command-model.md](command-model.md).

## Entry bytes (from binary bitbang mode)

| Byte | Transport | Host command | Reachable targets |
|---|---|---|---|
| `0x08` | USB mkII personality | `avrdude -c avrispmkII -P usb` | ISP, PDI, TPI |
| `0x09` | framed STK500v2 over CDC | `avrdude -c stk500v2 -P COM5` | ISP with `avrdude`; the engine also accepts XPROG |

Both bytes are accepted only on v4; on a v3 build both answer the failure
byte `0x00`, then `BBIO1`.  How a host sends them, and how the mode ends, is
[entry.md](entry.md).

## Identity naming

The feature, the USB product and the protocol identifier are three forms of
the same name.  This is the mapping used everywhere in this set:

| Form | Value | Where it appears |
|---|---|---|
| feature name | `AVRISP-MKII` | this documentation, the firmware feature |
| USB product string | `AVRISP mkII` | host device list; VID/PID `03EB:2104` |
| USB manufacturer string | `ATMEL` | host device list |
| USB `SIGN_ON` identifier | `AVRISP_MK2` | reply to command `0x01` on the USB route |
| serial `SIGN_ON` identifier | `STK500_2` | reply to command `0x01` on the serial route |
| serial device | the normal Bus Pirate CDC port | no separate USB identity |

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
| USB version | 1.10 |
| interface | 1, vendor specific, 2 endpoints |
| OUT endpoint | bulk, address `0x02`, 64-byte packets |
| IN endpoint | bulk, address `0x82`, 64-byte packets |
| power | bus powered, 100 mA requested |
| request framing | the **raw V2 payload**, no AVR068 start/size/token/checksum header |
| request end | a packet shorter than 64 bytes, or a zero-length packet |
| reply end | a short packet, or a zero-length packet when the length is an exact multiple of 64 |
| maximum payload | 256 memory data bytes |

- A request may span several packets.  The personality buffers the whole
  transfer before dispatching it, so a command split across packets is
  handled as one request.
- A request that ends exactly on a 64-byte boundary **must** be followed by
  a zero-length packet.  Without it the personality keeps waiting for the
  rest of the transfer; when the command budget expires the mode ends.
- An empty OUT packet does not start a request and does not extend the
  120-second startup budget or renew the five-second idle deadline.
- A reply whose length is an exact multiple of 64 bytes is terminated with
  a zero-length packet; without it the host would wait for the next packet.
- One transfer carries **one** command.  Concatenating two commands in one
  transfer makes it the wrong length and the whole transfer is answered
  with the failure status.
- An oversized request is drained to its terminating short packet, then
  answered with a failure status; the link stays synchronised.

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
frame — worked example in [serial-route.md](serial-route.md) §Worked frame.

## Requests and replies

A request starts with the command byte.  The tables below give the bytes
**after** that byte and the reply that follows.  A request whose length does
not match the command is answered with failure (`0xC0`) before any target pin
is touched; an `XPROG` request keeps the XPROG shape (`0x50`, subcommand,
result `1`).

### STK500v2 commands handled

| Code | Command | Request bytes after the command | Reply |
|---|---|---|---|
| `0x01` | SIGN_ON | — | `01 00` + length + identifier |
| `0x02` | SET_PARAMETER | id, value | command + status |
| `0x03` | GET_PARAMETER | id | command + status + value |
| `0x05` | OSCCAL | — | command + status |
| `0x06` | LOAD_ADDRESS | four address bytes, big-endian | command + status |
| `0x0A` | RESET_PROTECTION | — | command + status (dummy acknowledgement) |
| `0x10` | ENTER_PROGMODE_ISP | timeout, stabilisation delay, execution delay, sync loops, byte delay, poll value, poll index, four enable bytes | command + status |
| `0x11` | LEAVE_PROGMODE_ISP | pre-delay, post-delay | command + status |
| `0x12` | CHIP_ERASE_ISP | erase delay, poll method, four erase bytes | command + status |
| `0x13` | PROGRAM_FLASH_ISP | count high, count low, mode, delay, three commands, two poll values, data | command + status |
| `0x14` | READ_FLASH_ISP | count high, count low, read command | command + OK + data + final status |
| `0x15` | PROGRAM_EEPROM_ISP | as PROGRAM_FLASH_ISP | command + status |
| `0x16` | READ_EEPROM_ISP | as READ_FLASH_ISP | command + OK + data + final status |
| `0x17` | PROGRAM_FUSE_ISP | four programming bytes | command + status + status |
| `0x18` | READ_FUSE_ISP | return index (1–4), four read bytes | command + OK + byte + OK |
| `0x19` | PROGRAM_LOCK_ISP | four programming bytes | command + status + status |
| `0x1A` | READ_LOCK_ISP | return index (1–4), four read bytes | command + OK + byte + OK |
| `0x1B` | READ_SIGNATURE_ISP | return index (1–4), four read bytes | command + OK + byte + OK |
| `0x1C` | READ_OSCCAL_ISP | return index (1–4), four read bytes | command + OK + byte + OK |
| `0x1D` | SPI_MULTI | transmit count, receive count, receive start, transmit data | command + OK + receive data + OK |
| `0x50` | XPROG | subcommand and its payload | `0x50`, subcommand, result [, data] |
| `0x51` | XPROG_SETMODE | protocol (`0x00` PDI, `0x02` TPI) | command + status |

- The `PROGRAM_FLASH_ISP` and `PROGRAM_EEPROM_ISP` commands carry a 16-bit
  data count; the limit is 256 bytes.  The `READ_*` commands carry a 16-bit
  count and read at most 256.
- The fuse/lock write replies carry two OK bytes; the fuse/lock read replies
  carry a leading OK and a trailing OK around the data byte.  These are
  LUFA's reply shapes, kept for host compatibility.
- `ENTER_PROGMODE_ISP` accepts a poll index of 0–4.  A value above 4 is
  rejected as a malformed request.  The timeout field is accepted but the
  firmware's own one-second command budget remains the bound.
- The ISP enable reply is exactly command + status, on success and on
  failure.  A host that reports `Unknown status 0x00` afterwards is reading
  its own secondary diagnostic; it should query the target-connection
  parameter instead.
- `0x07` (FIRMWARE_UPGRADE) is **not** handled: it is answered with `0xC9`,
  not acted on.  A host that insists on upgrading the tool fails here.
- `0x05` (`OSCCAL`) is a one-byte request.  It replies `05 00` on success,
  `05 80` on the 32 ms timeout, and `05 C0` for a request with extra bytes.
  It does not return or write an OSCCAL value.  Current `avrdude` builds
  send it with `-O` / `--osccal`; a custom host can send the byte directly.
  See [rescue-clock-osccal.md](rescue-clock-osccal.md) §Worked calibration
  exchange.

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

For a V2 `SET_PARAMETER`, an unknown or read-only parameter id is answered
with `0xC0`; a request of the wrong length is also answered with `0xC0`
before any target pin is touched.  A writable parameter accepts any byte
value; the ISP engine clamps `SCK_DURATION` to 163.  The exception is a
malformed `XPROG` request: it is answered in the XPROG shape, with result
`1` (FAILED).

The ISP connection-status parameter (`0xA1`) reads `0x00` when ISP is ready.
There are no host-visible connection-fault bits.

## Parameters

| Id | Parameter | Access | Default | Notes |
|---|---|---|---|---|
| `0x80` | build number low | read | 0 | — |
| `0x81` | build number high | read | 0 | — |
| `0x90` | hardware version | read | 0 | — |
| `0x91` | software major | read | 1 | — |
| `0x92` | software minor | read | `0x18` | reported version 1.24 |
| `0x94` | VTARGET | read | ADC-derived, 0.1 V units | refreshed before each command |
| `0x98` | SCK duration | read/write | 6 (125 kHz) | values above 163 are stored and read back, but the ISP engine clamps to 163 |
| `0x9E` | reset polarity | read/write | `0x01` (active-low) | `0` selects active-high; any nonzero value selects active-low |
| `0xA1` | target connection status | read | `0x00` (ISP ready) | no fault bits |
| `0xA4` | discharge delay | read/write | 0 | accepted and stored; **no discharge delay is applied** |

- Parameters are set by the host each session and are **not persisted** to
  EEPROM; they return to the defaults when the personality is entered again.
- VTARGET is informational and does not interlock programming.  With nothing
  wired to `ADC` it is a floating reading.
- Writing a read-only parameter and reading or writing an unknown id both
  answer `0xC0`.

## XPROG subcommands (PDI / TPI)

The `XPROG` command (`0x50`) wraps these subcommands; a host normally
selects the target interface first with `XPROG_SETMODE` (`0x51`).  The
default selected protocol is PDI, so a host that omits the selection gets
PDI.

| Code | Subcommand | Request after the subcommand | Reply after the result |
|---|---|---|---|
| `0x01` | ENTER_PROGMODE | — | — |
| `0x02` | LEAVE_PROGMODE | — | — |
| `0x03` | ERASE | memory type, four address bytes | — |
| `0x04` | WRITE_MEM | memory type, page mode, four address bytes, two length bytes, data | — |
| `0x05` | READ_MEM | memory type, four address bytes, two length bytes | data when OK |
| `0x06` | CRC | CRC type | three CRC bytes when OK |
| `0x07` | SET_PARAM | parameter id and its value | — |

`XPROG_SETMODE` takes `0x00` = PDI or `0x02` = TPI; `0x01` (JTAG) and any
other value are rejected.  The selected protocol defaults to PDI and
persists for the session.

| Item | Values |
|---|---|
| Memory types | `1` application, `2` boot, `3` EEPROM, `4` fuse, `5` lockbits, `6` user signature, `7` factory calibration |
| Write vs read | `WRITE_MEM` accepts types 1–6; `READ_MEM` accepts types 1–7 |
| Erase types | `1` chip, `2` app, `3` boot, `4` EEPROM, `5` app page, `6` boot page, `7` EEPROM page, `8` user signature, `9` config |
| Results | `0` OK, `1` FAILED, `2` collision (defined, not emitted by this implementation), `3` timeout |
| Physical link | bit-banged half-duplex 12-bit frame (start, 8 data bits LSB first, even parity, two stop bits) at about 125 kHz; no host speed setting |

- On PDI a fuse or lockbits write must be exactly one byte.  On TPI a
  four-byte fuse write is accepted.
- PDI supports erase types 1–8 and rejects type 9.  TPI accepts types 1–9;
  type 1 is chip erase, and types 2–9 use the section-erase command.  Any
  other erase type is rejected before the target is touched.
- A parity or stop-bit error aborts the command rather than returning
  corrupt data.

### SET_PARAM ids

| Id | Parameter | Value |
|---|---|---|
| `0x01` | NVMBASE | 4 bytes, big-endian (default `0x010001C0`) |
| `0x02` | EEPROM page size | 2 bytes; accepted 1–256, else FAILED (default 32) |
| `0x03` | NVMCMD register address | 1 byte (default `0x33`) |
| `0x04` | NVMCSR register address | 1 byte (default `0x32`) |
| `0x05` | unknown, host compatibility | 2 bytes, accepted and discarded |

An unknown id is answered FAILED.  A host sets these before an XMEGA or TPI
write; the defaults are inherited from LUFA and are what `avrdude` expects to
override from the part definition.

## XPROG properties

| Topic | Behaviour |
|---|---|
| CRC reply order | High byte, low byte, middle byte; not a normalised order. |
| TPI memory CRC | Not supported; a CRC request returns FAILED, not data. |
| Unknown subcommand | Answered `0x50`, the subcommand, result `1` (FAILED); it does not fall through to the V2 unknown-command path. |
| ERASE request length | 7 bytes: command, subcommand, memory type, four address bytes. |
| Address width | PDI addresses are 32-bit; TPI addresses are 16-bit.  A TPI read or write past `0xFFFF` is rejected.  A TPI write also requires an even start address and a nonzero length; an odd byte count is padded with `0xFF`. |
| Page mode | PDI only: bit 0 erases the page buffer before loading, bit 1 commits the loaded page after writing.  TPI ignores the page-mode bits. |
| Page boundaries | A write is not a page-boundary check.  The host supplies the address and the page-mode bits; the firmware does not know the target's page size. |

## Request handling

- Every complete request is validated for its exact expected length (and
  for payload bounds) **before** any target pin is touched.  A request that
  does not match is answered with a failure/unknown status; the personality
  does **not** stall the endpoint or leave it unanswered.
- Requests and replies carry at most **256 memory data bytes**.
- A TPI write is the one place where a length is rewritten rather than
  rejected: an odd byte count is padded with `0xFF` (see
  [tpi.md](tpi.md) §Word rules).
- The V2 engine executes the host-supplied AVR programming opcodes and poll
  values.  It has no device database and does not compare written data by
  itself.

## Related docs

- [entry.md](entry.md) — entry bytes in practice, the startup and idle
  deadlines, the return to `BBIO1`.
- [command-model.md](command-model.md) — the session/state model.
- [sessions.md](sessions.md) — worked request and reply sequences.
- [isp.md](isp.md) / [pdi.md](pdi.md) / [tpi.md](tpi.md) — the target
  paths and their host commands.
- [serial-route.md](serial-route.md) — the AVR068 frame and a worked
  serial exchange.
- [edge-cases.md](edge-cases.md) — failure modes.
- [rescue-clock-osccal.md](rescue-clock-osccal.md) — the `0x05` command.
