# BASIC family review

Scope: `tool_basic/basic.c/.h`, plus the MODE, TIME and raw ADC call fronts.
This family stacks on the binary/capability family (#218). Shared PWM,
ADC, timer, mode-reset and protocol implementations stay with their owners.

## Regression evidence

`python Firmware/tests/test_basic.py --debug --group all` compiles the actual
production tokenizer and interpreter with deterministic terminal/hardware
stubs for both board defines. The fixture does not emulate PIC24 register
timing, ADC electronics, or XC16's 16-bit native `int` model.

Focused groups cover PWM operand position/separation/tails, logical PULLUP,
DEBUG, board-specific AUXPIN selectors, active FOR/NEXT frame bounds, malformed
operands, atomic editing and the END sentinel, IF branch line boundaries,
DATA exhaustion/RESTORE, INPUT forms, raw token injection, complete prompt
commands, ADC ownership, and corrupted matching line headers. Baseline checks
retain byte order on SEND statements/expressions and RECEIVE, Ctrl-C,
GOSUB/RETURN bounds, TIME units and the MODE reset-call contract.

Additional configuration checks:

```
python Firmware/tests/test_basic.py --program-space 4
python Firmware/tests/test_basic.py --debug --program-space 1025 --group debug
```

The first explicitly rejects an area too small for the four-byte END sentinel
and its terminating NUL. The second verifies that DEBUG dumps exactly the
configured area even when its length is not divisible by 16.

## Manual and instruction-set review

References: DS70000157G (PIC24 instruction set), DS39897C (I/O ports and
peripherals). XC16 2.10, `-O1 -Wall`, both board targets:

- Runtime signed divide remains `repeat #0x11` / `div.sw`: 18 divide cycles
  plus one REPEAT setup cycle on PIC24F (DS70000157G p233). Division by zero
  traps; signed 0x8000/0xFFFF overflow sets OV with an unusable signed result.
  The interpreter explicitly returns zero for either exceptional case.
- Multiplication uses native `mul.ss`; decimal accumulation uses native
  multiply-by-10 and add. AND/OR are native word instructions. No custom
  bit-serial arithmetic or assembly replacement is justified.
- Statement and special-token switches compile to bounded computed jumps,
  not linear keyword chains. Keyword scanning occurs only while entering
  program text; stored programs dispatch one-byte tokens.
- TIME uses fixed shifts of the shared 32-bit microsecond reading; it does
  not reset or reconfigure that timer. Basic subtraction retains the signed
  16-bit storage/wrap convention.
- DS39897C 10.1: PORT reads sample pins; LAT writes update latches. BASIC
  PULLUP now logically inverts the active-low pin reading. GPIO/PWM/protocol
  writes use the existing shared callbacks; BASIC adds no PORT RMW writes.
- ADC uses the published foreground-ownership query before touching the
  converter. The raw terminal display and manual transaction remain #218's
  implementations; no duplicate conversion or discard/retry path is added.
- Live hardware/SFR state is not hoisted or cached. Parser bounds and active
  loop/data state are correctness checks, not candidates for REPEAT loops.

No additional instruction-level optimization is warranted by this review.
Correctness fixes take priority over removing checked parser calls. DEBUG's
flat bounded loop also removes its undeclared index and handles partial rows.

## Policy and hardware gate

The onboard filesystem remains disabled. SAVE/LOAD/FORMAT and a shared
storage API require owner approval; none is enabled by this family. MODE's
supply/VPU/AUX/bit-order reset contract is deliberate and unchanged.

Native checks and V4/V3 builds are not electrical acceptance. Reuse the
existing BASIC battery, wire-slave byte-order fixture, and the established
input-only PC0/CH2 scope/clk-meter method for PWM and AUX selection. Require
explicit wiring confirmation before flashing or live tests. Score and retain
every scope frame, including the first; never silently discard transients.

V3 remains build/native-only until hardware is available. A disabled DEBUG
leg is expected on the normal image; its optional build is checked separately.
