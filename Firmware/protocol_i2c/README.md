# I2C failure handling

Software I2C releases SCL, waits for the wire to rise, then runs its full
high-phase delay. This applies to data, ACK/NACK, START/repeated START and
STOP. The legacy non-waiting bitbang primitives remain available to raw
protocols. See DS39897C section 16 and the I2C/SHT3x timing diagrams: high
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
bounded SCL wait on each edge (SHT3x-DIS section 4.9, Interface Reset).
This recovery is for the Bus Pirate's single-master use, not multi-master
arbitration. STAT accesses are word-wide for old v3 silicon
(DS39881 errata #26). See DS39897C registers 16-1/16-2 for completion and
error bits. Successful reads still defer the ninth bit until the next command.
