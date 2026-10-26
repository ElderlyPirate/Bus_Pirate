/*
 * This file is part of the Bus Pirate project
 * (http://code.google.com/p/the-bus-pirate/).
 *
 * Initially written by Chris van Dongen, 2010.
 *
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
 * @file basic.c
 *
 * @brief BASIC interpreter functions implementation file.
 */

#include "basic.h"

#ifdef BP_ENABLE_BASIC_SUPPORT

#if defined(BUSPIRATEV3) && defined(BP_BASIC_I2C_FILESYSTEM)
#warning "BP_BASIC_I2C_FILESYSTEM is not supported on v3 boards!"
#undef BP_BASIC_I2C_FILESYSTEM
#endif /* BP_BASIC_I2C_FILESYSTEM */

#if BP_BASIC_PROGRAM_SPACE <= 0
#error "Invalid BASIC program space value"
#endif /* BP_BASIC_PROGRAM_SPACE <= 0 */

#if BP_BASIC_NESTED_FOR_LOOP_COUNT <= 1
#error "Invalid nested BASIC FOR-LOOP count"
#endif /* BP_BASIC_NESTED_FOR_LOOP_COUNT <= 1 */

#if BP_BASIC_STACK_FRAMES_DEPTH <= 1
#error "Invalid BASIC stack depth"
#endif /* BP_BASIC_STACK_FRAMES_DEPTH <= 1*/

/**
 * @brief How many variables the BASIC interpreter can handle.
 *
 * Currently set to 26 to handle variables identified from 'A' to 'Z'.
 */
#define BP_BASIC_VARIABLES_COUNT 26

#include "aux_pin.h"
#include "base.h"
#include "bitbang.h"
#include "core.h"
#include "proc_menu.h"

#define INVALID_TOKEN 0x00

#define TOKENS 0x80
#define TOK_LET 0x80
#define TOK_IF 0x81
#define TOK_THEN 0x82
#define TOK_ELSE 0x83
#define TOK_GOTO 0x84
#define TOK_GOSUB 0x85
#define TOK_RETURN 0x86
#define TOK_REM 0x87
#define TOK_PRINT 0x88
#define TOK_INPUT 0x89
#define TOK_FOR 0x8A
#define TOK_TO 0x8B
#define TOK_NEXT 0x8C
#define TOK_READ 0x8D
#define TOK_DATA 0x8E
#define TOK_STARTR 0x8F
#define TOK_START 0x90
#define TOK_STOPR 0x91
#define TOK_STOP 0x92
#define TOK_SEND 0x93
#define TOK_RECEIVE 0x94
#define TOK_CLK 0x95
#define TOK_DAT 0x96
#define TOK_BITREAD 0x97
#define TOK_ADC 0x98
#define TOK_AUXPIN 0x99
#define TOK_PSU 0x9A
#define TOK_PULLUP 0x9B
#define TOK_DELAY 0x9C

#define TOK_AUX 0x9D
#define TOK_FREQ 0x9E
#define TOK_DUTY 0x9F

#define TOK_MACRO 0xA0
#define TOK_END 0xA1

/*
 * `RESTORE` resets the DATA read pointer so a program can read the same DATA
 * sequence again. 0xA2 was free: statement tokens run from 0x80 to 0xA1 and the
 * statement length marker starts at 0xE0, so programs already stored are
 * unaffected.
 */
#define TOK_RESTORE 0xA2

/*
 * `MODE` switches the bus mode from within a running program, or reports the
 * mode that is currently active. 0xA3 is the next free statement token: the
 * statement length marker starts at 0xE0.
 */
#define TOK_MODE 0xA3

#define TOK_LEN 0xE0

#define NUMTOKEN (TOK_MODE - TOKENS) + 1

#define STAT_LET "LET"
#define STAT_IF "IF"
#define STAT_THEN "THEN"
#define STAT_ELSE "ELSE"
#define STAT_GOTO "GOTO"
#define STAT_GOSUB "GOSUB"
#define STAT_RETURN "RETURN"
#define STAT_REM "REM"
#define STAT_PRINT "PRINT"
#define STAT_INPUT "INPUT"
#define STAT_FOR "FOR"
#define STAT_TO "TO"
#define STAT_NEXT "NEXT"
#define STAT_END "END"
#define STAT_READ "READ"
#define STAT_DATA "DATA"
#define STAT_RESTORE "RESTORE"
#define STAT_MODE "MODE"
#define STAT_START "START"
#define STAT_STARTR "STARTR"
#define STAT_STOP "STOP"
#define STAT_STOPR "STOPR"
#define STAT_SEND "SEND"
#define STAT_RECEIVE "RECEIVE"
#define STAT_CLK "CLK"
#define STAT_DAT "DAT"
#define STAT_BITREAD "BITREAD"
#define STAT_ADC "ADC"
#define STAT_AUX "AUX"
#define STAT_PSU "PSU"
#define STAT_PULLUP "PULLUP"
#define STAT_DELAY "DELAY"

#define STAT_AUXPIN "AUXPIN"
#define STAT_FREQ "FREQ"
#define STAT_DUTY "DUTY"
#define STAT_MACRO "MACRO"

/**
 * @brief Status codes enumeration for the internal BASIC parser.
 */
typedef enum {

  /**
   * No status has been obtained yet.
   */
  STATUS_CODE_UNKNOWN = 0,

  /**
   * An `END` statement was reached whilst parsing the program.
   */
  STATUS_CODE_FINISHED,

  /**
   * A partial instruction was found in the program listing.
   */
  STATUS_CODE_PARTIAL_INSTRUCTION_ERROR,

  /**
   * A syntax error was found.
   */
  STATUS_CODE_SYNTAX_ERROR,

  /**
   * An invalid `FOR` statement was parsed.
   */
  STATUS_CODE_FOR_ERROR,

  /**
   * An invalid `NEXT` statement was parsed.
   */
  STATUS_CODE_NEXT_ERROR,

  /**
   * An invalid `GOTO` statement was parsed.
   */
  STATUS_CODE_GOTO_ERROR,

  /**
   * A call stack overflow was observed.
   */
  STATUS_CODE_STACK_ERROR,

  /**
   * An invalid `RETURN` statement was parsed.
   */
  STATUS_CODE_RETURN_ERROR,

  /**
   * One or more invalid `DATA` statements were found.
   */
  STATUS_CODE_DATA_ERROR,

  /**
   * The running program was stopped from the terminal with Ctrl-C.
   */
  STATUS_CODE_INTERRUPTED
} __attribute__((packed)) basic_status_code_t;

