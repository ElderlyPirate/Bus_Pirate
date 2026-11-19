/*
 * This file is part of the Bus Pirate project
 * (http://code.google.com/p/the-bus-pirate/).
 *
 * Written and maintained by the Bus Pirate project.
 *
 * To the extent possible under law, the project has
 * waived all copyright and related or neighboring rights to Bus Pirate. This
 * work is published from United States.
 *
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef BP_SELFTEST_H
#define BP_SELFTEST_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Performs the board self-test procedure.
 *
 * @param[in] show_progress pass in true if the self-test should print
 *                          progress information during the procedure,
 *                          pass in false for not reporting any progress.
 * @param[in] jumper_test flag to indicate whether to include test passes that
 *                        require user interaction.
 * @return the number of detected errors.
 */
uint8_t perform_selftest(bool show_progress, bool jumper_test);

#endif /* !BP_SELFTEST_H */