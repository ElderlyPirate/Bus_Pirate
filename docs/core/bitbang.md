# Bitbang (`core_bitbang/`)

**Status:** maintainer-facing.  Covers `bitbang.c/h`: the shared software
GPIO/clock engine used by the raw 2-wire and 3-wire modes, the software I2C
engine, and any mode that needs timed pin edges.

Hardware SPI and hardware I2C do not use this engine.  The engine does not
select board pin functions; callers set up the mode and provide a valid width.
It only drives/releases the four bus pins and times the edges.  v3 pin/timing
claims here are established from source and native tests only (see the README
limits).

## Scope and data model

| Object | Meaning |
|---|---|
| `BITBANG_DELAYS[]` | four settlement/clock delay profiles in microseconds |
| `delay_profile` | pointer to the profile selected by `bitbang_setup()`; starts unset |
| `miso_pin` | input pin for single-bit reads: MISO in 3-pin setups, MOSI in 2-pin setups |
| `bitbang_clk_stretch_timeouts` | saturating count of clock-stretch waits that hit the bound since reset |
| `i2c_error` | sticky status for the waiting-I2C operations |

The pin masks come from the board header (`MOSI`, `CLK`, `MISO`, `CS`) and are
relative to `IOLAT`/`IODIR`/`IOPOR` (`LATD`/`TRISD`/`PORTD` on v4,
`LATB`/`TRISB`/`PORTB` on v3).  On v4 the comments beside the masks say
`B1`..`B4`; the actual port aliases are D1..D4.  Use the port aliases, not the
comments.

## Setup contract

```c
void bitbang_setup(unsigned char pins, const bp_bitbang_speed_t speed);
```

- `pins` is 3 for a three-pin (SPI-like) setup and 2 for a two-pin
  (I2C-like) setup; any other value selects the 2-pin input.
- `speed` selects a profile; indices above `DELAY_PROFILES_MAX_INDEX` (3) are
  clamped to the maximum profile.
- The call is **state-only**: it changes `miso_pin` and `delay_profile` and
  drives no pins.  There is no implicit default profile; calling any
  profile-dependent function before `bitbang_setup()` dereferences NULL.
- Callers own `mode_configuration.numbits` (1..16) before a value transfer.
  The terminal parser validates widths; binary raw entry fixes eight bits.
- Call it again when the pin count or speed changes.

Speed profiles (microseconds; "legacy" enum names are nominal, not guaranteed
frequencies):

| Enum | Settle | Clock | Settle (no HW timer) | Clock (no HW timer) |
|---|---|---|---|---|
| `BITBANG_SPEED_5KHZ` | 20 | 80 | 19 | 80 |
| `BITBANG_SPEED_50KHZ` | 1 | 8 | 1 | 9 |
| `BITBANG_SPEED_100KHZ` | 1 | 3 | 1 | 4 |
| `BITBANG_SPEED_MAXIMUM` | 0 | 0 | 0 | 0 |

The first three profiles produce roughly 6/60/150 kHz raw clock ticks at
FCY = 16 MHz; data setup, sampling, open-drain rise time and protocol/transport
gaps reduce the effective transfer rate.  `MAXIMUM` has no deliberate phase
delay and is limited by the primitive and the CPU.

## Pin semantics

| State | Driven outputs (`high_impedance == OFF`) | Open-drain (`high_impedance == ON`) |
|---|---|---|
| LOW | latch cleared, direction output | latch cleared, direction output |
| HIGH | latch set, direction output | direction input (released to the external pull-up) |

Reads always go through the port, never the latch: `bitbang_read_pin()` sets
the direction to input, inserts three settling Nops, then returns
`IOPOR & mask`.  The three Nops implement the datasheet's direction-to-read
spacing.  State writes use the latch and direction register; no
read-modify-write of the port is performed.

## API groups

| Group | Functions | Contract |
|---|---|---|
| setup | `bitbang_setup` | state only; must precede use |
| value transfer | `bitbang_write_value(value, wait)`, `bitbang_read_value(wait)`, `bitbang_read_with_write(value)` | width from `mode_configuration.numbits`; `bitbang_read_with_write` always samples the fixed MISO pin (3-pin) |
| bit/pin primitives | `bitbang_write_bit`, `bitbang_read_bit(wait)`, `bitbang_read_miso`, `bitbang_set_mosi`, `bitbang_set_clk`, `bitbang_set_cs` | single pin/bit; no width argument |
| clock | `bitbang_advance_clock_ticks(ticks)` | pulses CLK and leaves MOSI alone |
| edge helpers | `bitbang_set_pins_high`, `bitbang_set_pins_low`, `bitbang_set_pins`, `bitbang_read_pin` | bitmask plus delay; `bitbang_read_pin` takes exactly one bit |
| waiting I2C | `bitbang_i2c_start_wait`, `bitbang_i2c_stop_wait`, `bitbang_i2c_write_bit`, `bitbang_i2c_clear_error`, `bitbang_i2c_error` | return `true` on success; sticky error on timeout |
| legacy raw I2C | `bitbang_i2c_start`, `bitbang_i2c_stop` | never wait for SCL; `bitbang_i2c_start` returns **true on error** |
| diagnostics | `bitbang_clock_stretch_timeouts` | saturating count, survives setup/error clears |