/**
 * @brief Flag indicating that the chosen line number was not found.
 */
#define LINE_NUMBER_NOT_FOUND NO

/**
 * @brief Flag indicating that the chosen line number was found.
 */
#define LINE_NUMBER_FOUND YES

extern bus_pirate_configuration_t bus_pirate_configuration;
extern mode_configuration_t mode_configuration;
extern command_t last_command;
extern bus_pirate_protocol_t enabled_protocols[ENABLED_PROTOCOLS_COUNT];

typedef struct {
  uint16_t from;
  uint16_t var;
  uint16_t to;
} __attribute__((packed)) basic_for_loop_t;

static int16_t basic_variables[BP_BASIC_VARIABLES_COUNT];
static int16_t basic_stack[BP_BASIC_STACK_FRAMES_DEPTH];
static basic_for_loop_t basic_nested_for_loops[BP_BASIC_NESTED_FOR_LOOP_COUNT];
static uint16_t basic_program_counter;
static uint16_t basic_current_nested_for_index;
static uint16_t basic_current_stack_frame;

/**
 * @brief Offset variable for `DATA` statements.
 */
static uint16_t basic_data_read_pointer;

static char *tokens[NUMTOKEN + 1] = {
    STAT_LET,     // 0x80
    STAT_IF,      // 0x81
    STAT_THEN,    // 0x82
    STAT_ELSE,    // 0x83
    STAT_GOTO,    // 0x84
    STAT_GOSUB,   // 0x85
    STAT_RETURN,  // 0x86
    STAT_REM,     // 0x87
    STAT_PRINT,   // 0x88
    STAT_INPUT,   // 0x89
    STAT_FOR,     // 0x8A
    STAT_TO,      // 0x8b
    STAT_NEXT,    // 0x8c
    STAT_READ,    // 0x8d
    STAT_DATA,    // 0x8e
    STAT_STARTR,  // 0x8f
    STAT_START,   // 0x90
    STAT_STOPR,   // 0x91
    STAT_STOP,    // 0x92
    STAT_SEND,    // 0x93
    STAT_RECEIVE, // 0x94
    STAT_CLK,     // 0x95
    STAT_DAT,     // 0x96
    STAT_BITREAD, // 0x97
    STAT_ADC,     // 0x98
    STAT_AUXPIN,  // 0x99
    STAT_PSU,     // 0x9a
    STAT_PULLUP,  // 0x9b
    STAT_DELAY,   // 0x9c
    STAT_AUX,     // 0x9d
    STAT_FREQ,    // 0x9e
    STAT_DUTY,    // 0x9f

    STAT_MACRO, // 0xA0
    STAT_END,   // 0xA1
    STAT_RESTORE, // 0xa2
    STAT_MODE,    // 0xa3
};

/**
 * @brief BASIC program data area.
 */
static uint8_t basic_program_area[BP_BASIC_PROGRAM_SPACE] = {0};

/**
 * @brief Scans the currently loaded program to find the token index of the
 * given line number.
 *
 * @param[in] line the line number to obtain an offset for.
 * @param[out] result the offset in program memory for the line in question.
 *
 * @return LINE_NUMBER_FOUND if a matching line was found in program memory.
 * @return LINE_NUMBER_NOT_FOUND if no matching line was found in program
 * memory.
 *
 * @see LINE_NUMBER_FOUND
 * @see LINE_NUMBER_NOT_FOUND
 *
 */
static bool search_line_number(const uint16_t line, uint16_t *result);

/**
 * @brief Handles execution of particular tokens that are related to
 * the interface with the board hardware.
 *
 * Said tokens are `RECEIVE`, `SEND`, `AUX`, `DAT`, `BITREAD`, `PSU`,
 * `PULLUP`, `ADC`, and `MODE`.
 *
 * @param[in] token the token identifier to handle.
 *
 * @return the result value for the given execution, or 0 if the token was not
 * one of the special ones the function can handle.
 *
 * @see TOK_RECEIVE
 * @see TOK_SEND
 * @see TOK_AUX
 * @see TOK_DAT
 * @see TOK_BITREAD
 * @see TOK_PSU
 * @see TOK_PULLUP
 * @see TOK_ADC
 * @see TOK_MODE
 */
static uint16_t handle_special_token(const uint8_t token);

/**
 * @brief Checks whether the string at the BASIC parser current position matches
 * a valid token, and returns it if one is found.
 *
 * @return the token identifier if a matching token is found.
 * @return INVALID_TOKEN if no valid token was found.
 *
 * @see INVALID_TOKEN
 */
static uint8_t get_token(void);

static void list(void);
static void interpreter(void);

/**
 * @brief Handles an else statement if one is detected.
 */
static void handle_else_statement(void);

static int16_t get_number_or_variable(void);
static int16_t get_multiplication_division_bitwise_ops(void);
static int16_t assign(void);
static void interpreter(void);
static void list(void);

/**
 * @brief Compares the string pointed by the given pointer with the current
 * program space buffer pointer.
 *
 * @param[in] pointer the string to match.
 *
 * @return YES if the string matches, NO otherwise.
 */
static bool compare(char *pointer);

#ifdef BP_BASIC_I2C_FILESYSTEM
static void directory(void);
static void save(void);
static void format(void);
static void load(void);
static void waiteeprom(void);
#endif /* BP_BASIC_I2C_FILESYSTEM */

void handle_else_statement(void) {
  if (basic_program_area[basic_program_counter] != TOK_ELSE) {
    return;
  }

  basic_program_counter++;
  /* Skip the rest of the line, bounded by the program area so corrupted
     data cannot walk past it. */
  while ((basic_program_counter < BP_BASIC_PROGRAM_SPACE) &&
         (basic_program_area[basic_program_counter] <= TOK_LEN)) {
    basic_program_counter++;
  }
}

bool search_line_number(const uint16_t line, uint16_t *result) {
  size_t index = 0;
  uint8_t token_length;
  uint16_t current_line_number;

  for (;;) {
    /* A line header needs index, index + 1 and index + 2: stop at the end of
       the area instead of reading past it on corrupted data. */
    if ((index + 2) >= BP_BASIC_PROGRAM_SPACE) {
      return LINE_NUMBER_NOT_FOUND;
    }

    if (basic_program_area[index] <= TOK_LEN) {
      return LINE_NUMBER_NOT_FOUND;
    }
    token_length = basic_program_area[index] - TOK_LEN;
    current_line_number =
        (basic_program_area[index + 1] << 8) + basic_program_area[index + 2];
    if (line == current_line_number) {
      *result = index;
      return LINE_NUMBER_FOUND;
    }
    index += token_length + 3;
  }

  return LINE_NUMBER_NOT_FOUND;
}

