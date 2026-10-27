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
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#include "uart.h"

#ifdef BP_ENABLE_UART_SUPPORT

#include "base.h"
#include "binary_io.h"
#include "proc_menu.h"
#include "uart2.h"

extern mode_configuration_t mode_configuration;
extern command_t last_command;

#define UART_COMMON_BAUD_RATES_COUNT 15
#define UART_BAUD_RATE_CALCULATION_SAMPLES 25
#define UART_MACRO_MENU 0
#define UART_MACRO_TRANSPARENT_BRIDGE 1
#define UART_MACRO_RAW_UART 2
#define UART_MACRO_BRIDGE_WITH_FLOW_CONTROL 3
#define UART_MACRO_AUTO_BAUD_RATE_DETECTION 4

typedef struct {
  uint8_t databits_parity : 2;
  uint8_t stop_bits : 1;
  uint8_t receive_polarity : 1;
  uint8_t echo_uart : 1;
#ifdef BUSPIRATEV4
  uint8_t autobaud : 1;
#endif /* BUSPIRATEV4 */
} UARTSettings;

static UARTSettings uart_settings = {0};

static const uint32_t UART_COMMON_BAUD_RATES[] = {
    0,     300,   600,   1200,  2400,  4800,   9600,   14400,
    19200, 28800, 38400, 56000, 57600, 115200, 128000, 256000};

const uint16_t UART_BRG_SPEED[] = {
    13332, /* 300 bps */
    3332,  /* 1200 bps */
    1666,  /* 2400 bps */
    832,   /* 4800 bps */
    416,   /* 9600 bps */
    207,   /* 19200 bps */
    103,   /* 38400 bps */
    68,    /* 57600 bps */
    34,    /* 115200 bps */
    127    /* 31250 bps */
};

/**
 * Obtains the closest-matching common rate for the given baud rate.
 *
 * @param[in] baud_rate the baud rate to get a match for.
 *
 * @return the closest common baud rate for the given value, or 0 if there was
 * no match.
 */
static uint32_t uart_get_closest_common_rate(const uint32_t baud_rate);

/**
 * Calculates the UART baud rate from detected activity on the port.
 *
 * @param[in] quiet true if no messages should be printed on the console, false
 * if no verbose reporting is needed.
 *
 * @return the calculated baud rate.
 */
static uint32_t uart_get_baud_rate(const bool quiet);

uint16_t uart_read(void) {
  if (uart2_rx_ready()) {
    uint16_t character;

    /* Parity error? */
    if (U2STAbits.PERR) {
      MSG_UART_PARITY_ERROR;
    }

    /* Framing error? */
    if (U2STAbits.FERR) {
      MSG_UART_FRAMING_ERROR;
    }

    character = uart2_rx();

    /* Overrun error? */
    if (U2STAbits.OERR) {
      MSG_UART_OVERRUN_ERROR;

      /* Clear overrun flag. */
      U2STAbits.OERR = OFF;
    }

    return character;
  }

  MSG_UART_FAILED_NO_DATA;
  return 0;
}

inline uint16_t uart_write(const uint16_t character) {
  uart2_tx(character);
  return 0;
}

void UARTsettings(void) {
  MSG_UART_MODE_HEADER;
  bp_write_dec_byte(mode_configuration.speed);
  bpSP;
  bp_write_dec_word((mode_configuration.speed == 9)
                        ? U2BRG
                        : UART_BRG_SPEED[mode_configuration.speed]);
  bpSP;
  bp_write_dec_byte(uart_settings.databits_parity);
  bpSP;
  bp_write_dec_byte(uart_settings.stop_bits);
  bpSP;
  bp_write_dec_byte(uart_settings.receive_polarity);
  bpSP;
  bp_write_dec_byte(mode_configuration.high_impedance);
  MSG_MODE_HEADER_END;
}

