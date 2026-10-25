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

#include "uart2.h"

#if defined(BP_ENABLE_UART_SUPPORT)

#include "base.h"

/* PPS Settings. */

#define UARTRX_PIN BP_MISO_RPIN
#define UARTTX_PIN BP_MOSI_RPOUT
#define UARTTX_ODC BP_MOSI_ODC

/*
 * UART2 receive buffer (both variants; the U2RX interrupt is available on
 * the v3 and v4 chips alike).
 *
 * The receive side must never depend on what the main loop is doing: the
 * host writes can block for a long time (USB CDC backpressure on the v4,
 * the UART1 transmitter on the v3), and this UART has no hardware FIFO -
 * an unread byte is simply replaced by the next one, so a stalled loop
 * silently drops received data.  The RX interrupt drains the hardware into
 * this ring instead, and the rest of the firmware reads from the ring.
 * The parity/framing status of each character is captured per byte as
 * well: PERR/FERR are read-only status bits for the character at the top
 * of the FIFO, and the interrupt consumes the characters the old code
 * used to inspect.
 */
#define UART2_RX_BUFFER_SIZE 128 /* power of two */
static volatile uint8_t uart2_rx_buffer[UART2_RX_BUFFER_SIZE];
static volatile uint8_t uart2_rx_flag_buffer[UART2_RX_BUFFER_SIZE];
static volatile uint8_t uart2_rx_head = 0;
static volatile uint8_t uart2_rx_tail = 0;
static volatile uint8_t uart2_rx_last_flags = 0;
static volatile uint16_t uart2_rx_overruns = 0;

void __attribute__((interrupt, no_auto_psv)) _U2RXInterrupt(void) {
  uint8_t next;

  /* Clear the interrupt flag. */
  IFS1bits.U2RXIF = OFF;

  /*
   * A hardware overrun means the previous byte was replaced before the
   * interrupt could read it; clearing OERR resets the receiver FIFO and
   * the shift register (per the family datasheet), so only count it.
   */
  if (U2STAbits.OERR) {
    U2STAbits.OERR = OFF;
    uart2_rx_overruns++;
  }

  if (U2STAbits.URXDA) {
    uint8_t flags = (uint8_t)((U2STAbits.PERR ? UART2_RX_FLAG_PARITY : 0) |
                              (U2STAbits.FERR ? UART2_RX_FLAG_FRAMING : 0));

    next = (uint8_t)((uart2_rx_head + 1) & (UART2_RX_BUFFER_SIZE - 1));

    if (next != uart2_rx_tail) {
      uart2_rx_flag_buffer[uart2_rx_head] = flags;
      uart2_rx_buffer[uart2_rx_head] = (uint8_t)(U2RXREG & 0xFF);
      uart2_rx_head = next;
    } else {
      /* Ring full: the byte is read and dropped. */
      (void)U2RXREG;
      uart2_rx_overruns++;
    }
  }
}

