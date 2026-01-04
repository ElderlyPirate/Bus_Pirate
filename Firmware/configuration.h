/*
 * This file is part of the Bus Pirate project
 * (http://code.google.com/p/the-bus-pirate/).
 *
 * Written and maintained by the Bus Pirate project.
 *
 * To the extent possible under law, the project has
 * waived all copyright and related or neighboring rights to Bus Pirate. This
 * work is published from United States.
 *
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef BP_CONFIGURATION_H
#define BP_CONFIGURATION_H

/* Hardware identification. */

#if defined(__PIC24FJ256GB106__)

/**
 * The firmware is built for a Bus Pirate v4.
 */
#define BUSPIRATEV4

#elif defined(__PIC24FJ64GA002__)

/**
 * The firmware is built for a Bus Pirate v3.
 */
#define BUSPIRATEV3

#endif /* __PIC24FJ256GB106__ || __PIC24FJ64GA002__ */

#ifdef BUSPIRATEV3

/**
 * If this firmware is guaranteed to not run on MCUs revision A3 or A4 and this
 * definition is enabled, all the workarounds for known erratas for
 * PIC24FJ64GA002 will not be enabled.  This will save a few bytes of code
 * overall.
 */
#undef BPV3_IS_REV_B4_OR_LATER

#endif /* BUSPIRATEV3 */

/* Feature inclusion/exclusion definitions. */

/**
 * #define BP_ENABLE_1WIRE_SUPPORT
 *
 * Enable 1-Wire protocol support in the built firmware.
 *
 * @note BPv3 default firmware status: INCLUDED
 * @note BPv4 default firmware status: INCLUDED
 *
 * From https://en.wikipedia.org/wiki/1-Wire :
 *
 * 1-Wire is a device communications bus system designed by Dallas
 * Semiconductor Corp. that provides low-speed data, signaling, and power over
 * a single conductor.  1-Wire is similar in concept to I2C, but with lower
 * data rates and longer range. It is typically used to communicate with small
 * inexpensive devices such as digital thermometers and weather instruments. A
 * network of 1-Wire devices with an associated master device is called a
 * MicroLAN.  One distinctive feature of the bus is the possibility of using
 * only two wires: data and ground.  To accomplish this, 1-Wire devices include
 * an 800 pF capacitor to store charge, and to power the device during periods
 * when the data line is active.
 */

/**
 * #define BP_ENABLE_BASIC_SUPPORT
 *
 * Enable a cut-down BASIC interpreter to automate tasks in a more structured
 * way than macros alone.
 *
 * @note BPv3 default firmware status: INCLUDED
 * @note BPv4 default firmware status: INCLUDED
 */

/**
 * #define BP_ENABLE_DIO_SUPPORT
 *
 * Enable Direct I/O control over the hardware pins using the binary
 * communication protocol.
 *
 * @note BPv3 default firmware status: INCLUDED
 * @note BPv4 default firmware status: INCLUDED
 */

/**
 * #define BP_ENABLE_HD44780_SUPPORT
 *
 * Enable controlling a HD44780 LCD controller connected via an expansion board.
 *
 * @note BPv3 default firmware status: OPTIONAL
 * @note BPv4 default firmware status: OPTIONAL
 *
 * This module allows interaction with a HD44780-based expansion board like this
 * one: http://dangerousprototypes.com/docs/Bus_Pirate_LCD_adapter_v2 or this
 * one: http://dangerousprototypes.com/docs/Bus_Pirate_v4_LCD_adapter or common
 * I2C "backpacks" based on the PCF8574 I/O expander
 */

