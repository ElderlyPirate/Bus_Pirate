# Self-test

**Status:** draft — the device self-test.  v4 is the documented target; v3 is
build-only (unverified).

The self-test is the firmware's own bring-up and fault-finding check.  It
drives each board element in turn — the control lines, the on-board EEPROM
(v4), the power rails, the ADC, and the four I/O pins — and prints `OK` or
`FAIL` for every check, then a total error count.  Use it to confirm a board
is healthy after a build or flash, and to narrow down which element is at
fault.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [terminal.md](terminal.md) | entering with `~` (HiZ only), setup, the full output, reading a failure |
| [checks.md](checks.md) | the reference: every check, what it verifies, the `0x10` split, failure guidance, v3/v4 differences |
| [binary.md](binary.md) | the scriptable variants: `0x10` short / `0x11` full |
| [edge-cases.md](edge-cases.md) | failure modes and recovery |

## The five-minute version

- At the `HiZ>` prompt type `~`.  The test runs **in HiZ only**; in any other
  mode it prints `Self-test in HiZ mode only` and does nothing.
- The terminal `~` always runs the **full test**.  Jumpers and the target
  disconnect are in [terminal.md](terminal.md) §Setup.
- Every **check** line carries a result: `<name> OK` or `<name> FAIL`; a
  measurement check also prints its reading as `name(v.vv)`.  Group headers
  (such as `EEPROM`) are printed without a result.
- The run ends with `Found <n> errors.` — `0` means no check failed.
- The same test is scriptable over the binary bitbang layer: send `0x10`
  (short) or `0x11` (full) and read back one byte = the error count.  Which
  checks each pass covers is the `0x10` column in [checks.md](checks.md).
- **Disconnect any target first** — the test drives the Bus Pirate's own pins
  and supply.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| terminal `~` | present (build-only, unverified) | present |
| binary `0x10` / `0x11` | present (unverified) | present |
| on-board EEPROM check | — | present |
| USB supply (`Vusb`) check | — | present |
| jumper prompt | `Vpu`→`+5V` and `ADC`→`+3.3V` | `ADC`→`+3.3V` |
| end-of-run LED prompt | `MODE` and `VREG` LEDs | `MODE`, `VREG` and `USB` LEDs |

The full per-check table is in [checks.md](checks.md).

## Limits & out of scope

- The self-test checks the Bus Pirate's own board, not a target.  It does not
  test an attached device or any bus protocol.
- It cannot test parts the firmware has no line to: header wiring beyond the
  four I/O pins, the USB data path, or the enclosure.
- Shared prompt grammar (`m`, `W`, `v`, …) lives in `docs/generic/` and is
  not repeated here.
- There is **no JTAG-specific self-test**: JTAG is exercised through the JTAG
  and XSVF paths, not here.
- The v3 behaviour is documented from the firmware and is **unverified**.

## Related docs

- [docs/generic/](../../generic/README.md) — the shared prompt and the
  binary-entry sequence; its "Idle mode (HiZ)" section covers the mode in
  which the self-test runs.
- [docs/protocol/binary/](../../protocol/binary/README.md) — the binary
  bitbang layer that carries `0x10` / `0x11`.
- [docs/SPEC.md](../../SPEC.md) — the capability index.
