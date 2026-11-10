/*
 * This file is part of the Bus Pirate project
 * (https://github.com/BusPirate/Bus_Pirate/).
 *
 * Written and maintained by the Bus Pirate project.
 *
 * To the extent possible under law, the project has waived all copyright and
 * related or neighboring rights to Bus Pirate. This work is published from
 * United States.
 *
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#include "spi.h"

#ifdef BP_ENABLE_SPI_SUPPORT

#include "base.h"
#include "binary_io.h"
#include "core.h"
#include "proc_menu.h"

#ifdef BUSPIRATEV4
#include "dp_usb/cdc.h"
#endif

/* Pin assignments. */

#define SPIMOSI_TRIS BP_MOSI_DIR
#define SPICLK_TRIS BP_CLK_DIR
#define SPIMISO_TRIS BP_MISO_DIR
#define SPICS_TRIS BP_CS_DIR
#define SPIMOSI BP_MOSI
#define SPICLK BP_CLK
#define SPIMISO BP_MISO
#define SPICS BP_CS
#define SPIMOSI_ODC BP_MOSI_ODC
#define SPICLK_ODC BP_CLK_ODC
#define SPICS_ODC BP_CS_ODC
#define SPICS_RPIN BP_CS_RPIN

extern mode_configuration_t mode_configuration;
extern command_t last_command;
extern bus_pirate_configuration_t bus_pirate_configuration;

typedef enum {
  SPI_COMMAND_BASE = 0,
  SPI_COMMAND_READ_DATA,
  SPI_COMMAND_CONFIGURE_PERIPHERALS = 4,
  SPI_COMMAND_SET_PULLUPS,
  SPI_COMMAND_SET_SPEED,
  SPI_COMMAND_CONFIGURE_SPI = 8
} spi_command_t;

typedef enum {
  SPI_BASE_COMMAND_EXIT = 0,
  SPI_BASE_COMMAND_SEND_IDENTIFIER,
  SPI_BASE_COMMAND_CS_LOW,
  SPI_BASE_COMMAND_CS_HIGH,
  SPI_BASE_COMMAND_WRITE_AND_READ_WITH_CS,
  SPI_BASE_COMMAND_WRITE_AND_READ_WITHOUT_CS,
  SPI_BASE_COMMAND_EXTENDED_AVR_COMMAND,
  SPI_BASE_COMMAND_SNIFF_ALL_TRAFFIC = 13,
  SPI_BASE_COMMAND_SNIFF_WHEN_CS_LOW
} spi_base_command_t;

typedef enum {
  SPI_MACRO_MENU = 0,
  SPI_MACRO_SNIFF_ON_CS_LOW,
  SPI_MACRO_SNIFF_ALL_TRAFFIC,
  SPI_MACRO_CLOCK_IDLE_LOW = 10,
  SPI_MACRO_CLOCK_IDLE_HIGH,
  SPI_MACRO_EDGE_IDLE_TO_ACTIVE,
  SPI_MACRO_EDGE_ACTIVE_TO_IDLE,
  SPI_MACRO_SAMPLING_ON_MIDDLE,
  SPI_MACRO_SAMPLING_ON_END
} spi_macro_identifier_t;

/**
 * Sniff data from the SPI bus only when the CS line goes low.
 */
#define SPI_SNIFF_ON_CS_LOW false

/**
 * Sniff data from the SPI bus all the time.
 */
#define SPI_SNIFF_ALWAYS true

/**
 * SPI bit read happens in the middle of a bit write.
 */
#define SPI_SAMPLING_ON_DATA_OUTPUT_MIDDLE 0

/**
 * SPI bit read happens after a bit write.
 */
#define SPI_SAMPLING_ON_DATA_OUTPUT_END 1

/**
 * SPI bus clock line will idle when low.
 */
#define SPI_CLOCK_IDLE_LOW 0

/**
 * SPI bus clock line will idle when high.
 */
#define SPI_CLOCK_IDLE_HIGH 1

/**
 * SPI data reads and writes happen when the clock goes from idle to active.
 */
#define SPI_TRANSITION_FROM_IDLE_TO_ACTIVE 0

/**
 * SPI data reads and writes happen when the clock goes from active to idle.
 */
#define SPI_TRANSITION_FROM_ACTIVE_TO_IDLE 1

/**
 * Set up the SPI interfaces to operate in slave mode.
 */
static void spi_slave_enable(void);

/**
 * Gets the SPI interfaces out of slave mode.
 */
static void spi_slave_disable(void);

/**
 * Sniffs data coming through the SPI bus.
 *
 * @param[in] trigger       flag indicating what triggers data sniffing.
 * @param[in] terminal_mode whether data is meant to be seen by the user via the
 *                          serial port interface.
 *
 * @see SPI_SNIFF_ON_CS_LOW
 * @see SPI_SNIFF_ALWAYS
 */
static void spi_sniffer(bool trigger, bool terminal_mode);

/**
 * Engages the CS line.
 *
 * @param[in] write_with_read flag indicating if data writes will have a
 *                            subsequent read operation or not.
 */
static void engage_spi_cs(bool write_with_read);

/**
 * Performs the bulk of the I/O work for binary-mode commands involving SPI
 * reads and writes.
 *
 * @param[in] engage_cs flag indicating whether CS must be held down when
 * performing an SPI write operation.
 */
