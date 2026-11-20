# Bus Pirate

>
> THIS REPOSITORY IS ONLY FOR THE OLD BUS PIRATE v3.x and v4.x HARDWARE!
>
> If you own a Bus Pirate 5 or later, please see https://buspirate.com
>

The Bus Pirate is an open source hacker multi-tool that talks to electronic stuff.
It's got a bunch of features an intrepid hacker might need to prototype their next project.

This community firmware was forked from the official Dangerous Prototypes firmware for these devices.
This repository represents the hard work of community members to sustain and continue the legacy of the Bus Pirate v3 and v4 devices.

## Protocols
    
* 1-Wire
* I2C
* SPI
* JTAG
* Asynchronous serial
* MIDI
* PC keyboard
* HD44780 LCD
* 2- and 3-wire libraries with bitwise pin control
* Scriptable binary bitbang, 1-Wire, I2C, SPI, and UART modes.

## Features

* 0 - 5.5v tolerant pins
* 0 - 6v measurement probe
* 1Hz - 40MHz frequency measurement
* 1kHz - 4MHz pulse-width modulator, frequency generator
* On-board multi-voltage pull-up resistors
* On-board 3.3v and 5v power supplies with software reset
* Macros for common operations
* Bus traffic sniffers (SPI, I2C)
* A bootloader for easy firmware updates
* Transparent USB&rarr;serial mode
* 10Hz - 1MHz low-speed logic analyzer
* Servo driver
* Can program many AVR microcontrollers
  * Supported by AVRdude
  * Can emulate the AVR STK500 v2 (built-in AVRISP-MKII personality)
  * Programs FPGAs and CPLDs with the built-in XSVF JTAG player
* Scriptable from Perl, Python, etc.
* Public domain (Creative Commons Zero) source. Prototype with the Bus Pirate, then use the code in your project however you want. 

## Application Support

The Bus Pirate is used through a simple terminal interface, but these applications also support the Bus Pirate as a programming device, etc.

* [AVRDude] AVR programmer (AVRDude v5.8+, firmware v4 (any) or v5.9+)
* [OpenOCD] JTAG debugger
* [flashrom] bios/flash programmer 

# Purchasing

> 2025 - If you're looking to purchase an old Bus Pirate v3, or v4.. don't
> spend over $30-40 USD. Spend your hard-earned cash on a Bus Pirate 5+ https://buspirate.com

* Bus Pirate v4.0 - *Newer, more space for bigger firmwares*
  * Worldwide: amazon.com has some overpriced old stock boards.

* Bus Pirate v3.6 - *Proven, more well tested*
  * Worldwide: amazon.com has some overpriced old stock boards.
  * Europe: <https://www.robotshop.com/uk/bus-pirate-v36a.html>
  * USA: <https://anibit.com/product/ptt07001>

[OpenOCD]: http://openocd.org
[AVRDude]: http://www.nongnu.org/avrdude
[flashrom]: https://www.flashrom.org/Flashrom

## Firmware Flashing

Refer to [building-and-flashing-firmware](Documentation/building-and-flashing-firmware.md) to see how to build and flash this firmware.
