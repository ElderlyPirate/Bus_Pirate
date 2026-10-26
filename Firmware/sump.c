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

/*
 * SUMP command set - taken from http://www.sump.org/projects/analyzer/protocol/
 * With additions taken from
 * http://dangerousprototypes.com/docs/The_Logic_Sniffer's_extended_SUMP_protocol
 *
 * @TODO: Add commands 0x0F, 0x9E, 0x9F from the extended SUMP protocol?
 * @TODO: Add "Set trigger configuration" command (0xC2, 0xC6, 0xCA, 0xCE).
 * @TODO: Implement continuous sampling.
 * @TODO: Remove sump_command_t.left and turn the structure into two separate
 *        fields.
 *
 * Samples are sent newest-first ("backwards").  That is not a bug: the
 * Bus Pirate profiles for the Openbench Logic Sniffer client say
 * device.samples.reverseOrder = false ("last sample first"), so the host
 * expects the most recent sample right after the capture starts streaming.
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
 *
 * The timer-driven sampler covers 1MHz and below; above that the capture
 * switches to cycle-counted polling loops (2, 4, 8 and 16MHz), where the
 * 16MHz rate is one sample per instruction cycle of the 16MIPs core.
 */
#define BP_SUMP_MAXIMUM_SAMPLE_RATE 16000000

/**
 * How many probes the Bus Pirate can use.
 */
#define BP_SUMP_PROBES_COUNT NUM_OF_SUMP_CHANNELS

/**
 * How a raw port read turns into one sample byte, and the channels that
 * are actually recorded in it.
 *
 * Both variants record every channel the JTR SUMP wiring exposes, mapped
 * to the least significant bits of the byte sent to the host:
 *
 *  - v3: CS, MISO, CLK, MOSI, AUX live on PORTB<10:6>.
 *  - v4: AUX2, MOSI, CLK, MISO, CS and AUX0 live on PORTD<5:0>; AUX1 is
 *    on PORTD<8> and is soft-wired into bit 7 of the byte.  Bit 6 has no
 *    pin behind it, so it is not recorded (the host sees the alternating
 *    pattern there).
 *
 * BP_SUMP_PORT_ADDRESS is the SFR address the fast sampling loops read;
 * the values come from the device .gld files shipped with this firmware
 * (p24FJ64GA002.gld and p24FJ256GB106.gld, part support 1.26.A).
 */
#ifdef BUSPIRATEV4
#define BP_SUMP_SAMPLE_MASK 0xBFu
#define BP_SUMP_SAMPLE_TRANSFORM(port)                                         \
  ((uint8_t)(((port)&0x3Fu) | (((port)&0x100u) >> 1)))
#define BP_SUMP_READ_PROBE() PORTD
#define BP_SUMP_PORT_ADDRESS 0x02DAu /* PORTD */
#else
#define BP_SUMP_SAMPLE_MASK 0x1Fu
#define BP_SUMP_SAMPLE_TRANSFORM(port) ((uint8_t)(((port) >> 6) & 0x1Fu))
#define BP_SUMP_READ_PROBE() PORTB
#define BP_SUMP_PORT_ADDRESS 0x02CAu /* PORTB */
#endif

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

    /* Sample rate (16MHz). */

    SUMP_METADATA_MAXIMUM_SAMPLE_RATE,
    (uint8_t)((uint32_t)BP_SUMP_MAXIMUM_SAMPLE_RATE >> 24),
    (uint8_t)(((uint32_t)BP_SUMP_MAXIMUM_SAMPLE_RATE >> 16) & 0xFF),
    (uint8_t)(((uint32_t)BP_SUMP_MAXIMUM_SAMPLE_RATE >> 8) & 0xFF),
    (uint8_t)((uint32_t)BP_SUMP_MAXIMUM_SAMPLE_RATE & 0xFF),

    /* Ancillary version: "enh" is the community marker for the enlarged
       sample buffers and rates, so the enhanced client profiles can be
       matched against it. */

    SUMP_METADATA_ANCILLARY_VERSION, 'e', 'n', 'h', '\0',

    /* Number of probes. */

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
 * How many samples should be acquired in the next sampling operation.
 *
 * This is the target for the capture: after the channel-count negotiation
 * in SUMP_CNT and the rate-based limits applied when the capture starts,
 * a capture always provides this many samples.  The host is always sent
 * samples_requested samples; when the configured sample rate or buffer
 * cannot supply them all, the shortfall is padded with the alternating
 * 0x55/0xAA pattern (see the transmit helpers).
 */
static unsigned int samples_to_acquire;

/**
 * How many samples the host requested in the last SUMP_CNT command.  The
 * host reads exactly this many sample bytes back.
 */
static unsigned int samples_requested;

