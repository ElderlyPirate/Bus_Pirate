# SUMP logic analyzer — extended (enhanced) capture

Part of the SUMP documentation set (`docs/sump/`). This page covers the
enhanced logic analyzer: the faster sampling engines and the trigger
position. The base protocol — entry/exit, channels, the command reference,
sample rates, capture depth and a worked exchange — is in
[capture.md](capture.md).

## Scope

The enhancement is a set of capabilities on top of the same SUMP protocol
and the same command set; there is no separate "extended mode" to enter. A
host unlocks it by requesting a larger sample count or a faster rate. This
page documents what the enhancement adds and how those engines behave; the
protocol facts it rests on are in [capture.md](capture.md).

## What the enhancement adds

Relative to a base SUMP capture (4096 samples, ≤ 1 MHz, trigger as a start
gate), the enhanced logic analyzer adds:

- sample rates up to **16 MHz**, using cycle-counted polling loops;
- capture depths from **2k up to 32k samples**, by packing several samples
  into each 16-bit storage word and dropping channels to fit — the sizes,
  packing and per-version channel sets are in
  [capture.md](capture.md) §Capture depth and channels;
- a **trigger position anywhere in the window** at the timer-paced rates,
  from a rolling pre-trigger ring;
- the `enh` ancillary marker in the metadata, so enhanced client profiles
  match the device.

## Engines

The rate is selected by the host's divider command.  The same five engines as
[capture.md](capture.md) §Sample rates apply; rates above 1 MHz cannot be
kept by the timer-paced sampler, so the capture switches to a cycle-counted
polling loop on the 16 MIPs core:

| Fast engine | Cycles per sample | Pass shape | Jitter |
|---|---|---|---|
| 2 MHz | 8 | 1 sample per pass | exact |
| 4 MHz | 4 | 4 samples per pass | up to 1 cycle per pass |
| 8 MHz | 2 | 4 samples per pass | up to 1 cycle per pass |
| 16 MHz | 1 | one sample per instruction cycle | exact |

The fast loops exchange samples in groups of four, so the sample counts are
always multiples of four. The 16 MHz rate takes one sample per instruction
cycle and leaves no headroom on the 16 MIPs core; the timer-paced rates below
are the reliable ones. The 4 MHz and 8 MHz loops unroll four samples per
pass; all fast loops end with a decrement-and-branch (the 16 MHz loop uses a
repeat).

At the timer-paced rates the sampling runs in small assembly fragments
(16 samples per call) so that one sample fits in a tick; the per-fragment
bookkeeping is amortised over those samples.  This keeps the window
effectively tick-exact.

## Trigger position

The enhancement's contribution is the **rolling pre-trigger ring** at the
timer-paced rates: the sampler keeps buffering while it waits for the
trigger, so the trigger can sit anywhere from the very start to the very end
of the window, rather than only at the start. The exact rules — the meaning
of the delay count, and the fixed start gate at the fast rates — are in
[capture.md](capture.md) §Trigger.

## Host view

The host sees the enhancement through the metadata: the `enh` ancillary
marker, the maximum sample rate (16 000 000), the sample memory and the probe
count. Enhanced SUMP client profiles use the `enh` marker to enable the
larger rate and depth selections. Sample ordering follows the base protocol
(see [capture.md](capture.md) §Client programs).

## Limits & out of scope

- At the fast rates the trigger is fixed at the window start (no pre-trigger).
- Larger captures record fewer channels; a 32k capture keeps a single
  channel.
- The top rate (16 MHz) has no headroom, so the window is short — a
  2048-sample capture at 16 MHz lasts about 0.13 ms.
- Flow control, the SUMP flags byte, trigger values/stages, external clock
  and RLE are not implemented (see [capture.md](capture.md)).

## v3 / v4

| Aspect | v3 | v4 |
|---|---|---|
| fast engines | 2/4 MHz store one byte per sample; 8/16 MHz one word | one 16-bit word per sample |
| fast capture capacity | 4096 bytes at 2/4 MHz; 2048 words at 8/16 MHz | 2048 words |
| capture tiers ≥ 8k | unverified | present |
| trigger position at timer-paced rates | same | same |

The v3 fast-engine behaviour and the v3 8k/16k/32k tiers are documented from
the firmware and are unverified.