static uint8_t spi_read_byte(void);
#ifdef BUSPIRATEV4
static void spi_read_bytes(uint8_t* data, uint16_t length);
#endif
static void spi_read_write_io(const bool engage_cs);

/**
 * Writes to the SPI bus the data read from the user-facing serial port
 *  or usb interface.
 *
 * @param[in] bytes_to_write how many bytes should be read from host
 * and written to the SPI bus.
 */
static void spi_write_from_host(const size_t bytes_to_write);

/**
 * Writes to the host the data read from the SPI bus.
 *
 * @param[in] bytes_to_read how many bytes should be read from the SPI bus
 * and written to the user-facing serial port or usb interface.
 */
static void spi_read_to_host(const size_t bytes_to_read);

#ifdef BP_SPI_ENABLE_AVR_EXTENDED_COMMANDS

/**
 * Extended AVR Binary I/O command for no operations.
 */
#define BINARY_IO_SPI_AVR_COMMAND_NOOP 0

/**
 * Extended AVR Binary I/O command for obtaining the protocol version.
 */
#define BINARY_IO_SPI_AVR_COMMAND_VERSION 1

/**
 * Extended AVR Binary I/O command for performing a bulk read.
 */
#define BINARY_IO_SPI_AVR_COMMAND_BULK_READ 2

/**
 * Extended AVR Binary I/O protocol version.
 */
#define BINARY_IO_SPI_AVR_SUPPORT_VERSION 0x0001

/**
 * AVR Fetch data command low byte.
 */
#define AVR_FETCH_LOW_BYTE_COMMAND 0x20

/**
 * AVR Fetch data command high byte.
 */
#define AVR_FETCH_HIGH_BYTE_COMMAND 0x28

/**
 * Handle an incoming extended binary I/O AVR SPI command.
 */
static void handle_extended_avr_command(void);

#endif /* BP_SPI_ENABLE_AVR_EXTENDED_COMMANDS */

/**
 * SPI protocol state structure.
 */
typedef struct {

  /**
   * Clock polarity.
   *
   * @see SPI_CLOCK_IDLE_LOW
   * @see SPI_CLOCK_IDLE_HIGH
   */
  uint8_t clock_polarity : 1;

  /**
   * Clock edge.
   *
   * @see SPI_TRANSITION_FROM_IDLE_TO_ACTIVE
   * @see SPI_TRANSITION_FROM_ACTIVE_TO_IDLE
   */
  uint8_t clock_edge : 1;

  /**
   * Data sample timing.
   *
   * @see SPI_SAMPLING_ON_DATA_OUTPUT_MIDDLE
   * @see SPI_SAMPLING_ON_DATA_OUTPUT_END
   */
  uint8_t data_sample_timing : 1;

  /** CS line state. */
  uint8_t cs_line_state : 1;

} spi_state_t;

/**
 * The SPI protocol state.
 */
static spi_state_t spi_state = {0};

/**
 * Available SPI bus speeds.
 */
static const uint8_t spi_bus_speed[] = {
    0b00000000, /*  31 kHz - Primary prescaler 64:1 / Secondary prescaler 8:1 */
    0b00011000, /* 125 kHz - Primary prescaler 64:1 / Secondary prescaler 2:1 */
    0b00011100, /* 250 kHz - Primary prescaler 64:1 / Secondary prescaler 1:1 */
    0b00011101, /*   1 MHz - Primary prescaler 16:1 / Secondary prescaler 1:1 */
    0b00011010, /*   2 MHz - Primary prescaler  4:1 / Secondary prescaler 2:1 */
    0b00001011, /* 2.6 MHz - Primary prescaler  1:1 / Secondary prescaler 6:1 */
    0b00011110, /*   4 MHz - Primary prescaler  4:1 / Secondary prescaler 1:1 */
    0b00011011, /*   8 MHz - Primary prescaler  1:1 / Secondary prescaler 2:1 */
    0b00001100, /*  50 kHz - Primary prescaler 64:1 / Secondary prescaler 5:1 */
    0b00010110, /* 1.3 MHz - Primary prescaler  4:1 / Secondary prescaler 3:1 */
    0b00001111, /* 3.2 MHz - Primary prescaler  1:1 / Secondary prescaler 5:1 */
    0b00010111  /* 5.3 MHz - Primary prescaler  1:1 / Secondary prescaler 3:1 */
};

void engage_spi_cs(bool write_with_read) {
  mode_configuration.write_with_read = write_with_read;
  SPICS = !spi_state.cs_line_state;
  if (spi_state.cs_line_state) {
    user_serial_transmit_character('/');
  }
  MSG_SPI_CS_ENABLED;
}

inline void spi_start_with_read(void) { engage_spi_cs(ON); }

inline void spi_start(void) { engage_spi_cs(OFF); }

void spi_stop(void) {
  SPICS = spi_state.cs_line_state;
  if (spi_state.cs_line_state) {
    user_serial_transmit_character('/');
  }
  MSG_SPI_CS_DISABLED;
}

inline uint16_t spi_read(void) { return spi_read_byte(); }

uint16_t spi_write(const uint16_t value) {
  uint8_t data = spi_write_byte(value);
  return mode_configuration.write_with_read ? data : 0;
}

void spi_print_settings(void) {
  MSG_SPI_MODE_HEADER_START;
  bp_write_dec_byte(mode_configuration.speed + 1);
  bpSP;
  bp_write_dec_byte(spi_state.clock_polarity);
  bpSP;
  bp_write_dec_byte(spi_state.clock_edge);
  bpSP;
  bp_write_dec_byte(spi_state.data_sample_timing);
  bpSP;
  bp_write_dec_byte(spi_state.cs_line_state);
  bpSP;
  bp_write_dec_byte(mode_configuration.high_impedance);
  MSG_MODE_HEADER_END;
}

