# AVRISP-MKII — command and state model

Part of the AVRISP-MKII documentation set.  This page explains the states a
session passes through and what the personality remembers between commands.
The byte-level catalogue is in [protocol.md](protocol.md); the worked
exchanges are in [sessions.md](sessions.md).

## Scope

- Covers: the personality lifecycle, the state a host sets up before
  programming, the command order for ISP and XPROG, and what happens on
  failure.
- Does not cover: `avrdude` options ([isp.md](isp.md), [pdi.md](pdi.md),
  [tpi.md](tpi.md)), wiring ([wiring.md](wiring.md)), or the transport
  framing ([protocol.md](protocol.md), [serial-route.md](serial-route.md)).

## Three layers

| Layer | Job | State it owns |
|---|---|---|
| transport | moves a complete request and reply | USB enumeration, CDC port, packet/frame assembly |
| V2 engine | parses the command, validates the length, executes it | parameters, the ISP address pointer, the selected XPROG protocol |
| target interface | clocks the target | ISP mode, PDI mode, TPI mode, `RESET`, `SCK`, the data line |

The host drives all three.  The personality has no part database, no
erase/write/compare algorithm and no automatic sequence; it executes the
opcodes and poll values the host supplies.

## The personality lifecycle

| State | Host-visible sign | Entered by | Left by |
|---|---|---|---|
| `BBIO1` | `BBIO1` on the CDC port | 20 × `0x00` from the terminal | entry byte, or `0x0F` to the terminal |
| attaching (USB route) | the CDC port disappears; the mkII device appears | `0x08` | host configures the interface, or the idle deadline restores CDC |
| ready | the host can send any V2 command | enumeration/configuration on USB; immediately on the serial route | idle deadline, host reset, deconfiguration, board reset |
| ISP session | target held in programming mode | `ENTER_PROGMODE_ISP` succeeds | `LEAVE_PROGMODE_ISP`, or the mode ends |
| XPROG session (PDI or TPI) | target interface enabled | `XPROG ENTER_PROGMODE` succeeds | `XPROG LEAVE_PROGMODE`, or the mode ends |
| ending | target released; `BBIO1` reprints | idle deadline, USB stall/reset, board reset | `0x0F` to the terminal |

The states are cooperative, not strictly enforced.  The firmware does not
refuse a memory command sent before `ENTER_PROGMODE`, and it does not police
a switch from one target interface to the other without a `LEAVE`.  In those
cases the command is executed against a target that is not listening, or
the previous interface's pin state can persist.  Follow the sequences below.

## What a session remembers

| Item | Lifetime | Notes |
|---|---|---|
| V2 parameters | one mode entry | reset to defaults at entry; not persisted to EEPROM |
| SCK duration | latched at `ENTER_PROGMODE_ISP` | changing it later has no effect until the next enable |
| reset polarity | read at each assert/release | a change affects the next reset action |
| ISP address pointer | from `LOAD_ADDRESS` to the next `LOAD_ADDRESS` or ISP enable | cleared by `ENTER_PROGMODE_ISP` |
| extended-address reload | until consumed | set by an address with bit 31, or at a 64K boundary |
| XPROG selected protocol | until the next `XPROG_SETMODE` or mode end | defaults to PDI |
| XPROG NVMBASE, page size, register addresses | until the next `XPROG SET_PARAM` or mode end | defaults at entry |
| VTARGET | refreshed before every command | read from the `ADC` pin; informational |
| target interface | until `LEAVE` or mode end | ISP and XPROG are mutually exclusive in practice |

The address pointer is a **word** address for flash: a flash byte pair writes
low then high at the same address, and the pointer advances after the high
byte.  EEPROM advances every byte.  An XPROG operation carries its own
address in the request, so it does not use the V2 pointer.

## ISP sequence

1. Optional but normal: `SIGN_ON` to identify the tool.
2. Read or set the parameters the host needs: `SCK` duration, reset
   polarity.
3. `ENTER_PROGMODE_ISP` with the part's enable bytes and poll values.  This
   latches the SCK speed, asserts `RESET`, runs the enable handshake and, on
   success, leaves the target in programming mode.
4. `LOAD_ADDRESS` and then one or more of `PROGRAM_FLASH_ISP`,
   `READ_FLASH_ISP`, `PROGRAM_EEPROM_ISP`, `READ_EEPROM_ISP`,
   `CHIP_ERASE_ISP`, `PROGRAM_FUSE_ISP`, `READ_FUSE_ISP`,
   `PROGRAM_LOCK_ISP`, `READ_LOCK_ISP`, `READ_SIGNATURE_ISP`,
   `READ_OSCCAL_ISP`, `SPI_MULTI`.
5. `LEAVE_PROGMODE_ISP` when the session ends.  It applies the host's pre-
   and post-delays, releases `RESET` and disables the interface.
6. Repeat from step 3 for another part, or let the mode end.

