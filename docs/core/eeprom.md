# EEPROM (`core_eeprom/`)

**Status:** current · maintainer-facing.  Covers the BPv4 onboard 24xx64
driver in `onboard_eeprom.c/h`: the internal-bus driver, its read/write
primitives, the address-only completion poll and the preserving self-test.
The driver is compiled on v4 only; v3 has no onboard-EEPROM driver.

## Scope and board model

| Item | Value |
|---|---|
| device | onboard 24xx64, 8K × 8, addresses `0x0000`..`0x1FFF` |
| bus | PIC24 I2C1 on the internal board bus; on v4 the external hardware I2C mode uses I2C3, and v3 I2C is software-driven |
| pins | RD9 SDA1, RD10 SCL1, RD11 write protect |
| slave address | 7-bit `0x50` (write byte `0xA0`, read byte `0xA1`) |
| clock programming | `I2C1BRG = 37`; `SCLREL` set; slew control enabled (`DISSLW = 0`); `SMEN`/`A10M` clear |
| page size | 32 bytes (datasheet); the driver only performs single-byte writes |
| supply / pull-ups | board-level 3.3 V and onboard pull-ups; the driver neither powers the device nor adds pull-ups, and these values are not encoded in firmware |

Write protect is active high: `LATD11 = 1` protects the array, and the
self-test drives it low to program.  "Assert WP" below therefore means "protect
the cells"; "deassert WP" means "allow writes for the duration of the test".

The 8192-byte array is **opaque storage**.  The firmware does not serialize
settings, versions, CRCs or slots into it, and RAM configuration is not
persisted by this module.  The disabled BASIC filesystem is a different,
incompatible layout (a 24LC256-sized image) and is not implemented here.

The interactive I2C mode's onboard-EEPROM macros (3 and 4) keep their own
access path and write-protect handling in `protocol_i2c/i2c.c`; this driver is
not that path.  Consolidating the two is an architectural follow-up, not part
of the current contract.

## Public API and callers

| Function | Contract |
|---|---|
| `eeprom_initialize()` | parks the internal bus: asserts WP, makes WP an output, releases I2C1, drives SDA/SCL latches low and leaves both pins released (inputs).  Called from `bp_reset_board_state()`. |
| `eeprom_test()` | preserving self-test at offset 0.  Returns true only when the original byte was restored and read back with the original value. |

Callers:

| Caller | Use |
|---|---|
| `capability_selftest/selftest.c` | `check_result(eeprom_test(), true)` after the WP/SCL/SDA line checks |
| `core_system/base.c` | `eeprom_initialize()` in the v4 board reset |
| `tests/eeprom_diagnostic_selftest.c` | test-image replacement for the self-test dispatcher; never a production source |

## Internal flow

### Bus ownership and release

`eeprom_test()` refuses to run when `I2C1CON.I2CEN` is set: another protocol
owns the controller.  It does not disable or reconfigure that owner.

`eeprom_release()` writes `I2C1CON = 0`, drains `I2C1RCV`, clears `I2C1STAT`
and releases both pins.  `eeprom_initialize()` asserts WP **before** releasing a
previously active peripheral or output.

`eeprom_i2c_setup()` clears the error flag, releases the controller, runs the
bus-clear sequence, then programs the controller: `I2C1CON = SCLREL`, address
mask/addressing cleared, `I2C1BRG = 37`, and finally `I2CEN` set.

### Bus clear

`eeprom_bus_clear()` is the single-master recovery path:

1. Release SDA/SCL and drive both latches low.
2. Require SCL to read high; otherwise fail.
3. If SDA reads high, the bus is clear.
4. Otherwise clock SCL nine times (5 µs phases), requiring SCL high after each
   release.
5. Require SDA high, then generate a STOP-like sequence (SCL low, SDA low,
   release SCL, release SDA) and require both lines high.

WP is asserted by the caller during recovery, so a recovery STOP cannot commit
an interrupted write that was never confirmed.

### Waits

All controller waits share `eeprom_wait(con_busy, stat_busy, stat_required)`:

- loops at most `EEPROM_POLL_LIMIT` (25000) iterations, one short delay each;
- aborts on `BCL`, `IWCOL` or `I2COV`, or if `I2CEN` disappears;
- succeeds when none of the nominated control bits are set, the nominated
  status-busy bits are clear and the required status bits are present;
- on any abort condition (timeout or a bus error) runs `eeprom_abort()`: set the
  error flag, assert WP, release the controller and pins.

Interrupts stay enabled during every wait.  The poll count is a bound, not a
wall-clock guarantee.  The write-completion poll additionally uses the shared
31-bit microsecond timestamp with a 10 ms budget.

### Read byte

`eeprom_read_byte(address, *value)` rejects `address >= 8192` and a NULL output
before touching the bus, then:

1. START, checked write `0xA0`, address high, address low.
2. Repeated START (`RSEN`, not a second `SEN`), checked write `0xA1`.
3. `RCEN`, wait for `RBF`, read `I2C1RCV`.
4. NACK the last byte (`ACKDT = 1`, `ACKEN`), STOP.
5. On any failure, attempt a STOP and return false; the output is untouched on
   failure.

