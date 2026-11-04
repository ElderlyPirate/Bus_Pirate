# I2C failure handling

Software I2C releases SCL, waits for the wire to rise, then runs its full
high-phase delay. This applies to data, ACK/NACK, START/repeated START and
STOP. The legacy non-waiting bitbang primitives remain available to raw
protocols. See DS39897C section 16 and the I2C timing diagrams: high
time is measured on the wire, not from the master's direction-register write.

A software wait is bounded to 25000 polling iterations (approximately 25 ms,
not an exact wall-clock deadline). On a timeout both bus pins are released.
The terminal prints `I2C bus error` and abandons the command line. Binary
START, STOP, ACK and NACK return `0x00` on failure instead of `0x01`.

Binary byte reads retain their single-byte reply for compatibility. Command
`0x0A` reports the last bus operation's status: `0x01` means no bus fault,
`0x00` means a timeout or bus fault. Querying status does not clear it;
starting the next bus operation does. In a bulk write the fault remains
latched for the whole bulk command, whose remaining payload is consumed and
NACKed without more bus activity. A normal slave NACK is not a bus fault.
Combined write/read reports failure rather than returning invalid read data.

Hardware I2C uses the same bounded polling budget for every master sequence
(SEN/RSEN, PEN, ACKEN, TRSTAT/TBF and RCEN/RBF), and for initially idle lines.
BCL, IWCOL and I2COV abort the operation. Recovery disables the selected
module, releases its pins and invalidates pending ACK state; a new START
reinitializes that same bus. If SDA is still held mid-read, that explicit
new START first attempts nine open-drain recovery clocks and STOP, with a
bounded SCL wait on each edge (the nine-clock serial-interface reset sequence).
This recovery is for the Bus Pirate's single-master use, not multi-master
arbitration. STAT accesses are word-wide for old v3 silicon
(DS39881 errata #26). See DS39897C registers 16-1/16-2 for completion and
error bits. Successful reads still defer the ninth bit until the next command.

## Bus electrical requirements / pull-up guidance

BPv4's switched on-board pull-ups are 10k on CLK/SCL, MISO and CS (RN1),
and 2k on MOSI/SDA (R10). External resistors are in parallel with these when
pull-ups are enabled. Select/feed VPU before enabling the pull-ups; use only
3.3 V pull-up sources for 3.3 V targets, including any other devices' pull-ups.

Keep leads short and check the target's rise-time, high-time and maximum
clock-frequency limits. Many 3.3 V targets require <=300 ns rises even when
operated at a slower clock. A 10k SCL pull-up can give 1.4-1.6 us rises with
approximately 230 pF bus loading. The hardware engine's clock synchronization
then makes the actual rate substantially lower than the selected nominal rate.

An external 2.2k SCL pull-up to 3.3 V has reduced that rise to approximately
0.37 us and enabled CRC-valid transfers at all three hardware settings; the
nominal 1 MHz setting measured about 766 kHz (about 780 kHz with reduced
loading). This is functional verification, not compliance with a 300 ns rise
limit. Approximately 1.5-2.2k external on SCL is a useful starting range for
fast modes, but select the value from the actual capacitance and sink-current
rating, then measure again. SDA's existing 2k is often sufficient; verify its
rise separately rather than automatically fitting the same resistor on both
lines. A stronger pull-up increases the current a slave must sink for ACK/data.

Measure at the target using adequate scope voltage range and sampling rate.
Check both 30%-70% rises, SCL high time above the target's input-high threshold,
low levels and actual clock frequency. Successful ACKs/CRCs alone do not prove
electrical timing compliance; firmware cannot compensate for inadequate bus
levels or excessive loading.