void spi_setup_prepare(void) {
  bool user_prompt;
  int spi_speed;
  int spi_clock_polarity;
  int spi_clock_edge;
  int spi_data_sampling;
  int spi_enabled;
  int spi_cs_line_state;

  consumewhitechars();
  spi_speed = getint();
  consumewhitechars();
  spi_clock_polarity = getint();
  consumewhitechars();
  spi_clock_edge = getint();
  consumewhitechars();
  spi_data_sampling = getint();
  consumewhitechars();
  spi_cs_line_state = getint();
  consumewhitechars();
  spi_enabled = getint();

  user_prompt = !(((spi_speed > 0) && (spi_speed <= 4)) &&
                  ((spi_clock_polarity > 0) && (spi_clock_polarity <= 2)) &&
                  ((spi_clock_edge > 0) && (spi_clock_edge <= 2)) &&
                  ((spi_data_sampling > 0) && (spi_data_sampling <= 2)) &&
                  ((spi_cs_line_state > 0) && (spi_cs_line_state <= 2)) &&
                  ((spi_enabled > 0) && (spi_enabled <= 2)));

  if (user_prompt) {
    mode_configuration.command_error = NO;

    MSG_SPI_SPEED_PROMPT;
    mode_configuration.speed = getnumber(1, 1, 12, 0) - 1;

    MSG_SPI_POLARITY_PROMPT;
    spi_state.clock_polarity = getnumber(1, 1, 2, 0) - 1;

    MSG_SPI_EDGE_PROMPT;
    spi_state.clock_edge = getnumber(2, 1, 2, 0) - 1;

    MSG_SPI_SAMPLE_PROMPT;
    spi_state.data_sample_timing = getnumber(1, 1, 2, 0) - 1;

    MSG_SPI_CS_MODE_PROMPT;
    spi_state.cs_line_state = getnumber(2, 1, 2, 0) - 1;

    MSG_PIN_OUTPUT_TYPE_PROMPT;
    mode_configuration.high_impedance = ~(getnumber(1, 1, 2, 0) - 1);
  } else {
    mode_configuration.speed = spi_speed - 1;
    spi_state.clock_polarity = spi_clock_polarity - 1;
    spi_state.clock_edge = spi_clock_edge - 1;
    spi_state.data_sample_timing = spi_data_sampling - 1;
    spi_state.cs_line_state = (spi_cs_line_state - 1);
    mode_configuration.high_impedance = ~(spi_enabled - 1);
    spi_print_settings();
  }

  mode_configuration.write_with_read = OFF;
}

void spi_setup_execute(void) {
  /* Setup speed. */
  spi_setup(spi_bus_speed[mode_configuration.speed]);

  /* Setup CS line state. */
  SPICS = spi_state.cs_line_state;
}

inline void spi_cleanup(void) { spi_disable_interface(); }

void spi_run_macro(const uint16_t macro) {

  switch (macro) {
  case SPI_MACRO_MENU:
    MSG_SPI_MACRO_MENU;
    break;

  case SPI_MACRO_SNIFF_ON_CS_LOW:
    MSG_SNIFFER_MESSAGE;
    MSG_ANY_KEY_TO_EXIT_PROMPT;
    spi_sniffer(SPI_SNIFF_ON_CS_LOW, true);
    break;

  case SPI_MACRO_SNIFF_ALL_TRAFFIC:
    MSG_SNIFFER_MESSAGE;
    MSG_ANY_KEY_TO_EXIT_PROMPT;
    spi_sniffer(SPI_SNIFF_ALWAYS, true);
    break;

  case SPI_MACRO_CLOCK_IDLE_LOW:
    spi_state.clock_polarity = SPI_CLOCK_IDLE_LOW;
    goto cleanup;

  case SPI_MACRO_CLOCK_IDLE_HIGH:
    spi_state.clock_polarity = SPI_CLOCK_IDLE_HIGH;
    goto cleanup;

  case SPI_MACRO_EDGE_IDLE_TO_ACTIVE:
    spi_state.clock_edge = SPI_TRANSITION_FROM_IDLE_TO_ACTIVE;
    goto cleanup;

  case SPI_MACRO_EDGE_ACTIVE_TO_IDLE:
    spi_state.clock_edge = SPI_TRANSITION_FROM_ACTIVE_TO_IDLE;
    goto cleanup;

  case SPI_MACRO_SAMPLING_ON_MIDDLE:
    spi_state.data_sample_timing = SPI_SAMPLING_ON_DATA_OUTPUT_MIDDLE;
    goto cleanup;

  case SPI_MACRO_SAMPLING_ON_END:
    spi_state.data_sample_timing = SPI_SAMPLING_ON_DATA_OUTPUT_END;

  cleanup:
    SPI1CON1bits.CKP = spi_state.clock_polarity;
    SPI1CON1bits.CKE = spi_state.clock_edge;
    SPI1CON1bits.SMP = spi_state.data_sample_timing;
    spi_print_settings();
    break;

  default:
    MSG_UNKNOWN_MACRO_ERROR;
    break;
  }
}

void spi_print_pins_state(void) { MSG_SPI_PINS_STATE; }