void uart_setup_prepare(void) {
  int speed, dbp, sb, rxp, output, brg = 0;
  // autobaud detection; multi uses
  unsigned long abd = 0;

#ifdef BUSPIRATEV4
  uart_settings.autobaud = OFF;
#endif /* BUSPIRATEV4 */

  consumewhitechars();
  speed = getint();
  if (speed == 10) // weird this is totaly ignored later as the Speed == 0 check
                   // later skips the calculation I added it below..
  {
    consumewhitechars();
    brg = getint();
  }
  consumewhitechars();
  dbp = getint();
  consumewhitechars();
  sb = getint();
  consumewhitechars();
  rxp = getint();
  consumewhitechars();
  output = getint();

  if ((speed > 0) && (speed <= 10)) {
    mode_configuration.speed = speed - 1;
  } else {
    speed = 0; // when speed is 0 we ask the user
  }

  if ((dbp > 0) && (dbp <= 4)) {
    uart_settings.databits_parity = dbp - 1;
  } else {
    speed = 0; // when speed is 0 we ask the user
  }

  if ((sb > 0) && (sb <= 2)) {
    uart_settings.stop_bits = sb - 1;
  } else {
    speed = 0; // when speed is 0 we ask the user
  }

  if ((rxp > 0) && (rxp <= 2)) {
    uart_settings.receive_polarity = rxp - 1;
  } else {
    speed = 0; // when speed is 0 we ask the user
  }

  if ((output > 0) && (output <= 2)) {
    mode_configuration.high_impedance = ~(output - 1);
  } else {
    speed = 0; // when speed is 0 we ask the user
  }

  if (speed == 0) {
    mode_configuration.command_error = NO;

    MSG_UART_SET_PORT_SPEED;

#if defined(BUSPIRATEV4)
    // BPv4 Mode; has custom BAUD entry and auto-baud detection

    mode_configuration.speed = getnumber(1, 1, 11, 0) - 1; // get user reply

    if (mode_configuration.speed == 10) {
      mode_configuration.speed = 8; // Set to 115200 for now
      abd = 1;                      // trigger to run baud detection
      uart_settings.autobaud = ON;
      MSG_BAUD_DETECTION_SELECTED;
    }

    if (mode_configuration.speed == 9) {
      MSG_UART_CUSTOM_BAUD_RATE_PROMPT;
      abd = getlong(115200, 1, 999999, 0); // get the baud rate from user
      abd = (((32000000 / abd) / 8) - 1);  // calculate BRG
      brg = abd;                           // set BRG
      abd = 0; // set abd to 0; so 'Auto Baud Detection' routine isnt ran below
      // hack hack hakc
      U2BRG = brg; // passing the brg variable to U2BRG so the UARTsetup_exc can
                   // use it to start UART2setup..
    }
#else
    // Normal mode; input BRG and no autobaud detection
    mode_configuration.speed = getnumber(1, 1, 10, 0) - 1; // get user reply

    if (mode_configuration.speed == 9) {
      MSG_UART_RAW_BRG_PROMPT;
      brg = getnumber(34, 1, 32767, 0);
      // hack hack hack
      U2BRG = brg; // passing the brg variable to U2BRG so the UARTsetup_exc can
                   // use it to start UART2setup..
    }
#endif /* BUSPIRATEV4 */

    MSG_UART_BITS_PARITY_PROMPT;
    uart_settings.databits_parity = getnumber(1, 1, 4, 0) - 1;

    MSG_UART_BITS_STOP_PROMPT;
    uart_settings.stop_bits = getnumber(1, 1, 2, 0) - 1;

    MSG_UART_POLARITY_PROMPT;
    uart_settings.receive_polarity = getnumber(1, 1, 2, 0) - 1;

    MSG_PIN_OUTPUT_TYPE_PROMPT;
    mode_configuration.high_impedance = ~(getnumber(1, 1, 2, 0) - 1);
  } else {
    if (mode_configuration.speed == 9) {
      abd = brg;
      abd = (((32000000 / abd) / 8) - 1); // calculate BRG
      brg = abd;                          // set BRG
      abd = 0; // set abd to 0; so 'Auto Baud Detection' routine isnt ran below
      // hack hack hakc
      U2BRG = brg; // passing the brg variable to U2BRG so the UARTsetup_exc can
                   // use it to start UART2setup..
    }
    UARTsettings();
  }
}

