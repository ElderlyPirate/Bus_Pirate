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
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#ifndef BP_HARDWARE_H
#define BP_HARDWARE_H

#ifdef BUSPIRATEV3
#include "hardwarev3.h"
#endif /* BUSPIRATEV3 */

#ifdef BUSPIRATEV4
#include "core_usb/usb_stack_globals.h"
#include "hardwarev4.h"
#include "onboard_eeprom.h"
#endif /* BUSPIRATEV4 */

#if !defined(BUSPIRATEV3) && !defined(BUSPIRATEV4)
#error "Board type not defined!"
#endif /* !BUSPIRATEV3 && !BUSPIRATEV4 */

#include "types.h"

// calculate the acceptable range of voltages for the power supplies
// these values will be compared to the ADC reading from the supply monitor
#define V5BASE 0x307 //(((5/2)/3.3)*1024))
#define V5H V5BASE + 0x50
#define V5L V5BASE - 0x50

#define V33BASE 0x200 //(((3.3/2)/3.3)*1024))
#define V33H V33BASE + 0x50
#define V33L V33BASE - 0x50

// Standard Output (PPS) Definitions
#define NULL_IO 0
#define C1OUT_IO 1
#define C2OUT_IO 2
#define U1TX_IO 3
#define U1RTS_IO 4
#define U2TX_IO 5
#define U2RTS_IO 6
#define SDO1_IO 7
#define SCK1OUT_IO 8
#define SS1OUT_IO 9
#define SDO2_IO 10
#define SCK2OUT_IO 11
#define SS2OUT_IO 12
#define OC1_IO 18
#define OC2_IO 19
#define OC3_IO 20
#define OC4_IO 21
#define OC5_IO 22

// define device ID and revision memory location
#define DEV_ADDR_UPPER 0x00FF
#define DEV_ADDR_TYPE 0x0000
#define DEV_ADDR_REV 0x0002

static inline void bp_enable_pullup(void) {
  BP_PULLUP_LAT = LOW;
  BP_PULLUP_DIR = INPUT;
}

static inline void bp_disable_pullup(void) {
  BP_PULLUP_LAT = LOW;
  BP_PULLUP_DIR = OUTPUT;
}

static inline void bp_set_pullup_state(bool state) {
  if (state == ON) {
    bp_enable_pullup();
  } else {
    bp_disable_pullup();
  }
}

static inline void bp_enable_voltage_regulator(void) {
  BP_VREGEN_LAT = ON;
  BP_VREGEN_DIR = OUTPUT;
}

static inline void bp_disable_voltage_regulator(void) {
  BP_VREGEN_LAT = OFF;
  BP_VREGEN_DIR = OUTPUT;
}

static inline bool bp_get_voltage_regulator_state(void) {
  return BP_VREGEN;
}

static inline void bp_set_voltage_regulator_state(bool state) {
  if (state == ON) {
    bp_enable_voltage_regulator();
  } else {
    bp_disable_voltage_regulator();
  }
}

/**
 * @brief Bitmask for all the available I/O pins.
 */
#define ALLIO (MOSI + CLK + MISO + CS)

static inline void bp_enable_adc(void) { AD1CON1bits.ADON = ON; }

static inline void bp_disable_adc(void) { AD1CON1bits.ADON = OFF; }

static inline void bp_set_adc_state(bool state) {
  if (state == ON) {
    bp_enable_adc();
  } else {
    bp_disable_adc();
  }
}

static inline void bp_enable_mode_led(void) {
  BP_LEDMODE_LAT = ON;
  BP_LEDMODE_DIR = OUTPUT;
}

static inline void bp_disable_mode_led(void) {
  BP_LEDMODE_LAT = OFF;
  BP_LEDMODE_DIR = OUTPUT;
}

static inline void bp_set_mode_led_state(const bool state) {
  if (state == ON) {
    bp_enable_mode_led();
  } else {
    bp_disable_mode_led();
  }
}

static inline void bp_toggle_mode_led(void) {
  BP_LEDMODE_LAT ^= ON;
  BP_LEDMODE_DIR = OUTPUT;
}

#endif /* !BP_HARDWARE_H */