uint16_t handle_special_token(const uint8_t token) {
  switch (token) {

  case TOK_RECEIVE: {
    uint16_t received =
        enabled_protocols[bus_pirate_configuration.bus_mode].read();

    /*
     * Honour the LSb/MSb setting, exactly as the interactive 'r' command
     * does in proc_menu.c.  Without this the byte order is silently ignored
     * in script mode (issue #129).
     */
    if (mode_configuration.little_endian == YES) {
      received = bp_reverse_integer(received, mode_configuration.numbits);
    }

    return received;
  }

  case TOK_SEND: {
    uint16_t value = (uint16_t)assign();

    /*
     * Honour the LSb/MSb setting, exactly as the interactive '8' command
     * does in proc_menu.c (issue #129).
     */
    if (mode_configuration.little_endian == YES) {
      value = bp_reverse_integer(value, mode_configuration.numbits);
    }

    return enabled_protocols[bus_pirate_configuration.bus_mode].send(value);
  }

  case TOK_AUX:
    return bp_aux_pin_read();

  case TOK_DAT:
    return enabled_protocols[bus_pirate_configuration.bus_mode].data_state();

  case TOK_BITREAD:
    return enabled_protocols[bus_pirate_configuration.bus_mode].read_bit();

  case TOK_PSU:
    return BP_VREGEN;

  case TOK_PULLUP:
    return ~BP_PULLUP;

  case TOK_MODE:
    /*
     * In an expression, `MODE` reports the bus mode that is currently active,
     * one-based, exactly as the mode menu numbers it.
     */
    return bus_pirate_configuration.bus_mode + 1;

  case TOK_ADC: {
    bp_enable_adc();
    uint16_t adc_measurement = bp_read_adc(BP_ADC_PROBE);
    bp_disable_adc();

    return adc_measurement;
  }

  default:
    return 0;
  }
}

int16_t get_number_or_variable(void) {
  int16_t temp = 0;

  if ((basic_program_area[basic_program_counter] == '(')) {
    basic_program_counter++;
    temp = assign();
    if ((basic_program_area[basic_program_counter] == ')')) {
      basic_program_counter++;
    }
  } else {
    if ((basic_program_area[basic_program_counter] >= 'A') &&
        (basic_program_area[basic_program_counter] <= 'Z')) {
      return basic_variables[basic_program_area[basic_program_counter++] - 'A'];
    }

    if (basic_program_area[basic_program_counter] > TOKENS) {
      return handle_special_token(basic_program_area[basic_program_counter++]);
    }

    while ((basic_program_area[basic_program_counter] >= '0') &&
           (basic_program_area[basic_program_counter] <= '9')) {
      temp *= 10;
      temp += basic_program_area[basic_program_counter] - '0';
      basic_program_counter++;
    }
  }

  return temp;
}

int16_t get_multiplication_division_bitwise_ops(void) {
  int16_t temp = get_number_or_variable();

  for (;;) {
    /* Inspect the byte, and advance only when it really is an operator: the
       caller relies on the counter still pointing at the terminator (a
       comparison in `IF`, the `TO` of `FOR`, and so on). */
    switch (basic_program_area[basic_program_counter]) {
    case '*':
      basic_program_counter++;
      temp *= get_number_or_variable();
      break;

    case '/': {
      basic_program_counter++;

      int16_t divisor = get_number_or_variable();

      /* The instruction set manual: "A math error exception occurs if den is
         zero", and an unhandled math error trap resets the board. Do not let
         a script divide by zero to reset the Bus Pirate; INT16_MIN / -1
         overflows the same way. */
      if ((divisor == 0) || ((temp == (-32767 - 1)) && (divisor == -1))) {
        temp = 0;
      } else {
        temp /= divisor;
      }
      break;
    }

    case '&':
      basic_program_counter++;
      temp &= get_number_or_variable();
      break;

    case '|':
      basic_program_counter++;
      temp |= get_number_or_variable();
      break;

    default:
      return temp;
    }
  }
}

int16_t assign(void) {
  int16_t temp = get_multiplication_division_bitwise_ops();

  for (;;) {
    /* As above: advance past the byte only once it is known to be part of
       the expression. The `>=`, `<=` and `<>` checks look at the byte the
       counter now points at - the old code tested `basic_program_counter + 1`
       and never matched. */
    switch (basic_program_area[basic_program_counter]) {
    case '-':
      basic_program_counter++;
      temp -= get_multiplication_division_bitwise_ops();
      break;

    case '+':
      basic_program_counter++;
      temp += get_multiplication_division_bitwise_ops();
      break;

    case '>':
      basic_program_counter++;
      if (basic_program_area[basic_program_counter] == '=') {
        basic_program_counter++;
        temp = (temp >= get_multiplication_division_bitwise_ops());
      } else {
        temp = (temp > get_multiplication_division_bitwise_ops());
      }
      break;

    case '<':
      basic_program_counter++;
      if (basic_program_area[basic_program_counter] == '>') {
        basic_program_counter++;
        temp = (temp != get_multiplication_division_bitwise_ops());
      } else if (basic_program_area[basic_program_counter] == '=') {
        basic_program_counter++;
        temp = (temp <= get_multiplication_division_bitwise_ops());
      } else {
        temp = (temp < get_multiplication_division_bitwise_ops());
      }
      break;

    case '=':
      basic_program_counter++;
      temp = (temp == get_multiplication_division_bitwise_ops());
      break;

    default:
      return temp;
    }
  }
}

/**
 * @brief Prints the active mode's name on behalf of a `MODE` statement.
 *
 * The interpreter runs with the board quiet, so the name is forced out the
 * same way the `PRINT` statement does it.
 */
static void write_mode_name(void) {
  bus_pirate_configuration.quiet = OFF;
  bp_write_line(enabled_protocols[bus_pirate_configuration.bus_mode].name);
  bus_pirate_configuration.quiet = ON;
}

