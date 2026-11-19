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

#ifndef BP_ONBOARD_EEPROM_H
#define BP_ONBOARD_EEPROM_H

#include "configuration.h"

#ifdef BUSPIRATEV4

#include <stdbool.h>

/* BPv4 IC3 is a 24xx64: 8K x 8, addresses 0x0000..0x1FFF.
 * This is opaque storage, not the disabled BASIC filesystem's 24LC256 layout. */
#define ONBOARD_EEPROM_CAPACITY 8192u

/**
 * Initializes the I/O port for communication with the on-board EEPROM.
 */
void eeprom_initialize(void);

/**
 * Tests the onboard EEPROM using a saved byte and its complement at offset 0.
 * The original byte is restored and verified before success; a healthy test
 * uses two write cycles, not a destructive erase/default-value assumption.
 * Requires an initialized board and an unowned (disabled) I2C1 controller.
 * I2C1 is disabled afterward. The previous WP latch value is restored unless
 * rollback cannot be verified, in which case WP stays asserted and false is
 * returned. Callers must report failure, not assume storage was repaired.
 *
 * @return true if the test passed, false otherwise.
 */
bool eeprom_test(void);

#endif /* BUSPIRATEV4 */

#endif /* !BP_ONBOARD_EEPROM_H */
