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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sump.h"

#ifdef BP_ENABLE_SUMP_SUPPORT

#include "base.h"
#include "core.h"
#include "uart.h"
#include "proc_menu.h"

/*
 * SUMP command set - taken from http://www.sump.org/projects/analyzer/protocol/
 * With additions taken from
 * http://dangerousprototypes.com/docs/The_Logic_Sniffer's_extended_SUMP_protocol
 *
 * @TODO: Add commands 0x0F, 0x9E, 0x9F from the extended SUMP protocol?
 * @TODO: Add "Set trigger configuration" command (0xC2, 0xC6, 0xCA, 0xCE).
 * @TODO: Check why samples are sent out backwards.
 * @TODO: Implement continuous sampling.
 * @TODO: Remove sump_command_t.left and turn the structure into two separate
 *        fields.
 */

/**
 * Resets the device.
 *
 * Should be sent 5 times when the receiver status is unknown. (It could be
 * waiting for up to four bytes of pending long command data.)
 */
#define SUMP_RESET 0x00

/**
 * Arms the trigger.
 */
#define SUMP_RUN 0x01

/**
 * Asks for device identification.
 *
 * The device will respond with four bytes. The first three ("SLA") identify
 * the device. The last one identifies the protocol version which is currently
 * either "0" or "1"
 */
#define SUMP_ID 0x02

/**
 * Get metadata.
 *
 * In response, the device sends a series of 1-byte keys, followed by data
 * pertaining to that key. The series ends with the key 0x00. The system can be
 * extended with new keys as more data needs to be reported.
 *
 * Type 0 Keys (null-terminated string, UTF-8 encoded):
 *
 * 0x00 Not used, key means end of metadata
 * 0x01 Device name (e.g. "Openbench Logic Sniffer v1.0", "Bus Pirate v3b")
 * 0x02 Version of the FPGA firmware
 * 0x03 Ancillary version (PIC firmware)
 *
 * Type 1 Keys (32-bit unsigned integer):
 *
 * 0x20 Number of usable probes
 * 0x21 Amount of sample memory available (bytes)
 * 0x22 Amount of dynamic memory available (bytes)
 * 0x23 Maximum sample rate (Hz)
 * 0x24 Protocol version (see below) [*]
 *
 * Type 2 Keys (8-bit unsigned integer):
 *
 * 0x40 Number of usable probes (short)
 * 0x41 Protocol version (short)
 *
 * [*]
 *
 * The protocol version key holds a 4-stage version, one per byte, where the
 * MSB holds the major version number. As of the first release to support this
 * metadata command, the protocol version should be 2. This would be encoded
 * as 0x00000002.
 */
#define SUMP_DESC 0x04

/**
 * Put transmitter out of pause mode.
 *
 * It will continue to transmit captured data if any is pending. This command
 * is being used for XON/XOFF flow control.
 */
#define SUMP_XON 0x11

/**
 * Put transmitter in pause mode.
 *
 * It will stop transmitting captured data. This command is being used for
 * XON/XOFF flow control.
 */
#define SUMP_XOFF 0x13

/**
 * Set Divider.
 *
 * When x is written, the sampling frequency is set to f = clock / (x + 1)
 *
 *          LSB                  MSB
 * 10000000 XXXXXXXXXXXXXXXXXXXXXXXX????????
 *          ||||||||||||||||||||||||
 *          ++++++++++++++++++++++++----------- Divider
 */
#define SUMP_DIV 0x80

/**
 * Set Read & Delay Count.
 *
 * Read Count is the number of samples (divided by four) to read back from
 * memory and sent to the host computer. Delay Count is the number of samples
 * (divided by four) to capture after the trigger fired. A Read Count bigger
 * than the Delay Count means that data from before the trigger match will be
 * read back. This data will only be valid if the device was running long
 * enough before the trigger matched.
 *
 *          LSB          MSB LSB          MSB
 * 10000001 XXXXXXXXXXXXXXXX YYYYYYYYYYYYYYYY
 *          |||||||||||||||| ||||||||||||||||
 *          |||||||||||||||| ++++++++++++++++--- Delay Count
 *          ++++++++++++++++-------------------- Read Count
 */
#define SUMP_CNT 0x81

