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

#include "smps.h"

#ifdef BP_ENABLE_SMPS_SUPPORT

#ifdef BUSPIRATEV4

#include <stdint.h>

#include "base.h"

/**
 * Switched power mode supply module state data holder.
 */
typedef struct {
  /**
   * The requested output voltage from the power supply.
   */
  uint16_t voltage_out;

  /**
   * The last voltage output reading coming in from the power supply adapter
   * module.
   */
  uint16_t voltage_reading;

  /**
   * The requested PWM duty cycle for performing the mode switching.
   */
  uint8_t pwm_duty_cycle;
} __attribute__((packed)) smps_state_t;

/**
 * The switched mode power supply module state.
 */
static smps_state_t smps_state = {0};

void smps_start(unsigned int requested_voltage) {

  /* Get a 7-bits duty cycle value for switching operations. */
  smps_state.pwm_duty_cycle = 128 - (64000 / requested_voltage);

  /* Rescale the voltage to something appropriate for the ADC to compare
   * against. */
  smps_state.voltage_out = requested_voltage * 45 / 58;

  /* Assign the AUX pin to Output Compare 5 */
  BP_AUX1_RPOUT = OC5_IO;

  /* Set the ADC to read from the ADC pin. */
  AD1CHS = BP_ADC_PROBE;

  /* Clear ADC interrupt flag. */
  IFS0bits.AD1IF = OFF;

  /* Enable ADC interrupt. */
  IEC0bits.AD1IE = ON;

  /* Enable auto sampling. */
  AD1CON1bits.ASAM = ON;

  /* Set the duty cycle for the PWM. */
  OC5R = smps_state.pwm_duty_cycle;

  /* Set the time period for the PWM (currently 125kHz). */
  OC5RS = 0x7F;

  /*
   * Set the output comparator as its synchronization source to enter PWM mode
   * (SYNCSEL<4:0> set to 1).
   *
   * MSB
   * 0000xxx000011111
   * ||||   |||||||||
   * ||||   ||||+++++--- SYNCSELx - Trigger source is the same module.
   * ||||   |||+-------- OCTRIS   - Peripheral connected to pin.
   * ||||   ||+--------- TRIGSTAT - Timer source has not been triggered.
   * ||||   |+---------- OCTRIG   - Output Compare synchronized with SYNCSELx.
   * ||||   +----------- OC32     - Cascade module operation disabled.
   * |||+--------------- OCINV    - Output Compare result is not inverted.
   * ||+---------------- FLTTRIEN - pin I/O unaffected by a fault.
   * |+----------------- FLTOUT   - PWM output driven low on a fault.
   * +------------------ FLTMD    - fault mode maintained.
   */
  OC5CON2 = 0x001F;

  /*
   * Set the system timer as the clock source.
   *
   * MSB
   * xx0111xx0xx00110
   *   ||||  |  |||||
   *   ||||  |  ||+++--- OCM      - Edge-aligned PWM mode.
   *   ||||  |  |+------ TRIGMODE - TRIGSTAT cleared by software.
   *   ||||  |  +------- OCFLT0   - No PWM fault condition occurred.
   *   ||||  +---------- ENFLT0   - Fault 0 input disabled.
   *   |+++------------- OCTSEL   - Output Compare Timer is System Clock
   *   +---------------- OCSIDL   - Output Compare stops on idle.
   */
  OC5CON1 = 0x1C06;

  /* Turn ADC on. */
  AD1CON1bits.ADON = ON;
}

void smps_stop(void) {
  /* Turn the ADC off. */
  AD1CON1bits.ADON = OFF;

  /* Disable ADC interrupts. */
  IEC0bits.AD1IE = OFF;

  /* Disable auto sampling. */
  AD1CON1bits.ASAM = OFF;

  /* Set 0% as the PWM duty cycle. */
  OC5R = 0;

  /* Set 0Hz as the PWM period. */
  OC5RS = 0;

  /*
   * Remove module synchronization source.
   *
   * MSB
   * 0000xxx000000000
   * ||||   |||||||||
   * ||||   ||||+++++--- SYNCSELx - Not synchronized with any other module.
   * ||||   |||+-------- OCTRIS   - Peripheral connected to pin.
   * ||||   ||+--------- TRIGSTAT - Timer source has not been triggered.
   * ||||   |+---------- OCTRIG   - Output Compare synchronized with SYNCSELx.
   * ||||   +----------- OC32     - Cascade module operation disabled.
   * |||+--------------- OCINV    - Output Compare result is not inverted.
   * ||+---------------- FLTTRIEN - pin I/O unaffected by a fault.
   * |+----------------- FLTOUT   - PWM output driven low on a fault.
   * +------------------ FLTMD    - fault mode maintained.
   */
  OC5CON2 = 0;

  /*
   * Disable the Output Compare channel.
   *
   * MSB
   * xx0000xx0xx00000
   *   ||||  |  |||||
   *   ||||  |  ||+++--- OCM      - Output Compare channel disabled.
   *   ||||  |  |+------ TRIGMODE - TRIGSTAT cleared by software.
   *   ||||  |  +------- OCFLT0   - No PWM fault condition occurred.
   *   ||||  +---------- ENFLT0   - Fault 0 input disabled.
   *   |+++------------- OCTSEL   - Output Compare Timer is Timer #2.
   *   +---------------- OCSIDL   - Output Compare stops on idle.
   */
  OC5CON1 = 0;

  /* Free the AUX pin from the Output Comparator unit #5. */
  BP_AUX1_RPOUT = NULL_IO;
}

void smps_adc(void) {

  /* Writes the last reading to the serial port. */

  user_serial_transmit_character(smps_state.voltage_reading >> 8);
  user_serial_transmit_character(smps_state.voltage_reading);
}

void __attribute__((interrupt, no_auto_psv)) _ADC1Interrupt() {

  /* Clear ADC interrupt flag. */
  IFS0bits.AD1IF = OFF;

  /* Read data from the ADC. */
  smps_state.voltage_reading = ADC1BUF0;

  /* Turn PWM off if it goes out of spec. */
  OC5R = (ADC1BUF0 > smps_state.voltage_out) ? 0 : smps_state.pwm_duty_cycle;
}

#endif /* BUSPIRATEV4 */

#endif /* BP_ENABLE_SMPS_SUPPORT */
