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

#include <stdbool.h>
#include <stdint.h>

#include "selftest.h"

#include "base.h"
#include "core.h"

/**
 * How many milliseconds to wait before reading all pins once set.
 */
#define PIN_STATE_TEST_DELAY 100

/**
 * How many milliseconds to wait before reading power pins status once set.
 */
#define PWR_STATE_TEST_DELAY 2

/**
 * Checks whether the given test value matches the expected result, and prints
 * the result to the serial port.
 *
 * If the values do not match, the internal error counter gets incremented by
 * one.
 *
 * @param[in] obtained the value obtained by the test procedure.
 * @param[in] expected the value that was meant to be obtained.
 */
void check_result(bool obtained, bool expected);

/**
 * Checks whether all pins are actually set to the same given state.
 *
 * @todo Check AUX pin too on v3
 * @todo Check AUX1/AUX2 pins too on v4
 *
 * @param[in] state the pin state to check pins against.
 */
void perform_pins_state_test(bool state);

/**
 * Takes an ADC measurement and checks whether the result is within the set
 * threshold range.
 *
 * @param[in] channel the channel to read from.
 * @param[in] minimum_threshold the minimum acceptable value.
 * @param[in] maximum_threshold the maximum acceptable value.
 */
void perform_adc_test(unsigned int channel, unsigned int minimum_threshold,
                      unsigned int maximum_threshold);

/**
 * Detected errors counter.
 */
static uint8_t errors;

/**
 * Board configuration information.
 */
extern bus_pirate_configuration_t bus_pirate_configuration;

