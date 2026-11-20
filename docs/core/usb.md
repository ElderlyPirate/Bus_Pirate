# USB (`core_usb/`)

**Status:** current · maintainer-facing.  Covers the BPv4 USB CDC device
stack: the transport that carries the terminal, the binary layer and the SUMP
stream.  v3 does not compile this group; it uses its FTDI serial link and the
v3 UART code in [`base.c`](system.md).

The stack derives from JTR's PIC USB/CDC implementation (CC-BY 3.0 headers in
the files).  It is a full-speed, bus-powered device with no hardware ping-pong
buffers; the CDC bulk path does its own **software A/B buffering** (two 64-byte
buffers per direction, one BD).  The stack selects the internal transceiver,
internal pull-ups and internal VREG, so the USB port needs no external pull-up
or supply rail from the firmware's point of view.

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
| interface 0 | CDC communications: class `0x02`, subclass `0x02` (ACM), protocol `0x01`; header, ACM, union and call-management functional descriptors; 1 endpoint |
| interface 1 | CDC data: class `0x0A`; 2 endpoints |
| endpoint 1 | `0x81`, interrupt IN, 10-byte max packet, interval `0x40`; configured with a zero-length BD but no notification data is queued by this firmware |
| endpoint 2 | `0x02` bulk OUT and `0x82` bulk IN, 64-byte max packet |
| strings | lang `0x0409`, manufacturer `Dangerous Prototypes`, product `Bus Pirate V4 ` (trailing space), serial `00000001` |

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

`usb_init()` clears all handler pointers, including the class handler that
`initCDC()` installed.  The class and SOF handlers are therefore live again
only after `user_configured_init()` runs during `SET_CONFIGURATION`.

The state values are ordered flags:

| State | Value | Meaning |
|---|---|---|
| `DETACHED_STATE` | 0x00 | no bus |
| `ATTACHED_STATE` | 0x01 | pull-up seen, waiting for SE0 |
| `DEFAULT_STATE` | 0x04 | after reset, address 0 |
| `ADDRESS_STATE` | 0x10 | address assigned, not configured |
| `CONFIGURED_STATE` | 0x20 | configured; CDC endpoints live |

The pending address is kept in `usb_addr_pending` and applied by the IN status
handler; there is no separate pending-address state value in this tree.

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
| `CLEAR_FEATURE` (endpoint) | clears the EP stall, restores the saved BD, and re-arms the data toggle for a fresh packet |
| `SET_ADDRESS` | accepted for `0..0x7F`; the address is applied by the IN status handler (`usb_set_address`) |
| `GET_DESCRIPTOR` | device, configuration and string descriptors are streamed from ROM with the host's `wLength` bound; interface/endpoint/unknown types stall |
| `GET_CONFIGURATION` | current configuration value |
| `SET_CONFIGURATION` | value 0 deconfigures (disables non-control endpoints, clears the SOF/class handlers, `ADDRESS_STATE`); value 1 configures and calls `user_configured_init()` |
| `GET_INTERFACE` / `SET_INTERFACE` | either interface (0 or 1), alternate 0 only |
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
to program a UART; a zero baud is therefore harmless.  DTR and RTS are
metadata for the host and do not gate the console.

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
nesting through those calls is safe.  The lock is one flag, not a reference
count: do not call `setLock()` twice and then `freeLock()` once.  The SOF
handler flushes only when the producer lock is clear and the IN BD is free; it
never waits for the host.  A full 64-byte packet leaves `ZLPpending` set so a
host that expects a short packet still sees one.  A zero-length `write_cdc(0)`
when the buffer is full publishes the full packet and makes progress rather
than spinning.

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
retain or commit one across a reconnect.  There is no generation counter:
treat a drop of `usb_device_state` below `CONFIGURED_STATE` or a new
`SET_CONFIGURATION` as invalidation, and re-fetch the span after the state
returns to configured.  When the host is configured but not reading, the IN
path waits for BD ownership to return (lossless backpressure); there is no
timeout that drops data.

