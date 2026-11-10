# USB (`core_usb/`)

**Status:** maintainer-facing.  Covers the BPv4 USB CDC device stack: the
transport that carries the terminal, the binary layer and the SUMP stream.  v3
does not compile this group; it uses its FTDI serial link and the v3 UART code
in [`base.c`](system.md).

The stack derives from JTR's PIC USB/CDC implementation (CC-BY 3.0 headers in
the files).  It is a full-speed, bus-powered device with no hardware ping-pong
buffers; the CDC bulk path does its own **software A/B buffering** (two 64-byte
buffers per direction, one BD).

## Scope and files

| File | Responsibility |
|---|---|
| `prj_usb_config.h` | board/stack constants: clock, CDC flush timeout, VID/PID, endpoint counts, buffer sizes, interrupt build |
| `picusb.h` | PIC24/PIC18 register layer, BDT layout/macros, interrupt and endpoint helpers |
| `usb_stack.c/h` | device state, EP0 control transfers, descriptor streaming, standard requests, handler registration |
| `cdc.c/h` | CDC class requests, line coding, the double-buffered bulk data path, locks, ZLP and timeout flush |
| `descriptors.h` | device, configuration and string descriptors |
| `usb_stack_globals.h` | `BYTE` typedef and the include order for the stack |

Configuration (`prj_usb_config.h`):

| Item | Value |
|---|---|
| stack clock constant (`CLOCK_FREQ`) | 32 MHz — the USB stack's own constant, not the CPU speed |
| USB build | full speed, internal transceiver, internal pull-ups, internal VREG, bus powered |
| buffers | no ping-pong (`USB_PP_BUF_MODE 0`) |
| control endpoint | EP0, 8-byte packets |
| endpoints | `MAX_EPNUM_USED = 2`; EP1 interrupt IN, EP2 bulk IN/OUT |
| bulk/notice buffers | 64-byte bulk, 10-byte notice |
| flush timeout (`CDC_FLUSH_MS`) | 4 |
| interrupts | `USB_INTERRUPTS` enabled |

`CLOCK_FREQ` is the USB stack's own 32 MHz constant.  The CPU instruction clock
is FCY = 16 MHz (`base.h`); bitbang and timing budgets use FCY, not
`CLOCK_FREQ`.

The PIC24 buffer descriptor table is `usb_bdt[2 + 2 * MAX_EPNUM_USED]` (six
4-byte entries), aligned to 512 bytes.  EP0 uses entries 0/1, EP1 uses 2/3
(only the IN entry 3 is configured) and EP2 uses 4/5 (OUT/IN);
`USB_CALC_BD(ep, dir, even)` maps the entry.

## Descriptors

| Descriptor | Contents |
|---|---|
| device | `bcdUSB 0x0200`; class `0x02` (CDC); EP0 max packet 8; VID `0x04d8`, PID `0xfb00`; `bcdDevice 0x0002`; 1 configuration; 3 named strings plus the language ID |
| configuration | 67 bytes; 2 interfaces; value 1; `bmAttributes 0x80` (bus powered, remote wakeup not advertised); `bMaxPower 0x64` = 100 × 2 mA = 200 mA |
| interface 0 | CDC communications: class `0x02`, subclass `0x02` (ACM), protocol `0x01`; header, ACM and union functional descriptors; 1 endpoint |
| interface 1 | CDC data: class `0x0A`; 2 endpoints |
| endpoint 1 | `0x81`, interrupt IN, 10-byte max packet, interval `0x40`; advertised but not driven by this firmware |
| endpoint 2 | `0x02` bulk OUT and `0x82` bulk IN, 64-byte max packet |
| strings | lang `0x0409`, manufacturer `Dangerous Prototypes`, product `Bus Pirate V4 `, serial `00000001` |

The inline comment next to `bMaxPower` still says "50=100 mA"; the actual byte
is `0x64` (200 mA).  Treat the descriptor arithmetic as authoritative.