void uart_setup_execute(void) {
  uart2_setup(mode_configuration.speed == 9
                  ? U2BRG
                  : UART_BRG_SPEED[mode_configuration.speed],
              mode_configuration.high_impedance, uart_settings.receive_polarity,
              uart_settings.databits_parity, uart_settings.stop_bits);

  if (uart_settings.databits_parity == 3) {
    mode_configuration.numbits = 9;
  }

  uart2_enable();

#ifdef BUSPIRATEV4
  unsigned long abd;
  int brg;
  if (uart_settings.autobaud == ON) {
    uart2_disable();
    bpBR;
    abd = uart_get_closest_common_rate(uart_get_baud_rate(false));
    bpBR;

    if (abd == 0) {
      uart2_setup(UART_BRG_SPEED[8], mode_configuration.high_impedance,
                  uart_settings.receive_polarity, uart_settings.databits_parity,
                  uart_settings.stop_bits);
    } else {
      mode_configuration.speed = 9;
      abd = (((32000000 / abd) / 8) - 1);
      brg = abd;
      uart2_setup(brg, mode_configuration.high_impedance,
                  uart_settings.receive_polarity, uart_settings.databits_parity,
                  uart_settings.stop_bits);
    }
    uart2_enable();
  }
#else
  if (U2BRG < U1BRG) {
    MSG_UART_POSSIBLE_OVERFLOW;
  }
#endif /* BUSPIRATEV4 */
}

inline void uart_cleanup(void) { uart2_disable(); }

void uart_run_macro(const uint16_t macro) {
  switch (macro) {
  case UART_MACRO_MENU:
    MSG_UART_MACRO_MENU;
    break;

  case UART_MACRO_BRIDGE_WITH_FLOW_CONTROL:
#ifdef BUSPIRATEV3
    /* FTDI CTS and RTS setup. */
    FTDI_CTS_DIR = OUTPUT;
    FTDI_RTS_DIR = INPUT;

    /* External CTS and RTS setup. */
    BP_CS_DIR = INPUT;
    BP_CLK_DIR = OUTPUT;
#else
/* Do nothing for v4? */
#endif /* BUSPIRATEV3 */

  /* Intentional fall-through. */

  case UART_MACRO_TRANSPARENT_BRIDGE:
    MSG_UART_BRIDGE;
    MSG_UART_BRIDGE_EXIT;

    if (!agree()) {
      break;
    }

    /* Make terminal speed match target speed to avoid overruns? */

    /* Clear overrun flag. */
    U2STAbits.OERR = OFF;

    for (;;) {
#ifdef BUSPIRATEV4
      if (BP_BUTTON_ISDOWN()) {
        break;
      }

      if (U2STAbits.URXDA == ON) {
        user_serial_transmit_character(U2RXREG);
      }
#else
      if ((U2STAbits.URXDA == ON) && (U1STAbits.UTXBF == OFF)) {
        U1TXREG = U2RXREG;
      }
#endif /* BUSPIRATEV4 */

      if (user_serial_ready_to_read() && (U2STAbits.UTXBF == OFF)) {
        U2TXREG = user_serial_read_byte();
      }

#ifdef BUSPIRATEV3
      if (U2STAbits.OERR || U1STAbits.OERR) {
        U2STAbits.OERR = OFF;
        U1STAbits.OERR = OFF;
        BP_LEDMODE = LOW;
      }
#else
      if (U2STAbits.OERR) {
        U2STAbits.OERR = OFF;
        BP_LEDMODE = LOW;
      }
#endif /* BUSPIRATEV3 */

#ifdef BUSPIRATEV3
      if (macro == UART_MACRO_BRIDGE_WITH_FLOW_CONTROL) {
        /* Relay flow control bits. */
        BP_CLK = FTDI_RTS;
        FTDI_CTS = BP_CS;
      }
#endif /* BUSPIRATEV3 */
    }
    break;

  case UART_MACRO_RAW_UART:
    MSG_UART_RAW_UART_INPUT;
    MSG_ANY_KEY_TO_EXIT_PROMPT;

    U2STAbits.OERR = OFF;
    for (;;) {
#ifdef BUSPIRATEV4
      if (BP_BUTTON_ISDOWN()) {
        break;
      }

      if (uart2_rx_ready()) {
        user_serial_transmit_character(uart2_rx());
      }
#else
      if ((U2STAbits.URXDA == ON) && (U1STAbits.UTXBF == OFF)) {
        U1TXREG = U2RXREG;
      }
#endif /* BUSPIRATEV4 */

      if (user_serial_ready_to_read()) {
        volatile uint16_t dummy;

        dummy = user_serial_read_byte();
        break;
      }
    }
    break;

  case UART_MACRO_AUTO_BAUD_RATE_DETECTION:
    uart2_disable();
    uart_get_baud_rate(false);
    uart2_enable();
#ifdef BUSPIRATEV3
    if (U2BRG < U1BRG) {
      MSG_UART_POSSIBLE_OVERFLOW;
    }
#endif /* BUSPIRATEV3 */
    break;

  default:
    MSG_UNKNOWN_MACRO_ERROR;
    break;
  }
}

