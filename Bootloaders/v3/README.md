# Bus Pirate v3 ds30 Loader

PIC24FJ64GA002 only; bootloader 5.0, one retained source set, UART1 RP5 RX / RP4 TX.
The UART BRG remains `0x22` for the existing silicon A3 workaround; the host
must select 115200 8N1 (not the old Windows host's erroneous 921600).

## Fixed Layout

All addresses below are PIC program/table addresses:

| Item | Address |
| --- | --- |
| Hardware reset GOTO | `0x0000` -> `0xA800` |
| IVT / AIVT | `0x0004` / `0x0104` (application-owned during normal operation) |
| Relocated application reset GOTO | `0xA7FC`, `0xA7FE` |
| Loader | `0xA800` through `0xABF7` |
| Legacy firmware entry | `0xABF8` |
| Version | `0xABFA` = `0x0500` (5.0) |
| CONFIG2 / CONFIG1 | `0xABFC` / `0xABFE` = `0x799F` / `0x3F7F` |
| Persistent entry flag / signature | RAM `0x27FA` / `0x27FC` |

The linker no longer offers application space to the loader allocator. The
last valid aligned row starts at `0xA780` and includes the relocated GOTO.
Only `TBLPAG=0` is allowed. The entire last erase page is protected, including
configuration words. ICSP, not this loader, installs or updates the loader.

## Entry And Errors

PGC/PGD jumpered at reset enters the loader. The normal firmware may request
entry through the existing persistent flag/signature. Legacy entry at
`0xABF8` now sets those values and executes RESET, rather than inheriting live
application peripherals and interrupts. The stack pointer and SPLIM are
initialized; no interrupt-driven UART or NVM path is used.

Without a jumper/request, the loader only leaves if the relocated application's
first instruction is not erased. Jumper and legacy `#` exit paths remain;
UART timeout/error clears the pending erase and either remains resident or
returns to the application. An erased application always stays recoverable.
UART receive handles overrun, framing and parity faults; transmit observes
FIFO capacity and drains before exit.

Strict command/length, table-page and alignment checks precede all NVM access.
The NVM unlock is guarded by DISI, followed by the two required NOPs; WREN is
cleared afterward and WRERR/readback failures return `V`.

Build/test via `../Makefile`; see `../README.md` for protocol and evidence.
The hello still reports ds30 protocol 1.0.2; the stored loader revision is 5.0.
The application revision is separately v9.0 on both board revisions.
**No v3 hardware is available: this version is not bench-qualified.**
