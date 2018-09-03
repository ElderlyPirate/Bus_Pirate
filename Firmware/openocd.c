#include "openocd.h"

#ifdef BP_JTAG_OPENOCD_SUPPORT

#include "base.h"
#include "binary_io.h"
#include "core.h"

extern mode_configuration_t mode_configuration;
extern bus_pirate_configuration_t bus_pirate_configuration;

// tris registers
#define OOCD_TDO_TRIS BP_MISO_DIR
#define OOCD_TMS_TRIS BP_CS_DIR
#define OOCD_CLK_TRIS BP_CLK_DIR
#define OOCD_TDI_TRIS BP_MOSI_DIR
#define OOCD_SRST_TRIS BP_AUX0_DIR
#ifdef BUSPIRATEV3
#define OOCD_TRST_TRIS BP_PGD_DIR
#else
#define OOCD_TRST_TRIS BP_AUX1_DIR
#endif /* BUSPIRATEV3 */

// io ports
#define OOCD_TDO BP_MISO
#define OOCD_TMS BP_CS
#define OOCD_CLK BP_CLK
#define OOCD_TDI BP_MOSI
#define OOCD_SRST BP_AUX0
#ifdef BUSPIRATEV3
#define OOCD_TRST BP_PGD
#else
#define OOCD_TRST BP_AUX1
#endif /* BUSPIRATEV3 */

// open-drain control
#define OOCD_TDO_ODC BP_MISO
#define OOCD_TMS_ODC BP_CS
#define OOCD_CLK_ODC BP_CLK
#define OOCD_TDI_ODC BP_MOSI
#define OOCD_SRST_ODC BP_AUX0
#ifdef BUSPIRATEV3
#define OOCD_TRST_ODC BP_PGD
#else
#define OOCD_TRST_ODC BP_AUX1
#endif /* BUSPIRATEV3 */

#define CMD_UNKNOWN 0x00
#define CMD_PORT_MODE 0x01
#define CMD_FEATURE 0x02
#define CMD_READ_ADCS 0x03
//#define CMD_TAP_SHIFT     0x04 // old protocol
#define CMD_TAP_SHIFT 0x05
#define CMD_ENTER_OOCD 0x06 // this is the same as in binIO
#define CMD_UART_SPEED 0x07
#define CMD_JTAG_SPEED 0x08

static void binOpenOCDPinMode(unsigned char mode);
static void binOpenOCDHandleFeature(unsigned char feat, unsigned char action);
static void binOpenOCDAnswer(unsigned char *buf, unsigned int len);
extern void binOpenOCDTapShiftFast(unsigned char *in_buf,
                                   unsigned char *out_buf, unsigned int bits,
                                   unsigned int delay);

enum {
  FEATURE_LED = 0x01,
  FEATURE_VREG = 0x02,
  FEATURE_TRST = 0x04,
  FEATURE_SRST = 0x08,
  FEATURE_PULLUP = 0x10
};

enum { SERIAL_NORMAL = 0, SERIAL_FAST = 1 };

enum {
  MODE_HIZ = 0,
  MODE_JTAG = 1,
  MODE_JTAG_OD = 2, // open-drain outputs
};

static unsigned int openocd_jtag_delay;

