# SUMP — sample rates and engines

Part of the SUMP set (`docs/protocol/sump/`).  This page covers the divider, the
five sampling engines, quantisation and jitter, capture duration, and the timer
and counter behaviour behind a long capture.  Depth and channels are in
[depth.md](depth.md); the trigger position is in [trigger.md](trigger.md).

## The divider

The host sets the rate with a 24-bit divider `x` (`0x80`, parameters 1–3).
The published SUMP formula labels the request as `100 MHz ÷ (x + 1)`, but the
Bus Pirate's instruction clock is 16 MHz, so the divider is quantised to a
timer period or to one of four fixed engines:

```
timer period (ticks) = floor(4 × x ÷ 25) + 1
timer rate           = 16 MHz ÷ that period
```

The divider is the authoritative input.  A client computes `x` from the rate it
wants; the published inverse is `x = round(100 000 000 ÷ rate) − 1`.  The
device then selects the timer period or engine band that `x` falls in, which is
not always the nearest rate.

## The five engines

| Divider `x` | Nominal request | Engine | Actual sample rate |
|---|---|---|---|
| 0–6 | 100 – 14.3 MHz | 16 MHz, cycle-counted | 16 MHz |
| 7–18 | 12.5 – 5.26 MHz | 8 MHz, cycle-counted | 8 MHz |
| 19–43 | 5.0 – 2.27 MHz | 4 MHz, cycle-counted | 4 MHz |
| 44–93 | 2.22 – 1.064 MHz | 2 MHz, cycle-counted | 2 MHz |
| 94 – 16 777 215 | 1.053 MHz – 5.96 Hz | timer-paced | 16 MHz ÷ (floor(4x/25) + 1) |

The boundary sits just above 1 MHz: divider 93 (nominal 1.064 MHz) selects the
2 MHz engine, while divider 94 (nominal 1.053 MHz) is timer-paced at 1 MHz.
There is no way to request a rate between those two points.

## Quantisation

Common requests and the rate they achieve:

| Request | Divider `x` | Engine | Actual | Timer ticks |
|---|---|---|---|---|
| 16 MHz | 5 | 16 MHz | 16 MHz | — |
| 8.33 MHz (nominal) | 11 | 8 MHz | 8 MHz | — |
| 4 MHz | 24 | 4 MHz | 4 MHz | — |
| 2 MHz | 49 | 2 MHz | 2 MHz | — |
| 1 MHz | 99 | timer | 1 MHz | 16 |
| 500 kHz | 199 | timer | 500 kHz | 32 |
| 250 kHz | 399 | timer | 250 kHz | 64 |
| 200 kHz | 499 | timer | 200 kHz | 80 |
| 100 kHz | 999 | timer | 100 kHz | 160 |
| 50 kHz | 1999 | timer | 50 kHz | 320 |
| 20 kHz | 4999 | timer | 20 kHz | 800 |
| 10 kHz | 9999 | timer | 10 kHz | 1600 |
| default period | 10 000 | timer | 9.994 kHz | 1601 |
| 1 kHz | 99 999 | timer | 1 kHz | 16 000 |

The 8.33 MHz row is a nominal request: divider 11 selects the 8 MHz engine.

A request that is not a round rate is quantised: for example 3 MHz falls in the
4 MHz engine, 1.5 MHz falls in the 2 MHz engine, and 300 kHz becomes
296.3 kHz at divider 332 (period 54 ticks).  The device does not interpolate:
it selects one engine band and runs it at its own pace.

## Jitter

| Engine | Sample spacing | Spacing accuracy |
|---|---|---|
| 16 MHz | one instruction cycle | exact |
| 8 MHz | 2 cycles average | 1–3 cycles per sample; exact long-term |
| 4 MHz, v4 | 4 cycles average | 3–5 cycles per sample; exact long-term |
| 4 MHz, v3 | 4 cycles average | 3, 4, 3 and 6 cycles within a pass; exact long-term, **not verified on v3** |
| 2 MHz | eight cycles | exact per sample |
| timer | period + 1 ticks | exact inside a burst |