/**
 * #define BP_ENABLE_I2C_SUPPORT
 *
 * Enable I2C protocol support in the built firmware.
 *
 * @note BPv3 default firmware status: INCLUDED
 * @note BPv3 default firmware status: INCLUDED
 *
 * From https://en.wikipedia.org/wiki/I%B2C :
 *
 * I2C (Inter-Integrated Circuit), pronounced I-squared-C, is a multi-master,
 * multi-slave, single-ended, serial computer bus invented by Philips
 * Semiconductor (now NXP Semiconductors). It is typically used for attaching
 * lower-speed peripheral ICs to processors and microcontrollers in
 * short-distance, intra-board communication. Alternatively I2C is spelled I2C
 * (pronounced I-two-C) or IIC (pronounced I-I-C).  Since October 10, 2006, no
 * licensing fees are required to implement the I2C protocol. However, fees are
 * still required to obtain I2C slave addresses allocated by NXP.  Several
 * competitors, such as Siemens AG (later Infineon Technologies AG, now Intel
 * mobile communications), NEC, Texas Instruments, STMicroelectronics (formerly
 * SGS-Thomson), Motorola (later Freescale, now merged with NXP), Nordic
 * Semiconductor and Intersil, have introduced compatible I2C products to the
 * market since the mid-1990s.  SMBus, defined by Intel in 1995, is a subset of
 * I2C that defines the protocol use more strictly. One purpose of SMBus is to
 * promote robustness and interoperability. Accordingly, modern I2C systems
 * incorporate some policies and rules from SMBus, sometimes supporting both
 * I2C and SMBus, requiring only minimal reconfiguration either by commanding
 * or output pin use.
 */

/**
 * #define BP_ENABLE_PIC_SUPPORT
 *
 * Enable support for programming PIC microcontrollers by using the PIC
 * programming software described at
 * http://dangerousprototypes.com/docs/Pirate_PIC_Programmer
 *
 * @note BPv3 default firmware status: INCLUDED
 * @note BPv4 default firmware status: INCLUDED
 *
 * Current support is for PIC10, PIC12, PIC14, PIC16, and PIC18 MCU families.
 */

/**
 * #define BP_ENABLE_PC_AT_KEYBOARD_SUPPORT
 *
 * Enable support for reading scancodes from a PC-AT keyboard bus.
 *
 * @note BPv3 default firmware status: OPTIONAL
 * @note BPv4 default firmware status: INCLUDED
 */

/**
 * #define BP_ENABLE_RAW_2WIRE_SUPPORT
 *
 * Enable support for controlling two I/O pins in an arbitrary way to implement
 * custom binary protocols.
 *
 * @note BPv3 default firmware status: INCLUDED
 * @note BPv4 default firmware status: INCLUDED
 */

/**
 * #define BP_ENABLE_RAW_3WIRE_SUPPORT
 *
 * Enable support for controlling three I/O pins in an arbitrary way to
 * implement custom binary protocols.
 *
 * @note BPv3 default firmware status: INCLUDED
 * @note BPv4 default firmware status: INCLUDED
 */

/**
 * #define BP_ENABLE_SPI_SUPPORT
 *
 * Enable SPI protocol support in the built firmware.
 *
 * @note BPv3 default firmware status: INCLUDED
 * @note BPv3 default firmware status: INCLUDED
 *
 * From https://en.wikipedia.org/wiki/Serial_Peripheral_Interface_Bus :
 *
 * The Serial Peripheral Interface (SPI) bus is a synchronous serial
 * communication interface specification used for short distance communication,
 * primarily in embedded systems.  The interface was developed by Motorola and
 * has become a de facto standard.  Typical applications include Secure Digital
 * cards and liquid crystal displays.  SPI devices communicate in full duplex
 * mode using a master-slave architecture with a single master.  The master
 * device originates the frame for reading and writing.  Multiple slave devices
 * are supported through selection with individual slave select (SS) lines.
 * Sometimes SPI is called a four-wire serial bus, contrasting with three-,
 * two-, and one-wire serial buses.  The SPI may be accurately described as a
 * synchronous serial interface, but it is different to the Synchronous Serial
 * Interface (SSI) protocol, which is also a four-wire synchronous serial
 * communication protocol, but employs differential signaling and provides
 * only a single simplex communication channel.
 */

/**
 * #define BP_ENABLE_UART_SUPPORT
 *
 * Enable UART I/O support in the built firmware.
 *
 * @note BPv3 default firmware status: INCLUDED
 * @note BPv4 default firmware status: INCLUDED
 *
 * From https://en.wikipedia.org/wiki/UART :
 *
 * A universal asynchronous receiver/transmitter, abbreviated UART, is a
 * computer hardware device that translates data between characters (usually
 * bytes) in a computer and an asynchronous serial communication format that
 * encapsulates those characters between start bits and stop bits.  UARTs are
 * commonly used in conjunction with communication standards such as TIA
 * (formerly EIA) RS-232, RS-422 or RS-485.  The universal designation
 * indicates that the data format and transmission speeds are configurable.
 * The electric signaling levels and methods (such as differential signaling
 * etc.) are handled by a driver circuit external to the UART.  A UART is
 * usually an individual (or part of an) integrated circuit (IC) used for
 * serial communications over a computer or peripheral device serial port.
 * UARTs are now commonly included in microcontrollers.  A dual UART, or DUART,
 * combines two UARTs into a single chip.  An octal UART or OCTART combines
 * eight UARTs into one package, such as the Exar XR16L788 or the NXP SCC2698.
 * A related device, the Universal Synchronous/Asynchronous
 * Receiver/Transmitter (USART) also supports synchronous operation.
 */