void uart2_setup(const uint16_t baud_rate_generator_prescaler,
                 const bool open_drain_output, const bool invert_polarity,
                 const uint8_t databits_and_parity, const bool stop_bits) {

  /* Set open drain output mode. */
  UARTTX_ODC = (open_drain_output == UART2_OPEN_DRAIN) ? OPEN_DRAIN : PUSH_PULL;

  /* Map RB7 as UART TX. */
  RPINR19bits.U2RXR = UARTRX_PIN;
  UARTTX_PIN = U2TX_IO;

  /* Set UART2 baud rate generator prescaler. */
  U2BRG = baud_rate_generator_prescaler;

  /*
   * U2MODE - UART2 MODE REGISTER
   *
   * MSB
   * 0x000x00000C1BBA
   * | ||| ||||||||||
   * | ||| |||||||||+-- STSEL:  One or two stop bits.
   * | ||| |||||||++--- PDSEL:  Data bits and parity.
   * | ||| ||||||+----- BRGH:   High speed mode (baud clock from Fcy/4).
   * | ||| |||||+------ RXINV:  RX Idle state high/low.
   * | ||| ||||+------- ABAUD:  Baud rate measurement disabled.
   * | ||| |||+-------- LPBACK: Loopback mode disabled.
   * | ||| ||+--------- WAKE:   No wake-up enabled.
   * | ||| ++---------- UEN:    CTS and RTS controlled by port latches.
   * | ||+------------- RTSMD:  RTS pin in Flow Control mode.
   * | |+-------------- IREN:   IrDA encoder/decoder disabled.
   * | +--------------- USIDL:  Continue module operation in Idle mode.
   * +----------------- UARTEN: Disable port.
   */
  U2MODE = (MASKBOTTOM8(stop_bits, 1) << _U2MODE_STSEL_POSITION) |
           (MASKBOTTOM8(databits_and_parity, 2) << _U2MODE_PDSEL_POSITION) |
           _U2MODE_BRGH_MASK |
           (MASKBOTTOM8(invert_polarity, 1) << _U2MODE_RXINV_POSITION);

  /*
   * U2STA - UART2 STATUS REGISTER
   *
   * MSB
   * 0A0x00--000---0-
   * ||| ||  |||   |
   * ||| ||  |||   +--- OERR:    Clear overrun error status bit.
   * ||| ||  ||+------- ADDEN:   Address detect mode disabled.
   * ||| ||  ++-------- URXISEL: RX Interrupt on every incoming byte.
   * ||| |+------------ UTXEN:   Transmission disabled.
   * ||| +------------- UTXBRK:  Sync break transmission disabled.
   * +-+--------------- UTXISEL: TX Interrupt on every outgoing byte.
   *  +---------------- UTXINV:  TX Idle state high/low.
   */
  U2STA = MASKBOTTOM8(invert_polarity, 1) << _U2STA_UTXINV_POSITION;
}

void uart2_enable(void) {
  /* Enable UART port. */
  U2MODEbits.UARTEN = ON;

  /* Enable data transmission. */
  U2STAbits.UTXEN = ON;

/* Reset the receive ring and enable the RX interrupt. */
  uart2_rx_head = 0;
  uart2_rx_tail = 0;
  uart2_rx_last_flags = 0;
  uart2_rx_overruns = 0;

  /* Clear UART2 interrupt flag and enable the RX interrupt. */
  IFS1bits.U2RXIF = OFF;
  IEC1bits.U2RXIE = ON;

  /*
   * Tristate bits are ignored by the PPS peripheral, and are set here only to
   * allow the 'v' command to report the right direction.
   */
  BP_MOSI_DIR = OUTPUT;
}

void uart2_disable(void) {
  /* Disable the RX interrupt before touching the port. */
  IEC1bits.U2RXIE = OFF;

  /* Disable UART port. */
  U2MODEbits.UARTEN = OFF;

  /* Enable data transmission. */
  U2STAbits.UTXEN = OFF;

  /* Map pin RB7 as UART2 RX. */
  RPINR19bits.U2RXR = 0b11111;

  /* Remove PPS assignment. */
  UARTTX_PIN = OFF;
  UARTTX_ODC = OFF;

  /*
   * Tristate bits are ignored by the PPS peripheral, and are set here only to
   * allow the 'v' command to report the right direction.
   */
  BP_MOSI_DIR = INPUT;
}

void uart2_tx(const uint8_t character) {
  /* Wait until the TX buffer is empty. */
  while (U2STAbits.UTXBF == ON) {
  }

  U2TXREG = character;
}

uint8_t uart2_rx(void) {
  uint8_t character;

  /* Wait until a character is available in the ring. */
  while (uart2_rx_head == uart2_rx_tail) {
  }

  character = uart2_rx_buffer[uart2_rx_tail];
  uart2_rx_last_flags = uart2_rx_flag_buffer[uart2_rx_tail];
  uart2_rx_tail = (uint8_t)((uart2_rx_tail + 1) & (UART2_RX_BUFFER_SIZE - 1));
  return character;
}

inline bool uart2_rx_ready(void) { return uart2_rx_head != uart2_rx_tail; }

uint8_t uart2_rx_flags(void) { return uart2_rx_last_flags; }

uint16_t uart2_rx_overrun_count(void) { return uart2_rx_overruns; }

#endif /* BP_ENABLE_UART_SUPPORT */