/**
 * Set Flags.
 *
 * Sets the following flags:
 *
 * - demux: Enables the demux input module. (Filter must be off.)
 * - filter: Enables the filter input module. (Demux must be off.)
 * - channel groups: Disable channel group. Disabled groups are excluded from
 *                   data transmissions. This can be used to speed up transfers.
 *                   There are four groups, each represented by one bit.
 *                   Starting with the least significant bit of the channel
 *                   group field channels are assigned as follows: 0-7, 8-15,
 *                   16-23, 24-31
 * - external: Selects the clock to be used for sampling. If set to 0, the
 *             internal clock divided by the configured divider is used, and if
 *             set to 1, the external clock will be used. (filter and demux are
 *             only available with internal clock)
 * - inverted: When set to 1, the external clock will be inverted before being
 *             used. The inversion causes a delay that may cause problems at
 *             very high clock rates. This option only has an effect with
 *             external set to 1.
 *
 * 10000010 ABCCCCDE ????????????????????????
 *          ||||||||
 *          |||||||+---------------------------- Demux (1: Enable)
 *          ||||||+----------------------------- Filter (1: Enable)
 *          ||++++------------------------------ Channel Groups (1: Disable)
 *          |+---------------------------------- External (1: Enable)
 *          +----------------------------------- Inverted (1: Enable)
 */
#define SUMP_FLAGS 0x82

/**
 * Set Trigger Values.
 *
 * Defines which trigger values must match. In parallel mode each bit
 * represents one channel, in serial mode each bit represents one of the last
 * 32 samples of the selected channel. The opcodes refer to stage 0-3 in the
 * order given below. (Protocol version 0 only supports stage 0.)
 *
 *          LSB                          MSB
 * 1100xx00 XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
 *          ||||||||||||||||||||||||||||||||
 *          ++++++++++++++++++++++++++++++++--- Trigger Mask
 */
#define SUMP_TRIG 0xC0

/**
 * Set trigger mask.
 *
 * Defines which values individual bits must have. In parallel mode each bit
 * represents one channel, in serial mode each bit represents one of the last
 * 32 samples of the selected channel. The opcodes refer to stage 0-3 in the
 * order given above. (Protocol version 0 only supports stage 0.)
 *
 *          LSB                          MSB
 * 1100xx01 XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
 *          ||||||||||||||||||||||||||||||||
 *          ++++++++++++++++++++++++++++++++--- Trigger Mask
 */
#define SUMP_TRIG_VALS 0xC1

/**
 * Not used, key means end of metadata.
 */
#define SUMP_METADATA_END 0x00

/**
 * Device name (e.g. "Openbench Logic Sniffer v1.0", "Bus Pirate v3b").
 */
#define SUMP_METADATA_DEVICE_NAME 0x01

/**
 * Version of the FPGA firmware.
 */
#define SUMP_METADATA_FPGA_VERSION 0x02

/**
 * Ancillary version (PIC firmware).
 */
#define SUMP_METADATA_ANCILLARY_VERSION 0x03

/**
 * Number of usable probes.
 */
#define SUMP_METADATA_USABLE_PROBES_NUMBER 0x20

/**
 * Amount of sample memory available (bytes).
 */
#define SUMP_METADATA_SAMPLE_MEMORY_AVAILABLE 0x21

/**
 * Amount of dynamic memory available (bytes).
 */
#define SUMP_METADATA_DYNAMIC_MEMORY_AVAILABLE 0x22

/**
 * Maximum sample rate (Hz).
 */
#define SUMP_METADATA_MAXIMUM_SAMPLE_RATE 0x23

/**
 * Protocol version.
 */
#define SUMP_METADATA_PROTOCOL_VERSION 0x24

/**
 * Number of usable probes (short).
 */
#define SUMP_METADATA_USABLE_PROBES_SHORT_NUMBER 0x40

/**
 * Protocol version (short).
 */
#define SUMP_METADATA_PROTOCOL_SHORT_VERSION 0x41

/**
 * Default timer period value for polling probes.
 */
#define BP_DEFAULT_TIMER_PERIOD 0x00000640

/**
 * How much memory is allocated for samples, in bytes.
 */
#define BP_SUMP_SAMPLE_MEMORY_SIZE BP_TERMINAL_BUFFER_SIZE

/**
 * The highest sample rate for the Bus Pirate to sample data at, in Hz.
 */
#define BP_SUMP_MAXIMUM_SAMPLE_RATE 1000000

/**
 * How many probes the Bus Pirate can use.
 */