void interpreter(void) {
  bool program_counter_updated = NO;
  basic_status_code_t status = STATUS_CODE_UNKNOWN;

  int len = 0;
  unsigned int lineno = 0;
  int i;
  int ifstat = 0;

  int temp;

  basic_program_counter = 0;
  basic_current_nested_for_index = 0;
  basic_current_stack_frame = 0;
  basic_data_read_pointer = 0;
  bus_pirate_configuration.quiet = ON;

  memset((void *)&basic_variables, 0, sizeof(basic_variables));

  while (status == STATUS_CODE_UNKNOWN) {
/*
     * A running program used to ignore the terminal until it ended, so a script
     * stuck in an endless loop could only be stopped by unplugging the board.
     * Ctrl-C now breaks out between statements.
     */
    if (user_serial_ready_to_read()) {
      if (user_serial_read_byte() == 0x03) {
        status = STATUS_CODE_INTERRUPTED;
        break;
      }
    }

    /* The header read and the statement token (pc + 3) must both be inside
       the program area; refuse to read past its end on corrupted data. */
    if ((basic_program_counter + 3) >= BP_BASIC_PROGRAM_SPACE) {
      status = STATUS_CODE_PARTIAL_INSTRUCTION_ERROR;
      break;
    }

    if (!ifstat) {
      if (basic_program_area[basic_program_counter] < TOK_LEN) {
        status = STATUS_CODE_PARTIAL_INSTRUCTION_ERROR;
        break;
      }

      len = basic_program_area[basic_program_counter] - TOK_LEN;
      lineno = ((basic_program_area[basic_program_counter + 1] << 8) |
                basic_program_area[basic_program_counter + 2]);
    }

    ifstat = 0;

    switch (basic_program_area[basic_program_counter + 3]) {
    case TOK_LET:
      program_counter_updated = YES;
      basic_program_counter += 6;

      /* The assignment target must exist; a stray byte must not be used as
         an array index (it would write outside basic_variables). */
      if ((basic_program_area[basic_program_counter - 2] < 'A') ||
          (basic_program_area[basic_program_counter - 2] > 'Z')) {
        status = STATUS_CODE_SYNTAX_ERROR;
        break;
      }

      basic_variables[basic_program_area[basic_program_counter - 2] - 0x41] =
          assign();
      handle_else_statement();
      break;

    case TOK_IF:
      program_counter_updated = YES;
      basic_program_counter += 4;

      if (assign()) {
        if (basic_program_area[basic_program_counter++] == TOK_THEN) {
          ifstat = 1;
          basic_program_counter -= 3; // simplest way (for now)
        }
      } else {
        while ((basic_program_counter < BP_BASIC_PROGRAM_SPACE) &&
               (basic_program_area[basic_program_counter] != TOK_ELSE) &&
               (basic_program_area[basic_program_counter] <= TOK_LEN)) {
          basic_program_counter++;
        }
        if (basic_program_area[basic_program_counter] == TOK_ELSE) {
          ifstat = 1;
          basic_program_counter -= 2; // simplest way (for now)
        }
      }
      break;

    case TOK_GOTO: {
      uint16_t line_number;

      program_counter_updated = YES;
      basic_program_counter += 4;

      if (search_line_number(assign(), &line_number)) {
        basic_program_counter = line_number;
      } else {
        status = STATUS_CODE_GOTO_ERROR;
      }
      break;
    }

    case TOK_GOSUB: {
      uint16_t line_number;

      program_counter_updated = YES;
      basic_program_counter += 4;

      if (basic_current_stack_frame < BP_BASIC_STACK_FRAMES_DEPTH) {
        basic_stack[basic_current_stack_frame] =
            basic_program_counter + len - 1;
        basic_current_stack_frame++;
        if (search_line_number(assign(), &line_number)) {
          basic_program_counter = line_number;
        } else {
          status = STATUS_CODE_GOTO_ERROR;
        }
      } else {
        status = STATUS_CODE_STACK_ERROR;
      }
      break;
    }

    case TOK_RETURN:
      bp_write_string(STAT_RETURN);

      program_counter_updated = YES;
      if (basic_current_stack_frame) {
        basic_program_counter = basic_stack[--basic_current_stack_frame];
      } else {
        status = STATUS_CODE_RETURN_ERROR;
      }
      break;

    case TOK_REM:
      program_counter_updated = YES;
      basic_program_counter += len + 3;
      break;

    case TOK_PRINT:
      program_counter_updated = YES;
      basic_program_counter += 4;
      while ((basic_program_counter < BP_BASIC_PROGRAM_SPACE) &&
             (basic_program_area[basic_program_counter] < TOK_LEN) &&
             (basic_program_area[basic_program_counter] != TOK_ELSE)) {
        if (basic_program_area[basic_program_counter] == '\"') {
          basic_program_counter++;
          /* Stop at the end of the line when the closing quote is missing:
             the old form scanned past the program area until it found one. */
          while ((basic_program_area[basic_program_counter] != '\"') &&
                 (basic_program_area[basic_program_counter] < TOK_LEN)) {
            bus_pirate_configuration.quiet = 0;
            user_serial_transmit_character(
                basic_program_area[basic_program_counter++]);
            bus_pirate_configuration.quiet = 1;
          }
          if (basic_program_area[basic_program_counter] == '\"') {
            basic_program_counter++;
          }
        } else if (basic_program_area[basic_program_counter] == ';') {
          basic_program_counter++;
        } else {
          /* Anything else is an expression: a literal number, a parenthesised
             value or a unary sign. The old form only handled letters and
             tokens, so "PRINT 42" never advanced and spun forever. */
          uint16_t before = basic_program_counter;

          temp = assign();
          if (basic_program_counter == before) {
            /* Unparsable byte: step over it instead of looping in place. */
            basic_program_counter++;
          }

          bus_pirate_configuration.quiet = 0;
          bp_write_dec_word(temp);
          bus_pirate_configuration.quiet = 1;
        }
      }
      if (basic_program_area[basic_program_counter - 1] != ';') {
        bus_pirate_configuration.quiet = 0;
        bpBR;
        bus_pirate_configuration.quiet = 1;
      }
      handle_else_statement();
      break;

    case TOK_INPUT:
      program_counter_updated = YES;
      bus_pirate_configuration.quiet = NO;
      basic_program_counter += 4;

      if (basic_program_area[basic_program_counter] == '\"') {
        basic_program_counter++;
        /* Stop at the end of the line if the closing quote is missing. */
        while ((basic_program_area[basic_program_counter] != '\"') &&
               (basic_program_area[basic_program_counter] < TOK_LEN)) {
          user_serial_transmit_character(
              basic_program_area[basic_program_counter++]);
        }
        if (basic_program_area[basic_program_counter] == '\"') {
          basic_program_counter++;
        }
      }
      if (basic_program_area[basic_program_counter] == ',') {
        basic_program_counter++;
      } else {
        status = STATUS_CODE_SYNTAX_ERROR;
      }

      /* The receiving variable must exist before it is indexed. */
      if ((basic_program_area[basic_program_counter] < 'A') ||
          (basic_program_area[basic_program_counter] > 'Z')) {
        bus_pirate_configuration.quiet = YES;
        status = STATUS_CODE_SYNTAX_ERROR;
        break;
      }

      basic_variables[basic_program_area[basic_program_counter] - 'A'] =
          getnumber(0, 0, 0x7FFF, 0);
      basic_program_counter++;
      handle_else_statement();
      bus_pirate_configuration.quiet = YES;
      break;

    case TOK_FOR:
      program_counter_updated = YES;
      basic_program_counter += 4;
      if (basic_current_nested_for_index < BP_BASIC_NESTED_FOR_LOOP_COUNT) {
        basic_current_nested_for_index++;
      } else {
        /* The loop array is full: stop before indexing past its end. */
        status = STATUS_CODE_FOR_ERROR;
        break;
      }

      /* The loop variable must exist before it is indexed. */
      if ((basic_program_area[basic_program_counter] < 'A') ||
          (basic_program_area[basic_program_counter] > 'Z')) {
        status = STATUS_CODE_SYNTAX_ERROR;
        break;
      }

      basic_nested_for_loops[basic_current_nested_for_index].var =
          (basic_program_area[basic_program_counter++] - 'A') + 1;

      if (basic_program_area[basic_program_counter] == '=') {
        basic_program_counter++;
        basic_variables
            [(basic_nested_for_loops[basic_current_nested_for_index].var) - 1] =
                assign();
      } else {
        status = STATUS_CODE_SYNTAX_ERROR;
      }
      if (basic_program_area[basic_program_counter++] == TOK_TO) {
        basic_nested_for_loops[basic_current_nested_for_index].to = assign();
      } else {
        status = STATUS_CODE_SYNTAX_ERROR;
      }
      if (basic_program_area[basic_program_counter] >= TOK_LEN) {
        basic_nested_for_loops[basic_current_nested_for_index].from =
            basic_program_counter;
      } else {
        status = STATUS_CODE_SYNTAX_ERROR;
      }
      handle_else_statement();
      break;

    case TOK_NEXT:
      program_counter_updated = YES;
      basic_program_counter += 4;

      temp = (basic_program_area[basic_program_counter++] - 'A') + 1;
      status = STATUS_CODE_NEXT_ERROR;

      for (i = 0; i <= basic_current_nested_for_index; i++) {
        if (basic_nested_for_loops[i].var == temp) {
          if (basic_variables[temp - 1] < basic_nested_for_loops[i].to) {
            basic_variables[temp - 1]++;
            basic_program_counter = basic_nested_for_loops[i].from;
          } else if (basic_current_nested_for_index > 0) {
            /* Pop the loop; never let the index underflow. */
            basic_current_nested_for_index--;
          }
          status = STATUS_CODE_UNKNOWN;
        }
      }
      handle_else_statement();
      break;

    case TOK_RESTORE:
      /*
       * Reading starts again from the first `DATA` statement: clearing the
       * pointer makes the next `READ` rescan for it. The program counter is
       * deliberately not updated, so the generic advance below skips the
       * whole statement.
       */
      basic_data_read_pointer = 0;
      break;

    case TOK_READ:
      program_counter_updated = YES;
      basic_program_counter += 4;

      if (basic_data_read_pointer == 0) {
        i = 0;
        while ((i + 3 < BP_BASIC_PROGRAM_SPACE) &&
               (basic_program_area[i + 3] != TOK_DATA) &&
               (basic_program_area[i] != 0x00)) {
          i += (basic_program_area[i] - TOK_LEN) + 3;
        }

        if ((i + 3 < BP_BASIC_PROGRAM_SPACE) &&
            (basic_program_area[i] != 0x00) &&
            (basic_program_area[i + 3] == TOK_DATA)) {
          basic_data_read_pointer = i + 4;
        } else {
          status = STATUS_CODE_DATA_ERROR;
        }
      }
      temp = basic_program_counter;

      /* The receiving variable must exist before it is indexed. */
      if ((basic_program_area[temp] < 'A') ||
          (basic_program_area[temp] > 'Z')) {
        status = STATUS_CODE_SYNTAX_ERROR;
        break;
      }

      basic_program_counter = basic_data_read_pointer;
      basic_variables[basic_program_area[temp] - 'A'] =
          get_number_or_variable();
      basic_data_read_pointer = basic_program_counter;
      basic_program_counter = temp + 1;

      if (basic_program_area[basic_data_read_pointer] == ',') {
        basic_data_read_pointer++;
      }
      if (basic_program_area[basic_data_read_pointer] > TOK_LEN) {
        if (basic_program_area[basic_data_read_pointer + 3] != TOK_DATA) {
          basic_data_read_pointer = 0;
        } else {
          basic_data_read_pointer += 4;
        }
      }

      handle_else_statement();
      break;

    case TOK_DATA:
      program_counter_updated = YES;
      basic_program_counter += len + 3;
      break;

    case TOK_START:
      program_counter_updated = YES;
      basic_program_counter += 4;

      enabled_protocols[bus_pirate_configuration.bus_mode].start();
      handle_else_statement();
      break;

    case TOK_STARTR:
      program_counter_updated = YES;
      basic_program_counter += 4;

      enabled_protocols[bus_pirate_configuration.bus_mode].start_with_read();
      handle_else_statement();
      break;

    case TOK_STOP:
      program_counter_updated = YES;
      basic_program_counter += 4;

      enabled_protocols[bus_pirate_configuration.bus_mode].stop();
      handle_else_statement();
      break;

    case TOK_STOPR:
      program_counter_updated = YES;
      basic_program_counter += 4;

      enabled_protocols[bus_pirate_configuration.bus_mode].stop_from_read();
      handle_else_statement();
      break;

    case TOK_SEND:
      program_counter_updated = YES;
      basic_program_counter += 4;
      enabled_protocols[bus_pirate_configuration.bus_mode].send((int)assign());
      handle_else_statement();
      break;

    case TOK_AUX:
      program_counter_updated = YES;
      basic_program_counter += 4;

      if (assign()) {
        bp_aux_pin_set_high();
      } else {
        bp_aux_pin_set_low();
      }
      handle_else_statement();
      break;

    case TOK_PSU:
      program_counter_updated = YES;
      basic_program_counter += 4;
      bp_set_voltage_regulator_state(assign() ? ON : OFF);
      handle_else_statement();
      break;

    case TOK_AUXPIN:
      program_counter_updated = YES;
      basic_program_counter += 4;

      if (assign()) {
        mode_configuration.alternate_aux = ON;
      } else {
        mode_configuration.alternate_aux = OFF;
      }
      handle_else_statement();
      break;

    case TOK_FREQ: {
      int16_t frequency = assign();
      int16_t duty_cycle = assign();

      program_counter_updated = YES;
      basic_program_counter += 4;

      if (frequency < PWM_MINIMUM_FREQUENCY) {
        frequency = PWM_MINIMUM_FREQUENCY;
      }
      if (frequency > PWM_MAXIMUM_FREQUENCY) {
        frequency = PWM_MAXIMUM_FREQUENCY;
      }

      if (duty_cycle < PWM_MINIMUM_DUTY_CYCLE) {
        duty_cycle = PWM_MINIMUM_DUTY_CYCLE;
      }
      if (duty_cycle > PWM_MAXIMUM_DUTY_CYCLE) {
        duty_cycle = PWM_MAXIMUM_DUTY_CYCLE;
      }

      bp_update_pwm(frequency, duty_cycle);

      handle_else_statement();
      break;
    }

    case TOK_DUTY: {
      int16_t duty_cycle = assign();

      program_counter_updated = YES;
      basic_program_counter += 4;

      if (duty_cycle < PWM_MINIMUM_DUTY_CYCLE) {
        duty_cycle = PWM_MINIMUM_DUTY_CYCLE;
      }
      if (duty_cycle > PWM_MAXIMUM_DUTY_CYCLE) {
        duty_cycle = PWM_MAXIMUM_DUTY_CYCLE;
      }

      bp_update_duty_cycle(duty_cycle);

      handle_else_statement();
      break;
    }

    case TOK_DAT:
      program_counter_updated = 1;
      basic_program_counter += 4;

      if (assign()) {
        enabled_protocols[bus_pirate_configuration.bus_mode].data_high();
      } else {
        enabled_protocols[bus_pirate_configuration.bus_mode].data_low();
      }
      handle_else_statement();
      break;

    case TOK_CLK:
      program_counter_updated = YES;
      basic_program_counter += 4;

      switch (assign()) {
      case 0:
        enabled_protocols[bus_pirate_configuration.bus_mode].clock_low();
        break;
      case 1:
        enabled_protocols[bus_pirate_configuration.bus_mode].clock_high();
        break;
      case 2:
        enabled_protocols[bus_pirate_configuration.bus_mode].clock_pulse();
        break;
      }
      handle_else_statement();
      break;

    case TOK_PULLUP:
      program_counter_updated = YES;
      basic_program_counter += 4;
      bp_set_pullup_state(assign() ? ON : OFF);
      handle_else_statement();
      break;

    case TOK_DELAY:
      program_counter_updated = YES;
      basic_program_counter += 4;
      temp = assign();
      bp_delay_ms(temp);
      handle_else_statement();
      break;

    case TOK_MODE: {
      /*
       * A bare `MODE` statement reports the active mode. `MODE n` switches to
       * the mode that the (one-based) mode menu lists as `n`, using the same
       * default settings that pressing ENTER through the interactive setup
       * prompts would choose.
       *
       * After a bare statement the next byte is either the NUL that ends the
       * stored line or the length marker of the following line; anything
       * else is the start of an argument.
       */
      if (((basic_program_counter + 4) >= BP_BASIC_PROGRAM_SPACE) ||
          (basic_program_area[basic_program_counter + 4] == 0x00) ||
          (basic_program_area[basic_program_counter + 4] >= TOK_LEN)) {
        write_mode_name();
        break;
      }

      program_counter_updated = YES;
      basic_program_counter += 4;

      if (bp_change_mode(assign())) {
        status = STATUS_CODE_SYNTAX_ERROR;
        break;
      }

      write_mode_name();
      handle_else_statement();
      break;
    }

    case TOK_MACRO:
      program_counter_updated = YES;
      basic_program_counter += 4;
      temp = assign();
      enabled_protocols[bus_pirate_configuration.bus_mode].run_macro(temp);
      handle_else_statement();
      break;

    case TOK_END:
      status = STATUS_CODE_FINISHED;
      break;

    default:
      status = STATUS_CODE_SYNTAX_ERROR;
      break;
    }

    if (!program_counter_updated) {
      basic_program_counter += len + 3;
    }
    program_counter_updated = NO;
  }

  bus_pirate_configuration.quiet = OFF;

  if ((status != STATUS_CODE_FINISHED) && (status != STATUS_CODE_UNKNOWN)) {
    BPMSG1047;
    bp_write_dec_word(status);
    BPMSG1048;
    bp_write_dec_word(lineno);
    BPMSG1049;
    bp_write_dec_word(basic_program_counter);
    bpBR;
  }
}

