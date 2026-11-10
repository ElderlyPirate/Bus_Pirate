# Self-test — edge cases & failure modes

Part of the self-test documentation set (`docs/capabilities/selftest/`).
Concrete, non-happy-path behaviour: trigger, symptom, behaviour, recovery.
The check meanings are in [checks.md](checks.md).

| Trigger | Symptom | Behaviour | Recovery |
|---|---|---|---|
| `~` while the mode is not HiZ | `Self-test in HiZ mode only` | the test does not run; the prompt is unchanged | switch to HiZ (`m1`) or reset (`#`), then `~` |
| full test with the `ADC` jumper missing or not seated | `ADC FAIL` and the total count rises by one | the external probe reads outside the 3.3V window; every other check still runs | fit `ADC`→`+3.3V` and rerun |
| v3 full test with the `Vpu`→`+5V` jumper missing | `VPU FAIL` and the four `Bus Hi-Z 1` pin checks fail from the same missing reference (`ADC FAIL` too if that jumper is also missing) | the `VPU` check reads outside its 5V window and the four pulled-up inputs do not read high | fit the jumper(s) and rerun |
| binary `0x11` with no jumpers | the reply byte is greater than 0 and the MODE LED stays on | `ADC` fails on both boards; on v3 `VPU` and the four `Bus Hi-Z 1` checks also fail from the missing reference | fit the jumpers and rerun, or use `0x10` (short) |
| a rail is shorted or unpowered | `5V FAIL` / `3.3V FAIL` / `Vusb FAIL`, with the reading near `0.00` or out of band | that rail check fails; other rails may still pass | clear the short or check the power source, then rerun |
| an I/O pin is stuck low | a pin in the `Bus high` group prints `FAIL` | the pin cannot be driven high | inspect the pin and header for a short or damage |
| an I/O pin does not pull high as an input | a pin in the `Bus Hi-Z 1` group prints `FAIL` | the pull-up reference or the pin is at fault | check the pull-up source (on v3, the `Vpu`→`+5V` jumper) |
| EEPROM write/read fails (v4) | `READ&WRITE FAIL`, while `SCL` / `SDA` / `WP` may still pass | the inverse-byte write did not read back or the original could not be restored; the test also declines if the hardware I2C engine is still enabled | reset (`#`) and rerun; if it persists, the on-board EEPROM may be faulty |
| a target is left connected | checks fail unpredictably | the test drives the Bus Pirate's own pins and rails into whatever is attached | disconnect any target before running (the firmware asks for this) |
| binary self-test: the host sends `0xFF` as data | the test exits early | `0xFF` is the leave-the-test byte, not data | avoid `0xFF` while the test is idle; rerun if needed |
| binary self-test: the host expects the terminal after the test | no `HiZ>` prompt appears | the firmware is still in the bitbang layer | send `0x0F` to return to the terminal |
| binary self-test: the host reads back the byte it sent | the reply is `byte + errors`, not a bare echo | the error count is added to the echoed byte | subtract the known error count, or read the count byte once at the start |

## Notes

- The terminal `~` has no short-test form; a jumper-free run is available only
  as the binary short test (`0x10`).  See [binary.md](binary.md).
- A `FAIL` means the observed result did not match the expected one.  A
  missing jumper and a faulty element both produce it; the label identifies
  the failed check, not the cause.
- The groups `EEPROM`, `ADC and supply`, `Bus high`, `Bus Hi-Z 0` and
  `Bus Hi-Z 1` never fail on their own; only the labelled checks below them
  do.

## Related docs

- [terminal.md](terminal.md) — the interactive run.
- [checks.md](checks.md) — every check and what it verifies.
- [binary.md](binary.md) — the `0x10` / `0x11` variants.
