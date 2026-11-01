# LCD (HD44780)

**Status:** stub — migrate + expand.  The LCD adapter is **not available for
testing**; write from the firmware and the existing page, and mark unconfirmed
behavior `Requires verification`.

HD44780 / 74HCT595 LCD adapter mode: display control via the Bus Pirate LCD
adapter.  Not built in the default v3/v4 images (`BP_ENABLE_HD44780_SUPPORT`
is off; optional build only).

- Source today: `Documentation/hd44780.md` (82 lines, 2016 — migrate, tidy).
- Planned: `README.md` (+ adapter wiring and macro reference).
- SPEC: `docs/SPEC.md` §2 (HD44780 row: `off` / `off`).