/**
 * #define BP_ENABLE_SUMP_SUPPORT
 *
 * Enables support for SUMP-compatible logic analyzer programs, using the
 * Bus Pirate as a signal acquisition device.
 *
 * @note BPv3 default firmware status: INCLUDED
 * @note BPv4 default firmware status: INCLUDED
 *
 * More information can be obtained at
 * http://www.sump.org/projects/analyzer/protocol/
 */

/**
 * #define BP_ENABLE_JTAG_SUPPORT
 *
 * Enables support for JTAG control programs, using the Bus Pirate as a hardware
 * debug module on the device under test.
 *
 * @note BPv3 default firmware status: INCLUDED
 * @note BPv4 default firmware status: INCLUDED
 *
 * OpenOCD support is not yet available on v4 boards, however plans are
 * underway to get this working.
 */

/**
 * #define BP_ENABLE_SMPS_SUPPORT
 *
 * Enables support for driving a switched mode power supply via binary I/O
 * commands.
 *
 * @note BPv3 default firmware status: DISABLED
 * @note BPv4 default firmware status: OPTIONAL
 *
 * The schematic for such an interface board can be found at
 * http://dangerousprototypes.com/docs/Bus_Pirate_SMPS - although it is not
 * clear whether this design is compatible with the current code.  In the
 * DangerousPrototypes' repository, support for the SMPS board was limited to
 * Bus Pirate v4 boards, whilst the information on that page is dated from 2010.
 *
 * From https://en.wikipedia.org/wiki/Switched-mode_power_supply :
 *
 * A switched-mode power supply (switching-mode power supply, switch-mode power
 * supply, switched power supply, SMPS, or switcher) is an electronic power
 * supply that incorporates a switching regulator to convert electrical power
 * efficiently. Like other power supplies, an SMPS transfers power from a DC or
 * AC source (often mains power), to DC loads, such as a personal computer,
 * while converting voltage and current characteristics. Unlike a linear power
 * supply, the pass transistor of a switching-mode supply continually switches
 * between low-dissipation, full-on and full-off states, and spends very little
 * time in the high dissipation transitions, which minimizes wasted energy.
 * Ideally, a switched-mode power supply dissipates no power. Voltage regulation
 * is achieved by varying the ratio of on-to-off time. In contrast, a linear
 * power supply regulates the output voltage by continually dissipating power in
 * the pass transistor. This higher power conversion efficiency is an important
 * advantage of a switched-mode power supply. Switched-mode power supplies may
 * also be substantially smaller and lighter than a linear supply due to the
 * smaller transformer size and weight.
 *
 * @todo Clarify whether this code can still be used.
 */

#ifndef BP_CUSTOM_FEATURE_SET

#ifdef BUSPIRATEV4
#define BP_ENABLE_1WIRE_SUPPORT
#define BP_ENABLE_BASIC_SUPPORT
#define BP_ENABLE_DIO_SUPPORT
#undef BP_ENABLE_HD44780_SUPPORT
#define BP_ENABLE_I2C_SUPPORT
#define BP_ENABLE_JTAG_SUPPORT
#define BP_ENABLE_PIC_SUPPORT
#define BP_ENABLE_PC_AT_KEYBOARD_SUPPORT
#define BP_ENABLE_RAW_2WIRE_SUPPORT
#define BP_ENABLE_RAW_3WIRE_SUPPORT
#undef BP_ENABLE_SMPS_SUPPORT
#define BP_ENABLE_SPI_SUPPORT
#define BP_ENABLE_SUMP_SUPPORT
#define BP_ENABLE_UART_SUPPORT
#endif /* BUSPIRATEV4 */