/**
 * How many samples after the trigger must fall inside the captured window
 * (the SUMP delay count).  Everything else in the window was captured
 * before the trigger.  At the fast rates (2MHz upwards) the trigger is
 * always at the start of the window and this value is ignored.
 */
static unsigned int trigger_delay_samples;

/**
 * Samples per 16-bit storage word for the timer-driven sampler, and the
 * matching pack/unpack shift: 16 samples per word for one channel, 8 for
 * two, 4 for four and 2 when every channel is recorded.
 */
static unsigned char sump_samples_per_word;
static unsigned char sump_pack_shift;

/**
 * Channel mask for the current capture.  Only these port bits are
 * recorded; the remaining bits of every byte are filled with the
 * alternating pattern to tell the host they were not recorded.
 */
static uint8_t sump_channel_mask;

/**
 * Non-zero when the capture uses one of the cycle-counted fast sampling
 * loops instead of the timer: 1 = 2MHz, 2 = 4MHz, 3 = 8MHz and
 * 4 = 16MHz; 0 selects the timer-driven sampler.
 */
static unsigned char sump_go_fast;

/**
 * How often the timer-driven capture checks for a host byte: the check
 * runs whenever `quiet_samples & sump_host_check_mask` is zero.  Slow
 * rates check after every fragment, to stay responsive; fast rates
 * amortise the call over several fragments.
 */
static unsigned int sump_host_check_mask;

// whether trigger is armed
static bool trigger_armed;

/**
 * Runs one acquisition and sends the captured window to the host.
 *
 * Once the sampler is armed (SUMP_RUN), this captures a full window in
 * one go.  For the timer-driven rates it keeps a rolling pre-trigger ring
 * until the requested number of post-trigger samples has been recorded
 * (the trigger can sit anywhere in the window); the fast rates cannot
 * buffer ahead, so there the trigger marks the start of the window.  A
 * byte arriving from the host aborts the capture; it stays in place for
 * the command loop.  Calling this function when the sampler is not armed
 * does nothing.
 *
 * To avoid rewriting interrupt vectors with the bootloader, this firmware
 * polls the trigger and timer from the sampling loops, rather than using
 * interrupts.
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

        /* A SUMP_RESET command was received, abort. */
        //return; //technically SUMP_RESET stays in SUMP mode
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
  /* Also leave the board button's change notice (CNEN1 bit 0) out of the
     way: CNIF is the SUMP trigger flag, so a button press must not be
     able to fake one.  Its pull-up stays on, for the pin reads elsewhere. */
  CNEN1 &= 0b1001111111111110; //turn off used pins change notice
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

  /* Default to acquire a full buffer with every channel recorded, and
     with the trigger marking the start of the captured window (every
     sample taken after it - the historical Bus Pirate behaviour). */
  samples_requested = BP_SUMP_SAMPLE_MEMORY_SIZE;
  samples_to_acquire = BP_SUMP_SAMPLE_MEMORY_SIZE;
  trigger_delay_samples = BP_SUMP_SAMPLE_MEMORY_SIZE;
  sump_samples_per_word = 1; /* one sample per stored byte */
  sump_pack_shift = 8;
  sump_channel_mask = BP_SUMP_SAMPLE_MASK;
  sump_go_fast = 0;
  sump_host_check_mask = 15;

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

    /* Reset the analyzer. */
    case SUMP_RESET:
      sump_reset();
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

    /* Read the requested sample count and trigger position. */
    case SUMP_CNT: {
      uint32_t read_count;
      uint32_t delay_count;

      read_count = ((((uint32_t)command_buffer.bytes[2] << 8) +
                     command_buffer.bytes[1]) +
                    1) *
                   4;
      delay_count = ((((uint32_t)command_buffer.bytes[4] << 8) +
                      command_buffer.bytes[3]) +
                     1) *
                    4;

      samples_requested = (unsigned int)read_count;

      /*
       * Pick the storage format for the requested capture size: the
       * sample buffer holds 4096 bytes (2048 packed words), so larger
       * captures drop channels to fit - 8k samples keep four channels,
       * 16k keep two and a full 32k capture keeps one.  This mirrors the
       * enhanced logic analyzer's capture sizes (2k/4k/8k/16k/32k).
       */
      if (read_count >= 16ul * 2048ul) {
        samples_to_acquire = 32768;
        sump_samples_per_word = 16;
        sump_pack_shift = 1;
        sump_channel_mask = 0x01;
      } else if (read_count >= 8ul * 2048ul) {
        samples_to_acquire = 16384;
        sump_samples_per_word = 8;
        sump_pack_shift = 2;
        sump_channel_mask = 0x03;
      } else if (read_count >= 4ul * 2048ul) {
        samples_to_acquire = 8192;
        sump_samples_per_word = 4;
        sump_pack_shift = 4;
        sump_channel_mask = 0x0F;
      } else if (read_count >= 2048ul * 2ul) {
        samples_to_acquire = 4096;
        sump_samples_per_word = 1; /* one sample per stored byte */
        sump_pack_shift = 8;
        sump_channel_mask = BP_SUMP_SAMPLE_MASK;
      } else {
        samples_to_acquire = (unsigned int)read_count;
        sump_samples_per_word = 1; /* one sample per stored byte */
        sump_pack_shift = 8;
        sump_channel_mask = BP_SUMP_SAMPLE_MASK;
      }

      /* The trigger position cannot sit past the end of the window. */
      trigger_delay_samples = (delay_count > samples_to_acquire)
                                  ? samples_to_acquire
                                  : (unsigned int)delay_count;
      break;
    }

    case SUMP_DIV: {
      uint32_t period;

      /*
       * Read the 24-bit divider value (x): SUMP asks for a rate of
       * 100MHz / (x + 1), and the timer flag fires every PR + 1 ticks at
       * 16 MIPs, so PR = (x + 1) * 16 / 100 - 1.  The reference
       * enhancement uses x * 4 / 25 for this, which is the same value
       * (the -1 is folded into its truncation); keep it exact like that,
       * so e.g. 1MHz comes out as PR = 15 -> 16MHz / 16 = 1.000 MHz.
       */
      period = (((uint32_t)command_buffer.bytes[3] << 16) +
                ((uint32_t)command_buffer.bytes[2] << 8) +
                (uint32_t)command_buffer.bytes[1]) *
               4 / 25;

      /*
       * Below 10kHz (1600 ticks) the abort check runs after every
       * fragment; faster rates amortise it over several fragments.
       */
      sump_host_check_mask = (period >= 1600u) ? 0u : 15u;

      /*
       * Rates above 1MHz cannot be kept by a timer-polled loop, so they
       * switch to the cycle-counted sampling loops.  The nominal timer
       * periods here: 16MHz = 1 cycle, 8MHz = 2, 4MHz = 4, 2MHz = 8.
       */
      if (period < 1) {
        sump_go_fast = 4; /* 16MHz */
      } else if (period < 3) {
        sump_go_fast = 3; /* 8MHz */
      } else if (period < 7) {
        sump_go_fast = 2; /* 4MHz */
      } else if (period < 15) {
        sump_go_fast = 1; /* 2MHz */
      } else {
        sump_go_fast = 0; /* 1MHz and below: timer-driven */

        /* Set timer period. */
        PR5 = HI16(period);
        PR4 = LO16(period);
      }

      break;
    }
    }

    command_processor_state = RX_COMMAND_IDLE;
    break;
  }

  return false;
}