The delay-bearing helpers accept delays within the short-delay helper's range
(0..16384 µs); all profile values are far below that.

## Waiting-I2C contract

All calls with `wait_for_clock_release = true` (and the `_wait` / I2C write-bit
signals) require:

- `mode_configuration.high_impedance == ON` (open-drain), and
- external pull-ups on SDA/SCL.

Releasing SCL is intentional; push-pull driving is not supported for these
operations.  On failure both lines are released and `i2c_error` is set.  Clear
the sticky status before starting a new operation, not between signals of one
operation.  Raw/non-waiting primitives neither wait for SCL nor consume the
status.

Flow:

1. `bitbang_i2c_clock_high()` releases CLK, waits for the slave to let it rise,
   then applies the clock delay.
2. A wait that exceeds `BB_CLK_STRETCH_TIMEOUT` (25000 poll iterations after the
   first full sample) increments `bitbang_clk_stretch_timeouts` (saturating at
   `0xFFFF`) and aborts the operation.
3. `bitbang_i2c_abort()` sets `i2c_error` and releases MOSI and CLK (direction
   input).  The engine does not retry, emit dummy clocks, or extend the wait.
4. Subsequent waiting calls return immediately (`false`, `0xFFFF`, or `true`
   for the bit read) until `bitbang_i2c_clear_error()`.

The iteration count is the hard bound.  It is **not** a portable wall-clock
deadline: the loop body, compiler and clock determine the elapsed time.

Signal shapes:

| Operation | Sequence |
|---|---|
| START | release SDA, clock high (wait), require SDA high, SDA low, CLK low, SDA high |
| repeated START | CLK low first, then the START sequence |
| STOP | CLK low, SDA low, clock high (wait), SDA high, require SDA high |
| write bit | drive/release SDA, clock high (wait), CLK low |
| write value | per bit: drive SDA, clock, CLK low; waiting variant uses the stretch check |
| read value / bit | clock high (wait), sample, CLK low |

## Legacy raw signals

`bitbang_i2c_start()` and `bitbang_i2c_stop()` are kept for raw/non-waiting
callers:

- neither waits for a slave to release the clock nor checks `i2c_error`;
- `bitbang_i2c_start()` returns `true` when it finds the bus not idle (SDA or
  CLK low), i.e. the opposite convention from `bitbang_i2c_start_wait()`;
- `bitbang_i2c_stop()` returns nothing.

## Control/data flow

`bitbang_write_value`, `bitbang_read_value`, `bitbang_read_with_write` and
`bitbang_advance_clock_ticks` are written as per-bit/per-tick hot loops: the
open-drain branch is hoisted, the profile delays are loaded once, and the value
pins are driven through latch/direction operations directly.  The delays are
the same profile values as the helper calls; only the call overhead is removed.

`bitbang_read_with_write` writes MOSI and samples MISO on the same clock; it
does not use the setup-selected single-bit input.  `bitbang_write_value` and
`bitbang_read_value` use MOSI as the data line (2-wire role).
`bitbang_read_bit` uses the setup-selected `miso_pin`.

## Failure modes and edge cases

- **No setup before use.**  `delay_profile` is NULL; any delay-bearing call
  faults.  There is no fallback profile.
- **Width out of range.**  The engine trusts `numbits`; a zero width makes
  `1 << (numbits - 1)` invalid.  The terminal parser and binary layer are
  responsible for 1..16.
- **Stuck SCL.**  A waiting operation aborts after the iteration bound, releases
  both lines and sets the sticky error; the timeout counter records it.
- **Push-pull waiting I2C.**  Not supported; the contract requires open-drain
  plus pull-ups.  The engine does not silently switch modes or add a guard.
- **Fault during a multi-byte operation.**  The sticky error makes the following
  waiting calls return immediately; the caller must clear it before a new
  operation.
- **Counter lifetime.**  The timeout counter is a diagnostic: setup and
  `bitbang_i2c_clear_error()` do not clear it; it saturates and survives until
  a device reset or power cycle.  The v4 `#` soft reset does not clear it.

## Invariants

- Pin state is written to the latch and direction registers; reads sample the
  port.  Never introduce a PORT read-modify-write.
- A HIGH on an open-drain pin means "released", never "driven high".
- Waiting operations return `true` on success; legacy raw `bitbang_i2c_start`
  returns `true` on error.  Do not mix the conventions.
- `i2c_error` is sticky and operation-scoped, not signal-scoped.
- `bitbang_clock_stretch_timeouts` is monotonic (saturating), not per-operation.

## Native test coverage

`tests/test_bitbang.py` compiles the production implementation with
deterministic MMIO/serial-free mocks.  It covers setup/pin selection and profile
clamping, the finite/saturating stretch wait, synthetic releases, every waiting
abort path and recovery, the profiled/raw/drive/width matrix, and the pre-setup
prerequisites.  It is a standalone harness (run from the repository root:
`python Firmware/tests/test_bitbang.py`); `tests/test_native.py` does not run
it.  It is not a cycle-accurate PIC24 simulator.

## Related docs

- [system.md](system.md) — dispatcher keys that reach these primitives.
- `docs/protocol/wire/2wire/README.md`, `docs/protocol/wire/3wire/README.md` —
  the raw modes that use the engine.
- `docs/protocol/i2c/README.md` — software vs hardware I2C.
- `docs/SPEC.md` — speed/profile capability rows.