void binOpenOCD(void) {
  unsigned char *buf =
      bus_pirate_configuration.terminal_input; // for simplicity :)
  unsigned int i, j;
  unsigned char inByte;
  unsigned char inByte2;

  openocd_jtag_delay = 1;

  MSG_OPENOCD_MODE_IDENTIFIER;

  while (1) {
    /*
    this will misbehave when polling is turned off in OpenOCD

                    // enable timeout timer (1sec) taken from AUX measurement
                    T4CON=0;
                    TMR5HLD=0x00; // reset the counter
                    TMR4=0x00;
                    T4CON=0b1000; // 32bit timer
                    PR5=0x1e8;  // 0xf4;
                    PR4=0x4800; // 0x2400;
                    IFS1bits.T5IF=0;

                    // enable timer
                    T4CONbits.TON=1;

                    // wait for byte or timeout
                    while(U1STAbits.URXDA == 0 && IFS1bits.T5IF==0);

                    if (IFS1bits.T5IF==1) {
                            // disable timer
                            T4CONbits.TON=0;
                            // we timeouted, set serial to normal speed
                            UART1Speed(UART_NORMAL_SPEED);
                            return;
                    }

                    // disable timer
                    T4CONbits.TON=0;

                    // read the byte
                    inByte=U1RXREG;
    */
    inByte = user_serial_read_byte();

    switch (inByte) {
    case CMD_UNKNOWN:
      return;
    case CMD_ENTER_OOCD:
      MSG_OPENOCD_MODE_IDENTIFIER;
      break;
    case CMD_READ_ADCS:
      buf[0] = CMD_READ_ADCS;
      buf[1] = 8;
      AD1CON1bits.ADON = 1; // turn ADC ON
      i = bp_read_adc(12);  // ADC pin
      buf[2] = (unsigned char)(i >> 8);
      buf[3] = (unsigned char)(i);
      i = bp_read_adc(11); // VEXT pin
      buf[4] = (unsigned char)(i >> 8);
      buf[5] = (unsigned char)(i);
      i = bp_read_adc(10); // V33 pin
      buf[6] = (unsigned char)(i >> 8);
      buf[7] = (unsigned char)(i);
      i = bp_read_adc(9); // V50 pin
      buf[8] = (unsigned char)(i >> 8);
      buf[9] = (unsigned char)(i);
      AD1CON1bits.ADON = 0; // turn ADC OFF
      binOpenOCDAnswer(buf, 10);
      break;
    case CMD_PORT_MODE:
      inByte = user_serial_read_byte();
      binOpenOCDPinMode(inByte);
      break;
    case CMD_FEATURE:
      inByte = user_serial_read_byte();
      inByte2 = user_serial_read_byte();
      binOpenOCDHandleFeature(inByte, inByte2);
      break;
    case CMD_JTAG_SPEED:
      inByte = user_serial_read_byte();
      inByte2 = user_serial_read_byte();
      openocd_jtag_delay = (inByte << 8) | inByte2;
      break;
    case CMD_UART_SPEED:
      inByte = user_serial_read_byte();
      i = inByte;
      if (inByte == SERIAL_FAST) {
        user_serial_set_baud_rate(UART_FAST_SPEED);
      } else {
        user_serial_set_baud_rate(UART_NORMAL_SPEED);
      }

      inByte = user_serial_read_byte();
      inByte2 = user_serial_read_byte();
      if ((inByte != 0xAA) || (inByte2 != 0x55)) {
        i = SERIAL_NORMAL;
        user_serial_set_baud_rate(UART_NORMAL_SPEED);
      }

      buf[0] = CMD_UART_SPEED;
      buf[1] = (unsigned char)i;
      binOpenOCDAnswer(buf, 2);
      break;
    case CMD_TAP_SHIFT: {
      inByte = user_serial_read_byte();
      inByte2 = user_serial_read_byte();

#if defined(BUSPIRATEV3)

      IFS0bits.U1RXIF = 0; // reset the RX flag

#endif /* BUSPIRATEV3 */

      j = (inByte << 8) | inByte2; // number of bit sequences

      j = min(j, BP_JTAG_OPENOCD_BIT_SEQUENCES_LIMIT);
      buf[0] = CMD_TAP_SHIFT;
      buf[1] = inByte;
      buf[2] = inByte2;
      binOpenOCDAnswer(buf, 3);

#if defined(BUSPIRATEV3)
      i = (j + 7) / 8; // number of bytes used

      // prepare the interrupt transfer
      UART1RXBuf = (unsigned char *)bus_pirate_configuration.terminal_input;
      UART1RXToRecv = 2 * i;
      UART1RXRecvd = 0;

      UART1TXBuf =
          (unsigned char *)(bus_pirate_configuration.terminal_input +
                            2100); // 2048 bytes + 3 command header + to be sure
      UART1TXSent = 0;
      UART1TXAvailable = 0;

      // enable RX interrupt
      IEC0bits.U1RXIE = 1;

      binOpenOCDTapShiftFast(UART1RXBuf, UART1TXBuf, j, openocd_jtag_delay);

#else

      int16_t bit_sequences = (int16_t)(j & 0x7FFF);

      do {
        /* Read TDI and TMS. */

        uint16_t tdi_data_out = user_serial_read_byte();
        uint16_t tms_data_out = user_serial_read_byte();
        tdi_data_out = tdi_data_out |
                       ((bit_sequences > 8) ? user_serial_read_byte() : 0) << 8;
        tms_data_out = tms_data_out |
                       ((bit_sequences > 8) ? user_serial_read_byte() : 0) << 8;

        /* Clock TDI and TMS out, while reading TDO in. */

        size_t bits_to_process = min(16, bit_sequences);
        size_t counter;
        uint16_t tdo_data_in = 0;

        for (counter = 0; counter < bits_to_process; counter++) {
#ifdef BP_JTAG_OPENOCD_DELAY
          __asm volatile("\t repeat %0 \n"
                         "\t nop       \n"
                         :
                         : "r"(openocd_jtag_delay));
#endif /* BP_JTAG_OPENOCD_DELAY */

          /* Clear TCK. */
          OOCD_CLK = LOW;

          /* Output TMS and TDI. */
          OOCD_TDI = (tdi_data_out & 0x0001) ? HIGH : LOW;
          tdi_data_out >>= 1;
          OOCD_TMS = (tms_data_out & 0x0001) ? HIGH : LOW;
          tms_data_out >>= 1;

#ifdef BP_JTAG_OPENOCD_DELAY
          __asm volatile("\t repeat %0 \n"
                         "\t nop       \n"
                         :
                         : "r"(openocd_jtag_delay));
#endif /* BP_JTAG_OPENOCD_DELAY */

          /* Set TCK. */
          OOCD_CLK = HIGH;

          /* Sample TDO. */
          tdo_data_in = (OOCD_TDO << 15) | (tdo_data_in >> 1);
        }

        /* Report TDO. */

        tdo_data_in >>= 15 - (bits_to_process - 1);
        user_serial_transmit_character(tdo_data_in & 0xFF);
        if (bits_to_process > 8) {
          user_serial_transmit_character((tdo_data_in >> 8) & 0xFF);
        }

        bit_sequences -= 16;
      } while (bit_sequences >= 0);

#endif /* BUSPIRATEV4 */

      break;
    }
    default:
      buf[0] = 0x00; // unknown command
      buf[1] = 0x00;
      binOpenOCDAnswer(buf, 1);
      break;
    }
  }
}