/*
 * Enhanced capture engine.
 *
 * The plain SUMP mode sampled one byte per timer tick, capped at 1MHz and
 * 4096 samples, and treated the trigger as a start gate.  The enhanced
 * logic analyzer (issue #32) adds three things, implemented here:
 *
 *  - Sample rates up to 16MHz.  Above 1MHz the timer-polled loop gets too
 *    slow, so the capture switches to the cycle-counted polling loops
 *    below, running at one sample per 1, 2, 4 and 8 instruction cycles of
 *    the 16MIPs core.
 *  - Larger captures on fewer channels: the 4096-byte buffer holds 4k
 *    samples with every channel, 8k with four channels, 16k with two and
 *    32k with one, by packing several samples into every 16-bit word.
 *    Channel bits that are not recorded are filled with an alternating
 *    0x55/0xAA pattern so the host can tell them apart.
 *  - A trigger position anywhere in the buffer.  The timer sampler keeps
 *    a rolling pre-trigger ring and stops once the requested number of
 *    post-trigger samples has been recorded; the sample count / ring
 *    bookkeeping is sample-exact.  The fast loops cannot afford that
 *    bookkeeping, so from 2MHz up the trigger marks the start of the
 *    captured window (the same limitation as the original enhancement).
 */

/*
 * Timer-paced sampling fragments.
 *
 * At 1MHz one sample must fit in 16 cycles, which a C loop cannot hold:
 * each sample only needs a port read, a shape into the sample byte, a
 * store and a wait for the next tick, but everything else - ring
 * bookkeeping, trigger logic, host escape - costs far more than that.
 * So the sampling runs in small assembly fragments (16 samples per call):
 * they only sample, shape and store, wait for each tick, and exit early
 * with the number of samples actually taken as soon as the change
 * notification flag shows a trigger edge (checked per sample in byte
 * mode, per stored word in the packed modes - the total overhead is what
 * has to fit in the tick).  The caller keeps all bookkeeping per
 * fragment, where its cost is amortised over 16 samples.
 *
 * IFS1 lives at 0x0086: CNIF is bit 8 (bit 0 of 0x87), T5IF is bit 12
 * (bit 4 of 0x87), identical on both supported parts.
 */

