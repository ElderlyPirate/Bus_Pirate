# Core EEPROM - core starter Part 5

Owns `onboard_eeprom.c/h`, the BPv4 internal-bus 24xx64 storage and its
preserving selftest. Public APIs remain `eeprom_initialize()` and
`bool eeprom_test()`. The new driver fixes are self-contained; no paused I2C
engine or BASIC filesystem is imported or rewritten.

## Accepted content and callers

The original module is byte-identical in master, family and accepted combined
`decefc4`: C blob `e052f41de5e379d19b02757b463f0a838ca0ca56`, header blob
`c35b74e8a8a9c2219ee4fa4dd3954f89b0499dcc`. No duplicate body/history is folded.

Required direct routing fixes are source-authored clean folds:
`95b2bb4` -> `73bb7b4` clears the onboard route at new I2C sessions;
`651d979` -> `aadf2d1` rejects software-driver routing and cleans up I2C1.
The combined already carries them. These are accepted caller slices, not a
rebase or import of `13ba33e`/`144686a`'s hardware-engine implementation.
All new production driver fixes remain in `onboard_eeprom.c/h`.

## Findings and final dispositions

| Candidate | Disposition |
| --- | --- |
| E1, unbounded controller polls | `8207c6b`: finite 25000-poll waits for sequence/TBF/TRSTAT/RBF completion, enable/error checks, protected abort/release, internal nine-clock recovery and STOP. Interrupts remain enabled. No watchdog dependency. |
| E2, SEN used for a random read's second START | `2563886`: use RSEN after acknowledged word-address setup. DS39897C explicitly distinguishes the controls; sequence fixtures and target instructions verify them. No unsupported SEN-as-RSEN assumption. |
| E3, write returns before programming completes | `db2469f`: address-only ACK polling after STOP, bounded by the shared 31-bit timestamp/10ms deadline and a finite poll budget. Never replay payload data. No fixed 10ms sleeps. |
| E4/E14, destructive offset-0 test and wear | `3f89140`: save original before writing, test its complement, restore and read-verify. Healthy tests need two writes rather than three. One bounded recovery/rollback attempt follows controller errors; unverified rollback returns false with WP asserted. |
| E5, PORTD RMW | `706a95c`: private LATD9/10/11 aliases, latch-before-direction WP setup and latch-intent preservation. `fc31330` explicitly separates GPIO direction changes from first PORT sampling by a NOP. No external sibling latch writes. |
| E6, DISSLW/SCLREL | The pre-read's direction claim is wrong: DISSLW=0 enables fast-mode slew control; SCLREL is slave-only. `8207c6b` deterministically sets the correct 400kHz configuration and clears stale state. No physical edge-rate claim. |
| E7, size/bounds/page ambiguity | `a96823b`: BPv4 design documents IC3 as 24xx64, 8K x 8, 32-byte pages. Reject private addresses beyond 0x1FFF and NULL read outputs before bus action. Writes remain single-byte+STOP+completion, so no page-crossing burst exists. |
| E8/E12, ignored address ACKs/ambiguous return | `8207c6b`: check device, word-address and data ACKs; byte reads use explicit status/output and writes return true on success. NACK cannot silently become valid 0xFF data. |
| E9, leaked controller/stale status | `8207c6b`: decline an enabled controller owned by another protocol without changing it. Owned operations deterministically initialize/clear state and leave I2C1 disabled/released. Init protects WP before releasing ownership. |
| E10, two independent I2C1 implementations | Architectural follow-up only, as owner directed. The generic I2C protocol engine stays paused; consolidation can be considered with family rebases, not forced into this core change. |
| E11, static/top-level const style | Prior static declarations already gave internal linkage; no standalone bug. New private status signatures use explicit static definitions as required by their functional changes. |
| E13, disabled BASIC filesystem | Remains a note: no PGMSIZE, wrong/external pin path and incompatible 24LC256 layout. Do not revive or define a new filesystem/persistence format here. |

The paused I2C-family macro (4) still uses its own PORT-based WP write; this
is a caller-family follow-up, not silently claimed fixed by the private driver.
The interactive whole-board selftest is `~`, not `v` (`v` prints pin states).
Its EEPROM labels/call and BBIO selftest call sites remain unchanged.

## Hardware/storage contract

BPv4 IC3 is documented as an onboard 24xx64 at 7-bit address 0x50, powered
at 3.3V, with dedicated RD9/SDA1, RD10/SCL1 and RD11/WP plus onboard 2K
pull-ups. The 8192-byte array is opaque. This tree has no live settings
serialization/version/CRC/slot layout; RAM configuration is not persisted by
this module. No new layout, capacity-probing write or migration is introduced.