#ifdef BUSPIRATEV3
#define BP_ENABLE_1WIRE_SUPPORT
#define BP_ENABLE_BASIC_SUPPORT
#define BP_ENABLE_DIO_SUPPORT
#define BP_ENABLE_HD44780_SUPPORT
#define BP_ENABLE_I2C_SUPPORT
#define BP_ENABLE_JTAG_SUPPORT
#define BP_ENABLE_PIC_SUPPORT
#undef BP_ENABLE_PC_AT_KEYBOARD_SUPPORT
#define BP_ENABLE_RAW_2WIRE_SUPPORT
#define BP_ENABLE_RAW_3WIRE_SUPPORT
#undef BP_ENABLE_SMPS_SUPPORT
#define BP_ENABLE_SPI_SUPPORT
#define BP_ENABLE_SUMP_SUPPORT
#define BP_ENABLE_UART_SUPPORT
#endif /* BUSPIRATEV3 */

#endif /* !BP_CUSTOM_FEATURE_SET */

#ifdef BP_CUSTOM_FEATURE_SET
#define BP_ENABLE_1WIRE_SUPPORT
#define BP_ENABLE_BASIC_SUPPORT
#define BP_ENABLE_DIO_SUPPORT
#define BP_ENABLE_HD44780_SUPPORT
#define BP_ENABLE_I2C_SUPPORT
#define BP_ENABLE_JTAG_SUPPORT
#define BP_ENABLE_PC_AT_KEYBOARD_SUPPORT
#define BP_ENABLE_PIC_SUPPORT
#define BP_ENABLE_RAW_2WIRE_SUPPORT
#define BP_ENABLE_RAW_3WIRE_SUPPORT
#define BP_ENABLE_SMPS_SUPPORT
#define BP_ENABLE_SPI_SUPPORT
#define BP_ENABLE_SUMP_SUPPORT
#define BP_ENABLE_UART_SUPPORT
#endif /* BP_CUSTOM_FEATURE_SET */

/* 1-Wire module configuration definitions. */

#ifdef BP_ENABLE_1WIRE_SUPPORT

/**
 * Maximum number of devices whose addresses will be stored as MACROs.
 */
#define BP_1WIRE_DEVICE_DEV_ROSTER_SLOTS 10

/**
 * Lookup family ID and print that information when searching devices.
 */
#define BP_1WIRE_LOOKUP_FAMILY_ID

#ifdef BP_1WIRE_LOOKUP_FAMILY_ID

/**
 * Print a description of the found device along with the device model when
 * searching devices.
 */
#define BP_1WIRE_PRINT_FAMILY_DESCRIPTION

#endif /* BP_1WIRE_LOOKUP_FAMILY_ID */

#endif /* BP_ENABLE_1WIRE_SUPPORT */

/* I2C module configuration definitions. */

#ifdef BP_ENABLE_I2C_SUPPORT

#ifdef BUSPIRATEV4

/**
 * Enable usage of the on-board hardware I2C bus.
 *
 * This is mandatory on Bus Pirate v4 hardware, optional on v3.
 */
#define BP_I2C_USE_HW_BUS

#endif /* BUSPIRATEV4 */

#endif /* BP_ENABLE_I2C_SUPPORT */

/* BASIC interpreter module configuration definitions. */

#ifdef BP_ENABLE_BASIC_SUPPORT

/**
 * Enable the `DEBUG` command at the BASIC interpreter prompt, to dump the
 * tokenised program that is currently in memory.
 * 
 * Disabled by default as it is not really useful for normal usage of the
 * Bus Pirate.
 */

#undef BP_BASIC_DEBUG_COMMAND

#ifdef BUSPIRATEV4

/**
 * Enable storing and retrieving saved BASIC scripts on the on-board 8 kilobytes
 * EEPROM that is available on Bus Pirate v4 boards.
 *
 * Disabled by default as the code involved has not been tested yet.
 */

#undef BP_BASIC_I2C_FILESYSTEM

#endif /* BUSPIRATEV4 */

/**
 * Size of the tokens buffer allocate to the BASIC interpreter, in bytes.
 */
#define BP_BASIC_PROGRAM_SPACE 1024

/**
 * Maximum number of nested FOR-LOOP statements the BASIC interpreter can
 * handle.
 * 
 * Each allowance for a nested FOR-LOOP consumes 6 bytes of RAM.
 */
#define BP_BASIC_NESTED_FOR_LOOP_COUNT 4

/**
 * Maximum stack depth for the BASIC interpreter, in frames.
 * 
 * This value changes how many nested GOSUB calls can be made.  Each stack frame
 * consumes 2 bytes.
 */