void spi_setup(const uint8_t spi_speed) {
  /* Disable interface. */
  SPI1STATbits.SPIEN = OFF;

  /* Set SPI to open drain if high impedance is on. */
  if (mode_configuration.high_impedance == ON) {
    SPIMOSI_ODC = OPEN_DRAIN;
    SPICLK_ODC = OPEN_DRAIN;
    SPICS_ODC = OPEN_DRAIN;
  } else {
    SPIMOSI_ODC = PUSH_PULL;
    SPICLK_ODC = PUSH_PULL;
    SPICS_ODC = PUSH_PULL;
  }

  /* Assign pins routing. */

  RPINR20bits.SDI1R = BP_MISO_RPIN;
  BP_MOSI_RPOUT = SDO1_IO;
  BP_CLK_RPOUT = SCK1OUT_IO;
  SPICS = HIGH;
  SPICS_TRIS = OUTPUT;
  SPICLK_TRIS = OUTPUT;
  SPIMISO_TRIS = INPUT;
  SPIMOSI_TRIS = OUTPUT;

  /*
   * SPI1CON1 - SPI1 CONTROL REGISTER 1
   *
   * MSB
   * ---000xx0x1xxxxx
   *    |||||||||||||
   *    |||||||||||++--- PPRE:   Primary prescale bits.
   *    ||||||||+++----- SPRE:   Secondary prescale bits.
   *    |||||||+-------- MSTEN:  Master mode.
   *    ||||||+--------- CKP:    Clock idle LOW.
   *    |||||+---------- SSEN:   Pin controlled by port function.
   *    ||||+----------- CKE:    Transition happens from idle to active.
   *    |||+------------ SMP:    Flag indicating when the data is sampled.
   *    ||+------------- MODE16: Communication is byte-wide.
   *    |+-------------- DISSDO: SDO1 pin is controlled by the module.
   *    +--------------- DISSCK: Internal SPI clock is enabled.
   */
  SPI1CON1 =
      (MASKBOTTOM8(spi_speed, 5) << _SPI1CON1_PPRE_POSITION) |
      (ON << _SPI1CON1_MSTEN_POSITION) |
      (MASKBOTTOM8(spi_state.clock_polarity, 1) << _SPI1CON1_CKP_POSITION) |
      (MASKBOTTOM8(spi_state.clock_edge, 1) << _SPI1CON1_CKE_POSITION) |
      (MASKBOTTOM8(spi_state.data_sample_timing, 1) << _SPI1CON1_SMP_POSITION);

  /*
   * SPI1CON2 - SPI1 CONTROL REGISTER 2
   *
   * MSB
   * 000-----------01
   * |||           ||
   * |||           |+--- SPIBEN: Enhanced Buffer Enable bit
   * |||           +---- FRMDLY: Frame sync pulse precedes first bit clock.
   * ||+---------------- FRMPOL: Frame sync pulse is active low.
   * |+----------------- SPIFSD: Frame sync pulse output.
   * +------------------ FRMEN:  Framed SPI1 support disabled.
   */

  // this clears the SPIROV bit
  SPI1STAT = 0;

  // enable enhanced mode
  SPI1CON2 = 0x0001;

  /*
   * SPI1STAT - SPI1 STATUS REGISTER
   *
   * MSB
   * 1-0------0----??
   * | |      |
   * | |      +--------- SPIROV:  Overflow flag cleared.
   * | +---------------- SPISIDL: Continue module operation in idle mode.
   * +------------------ SPIEN:   Module enabled.
   */
  SPI1STAT = _SPI1STAT_SPIEN_MASK;
}

void spi_disable_interface(void) {

  bp_disable_usb_led();

    /* Disable interface. */
  SPI1STATbits.SPIEN = OFF;

  /* Deassign pins. */
  RPINR20bits.SDI1R = 0b11111;
  BP_MOSI_RPOUT = 0b00000;
  BP_CLK_RPOUT = 0b00000;

  /* Disable open drain. */
  SPIMOSI_ODC = PUSH_PULL;
  SPICLK_ODC = PUSH_PULL;
  SPICS_ODC = PUSH_PULL;
}

uint8_t spi_write_byte(const uint8_t value)
{
    while (SPI1STATbits.SPIBEC2 || (!SPI1STATbits.SRXMPT));

    /* Put the value on the bus. */
    SPI1BUF = value;

    while (SPI1STATbits.SRXMPT);

    /* Get the byte read from the bus. */
    uint8_t result = SPI1BUF;

    return result;
}

static inline uint8_t spi_read_byte(void)
{
    while (SPI1STATbits.SPIBEC2 || (!SPI1STATbits.SRXMPT));

    /* Put the value on the bus. */
    SPI1BUF = 0xff;

    while (SPI1STATbits.SRXMPT);

    /* Get the byte read from the bus. */
    uint8_t result = SPI1BUF;

    return result;
}

#ifdef BUSPIRATEV3
static void spi_read_bytes_to_uart(uint16_t length)
{
#if 1
    uint16_t sent = length;
    uint16_t rec = length;

    while(sent || rec)
    {
        // PIC24FJ64GA004 Family Errata suggests not to use SPITBF
        if( (SPI1STATbits.SPIBEC < 7) && SPI1STATbits.SRXMPT && sent ) {
            SPI1BUF = 0x00;
            sent--;
        }

        if( (!SPI1STATbits.SRXMPT) && (!U1STAbits.UTXBF) && rec ) {
            U1TXREG = SPI1BUF;
            rec--;
        }
    }
#else
  /* Writes data to the serial port as soon as read from the SPI bus. */

    while (length--)
    {
        while (SPI1STATbits.SPITBF);

        /* Put the value on the bus. */
        SPI1BUF = 0xff;

        while (U1STAbits.UTXBF || SPI1STATbits.SRXMPT);

        /* read from SPI BUS and write to UART */
        U1TXREG = SPI1BUF;
    }
#endif
}

