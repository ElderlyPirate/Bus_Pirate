/*
 * This file is part of the Bus Pirate project
 * (http://code.google.com/p/the-bus-pirate/).
 *
 * Written and maintained by the Bus Pirate project.
 *
 * To the extent possible under law, the project has waived all copyright and
 * related or neighboring rights to Bus Pirate.  This work is published from
 * United States.
 *
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

/**
 * @file aux_pin.h
 *
 * @brief AUX pins handler definition file.
 */

#ifndef BP_AUX_PIN_H
#define BP_AUX_PIN_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Minimum allowed frequency for the PWM generator, in Hz.
 *
 * Setting the PWM generator to this frequency doubles as an OFF value.
 */
#define PWM_MINIMUM_FREQUENCY 0

/**
 * @brief Maximum allowed frequency for the PWM generator, in Hz.
 */
#define PWM_MAXIMUM_FREQUENCY 4000

/**
 * @brief Special frequency value to signal the PWM generator to shut down.
 */
#define PWM_OFF PWM_MINIMUM_FREQUENCY

/**
 * @brief Minimum duty cycle value, as percentage.
 */
#define PWM_MINIMUM_DUTY_CYCLE 1

/**
 * @brief Maximum duty cycle value, as percentage.
 */
#define PWM_MAXIMUM_DUTY_CYCLE 100

/**
 * @brief Updates the internal PWM generation variables.
 *
 * @param[in] frequency  the generated signal frequency in Hz, between
 *                       PWM_MINIMUM_FREQUENCY and PWM_MAXIMUM_FREQUENCY,
 *                       inclusive.  Set to PWM_OFF to stop signal generation.
 * @param[in] duty_cycle the generated signal duty cycle, on a range from
 *                       PWM_MINIMUM_DUTY_CYCLE to PWM_MAXIMUM_DUTY_CYCLE,
 *                       not inclusive.
 *
 * @see PWM_MINIMUM_FREQUENCY
 * @see PWM_MAXIMUM_FREQUENCY
 * @see PWM_MINIMUM_DUTY_CYCLE
 * @see PWM_MAXIMUM_DUTY_CYCLE
 */
void bp_update_pwm(const uint16_t frequency, const uint16_t duty_cycle);

/**
 * @brief Updates the duty cycle of the currently generated PWM signal.
 *
 * @param[in] duty_cycle the generated signal duty cycle, on a range from
 *                       PWM_MINIMUM_DUTY_CYCLE to PWM_MAXIMUM_DUTY_CYCLE,
 *                       not inclusive.
 *
 * @see PWM_MINIMUM_DUTY_CYCLE
 * @see PWM_MAXIMUM_DUTY_CYCLE
 */
void bp_update_duty_cycle(const uint16_t duty_cycle);

/**
 * @brief Sets the currently chosen AUX pin into INPUT/HiZ mode.
 */
void bp_aux_pin_set_high_impedance(void);

/**
 * @brief Pulls the currently chosen AUX pin HIGH.
 */
void bp_aux_pin_set_high(void);

/**
 * @brief Brings the currently chosen AUX pin LOW.
 */
void bp_aux_pin_set_low(void);

/**
 * @breif Reads the state of the currently chosen AUX pin.
 *
 * @return `HIGH` or `LOW` according to the chosen AUX pin state.
 */
bool bp_aux_pin_read(void);

/**
 * @brief Starts the setup process for driving a servomotor.
 */
void bp_servo_setup(void);

/**
 * @brief Starts the setup process for reading frequency values.
 */
void bp_frequency_counter_setup(void);

/**
 * @brief Polls the frequency counter and returns the read frequency.
 *
 * @return the frequency read by the frequency counter hardware, in Hz.
 */
unsigned long bp_measure_frequency(void);

/**
 * @brief Starts the setup process for generating a PWM signal.
 */
void bp_pwm_setup(void);

#endif /* !BP_AUX_PIN_H */
