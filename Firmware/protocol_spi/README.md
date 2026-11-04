# SPI implementation notes

The twelve prescaler entries follow DS39897C register 15-2. Menu numbers are
1-12; binary `0x6x` uses indices 0-11 in the same order. The table is not
sorted by frequency. Measure the actual clock and select electrical settings
appropriate to the target; successful replies from a slow fixture do not
qualify the faster settings.

## Transfer faults and framing

Byte and bulk peripheral polling is bounded by `SPI_POLL_TIMEOUT`. Bulk
budgets reset only on progress, and a module fault remains latched for the
current bulk request. Unread bytes are filled with `0xFF`; write payload is
still consumed, so it cannot become subsequent commands. This retains the
legacy reply framing, not a new error-status channel. It does not impose a
deadline on host input when no SPI transfer is pending. V3 retains the
SPITBF errata workaround. See DS39897C 15.1/register 15-1 for FIFO semantics.

Binary `0x04` controls CS around a combined transfer; `0x05` leaves CS to
the caller's `0x02`/`0x03` commands. A zero-write/zero-read request succeeds
without changing CS, as required by avrdude's capability probe. SPI extended
AVR commands use `0x06` (noop/version/bulk read). `0x18` is instead a nine-byte
full-duplex transfer. AVR bulk addresses are words and lengths are bytes;
valid reads include the final 16-bit word but must not wrap its address.

## Sniffer limits

Both sampling interfaces use slave select when CKE=1, as required by the
device setup sequence. Sniff-all routes SS to the null low PPS input rather
than gating clocks with physical CS (DS39897C 10.4.5); the CN marker capture
still observes physical CS. Filtered capture uses the actual CS input.

Captured CS transitions have a bounded 32-entry ISR ring. A full ring must
not overwrite unread entries. CS-ring, SPI FIFO and output-buffer overflow
share capture-abort handling: terminal mode reports inability to keep up and
restarts; binary mode emits an unescaped `0x00` failure byte, disables capture
and stays passive until the host stops it. The host's stopping byte remains
available to the subsequent command loop. Wait until the external master is
quiet before stopping: normal exit restores master pin configuration.

CN is change notification, not an unlimited hardware edge FIFO. Interrupt
latency can merge transitions, and finite SPI/output FIFOs limit throughput.
An overflow-free capture is not a guarantee of arbitrary-rate losslessness.