A failed `ENTER_PROGMODE_ISP` leaves `RESET` asserted: the firmware toggles
the line during the retry loop and the last action is an assert.  Send
`LEAVE_PROGMODE_ISP` (or let the mode end) to release the target.

## XPROG sequence (PDI / TPI)

1. `XPROG_SETMODE` with `0x00` (PDI) or `0x02` (TPI).  The default is PDI.
2. Optional: `XPROG SET_PARAM` for NVMBASE, EEPROM page size, NVMCMD and
   NVMCSR.  The defaults suit the parts `avrdude` drives; a host with a
   different part overrides them.
3. `XPROG ENTER_PROGMODE`.  PDI enables the PDI clock/data interface and
   sends the NVM key; TPI asserts `RESET`, enables the TPI physical layer and
   sends the NVM key.
4. `XPROG ERASE`, `WRITE_MEM`, `READ_MEM`, `CRC`, `SET_PARAM`.
5. `XPROG LEAVE_PROGMODE` to disable the interface and release the target.

A failed XPROG enable can leave the interface enabled even though the reply
is FAILED.  Send `XPROG LEAVE_PROGMODE`, or let the mode end, before
rewiring.

PDI keeps a clock on the clock line between commands so the target does not
time out; the firmware does this automatically.  TPI holds `RESET` low for
the whole session.

## Changing target interface

The target interface is not switched safely in the middle of a session.
End the current session, let the mode end, rewire with the personality
inactive, then re-enter and select the new interface.  `CS` is a 4 MHz
output for the whole active session; treat it as live until the mode ends.
A failed ISP enable leaves `RESET` asserted; a failed XPROG enable can leave
the interface enabled.  Send the matching `LEAVE` in either case, then let
the mode end before rewiring.

| From → to | End the current session | Wait and re-enter | Rewire (mode inactive) | Select the new interface |
|---|---|---|---|---|
| ISP → PDI | `11 01 01` (`LEAVE_PROGMODE_ISP`) | wait for `BBIO1`; send `0x08` | `CLK`→`RESET/PDI_CLK`; data = `MOSI`+`MISO` through 220 Ω → `PDI_DATA`; `AUX` and `CS` open | `51 00`, then `50 01` |
| ISP → TPI | `11 01 01` | wait for `BBIO1`; send `0x08` | `CLK`→`TPI_CLK`; data = the same join → `TPIDATA`; `AUX`→`RESET`; 5 V target supply | `51 02`, then `50 01` |
| PDI → TPI | `50 02` (`XPROG LEAVE_PROGMODE`) | wait for `BBIO1`; send `0x08` | keep the data join → `TPIDATA`; move `CLK` from `RESET/PDI_CLK` to `TPI_CLK`; add `AUX`→`RESET`; 5 V target supply | `51 02`, then `50 01` |
| TPI → PDI | `50 02` | wait for `BBIO1`; send `0x08` | keep the data join → `PDI_DATA`; remove `AUX` from `RESET`; move `CLK` from `TPI_CLK` to `RESET/PDI_CLK`; `CS` open | `51 00`, then `50 01` |
| PDI or TPI → ISP | `50 02` | wait for `BBIO1`; send `0x08` | separate `MOSI` and `MISO` (remove the join); `CLK`→`SCK`; `AUX`→`RESET`; `CS` open | `10 ...` with the part's enable bytes |

The table shows the USB route (`0x08`) and raw payloads.  A custom host on
the serial route sends `0x09` and wraps the same payloads in AVR068 frames.
The wiring behind the rewire column is in [wiring.md](wiring.md) §ISP, §PDI
and §TPI.

## Failure behaviour

| Event | State after the reply | What the host should do |
|---|---|---|
| malformed request | unchanged; no target pin touched | correct the request and retry |
| unknown command | unchanged | use a supported command |
| target timeout in a memory operation | interface stays enabled | `LEAVE`, check wiring/power, retry |
| failed ISP enable | `RESET` asserted | `LEAVE`, check target and clock, retry |
| failed XPROG enable | interface may stay enabled | `LEAVE`, check target, retry |
| idle deadline | mode ends; target released; `BBIO1` | re-enter if more work is needed |
| host USB reset/deconfiguration | mode ends; CDC restored | re-enter with `0x08` |

A timeout does not end the mode by itself.  The host can send `LEAVE` and
start again, or continue if the target recovered.  Only the mode-level
events (idle, USB stall/reset, board reset) end the personality.

## Related docs

- [protocol.md](protocol.md) — command, status and parameter catalogue.
- [sessions.md](sessions.md) — worked sequences for identity, parameters and
  ISP.
- [entry.md](entry.md) — entry, deadline and exit.
- [isp.md](isp.md) / [pdi.md](pdi.md) / [tpi.md](tpi.md) — the target paths.
- [edge-cases.md](edge-cases.md) — concrete failure and recovery entries.