The 8 MHz and 4 MHz loops unroll four samples per pass, so the spacing varies
within the pass while the average is exact.  Use 1 MHz, 2 MHz or 16 MHz when
every interval must be the same length; 4 and 8 MHz have one cycle of jitter
around the mean (up to two cycles on the v3 4 MHz loop).

## Capture duration

The capture time is samples ÷ rate; the readout follows afterwards.

| Capture | Rate | Duration |
|---|---|---|
| 2048 samples | 16 MHz | 128 µs |
| 2048 samples | 2 MHz | 1.02 ms |
| 4096 samples | 2 MHz (v3) | 2.05 ms |
| 4096 samples | 1 MHz | 4.1 ms |
| 32 768 samples | 1 MHz | 32.8 ms |
| 4096 samples | 10 kHz | 410 ms |
| 32 768 samples | 10 kHz | 3.3 s |

The fast buffer is the reason a fast capture is short: it holds 2048 samples on
v4 (4096 at 2/4 MHz on v3).  A long window always means a timer rate.

## The default rate

After entry, `0x00` or any completed capture, **no divider is set**.  The
default timer period is 1601 ticks, which gives about 9.994 kHz.  The 1601 is
a timer period, not a divider value; sending `0x80` with `x=1601` would select
a different, much faster rate (about 62 kHz).  A host that sends `0x01` without
setting a divider gets a 4096-sample capture at about 9.994 kHz, not at 1 MHz.
Always send `0x80` before a capture whose rate matters; `x=10 000` selects the
default period explicitly.

## Timer-paced sampling

Below the fast boundary the hardware timer paces the samples:

- the period is the programmed timer value plus one tick of the 16 MHz
  instruction clock, held as a 32-bit value; slow rates are not clipped at
  65 536 ticks;
- sampling runs in bursts of about 4 ms worth of samples, with a minimum of 16
  samples per burst.  Inside a burst each sample waits for its own tick, so the
  spacing is exact; between bursts the bookkeeping can add a short gap, most
  visible at 1 MHz.  The window is effectively tick-exact;
- the timer starts with the capture and stops when it ends.  It keeps running
  while a trigger is awaited;
- a host byte aborts the capture between bursts, not inside one.

A trigger wait can last far longer than the capture.  The sample counter is
32-bit and is rebased by whole ring revolutions before it can wrap, so a wait
of many minutes does not corrupt the pre-trigger history or the readout.
[trigger.md](trigger.md#missing-trigger) covers the case where the trigger
never arrives.

### Counter rollover

| Counter | Width | What it means |
|---|---|---|
| sample counter | 32 bits | a trigger wait longer than one ring revolution keeps the ring contents and the slot phase; the counter is rebased at a whole number of revolutions before wrapping |
| timer period | 32 bits | the slow end of the divider is not clipped at 65 536 ticks |
| ring slot | per format | wraps at the ring size; the readout walks backwards from the newest sample |

The rollover handling is invisible to the host: the reply length, the
newest-first order and the pre-trigger split are unchanged by a long wait.

## Divider limits

| Limit | Value |
|---|---|
| divider field | 24 bits, 0–16 777 215 |
| fastest rate | 16 MHz (divider 5; dividers 0–6 all select the 16 MHz engine) |
| slowest timer rate | about 5.96 Hz (divider 16 777 215) |
| timer period step | one instruction tick |

A client that asks for a rate slower than about 6 Hz cannot express it: only the
low 24 bits of the divider fit the command, so the request wraps and the capture
runs at a different rate.  Clamp the rate at the slow end before computing the
divider.

## Choosing a rate

| Need | Use |
|---|---|
| a trigger anywhere in the window | a timer rate (1 MHz or slower) |
| a long window | a timer rate, with a count that fits the depth tier |
| maximum rate | 16 MHz; the window is at most 128 µs on v4 |
| exact spacing with a long window | 1 MHz |
| every channel with a moderate window | 4096 samples or fewer, at a timer rate |
| the deepest capture | 32 768 samples at a timer rate; one channel is kept |

On v3 the engines and divider mapping are the same and the 4 MHz loop has the
larger jitter noted above.  The v3 fast capacities differ; see
[depth.md](depth.md#fast-captures).
