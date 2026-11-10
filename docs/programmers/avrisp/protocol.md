# AVRISP-MKII — entry bytes and protocol reference

Part of the AVRISP-MKII documentation set (`docs/avrisp/`).  This page is
the reference for the entry bytes and the STK500v2 / XPROG command and
status numbering the host uses.

## Entry bytes (from binary bitbang mode)

| Byte | Transport | Host command | Reachable targets |
|---|---|---|---|
| `0x08` | USB mkII personality | `avrdude -c avrispmkII -P usb` | ISP, PDI, TPI |
| `0x09` | framed STK500v2 over CDC | `avrdude -c stk500v2 -P COMx` | ISP only |

Both bytes are accepted only on v4; on a v3 build both report a failure
byte (`0x00`).  See `docs/generic/` for reaching binary bitbang mode.

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

## STK500v2 commands handled

Command bytes are the AVR068 / LUFA numbering.

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
answered, not acted on.

## Status codes

| Code | Status | Meaning |
|---|---|---|
| `0x00` | CMD_OK | success |
| `0x80` | CMD_TOUT | command timed out |
| `0x81` | RDY_BSY_TOUT | target stayed busy to timeout |
| `0x82` | SET_PARAM_MISSING | a required parameter was not set |
| `0xC0` | CMD_FAILED | command failed |
| `0xC9` | CMD_UNKNOWN | command not recognised |
| `0xCA` | CMD_ILLEGAL_PARAM | parameter not permitted |

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

Parameters are set by the host each session and are **not persisted** to
EEPROM.  VTARGET is informational and does not interlock programming.

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

## Request handling

- Every complete request is validated for its exact expected length (and
  for payload bounds) **before** any target pin is touched.  A request
  that does not match is answered with a failure/unknown status; the
  personality does **not** stall the endpoint or leave it unanswered.
- The USB transport buffers a complete request through its terminating
  short packet or ZLP.  An exact-multiple transfer is terminated with a
  ZLP on the reply side.
- Requests and replies carry at most **256 memory data bytes**.