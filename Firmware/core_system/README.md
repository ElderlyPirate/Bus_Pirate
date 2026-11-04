# Core system - core starter Part 2

Owns board/console initialization, shared mode state and dispatch, terminal
editing/parsing, formatting, delay helpers and the user-facing serial
abstraction: `base.c/h`, `core.c/h`, `proc_menu.c/h`, `main.c`.
USB transport remains in `core_usb/`. Protocol engines and the later
`core_bitbang` / messages / EEPROM passes are not included here.

## Pre-read dispositions

| Owner candidate | Verified disposition |
| --- | --- |
| 1. Unused width mask | Confirmed. `4455210` applies the masked unsigned value to HEX/DEC/BIN/RAW; the 1..16-bit test matrix and bench cover partial widths. |
| 2. Mode state leakage | Confirmed. `7aea6ff` resets byte width, word/duplex/error state centrally before the new protocol setup. The old I2C-to-UART bench sequence retained 16 bits without doing wire I/O. Protocol setup may subsequently override its own settings. |
| 3. Millisecond truncation | Historical/already fixed in combined, not rediscovered. `1f0e211` carries the existing #208 / `a94ebc7` unsigned-32 multiplication into this standalone core base. Baseline 66/100/500 ms delays still wrapped. |
| 4. Ring wrap | Confirmed. `3329c1e` masks insertion/deletion and periodic redraw indices. It also fixes first/end-cursor delete bounds. Production-handler tests cover every seam with guard bytes. |
| 5. Mode-name asymmetry | Confirmed. `689a85b` uses `1WIRE`, matching `2WIRE`/`3WIRE`, in the fixed-size dispatch name. The owner routed this item here rather than the wire engine. |
| 6. Baud wait | Confirmed. `e53c4a8` makes USB baud selection inert/host-controlled. Physical v3 UART confirmation has a five-second Timer1 deadline and restores the previous speed/BRG if abandoned. |
| 7. Escape reads | Confirmed. `17d12d5` replaces speculative blocking reads with an incremental, bounded escape state machine in the input loop. No timeout/retry workaround is used. |
| 8. NUL counter | Confirmed. `80ab44a` requires consecutive prefixes; ANSI keys/ordinary input clear them. Raw NUL/STX SUMP and twenty-NUL BBIO remain supported. `5d8c09f` also removes the BBIO goto into uninitialized parser locals and performs a real terminal/board state reset. |
| 9. Power/pullup RMW | Confirmed. `c9cae6e` writes v3/v4 regulator/pullup and core-owned LED latches, not PORT readback, and establishes latch data before output direction. Getters still read pins. |
| 10. getint | Confirmed. `4ffe2a7` is nonmutating, ring-bounded and overflow-checked, fixes leading-zero decimal duplication, and validates widths/counts. `b7f027a` promotes before adding repeat sentinels. `4d3b336` / `9637722` also fix signed and overlong dialog values before table selection. |

Secondary pre-read results:

- The claimed dead `busmode == -2` abort is **rejected**: `getnumber()` returns
  -1 for `x`, and the caller subtracts one. `m0` remains a compatible menu
  alias. Negative/out-of-range actual selections are rejected before indexing.
- The Timer1 comment was wrong, not the clock configuration. `7bf1b80`
  corrects TCS and the nonexistent Timer1 T32 label without changing T1CON.
- Macro auto-execution is **not introduced**: loading for review is retained.
  The real cursor/terminator/bounds fault is fixed by `aef8a36`, which replays
  through the same editor as normal typing.
- The MISO/PPS question is outside these files and is not a core-system fix.

## Additional audit findings

- `d138246`: the low-word flash helper ORed TBLRDH into bits 15:8; use TBLRDL
  alone and preserve TBLPAG restoration.
- `c852f47`: decimal dword output selected only eight positions. Ten positions
  now cover all of uint32, including values above 99,999,999.
- `cca13b8`: history allowed an uninitialized position, listed its current
  command and recalled multiple entries per arrow. Bound selections to
  initialized entries, exclude the active command and stop after one recall.
