# SUMP — depth, channels and capacity

Part of the SUMP set (`docs/protocol/sump/`).  This page owns the capture depth:
how the read count becomes a stored window, which channels survive, how the
ring arithmetic works, and how much the fast engines can hold.  The reply
framing is in [readout.md](readout.md); the rate is in [rates.md](rates.md).

## Sample memory

Both boards in this image allocate **4096 bytes** for the sample buffer.  The
metadata reports that value as key `0x21`, so a client sizes its depth control
from 4096 on both boards.

The same memory is read as 2048 16-bit storage words when samples are packed.
The word view is internal: the host always receives one byte per sample
([readout.md](readout.md#reply-shape)).

## Channel bits

Each sample byte carries the probe pins in fixed bit positions.  The map is
different on the two boards, so the same mask byte selects a different pin.

v4 (8 probe slots advertised, 7 wired):

| Bit | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| Signal | AUX2 | MOSI | CLK | MISO | CS | AUX | — | AUX1 |

v3 (5 probes):

| Bit | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| Signal | CS | MISO | CLK | MOSI | AUX | — | — | — |

Bit 6 on v4 has no header pin behind it; the firmware still advertises 8 probe
slots, and the empty bit is filled with the alternating pattern.  Bits 5–7 on
v3 are likewise empty.

A bit with no pin can never carry data and cannot arm a trigger.  A mask that
selects only empty bits disables the trigger rather than waiting forever.

## Depth tiers

The read count from `0x81` selects the storage format and the capture size.
The tier is chosen from the **requested** count, before any capture happens:

| Requested read count | Captured samples | Samples per 16-bit word | Channel bits kept |
|---|---|---|---|
| below 4096 | as requested | 1 per byte | all wired |
| 4096 to 8188 | 4096 | 1 per byte | all wired |
| 8192 to 16 380 | 8192 | 4 | 0–3 |
| 16 384 to 32 764 | 16 384 | 8 | 0–1 |
| 32 768 or more | 32 768 | 16 | 0 |

"Bits kept" are the low channel bits of the table above.  The effect per board:

| Captured | v4 channels kept | v3 channels kept |
|---|---|---|
| 4096 | AUX2, MOSI, CLK, MISO, CS, AUX, AUX1 | CS, MISO, CLK, MOSI, AUX |
| 8192 | AUX2, MOSI, CLK, MISO | CS, MISO, CLK, MOSI |
| 16 384 | AUX2, MOSI | CS, MISO |
| 32 768 | AUX2 | CS |

Larger captures trade channels for depth.  The dropped bits are the **high**
channel bits, not the low ones; they are not recorded, and every sample byte
fills them with the alternating pattern so a client can tell them from real
data.

The tier is independent of the rate at timer speeds: 8192 samples at 100 kHz
keeps the same four channels as 8192 samples at 1 MHz.  At the fast rates the
tier is overridden; see [fast captures](#fast-captures).

## Why the packing exists

A byte-per-sample window is limited to 4096 samples by the buffer.  To capture
more, several samples share each 16-bit storage word: four samples for 8192,
eight for 16 384 and sixteen for 32 768.  Fewer channel bits are stored per
sample, which is why the depth tiers drop channels.

The host is not involved in the packing.  The device packs the samples while
capturing and unpacks them into one byte per sample while sending, so the reply
format is identical at every depth.  Only the reply **length** and the set of
recorded channels change.

## Ring arithmetic

The timer-paced capture writes into a rolling ring, so the window can hold
samples from before the trigger:

| Format | Ring slots | Samples in the ring |
|---|---|---|
| byte tier | 4096 one-byte slots | 4096 |
| 8192 tier | 2048 words × 4 | 8192 |
| 16 384 tier | 2048 words × 8 | 16 384 |
| 32 768 tier | 2048 words × 16 | 32 768 |

A full-tier window is never larger than the ring: the tier sizes the window to
the ring exactly.  A request below the tier size fills only that many slots; a
request inside a tier fills the tier size and pads the rest.  The readout walks
backwards from the newest sample, which is why the stream is newest-first.

Two consequences of the arithmetic:

- The pre-trigger and post-trigger parts together are the window; the delay
  count decides the split, not the total.
- A packed capture can overshoot the window end by part of one storage word
  (up to 3, 7 or 15 samples, depending on the tier).  Those samples overwrite
  the oldest samples of a full window, which then read back with their data bits
  cleared and only the alternating pattern in the unrecorded bits.  A byte-tier
  capture does not overshoot.

The oldest part of a full window is therefore the only part at risk, and only
in the packed tiers.  A host cannot distinguish an overwritten sample from a
real all-low sample on the recorded channels.

## Fast captures

At 2 MHz and above the capture is a linear buffer, not a rolling ring, and the
depth tier does not apply.  Every wired channel is recorded.

| Board | 2 MHz | 4 MHz | 8 MHz | 16 MHz |
|---|---|---|---|---|
| v4 | 2048 samples | 2048 | 2048 | 2048 |
| v3 | 4096 samples | 4096 | 2048 | 2048 |

On v4, 2048 samples of 16-bit storage is the whole buffer at every fast rate.
On v3 the 2/4 MHz loops store one byte per sample, so the same 4096 bytes hold
4096 samples; the 8/16 MHz loops store words and hold 2048.  The v3 fast
figures are **not verified on v3 hardware**.

A fast capture always keeps every channel, even if the read count selected a
packed tier and a reduced channel mask.  The reply is still as long as the read
count; the difference is padding.

## The tier cliff

Because the tier is chosen from the requested count, a count just below a
boundary keeps the smaller capture and pads the rest of the reply.  The read
count is encoded as `(raw + 1) × 4`, so the counts below are multiples of four;
a client UI may round a typed count, so check the count it actually sends:

| Read count | Captured | Padding bytes | v4 channels kept |
|---|---|---|---|
| 4092 | 4092 | 0 | all |
| 4096 | 4096 | 0 | all |
| 8188 | 4096 | 4092 | all |
| 8192 | 8192 | 0 | 4 |
| 16 380 | 8192 | 8188 | 4 |
| 16 384 | 16 384 | 0 | 2 |
| 32 764 | 16 384 | 16 380 | 2 |
| 32 768 | 32 768 | 0 | 1 |
| 65 532 | 32 768 | 32 764 | 1 |
| 262 144 | 32 768 | 229 376 | 1 |

The practical rule: request exactly a tier boundary when the deeper tier is
wanted.  8188 pads 4092 bytes and still records every channel; 8192 records
four channels for almost the same reply length.

The largest expressible read count is 262 144 samples, but the largest capture
is 32 768.  A larger reply is mostly padding.

## v3 differences

| Item | v3 | v4 |
|---|---|---|
| sample memory | 4096 bytes | 4096 bytes |
| channel map | CS, MISO, CLK, MOSI, AUX | AUX2, MOSI, CLK, MISO, CS, AUX, —, AUX1 |
| timer tiers | same five tiers | same five tiers |
| fast 2/4 MHz | 4096 samples | 2048 samples |
| fast 8/16 MHz | 2048 samples | 2048 samples |
| 4 MHz jitter | up to two cycles | one cycle |
| hardware | present in the build, not verified | present |

The v3 command set is the same.
