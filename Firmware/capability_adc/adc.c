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

/**
 * @file adc.c
 *
 * @brief ADC capability implementation (split from base.c): the read
 * primitive and the terminal probe front ends.
 */

#include "adc.h"

#include "base.h"

uint16_t bp_read_adc(const uint16_t channel) {
  /* Retire the previous owner's transaction before selecting a new input. */
  AD1CON1bits.ADON = OFF;
  AD1CHS = channel;
  AD1CON1bits.DONE = OFF;
  AD1CON1bits.ADON = ON;

  /* DS39705B 17.4.5: analog startup precedes sampling (300ns typical).
   * Five cycles provide at least 312.5ns at the maximum 16MHz FCY. */
  Nop();
  Nop();
  Nop();
  Nop();
  Nop();
  AD1CON1bits.SAMP = ON;

  /* A prior DONE is not completion of this input's acquisition. */
  while (AD1CON1bits.SAMP == ON) {
  }

  /* Wait for conversion to finish. */
  while (AD1CON1bits.DONE == OFF) {
  }

  /* Return value. */
  return ADC1BUF0;
}

void bp_adc_probe(void) {
  if (!bp_adc_check_foreground()) {
    return;
  }

  /* Turn the ADC on. */
  bp_enable_adc();

  /* Perform the measurement. */
  uint16_t measurement = bp_read_adc(BP_ADC_PROBE);
  bp_write_voltage(measurement);
  MSG_VOLTAGE_UNIT;

  /* Turn the ADC off. */
  bp_disable_adc();

  /* Report the raw ADC value alongside the conversion, as asked for in
     issue #148. */
  bp_write_string(" (raw ");
  bp_write_dec_word(measurement);
  bp_write_string(")");
}

void bp_adc_continuous_probe(void) {
  uint16_t measurement;

  if (!bp_adc_check_foreground()) {
    return;
  }

  MSG_ADC_VOLTMETER_MODE;
  MSG_ANY_KEY_TO_EXIT_PROMPT;
  MSG_ADC_VOLTAGE_PROBE_HEADER;
  bp_write_voltage(0);
  MSG_VOLTAGE_UNIT;

  /* Perform ADC probes until a character is sent to the serial port. */
  while (!user_serial_ready_to_read()) {
    /* Turn the ADC on. */
    AD1CON1bits.ADON = ON;

    /* Perform the measurement. */
    measurement = bp_read_adc(BP_ADC_PROBE);

    /* Turn the ADC off. */
    AD1CON1bits.ADON = OFF;

    /* Erase previous measurement. */
    bp_write_string("\x08\x08\x08\x08\x08");

    /* Print new measurement. */
    bp_write_voltage(measurement);
    MSG_VOLTAGE_UNIT;
  }

  /* Flush the incoming serial buffer. */
  user_serial_read_byte();
  bpBR;
}