static void spi_write_bytes_from_uart(uint16_t length)
{
    uint16_t sent = length;
    uint16_t rec = length;

    while(sent || rec)
    {
        // PIC24FJ64GA004 Family Errata suggests not to use SPITBF
        if( (SPI1STATbits.SPIBEC < 7) && U1STAbits.URXDA && sent ) {
            SPI1BUF = LO8(U1RXREG);
            sent--;
        }

        if( !SPI1STATbits.SRXMPT && rec ) {
            SPI1BUF;
            rec--;
        }
    }
}
#endif

#ifdef BUSPIRATEV4

static void spi_read_bytes(uint8_t* data, uint16_t length)
{
    uint16_t cnt = length;
    uint16_t rec = 0;

    while(cnt) {
        if(!(SPI1STAT & (_SPI1STAT_SPIBEC2_MASK|_SPI1STAT_SPIRBF_MASK)))
            SPI1BUF = 0x00, cnt--;

        if(!SPI1STATbits.SRXMPT)
            data[rec++] = SPI1BUF;
    }

    while(rec != length)
    {
        if( (!SPI1STATbits.SRXMPT))
            data[rec++] = SPI1BUF;
    }
}

static void spi_write_bytes(uint8_t* data, uint16_t length)
{
    uint16_t cnt = length;
    uint16_t sent = 0;

    while(sent != length)
    {
        if(!(SPI1STAT & (_SPI1STAT_SPIBEC2_MASK|_SPI1STAT_SPIRBF_MASK)))
            SPI1BUF = data[sent++];

        if(!SPI1STATbits.SRXMPT)
            SPI1BUF, cnt--;
    }

    while(cnt)
    {
        if( !SPI1STATbits.SRXMPT)
            SPI1BUF, cnt--;
    }
}
#endif

void spi_sniffer(bool trigger, bool terminal_mode) {
  bool last_cs_line_state;

restart:

  last_cs_line_state = HIGH;

  user_serial_ringbuffer_setup();
  spi_disable_interface();
  spi_slave_enable();

  if (trigger == SPI_SNIFF_ON_CS_LOW) {
    SPI1CON1bits.SSEN = ON;
    SPI2CON1bits.SSEN = ON;
  }

  SPI1STATbits.SPIEN = ON;
  SPI2STATbits.SPIEN = ON;

  for (;;) {

    /* Detect a CS line state change. */
    if ((last_cs_line_state == LOW) && (SPICS == HIGH)) {
      user_serial_ringbuffer_append(']');
      last_cs_line_state = HIGH;
    }

    /* Is there any data to read? */
    if ((SPI1STATbits.SRXMPT == NO) && (SPI2STATbits.SRXMPT == NO)) {
      uint8_t data;

      data = SPI1BUF;

      if (last_cs_line_state == HIGH) {
        user_serial_ringbuffer_append('[');
        last_cs_line_state = LOW;
      }

      if (terminal_mode) {
        bp_write_hex_byte_to_ringbuffer(data);
      } else {
        user_serial_ringbuffer_append('\\');
        user_serial_ringbuffer_append(data);
      }

      data = SPI2BUF;

      if (terminal_mode) {
        user_serial_ringbuffer_append('(');
        bp_write_hex_byte_to_ringbuffer(data);
        user_serial_ringbuffer_append(')');
      } else {
        user_serial_ringbuffer_append(data);
      }
    }

    /* Check for overflows. */
    if ((SPI1STATbits.SPIROV == ON) || (SPI2STATbits.SPIROV == ON) ||
        (bus_pirate_configuration.overflow == YES)) {

      /* Was the overflow coming from the serial port? */
      if (bus_pirate_configuration.overflow == NO) {
        user_serial_ringbuffer_flush();
      }

      /*
       * SPI1STAT - SPI1 STATUS REGISTER
       *
       * MSB
       * 0-0------0----??
       * | |      |
       * | |      +--------- SPIROV:  Overflow flag cleared.
       * | +---------------- SPISIDL: Continue module operation in idle mode.
       * +------------------ SPIEN:   Module disabled.
       */
      SPI1STAT = 0x0000;

      /*
       * SPI2STAT - SPI2 STATUS REGISTER
       *
       * MSB
       * 0-0------0----??
       * | |      |
       * | |      +--------- SPIROV:  Overflow flag cleared.
       * | +---------------- SPISIDL: Continue module operation in idle mode.
       * +------------------ SPIEN:   Module disabled.
       */
      SPI2STAT = 0x0000;

      if (terminal_mode) {
        MSG_SPI_COULD_NOT_KEEP_UP;
        goto restart;
      }

      BP_LEDMODE = OFF;
      break;
    }

    user_serial_ringbuffer_process();

    if (user_serial_ready_to_read()) {
      user_serial_read_byte();

      if (terminal_mode) {
        bpBR;
      }
      break;
    }
  }

  spi_slave_disable();

  spi_setup(spi_bus_speed[mode_configuration.speed]);
}