#define BP_SUMP_PROBES_COUNT NUM_OF_SUMP_CHANNELS

/**
 * SUMP protocol version the Bus Pirate supports.
 */
#define BP_SUMP_PROTOCOL_VERSION 2

/**
 * SUMP metadata information for the Bus Pirate device.
 */
static const uint8_t SUMP_METADATA[] = {
    /* Device name. */

    SUMP_METADATA_DEVICE_NAME,
#ifdef BUSPIRATEV4
    'B', 'P', 'v', '4', '\0',
#elif defined(BUSPIRATEV3)
    'B', 'P', 'v', '3', '\0',
#else
#error "Invalid or unknown Bus Pirate version!"
#endif /* BUSPIRATEV4 || BUSPIRATEV3 */

    /* Sample memory (4096 bytes). */

    SUMP_METADATA_SAMPLE_MEMORY_AVAILABLE,
    (uint8_t)((uint32_t)BP_SUMP_SAMPLE_MEMORY_SIZE >> 24),
    (uint8_t)(((uint32_t)BP_SUMP_SAMPLE_MEMORY_SIZE >> 16) & 0xFF),
    (uint8_t)(((uint32_t)BP_SUMP_SAMPLE_MEMORY_SIZE >> 8) & 0xFF),
    (uint8_t)((uint32_t)BP_SUMP_SAMPLE_MEMORY_SIZE & 0xFF),

    /* Sample rate (1MHz). */

    SUMP_METADATA_MAXIMUM_SAMPLE_RATE,
    (uint8_t)((uint32_t)BP_SUMP_MAXIMUM_SAMPLE_RATE >> 24),
    (uint8_t)(((uint32_t)BP_SUMP_MAXIMUM_SAMPLE_RATE >> 16) & 0xFF),
    (uint8_t)(((uint32_t)BP_SUMP_MAXIMUM_SAMPLE_RATE >> 8) & 0xFF),
    (uint8_t)((uint32_t)BP_SUMP_MAXIMUM_SAMPLE_RATE & 0xFF),

    /* Number of probes (5). */

    SUMP_METADATA_USABLE_PROBES_SHORT_NUMBER, BP_SUMP_PROBES_COUNT,

    /* Protocol version (v2). */

    SUMP_METADATA_PROTOCOL_SHORT_VERSION, BP_SUMP_PROTOCOL_VERSION,

    SUMP_METADATA_END};

/**
 * SUMP_ID response buffer, advertise ourselves as a Logic Sniffer.
 *
 * According to the specifications on sump.org, this should either be 'SLA0' or
 * 'SLA1', however both Logic Sniffer (http://ols.lxtreme.nl) and
 * sigrok (https://sigrok.org) will not handle the device otherwise.
 */
static const uint8_t SUMP_DEVICE_ID[] = {'1', 'A', 'L', 'S'};

extern bus_pirate_configuration_t bus_pirate_configuration;

/**
 * Sampler states.
 */
typedef enum {
  /** Sampler is not armed, no acquisition is in progress. */
  SAMPLER_IDLE = 0,

  /** Sampler is either ready for acquisition or is currently acquiring. */
  SAMPLER_ARMED
} sump_sampler_state_t;

/**
 * Incoming command processing states.
 */
typedef enum {
  /** Waiting for a command to arrive on the serial port. */
  RX_COMMAND_IDLE = 0,

  /** A long command was received, and parameters need to be acquired. */
  RX_COMMAND_PARAMETERS,

  /** A fully formed command was received and is being processed. */
  RX_COMMAND_PROCESS
} sump_analyzer_command_state_t;

/**
 * SUMP command buffer size.
 *
 * A SUMP command can be up to 5 bytes long (SUMP_DIV, SUMP_CNT, SUMP_FLAGS,
 * SUMP_TRIG, SUMP_TRIG_VALS).
 */
#define SUMP_COMMAND_BUFFER_LENGTH 5

/**
 * SUMP command storage structure.
 */
typedef struct {
  /** Command bytes storage buffer. */
  uint8_t bytes[SUMP_COMMAND_BUFFER_LENGTH];

  /** How many parameter bytes still need to be obtained. */
  uint8_t left;

  /** How many parameter bytes have been obtained so far. */
  uint8_t count;
} __attribute__((packed)) sump_command_t;

/**
 * The current state of the sampler.
 */
static sump_sampler_state_t sampler_state = SAMPLER_IDLE;