void list(void) {
  unsigned char c;
  unsigned int lineno;

  basic_program_counter = 0;

  while ((basic_program_counter + 2 < BP_BASIC_PROGRAM_SPACE) &&
         (basic_program_area[basic_program_counter])) {
    c = basic_program_area[basic_program_counter];
    if (c < TOK_LET) {
      user_serial_transmit_character(c);
    } else if (c > TOK_LEN) {
      bpBR;
      lineno = (basic_program_area[basic_program_counter + 1] << 8) +
               basic_program_area[basic_program_counter + 2];
      basic_program_counter += 2;
      bp_write_dec_word(lineno);
      bpSP;
    } else if (c <= TOK_RESTORE) {
      bpSP;
      bp_write_string(tokens[c - TOKENS]);
      bpSP;
    }
    basic_program_counter++;
  }
  bpBR;
  bp_write_dec_word(basic_program_counter - 1);
  BPMSG1050;
}

bool compare(char *pointer) {
  int oldstart = cmdstart;

  while (*pointer) {
    /* Never match past the end of the current command line: the buffer
       still holds the previous line beyond cmdend. */
    if (cmdstart == cmdend) {
      cmdstart = oldstart;
      return NO;
    }

    if (*pointer != cmdbuf[cmdstart]) {
      cmdstart = oldstart;
      return NO;
    }

    cmdstart = (cmdstart + 1) & CMDLENMSK;
    pointer++;
  }

  return YES;
}