- DS39897C 10.1/10.2.1: LAT versus PORT, TRIS release and direction-to-read
  spacing. Local latch writes preserve all external PORTD siblings.
- DS39897C 16.1, Register 16-1 (pp.194-195), Register 16-2 (pp.196-197):
  SEN/RSEN/PEN/ACKEN/RCEN, TBF/TRSTAT/RBF, ACKSTAT and collision/overflow flags.
  SCLREL is slave-only; DISSLW=0 enables slew control.
- DS20001189 5.0/6.1/6.2/6.3/7.0: 24xx64 addressing, byte/page writes,
  32-byte page boundary, WP sampled at STOP, and address ACK polling. A
  protected write may still ACK without programming; readback is essential.
- DS70000157G BSET/BCLR, word arithmetic and branches support the target
  register/latch and finite-loop proof. The poll count is not advertised as
  a portable wall-clock deadline; write-cycle deadline math follows the
  existing 31-bit microsecond API and is tested across wrap.
- BPv4 design/onboard-24AA documentation identifies IC3 as 24xx64/8K, unlike
  the disabled BASIC 24LC256 assumptions. Replacement boards/parts must honor
  this explicit contract; destructive wrap-probing is not used to size them.

True from `eeprom_test` means original contents were restored and verified.
False must be reported. If physical failure prevents rollback verification,
the driver fails closed with WP high; no software can promise restoration to
an absent/broken device. Callers must not hide that condition with retries.

## Source/native proof

`python Firmware/tests/test_core_eeprom.py` compiles the actual driver with
deterministic register/device fixtures, not a PIC24/SFR simulator. It covers:

- 512 original-byte/WP combinations, full-array preservation and two writes;
- stuck START/RSEN/STOP/TX/RX/ACK/RBF, collision, held-clock/data and absent
  device, address/data NACKs, protected-write false-pass prevention, successful
  error rollback and explicit failed-rollback protection;
- all 8192 reads, first/page-edge/last byte writes, invalid addresses/NULL
  rejection before bus action and untouched outputs on failure;
- address-only ready polling, timestamp wrap and a frozen test clock budget;
- protected nine-clock recovery, deterministic config, LAT preservation,
  controller-ownership rejection, V3 guards and accepted route/caller checks.

`--before` runs the original public selftest body against saved 0x3C and
demonstrates its successful return with storage replaced by 0xFF; source and
the original target object preserve the five unbounded loops/SEN-only proof.
The original destructive/unbounded test is never deliberately run on the unit.

## Safe hardware gate

A full 8192-byte backup is double-read and hash-verified before any cell write.
Read-only pointer frames contain no data byte. No raw macro (4), external
power/pull-up source, out-of-range physical write or whole-board GPIO selftest
is used against the attached SPI rig.

`tests/eeprom_diagnostic_selftest.c` is a test-image replacement for the
whole-board selftest dispatcher, never a production source. In a temporary
copy it calls the real EEPROM API only, checks WP/disabled/released state,
then restores the original WP. The healthy EEPROM object is instruction/
relocation-identical to production. Both WP-entry states are exercised.

Two explicit negative images use a non-EEPROM device code or RAM MMIO where
SEN cannot complete. They verify NACK/dead-controller failure returns without
physically holding the internal bus or programming cells. Every diagnostic
image has a unique exact banner; all 8192 bytes are checked against the full
backup after every image. The exact production image is then restored for
its core/CDC/read-only EEPROM gate. Diagnostic evidence is not mislabeled as
the production image, nor as a physical stuck-wire test.

Internal RD9/RD10 probes are not connected: no internal-bus scope capture or
formal 400kHz edge compliance is claimed. RSEN is verified by the datasheet,
register sequence, actual target instructions and successful real-chip API
read/write/restore. The scope remains on the external fixture; the AUX-CLK
bridge remains removed and XM64 remains spi-flash-emu.

Final tips, backup hashes, diagnostic cases and all acceptance logs are indexed
in local `.bpv4-work/eeprom/acceptance-final.json` and the single PR #209 body.
No new PR comment is posted. No WORKLOG edit or other-family rebase is done.

## Grouping and completion

Both files live under `Firmware/core_eeprom/`. MPLAB item paths, V3 exclusions
and exported header directories follow, as do CLI source/include paths.
Normal onboard_eeprom.h spelling remains valid, without SPI include edits.
The family local Makefile remains pre-existing/untracked; combined's tracked
Makefile follows the move. Grouping is measured separately from the fixes:
V4 EEPROM instructions/relocations and all V4/V3 map summary rows match.
V3 does not compile this V4-only driver.

Core starter Parts 1-5 are complete at acceptance. Final bench state is the
exact combined image at HiZ, supplies off, AUX input, original EEPROM bytes
verified unchanged. Downstream family rebases remain owner-directed.