/**
 * The current state of the command processor.
 */
static sump_analyzer_command_state_t command_processor_state = RX_COMMAND_IDLE;

/**
 * How many bytes should be acquired in the next sampling operation, in bytes.
 */
static unsigned int samples_to_acquire;

// whether trigger is armed
static bool trigger_armed;

/**
 * Acquires data from the probes and sends it out to the controlling software.
 *
 * This function will acquire up to BP_SUMP_SAMPLE_MEMORY_SIZE, by using the
 * value read from the samples_to_acquire variable.  Calling this function
 * where the sampler is not armed will not trigger any action.
 *
 * To avoid rewriting interrupt vectors with the bootloader, this firmware
 * currently uses polling to read the trigger and timer.  A final version
 * should use interrupts after lots of testing.
 *
 * @return true if data acquisition was performed, false otherwise.
 */
static bool sump_acquire_samples(void);

/**
 * Resets the device to start another buffer acquisition.
 */
static void sump_reset(void);

/**
 * Processes the given byte as a part of a SUMP command.
 *
 * @param[in] input_byte the byte to process.
 * @return true if a SUMP_RESET command was received, false otherwise.
 */
static bool sump_handle_command_byte(uint8_t input_byte);

void enter_sump_mode(void) {

  /* Set probing channels to INPUT mode. */
#ifndef BUSPIRATEV4
  IODIR |= (AUX + MOSI + CLK + MISO + CS);
#else
  IODIR |= (AUX0 + AUX1 + AUX2 + MOSI + CLK + MISO + CS + SUMP_SPARE6 + SUMP_SPARE7); // BPv4 PORTD SUMP inputs x9
#endif

  /* Reset the analyzer state. */
  sump_reset();

  /* Trigger the device ID broadcast response. */
  sump_handle_command_byte(SUMP_ID);

  /* Sampling action. */
  for (;;) {

    /* Wait for a command byte to be available on the bus. */
    if (user_serial_ready_to_read()) {

      /* Process the command byte. */
      if (sump_handle_command_byte(user_serial_read_byte())) {
        /*
         * A reset byte was received: leave SUMP and give the terminal
         * the same state a simulated reset produces - HiZ mode plus the
         * banner - so tools (0x0F) and users ('#') can always recover
         * the board without a power cycle.  This mirrors the reset
         * contract that the terminal and binary modes already follow.
         */
        sump_reset();
        bus_pirate_configuration.bus_mode = BP_HIZ;
        print_version_info();
        return;
      }
    }

    /*
     * Start the acquisition process (if the device is not properly set up
     * nothing will happen, so it's safe to call this anyway).
     */
    sump_acquire_samples();
  }
}

void sump_reset(void) {
  /* Switch LED off. */
  BP_LEDMODE = OFF;

#ifdef BUSPIRATEV4 
  CNPU1 &= 0b1001111111111111; //turn off pullups on used pins
  CNPU4 &= 0b1111111111000001;
  CNEN1 &= 0b1001111111111111; //turn off used pins change notice
#ifdef BPv4_SUMP_SOFT_WIRE // See hardwarev4.h
  CNEN4 &= 0b1111111111000001;
#else 
  CNEN4 &= 0b1111111111100001;
  CNEN2 &= 0b1111111111111110;
#endif
#else   
  /* Switch pull-ups off for all pins. */
  CNPU1 = 0;
  CNPU2 = 0;

  /* Disable change notification for all pins. */
  CNEN1 = 0;
  CNEN2 = 0;
#endif

  /* Stop timer #4. */
  T4CON = 0;

  /* Disable change notification interrupts (set priority to 0). */
  IPC4bits.CNIP = 0;

  /* Setup timer periods. */
  PR5 = HI16(BP_DEFAULT_TIMER_PERIOD);
  PR4 = LO16(BP_DEFAULT_TIMER_PERIOD);

  /* Default to acquire a full buffer. */
  samples_to_acquire = BP_SUMP_SAMPLE_MEMORY_SIZE;

  /* Initialize the sampler. */
  sampler_state = SAMPLER_IDLE;
  trigger_armed = false;
}