static unsigned int sump_sample_bytes(uint8_t *dst, unsigned int count) {
  unsigned int taken;

  asm("MOV %0, W8" : : "g"(dst) : "w4", "w5", "w6", "w7", "w8");
  asm("MOV %0, W7" : : "g"(count) : "w6", "w7");
  asm("MOV %0, W6" : : "g"(count) : "w6");
#ifdef BUSPIRATEV4
  asm("MOV #0x02DA, W5"); /* PORTD, see BP_SUMP_PORT_ADDRESS */
#else
  asm("MOV #0x02CA, W5"); /* PORTB, see BP_SUMP_PORT_ADDRESS */
#endif
  asm("sump_1mhz_byte_loop:");
#ifdef BUSPIRATEV4
  asm("MOV.B [W5], W4"); /* RD0..RD7                      */
  asm("AND #0x3F, W4");  /* channels 0-5                  */
  asm("BTSC PORTD, #8"); /* AUX1 is on RD8                */
  asm("BSET W4, #7");    /* ...soft-wired into bit 7      */
#else
  asm("MOV [W5], W4");
  asm("LSR W4, #6, W4");
  asm("AND #0x1F, W4");
#endif
  asm("MOV.B W4, [W8++]");
  asm("sump_1mhz_byte_tick:");
  asm("BTST 0x87, #4"); /* wait for the sampling tick */
  asm("BRA Z, sump_1mhz_byte_tick");
  asm("BCLR 0x87, #4");
  asm("BTSC 0x87, #0"); /* a trigger edge? */
  asm("BRA sump_1mhz_byte_out");
  asm("DEC W7, W7");
  asm("BRA NZ, sump_1mhz_byte_loop");
  asm("sump_1mhz_byte_out:");
  asm("SUB W6, W7, W0"); /* taken = count - remaining */
  asm("MOV W0, %0" : "=r"(taken));
  return taken;
}

static unsigned int sump_sample_words(uint16_t *dst, unsigned int words,
                                      unsigned int per_word,
                                      unsigned int shift, uint8_t mask) {
  unsigned int done;

  asm("MOV %0, W8" : : "g"(dst) : "w0", "w2", "w4", "w5", "w6", "w7",
      "w8", "w9", "w10", "w11", "w12");
  asm("MOV %0, W7" : : "g"(words) : "w7");
  asm("MOV %0, W10" : : "g"(words) : "w10", "w2");
  asm("MOV %0, W12" : : "g"(per_word) : "w12");
  asm("MOV %0, W6" : : "g"(shift) : "w6");
  asm("MOV %0, W9" : : "g"(mask) : "w9");
  asm("MOV W12, W11"); /* samples per word countdown */
  asm("MOV W12, W2");  /* ...and its reload copy */
  asm("CLR W0");
#ifdef BUSPIRATEV4
  asm("MOV #0x02DA, W5"); /* PORTD, see BP_SUMP_PORT_ADDRESS */
#else
  asm("MOV #0x02CA, W5"); /* PORTB, see BP_SUMP_PORT_ADDRESS */
#endif
  asm("sump_1mhz_word_loop:");
#ifdef BUSPIRATEV4
  asm("MOV [W5], W4");
#else
  asm("MOV [W5], W4");
  asm("LSR W4, #6, W4");
#endif
  asm("AND W4, W9, W4");
  asm("SL W0, W6, W0");
  asm("IOR W4, W0, W0");
  asm("sump_1mhz_word_tick:");
  asm("BTST 0x87, #4"); /* wait for the tick */
  asm("BRA Z, sump_1mhz_word_tick");
  asm("BCLR 0x87, #4");
  asm("DEC W11, W11");
  asm("BRA NZ, sump_1mhz_word_loop");
  asm("MOV W0, [W8++]"); /* store the completed word */
  asm("MOV W2, W11");    /* reload the per-word countdown */
  asm("CLR W0");
  asm("BTSC 0x87, #0"); /* a trigger edge? (word granularity) */
  asm("BRA sump_1mhz_word_out");
  asm("DEC W7, W7");
  asm("BRA NZ, sump_1mhz_word_loop");
  asm("sump_1mhz_word_out:");
  asm("SUB W10, W7, %0" : "=r"(done)); /* words done */
  return done;
}

/*
 * The fast sampling loops.  Each samples the port and stores a raw 16-bit
 * word per sample; the timer sampler and the read-out convert those words
 * into host bytes.  The loops unroll in groups of four samples and use DEC
 * + BRA NZ for termination (Z is unambiguous and MOV/NOP do not touch it);
 * SUMP exchanges sample counts in groups of four, so the counts are
 * always a multiple of four.  Cycle counts are noted per instruction.
 */