void uart_start(void) {
  /* Clear overrun flag. */
  U2STAbits.OERR = OFF;

  /* Enable UART echo on the console. */
  uart_settings.echo_uart = ON;

  /* Start periodic service calls. */
  mode_configuration.periodicService = ON;

  MSG_UART_LIVE_DISPLAY_START;
}

void uart_stop(void) {
  /* Disable UART echo on the console. */
  uart_settings.echo_uart = OFF;

  /* Stop periodic service calls. */
  mode_configuration.periodicService = OFF;

  MSG_UART_LIVE_DISPLAY_STOP;
}

bool uart_periodic_callback(void) {
  bool handled;

  handled = false;
  while (uart2_rx_ready()) {
    handled = true;

    if (!uart_settings.echo_uart) {
      /* Clear RX queue. */
      uart2_rx();
      continue;
    }

    bpBR;
    MSG_READ_HEADER;

    /* Parity error? */
    if (U2STAbits.PERR) {
      MSG_UART_PARITY_ERROR;
    }

    /* Framing error? */
    if (U2STAbits.FERR) {
      MSG_UART_FRAMING_ERROR;
    }

    bp_write_formatted_integer(uart2_rx());

    /* Overrun error? */
    if (U2STAbits.OERR) {

      /* Clear overrun flag. */
      U2STAbits.OERR = OFF;

      MSG_UART_OVERRUN_ERROR;
    }

    bpBR;
  }

  return handled;
}

inline void uart_pins_state(void) { MSG_UART_PINS_STATE; }

uint32_t uart_get_closest_common_rate(const uint32_t baud_rate) {
  size_t counter;

  for (counter = 1; counter < UART_COMMON_BAUD_RATES_COUNT; counter++) {
    if (baud_rate < UART_COMMON_BAUD_RATES[counter]) {
      uint32_t lower;
      uint32_t upper;

      lower = UART_COMMON_BAUD_RATES[counter - 1];
      upper = UART_COMMON_BAUD_RATES[counter];

      return (baud_rate > lower + (upper - lower) / 2) ? upper : lower;
    }
  }

  return 0;
}