bool sump_handle_command_byte(unsigned char input_byte) {

  /*
   * The command storage buffer.
   *
   * No need to clear it first, as it will be properly initialized upon
   * receiving a long (5 bytes) command.
   */
  static sump_command_t command_buffer = {.bytes = {0}, .count = 0, .left = 0};

  switch (command_processor_state) {

  /* No command bytes received yet, this is the first one. */
  case RX_COMMAND_IDLE:
    switch (input_byte) {

    /* Reset the analyzer (stays in SUMP mode, as documented). */
    case SUMP_RESET:
      sump_reset();
      return false;

    /*
     * The simulated-reset bytes leave SUMP: 0x0F is the reset byte the
     * binary modes use, '#' is the terminal's reset command.  SUMP has no
     * protocol-level exit, so without this the board stays trapped until
     * a power cycle.
     */
    case 0x0F:
    case '#':
      return true;

    /* Send the device identification buffer. */
    case SUMP_ID:
      bp_write_buffer(SUMP_DEVICE_ID, sizeof(SUMP_DEVICE_ID));
      break;

    /* Arm the sampler. */
    case SUMP_RUN:
      /* Turn the LED on. */
      BP_LEDMODE = ON;

      /* Stop timer #4. */
      T4CON = 0;

      /* Clear timer #5 holding register. */
      TMR5HLD = 0;

      /* Setup timer #4 counter. */
      TMR4 = 0;

      /* Timer #4 counter will be 32 bits wide. */
      T4CONbits.T32 = ON;

      /* Clear change notification interrupt flag. */
      IFS1bits.CNIF = OFF;

      /* Disable change notification interrupts. */
      IEC1bits.CNIE = OFF;

      /* Set change notification interrupt priority to 1. */
      IPC4bits.CNIP = 1;

      /* Update sampler state. */
      sampler_state = SAMPLER_ARMED;
      break;

    /* Send device description. */
    case SUMP_DESC:
      bp_write_buffer(SUMP_METADATA, sizeof(SUMP_METADATA));
      break;

    /* Start/Stop data flow. */
    case SUMP_XON:
    case SUMP_XOFF:
      /* Stop and resume are not supported yet. */
      break;

    /* It must be a long command then. */
    default:
      /* Store the first byte. */
      command_buffer.bytes[0] = input_byte;

      /* Update counters. */
      command_buffer.left = 4;
      command_buffer.count = 0;

      /* Update state. */
      command_processor_state = RX_COMMAND_PARAMETERS;
      break;
    }

    break;

  /* Keep reading parameter data. */
  case RX_COMMAND_PARAMETERS:

    /* Update command count. */
    command_buffer.count++;

    /* Fill buffer. */
    command_buffer.bytes[command_buffer.count] = input_byte;

    /* Check whether the buffer is full or not. */
    if (command_buffer.count < command_buffer.left) {
      break;
    }

  /* Intentional fall-through. */

  /* Process the fully read command buffer. */
  case RX_COMMAND_PROCESS:
    switch (command_buffer.bytes[0]) {
    /* Set triggers. */
    case SUMP_TRIG:
      trigger_armed = false;

#ifdef BUSPIRATEV4                    
#ifdef BPv4_SUMP_SOFT_WIRE // See hardwarev4.h
      //AUX1 on PORTD:8 will be moved in firmware to B7 of SUMP byte
      if (command_buffer.bytes[1] & 0b10000000)
        CNEN4 |= 0b0000000000100000;
#else // possible GREEN WIRE mode PORTD8 to PORTD7
      if (command_buffer.bytes[1] & 0b10000000)
        CNEN2 |= 0b0000000000000001; //AUX1 on PORTD:7
#endif
      if (command_buffer.bytes[1] & 0b00100000)
        CNEN1 |= 0b0100000000000000; // AUX0
      if (command_buffer.bytes[1] & 0b00010000)
        CNEN1 |= 0b0010000000000000; // CS
      if (command_buffer.bytes[1] & 0b00001000)
        CNEN4 |= 0b0000000000010000; // MISO
      if (command_buffer.bytes[1] & 0b00000100)
        CNEN4 |= 0b0000000000001000; // CLK
      if (command_buffer.bytes[1] & 0b00000010)
        CNEN4 |= 0b0000000000000100; // MOSI
      if (command_buffer.bytes[1] & 0b00000001)
        CNEN4 |= 0b0000000000000010; //AUX2

      if (command_buffer.bytes[1] & 0b10111111)
        trigger_armed = true;
#else 
      if (command_buffer.bytes[1] & 0b00010000)
        CNEN2 |= 0b0000000000000001; // AUX
      if (command_buffer.bytes[1] & 0b00001000)
        CNEN2 |= 0b0000000000100000; // MOSI
      if (command_buffer.bytes[1] & 0b00000100)
        CNEN2 |= 0b0000000001000000; // CLK
      if (command_buffer.bytes[1] & 0b00000010)
        CNEN2 |= 0b0000000010000000; // MISO
      if (command_buffer.bytes[1] & 0b00000001)
        CNEN2 |= 0b0000000100000000; // CS

      if (command_buffer.bytes[1] & 0b00011111)
        trigger_armed = true;
#endif
      break;

    case SUMP_FLAGS:
      /* @TODO: Fill this? */
      break;

    /* Read requested samples buffer size. */
    case SUMP_CNT:
      samples_to_acquire =
          (((command_buffer.bytes[2] << 8) + command_buffer.bytes[1]) + 1) * 4;

      /* Clamp sample counter if more bytes are requested. */
      if (samples_to_acquire > BP_SUMP_SAMPLE_MEMORY_SIZE) {
        samples_to_acquire = BP_SUMP_SAMPLE_MEMORY_SIZE;
      }
      break;

    case SUMP_DIV: {
      uint32_t period;

      /*
       * Read the 24-bits period value and rescale from SUMP's
       * own 100MHz frequency range to the internal 16MIPs
       * range.
       */
      period = (((((uint32_t)command_buffer.bytes[3] << 16) +
                  ((uint32_t)command_buffer.bytes[2] << 8) +
                  (uint32_t)command_buffer.bytes[1]) + 1) * 4) / 25;

      /* Round down if needed. */
      if (period > 0x10) {
        period -= 0x10;
      } else {
        period = 1;
      }

      /* Set timer period. */
      PR5 = HI16(period);
      PR4 = LO16(period);

      break;
    }
    }

    command_processor_state = RX_COMMAND_IDLE;
    break;
  }

  return false;
}