- `d583f9b`: v3 UART transfer globals need foreground/ISR volatile discipline;
  null, zero and already-completed transfers must not index the buffers.
- `9e410fb`: consume `rx/rd/rb/rw`'s display selector before repeat/width
  suffixes; otherwise the selector becomes a spurious next command.
- `d0d8797`: carry the existing #232 / `c3185fa` word-alignment contract on
  the shared terminal/sample allocation into the core-owned declaration.
- The final context-header sweep also moves both VPU source-selection
  writes from PORTF to LATF, preserving break-before-make and the low-latch
  open-drain policy. Five-volt source injection is not used on the attached
  3.3 V fixture; the source/MMIO proof covers that leg.
- Bit-reversal table entries are cast to uint16 before shifting: otherwise
  uint8 promotes to signed 16-bit int and high-byte values overflow that
  signed shift. Exhaustive word/partial-width reference checks cover it.

The reset path cleans the old protocol and clears power, width, duplex,
editor/prefix and pending macro state. It does not jump across initialization
of command-parser locals. SUMP/BBIO engines themselves are not rewritten.
The combined integration retains existing capability-reset, scripted-mode,
timer, private-dispatch and protocol batching additions from their families.

## Manual and optimization basis

- **DS39897C 10.1 / p.133**: PORT reads pins, LAT reads the output latch;
  TRIS controls direction. 10.2.1 / p.134 specifies write/read spacing.
- **DS39897C 11.0 / pp.161-163**: Timer1 TCS=0 selects FOSC/2; prescale 1:8
  gives two ticks/us at FCY=16 MHz. Elapsed unsigned differences handle wraps.
- **DS39897C 4.3.2**, **DS70000157G pp.440-443**: separate high program byte
  and low word; TBLRDL returns bits 15:0, not a partial word needing TBLRDH.
- **DS70000157G programmer model, addressing, SL, MUL and DIV/REPEAT**:
  16-bit arithmetic, unsigned mask/count operations, even word alignment,
  and instruction-set proof. PIC24F has iterative divide instructions; it
  is incorrect to claim it has no hardware divider.
- C bounds/unsigned arithmetic and the public UI contract, rather than a
  peripheral manual, govern editor indices and display/token semantics.

`7bf1b80` replaces per-digit division in checked parsing with precomputed
overflow thresholds for the three radices. Exhaustive decimal/uppercase-hex
equivalence covers all 65,536 values. Flash reads also lose an unnecessary
TBLRDH/shift/OR sequence. No wait/retry or blind recovery is used.

## Tests and integration

`python Firmware/tests/test_core_system.py` compiles extracted production
functions with deterministic serial/MMIO mocks. It covers formatting/state,
all ring seams, full uint16 parsing and nonmutation, signed/overlong dialogs,
baud/Timer1 wraps, ESC/prefix/reset, latches, full uint32 output, editable
macros, history, v3 ISR bounds and read selectors. `--revision 7c02028 --case
<name>` retains before-state assertions. The mocks are not a PIC simulator.

The durable bench `core_battery.py` exercises terminal/mode entry, formatting,
state hygiene, editing/history/macros, delays, baud/escape/NUL behavior,
power/pullups and reset. It never enters SPI or accesses XM64. `cdc_battery.py`
is rerun as transport regression; enhanced SUMP coverage belongs on combined,
since the standalone upstream SUMP has no exit. Known old-image hang and
memory-corruption probes use source/manual proof rather than a live wedge.
V3 physical baud/ISR and forced shared-port pin/latch mismatches are not
available on this v4-only bench; source tests, disassembly and clean v3 builds
are the cited verification path.

All seven sources/headers live in `core_system/`. The project exports that
header search directory so existing clients, including the paused `spi.c`,
need no implementation or include edits. MPLAB/CLI source paths follow the
move; legacy nested relative includes are updated mechanically. Grouping
is verified by unchanged V4/V3 map layouts and all four core objects'
instructions/relocations, separately from the functional fixes.
