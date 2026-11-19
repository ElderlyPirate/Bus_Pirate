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
 * @file adc.h
 *
 * @brief ADC capability declarations (split from base.c): the read
 * primitive and the terminal probe front ends.
 */

#ifndef BP_ADC_H
#define BP_ADC_H

#include <stdint.h>

/**
 * @brief Reads a value from the ADC on the given channel.
 *
 * @warning Requires an initialized, foreground-owned manual ADC, with no
 *          concurrent auto-sampling/interrupt owner. The converter is restarted
 *          internally to retire pending results and select the requested input
 *          before sampling. It remains enabled on return; the caller disables
 *          it when the manual measurement session is complete.
 *          Callers must check bp_adc_foreground_available() before enabling or
 *          disabling the ADC; terminal clients use bp_adc_check_foreground().
 *
 * @param[in] channel the channel to read data from.
 * @return the value read from the channel.
 */
uint16_t bp_read_adc(const uint16_t channel);

/**
 * @brief Takes one single ADC measurement and prints it to the serial port.
 */
void bp_adc_probe(void);

/**
 * @brief Takes ADC measurements and prints them to the serial port until a byte
 * is sent to the serial port.
 */
void bp_adc_continuous_probe(void);

#endif /* BP_ADC_H */