#define BP_BASIC_STACK_FRAMES_DEPTH 10

#endif /* BP_ENABLE_BASIC_SUPPORT */

/* SPI module configuration definitions. */

#ifdef BP_ENABLE_SPI_SUPPORT

/**
 * Enable special AVR-specific commands for bulk flash reading and other
 * purposes.
 */
#define BP_SPI_ENABLE_AVR_EXTENDED_COMMANDS

/**
 * Enable experimental UART streaming read support.
 */
#undef BP_SPI_ENABLE_STREAMING_READ

/**
 * Enable experimental UART streaming write support.
 */
#undef BP_SPI_ENABLE_STREAMING_WRITE

#endif /* BP_ENABLE_SPI_SUPPORT */

/* SMPS module configuration definitions. */

#ifdef BP_ENABLE_SMPS_SUPPORT

#ifdef BUSPIRATEV3

#error "SMPS support is not available on Bus Pirate v3 boards."

#endif /* BUSPIRATEV3 */

#endif /* BP_ENABLE_SMPS_SUPPORT */

/* JTAG module configuration definitions. */

#ifdef BP_ENABLE_JTAG_SUPPORT

/**
 * Maximum number of bit sequences to handle in a single command set.
 */
#define BP_JTAG_OPENOCD_BIT_SEQUENCES_LIMIT 0x2000

/**
 * Allow OpenOCD to interact with the Bus Pirate board for JTAG operations.
 */
#define BP_JTAG_OPENOCD_SUPPORT

#ifdef BUSPIRATEV4

#ifdef BP_JTAG_OPENOCD_SUPPORT

/**
 * Add an extra delay before and after OpenOCD I/O bursts.
 */
#define BP_JTAG_OPENOCD_DELAY

#endif /* BP_JTAG_OPENOCD_SUPPORT */

/**
 * Enable support for XSVF JTAG commands.
 *
 * This is not enabled on v3 boards due to taking up too much memory.
 *
 * More information at https://en.wikipedia.org/wiki/Serial_Vector_Format
 */
#define BP_JTAG_XSVF_SUPPORT

#endif /* BUSPIRATEV4 */

#endif /* BP_ENABLE_JTAG_SUPPORT */

/* SMPS module configuration definitions. */

#ifdef BP_ENABLE_SMPS_SUPPORT

#ifndef BUSPIRATEV4
#error "SMPS support is not available on Bus Pirate v3"
#endif /* !BUSPIRATEV4 */

#endif /* BP_ENABLE_SMPS_SUPPORT */

/* Module-agnostic configuration definitions. */

/**
 * Enable the 'h' command to show command history.
 */
#define BP_ENABLE_COMMAND_HISTORY

#ifdef BP_ENABLE_COMMAND_HISTORY

/**
 * How many past entries the command history should keep track of.
 */
#ifdef BUSPIRATEV3
#define BP_COMMAND_HISTORY_LENGTH 7
#else
#define BP_COMMAND_HISTORY_LENGTH 15
#endif /* BUSPIRATEV3 */

#endif /* BP_ENABLE_COMMAND_HISTORY */

/**
 * How many user-defined macros can be set.
 */
#ifdef BUSPIRATEV3
#define BP_USER_MACROS_COUNT 3
#else
#define BP_USER_MACROS_COUNT 5
#endif /* BUSPIRATEV3 */

/**
 * Maximum length, in bytes of a user-defined macro.
 */
#ifdef BUSPIRATEV3
#define BP_USER_MACRO_MAX_LENGTH 24
#else
#define BP_USER_MACRO_MAX_LENGTH 32
#endif /* BUSPIRATEV3 */

/**
 * How big the serial command buffer can be, in bytes.
 *
 * @warning This must be set to a power of two, ie. 256, 128, 64, 32, etc.
 */
#ifdef BUSPIRATEV3
#define BP_COMMAND_BUFFER_SIZE 128
#else
#define BP_COMMAND_BUFFER_SIZE 256
#endif /* BUSPIRATEV3 */

/**
 * How big the serial terminal buffer can be, in bytes.
 *
 * @todo Make sure that no hard-coded buffer sizes are present.
 */
#define BP_TERMINAL_BUFFER_SIZE 4096

/**
 * Use a hardware timer to calculate delays instead of a NOP-based busy loop.
 */
#define BP_USE_HARDWARE_DELAY_TIMER

#endif /* !BP_CONFIGURATION_H */