## Initialisation and enumeration

1. `initialize_board()` (v4) calls `initCDC()`: loads the default line coding
   (115200, one stop bit, no parity, 8 data bits) and registers `cdc_setup` as
   the class handler.
2. `usb_init(device, config, strings, count)` stores the descriptor pointers,
   clears the handlers, calls `ClearUSBtoDefault()` and configures the
   peripheral (`ConfigureUsbHardware()`, packet transfer enabled).
3. `usb_start()` enables USB, sets `ATTACHED_STATE`, waits for the bus to leave
   SE0 and sets `DEFAULT_STATE`.
4. `main()` waits for `usb_device_state >= CONFIGURED_STATE`, then registers
   `CDCFlushOnTimeout` as the SOF handler.
5. `SET_CONFIGURATION(1)` sets `CONFIGURED_STATE` and calls
   `user_configured_init()`: EP1/EP2 are enabled and their BDs are armed,
   `cdc_setup` and the SOF flush handler are registered, and the A/B CDC
   pointers are reset.

The state values are ordered flags:

| State | Value | Meaning |
|---|---|---|
| `DETACHED_STATE` | 0x00 | no bus |
| `ATTACHED_STATE` | 0x01 | pull-up seen, waiting for SE0 |
| `POWERED_STATE` | 0x02 | defined; not assigned by this stack |
| `DEFAULT_STATE` | 0x04 | after reset, address 0 |
| `ADR_PENDING_STATE` | 0x08 | defined; the pending address lives in `usb_addr_pending` |
| `ADDRESS_STATE` | 0x10 | address assigned, not configured |
| `CONFIGURED_STATE` | 0x20 | configured; CDC endpoints live |

## Interrupt and transaction flow

`_USB1Interrupt` (in `main.c`) calls `usb_handler()` and clears the USB1 flag.
`usb_handler()` checks the peripheral flags in order:

| Flag | Action |
|---|---|
| idle/suspend | calls `usb_suspend()` (a no-op in `main.c`), clears the flag |
| reset | `usb_handle_reset()`: clears the current configuration/address, resets the BDT and endpoints, re-arms EP0 |
| error | clears the error flags (no recovery handler) |
| stall | clears the flag |
| SOF | calls the registered `sof_handler` (`CDCFlushOnTimeout` on v4) |
| transaction | dispatches only EP0; non-EP0 BDs are owned and polled by the CDC layer |

`usb_handle_transaction()` reads `USTAT`, maps it to the EP0 OUT/IN BDs, and
dispatches on the OUT BD's token: SETUP, OUT or IN.

## Control transfers (EP0)

`usb_handle_setup()` follows the USB control-transfer contract:

1. A new SETUP aborts the previous data/status stage: both EP0 handlers are
   cleared, the IN BD is reclaimed, the EP0 stall is cleared.  A SETUP that is
   not exactly 8 bytes is stalled.
2. The request type selects a standard recipient handler, the registered class
   handler (`cdc_setup`), or the registered vendor handler.  No vendor handler
   is registered, so vendor requests stall.
3. On success, the OUT BD is re-armed for the next SETUP; its data toggle is
   DATA1 when `wLength != 0` (the following OUT is a data stage) and DATA0
   otherwise.  `PKTDIS` is released only after both BDs are ready.