void spi_slave_enable(void) {

  /* Assign slave SPI pin directions. */
  SPICS_TRIS = INPUT;
  SPICLK_TRIS = INPUT;
  SPIMISO_TRIS = INPUT;
  SPIMOSI_TRIS = INPUT;

  /* Route SPI pins to the appropriate destinations. */
  RPINR21bits.SS1R = BP_CS_RPIN;
  RPINR23bits.SS2R = BP_CS_RPIN;
  RPINR20bits.SDI1R = BP_MOSI_RPIN;
  RPINR20bits.SCK1R = BP_CLK_RPIN;
  RPINR22bits.SDI2R = BP_MISO_RPIN;
  RPINR22bits.SCK2R = BP_CLK_RPIN;

  /* Prepare SPI interfaces first. */

  /*
   * SPI1CON1 - SPI1 CONTROL REGISTER 1
   *
   * MSB
   * ---0000C0B0AAAAA
   *    |||||||||||||
   *    |||||||||||++--- PPRE:   Primary prescale bits.
   *    ||||||||+++----- SPRE:   Secondary prescale bits.
   *    |||||||+-------- MSTEN:  Slave mode.
   *    ||||||+--------- CKP:    Clock idle state.
   *    |||||+---------- SSEN:   Pin controlled by port function.
   *    ||||+----------- CKE:    Flag indicating when transitions happen.
   *    |||+------------ SMP:    Data sampled on data output middle.
   *    ||+------------- MODE16: Communication is byte-wide.
   *    |+-------------- DISSDO: SDO1 pin is controlled by the module.
   *    +--------------- DISSCK: Internal SPI clock is enabled.
   */
  SPI1CON1 =
      (MASKBOTTOM8(spi_bus_speed[mode_configuration.speed], 5)
       << _SPI1CON1_PPRE_POSITION) |
      (MASKBOTTOM8(spi_state.clock_polarity, 1) << _SPI1CON1_CKP_POSITION) |
      (MASKBOTTOM8(spi_state.clock_edge, 1) << _SPI1CON1_CKE_POSITION);

  /*
   * SPI1CON2 - SPI1 CONTROL REGISTER 2
   *
   * MSB
   * 000-----------0-
   * |||           |
   * |||           +---- FRMDLY: Frame sync pulse precedes first bit clock.
   * ||+---------------- FRMPOL: Frame sync pulse is active low.
   * |+----------------- SPIFSD: Frame sync pulse output.
   * +------------------ FRMEN:  Framed SPI1 support disabled.
   */
  SPI1CON2 = 0x0000;

  /*
   * SPI1STAT - SPI1 STATUS REGISTER
   *
   * MSB
   * 0-0------0----??
   * | |      |
   * | |      +--------- SPIROV:  Overflow flag cleared.
   * | +---------------- SPISIDL: Continue module operation in idle mode.
   * +------------------ SPIEN:   Module disabled.
   */
  SPI1STAT = 0x0000;

  /*
   * SPI2CON1 - SPI2 CONTROL REGISTER 1
   *
   * MSB
   * ---0000C0B0AAAAA
   *    |||||||||||||
   *    |||||||||||++--- PPRE:   Primary prescale bits.
   *    ||||||||+++----- SPRE:   Secondary prescale bits.
   *    |||||||+-------- MSTEN:  Slave mode.
   *    ||||||+--------- CKP:    Clock idle state.
   *    |||||+---------- SSEN:   Pin controlled by port function.
   *    ||||+----------- CKE:    Flag indicating when transitions happen.
   *    |||+------------ SMP:    Data sampled on data output middle.
   *    ||+------------- MODE16: Communication is byte-wide.
   *    |+-------------- DISSDO: SDO2 pin is controlled by the module.
   *    +--------------- DISSCK: Internal SPI clock is enabled.
   */
  SPI2CON1 =
      (MASKBOTTOM8(spi_bus_speed[mode_configuration.speed], 5)
       << _SPI2CON1_PPRE_POSITION) |
      (MASKBOTTOM8(spi_state.clock_polarity, 1) << _SPI2CON1_CKP_POSITION) |
      (MASKBOTTOM8(spi_state.clock_edge, 1) << _SPI2CON1_CKE_POSITION);

  /*
   * SPI2CON2 - SPI2 CONTROL REGISTER 2
   *
   * MSB
   * 000-----------0-
   * |||           |
   * |||           +---- FRMDLY: Frame sync pulse precedes first bit clock.
   * ||+---------------- FRMPOL: Frame sync pulse is active low.
   * |+----------------- SPIFSD: Frame sync pulse output.
   * +------------------ FRMEN:  Framed SPI2 support disabled.
   */
  SPI2CON2 = 0x0000;

  /*
   * SPI2STAT - SPI2 STATUS REGISTER
   *
   * MSB
   * 0-0------0----??
   * | |      |
   * | |      +--------- SPIROV:  Overflow flag cleared.
   * | +---------------- SPISIDL: Continue module operation in idle mode.
   * +------------------ SPIEN:   Module disabled.
   */
  SPI2STAT = 0x0000;

  /* Setup the SPI module to operate in enhanced buffer mode. */

  /* Clear RX/TX registers. */
  SPI1BUF = 0x0000;
  SPI2BUF = 0x0000;

  /* Set the appropriate mode bits while MSTEN is OFF. */
  SPI1CON1bits.DISSDO = ON;
  SPI2CON1bits.DISSDO = ON;

  /* Clear the SMP bits. */
  SPI1CON1bits.SMP = OFF;
  SPI2CON1bits.SMP = OFF;

  /* Clear the overflow bits. */
  SPI1STATbits.SPIROV = OFF;
  SPI2STATbits.SPIROV = OFF;

  /* Select enhanced buffer mode. */
  SPI1CON2bits.SPIBEN = ON;
  SPI2CON2bits.SPIBEN = ON;
}

