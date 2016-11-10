SPI Mode
===================

  - **Bus:** [SPI](https://en.wikipedia.org/wiki/Serial_Peripheral_Interface_Bus)
  - **Connections:** four pins (MOSI/MISO/CLOCK/CS) and ground
  - **Output types:** 
    - 3.3volt normal output (high=3.3volts, low=ground)
    - [open collector](http://en.wikipedia.org/wiki/High_impedence) (high=Hi-Z, low=ground), [pull-up resistors](http://dangerousprototypes.com/docs/Practical_guide_to_Bus_Pirate_pull-up_resistors) required. Use open drain/open collector output types with pull-up resistors for multi-voltage interfacing.
  - **Pull-up resistors:** required for open collector output mode (2K – 10K)
  - **Maximum voltage:** 5.5volts (5volt safe)
  - **Speed:** 30, 50, 125, 250 kHz; 1, 1.3, 2, 2.6, 3.2, 4, 5.3, 8 MHz
  - **Clock polarity:** idle 1, idle 0
  - **Output clock edge:** idle to active, active to idle. Point at which data is changed on the SDO line, this does not include the MSB (first bit) which is ready immediately after CS becomes active.
  - **Input sample phase:** middle, end.


Starting SPI Mode
---------------------
    HiZ>m    <<<open the mode menu
    1. HiZ
    …
    5. SPI
    …

    (1)>5    <<<choose SPI mode
    Set speed:
     1.  30 KHz
     2. 125 KHz
     3. 250 KHz
     4.   1 MHz
     5.  50 KHz
     6. 1.3 MHz
     7.   2 MHz
     8. 2.6 MHz
     9. 3.2 MHz
    10.   4 MHz
    11. 5.3 MHz
    12.   8 MHz

    (1)>4    <<<choose SPI speed
    Clock polarity:
     1. Idle low *default
     2. Idle high

    (1)>     <<<choose clock polarity (default accepted)
    Output clock edge:
     1. Idle to active
     2. Active to idle *default

    (2)>     <<<choose output edge (default accepted)
    Input sample phase:
     1. Middle *default
     2. End

    (1)>     <<<choose input sample phase (default accepted)
    CS:
     1. CS
     2. /CS *default

    (2)>     <<<choose chip select polarity (default accepted)
    Select output type:
     1. Open drain (H=Hi-Z, L=GND)
     2. Normal (H=3.3V, L=GND)

    (1)>2     <<<choose output type
    Ready


Command Syntax
-------------------

  * A/a/@
	  * Toggle auxiliary pin.
	  * Capital “A” sets AUX high.
	  * Small “a” sets to ground.
	  * @ sets aux to input (high impedance mode) and reads the pin value.
  * D/d (v1+ hardware only)
	  * Measure voltage on the ADC pin.
  * W/w (v1+ hardware only)
	  * Capital ‘W' enables the on-board power supplies.
	  * Small ‘w' disables them.
  * [
      * Chip select (CS) active.
  * {
	  * CS active, show the SPI read byte after every write.
  * ] or }
	  * CS disable. 
  * r
	  * Read one byte by sending dummy byte (0xff). (r:1…255 for bulk reads) 
  * 0b
	  * Write this binary value.
	  * Format is 0b00000000 for a byte, but partial bytes are also fine: 0b1001.
  * 0x
	  * Write this HEX value.
	  * Format is 0x01. Partial bytes are fine: 0xA. A-F can be lower-case or capital letters.
  * 0-255
	  * Write this decimal value, check ACK.
	  * Any number not preceded by 0x or 0b is interpreted as a decimal value.
  * , space
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
| 1 | SPI bus sniffer, sniff when CS is low (hardware CS filter). |
| 2 | SPI bus sniffer, sniff all traffic (no CS filter). |
| 3 | ~~SPI bus sniffer, sniff when CS is high (software CS filter)~~ Temporarily removed to increase speed. |
| 10 | Change clock polarity to 0 without re-entering SPI mode. |
| 11 | Change clock polarity to 1 without re-entering SPI mode. |
| 12 | Change clock edge to 0 without re-entering SPI mode. |
| 13 | Change clock edge to 1 without re-entering SPI mode. |
| 14 | Change sample phase to 0 without re-entering SPI mode. |
| 15 | Change sample phase to 1 without re-entering SPI mode. |
  
### SPI Bus sniffer

The Bus Pirate can read the traffic on an SPI bus.

The SPI sniffer is implemented in hardware and should work up to 10MHz. It follows the configuration settings you entered for SPI mode. 

| _Warning!_ |
|:---------- |
| Enter sniffer mode before connecting the target!! The Bus Pirate SPI CLOCK or DATA lines could be grounded and ruin the target device! Reset with the CS pin to clear garbage if needed. |

---------


Pin connections are the same as normal SPI mode. Connect the Bus Pirate clock to the clock on the SPI bus you want to sniff. The data pins MOSI and MISO are both inputs, connect them to the SPI bus data lines. Connect the CS pin to the SPI bus CS pin.

  * [/] – CS enable/disable
  * 0xXX – MOSI read
  * (0xXX) – MISO read

SPI CS pin transitions are represented by the normal Bus Pirate syntax. The byte sniffed on the MISO pin is displayed inside ().

    SPI> (0)
    0.Macro menu
    1.Sniff CS low
    2.Sniff all traffic
    SPI> (1)
    Sniffer
    Any key to exit
    [0x30(0x00)0xff(0x12)0xff(0x50)][0x40(0x00)]


The SPI sniffer can read all traffic, or filter by the state of the CS pin. The byte sniffed on the MOSI pin is displayed as a HEX formatted value, the byte sniffed on the MISO pin is inside the ().

| |
|:-------- |
| There may be an issue in the sniffer terminal mode from v5.2+. Try the [binary mode sniffer utility](Bus_Pirate_binary_SPI_sniffer_utility) for best results. |


#### Notes
The sniffer uses a 4096byte output [ring buffer](http://en.wikipedia.org/wiki/Circular_buffer). Sniffer output goes into the ring buffer and gets pushed to the PC when the UART is free. This should eliminate problems with dropped bytes, regardless of UART speed or display mode. 

| _Warning!_ |
|:---------- |
| Enter sniffer mode before connecting the target!! The Bus Pirate SPI CLOCK or DATA lines could be grounded and ruin the target device! Reset with the CS pin to clear garbage if needed. |

A long enough stream of data will eventually overtake the buffer, after which the MODE LED turns off (v5.2+). No data can be trusted if the MODE LED is off - this will be improved in a future firmware.

The SPI hardware has a 4 byte buffer. If it fills before we can transfer the data to the ring buffer, then the terminal will display "Can't keep up" and drop back to the SPI prompt. This error and the ring buffer error will be combined in a future update.

Any commands entered after the sniffer macro will be lost.

Pins that are normally output become inputs in sniffer mode. MOSI, CLOCK, MISO, and CS are all inputs in SPI sniffer mode.

Since v5.3 the SPI sniffer uses hardware chip select for the CS low sniffer mode. The minimum time between CS falling and the first clock is 120ns theoretical, and less then 1.275us in tests. The software CS detect (CS high sniffer mode) requires between 27usec and 50usec minimum delay between the transition of the CS line and the start of data. Thanks to Peter Klammer for testing and updates.

The sniffer follows the output clock edge and output polarity settings of the SPI mode, but not the input sample phase.
    
### Clock edge/clock polarity/sample phase macros

Macros 10-15 change SPI setttings without disabling the SPI module. I have no idea if this will work or if it's allowable. These macros were added at a user's request, but they never reported if it worked. More here.

    SPI> (10)(11)(12)(13)(14)(15)
    SPI (spd ckp ske smp csl hiz)=( 3 0 1 0 1 1 )
    SPI (spd ckp ske smp csl hiz)=( 3 1 1 0 1 1 )
    SPI (spd ckp ske smp csl hiz)=( 3 1 0 0 1 1 )
    SPI (spd ckp ske smp csl hiz)=( 3 1 1 0 1 1 )
    SPI (spd ckp ske smp csl hiz)=( 3 1 1 0 1 1 )
    SPI (spd ckp ske smp csl hiz)=( 3 1 1 1 1 1 )
    SPI>

Connections
------------------

| Bus Pirate | Dir. | Circuit | Description          |
| ----------:|:----:|:------- | -------------------- |
| MOSI       | →    | MOSI    | Master Out, Slave In |
| MISO       | ←    | MISO    | Master In, Slave Out |
| CS         | →    | CS      | Chip Select          |
| CLK        | →    | CLK     | Clock signal         |
| GND        | ⏚    | GND     | Signal Ground        |