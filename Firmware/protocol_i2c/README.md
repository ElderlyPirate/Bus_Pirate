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