uint8_t get_token(void) {
  size_t index;

  for (index = 0; index < NUMTOKEN; index++) {
    if (compare(tokens[index])) {
      return TOKENS + index;
    }
  }

  return 0;
}

void bp_basic_enter_interactive_interpreter(void) {
  int i, temp;
  int end, len; //, string;
  bool string_found = NO;
  unsigned char line[35];
  uint16_t pos;
  size_t index = cmdstart;

  unsigned int lineno1, lineno2;

  /* Convert all text to upper case ASCII. */

  while (index != cmdend) {
    if ((cmdbuf[index] >= 'a') && (cmdbuf[index] <= 'z')) {
      cmdbuf[index] &= 0xDF;
    }
    index = (index + 1) & CMDLENMSK;
  }

  i = 0;
  // command or a new line?
  if ((cmdbuf[cmdstart] >= '0') &&
      (cmdbuf[cmdstart] <= '9')) { // bpWline("new line");

    /* No need to clear line[]: every byte copied into the program area below
       is written first (the header here, the payload by the tokenizer). */

    temp = getint();
    line[1] = temp >> 8;
    line[2] = temp & 0xFF;

    if (search_line_number(temp, &pos)) {
      len = (basic_program_area[pos] - TOK_LEN) + 3;

      /* The line is being replaced: close the gap it leaves, then clear the
         tail so no stale bytes are left for a walk to find. */
      memmove(&basic_program_area[pos], &basic_program_area[pos + len],
              BP_BASIC_PROGRAM_SPACE - pos - len);
      memset(&basic_program_area[BP_BASIC_PROGRAM_SPACE - len], 0x00, len);
    }

    i = 3;

    consumewhitechars();
    while (cmdstart != cmdend) {
/* Guard before the write: line[] is sizeof(line) bytes and the item
         about to be stored claims line[i]. The old check ran after the loop
         instead, one byte too late. */
      if (i >= (int)sizeof(line)) {
        BPMSG1051;
        return;
      }

      if (!string_found) {
        consumewhitechars();
      }

      temp = string_found ? 0 : get_token();

      if (temp) {
        line[i] = temp;
        if (temp == TOK_REM) {
          string_found = YES;
        }
      } else {
        if (cmdbuf[cmdstart] == '"') {
          string_found = !string_found;
        }
        line[i] = cmdbuf[cmdstart];
        cmdstart = (cmdstart + 1) & CMDLENMSK;
      }
      i++;
    }

    /* An i of 4 means the only item the tokenizer found was the NUL byte that
       handle_enter_key() leaves at the end of the line buffer: the line has a
       number but no statement, so there is nothing to store. */
    if (i <= 4) {
      return;
    }

/* The length marker counts statement bytes only. i is 3 header bytes plus
       one per stored item, and the items include the trailing NUL that
       handle_enter_key() appends - so the payload length is i - 4. */
    line[0] = TOK_LEN + (i - 4);

    i = 0;
    end = 0;
    pos = 0;

    /* Find where the line belongs: pos ends up after the last stored line
       whose number is lower, end at the first byte past the program. The walk
       is bounded, because a corrupted header used to send it off the end of
       the array, where the read hits unimplemented memory - an address error
       trap, and the default trap handler resets the board (issue #130). */
    while (!end) {
      if (i >= BP_BASIC_PROGRAM_SPACE) {
        BPMSG1051;
        return;
      }

      if (basic_program_area[i] > TOK_LEN) {
        if ((i + 2) >= BP_BASIC_PROGRAM_SPACE) {
          BPMSG1051;
          return;
        }

        len = basic_program_area[i] - TOK_LEN;
        lineno1 = (basic_program_area[i + 1] << 8) + basic_program_area[i + 2];
        lineno2 = (line[1] << 8) + line[2];
        if (lineno1 < lineno2) {
          pos = i + len + 3;
        }
        i += (len + 3);
      } else {
        end = i; // we found the end! YaY!
      }
    }

    temp = (line[0] - TOK_LEN) + 3;

    /* Refuse to store when the program area has no room for the line, rather
       than writing past the end of the array. */
    if ((pos > end) || ((end + temp) >= BP_BASIC_PROGRAM_SPACE)) {
      BPMSG1051;
      return;
    }
/* Open a gap and copy the line in. The old code did this with loops; a
       cleanup rewrote the copy as memcpy(&basic_program_area[pos + i],
       &line[i], temp) while i was still pos - 1 from the shift loop, so the
       first line of a program wrote one byte before the array and every other
       line landed at the wrong offset. */
    memmove(&basic_program_area[pos + temp], &basic_program_area[pos],
            end - pos + 1);
    memcpy(&basic_program_area[pos], line, temp);
  } else {
    if (compare("RUN")) {
      interpreter();
      bpBR;
    } else if (compare("LIST")) {
      list();
    } else if (compare("EXIT")) {
      bus_pirate_configuration.basic = NO;
    }
#ifdef BP_BASIC_I2C_FILESYSTEM
    else if (compare("FORMAT")) {
      format();
    } else if (compare("SAVE")) {
      save();
    } else if (compare("LOAD")) {
      load();
    }
#endif /* BP_BASIC_I2C_FILESYSTEM */

#ifdef BP_BASIC_DEBUG_COMMAND
    else if (compare("DEBUG")) {
      for (i = 0; i < BP_BASIC_PROGRAM_SPACE; i += 16) {
        for (j = 0; j < 16; j++) {
          bp_write_hex_byte(basic_program_area[i + j]);
          bpSP;
        }
      }
    }
#endif /* BP_BASIC_DEBUG_COMMAND */

    else if (compare("NEW")) {
      bp_basic_initialize();
    } else {
      BPMSG1052;
    }
  }
}

