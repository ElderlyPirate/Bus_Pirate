# Verification & evidence — base layer

Part of the generic-layer documentation set (`docs/generic/`).

## Verification

Evidence is listed as dated bench runs with their result.  The development logs
live outside this repository; individual runs are identified by log filename
where that helps.

### v4 bench, 2026-11-01 — this round

- **`m3` → `W` → `P` → `o1` → `v`** (UART).  Captured the full UART setup
  prompt chain, `POWER SUPPLIES ON` / `Clutch engaged!!!`, `Pull-up resistors ON`
  / `Warning: no voltage on Vpullup pin`, `Display format set`, and the `v`
  pin/rail table (VPU read 2.19 V), then a clean return to HiZ.  Firmware
  `e4b2239`.  Inlined in `usage.md` and `README.md`.
- **2WIRE wrapper/bit-op and read session.**  Captured `m6` setup prompts
  (`Set speed:`, `Select output type:`), `W`, then `[` → `(\-/_\-)I2C START
  BIT`, `/` → `CLOCK, 1`, `-` → `DATA OUTPUT, 1`, `_` → `DATA OUTPUT, 0`,
  `^` → `CLOCK TICKS: 0x01`, `\` → `CLOCK, 0`, `]` → `(\_/-)I2C STOP BIT`;
  then `m` + `1` back to HiZ.  A second run captured the read path with nothing
  on the bus: `r` → `READ: 0x00`, `{` → `(\-/_\-)I2C START BIT`, `]` →
  `(\_/-)I2C STOP BIT`, and `r:2` → `READ: 0x00 0x00`.  Firmware `e4b2239`.
  Inlined in `usage.md`.

### v4 bench, earlier runs

- **`generic_audit_20261101T144711Z`** — `=5` / `|5` conversions, the
  `<0>` / `<1=0x55>` / `<1>` macro flow, `rx` in HiZ, and a fresh `#` reset
  banner on the current build (`e4b2239`).
- **HDUART battery** (`hduart_presence2_20261031T173526Z`,
  `hduart_9600_16_20261101T092623Z`) — **19/19**: `PASS mode menu -> UART`,
  `PASS supplies on (W)`, `PASS pull-ups toggled (P)`, `PASS clean exit to HiZ`.
- **`dec_after` / `dec_after_run`** — exact `i`, `v`, and `e`-in-HiZ output.
- **`xsvf_after2` / `xsvf_after4`** — exact `i` banner after a binary-layer
  reset (also shows the `HiZ>` prompt).
- **`pic_frame_before`** — `P` + `Warning: no voltage on Vpullup pin`.
- **`verify_117_final`** — `W` → `POWER SUPPLIES ON` / `Clutch engaged!!!`.
- **`uart_terminal_console_20261026T130756Z`** — mode menu, `@` AUX read, the
  `Are you sure? ` bridge prompt.
- **`raw_numbits_after`** — `;16` accept and `;20` reject on writes (`WRITE:`)
  and reads (`READ:`); `m 6` direct mode change + `W`.

### Source verification

Every command mapping, message string, prompt and default on these pages was
verified against the firmware source before writing; the maintainer code map
lives in `docs/dev/code-map.md`.

## Not verified

- **v3 behaviour:** **build-only** on this bench.  The v3 column is derived
  from the v3 firmware; no v3 hardware was used.  In particular the v3 route
  to feed the Vpullup pin externally is derived from the firmware and the
  reference card, and is **not verified on v3 hardware**.
- Exact `h` history listing, exact `<n>` macro list/assign/run output, and
  exact `=`/`|` conversion spacing were not captured — derived from the firmware, so byte-exact output **requires verification**.
- The `r` + display-letter override (`rx`/`rd`/`rb`/`rw`) was not run in a mode
  where the read succeeds; code reading shows the letter is not consumed, so the
  leftover error is expected at char 2 — **requires verification**.
- SUMP entry (≥ 5× `0x00` + Left-arrow) and the binary-bitbang return (`0x0F`)
  are documented from the source but were not exercised this round.