| Standard request | Behaviour |
|---|---|
| `GET_STATUS` | device: `usb_device_status`; interface: zeros; endpoint: stall bit from the even BD, bounded to allocated/enabled endpoints |
| `CLEAR_FEATURE` / `SET_FEATURE` (device) | remote wakeup only, and only when the configuration advertises it (it does not) |
| `SET_FEATURE` (endpoint) | saves the BD state and publishes a STALL BD |
| `CLEAR_FEATURE` (endpoint) | clears the EP stall, restores the saved BD, restarts at DATA0 |
| `SET_ADDRESS` | accepted for `0..0x7F`; the address is applied by the IN status handler (`usb_set_address`) |
| `GET_DESCRIPTOR` | device, configuration and string descriptors are streamed from ROM with the host's `wLength` bound; interface/endpoint/unknown types stall |
| `GET_CONFIGURATION` | current configuration value |
| `SET_CONFIGURATION` | value 0 deconfigures (disables non-control endpoints, clears the SOF/class handlers, `ADDRESS_STATE`); value 1 configures and calls `user_configured_init()` |
| `GET_INTERFACE` / `SET_INTERFACE` | interface 0 / alternate 0 only |
| `SET_DESCRIPTOR`, `SYNCH_FRAME` | stall |

Descriptor streaming (`usb_start_rom_reply`/`usb_send_rom`) sends at most the
requested length, toggles the data phase per packet and adds a terminating ZLP
only when the reply ends exactly on an EP0 packet boundary and is shorter than
the host's request.  A failed request calls `usb_RequestError()`, which stalls
both EP0 BDs.

## CDC class requests

`cdc_setup()` requires `wIndex == 0` (the control interface) and dispatches on
`bRequest`:

| Request | Behaviour |
|---|---|
| `SEND_ENCAPSULATED_COMMAND`, `GET_ENCAPSULATED_RESPONSE` | acknowledged with an empty status stage (not implemented) |
| `SET_COMM_FEATURE`, `GET_COMM_FEATURE`, `CLEAR_COMM_FEATURE` | stall (not advertised) |
| `SET_LINE_CODING` | requires `bmRequestType 0x21`, zero `wValue`, `wLength == 7`; data stage validated by `cdc_set_line_coding_data()` |
| `GET_LINE_CODING` | requires `0xA1` and zero `wValue`; returns `min(7, wLength)` bytes |
| `SET_CONTROL_LINE_STATE` | requires `0x21`, zero `wLength`, `wValue` limited to DTR/RTS (`& ~3`); status only, does not gate the console |
| `SEND_BREAK`, unknown | stall |

Line coding is seven wire bytes (CDC PSTN 1.2 Table 17) of virtual-port
metadata.  Validation checks the length, stop bits `<= 2`, parity `<= 4` and
data bits in `{5,6,7,8,16}`.  The baud field is stored, never used to divide or
to program a UART; a zero baud is therefore harmless.

## Bulk data path

Each direction has two 64-byte buffers (A/B) and one BD.  Ownership is tracked
by `UOWN`; the CPU removes ownership last when publishing and the SIE sets it
when done.

| Producer API | Contract |
|---|---|
| `getinbuf_cdc(&space)` | returns the current IN buffer and the free span (`64 - cdc_In_len`), with USB IRQs excluded |
| `write_cdc(count)` | commits at most the advertised span; a full 64-byte commit queues the packet and marks a pending ZLP; a larger count is rejected |
| `putc_cdc(byte)` | one byte through the same path, preserving an outer lock |
| `CDC_Flush_In_Now()` | commits a partial packet now |
| `setLock()` / `freeLock()` | exclude the SOF flusher while a multi-buffer producer writes; `freeLock()` flushes the partial tail first |

`CDC_Flush_In_Now()` and `putc_cdc()` save and restore the outer lock state, so
nesting is safe but not a reference count.  The SOF handler flushes only when
the producer lock is clear and the IN BD is free; it never waits for the host.
A full 64-byte packet leaves `ZLPpending` set so a host that expects a short
packet still sees one.

| Consumer API | Contract |
|---|---|
| `getc_cdc()` | blocking byte read; waits for a packet, then copies from the OUT buffer |
| `poll_getc_cdc(&b)` | non-blocking: recycles a completed OUT packet if needed, returns 0/1 |
| `peek_getc_cdc(&b)` | as poll but does not consume the byte |
| `getoutbuf_cdc(&count)` | returns a span of the current OUT packet, capped by the caller's request; with a non-zero request it blocks until a non-empty packet exists |
| `read_cdc(dest,count)` | copy wrapper over `getoutbuf_cdc()` |

