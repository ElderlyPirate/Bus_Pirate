# Core bitbang - core starter Part 3

Shared software GPIO bus engine: `bitbang.c/h`. Protocol engines, SPI and the
next `core_messages` audit are not part of this pass.

## Accepted baseline and clean folds

The pre-read did not inspect the starter branch. Its original `bitbang.c`
was master blob `aec017601f4c...`, not the combined's accepted implementation.
At `237d175`, both files exactly match combined `b4df1be`:
`5384b0e96aecb7f085934bc715037179481af038` and
`38dc8a12556cd5972e2afd5e5a9d13aa3958fc7c`.

| Accepted source | Clean starter fold |
| --- | --- |
| `78e25bd`, initial I2C stretch intent, Brett Foster co-author | `f02bcc7` |
| `2a54822`, bounded data/ACK waits and saturating count | `b264e4e` |
| `aed44fd` / `c4c401c`, direct release polling | `3eeed5a`; alias not duplicated |
| `a6079c2` / `af1bc3e`, hoisted raw clock loop | `dabbbf7`; core delay prerequisite already in Part 2 |
| `b298457`, short-delay setters | `2b20f38` |
| `caaf1fd` / `80cffbc`, inline delay poll | `6035b17`; core batching prerequisite already in Part 2 |
| `a2ce1b3`, sticky faults, full post-release high phase, bounded signals | `b9d7817`, including minimal standalone I2C client adaptation |
| `868a673`, inline value loops with the accepted combined wait integration | `237d175` |
| `95f91cd` / `bf47033`, corrected profile menu/catalog | `521e344`; exact combined strings, no regeneration/reimplementation |
| `ce6a9b9` / `2ebc544`, state-only setup at mode entry | `5755fc3`; required raw2/raw3/software-I2C setup blocks only |

Original author identity/date are preserved on all ten folds, and the initial
stretch fold retains Brett Foster's co-authorship. Messages cite their sources.
No merge commits, duplicate aliases, instrumentation or other-family grouping
are imported. Signature adaptations explicitly keep raw callers non-waiting.
`7def1f8` restores only pre-existing I2C comment bytes after the local fold
helper's Windows-codepage conversion; it changes no executable code.

The final executable bitbang source, **including all waiting paths**, remains
identical to the accepted `237d175` state after comments/includes/whitespace
are removed. All nine raw helper bodies named by the I2C raw-source audit are
also byte-identical. This pass neither re-derives nor re-optimises accepted work.

## Candidate dispositions

| Candidate | Verified disposition |
| --- | --- |
| B1, profile labels/path dependence | Confirmed stale labels on starter. Absorb the already-accepted label correction (`521e344`); `66d64be` documents legacy enum names, approximate tick rates and per-primitive overhead. Do not retime accepted loops. |
| B2, waiting calls release CLK in push-pull mode | Public contract gap, not a shipped caller bug. `778d9e4` explicitly requires high_impedance=ON and pull-ups for waiting I2C; raw clients pass false. No silent mode change or extra hot-path guard. |
| B3, NULL delay profile before setup | Explicit setup is required, not an implicit default. `b665b46` documents it; the existing profile-at-entry fix is absorbed as `5755fc3` so raw/software-I2C callers establish it before W. No new default/guard implementation. |
| B4, 25000 iterations allegedly below 25 ms | Rejected on the tested accepted XC16 target: CH2 low-clamp pulses are 25.54..25.64 ms, versus 0.171 ms control. `09a1266` distinguishes the iteration bound from a portable wall-clock deadline. No waiter rewrite. |
| B5, fixed MOSI/MISO sampling | Intentional primitive roles; `ac73a68` documents fixed value pins versus setup-selected single-bit input. |
| B6, widths validated by callers | Retain the split. Part 2 rejects invalid terminal widths/counts; binary raw fixes eight bits. `b665b46` states 1..16 and setup preconditions. No clamp or extra engine branches. |
| B7, legacy START true=error vs waiting true=success | Intentional ABI; `ac73a68` makes the inversion and non-waiting legacy behavior explicit. |
| B8, set_pins delay says LOW | Documentation defect fixed by `ac73a68`: requested state; high-impedance HIGH is released, not driven. |
| B9, error/counter lifetime | Intentional. Operation status is explicitly cleared, counter saturates since reset and survives setup; documented, not reset behind callers' backs. |
| B10, RMW/settling | No defect: state writes use IOLAT/IODIR, reads use IOPOR, and direction-to-sample paths retain their three Nops. No PORT write is introduced. |

