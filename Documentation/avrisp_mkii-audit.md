# AVRISP-MKII Source Audit And Validation

The original source audit was performed on 2026-10-28 against ffd7aa9.
The fixes are committed in 9492bbb and subsequent transport/NVM changes.
Hardware verification followed on 2026-10-30. This note describes the
current **BPv4-only** implementation, not an uncommitted working diff.
BPv3 excludes the personality; its programmer entry points are unavailable.

## Public References

- LUFA revision `90d65ba059d91078a34b9c26c8772ee14b556a13`, project
  `Projects/AVRISP-MKII`: V2Protocol, ISPProtocol/ISPTarget,
  XPROGProtocol, XMEGANVM and TINYNVM implementations and descriptors.
- avrdude revision `9aa44d08cb207a80a5d1005ead61142d21fb50de` and avrdude
  8.3: `src/stk500v2.c`, `src/usb_libusb.c`, `src/usbdevs.h`.
- PIC24FJ256GB110 family datasheet **DS39897C**, covering the compiled
  PIC24FJ256GB106: USB, GPIO/PPS, enhanced output compare, SPI, ADC,
  timers and interrupt nesting.
- PIC24 programmer's reference **70000157G**: RCOUNT, REPEAT, NOP, MUL.UU
  and interrupt behavior. XC16 disassembly was used for ISR/delay checks.
- XMEGA AU manual **8331F**, sections 32.3, 32.7 and 33.12; ATxmega128A3U
  datasheet **8386**, reset/PDI characteristics.
- ATmega64A datasheet **8160E**, serial programming pin mapping/algorithm,
  clock requirements, lock protection and fuse descriptions.
- ATtiny4/5/9/10 datasheet **8127H**, sections 15, 16 and 17: TPI,
  programming supply requirements and reset limitations.
- AVR053 application note **2555H**, sections 2 and 3: calibration protocol.

Board wiring/divider facts are defined in `Firmware/hardwarev4.h` and
`Firmware/base.c`. Native tests use explicit mocks; they do not establish
electrical compliance or substitute for a target datasheet.

## USB And Transport Corrections

| Machinery | Reason for the implementation |
|---|---|
| IN descriptor ownership | Wait for SIE release BEFORE copying the next packet; otherwise the previous payload can be overwritten during transmission. Wait for the final short packet/ZLP too. |
| OUT transfer collection | Buffer a complete command through its actual short packet/ZLP. Consume an exact-64-byte request's terminating ZLP and drain oversize transfers without confusing the next command. |
| Complete-request validation | Check command-specific headers, payload length, poll indices, memory bounds and spans BEFORE touching target pins. Missing bytes must not become synthesized programming instructions. |
| DMA/ISR visibility | Descriptor pointers/counts and the active personality flag use volatile accesses where hardware/interrupt state can change. |
| Descriptors | Vendor-class bulk endpoint 0x02 OUT / 0x82 IN, 64-byte packets, VID/PID 03EB:2104 and bus-powered configuration match the host transport and board. |
| Personality replacement | Mask USB IRQs across detach and descriptor/endpoint replacement. Do not wait for a CDC reader before detaching. Restore normal CDC initialization after usb_init clears callbacks. |
| CDC configuration | Register CDC flush handling after EVERY configuration/reset; clear stale flush/lock state. Resume BBIO only after CDC is configured. |
| Endpoint GET_STATUS | Read BSTALL only; querying halt state must not clear it or confuse other descriptor flags with a stall. |
| Startup/recovery bounds | The personality uses a watchdog-bounded attach wrapper, including host-held SE0. Failed CDC configuration retries detach/attach; target cleanup precedes recovery. Initial application startup outside this personality is unchanged. |
| CDC serial framing | Nonblocking poll_getc_cdc handles an empty packet/ZLP without entering an unbounded blocking read. Output waits consult the watchdog and configuration state. |
| Reply budget and idle policy | Refresh the reply budget even after target timeout; idle time is measured after completion, so a long active command is not mistaken for host inactivity. Empty USB packets/idle handles do not keep the session alive. |

The CDC serial fallback uses AVR068: start 0x1B, sequence echo, big-endian
length, token 0x0E and XOR checksum. Checksum errors have the B0/C1 reply;
malformed, oversize and partial frames resynchronize with bounded waits.
Its `STK500_2` identity keeps stock avrdude on the framed path. Stock
avrdude uses ISP on this route; PDI/TPI initialization is enabled for
mkII-class hosts on the USB route. There is no hardware-UART/baud-switch
personality: both supported entry routes run on BPv4.

## Target PHY And Timing

- GPIO writes use LAT, with latch values preloaded before changing TRIS.
  This avoids PORT read/modify/write corruption and reset/data glitches.
  Assert reset according to the host polarity; release it to high impedance.
- Seven hardware SPI masks implement 8 MHz through 125 kHz at FCY=16 MHz;
  CKP=0/CKE=1/SMP=0 is mode 0. All 164 AVR069 ladder indices (0..163)
  are represented, including 157 approximate software rates.
- Timer 1 remains the delay timebase. Timer 2/OC5 supplies the 4 MHz rescue
  clock using enhanced OC self-synchronization, OC5RS=3 and OC5R=2.
  The corrected millisecond helper widens BEFORE multiplying by 1000;
  a requested 100 ms detach must not wrap at 16-bit arithmetic limits.
- Timer 3 supplies 10 ms watchdog ticks. Priority-6 Timer 4 supplies a
  25 kHz idle PDI/TPI clock. Temporarily enable interrupt nesting so the
  clock can preempt USB servicing, and restore nesting on exit.