## AVRISP-MKII personality hook

`user_configured_init()` checks `avrisp_usb_personality_active` before the CDC
setup.  When the AVRISP personality owns the device, it delegates endpoint
setup to `avrisp_usb_configure_endpoints()` and returns; the personality then
uses EP2 directly.  This is the only place `core_usb` calls into
`programmer_avrisp/`; the personality itself is documented in
`docs/programmers/avrisp/README.md`.

## Failure modes and edge cases

| Trigger | Symptom | Behaviour | Recovery |
|---|---|---|---|
| Unplug or bus reset while a producer holds a span | state drops below `CONFIGURED_STATE`; the retained pointer is stale | `usb_handle_reset()` resets the BDT and endpoints; the span is invalid | do not commit it; re-fetch after reconfiguration |
| Host configured but not reading IN data | prompt stops after the pipe fills; a producer blocks in `WaitInReady()`/`putda_cdc()` | IN path waits for BD ownership; no timeout drops data; a SOF producer does not block and leaves the data queued | drain or reopen the host port; the firmware cannot drop |
| Malformed control request | request stalls | length, interface, direction, request-type and field-range checks stall; a new SETUP always recovers | issue a valid request |
| `SET_CONFIGURATION(0)` | CDC endpoints disappear | EP1/EP2 disabled, handlers and BDs cleared, `ADDRESS_STATE` | reconfigure with value 1 |
| Malformed line coding | request stalls; stored coding unchanged | the seven-byte payload is validated before it replaces the stored value | resend a valid payload |
| Invalid OUT count or ZLP | no application byte | recycled silently | none |
| Suspended bus | no traffic | `usb_suspend()` is a no-op; the stack clears the idle flag and continues | resume the host |
| Startup held in SE0 | `usb_start()` never returns | waits indefinitely for the bus to leave SE0; the AVRISP personality uses a bounded attach | fix the D+/D- connection or host port |
| Endpoint 1 | a host waiting for a CDC notification waits forever | EP1 has a zero-length BD and no notification is queued | use the EP2 data path; do not wait on notifications |

`WakeupUsb` exists in the register layer but the live path never calls it.

## Invariants

- A BD is published by writing the data/count first and `UOWN` last.
- A CPU-owned BD carries control bits; a SIE-owned BD carries a completed PID.
  Do not interpret `PID0` as `BSTALL` across ownership.
- Only EP0 is dispatched from the transaction interrupt; CDC endpoint BDs are
  serviced by the CDC API.
- `getinbuf_cdc()`/`write_cdc()` form one span contract: never commit more than
  the returned free space.
- `setLock()`/`freeLock()` protect a producer span from the SOF flusher; the
  lock is a flag, not a counter, so match each `setLock()` with one
  `freeLock()`.
- All shared CDC pointers/counts and BD publications are updated with USB
  interrupts excluded; waits for the host happen outside those sections.

## Native test coverage

`tests/test_usb.py` host-compiles the production control and CDC functions with
fake BDs and explicit IRQ state.  Its cases cover line coding (including zero
baud), CDC class requests, endpoint status/bounds/halt, SETUP termination,
buffer state, poll/reset interleaving, descriptor termination, deconfiguration,
OUT ZLPs and a static PIC24 write-one-to-clear assertion.  Run it directly from
the repository root:

```
python Firmware/tests/test_usb.py
```

It is not an SIE model or a bus-analyzer trace: it does not model a physical
unplug race, bus timing or the electrical layer.

## Related docs

- [system.md](system.md) — startup, dispatcher and the serial abstraction.
- [building](../dev/building.md) — build and native tests.
- [avrisp](../programmers/avrisp/README.md) — the personality that reuses
  this stack.
- [binary](../protocol/binary/README.md), [sump](../protocol/sump/README.md)
  — the protocols that share the CDC transport.