static void sump_fast_capture_16mhz(uint16_t *buffer, unsigned int samples) {
  /* 16MHz: one sample per instruction cycle. */
  asm("MOV %0, W8" : : "g"(buffer) : "w4", "w8", "w9");
  asm("MOV %0, W9" : : "g"(samples));
#ifdef BUSPIRATEV4
  asm("MOV #0x02DA, W4"); /* PORTD, see BP_SUMP_PORT_ADDRESS */
#else
  asm("MOV #0x02CA, W4"); /* PORTB, see BP_SUMP_PORT_ADDRESS */
#endif
  asm("DEC W9, W9"); /* REPEAT executes one extra time */
  asm("REPEAT W9");
  asm("MOV [W4], [W8++]");
}

static void sump_fast_capture_8mhz(uint16_t *buffer, unsigned int samples) {
  /* 8MHz: two cycles per sample.  Four samples per pass, taken at cycles
     0, 1, 3 and 5 (the documented one-cycle jitter, exact long term). */
  asm("MOV %0, W8" : : "g"(buffer) : "w5", "w7", "w8");
  asm("MOV %0, W7" : : "g"(samples));
#ifdef BUSPIRATEV4
  asm("MOV #0x02DA, W5");
#else
  asm("MOV #0x02CA, W5");
#endif
  asm("LSR W7, W7");
  asm("LSR W7, W7"); /* samples / 4 */
  asm("sump_la_8mhz_loop:");
  asm("MOV [W5], [W8++]");          /* cycle 0: sample */
  asm("MOV [W5], [W8++]");          /* cycle 1: sample */
  asm("NOP");                       /* cycle 2 */
  asm("MOV [W5], [W8++]");          /* cycle 3: sample */
  asm("DEC W7, W7");                /* cycle 4: sets Z */
  asm("MOV [W5], [W8++]");          /* cycle 5: sample */
  asm("BRA NZ, sump_la_8mhz_loop"); /* cycles 6-7 */
}

#ifdef BUSPIRATEV4

static void sump_fast_capture_4mhz(uint16_t *buffer, unsigned int samples) {
  /* 4MHz: four cycles per sample.  Four samples per pass, taken at cycles
     0, 4, 8 and 11 (one cycle of jitter per pass, exact long term). */
  asm("MOV %0, W8" : : "g"(buffer) : "w5", "w7", "w8");
  asm("MOV %0, W7" : : "g"(samples));
  asm("MOV #0x02DA, W5");
  asm("LSR W7, W7");
  asm("LSR W7, W7"); /* samples / 4 */
  asm("sump_la_4mhz_loop:");
  asm("MOV [W5], [W8++]");          /* cycle 0: sample */
  asm("NOP");
  asm("NOP");
  asm("NOP");
  asm("MOV [W5], [W8++]");          /* cycle 4: sample */
  asm("NOP");
  asm("NOP");
  asm("NOP");
  asm("MOV [W5], [W8++]");          /* cycle 8: sample */
  asm("NOP");
  asm("NOP");
  asm("MOV [W5], [W8++]");          /* cycle 11: sample */
  asm("NOP");
  asm("DEC W7, W7");                /* cycle 13: sets Z */
  asm("BRA NZ, sump_la_4mhz_loop"); /* cycles 14-15 */
}

static void sump_fast_capture_2mhz(uint16_t *buffer, unsigned int samples) {
  /* 2MHz: eight cycles per sample, one sample per pass. */
  asm("MOV %0, W8" : : "g"(buffer) : "w5", "w7", "w8");
  asm("MOV %0, W7" : : "g"(samples));
  asm("MOV #0x02DA, W5");
  asm("sump_la_2mhz_loop:");
  asm("MOV [W5], [W8++]");          /* cycle 0: sample */
  asm("NOP");
  asm("NOP");
  asm("NOP");
  asm("DEC W7, W7");                /* cycle 4: sets Z */
  asm("NOP");
  asm("BRA NZ, sump_la_2mhz_loop"); /* cycles 6-7 */
}

#else

static void sump_fast_capture_4mhz(uint8_t *buffer, unsigned int samples) {
  /* 4MHz: four cycles per sample, one byte per sample, pre-shifted into
     its five channel bits.  Four samples per pass, taken at cycles 0, 3,
     7 and 10 (the documented one-cycle jitter, exact long term). */
  asm("MOV %0, W8" : : "g"(buffer) : "w4", "w5", "w7", "w8");
  asm("MOV %0, W7" : : "g"(samples));
  asm("MOV #0x02CA, W5");
  asm("LSR W7, W7");
  asm("LSR W7, W7"); /* samples / 4 */
  asm("sump_la_4mhz_loop:");
  asm("MOV [W5], W4");              /* cycle 0: sample */
  asm("LSR W4, #6, W4");            /* cycle 1 */
  asm("MOV.B W4, [W8++]");          /* cycle 2 */
  asm("MOV [W5], W4");              /* cycle 3: sample */
  asm("LSR W4, #6, W4");            /* cycle 4 */
  asm("MOV.B W4, [W8++]");          /* cycle 5 */
  asm("NOP");                       /* cycle 6 */
  asm("MOV [W5], W4");              /* cycle 7: sample */
  asm("LSR W4, #6, W4");            /* cycle 8 */
  asm("MOV.B W4, [W8++]");          /* cycle 9 */
  asm("MOV [W5], W4");              /* cycle 10: sample */
  asm("LSR W4, #6, W4");            /* cycle 11 */
  asm("MOV.B W4, [W8++]");          /* cycle 12 */
  asm("DEC W7, W7");                /* cycle 13: sets Z */
  asm("BRA NZ, sump_la_4mhz_loop"); /* cycles 14-15 */
}