- Idle/frame clock ownership changes only at target-byte boundaries.
  Response construction keeps the idle clock alive; USB buffer copying
  must not exceed PDI's approximately 100 us inactivity limit without it.
- The idle ISR uses sixteen NOPs, not an inline REPEAT that could clobber
  interrupted RCOUNT. Disassembly checks include GPIO writes, flag clear,
  RETFIE and register preservation.
- PDI/TPI uses software synchronous 12-bit 8E2 framing, LSB first. Preserve
  setup/sample edges, stop bits, guard clocks, high-before-drive and
  float-before-latch-clear ordering. Receive parity/both stops are checked.
  The asynchronous PIC24 UART is not a substitute for this waveform.
- Cleanup clears PDI reset/TPI NVMEN while the watchdog/clock still run,
  then releases pins, shuts down ISP, and stops the mode timer.
- AVR053 OSCCAL supplies a hardware 32786.9 Hz MOSI clock (divider 488),
  MISO pull-up and eight-toggle bounded response test. It requires target
  calibration firmware; it does not install that firmware or write a fuse.
  Restore the rescue clock after calibration.

## Protocol And NVM Corrections

| Area | Audited behavior |
|---|---|
| ISP entry | Exact host field order, all four enable bytes, one-based poll index, reset polarity/delays and retry sequence. Enter reply is exactly command + status; connection status is queried separately by the host. |
| ISP addressing | Flash low/high opcode toggle, word increments, EEPROM byte increments, extended-address emission and boundary reload. A pending extended reload survives a subsequent LOAD_ADDRESS without bit 31, matching LUFA. |
| ISP programming | Preserve all six completion modes. Use a separate valid-poll-address flag so address zero is legal; fallback-to-delay applies to only that byte/page, not all later writes. Stop at timeout, preserve the failure and do not commit a failed page. |
| SPI_MULTI | Allow all 255 TX bytes, pad the RX window and transmit any remaining TX tail independently of RX count. |
| XPROG request validation | Accept 256-byte chunks and seven-byte erase requests. Validate memory/mode values, PDI single-byte fuse/lock writes and TPI alignment/span/padded-word limits before access. Unknown subcommands return framed failure. |
| PDI addressing | Add NVM register offsets to the configured base rather than ORing them; this works beyond the normally aligned base. |
| PDI reads/writes | Zero-length read performs no target read. Page-buffer erase/load/commit and fuse/page writes wait for controller completion before success. Resume the idle clock during long reply copies. |
| EEPROM tagging/erase | Use a 16-bit counter for all 256 possible page offsets and keep timeout checks. Tag EEPROM addresses BEFORE erase because the controller clears page-access state afterward. |
| TPI NVM | SKEY, control/status accesses, split pointer registers, word padding/order, odd-address erase trigger and final ready polling follow the primary algorithm. |
| Shared NVM machinery | PDI/TPI share the identical eight-byte enable key and masked status polling. Protocol opcodes, pointer setup and completion conditions stay explicit at their call sites. |
| XPROG CRC | Preserve LUFA's unusual high-byte, low-byte, middle-byte reply order; do not silently normalize it for a hypothetical consumer. |
| Parameters | Defaults/privileges follow the host protocol; values reset on entry rather than being persisted to EEPROM. VTARGET is ADC-derived rounded decivolts, informational only. It does not interlock programming or set logic voltage. |

The AU manual contradicts itself for application/boot section erase:
Table 33-5 specifies a PDI-write trigger for 0x20/0x68, while section
33.12.3.8 says CMDEX. The implementation keeps LUFA/Table 33-5's sequence.
Application/boot section erase and restoration were verified on the
ATxmega128A3U; this is not a claim about every XMEGA silicon revision.

## Verification And Remaining Limits

- ATxmega128A3U PDI: signature, complete memory extraction, erase/write/
  verify/restore, EEPROM, fuse/lock reads, section/page operations and CRC.
- ATmega64A ISP: signature, fuse/lock/calibration reads, erase/unlock,
  complete 64 KiB test-image programming/automatic verification/independent
  full readback, 2 KiB EEPROM pattern rewrite/readback and an actual safe
  idempotent extended-fuse programming command. Rescue input was required.
- BPv4 USB entry/enumeration/idle return and CDC serial framing were
  exercised, including driver binding, host port restart/reset and repeated
  automatic recovery. A serial port's published name alone is not proof
  that it can already be opened; host enumeration races require retries.
- TPI remains source/build/native-tested only; no physical TPI target was
  verified. ATtiny4/5/9/10 needs a 5 V NVM supply and appropriate level
  translation. Never drive a BP GPIO with 5 V or 12 V; high-voltage reset
  targets are unsupported.
- The one-second target budget limits full-page work at the slowest ISP
  rates (51.1 Hz takes about 0.626 s for one four-byte instruction).
  Use small blocks/custom-host timeouts there. Integer delays plus overhead
  mean software rates are approximate; not every full page at every rate
  is promised.
- A damaged TPI link can prevent NVMEN clearing; bounded cleanup cannot
  recover an electrically inaccessible device, and target power cycling
  may still be necessary. A disabled host USB port cannot be re-enabled
  by firmware.

From `Firmware`, run `python tests/test_avrisp.py`,
`python tests/test_avrisp_cdc_gate.py` and
`python tests/test_avrisp_board_gate.py`. They cover actual production
sources with mocks, real XC16 CDC preprocessing, and real board gating.
The native serial tests include 266-byte CDC-framed payloads, not an
unsupported BPv3 UART simulation. Native tests are not electrical/timing
proof. Full ROM builds use the ordinary physical linker scripts; tests
and generated fixtures do not ship in the firmware image.
