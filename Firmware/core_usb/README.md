# Core USB / CDC

USB/CDC is Part 1 of the core-family starter PR. Other `core_<x>/`
subsystems are separate owner-directed passes, not part of this USB audit.

## Ownership and calling contract

- PIC24FJ256GB106, full speed, 8-byte control endpoint, 64-byte bulk
  endpoint 2, interrupt-notification endpoint 1, no hardware ping-pong.
  The BDT is 512-byte aligned and contains six four-byte descriptors.
- UOWN is published last. A CPU-owned BD contains a completed PID where
  an SIE-owned BD contains CPU control bits; do not interpret PID0 as BSTALL.
- `setLock()` excludes the SOF flusher while a producer fills the span
  returned by `getinbuf_cdc()`. Commit no more than that span with
  `write_cdc()`. `freeLock()` flushes a partial tail before releasing it.
  A full tail schedules one idle ZLP, not a synthetic application byte.
- `CDC_Flush_In_Now()` and `putc_cdc()` preserve an outer producer lock.
  USB-IRQ critical sections protect reset-shared pointer/count updates and
  BD publication. Blocking acquisition/transmission waits are outside them.
  Nonblocking poll/peek only acquire a packet while it is known CPU-owned.
- Bus reset/reconfiguration invalidates outstanding raw buffer spans and
  abandons the old transport session. Do not retain a span across reconnect.
  Configured-host backpressure is intentional and lossless; no arbitrary
  timeout drops packets. The SOF ISR never waits for the host to read.
- CDC line coding is seven wire bytes of virtual-port metadata, not a
  physical UART configuration. DTR/RTS do not gate the Bus Pirate console.

## Audit and provenance

Reviewed `cdc.c/h`, `usb_stack.c/h`, `usb_stack_globals.h`, `picusb.h`,
the project configuration and CDC descriptors as one subsystem.

| Area | Finding / disposition |
| --- | --- |
| Prior stall investigation | Production patch already folded as `e01a05c`; `ebefa34` carries the base bulk writer and immediate tail flush. Do not fold the diagnostic instrumentation. SUMP stream formatting stays in the SUMP family. |
| CDC controls | `bbf0904`: replace native enums/padding with the specified seven bytes; validate lengths/interface/direction. `c1fea3b`: eliminate unchecked ISR division by baud zero and the unrelated UART1 write. |
| Endpoint controls | `d6a6774`: read-only, ownership-aware status. `64a71f8`: bound the allocated BDT and enabled direction, not the silicon endpoint count. `38b620d`: arm halts on free as well as queued BDs and reset DATA0 correctly on clear. |
| EP0 lifecycle | `1fa46a0`: SETUP aborts old callbacks, preserves error stalls, accepts DATA1 status and releases PKTDIS only after publication. `a6da6be`: descriptor replies stop at wLength/short tail, adding a ZLP only when needed. |
| Reset / configuration | `3b55e7e`: restore SOF on configuration; serialize pointer/count/BD changes and retain producer locks. The conditional AVRISP endpoint dispatch includes configuration before testing its feature gate (CDC side of `af427f9` / #240); no programmer engine is imported here. `8415c9b`: configuration zero retires noncontrol endpoints and callbacks. |
| Validation / dormant helpers | `af108b4`: active-personality interface bounds, standard request fields and advertised remote wakeup. `5ea9ff9`: correct the dormant PIC24 resume acknowledgement to word W1C. Unused CDC transfer/line-state and suspended-state storage is removed. |
| RX packets / races | `0e1c3bd`: recycle OUT ZLPs without advertising a byte, reject impossible counts. `611803f`: keep readiness and nonblocking consumption atomic across reset. |
| Instruction-set pass | `26d3da3`: specialize byte reads to avoid a general-span call per byte, retaining reset exclusion. CDC no longer imports 32-bit unsigned division/remainder helpers. MMIO flag clears remain word MOV writes; interrupt masking uses the hardware USB interrupt enable. No assembly timing loop or stack rewrite. |

## Manual basis

- PIC24FJ256GB110 Family Data Sheet **DS39897C**, 18.2 / pp.212-215:
  BDT layout/alignment, ownership, completed PID versus control fields,
  byte counts and data toggles.
- DS39897C 18.3-18.4 / pp.216-218: interrupt funnel, word write-one-to-clear,
  IN/OUT publication and completion. U1STAT p.225 and U1CON p.226 define
  direction/BD selection and SETUP's automatic PKTDIS gate.
- DS39897C pp.229-230, 233-238: no-ping-pong mode, internal transceiver,
  interrupt flags and DMA truncation, endpoint enables and stall status.
- USB 2.0 5.5.3, 8.5.3, 9.3-9.4: control transfer termination/abort,
  request fields, endpoint halt, configuration and interface semantics.
- CDC PSTN 1.2 6.3.10 / Table 17: fixed seven-byte line coding.

## Verification and limits

`python Firmware/tests/test_usb.py` host-compiles extracted production
functions with fake BDs and explicit IRQ state. Its cases cover line coding,
zero baud, status, bounds, halt, SETUP, buffer state, poll/reset interleaving,
descriptor termination, deconfiguration, personality-specific requests,
OUT ZLPs, and a static PIC24 W1C assertion. `--revision <commit> --case <case>`
supports the historical ungrouped source for before-state assertions.
These tests are not an SIE simulator or a USB analyzer trace.

The bench's durable `cdc_battery.py` uses simultaneous host reads/writes,
including 4096-byte OUT / 20480-byte exact-content IN bursts, full/partial
packet boundaries, idle/reopen/line-coding, and BBIO/terminal switches.
On the combined image it adds the SUMP family's 31-size capture matrix
(4..32768, including 64/2048/4032/4160 stall regressions), repeated captures
without post-RUN writes, padding/length/trailing-byte checks, metadata and
repeated SUMP/BBIO/terminal switches. The old standalone upstream SUMP has
no exit, so that protocol battery is only run on the combined image.

No USB bus analyzer, raw-control injection through Windows usbser, alternate
host OS, or in-flight physical-unplug stress is available in this pass.
Malformed controls, forced reset interleavings, halt and explicit OUT ZLP
injection use source tests/manual proof rather than dangerous live probes.
The dormant hardware-suspend helper is not a claim of live suspend testing:
the production suspend callback is a no-op. XM64 and the SPI fixture are
not required or changed.
