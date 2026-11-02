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

/**
 * @file raw_common.h
 *
 * @brief Raw 2-Wire/3-Wire common code.
 *
 * This file contains functions that are shared among Raw 2-Wire and Raw 3-Wire
 * modes.  These functions are meant to be aggressively inlined by the compiler,
 * hence the fact they are declared as `static inline` as per C11 inlining
 * rules.
 */

#ifndef BP_RAW_COMMON_H
#define BP_RAW_COMMON_H

#include "configuration.h"

#if defined(BP_ENABLE_RAW_2WIRE_SUPPORT) || defined(BP_ENABLE_RAW_3WIRE_SUPPORT)

#include <stdbool.h>
#include <stdint.h>

#include "base.h"
#include "bitbang.h"

static inline bool raw_read_bit(void) { return bitbang_read_bit(false); }

static inline uint16_t raw_read_pin(void) { return bitbang_read_miso(); }

static inline void raw_set_clock_high(void) { bitbang_set_clk(HIGH); }

static inline void raw_set_clock_low(void) { bitbang_set_clk(LOW); }

static inline void raw_set_data_low(void) { bitbang_set_mosi(LOW); }

static inline void raw_set_data_high(void) { bitbang_set_mosi(HIGH); }

static inline void raw_advance_clock(void) { bitbang_advance_clock_ticks(1); }

#endif /* BP_ENABLE_RAW_2WIRE_SUPPORT || BP_ENABLE_RAW_3WIRE_SUPPORT */

#endif /* !BP_RAW_COMMON_H */