bool sump_acquire_samples(void) {
  switch (sampler_state) {

  /* Can start sampling. */
  case SAMPLER_ARMED: {
    size_t offset;

    /* Skip if no interrupt and no trigger set. */
    if (!IFS1bits.CNIF && trigger_armed) {
      break;
    }

    /* Take samples. */

    /* Start timer #4. */
    T4CONbits.TON = ON;

    /* Clear timer #4 interrupt flag. */
    IFS1bits.T5IF = OFF;

    /* Capture samples into the terminal buffer. */
    for (offset = 0; offset < samples_to_acquire; offset++) {
#ifndef BUSPIRATEV4
      bus_pirate_configuration.terminal_input[offset] = PORTB >> 6;
#else

#ifdef BPv4_SUMP_SOFT_WIRE // See hardwarev4.h
      unsigned int sump_port = (PORTD & 0x13f);
      if (sump_port & 0x100)
          sump_port |= 0x80;
      
      bus_pirate_configuration.terminal_input[offset] = sump_port;
#else
      bus_pirate_configuration.terminal_input[offset] = (BYTE)(PORTD & 0xff);
#endif

#endif  
      /* Wait for timer4 interrupt to trigger. */
      while (IFS1bits.T5IF == OFF) {
      }

      /* Clear timer #4 interrupt flag. */
      IFS1bits.T5IF = OFF;
    }

#ifdef BUSPIRATEV4
    //turn off used pins change notice
    CNEN1 &= 0b1001111111111111;
#ifdef BPv4_SUMP_SOFT_WIRE // See hardwarev4.h
    CNEN4 &= 0b1111111111000001;
#else 
    CNEN4 &= 0b1111111111100001;
    CNEN2 &= 0b1111111111111110;
#endif
#else
    /* Disable change notification for pins 16 to 31. */
    CNEN2 = 0;
#endif

    /* Stop timer #4. */
    T4CON = OFF;

    /* Write captured samples out. */
    for (offset = samples_to_acquire; offset > 0; offset--) {
      user_serial_transmit_character(bus_pirate_configuration.terminal_input[offset - 1]);
    }

    /* Reset the analyzer state. */
    sump_reset();

    /* Acquisition complete. */
    return true;
  }

  case SAMPLER_IDLE:
  default:
    /* Nothing to do. */
    break;
  }

  /* Acquisition was not performed. */
  return false;
}

#endif /* BP_ENABLE_SUMP_SUPPORT */
