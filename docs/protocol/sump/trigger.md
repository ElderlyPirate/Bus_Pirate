# SUMP — trigger mask and position

Part of the SUMP set (`docs/protocol/sump/`).  This page owns the trigger: which
pins can arm it, what counts as a trigger, where the trigger sits in the
captured window, and what happens when it never arrives.  The command bytes are
in [commands.md](commands.md); the depth tiers are in [depth.md](depth.md).

## What the trigger is

The Bus Pirate's trigger is an **edge on any selected probe pin**.  It is not a
level match, not a pattern and not a sequence.  The first change on any pin in
the mask fires the capture; later edges during the post-trigger sampling do not
retrigger it.

Two consequences follow:

- A pin that is already at the wanted level and never changes never triggers,
  even if it is selected.  The signal must produce an edge.
- A floating or noisy pin in the mask produces edges of its own.  Keep unused
  pins out of the mask, or tie them to a defined level.

## Mask bits

The mask is parameter byte 1 of `0xC0`.  Bits with no pin are ignored; a mask
with no wired bit disables the trigger and the capture runs free.

| Bit | v4 signal | v3 signal |
|---|---|---|
| 0 | AUX2 | CS |
| 1 | MOSI | MISO |
| 2 | CLK | CLK |
| 3 | MISO | MOSI |
| 4 | CS | AUX |
| 5 | AUX | — |
| 6 | — (no pin) | — |
| 7 | AUX1 | — |

The same mask byte therefore means different pins on the two boards: `0x10`
watches CS on v4 and AUX on v3.  A saved trigger mask is board-specific.

| Mask value | Effect on v4 | Effect on v3 |
|---|---|---|
| `0x00` | no trigger; free-run | no trigger; free-run |
| `0x01` | AUX2 | CS |
| `0x04` | CLK | CLK |
| `0x10` | CS | AUX |
| `0x18` | MISO and CS | MOSI and AUX |
| `0x40` | no wired bit; free-run | no wired bit; free-run |
| `0x80` | AUX1 | no wired bit; free-run |

At the fast rates the trigger mask still applies, but every channel is recorded
regardless ([depth.md](depth.md#fast-captures)).

## Trigger position

The trigger position is the delay count from `0x81`: the number of samples
captured **after** the trigger.  The rest of the window is before it.

```
oldest                                                    newest
+------------------------+--------------------------------+
|  pre-trigger samples   |  delay samples after the edge  |
+------------------------+--------------------------------+
                         ^ trigger edge
```

| Requested read count | Delay samples | Pre-trigger | Post-trigger |
|---|---|---|---|
| 4096 | 4 | 4092 | 4 |
| 4096 | 1024 | 3072 | 1024 |
| 4096 | 4096 | 0 | 4096 |
| 1024 | 1024 | 0 | 1024 |
| 1024 | 4096 | 0 | 1024 (clamped) |

The delay is encoded as `(raw + 1) × 4`, so the shortest post-trigger part is
4 samples (raw 0) and the longest is the whole window.  A delay larger than the
capture is clamped to the capture size, which puts the trigger at the start.

The default after a reset is a delay equal to the read count, so a trigger set
without a matching `0x81` also sits at the start.

## How the window is filled

At a timer rate the sampler fills the pre-trigger part first and only then arms
the edge detection.  The fill runs in whole bursts; a burst holds 4 ms worth of
samples, or 16 samples if that is more.  Even a zero-length pre-trigger part
costs one burst, so an edge inside that first burst is ignored:

| Rate | Samples in the first burst | Time before the trigger arms |
|---|---|---|
| 1 MHz | 4000 | 4 ms |
| 100 kHz | 400 | 4 ms |
| 10 kHz | 40 | 4 ms |
| 1 kHz | 16 | 16 ms |
| 100 Hz | 16 | 160 ms |
| 5.96 Hz | 16 | 2.7 s |

The trigger arms at the end of the burst that completes the pre-fill, so an
edge inside that burst is lost.  The table is the wait per burst; a non-zero
pre-trigger adds its own fill time.  Start the capture earlier than the whole
pre-fill plus one burst, use a free-run capture and find the event in the
window, or accept the loss.  After the fill, the first accepted edge anchors
the window:

1. Pre-trigger samples are taken at the timer pace.
2. The selected pins are armed for edges.
3. The first edge sets the trigger point.
4. Sampling continues for the delay count.
5. The window ends and is sent newest-first.

The burst rule is derived in [rates.md](rates.md#timer-paced-sampling).

Because the pre-trigger part is a rolling ring, the oldest samples are the ones
at risk if the post-trigger part overshoots a packed word
([depth.md](depth.md#ring-arithmetic)).

## Fast rates

At 2 MHz and above the trigger always marks the **start** of the window.  The
delay count is ignored, and no samples before the trigger are possible: the fast
loops cannot buffer ahead.  The capture waits for the first edge on the mask,
then fills the window at the fixed engine rate.

The trigger wait at a fast rate is checked for a host byte every few thousand
polls.  A byte sent during the wait aborts the capture.  A byte sent during the
short post-trigger window is not polled until the window and the readout
finish; the window is about 1 ms on v4 (2.05 ms for 4096 samples at 2 MHz on
v3), and the readout follows it (about 0.36 s for 4096 bytes at 115200 baud on
v3; [readout.md](readout.md#aborts-and-bytes-during-a-reply)).

## Missing trigger

A capture with an armed trigger waits indefinitely.  The MODE LED stays on and
no samples are sent.

| Situation | Behaviour | Recovery |
|---|---|---|
| the selected pin never changes | the capture waits | send any byte to abort, or `0x00` to reset |
| the mask selects only empty bits | no trigger is armed; the capture runs free | select a wired pin |
| the edge arrives inside the first burst, before the trigger arms | ignored; the next edge is accepted | start the capture more than one burst before the event, use a free-run capture, or accept the loss; see the table above |
| the edge arrives during the post-trigger part | no effect; the window is already anchored | — |

A host that aborts a waiting capture gets no partial data.  The byte that
aborted it is then handled as a command, so `#` both stops the wait and leaves
SUMP.

## Trigger on a channel that is not recorded

The mask is independent of the recorded-channel set.  A capture tier of 8192 or
deeper drops **high** channel bits ([depth.md](depth.md#depth-tiers)), but a
trigger on a dropped channel still fires.  On v4 at 8192, bits 0–3 (AUX2,
MOSI, CLK, MISO) are kept and CS (bit 4) is dropped; a CS trigger still fires,
but CS is not in the reply.  The capture is valid; the host simply cannot see
the channel that armed it.  Choose a depth that records the trigger channel
when its waveform matters.

## Values, levels and stages

Trigger **values** (`0xC1`) and the staged trigger commands are not
implemented.  A client that asks for "high level on CS" gets "any edge on CS"
instead.  The published high/low and staged options have no effect on this
device; only the one-byte mask is acted on.

The v3 build uses the same trigger rules and the same position arithmetic with
its five-bit mask.