uint32_t uart_get_baud_rate(const bool quiet) {
  size_t samples;
  uint32_t current_sample;
  uint32_t bit_sample;

  BP_MISO = LOW;
  BP_MISO_DIR = INPUT;
  bit_sample = 0;

  if (!quiet) {
    MSG_UART_WAITING_ACTIVITY;
  }

  /* Wait for the UART to stabilise. */
  while ((BP_MISO == HIGH) && (U1STAbits.URXDA == OFF)) {
    Nop();
  }

  /* Key pressed during detection, bailing out. */
  if (U1STAbits.URXDA == ON) {
    volatile uint16_t dummy;

    /* Clear RX queue. */
    dummy = U1RXREG;

    /* Stop timers. */

    /* Clear timer 5 holding register. */
    TMR5HLD = 0;

    /*
     * T4CON: TIMER4 CONTROL REGISTER
     *
     * MSB
     * 0-0------0001-0-
     * | |      |||| |
     * | |      |||| +--- TCS:   Internal clock (FOSC/2)
     * | |      |||+----- T32:   Timerx and Timery form a single 32-bit timer.
     * | |      |++------ TCKPS: Input prescaler 1:1.
     * | |      +-------- TGATE: Gated time accumulation is disabled.
     * | +--------------- TSIDL  Continues module operation in Idle mode.
     * +----------------- TON:   Stops 16-bit Timerx.
     */
    T4CON = 1 << _T4CON_T32_POSITION;

    /* Clear timer 4 register. */
    TMR4 = 0;

    /* Clear timer 3 holding register. */
    TMR3HLD = 0;

    /*
     * T2CON: TIMER2 CONTROL REGISTER
     *
     * MSB
     * 0-0------0001-0-
     * | |      |||| |
     * | |      |||| +--- TCS:   Internal clock (FOSC/2)
     * | |      |||+----- T32:   Timerx and Timery form a single 32-bit timer.
     * | |      |++------ TCKPS: Input prescaler 1:1.
     * | |      +-------- TGATE: Gated time accumulation is disabled.
     * | +--------------- TSIDL  Continues module operation in Idle mode.
     * +----------------- TON:   Stops 16-bit Timerx.
     */
    T2CON = 1 << _T2CON_T32_POSITION;

    /* Clear timer 2 register. */
    TMR2 = 0;

    if (!quiet) {
      MSG_UART_EARLY_EXIT;
    }

    return 0;
  }

  for (samples = 0; samples < UART_BAUD_RATE_CALCULATION_SAMPLES; samples++) {

    /* Wait for activity. */
    while (BP_MISO == LOW) {
    }

    /* Start counting. */
    T4CONbits.TON = ON;
    T2CONbits.TON = ON;

    /* Wait for line to go down. */
    while (BP_MISO == HIGH) {
    }

    /* Stop counting. */
    T2CONbits.TON = OFF;
    T4CONbits.TON = OFF;

    /* Neuter Timer2. */
    RPINR3bits.T2CKR = 0b011111;

    /* Stop timers. */

    /*
     * T4CON: TIMER4 CONTROL REGISTER
     *
     * MSB
     * 0-0------0000-0-
     * | |      |||| |
     * | |      |||| +--- TCS:   Internal clock (FOSC/2)
     * | |      |||+----- T32:   Timerx and Timery act as 2 16-bit timers.
     * | |      |++------ TCKPS: Input prescaler 1:1.
     * | |      +-------- TGATE: Gated time accumulation is disabled.
     * | +--------------- TSIDL  Continues module operation in Idle mode.
     * +----------------- TON:   Stops 16-bit Timerx.
     */
    T4CON = 0;

    /*
     * T2CON: TIMER2 CONTROL REGISTER
     *
     * MSB
     * 0-0------0000-0-
     * | |      |||| |
     * | |      |||| +--- TCS:   Internal clock (FOSC/2)
     * | |      |||+----- T32:   Timerx and Timery act as 2 16-bit timers.
     * | |      |++------ TCKPS: Input prescaler 1:1.
     * | |      +-------- TGATE: Gated time accumulation is disabled.
     * | +--------------- TSIDL  Continues module operation in Idle mode.
     * +----------------- TON:   Stops 16-bit Timerx.
     */
    T2CON = 0;

    /* Get the smallest timing value after the first one. */

    current_sample = ((uint32_t)TMR3HLD << 16) | TMR2;
    if ((samples > 0) && ((bit_sample == 0) || (bit_sample > current_sample))) {
      bit_sample = current_sample;
    }
  }

  /* Neuter Timer2. */
  RPINR3bits.T2CKR = 0b011111;

  /* Clear timer 2 register. */
  TMR2 = 0;

  /* Clear timer 3 holding register. */
  TMR3HLD = 0;

  bit_sample = FCY / bit_sample;

  if (!quiet) {
    if (bit_sample > 150000) {
      MSG_UART_BAUD_OVERFLOW;
    } else {
      MSG_UART_BAUD_ESTIMATED;
      bp_write_dec_dword(bit_sample);
      MSG_UART_BPS_MARKER;
      MSG_UART_BAUD_CALCULATED;
      bp_write_dec_dword(uart_get_closest_common_rate(bit_sample));
      MSG_UART_BPS_MARKER;
    }
  }

  return bit_sample;
}