static void binOpenOCDPinMode(unsigned char mode) {
  // reset all pins
  OOCD_TMS = 0;
  OOCD_TDI = 0;
  OOCD_CLK = 0;
  OOCD_SRST = 0;
  OOCD_TRST = 0;
  // setup open-drain if necessary
  if (mode == MODE_JTAG_OD) {
    OOCD_TMS_ODC = 1;
    OOCD_CLK_ODC = 1;
    OOCD_TDI_ODC = 1;
    OOCD_SRST_ODC = 1;
    OOCD_TRST_ODC = 1;
  } else {
    OOCD_TMS_ODC = 0;
    OOCD_CLK_ODC = 0;
    OOCD_TDI_ODC = 0;
    OOCD_SRST_ODC = 0;
    OOCD_TRST_ODC = 0;
  }
  // make pins output
  if (mode == MODE_JTAG || mode == MODE_JTAG_OD) {
    OOCD_TMS_TRIS = 0;
    OOCD_TDI_TRIS = 0;
    OOCD_CLK_TRIS = 0;
    OOCD_SRST_TRIS = 0;
    OOCD_TRST_TRIS = 0;
    OOCD_TDO_TRIS = 1;
  } else {
    OOCD_TMS_TRIS = 1;
    OOCD_TDI_TRIS = 1;
    OOCD_CLK_TRIS = 1;
    OOCD_SRST_TRIS = 1;
    OOCD_TRST_TRIS = 1;
    OOCD_TDO_TRIS = 1;
  }
}

static void binOpenOCDAnswer(unsigned char *buf, unsigned int len) {
  unsigned int i;
  for (i = 0; i < len; i++) {
    user_serial_transmit_character(buf[i]);
  }
}

static void binOpenOCDHandleFeature(unsigned char feat, unsigned char action) {
  switch (feat) {
  case FEATURE_LED:
    BP_LEDMODE_DIR = 0; // LED to output
    BP_LEDMODE = action;
    break;
  case FEATURE_VREG:
    if (action) {
      BP_VREG_ON();
    } else {
      BP_VREG_OFF();
    }
    break;
  case FEATURE_PULLUP:
    if (action) {
      BP_PULLUP_ON();
    } else {
      BP_PULLUP_OFF();
    }
    break;
  case FEATURE_TRST:
    OOCD_TRST = action;
    break;
  case FEATURE_SRST:
    OOCD_SRST = action;
    break;
  default:
    break;
  }
}

#endif /* BP_JTAG_OPENOCD_SUPPORT */