## Manual and instruction-set basis

- DS39897C 10.1 / p.133: TRIS=1 releases the output, LAT reads/writes latch
  state, PORT reads actual pin state. 10.1.1 covers open-drain behavior.
- DS39897C 10.2.1 / p.134: an instruction cycle must separate a direction or
  port write from a read of that port. Accepted sampling Nops are retained.
- DS39897C 11.0 / pp.161-163: FCY=16 MHz, Timer1 prescale 1:8 gives 2 ticks/us.
  A half-microsecond tick alone is not the complete bounded-wait iteration.
- DS70000157G programmer model and BSET/BCLR, CP, ADD/ADDC, SUB/SUBB, BRA and
  shift descriptions: single-bit latch/direction operations, 16-bit widths,
  branch/counter overhead and bounded-loop instruction proof. PIC24F does
  have divide instructions; no contrary optimization premise is used here.

Inspection of the actual V4/V3 objects retains the accepted direct GPIO and
inline poll paths. No new optimization is justified by these findings;
instruction/relocation comparison guards against accidental rework instead.

## Bench and regressions

The CH2-only Owon path uses x10, range `10v`, DC. A static LOW/HIGH/LOW check
verifies the probe chain. Three pipelined frames are discarded after changing
stimulus; early stale-frame/missed-burst captures are not acceptance evidence.
No USB/command gaps are included in intra-burst rate estimates.

Accepted before-state `237d175`, representative 2WIRE rates (kHz):

| Profile | Ticks | Write | Read |
| --- | --- | --- | --- |
| 1 | 6.23 | 5.54 | 6.23 |
| 2 | 60.68 | 57.47 | 59.67 |
| 3 | 153.85 | 134.77 | 147.93 |
| Maximum | 1250 | 568 | 641 |

3WIRE data paths are slower (about 123..127 kHz at profile 3); maximum is
primitive-dependent, not a universal 400 kHz. Labels name approximate profiles,
not guaranteed transaction rates. Final scope legs compare all modes/profiles
to this baseline with a stated 15% tolerance and exact finite clock counts.

`tests/test_bitbang.py` compiles the actual implementation with deterministic
MMIO/serial-free mocks: setup, finite/saturating wait count, synthetic releases,
all six waiting abort paths and recovery, every profile/width/drive combination,
accepted source identity and pre-W setup prerequisites. It is not a PIC24
cycle simulator. The early experimental NULL-default/deadline assertions were
not valid requirements of the accepted API and are not counted as defects.

The durable `core_battery.py --bitbang` runs CH2 profile/pre-W/count checks and
held-clock I2C/raw regressions, then the normal core/CDC gates are rerun on
standalone and combined. The temporary owner-installed AUX-to-CLK bridge is
used only as a LOW clamp and always released to INPUT; never driven HIGH.
Healthy STOP follows each timeout to verify recovery. There is no firmware
retry, dummy byte, wait extension or other recovery workaround.

XM64 remains `spi-flash-emu`, listening on the existing SPI wiring. No reflash,
rig-console control, SHT3x or real stretching-slave test is claimed. Releasing
synthetic holds within the bound uses source fixtures; physical held-SCL
timeout/recovery and non-waiting raw behavior use the approved bridge. V3 is
build/source/instruction proof, not a physical v3 bench run.

## Grouping and ownership

Both files live in `Firmware/core_bitbang/`. MPLAB/CLI source and exported
header search paths follow. Normal include spelling stays `bitbang.h`, so
existing clients require no include churn. SPI source stays untouched.
`bitbang-group-proof.txt` compares all V4/V3 map summary rows and both bitbang
object instruction/relocation streams across the move, separately from the
accepted contributions and documentation changes.

No WORKLOG ledger or other-family branch is edited/rebased. Final exact tips,
complete battery totals and local evidence filenames are recorded in PR #209
Part 3 and the local acceptance manifest. Stop before `core_messages`.

**Owner must REMOVE the temporary AUX-to-CLK bridge after this gate.**