### Write byte

`eeprom_write_byte(value, address)` rejects out-of-range addresses, then START,
checked write `0xA0`, address high, address low, data, STOP, and finally
`eeprom_ready()`.  It does not replay the payload.  Only single-byte writes are
performed, so there is no page-crossing burst path.

### Completion poll

`eeprom_ready()` uses address-only ACK polling after STOP (datasheet `7.0`):

1. START, write `0xA0`, sample `ACKSTAT`, STOP.
2. ACK means the write cycle completed.
3. Otherwise repeat, bounded by `EEPROM_READY_POLLS` (1024) and by a 10 ms
   deadline computed from `bp_timer_microseconds()` with the 31-bit mask.
4. Timeout aborts.

A protected write may still ACK without programming; the self-test therefore
always reads the byte back.

## Self-test

`eeprom_test()` preserves the byte at offset 0:

1. Refuse if I2C1 is enabled (another owner).
2. Record the current WP latch value; assert WP; set up the controller, wait
   for ready, read the original byte.
3. Deassert WP; mark the cell "touched"; write the complement; read it back and
   compare.
4. If a controller error occurred, assert WP, re-setup and wait for ready, then
   deassert WP for one bounded rollback attempt.
5. Write the original byte back and read it back again.
6. Release the controller.  WP returns to its previous latch value only if the
   cell was never touched or the restored value was read back successfully;
   otherwise WP stays asserted and the test returns false.

The healthy path uses two programming writes (complement, restore), not three.
A failure before the first write leaves the cell untouched and returns false
without claiming a repair.

## Failure modes and edge cases

| Trigger | Symptom | Behaviour | Recovery |
|---|---|---|---|
| Controller owned | `eeprom_test()` returns false; I2C1 unchanged | refuses when `I2C1CON.I2CEN` is set; does not disable the owner | the caller must not assume the EEPROM is good |
| Bus not clearable | the clear sequence fails | WP asserted; the test fails closed | check SDA/SCL and the onboard pull-ups |
| Address or data NACK | a checked write fails; the read returns false | a NACK never becomes a valid `0xFF` byte; the output is untouched | report the failure and retry at the caller |
| Protected write | the device may ACK while the cell is unchanged | only the readback decides success | deassert WP for the program window |
| Rollback impossible | the driver returns false and leaves WP asserted | one bounded restore attempt; it does not promise that a broken device was restored | treat the device as suspect and inspect WP |
| Timeout | every wait and the completion poll abort, the bus is released, failure is reported | bounded iterations and a 10 ms deadline; no fixed sleep or infinite wait | check the bus and device, then retry |
| Caller retry | none in the driver | callers must report a false result; silent retry is not part of the contract | the caller decides whether to retry |
| Another bus owner | the driver leaves I2C1 alone | the interactive I2C mode's onboard macros are a separate path | use that path or release the controller first |

## Invariants

- WP is asserted before the controller or pins are released on any abort that
  follows a write attempt.
- `eeprom_test()` never steals I2C1 from an enabled owner; the unconditional
  `eeprom_initialize()` is only reached from the board reset, after the mode
  cleanup.
- Reads and writes bound the private address to the 8192-byte array.
- Payload data is never replayed during completion polling; polling uses the
  address byte only.
- The original contents are restored and read back before `eeprom_test()`
  returns true.
- No settings format, version field or migration logic is introduced by this
  module.
- The driver performs single-byte transactions only; there is no page buffer.

## Native test coverage

`tests/test_core_eeprom.py` compiles the production driver against
deterministic register/device fixtures.  Its cases cover:

- the full self-test value/WP-entry matrix, full-array preservation and two
  completed programming writes;
- stuck START/RSEN/STOP/TX/RX/RBF/ACK, collision, held-clock/data, absent
  device, address/data NACKs, protected-write readback, successful error
  rollback and explicit failed-rollback protection;
- reads across all 8192 addresses, first/page-edge/last-byte writes, and
  invalid address/NULL rejection before bus action;
- address-only ready polling, timestamp wrap and a frozen test clock budget;
- the protected nine-clock recovery, deterministic configuration, latch
  preservation, ownership rejection and v3 exclusion guards.

Run it directly from the repository root:

```
python Firmware/tests/test_core_eeprom.py
```

The fixture is a register model, not a PIC24/SFR simulator, and no
internal-bus signal measurement is part of this set.

## Related docs

- [selftest](../capabilities/selftest/README.md) — the `~` self-test that
  calls this driver.
- [i2c](../protocol/i2c/README.md) and [i2c wiring](../protocol/i2c/wiring.md)
  — the external I2C mode, its own onboard-EEPROM macros and bus wiring.
- [system.md](system.md) — `bp_reset_board_state()` calls
  `eeprom_initialize()`.
- [messages.md](messages.md) — the strings the self-test prints.
