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

#include "messages.h"

#include "base.h"

void bp_message_write_buffer(unsigned long strptr) {
  uint8_t tblpag_prev = TBLPAG;
  uint8_t index = 0;
  char ch = '\0';

  do {
    TBLPAG = (strptr >> 16) & 0xFF;
    switch (index) {
    case 0:
      ch = LO8(__builtin_tblrdl(strptr));
      index++;
      break;

    case 1:
      ch = HI8(__builtin_tblrdl(strptr));
      index++;
      break;

    case 2:
      ch = LO8(__builtin_tblrdh(strptr));
      strptr += 2;
      index = 0;
      break;
    }

    if (ch != '\0') {
      user_serial_transmit_character(ch);
    }
  } while (ch != '\0');

  TBLPAG = tblpag_prev;
}

void bp_message_write_line(unsigned long strptr) {
  bp_message_write_buffer(strptr);
  bpBR;
}

/* Individual sections let the linker discard messages for unused features. */
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
  response = user_serial_read_byte();

  /* Local echo. */
  user_serial_transmit_character(response);
  bpBR;

  /* 'y' and 'Y' are exactly one bit apart. */
  return ((response & 0b11011111) == 'Y');
}