void spi_slave_disable(void) {

  /* Turn the modules off. */
  SPI1STATbits.SPIEN = OFF;
  SPI2STATbits.SPIEN = OFF;

  /* Restore the initial SDOx pin state. */
  SPI1CON1bits.DISSDO = OFF;
  SPI2CON1bits.DISSDO = OFF;

  /* Clear pin assignments. */
  RPINR21bits.SS1R = 0b11111;
  RPINR23bits.SS2R = 0b11111;
  RPINR20bits.SDI1R = 0b11111;
  RPINR20bits.SCK1R = 0b11111;
  RPINR22bits.SDI2R = 0b11111;
  RPINR22bits.SCK2R = 0b11111;
}

void spi_enter_binary_io(void) {
  mode_configuration.speed = 1;
  spi_state.clock_polarity = SPI_CLOCK_IDLE_LOW;
  spi_state.clock_edge = SPI_TRANSITION_FROM_ACTIVE_TO_IDLE;
  spi_state.data_sample_timing = SPI_SAMPLING_ON_DATA_OUTPUT_MIDDLE;
  mode_configuration.high_impedance = ON;
  spi_setup(spi_bus_speed[mode_configuration.speed]);
  MSG_SPI_MODE_IDENTIFIER;

  for (;;) {
    uint8_t input_byte = user_serial_read_byte();
    uint8_t command = input_byte >> 4;

    switch (command) {
    case SPI_COMMAND_BASE:
      switch (input_byte) {
      case SPI_BASE_COMMAND_EXIT:
        spi_disable_interface();
        return;

      case SPI_BASE_COMMAND_SEND_IDENTIFIER:
        MSG_SPI_MODE_IDENTIFIER;
        break;

      case SPI_BASE_COMMAND_CS_LOW:
        IOLAT &= ~CS;
        REPORT_IO_SUCCESS();
        break;

      case SPI_BASE_COMMAND_CS_HIGH:
        IOLAT |= CS;
        REPORT_IO_SUCCESS();
        break;

      case SPI_BASE_COMMAND_SNIFF_ALL_TRAFFIC:
        REPORT_IO_SUCCESS();
        spi_sniffer(SPI_SNIFF_ALWAYS, false);
        break;

      case SPI_BASE_COMMAND_SNIFF_WHEN_CS_LOW:
        REPORT_IO_SUCCESS();
        spi_sniffer(SPI_SNIFF_ON_CS_LOW, false);
        break;

      case SPI_BASE_COMMAND_WRITE_AND_READ_WITH_CS:
      case SPI_BASE_COMMAND_WRITE_AND_READ_WITHOUT_CS:
        spi_read_write_io(input_byte ==
                          SPI_BASE_COMMAND_WRITE_AND_READ_WITH_CS);
        break;

#ifdef BP_SPI_ENABLE_AVR_EXTENDED_COMMANDS

      case SPI_BASE_COMMAND_EXTENDED_AVR_COMMAND:
        handle_extended_avr_command();
        break;

#endif /* BP_SPI_ENABLE_AVR_EXTENDED_COMMANDS */

      default:
        REPORT_IO_FAILURE();
        break;
      }

    break;

    case SPI_COMMAND_READ_DATA: {
      uint8_t bytes_to_read = (input_byte & 0x0F) + 1;
      REPORT_IO_SUCCESS();
      for (size_t count = 0; count < bytes_to_read; count++) {
        user_serial_transmit_character(spi_write_byte(user_serial_read_byte()));
      }
      break;
    }

    case SPI_COMMAND_CONFIGURE_PERIPHERALS:
      bp_binary_io_peripherals_set(input_byte);
      REPORT_IO_SUCCESS();
      break;

#ifdef BUSPIRATEV4

    case SPI_COMMAND_SET_PULLUPS:
      user_serial_transmit_character(bp_binary_io_pullup_control(input_byte));
      break;

#endif /* BUSPIRATEV4 */

    case SPI_COMMAND_SET_SPEED: {
      uint8_t speed = input_byte & 0x0F;
      if (speed > sizeof(spi_bus_speed)) {
        REPORT_IO_FAILURE();
        break;
      }

      mode_configuration.speed = speed;
      spi_setup(spi_bus_speed[mode_configuration.speed]);
      REPORT_IO_SUCCESS();
      break;
    }

    case SPI_COMMAND_CONFIGURE_SPI:
      spi_state.clock_polarity = SPI_CLOCK_IDLE_LOW;
      spi_state.clock_edge = SPI_TRANSITION_FROM_IDLE_TO_ACTIVE;
      spi_state.data_sample_timing = SPI_SAMPLING_ON_DATA_OUTPUT_MIDDLE;
      mode_configuration.high_impedance = OFF;
      spi_state.clock_polarity =
          (input_byte & 0b0100) ? SPI_CLOCK_IDLE_HIGH : SPI_CLOCK_IDLE_LOW;
      spi_state.clock_edge = (input_byte & 0b0010)
                                 ? SPI_TRANSITION_FROM_ACTIVE_TO_IDLE
                                 : SPI_TRANSITION_FROM_IDLE_TO_ACTIVE;
      spi_state.data_sample_timing = (input_byte & 0b0001)
                                         ? SPI_SAMPLING_ON_DATA_OUTPUT_END
                                         : SPI_SAMPLING_ON_DATA_OUTPUT_MIDDLE;
      mode_configuration.high_impedance = (input_byte & 0b1000) == 0 ? ON : OFF;
      spi_setup(spi_bus_speed[mode_configuration.speed]);
      REPORT_IO_SUCCESS();
      break;

    default:
      REPORT_IO_FAILURE();
      break;
    }

#ifdef BUSPIRATEV3
    /* avoid the 16ms FTDI buffer send latency */
    user_serial_wait_transmission_done();
    /* Wait for the bus to settle. */
    bp_delay_us(50);
    FTDI_CTS = !FTDI_CTS;
#endif /* BUSPIRATEV3 */

#ifdef BUSPIRATEV4
    /* avoid the 4ms PIC USB send latency */
    user_serial_ringbuffer_flush();
#endif /* BUSPIRATEV4 */
  }
}