OUT ZLPs and DMA counts larger than the buffer are recycled without being
reported as application bytes.  Bus reset/reconfiguration invalidates any span
previously returned by `getinbuf_cdc()`/`getoutbuf_cdc()`; a producer must not
retain or commit one across a reconnect.  When the host is configured but not
reading, the IN path waits for BD ownership to return (lossless backpressure);
there is no timeout that drops data.

## AVRISP-MKII personality hook

`user_configured_init()` checks `avrisp_usb_personality_active` before the CDC
setup.  When the AVRISP personality owns the device, it delegates endpoint
setup to `avrisp_usb_configure_endpoints()` and returns; the personality then
uses EP2 directly.  This is the only place `core_usb` calls into
`programmer_avrisp/`; the personality itself is documented in
`docs/programmers/avrisp/README.md`.

## Failure modes and edge cases

- **Unplug / bus reset while a producer holds a span.**  The BD is reset by the
  stack; the retained span is invalid.  Producers must discard it.
- **Host not reading.**  The IN path blocks for BD ownership instead of
  dropping data.  A producer in the SOF handler does not block; it leaves the
  data queued for the next flush.
- **Malformed control request.**  Length, interface, direction, request-type
  and field-range checks stall the request; a new SETUP always recovers.
- **Deconfiguration.**  `SET_CONFIGURATION(0)` disables EP1/EP2, clears the
  SOF/class handlers and their BDs, and returns to `ADDRESS_STATE`.
- **Line coding.**  A malformed seven-byte payload stalls the request and does
  not modify the stored line coding.
- **Invalid OUT count / ZLP.**  Recycled silently; the application never sees a
  phantom byte.
- **Suspended bus.**  The production `usb_suspend()` is a no-op; the stack
  clears the idle flag and continues.  A `WakeupUsb` macro exists in the
  register layer but is not called by the live path.
- **Startup held in SE0.**  `usb_start()` waits indefinitely for the bus to
  leave SE0.  The AVRISP personality uses its own bounded attach for the same
  wait; the core startup path has no timeout.

## Invariants

- A BD is published by writing the data/count first and `UOWN` last.
- A CPU-owned BD carries control bits; a SIE-owned BD carries a completed PID.
  Do not interpret `PID0` as `BSTALL` across ownership.
- Only EP0 is dispatched from the transaction interrupt; CDC endpoint BDs are
  serviced by the CDC API.
- `getinbuf_cdc()`/`write_cdc()` form one span contract: never commit more than
  the returned free space.
- `setLock()`/`freeLock()` protect a producer span from the SOF flusher; the
  lock is a flag, not a counter.
- All shared CDC pointers/counts and BD publications are updated with USB
  interrupts excluded; waits for the host happen outside those sections.

## Native test coverage

`tests/test_usb.py` (run from the repository root:
`python Firmware/tests/test_usb.py`) host-compiles the production control and
CDC functions with fake BDs and explicit IRQ state.  Its cases cover line
coding (including zero baud), endpoint status/bounds/halt, SETUP termination,
buffer state, poll/reset interleaving, descriptor termination,
deconfiguration, personality-specific requests, OUT ZLPs and a static PIC24
write-one-to-clear assertion.  It is a standalone harness; `test_native.py`
does not run it.  It is not an SIE model or a bus-analyzer trace, and it does
not model a physical unplug race.

## Related docs

- [system.md](system.md) — startup, dispatcher and the serial abstraction.
- `docs/dev/building.md` — build and native tests.
- `docs/programmers/avrisp/README.md` — the personality that reuses this
  stack.
- `docs/protocol/binary/README.md`, `docs/protocol/sump/README.md` — the
  protocols that share the CDC transport.
