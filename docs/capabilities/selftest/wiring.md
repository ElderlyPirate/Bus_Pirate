# Self-test — wiring and setup

Part of the self-test documentation set (`docs/capabilities/selftest/`).  The
physical setup for a run: the jumpers, what they connect, the pull-up and
supply requirements, and the state the run leaves behind.  The prompt text and
the run are in [terminal.md](terminal.md); the effect of a missing jumper on
the count is in [checks.md](checks.md) and [edge-cases.md](edge-cases.md).

## The jumpers

The run measures board rails through header pins, so it needs a wire between
pins.  The terminal full test asks for them before it starts; the short
binary pass needs none.  The binary full pass assumes they are already
fitted.

| Board | Jumper | Connects | Feeds | Checks that depend on it |
|---|---|---|---|---|
| v4 | `ADC` → `+3.3V` | the `ADC` pin to the `+3.3V` pin | the probe input | `ADC` |
| v3 | `Vpu` → `+5V` | the `Vpu` pin to the `+5V` pin | the pull-up reference | `VPU`, the four `Bus Hi-Z 1` checks |
| v3 | `ADC` → `+3.3V` | the `ADC` pin to the `+3.3V` pin | the probe input | `ADC` |

- v4 needs one jumper; v3 needs two.
- The jumper is a plain wire between two header pins, not a resistor.
- The terminal prompt lists the jumpers it expects; match it to the board in
  [terminal.md](terminal.md#setup).
- A missing jumper fails only the checks fed from it; the rest of the run
  still executes.  On v3 the `Vpu` jumper is shared by five checks, so one
  missing wire can produce five failures.
- The prompt writes `Vpu` and `+5V`; the check labels are `VPU` and `5V`.
  The mapping is in [checks.md](checks.md#names).

The v3 jumper set is **not verified on v3**.

## Pins used by the run

| Pin | Role | During the run |
|---|---|---|
| `ADC` | probe input on both boards | jumper to `+3.3V` fitted |
| `Vpu` | pull-up reference on v3; v4 uses internal sources | v3: jumper to `+5V` fitted; v4: no jumper, no external voltage |
| `+5V`, `+3.3V` | the jumper targets | keep the pins free for the jumper |
| `MOSI`, `CLK`, `MISO`, `CS` | the four I/O pins, driven by the three pin groups | leave unconnected |
| `AUX` | driven and read by the `AUX` check | leave unconnected |
| `AUX1`, `AUX2` | no check covers them (v4) | leave unconnected |

The `AUX` check uses the `AUX` pin itself, whatever AUX assignment was made
before the run.

## Pull-ups and supply

- **No external pull-up resistor is needed.**  The self-test enables the
  board's own pull-up network itself and checks the control line
  (`PULLUP H`, `PULLUP L`) and the pulled-up inputs (`Bus Hi-Z 1`).  Do not
  add a resistor to the header.
- On v3 the pull-up network is fed from the `Vpu` pin, so the `Vpu`→`+5V`
  jumper is what gives it a reference.  On v4 the test selects the on-board
  3.3V source for the pulled-up input group.
- **No external supply is required.**  Power the Bus Pirate as usual (USB is
  the normal source).  The test enables the board's regulator itself (the
  `VREG` check), measures the 5V and 3.3V rails, and switches them off at
  the end.
- The `Vusb` check (v4) reads the 5V arriving on the USB connection, not the
  board's 5V rail.  The board must be powered that way for it to pass.

## Before and after the run

Before:

1. Disconnect every device from the header.  The run drives all four I/O
   pins and switches the supply and pull-up rails, so a connected target
   sees those changes.
2. Remove any other jumper, probe or external pull-up from the I/O pins.  On
   v3 remove any external voltage from `Vpu` before fitting the
   `Vpu`→`+5V` jumper; on v4 leave `Vpu` unconnected.
3. Fit the jumpers in the table above.
4. Leave the Bus Pirate powered and connected to the terminal.

After the run, remove the self-test jumpers before using the probe or
reconnecting a target: a fitted `ADC`→`+3.3V` jumper is a plain wire to the
3.3 V rail, and a connected source back-drives it.

After a completed run the firmware restores the idle state:

| Item | State after the run |
|---|---|
| four I/O pins | inputs |
| `AUX` | input, default `AUX` assignment restored |
| `AUX1` / `AUX2` sources (v4) | detached |
| supply rails | off |
| pull-ups | off |
| bus mode | HiZ |

The binary run leaves the same board state but keeps the host in the bitbang
layer, not the terminal; see [binary.md](binary.md).

## Related docs

- [terminal.md](terminal.md) — the setup prompt and the run.
- [checks.md](checks.md) — every check and the jumper it needs.
- [binary.md](binary.md) — the `0x10` / `0x11` passes.
- [edge-cases.md](edge-cases.md) — missing jumpers and other failures.
- [capabilities/auxiliary/README.md](../auxiliary/README.md) — `AUX` pin and
  target naming.