static void spi_write_from_host(uint16_t bytes_to_write)
{
#ifdef BUSPIRATEV3
    spi_write_bytes_from_uart(bytes_to_write);
#else
    while(bytes_to_write)
    {
        uint16_t len;
        len = bytes_to_write;
        BYTE* data = getoutbuf_cdc(&len);
        spi_write_bytes(data, len);
        bytes_to_write -= len;
    }
#endif /* BUSPIRATEV3 */
}

static void spi_read_to_host(uint16_t bytes_to_read)
{
  REPORT_IO_SUCCESS();
#ifdef BUSPIRATEV3
    spi_read_bytes_to_uart(bytes_to_read);
#else
  bp_toggle_usb_led();

    setLock();

    while(bytes_to_read)
    {
        uint16_t chunksize = 0;
        BYTE* cdc_buf = getinbuf_cdc(&chunksize);

        if (chunksize > bytes_to_read)
            chunksize = bytes_to_read;

        // read the SPI data into the USB buffer
        spi_read_bytes(cdc_buf, chunksize);

        // tell the USB stack to send out the data
        write_cdc(chunksize);

        bytes_to_read -= chunksize;
    }

    freeLock();

#endif
}

void spi_read_write_io(const bool engage_cs) {

  /* How many bytes to send to the bus. */
  uint16_t bytes_to_write = user_serial_read_big_endian_word();

  /* How many bytes to read from the bus. */
  uint16_t bytes_to_read = user_serial_read_big_endian_word();

  if ((bytes_to_write == 0) && (bytes_to_read == 0)) {
    REPORT_IO_FAILURE();
    return;
  }

  /* Update the CS line if needed. */
  if (engage_cs == true) {
    SPICS = LOW;
  }

  if (bytes_to_write > 0) {
    spi_write_from_host(bytes_to_write);

    /* Wait for the bus to settle. */
    bp_delay_us(1);
  }

  if (bytes_to_read > 0) {
    spi_read_to_host(bytes_to_read);
  } else {
    REPORT_IO_SUCCESS();
  }

    /* Reset the CS line if needed. */
  if (engage_cs == true) {
    SPICS = HIGH;
  }
}

#ifdef BP_SPI_ENABLE_AVR_EXTENDED_COMMANDS

void handle_extended_avr_command(void) {
  /* Acknowledge extended command. */
  REPORT_IO_SUCCESS();

  uint8_t command = user_serial_read_byte();
  switch (command) {
  case BINARY_IO_SPI_AVR_COMMAND_NOOP:
    REPORT_IO_SUCCESS();
    break;

  case BINARY_IO_SPI_AVR_COMMAND_VERSION:
    REPORT_IO_SUCCESS();
    user_serial_transmit_character(HI8(BINARY_IO_SPI_AVR_SUPPORT_VERSION));
    user_serial_transmit_character(LO8(BINARY_IO_SPI_AVR_SUPPORT_VERSION));
    break;

  case BINARY_IO_SPI_AVR_COMMAND_BULK_READ: {
    uint32_t address = user_serial_read_big_endian_long_word();
    uint32_t length = user_serial_read_big_endian_long_word();

    if ((address > 0xFFFF) || (length > 0xFFFF) ||
        ((address + length) > 0xFFFF)) {
      REPORT_IO_FAILURE();
      return;
    }

    REPORT_IO_SUCCESS();
    while (length > 0) {
      /* Fetch low byte from word. */
      spi_write_byte(AVR_FETCH_LOW_BYTE_COMMAND);
      spi_write_byte((address >> 8) & 0xFF);
      spi_write_byte(address & 0xFF);
      user_serial_transmit_character(spi_write_byte(0x00));
      length--;

      if (length > 0) {
        /* Fetch high byte from word. */
        spi_write_byte(AVR_FETCH_HIGH_BYTE_COMMAND);
        spi_write_byte((address >> 8) & 0xFF);
        spi_write_byte(address & 0xFF);
        user_serial_transmit_character(spi_write_byte(0x00));
        length--;
      }

      address++;
    }

    break;
  }

  default:
    REPORT_IO_FAILURE();
    break;
  }
}

#endif /* BP_SPI_ENABLE_AVR_EXTENDED_COMMANDS */

#endif /* BP_ENABLE_SPI_SUPPORT */