/*
databits and parity (2bits)
1. 8, NONE *default \x0D\x0A 2. 8, EVEN \x0D\x0A 3. 8, ODD \x0D\x0A 4. 9, NONE
Stop bits:\x0D\x0A 1. 1 *default\x0D\x0A 2. 2 \x0D\x0A
Receive polarity:\x0D\x0A 1. Idle 1 *default\x0D\x0A 2. Idle 0\x0D\x0A
output type (hiz or regular
peripheral settings

# 00000000//reset to BBIO
# 00000001 � mode version string (ART1)
# 00000010 � UART start echo uart RX
# 00000011 � UART stop echo uart RX
# 00000111 - UART speed manual config, 2 bytes (BRGH, BRGL)
# 00001111 - bridge mode (reset to exit)
# 0001xxxx � Bulk transfer, send 1-16 bytes (0=1byte!)
# 0100wxyz � Set peripheral w=power, x=pullups, y=AUX, z=CS
# 0101wxyz � read peripherals
# 0110xxxx - Set speed,0000=300,0001=1200,10=2400,4800,9600,19200,31250,
38400,57600,1010=115200,
# 0111xxxx - Read speed,
# 100wxxyz � config, w=output type, xx=databits and parity, y=stop bits, z=rx
polarity (default :00000)
# 101wxxyz � read config
*/