uint8_t perform_selftest(bool show_progress, bool jumper_test) {

  errors = 0;
  if (!show_progress) {
    bus_pirate_configuration.quiet = true;
  }

  /* Alert the user to perform the required manual operations if needed. */

  if (show_progress && jumper_test) {

    /* Print alert. */
    BPMSG1163;
    BPMSG1251;

    /* Wait for a character to come in. */
    user_serial_read_byte();
  }

  /* Start the test procedure. */

  BPMSG1164;

  /* Check whether AUX0 goes HIGH when requested. */

  BP_AUX0_LAT = HIGH;
  BP_AUX0_DIR = OUTPUT;
  BPMSG1165;
  check_result(BP_AUX0, HIGH);
  BP_AUX0_LAT = LOW;
  BP_AUX0_DIR = INPUT;

  /* Check whether the LED line goes HIGH when requested. */

  BP_LEDMODE = HIGH;
  BP_LEDMODE_DIR = OUTPUT;
  BPMSG1166;
  check_result(BP_LEDMODE, HIGH);
  BP_LEDMODE = LOW;

  /* Check whether the pull-up line goes HIGH when requested. */

  bp_enable_pullup();
  BPMSG1167;
  check_result(BP_PULLUP, HIGH);

  /* Check whether the pull-up line goes LOW when requested. */

  bp_disable_pullup();
  BPMSG1168;
  check_result(BP_PULLUP, LOW);

  /* Check whether the regulated voltage line goes HIGH when requested. */

  bp_enable_voltage_regulator();
  bp_delay_ms(PWR_STATE_TEST_DELAY);
  BPMSG1169;
  check_result(BP_VREGEN, HIGH);

#ifdef BUSPIRATEV4

  /* Test the internal EEPROM. */

  BPMSG1265;

  /* Check the I2C flash clock line. */
  BPMSG1266;
  check_result(BP_EE_SCL, HIGH);

  /* Check the I2C flash data line. */
  BPMSG1267;
  check_result(BP_EE_SDA, HIGH);

  /* Check the I2C flash WRITE PROTECT line. */
  BPMSG1268;
  check_result(BP_EE_WP, HIGH);

  /* Performs a more complete EEPROM test. */

  BPMSG1269;
  check_result(eeprom_test(), true);

#endif /* BUSPIRATEV4 */

  /* ADC check. */

  BPMSG1170;

  /* Turn ADC on. */
  bp_enable_adc();

#ifdef BUSPIRATEV4

  /* Check whether the voltage coming in from the USB port is within range. */

  BPMSG1270;
  perform_adc_test(BP_ADC_USB, V5L, V5H);

#endif /* BUSPIRATEV4 */

  /* Check whether the +5v rail output is within range. */

  BPMSG1171;
  perform_adc_test(BP_ADC_5V0, V5L, V5H);

#ifdef BUSPIRATEV4

  /* Test the +5v pull-up line. */

  bp_enable_5v0_pullup();
  BPMSG1171;
  bpSP;
  BPMSG1172;
  bp_delay_ms(PWR_STATE_TEST_DELAY);
  perform_adc_test(BP_ADC_VPU, V5L, V5H);
  bp_disable_5v0_pullup();

  if (jumper_test) {

    /*
     * Check whether the +3.3v rail output is within range when measured from
     * outside the board circuitry, once a jumper wire is manually placed
     * between the +3.3v rail pin and the ADC input pin.
     */

    BPMSG1174;
    perform_adc_test(BP_ADC_PROBE, V33L, V33H);
  }

  /*
   * Check whether the +3.3v rail output is within range when measured from
   * inside the board circuitry.
   */

  BPMSG1173;
  perform_adc_test(BP_ADC_3V3, V33L, V33H);

  /* Test the +3.3v pull-up line. */

  bp_enable_3v3_pullup();
  BPMSG1173;
  bpSP;
  BPMSG1172;
  bp_delay_ms(PWR_STATE_TEST_DELAY);
  perform_adc_test(BP_ADC_VPU, V33L, V33H);
  bp_disable_3v3_pullup();

#elif defined(BUSPIRATEV3)

  if (jumper_test) {

    /*
     * Check whether the +5v pull-up output is within range when measured from
     * outside the board circuitry, once a jumper wire is manually placed
     * between the +5v pull-up pin and the ADC input pin.
     */

    BPMSG1172;
    perform_adc_test(BP_ADC_VPU, V5L, V5H);
  }

  /* Check whether the +3.3v rail output is within range. */

  BPMSG1173;
  perform_adc_test(BP_ADC_3V3, V33L, V33H);

  if (jumper_test) {

    /*
     * Check whether the +3.3v rail output is within range when measured from
     * outside the board circuitry, once a jumper wire is manually placed
     * between the +3.3v rail pin and the ADC input pin.
     */

    BPMSG1174;
    perform_adc_test(BP_ADC_PROBE, V33L, V33H);
  }

#endif /* BUSPIRATEV4 || BUSPIRATEV3 */

  /* Turn ADC off. */
  bp_disable_adc();

  /*
   * Pull all I/O pins HIGH with pull-ups deactivated, and check the pins
   * state afterwards.
   */

  BPMSG1175;
  IODIR &= ~ALLIO;
  IOLAT |= ALLIO;
  bp_delay_ms(PIN_STATE_TEST_DELAY);
  perform_pins_state_test(HIGH);

  /*
   * Pull all I/O pins LOW with pull-ups active and check the pins state
   * afterwards.
   */

  BPMSG1176;
  IOLAT &= ~ALLIO;
  if (jumper_test) {
    bp_enable_3v3_pullup();
    bp_enable_pullup();
  }
  bp_delay_ms(PIN_STATE_TEST_DELAY);
  perform_pins_state_test(LOW);

  if (jumper_test) {
    /*
     * Pull all I/O pins HIGH with pull-ups active and check the pins state
     * afterwards.
     */

    /* TODO: should this be done without jumpers too? */

    BPMSG1177;
    IODIR |= ALLIO;
    bp_delay_ms(PIN_STATE_TEST_DELAY);
    perform_pins_state_test(HIGH);
    bp_disable_3v3_pullup();
  }

  if (show_progress && jumper_test) {

    /*
     * Alert the user to check the LED on/off states, and prompt for a key
     * to be pressed to continue.
     */

    bp_enable_voltage_regulator();
    bp_enable_mode_led();
    bp_enable_usb_led();
    BPMSG1178;
    MSG_ANY_KEY_TO_EXIT_PROMPT;
    user_serial_read_byte();
    bp_disable_usb_led();
    bp_disable_mode_led();
    bp_disable_voltage_regulator();
  }

  bp_reset_board_state();

  BPMSG1179;
  bp_write_dec_byte(errors);
  BPMSG1180;
  bus_pirate_configuration.quiet = false;

  return errors;
}

void perform_adc_test(unsigned int channel, unsigned int minimum_threshold,
                      unsigned int maximum_threshold) {

  unsigned int measurement;

  user_serial_transmit_character('(');
  measurement = bp_read_adc(channel);
  bp_write_voltage(measurement);
  user_serial_transmit_character(')');
  check_result(
      ((measurement > minimum_threshold) && (measurement < maximum_threshold)),
      true);
}

void perform_pins_state_test(bool state) {

  /* Check MOSI pin state. */
  BPMSG1181;
  check_result(BP_MOSI, state);

  /* Check CLK pin state. */
  BPMSG1182;
  check_result(BP_CLK, state);

  /* Check MISO pin state. */
  BPMSG1183;
  check_result(BP_MISO, state);

  /* Check CS pin state. */
  BPMSG1184;
  check_result(BP_CS, state);
}

void check_result(bool obtained, bool expected) {
  if (obtained == expected) {
    BPMSG1185;
  } else {
    BPMSG1186;
    errors++;
  }
}