static void sump_fast_capture_2mhz(uint8_t *buffer, unsigned int samples) {
  /* 2MHz: eight cycles per sample, one sample per pass. */
  asm("MOV %0, W8" : : "g"(buffer) : "w4", "w5", "w7", "w8");
  asm("MOV %0, W7" : : "g"(samples));
  asm("MOV #0x02CA, W5");
  asm("sump_la_2mhz_loop:");
  asm("MOV [W5], W4");              /* cycle 0: sample */
  asm("LSR W4, #6, W4");            /* cycle 1 */
  asm("MOV.B W4, [W8++]");          /* cycle 2 */
  asm("NOP");                       /* cycle 3 */
  asm("NOP");                       /* cycle 4 */
  asm("DEC W7, W7");                /* cycle 5: sets Z */
  asm("BRA NZ, sump_la_2mhz_loop"); /* cycles 6-7 */
}

#endif /* BUSPIRATEV4 */

/**
 * Waits for the change-notification trigger at the fast rates.
 *
 * The fast loops cannot buffer ahead of the trigger, so the window always
 * starts at the trigger event.  A byte arriving from the host during the
 * wait aborts the capture; it is left in place for the command loop.
 *
 * @return true when the trigger fired, false when the host aborted.
 */
static bool sump_wait_for_fast_trigger(void) {
  unsigned int poll = 0;

  IFS1bits.CNIF = OFF;

  while (!IFS1bits.CNIF) {
    /* Do not poll the host on every pass: that would spin the wait. */
    if ((++poll & 0x0FFF) == 0 && user_serial_ready_to_read()) {
      return false;
    }
  }

  return true;
}

/**
 * Sends the captured window to the host, newest sample first.
 *
 * The host is sent exactly `requested` sample bytes.  Samples the rate or
 * the buffer could not supply are padding, sent first because the stream
 * runs backwards in time; the padding alternates 0x55/0xAA.  Samples that
 * the rolling ring has already overwritten (it can happen right at the
 * oldest edge of a full-size window) carry no data bits, only the
 * alternating pattern in the unrecorded channels.
 */
static void sump_transmit_fast(unsigned int samples, unsigned int requested,
                               uint8_t mask, bool word_samples) {
  uint8_t pattern = 0x55;
  unsigned int index;

  for (index = samples; index < requested; index++) {
    user_serial_transmit_character(pattern);
    pattern ^= 0xFF;
  }

  if (word_samples) {
    const uint16_t *buffer =
        (const uint16_t *)bus_pirate_configuration.terminal_input;

    for (index = samples; index > 0; index--) {
      uint8_t value = BP_SUMP_SAMPLE_TRANSFORM(buffer[index - 1]) & mask;

      user_serial_transmit_character((pattern & ~mask) | value);
      pattern ^= 0xFF;
    }
  } else {
    const uint8_t *buffer = bus_pirate_configuration.terminal_input;

    for (index = samples; index > 0; index--) {
      user_serial_transmit_character((pattern & ~mask) |
                                     (buffer[index - 1] & mask));
      pattern ^= 0xFF;
    }
  }
}

/**
 * Sends a byte-mode window captured by the timer-driven sampler (one
 * stored byte per sample), newest sample first.  See
 * sump_transmit_fast() for the padding rules.
 */
static void sump_transmit_bytes(unsigned int samples, unsigned int requested,
                                uint32_t end_index, uint32_t written,
                                uint8_t mask) {
  const uint8_t *buffer = bus_pirate_configuration.terminal_input;
  uint32_t floor = (written > 4096u) ? (written - 4096u) : 0u;
  uint32_t last = end_index - 1u; /* the newest sample of the window */
  unsigned int slot = (unsigned int)(last % 4096u);
  uint8_t pattern = 0x55;
  unsigned int index;

  for (index = samples; index < requested; index++) {
    user_serial_transmit_character(pattern);
    pattern ^= 0xFF;
  }

  for (index = 0; index < samples; index++) {
    uint32_t absolute = last - index;

    if (absolute < floor) {
      /* The oldest part of the window was overwritten in the ring. */
      user_serial_transmit_character(pattern & ~mask);
    } else {
      user_serial_transmit_character((pattern & ~mask) |
                                     (buffer[slot] & mask));
    }

    pattern ^= 0xFF;
    slot = (slot == 0) ? 4095 : slot - 1;
  }
}

