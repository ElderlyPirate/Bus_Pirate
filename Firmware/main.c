/*
 * This file is part of the Bus Pirate project
 * (http://code.google.com/p/the-bus-pirate/).
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

#include <stdint.h>

#include "base.h"
#include "basic.h"
#include "core.h"
#include "proc_menu.h"
#include "selftest.h"

#ifdef BUSPIRATEV4

#include "descriptors.h"

void usb_suspend(void);
extern volatile uint8_t usb_device_state;

#endif /* BUSPIRATEV4 */

/**
 * Perform board hardware initialization.
 */
static void initialize_board(void);

/**
 * Internal terminal buffer area.
 *
 * Also used as the SUMP sample buffer, which stores samples as 16-bit
 * packed words: keep it word-aligned.
 */
static uint8_t bp_buffer[BP_TERMINAL_BUFFER_SIZE]
    __attribute__((section(".bss.end"), aligned(2)));

/**
 * Global configuration data holder.
 */
bus_pirate_configuration_t bus_pirate_configuration = {.terminal_input =
                                                           bp_buffer};

/**
 * Mode-specific configuration data holder.
 */
mode_configuration_t mode_configuration;

/**
 * The last command being input by the user.
 */
command_t last_command;

#define FIRMWARE_SIGNATURE 0x31415926

#ifdef BUSPIRATEV4
static uint32_t __attribute__((address(0x47FC), persistent)) firmware_signature;
#else
static uint32_t __attribute__((address(0x27FC), persistent)) firmware_signature;
#endif /* BUSPIRATEV4 */

#pragma code

int main(void) {
  firmware_signature = FIRMWARE_SIGNATURE;

  initialize_board();
  bp_enable_usb_led();

#if defined(BUSPIRATEV4)

#ifdef USB_INTERRUPTS
  EnableUsbPerifInterrupts(USB_TRN | USB_SOF | USB_UERR | USB_URST);
  EnableUsbGlobalInterrupt();
#endif /* USB_INTERRUPTS */

  /* Wait until the USB interface is configured. */

  do {
#ifndef USB_INTERRUPTS
    usb_handler();
#endif /* !USB_INTERRUPTS */
  } while (usb_device_state < CONFIGURED_STATE);

  usb_register_sof_handler(CDCFlushOnTimeout);

#endif /* BUSPIRATEV4 */

  bp_disable_usb_led();

  /* Starts processing user requests. */
  serviceuser();

  return 0;
}

#ifdef BUSPIRATEV3

/**
 * Table for all possible v3 hardware versions.
 */
static const uint8_t BPV3_HARDWARE_VERSION_TABLE[] = {
    /* RB3 == 0 && RB2 == 0 - v3.5 */
    '5',
    /* RB3 == 0 && RB2 == 1 - Unknown */
    '?',
    /* RB3 == 1 && RB2 == 0 - v3b */
    'b',
    /* RB3 == 1 && RB2 == 1 - v2go, v3a */
    'a'};

#endif /* BUSPIRATEV3 */

void initialize_board(void) {
  volatile unsigned long delay = 0xFFFF;

#ifdef BUSPIRATEV3
  /* Set clock divider to 0. */
  CLKDIVbits.RCDIV0 = 0;

  /* All pins are digital. */
  AD1PCFG = 0xFFFF;
#elif defined(BUSPIRATEV4)

  /* Disable nested interrupts. */
  INTCON1bits.NSTDIS = ON;

  /* Set PLL prescaler to 1:1 ratio. */
  CLKDIV = 0x0000;
  BP_LEDUSB_DIR = OUTPUT;

  /* Initialize program space visibility. */
  CORCONbits.PSV = ON;

  /* Map top part of the firmware. */
  PSVPAG = 0;

  /* Disable secondary oscillator. */
  OSCCONbits.SOSCEN = OFF;

  /* Set pins B0, B1, B2, B5, and B15 as analog, rest are digital. */
  AD1PCFGL = 0x7FD8;

  /* Only enable Vbg/2 channel for input scan. */
  AD1PCFGH = 0x0002;
#endif /* BUSPIRATEV3 || BUSPIRATEV4 */

  /* Disable secondary oscillator. */
  OSCCONbits.SOSCEN = OFF;

  /* Wait a bit. */
  while (delay--) {
  }

  /* Set up delay timer. */
  bp_initialise_delay_timer();

  /* Set up the UART port pins. */

#ifdef BUSPIRATEV3
  BP_TERM_RX = BP_TERM_RX_RP;
  BP_TERM_TX_RP = BP_TERM_TX;
#endif /* BUSPIRATEV3 */

  /* Set the UART port speed to 115200 bps. */
  bus_pirate_configuration.terminal_speed = 8;

  /* Default to print numbers in hexadecimal format. */
  bus_pirate_configuration.display_mode = HEX;

  /* Start from a known, clear state. */
  bp_reset_board_state();

#ifdef BUSPIRATEV3
  /* Initialize the internal UART port. */
  user_serial_initialise();
#endif /* BUSPIRATEV3 */

#if defined(BUSPIRATEV4)

  /* Initialize the USB-based serial port. */

  initCDC();
  usb_init(cdc_device_descriptor, cdc_config_descriptor, cdc_str_descs,
           USB_NUM_STRINGS);
  usb_start();
#endif /* BUSPIRATEV4 */

#ifdef BUSPIRATEV3
  /* Turn pull-ups ON. */
  CNPU1 |= _CNPU1_CN6PUE_MASK | _CNPU1_CN7PUE_MASK;
#endif /* BUSPIRATEV3 */

  /* Read device type and revision. */

  bus_pirate_configuration.device_type =
      bp_read_from_flash(DEV_ADDR_UPPER, DEV_ADDR_TYPE);
  bus_pirate_configuration.device_revision =
      bp_read_from_flash(DEV_ADDR_UPPER, DEV_ADDR_REV);

#ifdef BUSPIRATEV3
  /* Get the revision identifier. */
  bus_pirate_configuration.hardware_version =
      BPV3_HARDWARE_VERSION_TABLE[(PORTB >> 2) & 0b00000011];

  /* Turn pull-ups OFF. */
  CNPU1 &= ~(_CNPU1_CN6PUE_MASK | _CNPU1_CN7PUE_MASK);
#else
  bus_pirate_configuration.hardware_version = 0;
#endif /* BUSPIRATEV3 */

  bus_pirate_configuration.quiet = OFF;
  mode_configuration.numbits = 8;

#ifdef BP_ENABLE_BASIC_SUPPORT
  bp_basic_initialize();
#endif /* BP_ENABLE_BASIC_SUPPORT */

  /* Move to page #0. */
  TBLPAG = 0;

#ifndef BUSPIRATEV4
  bpBR;
  print_version_info();
#endif /* !BUSPIRATEV4 */
}

#ifdef BUSPIRATEV4

void usb_suspend(void) {}

#pragma interrupt _USB1Interrupt

void __attribute__((interrupt, no_auto_psv)) _USB1Interrupt() {

  /* Handle USB operation. */
  usb_handler();

  /* Clear the USB Interrupt flag. */
  IFS5bits.USB1IF = OFF;
}

#pragma code

#endif /* BUSPIRATEV4 */
