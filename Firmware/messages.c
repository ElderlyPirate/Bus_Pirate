/*
 * This file is part of the Bus Pirate project (http://code.google.com/p/the-bus-pirate/).
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

#include "messages.h"

#include "base.h"

void bp_message_write_buffer(void (*strptr)(void), size_t length) {
    size_t index;
    size_t str = (size_t)strptr;

    for (index = 0; index < length; index++) {
        switch (index % 3) {
            case 0:
               UART1TX(__builtin_tblrdl(str+((index/3)<< 1))
                       & 0xFF);
               break;

            case 1:
                UART1TX((__builtin_tblrdl(str+((index/3)<< 1)) >> 8)
                        & 0xFF);
                break;

            case 2:
                UART1TX(__builtin_tblrdh(str+((index/3)<< 1))
                        & 0xFF);
                break;

            default:
                break;
        }
    }
}

void bp_message_write_line(void (*strptr)(void), size_t length) {
    bp_message_write_buffer(strptr, length);
    bpBR;
}

/* @todo Check if merging all the strings can save some space or not. */
void print_help(void) {
    HLP1000;
    HLP1001;
    HLP1002;
    HLP1003;
    HLP1004;
    HLP1005;
    HLP1006;
    HLP1007;
    HLP1008;
    HLP1009;
    HLP1010;
    HLP1011;
    HLP1012;
    HLP1013;
    HLP1014;
    HLP1015;
    HLP1016;
    HLP1017;
    HLP1018;
    HLP1019;
    HLP1020;
    HLP1021;
    HLP1022;
}

bool agree(void) {
    char response;

    /* Prompt the user. */
    BPMSG1135;

    /* Read the response. */
    response = UART1RX();

    /* Local echo. */
    UART1TX(response);
    bpBR;

    /* 'y' and 'Y' are exactly one bit apart. */
    return ((response & 0b11011111) == 'Y');
}