/**
 * Sends the packed window captured by the timer-driven sampler, newest
 * sample first.  See sump_transmit_fast() for the padding rules.
 *
 * @param samples how many real samples the window holds.
 * @param requested how many sample bytes the host asked for.
 * @param end_index one past the newest sample of the window.  The capture
 *        may have overshot it by up to one storage word (bookkeeping runs
 *        per word), so only samples before it are sent.
 * @param written total number of samples taken; it bounds what the ring
 *        can still hold.
 * @param per_word samples packed into each 16-bit word.
 * @param shift bit shift between consecutive samples in a word.
 * @param mask channel bits recorded in each sample.
 */
static void sump_transmit_packed(unsigned int samples, unsigned int requested,
                                 uint32_t end_index, uint32_t written,
                                 unsigned int per_word, unsigned char shift,
                                 uint8_t mask) {
  const uint16_t *buffer =
      (const uint16_t *)bus_pirate_configuration.terminal_input;
  uint32_t words = (written + per_word - 1) / per_word;
  uint32_t floor = (words > 2048u) ? (words - 2048u) * per_word : 0u;
  uint32_t last = end_index - 1u; /* the newest sample of the window */
  unsigned int slot = (unsigned int)((last / per_word) % 2048u);
  /* The newest sample of a word is the one packed last, i.e. at the low
     bits; up to a word of overshoot may sit below it. */
  unsigned int bitpos = (per_word - 1u - (unsigned int)(last % per_word)) * shift;
  uint16_t packed = buffer[slot];
  uint8_t pattern = 0x55;
  unsigned int index;

  for (index = samples; index < requested; index++) {
    user_serial_transmit_character(pattern);
    pattern ^= 0xFF;
  }

  for (index = 0; index < samples; index++) {
    uint32_t absolute = last - index;

    if (absolute < floor) {
      /* The oldest part of the window was overwritten in the ring. */
      user_serial_transmit_character(pattern & ~mask);
    } else {
      user_serial_transmit_character((pattern & ~mask) |
                                     ((packed >> bitpos) & mask));
    }

    pattern ^= 0xFF;

    /* Step one sample back in time: within a word the older sample sits
       `shift` bits higher, then the previous word. */
    bitpos += shift;
    if (bitpos >= per_word * shift) {
      bitpos = 0;
      slot = (slot == 0) ? 2047 : slot - 1;
      packed = buffer[slot];
    }
  }
}

