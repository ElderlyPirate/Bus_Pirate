# Bus Pirate v4 USB Bootloader

PIC24FJ256GB106, USB CDC `04D8:FAFF`, loader 5.0. One source set; no
PIC18 variants, alternate-device implementations or unused buffering modes.

## Fixed Layout And Flash Safety

All addresses below are PIC program/table addresses:

| Item | Address |
| --- | --- |
| Hardware reset GOTO | `0x0000` -> `0x0400` |
| IVT / AIVT | `0x0004` / `0x0104` |
| Loader allocation | `0x0400` through `0x1FFF` |
| Version | `0x1FF6` = `0x0500` (5.0) |
| Application entry | `0x2000` |
| Protected configuration erase page | `0x2A800` through `0x2ABFF` |
| CONFIG3 / CONFIG2 / CONFIG1 | `0x2ABFA` / `0x2ABFC` / `0x2ABFE` |
| Persistent entry flag / signature | RAM `0x47FA` / `0x47FC` |

CONFIG1/2 remain `0x3E7F` / `0x239E`; the 12 MHz primary crystal and DIV3
PLL settings yield the USB 48 MHz clock and FCY=16 MHz. Full rows are staged,
programmed and verified, with DISI protecting the key sequence. Erases are
bound to an accepted page; rows are aligned and checked against the complete
loader/configuration windows before any table write. Page zero is allowed for
application vectors but its reset GOTO always points to `0x0400`.

## Entry And Exit

NORMAL held, a PGC/PGD jumper, or the existing firmware signature plus entry
flag forces residency. Otherwise the loader reads the first application
instruction at **`0x2000`, not `0x4000`** and remains resident if it is erased.
Both its low 16 bits and high byte are checked. This is a presence guard, not
an image checksum or a guarantee against interrupted application programming.
The flag is consumed once; application handoff releases jumper/button
pull-ups, outputs and the loader timer. RB9 is the target supply enable,
never an activity LED, and is held off throughout loader execution.

Restart acknowledges the command without overwriting an SIE-owned buffer,
waits for IN completion (bounded to 250 ms), detaches for 100 ms, then resets.
A blank application returns to the loader. Interrupted packets require a new
hello after a 1 s idle timeout. USB reset, reconfiguration and endpoint halt
invalidate partial protocol state and pending erase; no stale packet may write.

## USB Implementation

The stack is deliberately polled: no fast path drops EP0 transactions and
no ISR shares C state with the parser. Hardware-updated BDs are volatile,
512-byte aligned, and use the PIC24 count/status/address layout. EP0 is 8
bytes; EP1 IN advertises a ten-byte notification endpoint; EP2 IN/OUT are
64-byte bulk endpoints. Boot replies are only 1/4 bytes, so each is already a
short packet; the firmware's streaming/ZLP machinery is unnecessary here.

EP0 tracks data/status phases, DATA1 status OUT, descriptor termination,
deferred SET_ADDRESS, stalls and SETUP cancellation. Requests validate type,
direction, recipient, interface, endpoint, index, selector and length. Invalid
descriptors/endpoints stall before any array traversal. GET_STATUS does not
modify a BD. Configuration zero disables all non-control endpoints. Halt
changes stop that DMA direction before reclaiming its BD and reset toggles.
Line coding is exactly seven wire bytes, never a native enum/struct ABI;
unsupported requests stall. OUT ZLPs are recycled, not delivered as bytes.

Timer1 supplies deadlines independently of USB SOF, so unplug/suspend cannot
freeze protocol timeouts. Idle enumeration is intentionally indefinite, but
never blocks USB servicing. Full USB suspend-current compliance is not
claimed: this loader does not implement board-wide low-power sleep.

Build/test via `../Makefile`; see `../README.md` for the manual audit, passing
5.0 loader-only bench and subsequent final combined application upload. The 5.0
hello is `F1 05 00 4B`; the reset GOTO opcode remains `0x04`, not `0x05`.

The owner's final combined `Firmware/bpv4.hex`, built from final head
`14256fd5`, SHA-256
`F15412B85A9C059E1C2D231C2316CB6FB8B5E4DFA59C0D8B2A1370F530E606D0`,
is now uploaded through bootloader 5.0. All 472 application rows plus eight
vector rows are accepted after loader-side flash readback, not an independent
application dump. Automatic post-flash restart returns to COM6 `04D8:FB00`,
banner `Bus Pirate v4` / `ElderlyPirate Firmware v9.0` / build `14256fd5`,
`HiZ>`, and a second responsive info query; no resident `04D8:FAFF` loader.
No ICSP command or manual reset/replug is used for that upload/check.

The 5.0 loader-only bench and this application upload are fresh evidence.
NORMAL-held and PGC/PGD-jumper physical entry cases were exercised on 4.15 and
were not repeated on 5.0. See `../README.md` for the separate evidence paths.
