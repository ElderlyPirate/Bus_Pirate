I2C Mode
===================
I2C is a common 2-wire bus for low speed interfaces.

  - **Bus:** I2C
  - **Connections:** two pins (SDA/SCL) and ground
  - **Output types:** open drain/open collector
  - **Pull-up resistors:** pull-ups always required (2K - 10K ohms)
  - **Maximum voltage:** 5.5volts (5 volt safe)
  - **Speed:** I2C has three speed options: ~50kHz, ~100kHz, and ~400kHz.

*v3 I2C implementation does not currently support clock stretching.

Bus Requirements
---------------------

### Pull-up resistors

I2C is an open-collector bus, it requires pull-up resistors to hold the clock and data lines high and create the data '1'. I2C parts don't output high, they only pull low, without pull-up resistors there can never be a '1'. This will cause common errors such as the I2C address scanner reporting a response at every address.


Starting I2C Mode
---------------------

    HiZ>m    <<<open the mode menu
    1 . HiZ
    2 . …
    4 . I2C
    …
    (1) >4    <<<choose I2C mode
    Set speed:
    1 . 50KHz
    2 . 100KHz
    3 . 400kHz
    (1) >1    <<<choose I2C speed
    I2C READY
    I2C>


Command Syntax
-------------------

  * A/a/@
	  * Toggle auxiliary pin.
	  * Capital “A” sets AUX high
	  * Small “a” sets to ground.
	  * @ sets aux to input (high impedance mode) and reads the pin value.
  * D/d
	  * Measure voltage on the ADC pin (v1+ hardware only).
  * W/w
	  * Capital ‘W’ enables the on-board power supplies.
	  * Small ‘w’ disables them. (v1+ hardware only).
  * { or [
	  * Issue I2C start condition.
  * ] or }
	  * Issue I2C stop condition.
  * r
	  * Read one byte, send ACK. (r:1…255 for bulk reads)
  * 0b
	  * Write this binary value, check ACK.
	  * Format is 0b00000000 for a byte, but partial bytes are also fine: 0b1001.
  * 0x
	  * Write this HEX value, check ACK.
	  * Format is 0×01. Partial bytes are fine: 0xA. A-F can be lower-case or capital letters.
  * 0-255
	  * Write this decimal value, check ACK.
	  * Any number not preceded by 0x or 0b is interpreted as a decimal value.
  * ,
	  * Value delimiter.
	  * Use a comma or space to separate numbers.
	  * Any combination is fine, no delimiter is required between non-number values: {0xa6,0, 0 16 5 0b111 0xaF}.
  * &
	  * Delay 1uS. (&:1…255 for multiple delays)
  * (#) 
	  * Run macro, (0) for macro list 

Macros
------------------

| (#) | Description |
|:---:| ----------- |
| 0 | Macro menu |
| 1 | 7bit address search. Find all connected devices by brute force. |
| 2 | I2C snooper. Listen for all i2c traffic |
  
### I2C address search scanner macro

You can find the I2C address for most I2C-compatible chips in the datasheet. But what if you're working with an unknown chip, a dated chip with no datasheet, or you're just too lazy to look it up?

The Bus Pirate has a built-in address scanner that checks every possible I2C address for a response. This brute force method is a fast and easy way to see if any chips are responding, and to uncover undocumented access addresses.

I2C chips respond to a 7bit address, so up to 128 devices can share the same two communication wires. An additional bit of the address determines if the operation is a write to the chip (0), or a read from the chip (1).

We connected the Bus Pirate to the 3EEPROM explorer board. The 7bit base address for the 24LC/AA I2C EEPROM is 101 0000 (0x50 in HEX). It answers at the write address 1010 0000 (0xA0) and the read address 1010 0001 (0xA1).
  
    I2C>(1)   <<< I2C search macro
    Searching 7bit I2C address space.
    Found devices at:
    0xA0(0x50 W) 0xA1(0x50 R)
    I2C>

Macro 1 in the I2C library runs the address scanner. The scanner displays the raw addresses the chip acknowledged (0xA0, 0xA1), and the 7bit address equivalent (0x50) with write or read bit indicators (W/R). Datasheets usually list the 7bit address, but the 8bit value is more recognizable on a logic analyzer, snooper, debugger, etc.

    I2C> (1)
    Searching I2C address space. Found devices at:
    Warning: *Short or no pull-up
    I2C>

The scanner will find a chip at every address if there's no pull-up resistors on the I2C bus. This is a really common error. Since firmware v5.2 the scanner now checks for pull-ups, and exits with an error if the bus isn't pulled up.
Scanner details

Details about the address scanner macro are at the end of this post and around here in the source.

    For I2C write addresses: the BP sends a start,
    the write address, looks for an ACK, then sends a stop.
    For I2C read addresses: the BP sends a start, the read address,
    looks for an ACK. If there is an ACK, it reads a byte and NACKs
    it. Finally it sends a stop. 

When the I2C chip responds to the read address, it outputs data and will miss a stop condition sent immediately after the read address (bus contention). If the I2C chip misses the stop condition, the address scanner will see ghost addresses until the read ends randomly. By reading a byte after any read address that ACKs, we have a chance to NACK the read and properly end the I2C transaction.

### I2C Bus Sniffer macro

The I2C sniffer is implemented in software and seems to work up to 100kHz (firmware v5.3+).
It’s not a substitute for a proper logic analyzer, but additional improvements are probably possible.

    [/] – Start/stop bit
    +/- – ACK/NACK 

I2C start and stop bits are represented by the normal Bus Pirate syntax.

    I2C> (2)
    Sniffer
    Any key to exit
    [0x40-][0x40-0x40-0x30-0x56-0x77-]
    I2C>

Sniffed data values are always HEX formatted in user mode. Press any key to exit the sniffer.

Notes The sniffer uses a 4096byte output ring buffer. Sniffer output goes into the ring buffer and gets pushed to the PC when the UART is free. This should eliminate problems with dropped bytes, regardless of UART speed or display mode. A long enough stream of data will eventually overtake the buffer, after which bytes are dropped silently (will be updated after v5.3).

Any commands entered after the sniffer macro will be lost.

Pins that are normally output become inputs in sniffer node. MOSI and CLOCK are inputs in I2C sniffer mode.

The I2C sniffer was updated in firmware v5.3, and the maximum speed increased from around 70kHz to around 100kHz.

### ACK/NACK management

These examples read and write from the RAM of a DS1307 RTC chip.

    I2C> [0xd1 rrrr]
    I2C START CONDITION
    WRITE: 0xD1 GOT ACK: YES<<<read address
    READ: 0×07 ACK <<<sent ACK[
    READ: 0x06 ACK
    READ: 0x05 ACK
    READ: 0x04 NACK <<<last read before STOP, sent NACK
    I2C STOP CONDITION
    I2C>

I2C read operations must be ACKed or NACKed by the host (the Bus Pirate). The Bus Pirate automates this, but you should know a few rules about how it works.

The I2C library doesn't ACK/NACK a read operation until the following command. If the next command is a STOP (or START) the Bus Pirate sends a NACK bit. On all other commands it sends an ACK bit. The terminal output displays the (N)ACK status.

    I2C> [0xd1 r:5]
    I2C START CONDITION
    WRITE: 0xD1 GOT ACK: YES
    BULK READ 0×05 BYTES:
    0×07 ACK 0×06 ACK 0×05 ACK 0×04 ACK 0×03 NACK
    I2C STOP CONDITION
    I2C>

Nothing changes for write commands because the slave ACKs to the Bus Pirate during writes. Here’s an example using the bulk read command (r:5).

    I2C>[0xd1 r <<<setup and read one byte
    I2C START CONDITION
    WRITE: 0xD1 GOT ACK: YES
    READ: 0x07 *(N)ACK PENDING <<<no ACK sent yet
    I2C>r<<<read another byte
    ACK <<<ACK for previous byte
    READ: 0x06 *(N)ACK PENDING <<<no ACK yet
    I2C>] <<<STOP command
    NACK <<<next command is STOP, so NACK
    I2C STOP CONDITION
    I2C>

A consequence of the delayed ACK/NACK system is that partial transactions will leave read operations incomplete.

Here, we setup a read operation ([0xd1) and read a byte (r). Since the Bus Pirate has no way of knowing if the next operation will be another read (r) or a stop condition (]), it leaves the ninth bit hanging. The warning "*(N)ACK PENDING" alerts you to this state.

Our next command is another read (r), so the Bus Pirate ACKs the previous read and gets another byte. Again, it leaves the (N)ACK bit pending until the next command.

The final command is STOP (]). The Bus Pirate ends the read with a NACK and then sends the stop condition. 

Connections
------------------

| Bus Pirate | Dir. | Circuit | Description   |
| ----------:|:----:|:------- | ------------- |
| MOSI       | ↔    | SDA     | Serial Data   |
| CLK        | →    | SCL     | Serial Clock  |
| GND        | ⏚    | GND     | Signal Ground |