bool sump_acquire_samples(void) {
  if (sampler_state != SAMPLER_ARMED) {
    /* Nothing to do. */
    return false;
  }

  /* Configuration for this capture.  Fast rates sample the full channel
     set straight off the port; slower rates pack samples into 16-bit
     words and may record fewer channels to stretch the buffer. */
  unsigned int samples = samples_to_acquire;
  unsigned int requested = samples_requested;
  uint8_t mask = sump_channel_mask;

  if (sump_go_fast) {
    uint16_t *words = (uint16_t *)bus_pirate_configuration.terminal_input;
    unsigned int capacity;

#ifdef BUSPIRATEV4
    capacity = 2048; /* one 16-bit word per sample */
#else
    /* 2/4MHz store one byte per sample; 8/16MHz one word. */
    capacity = (sump_go_fast >= 3) ? 2048 : 4096;
#endif
    if (samples > capacity) {
      samples = capacity;
    }

    /* Wait for the trigger, if one is configured; the fast rates cannot
       record anything before it. */
    if (trigger_armed && !sump_wait_for_fast_trigger()) {
      /* The host aborted; reset and let the command loop handle it. */
      sump_reset();
      return true;
    }

    mask = BP_SUMP_SAMPLE_MASK;

    switch (sump_go_fast) {
    case 4:
      sump_fast_capture_16mhz(words, samples);
      break;
    case 3:
      sump_fast_capture_8mhz(words, samples);
      break;
    case 2:
#ifdef BUSPIRATEV4
      sump_fast_capture_4mhz(words, samples);
#else
      sump_fast_capture_4mhz((uint8_t *)words, samples);
#endif
      break;
    default:
#ifdef BUSPIRATEV4
      sump_fast_capture_2mhz(words, samples);
#else
      sump_fast_capture_2mhz((uint8_t *)words, samples);
#endif
      break;
    }

#ifdef BUSPIRATEV4
    sump_transmit_fast(samples, requested, mask, true);
#else
    sump_transmit_fast(samples, requested, mask, sump_go_fast >= 3);
#endif
  } else {
    /* Timer-driven capture, 1MHz and below.  Base captures store one byte
       per sample; larger captures (fewer channels) pack several samples
       into every 16-bit word.  The buffer doubles as a rolling ring so
       the window can hold samples from before the trigger; the read-out
       walks it backwards from the newest sample. */
    uint8_t *bytes = bus_pirate_configuration.terminal_input;
    uint16_t *buffer = (uint16_t *)bus_pirate_configuration.terminal_input;
    unsigned int per_word = sump_samples_per_word; /* 1 = one byte per sample */
    unsigned char shift = sump_pack_shift;
    unsigned int delay = trigger_delay_samples;
    unsigned int pre_trigger;
    unsigned int capacity = (per_word == 1) ? 4096u : 2048u;
    uint32_t written = 0;    /* samples taken so far */
    uint32_t trigger_at = 0; /* sample count when the trigger fired */
    uint32_t end_index = 0;  /* one past the newest sample of the window */
    unsigned int quiet_samples = 0; /* samples since the last host check */
    unsigned int slot = 0;   /* next ring position: byte or word */
    bool armed = false;
    bool triggered = false;

    /* The capture window must fit in the ring; SUMP_CNT sizes the
       channel count for the requested samples, so this is a safety net. */
    if (samples > capacity * per_word) {
      samples = capacity * per_word;
    }
    if (delay > samples) {
      delay = samples;
    }
    pre_trigger = samples - delay;

    /* Start the sampling timer. */
    T4CONbits.TON = ON;
    IFS1bits.T5IF = OFF;

    for (;;) {
      unsigned int chunk = 16;

      /* Decide how many samples this fragment may take: bounded by the
         requested window, rounded up to whole words where samples are
         packed (the readout trims the overshoot), and clamped to the
         ring.  While waiting for a trigger there is no bound. */
      if (!trigger_armed) {
        if (chunk > samples - written) {
          chunk = (unsigned int)(samples - written);
        }
      } else if (triggered) {
        if (chunk > trigger_at + delay - written) {
          chunk = (unsigned int)(trigger_at + delay - written);
        }
      }

      if (per_word == 1) {
        if (chunk > capacity - slot) {
          chunk = capacity - slot;
        }
        if (chunk) {
          unsigned int taken = sump_sample_bytes(bytes + slot, chunk);

          slot += taken;
          if (slot >= capacity) {
            slot = 0;
          }
          written += taken;
        }
      } else {
        chunk = ((chunk + per_word - 1) / per_word) * per_word;
        if (chunk > (capacity - slot) * per_word) {
          chunk = (capacity - slot) * per_word;
        }
        if (chunk) {
          unsigned int words_done = sump_sample_words(
              buffer + slot, chunk / per_word, per_word, shift, mask);

          slot += words_done;
          if (slot >= capacity) {
            slot = 0;
          }
          written += words_done * per_word;
        }
      }

      /* Trigger bookkeeping: buffer the pre-trigger part first, then arm
         on a clean state; a trigger before that point is ignored, as the
         window cannot hold what came before it. */
      if (trigger_armed) {
        if (!armed) {
          if (written >= pre_trigger) {
            armed = true;
            IFS1bits.CNIF = OFF;
          } else if (IFS1bits.CNIF) {
            /* The fragment stopped for an early edge; discard it. */
            IFS1bits.CNIF = OFF;
          }
        } else if (!triggered && IFS1bits.CNIF) {
          triggered = true;
          trigger_at = written;
          IFS1bits.CNIF = OFF;
        }
      }

      /* Stop once the requested window is covered; the readout trims up
         to one fragment of overshoot. */
      if (trigger_armed) {
        if (triggered && written >= trigger_at + delay) {
          end_index = trigger_at + delay;
          break;
        }
      } else if (written >= samples) {
        end_index = samples;
        break;
      }

      /* Host escape, checked at rate-dependent intervals so the fast end
         stays fast and the slow end stays responsive.  Any host byte
         aborts the capture; it stays in place for the command loop. */
      quiet_samples += chunk;
      if ((quiet_samples & sump_host_check_mask) == 0 &&
          user_serial_ready_to_read()) {
        T4CON = OFF;
        sump_reset();
        return true;
      }
    }

    /* Stop sampling.  The readout walks to the exact end index, so up to
       one fragment of samples past the window end is simply not read
       back. */
    T4CON = OFF;

    if (per_word == 1) {
      sump_transmit_bytes(samples, requested, end_index, written, mask);
    } else {
      sump_transmit_packed(samples, requested, end_index, written, per_word,
                           shift, mask);
    }
  }

  /* Reset the analyzer state. */
  sump_reset();

  /* Acquisition complete. */
  return true;
}

#endif /* BP_ENABLE_SUMP_SUPPORT */