void bp_basic_initialize(void) {
  basic_program_area[0] = TOK_LEN + 1;
  basic_program_area[1] = 0xFF;
  basic_program_area[2] = 0xFF;
  basic_program_area[3] = TOK_END;
  memset(&basic_program_area[4], 0x00, BP_BASIC_PROGRAM_SPACE - 4);
}

#ifdef BP_BASIC_I2C_FILESYSTEM

// i2c eeprom interaction
// need to incorperate it in bitbang or r2wire!
// CvD: I stole most off it from bitbang.c/h

#define BASSDA 1
#define BASSCL 2
#define BASI2CCLK 100

#define EEP24LC256

#ifdef EEP24LC256

#define I2CADDR 0xA0
#define EEPROMSIZE 0x8000
#define EEPROMPAGE 64

#endif

// globals
int eeprom_lastprog;
unsigned int eeprom_lastmem;

void HIZbbL(unsigned int pins, int delay) {
  IOLAT &= (~pins);   // pins to 0
  IODIR &= (~pins);   // direction to output
  bp_delay_us(delay); // delay
}
void HIZbbH(unsigned int pins, int delay) {
  IODIR |= pins;      // open collector output high
  bp_delay_us(delay); // delay
}
unsigned char HIZbbR(unsigned int pin) {
  IODIR |= pin; // pin as input
  Nop();
  Nop();
  Nop();
  if (IOPOR & pin)
    return 1;
  else
    return 0; // clear all but pin bit and return result
}

