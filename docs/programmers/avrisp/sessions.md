# AVRISP-MKII — worked sessions

Part of the AVRISP-MKII documentation set.  These are short, self-contained
examples of the exchanges a host and the personality produce.  The byte
catalogue is in [protocol.md](protocol.md); the state model is in
[command-model.md](command-model.md).

## How to read the examples

- `host ->` is one request: one USB transfer on the USB route, one AVR068
  frame on the serial route.
- `host <-` is the reply.  On the USB route it is the raw V2 payload; on the
  serial route the examples show the complete frame.
- All values are hexadecimal.  The USB route carries the payload with no
  start, size, token or checksum header.
- Target-dependent bytes are marked as such.

## Identity and parameters over USB

Enter with `0x08` (the device re-enumerates) and start the host tool at
once — the mode ends after about five seconds without a request.  Then
send:

```
host -> 01
host <- 01 00 0A 41 56 52 49 53 50 5F 4D 4B 32
```

`01` is `SIGN_ON`; `00` is `CMD_OK`; `0A` is the ten-byte identifier
`AVRISP_MK2`.  The whole reply fits in one short packet.

Read the reported firmware version:

```
host -> 03 91
host <- 03 00 01

host -> 03 92
host <- 03 00 18
```

`03` is `GET_PARAMETER`; `91` is the software-major id, `92` the
software-minor id.  The pair reads as version 1.24.

Set and read the ISP clock.  The parameter is a speed-ladder **index**, not a
frequency and not the `-B` period: index 6 is 125 kHz.

```
host -> 02 98 06
host <- 02 00

host -> 03 98
host <- 03 00 06
```

Read the reset polarity and the connection-status parameter:

```
host -> 03 9E
host <- 03 00 01

host -> 03 A1
host <- 03 00 00
```

`9E` is `0x01` (active-low reset).  `A1` is `0x00` (ISP ready).  VTARGET is
read with `03 94`; the value is a fresh ADC reading in 0.1 V units.  With
about 3.3 V on the `ADC` pin the reply is `03 00 21`; with nothing wired to
`ADC` the value is undefined, because the input floats.

## Rejected requests over USB

The personality answers a malformed or unsupported request without touching
the target:

```
host -> 07
host <- 07 C9

host -> 10 00
host <- 10 C0

host -> 02 94 32
host <- 02 C0

host -> 03 99
host <- 03 C0
```

| Request | Meaning | Reply |
|---|---|---|
| `07` | `FIRMWARE_UPGRADE`, not implemented | `07 C9` unknown command |
| `10 00` | `ENTER_PROGMODE_ISP` with the wrong length | `10 C0` failed before any pin is touched |
| `02 94 32` | write a read-only parameter (VTARGET) | `02 C0` |
| `03 99` | read an unknown parameter | `03 C0` |

A valid command with no target is still accepted if it does not need one:

```
host -> 06 00 00 00 00
host <- 06 00

host -> 11 01 01
host <- 11 00
```

`06` is `LOAD_ADDRESS` (address 0); `11` is `LEAVE_PROGMODE_ISP` with a 1 ms
pre-delay and 1 ms post-delay.  `LEAVE` succeeds even when no programming
session was started.

## Identity over the serial route

The serial route keeps the same CDC port and wraps the payload in AVR068
frames.  `SIGN_ON` on sequence `0x01`:

```
host -> 1B 01 00 01 0E 01 14
host <- 1B 01 00 0B 0E 01 00 08 53 54 4B 35 30 30 5F 32 02
```

The reply payload is `01 00 08` followed by `STK500_2`
(`53 54 4B 35 30 30 5F 32`).  Read the software-major parameter on sequence
`0x02`:

```
host -> 1B 02 00 02 0E 03 91 87
host <- 1B 02 00 03 0E 03 00 01 16
```

Set the SCK index to 6 on sequence `0x03`:

```
host -> 1B 03 00 03 0E 02 98 06 89
host <- 1B 03 00 02 0E 02 00 16
```

A frame with a bad checksum (sequence `0x04`, correct checksum would be
`0x11`) is answered with the AVR068 error pair:

```
host -> 1B 04 00 01 0E 01 10
host <- 1B 04 00 02 0E B0 C1 62
```

`B0` is the answer marker and `C1` the checksum-error status.  The sequence
is echoed so the host can match the reply to the bad frame.

## An ISP session at command level

The exact enable bytes, poll value and delays come from the host's part
definition.  The example below uses the standard ATmega328P enable sequence
`AC 53 00 00` with poll value `53` at index 3.  The delay fields are host
choices; the values in the request are a valid set, and a shorter set works
if the target is ready.

| Step | Request bytes (command included) | Reply | Notes |
|---|---|---|---|
| identify | `01` (no data) | `01 00 0A` + `AVRISP_MK2` | optional; confirms the route |
| read reset polarity | `03 9E` | `03 00 01` | active-low default |
| set SCK index | `02 98 06` | `02 00` | index 6 = 125 kHz |
| enter ISP | `10 C8 64 20 03 02 53 03 AC 53 00 00` | `10 00` on success, `10 C0` on failure | timeout 200, stabilisation 100 ms, execution 32 ms, 3 sync loops, 2 ms byte delay; `SCK` is latched here |
| load address | `06 00 00 00 00` | `06 00` | address 0 |
| read flash | `14 00 04 20` | `14 00` + four target bytes + `00` | read command `0x20`; data is target-dependent |
| leave | `11 01 01` | `11 00` | releases `RESET` and disables the interface |

A failed enable leaves `RESET` asserted; send `LEAVE_PROGMODE_ISP` before
rewiring or retrying.  The personality does not compare written data;
`avrdude` reads back and compares unless `-V` is given.  The procedure for
changing to another target interface is in
[command-model.md](command-model.md) §Changing target interface.

For PDI or TPI the sequence is:

1. `51 00` or `51 02` (`XPROG_SETMODE`) and read the reply.
2. `50 01` (`XPROG ENTER_PROGMODE`).
3. `50 03` / `50 04` / `50 05` / `50 06` / `50 07` operations.
4. `50 02` (`XPROG LEAVE_PROGMODE`).

The exact addresses, lengths and memory types are in [protocol.md](protocol.md)
§XPROG subcommands.

## End of the mode

After about five seconds with no host traffic the personality ends and the
binary layer reprints its identifier.  On the serial route the same CDC port
shows it; on the USB route the CDC port returns first.

| Actor | Bytes | Meaning |
|---|---|---|
| personality | `42 42 49 4F 31` | `BBIO1` on the returned CDC port after the idle deadline |
| host | `0F` | binary-layer return to the terminal |
| personality | `01` | accepted |
| personality | — | the banner and `HiZ>` follow |

`0x0F` is the binary-layer return to the terminal; it is not an AVRISP
command.  Sending `0x0F` while the personality is still active does not end
it — the byte is protocol input: an unknown V2 command on the USB route and
ignored outside a frame on the serial route.

## Related docs

- [protocol.md](protocol.md) — request and reply shapes, status codes.
- [command-model.md](command-model.md) — the states behind these exchanges.
- [entry.md](entry.md) — how to send the entry bytes.
- [serial-route.md](serial-route.md) — the AVR068 frame in detail.
- [isp.md](isp.md) / [pdi.md](pdi.md) / [tpi.md](tpi.md) — the host flows.