void binUART(void) {
  uint16_t brg_value;

  uart_settings.databits_parity = 0;
  uart_settings.stop_bits = 0;
  uart_settings.receive_polarity = 0;
  mode_configuration.high_impedance = ON;
  brg_value = UART_BRG_SPEED[0]; // start at 300bps
  uart_settings.echo_uart = OFF;
  uart2_setup(brg_value, mode_configuration.high_impedance,
              uart_settings.receive_polarity, uart_settings.databits_parity,
              uart_settings.stop_bits);
  uart2_enable();
  MSG_UART_MODE_IDENTIFIER;

  for (;;) {
    uint8_t input_byte;

    if (uart2_rx_ready()) {
      if (uart_settings.echo_uart) {
        user_serial_transmit_character(uart2_rx());
      } else {
        uart2_rx();
      }
    }
    
    U2STAbits.OERR = OFF;

    if (!user_serial_ready_to_read()) {
      continue;
    }

    input_byte = user_serial_read_byte();

    switch (input_byte & 0xF0) {
    case 0:
      switch (input_byte) {
      case 0:
        uart2_disable();
        return;

      case 1:
        MSG_UART_MODE_IDENTIFIER;
        break;
        
      case 2:
        REPORT_IO_SUCCESS();
        U2STAbits.OERR = OFF;
        uart_settings.echo_uart = ON;
        break;
        
      case 3:
        uart_settings.echo_uart = OFF;
        REPORT_IO_SUCCESS();
        break;
        
      case 7:
        REPORT_IO_SUCCESS();
        uart2_disable();
        brg_value = (uint16_t)user_serial_read_byte() << 8;
        REPORT_IO_SUCCESS();
        brg_value |= user_serial_read_byte();
        uart2_setup(brg_value, mode_configuration.high_impedance,
                    uart_settings.receive_polarity,
                    uart_settings.databits_parity, uart_settings.stop_bits);
        uart2_enable();
        REPORT_IO_SUCCESS();
        break;
        
      case 15:
        REPORT_IO_SUCCESS();
        U2STAbits.OERR = OFF;
        for (;;) {

#ifdef BUSPIRATEV4
          if (U2STAbits.URXDA == ON) {
            user_serial_transmit_character(U2RXREG);
          }
#else
          if ((U2STAbits.URXDA == 1) && (U1STAbits.UTXBF == 0)) {
            U1TXREG = U2RXREG;
          }
#endif /* BUSPIRATEV4 */
          
          if (user_serial_ready_to_read() && (U2STAbits.UTXBF == 0)) {
            U2TXREG = user_serial_read_byte();
          }
        }
        break;
        
      default:
        REPORT_IO_FAILURE();
        break;
      }
      break;
      
    case 0b00010000: {
      size_t counter;
        
      input_byte &= 0b00001111;
      input_byte++;
      REPORT_IO_SUCCESS();

      for (counter = 0; counter < input_byte; counter++) {
        uart2_tx(user_serial_read_byte());
        REPORT_IO_SUCCESS();
      }
      break;
    }
      
    case 0b01000000:
      bp_binary_io_peripherals_set(input_byte);
      REPORT_IO_SUCCESS();
      break;

#ifdef BUSPIRATEV4
    case 0b01010000:
      user_serial_transmit_character(bp_binary_io_pullup_control(input_byte));
      break;
#endif /* BUSPIRATEV4 */
      
    case 0b01110000: // wait for an edge on the AUX pin
      handle_wait_command(input_byte);
      break;

    case 0b01100000:
      input_byte &= 0b00001111;
      if (input_byte > 0b1010) {
        input_byte = 0b1010;
      }
      brg_value = UART_BRG_SPEED[input_byte];
      uart2_disable();
      uart2_setup(brg_value, mode_configuration.high_impedance,
                  uart_settings.receive_polarity, uart_settings.databits_parity,
                  uart_settings.stop_bits);
      uart2_enable();
      REPORT_IO_SUCCESS();
      break;
      
    case 0b10000000:
    case 0b10010000:
      uart_settings.databits_parity = 0;
      uart_settings.stop_bits = 0;
      uart_settings.receive_polarity = 0;
      mode_configuration.high_impedance = OFF;
      if (input_byte & 0b1000) {
        uart_settings.databits_parity |= 0b10;
      }
      if (input_byte & 0b100) {
        uart_settings.databits_parity |= 0b1;
      }
      if (input_byte & 0b10) {
        uart_settings.stop_bits = 1;
      }
      if (input_byte & 0b1) {
        uart_settings.receive_polarity = 1;
      }
      if ((input_byte & 0b10000) == 0) {
        mode_configuration.high_impedance = ON;
      }
      uart2_disable();
      uart2_setup(brg_value, mode_configuration.high_impedance,
                  uart_settings.receive_polarity, uart_settings.databits_parity,
                  uart_settings.stop_bits);
      uart2_enable();
      REPORT_IO_SUCCESS();
      break;

    default:
      REPORT_IO_FAILURE();
      break;
    }
  }
}

#endif /* BP_ENABLE_UART_SUPPORT */