void basi2cstart(void) {
  HIZbbH((BASSDA | BASSCL), BASI2CCLK); // both hi
  HIZbbL(BASSDA, BASI2CCLK);            // data down
  HIZbbL(BASSCL, BASI2CCLK);            // clk down
  HIZbbH(BASSDA, BASI2CCLK);            // data up
}

void basi2cstop(void) {
  HIZbbL((BASSDA | BASSCL), BASI2CCLK);
  HIZbbH(BASSCL, BASI2CCLK);
  HIZbbH(BASSDA, BASI2CCLK);
}

unsigned char basi2cread(int ack) {
  int i;
  unsigned char c;

  c = 0;
  HIZbbR(BASSDA);

  for (i = 0; i < 8; i++) {
    HIZbbL(BASSCL, BASI2CCLK / 5);
    HIZbbH(BASSCL, BASI2CCLK);
    c <<= 1;
    c |= HIZbbR(BASSDA);

    HIZbbL(BASSCL, BASI2CCLK);
  }

  if (ack) {
    HIZbbL(BASSDA, BASI2CCLK / 5);
  }
  HIZbbH(BASSCL, BASI2CCLK);
  HIZbbL(BASSCL, BASI2CCLK);

  return c;
}

int basi2cwrite(unsigned char c) {
  int i;
  unsigned char mask;

  mask = 0x80;

  for (i = 0; i < 8; i++) {
    if (c & mask) {
      HIZbbH(BASSDA, BASI2CCLK / 5);
      // bpWstring("W1");
    } else {
      HIZbbL(BASSDA, BASI2CCLK / 5);
      // bpWstring("W0");
    }
    HIZbbH(BASSCL, BASI2CCLK);
    HIZbbL(BASSCL, BASI2CCLK);
    mask >>= 1;
  }

  HIZbbH(BASSCL, BASI2CCLK);
  i = HIZbbR(BASSDA);
  HIZbbL(BASSCL, BASI2CCLK);

  return (i ^ 0x01);
}

int checkeeprom(void) { // just to be sure
  basi2cstop();
  basi2cstop();
  basi2cstart();
  if (!basi2cwrite(I2CADDR)) { // bpWline("No EEPROM");
    BPMSG1053;
    return 0;
  }
  basi2cwrite(0x00);
  basi2cwrite(0x00);
  basi2cstart();
  if (basi2cread(1) == 0x00) // check for any data
  {
    bp_write_line("No EEPROM"); // if 0 prolly no pullup and eeprom (PROLLY!)
    BPMSG1053;
    return 0;
  }
  basi2cstop();
  return 1;
}

void format(void) {
  int i, j;

  basi2cstop();
  basi2cstart();
  if (!basi2cwrite(I2CADDR)) { // bpWline("No EEPROM");
    BPMSG1053;
    return;
  }
  basi2cstop();

  // bpWstring("Erasing");
  BPMSG1054;
  for (i = 0; i < EEPROMSIZE; i += EEPROMPAGE) {
    basi2cstart();
    basi2cwrite(I2CADDR);
    basi2cwrite((i >> 8));
    basi2cwrite((i & 0x0FF));
    for (j = 0; j < EEPROMPAGE; j++) {
      basi2cwrite(0xFF);
    }
    basi2cstop();
    user_serial_transmit_character('.');
    waiteeprom();
  }
  // bpWline("done");
  BPMSG1055;
}

void waiteeprom(void) {
  int wait;
  wait = 1;
  while (wait) {
    basi2cstart();
    wait = basi2cwrite(I2CADDR);
    basi2cstop();
  }
}

void save(void) {
  int i, j;
  int slot;

  consumewhitechars();
  slot = getint();

  if (slot == 0) { // bpWline("Syntax error");
    BPMSG1052;
    return;
  }

  // bpWstring("Saving to slot ");
  BPMSG1056;
  bp_write_dec_byte(slot);
  bpBR;

  if (slot > (EEPROMSIZE / PGMSIZE)) { // bpWline("Invalid slot");
    BPMSG1057;
    return;
  }

  if (!checkeeprom()) {
    return;
  }

  slot *= PGMSIZE;

  basi2cstop();
  basi2cwrite(I2CADDR);
  basi2cwrite(slot >> 8);
  basi2cwrite(slot & 0x0FF);
  basi2cstart();
  basi2cwrite(I2CADDR + 1);

  slot *= EEPROMPAGE;

  for (i = 0; i < PGMSIZE;
       i += EEPROMPAGE) // we assume that pgmsize is dividable by eeprompage
  {
    basi2cstart();
    basi2cwrite(I2CADDR);
    basi2cwrite((slot + i) >> 8);
    basi2cwrite((slot + i) & 0x0FF);
    for (j = 0; j < EEPROMPAGE; j++) {
      basi2cwrite(basic_program_area[i + j]);
    }
    basi2cstop();
    user_serial_transmit_character('.');
    waiteeprom();
  }
}

void load(void) {
  int i;
  int slot;

  consumewhitechars();
  slot = getint();

  if (slot == 0) { // bpWline("Syntax error");
    BPMSG1052;
    return;
  }

  // bpWstring("Loading from slot ");
  BPMSG1058;
  bp_write_dec_byte(slot);
  bpBR;

  if (slot > (EEPROMSIZE / PGMSIZE)) { // bpWline("Invalid slot");
    BPMSG1057;
    return;
  }

  if (!checkeeprom()) {
    return;
  }

  slot *= PGMSIZE;

  basi2cstop();
  basi2cwrite(I2CADDR);
  basi2cwrite(slot >> 8);
  basi2cwrite(slot & 0x0FF);
  basi2cstart();
  basi2cwrite(I2CADDR + 1);

  for (i = 0; i < PGMSIZE; i++) {
    if (!(i % EEPROMPAGE))
      user_serial_transmit_character('.'); // pure estetic
    basic_program_area[i] = basi2cread(1);
  }
}

#endif /* BP_BASIC_I2C_FILESYSTEM */
#endif /* BP_ENABLE_BASIC_SUPPORT */