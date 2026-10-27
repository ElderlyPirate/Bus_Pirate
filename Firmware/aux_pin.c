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
 * @file aux_pin.c
 *
 * @brief AUX pins handler implementation file.
 */
#include <stdint.h>

#include "aux_pin.h"
#include "base.h"
#include "proc_menu.h"

#define AUXPIN_DIR BP_AUX0_DIR
#define AUXPIN_RPIN BP_AUX_RPIN
#define AUXPIN_RPOUT BP_AUX_RPOUT

extern mode_configuration_t mode_configuration;

/**
 * @brief Possible modes for the AUX pins.
 */
typedef enum {
  /** The AUX pin is set in I/O mode. */
  AUX_MODE_IO = 0,
  /** The AUX pin is set in Frequency Counting mode. */
  AUX_MODE_FREQUENCY,
  /** The AUX pin is set in PWM Signal Generation mode. */
  AUX_MODE_PWM
} __attribute__((packed)) aux_mode_t;

/**
 * @brief Reads the AUX signal for one second, returning the detected frequency.
 *
 * @return the detected frequency on the AUX pin in the second spent sampling,
 * in Hz.
 */
static uint32_t poll_frequency_counter_value(void);

/**
 * @brief Gets the average frequency for the given samples count.
 *
 * @param count[in] the number of samples to obtain.
 *
 * @return the average frequency value, in Hz.
 */
static uint32_t average_sample_frequency(const uint16_t count);

/**
 * @brief Stops the two timers used in the PWM/frequency counting process.
 */
static inline void stop_timers(void);

/**
 * @brief AUX pins manager internal state variables container.
 */
typedef struct {
  /** The PWM frequency in use. */
  uint16_t pwm_frequency;
  /** The PWM duty cycle in use. */
  uint16_t pwm_duty_cycle;
  /** The AUX pin mode. */
  aux_mode_t mode;
} __attribute__((packed)) aux_state_t;

/**
 * @brief AUX pins manager state.
 */
static aux_state_t state = {0};

/**
 * @brief Sets up input clock prescaler and returns an appropriate divisor.
 *
 * Sets up timer #1's input clock prescaler for the given frequency and returns
 * an appropriate divisor for it.
 *
 * @param[in] frequency the given frequency to set things up for.
 *
 * @return the appropriate PWM frequency divisor.
 */
static uint16_t setup_prescaler_divisor(const uint16_t frequency);

/**
 * @brief PWM frequency divisor for 1:256 prescaler.
 */
#define PWM_DIVISOR_PRESCALER_1_256 62

/**
 * @brief PWM frequency divisor for 1:64 prescaler.
 */
#define PWM_DIVISOR_PRESCALER_1_64 250

/**
 * @brief PWM frequency divisor for 1:8 prescaler.
 */
#define PWM_DIVISOR_PRESCALER_1_8 2000

/**
 * @brief PWM frequency divisor for 1:1 prescaler.
 */
#define PWM_DIVISOR_PRESCALER_1_1 16000

uint16_t setup_prescaler_divisor(const uint16_t frequency) {

  /* Use 1:256 prescaler. */

  if (frequency < 4) {
    T2CONbits.TCKPS1 = ON;
    T2CONbits.TCKPS0 = ON;

    return PWM_DIVISOR_PRESCALER_1_256;
  }

  /* Use 1:64 prescaler. */

  if (frequency < 31) {
    T2CONbits.TCKPS1 = ON;
    T2CONbits.TCKPS0 = OFF;

    return PWM_DIVISOR_PRESCALER_1_64;
  }

  /* Use 1:8 prescaler. */

  if (frequency < 245) {
    T2CONbits.TCKPS1 = OFF;
    T2CONbits.TCKPS0 = ON;

    return PWM_DIVISOR_PRESCALER_1_8;
  }

  /* Use 1:1 prescaler. */

  T2CONbits.TCKPS1 = OFF;
  T2CONbits.TCKPS0 = OFF;

  return PWM_DIVISOR_PRESCALER_1_1;
}

inline void bp_update_duty_cycle(const uint16_t duty_cycle) {
  bp_update_pwm(state.pwm_frequency, duty_cycle);
}

void bp_update_pwm(const uint16_t frequency, const uint16_t duty_cycle) {
  uint16_t period;
  uint16_t cycle;
  uint16_t divisor;

  state.pwm_frequency = frequency;
  state.pwm_duty_cycle = duty_cycle;

  stop_timers();
#if defined(BUSPIRATEV4)
  /*
   * OC5CON1
   *
   * MSB
   * --0000--0--00000
   *   ||||  |  |||||
   *   ||||  |  ||+++-- OCM:      Output compare is disabled.
   *   ||||  |  |+----- TRIGMODE: TRIGSTAT is only cleared by software.
   *   ||||  |  +------ OCFLT0:   No PWM fault condition.
   *   ||||  +--------- ENFLT0:   Fault 0 input disabled.
   *   |+++------------ OCTSEL:   Output compare with Timer 2.
   *   +--------------- OCSIDL:   Output Compare continues in CPU idle mode.
   */
  OC5CON = 0x0000;
#else
  /*
   * OC5CON
   *
   * MSB
   * --0--------00000
   *   |        |||||
   *   |        ||+++-- OCM:    Output compare is disabled.
   *   |        |+----- OCTSEL: Output compare with Timer 2.
   *   |        +------ OCFLT:  No PWM fault condition.
   *   +--------------- OCSIDL: Output Compare continues in CPU idle mode.
   */
  OC5CON = 0x0000;
#endif /* BUSPIRATEV4 */

  /* Detach the AUX pin from the PWM generator if no PWM signal is needed. */
  if (frequency == 0) {
    AUXPIN_RPOUT = 0;
    state.mode = AUX_MODE_IO;
    return;
  }

  divisor = setup_prescaler_divisor(frequency);
  period = (divisor / frequency) - 1;
  PR2 = period;
  cycle = (period * duty_cycle) / 100;

  /* Attach the AUX pin to the PWM generator. */
  AUXPIN_RPOUT = OC5_IO;

  /* Setup the PWM generator. */
  OC5R = cycle;
  OC5RS = cycle;
#if defined(BUSPIRATEV4)
  /*
   * OC5CON1
   *
   * MSB
   * --0000--0--00110
   *   ||||  |  |||||
   *   ||||  |  ||+++-- OCM:      Edge-aligned PWM mode.
   *   ||||  |  |+----- TRIGMODE: TRIGSTAT is only cleared by software.
   *   ||||  |  +------ OCFLT0:   No PWM fault condition.
   *   ||||  +--------- ENFLT0:   Fault 0 input disabled.
   *   |+++------------ OCTSEL:   Output compare with Timer 2.
   *   +--------------- OCSIDL:   Output Compare continues in CPU idle mode.
   */
  OC5CON =
      (0b110 << _OC5CON1_OCM_POSITION) | (OFF << _OC5CON1_TRIGMODE_POSITION) |
      (OFF << _OC5CON1_OCFLT0_POSITION) | (OFF << _OC5CON1_ENFLT0_POSITION) |
      (OFF << _OC5CON1_OCTSEL_POSITION) | (OFF << _OC5CON1_OCSIDL_POSITION);
#else
  /*
   * OC5CON
   *
   * MSB
   * --0--------00110
   *   |        |||||
   *   |        ||+++-- OCM:    Edge-aligned PWM mode.
   *   |        |+----- OCTSEL: Output compare with Timer 2.
   *   |        +------ OCFLT:  No PWM fault condition.
   *   +--------------- OCSIDL:   Output Compare continues in CPU idle mode.
   */
  OC5CON = (0b110 << _OC5CON_OCM_POSITION) | (OFF << _OC5CON_OCTSEL_POSITION) |
           (OFF << _OC5CON_OCFLT_POSITION) | (OFF << _OC5CON_OCSIDL_POSITION);
#endif /* BUSPIRATEV4 */
  T2CONbits.TON = ON;
  state.mode = AUX_MODE_PWM;
}

void bp_pwm_setup(void) {
  uint16_t pwm_period;
  uint16_t pwm_duty_cycle;
  uint16_t pwm_frequency;
  uint16_t pwm_divisor;
  float pwm_period_float;
  bool valid_arguments;

  stop_timers();
#if defined(BUSPIRATEV4)
  /*
   * OC5CON1
   *
   * MSB
   * --0000--0--00000
   *   ||||  |  |||||
   *   ||||  |  ||+++-- OCM:      Output compare is disabled.
   *   ||||  |  |+----- TRIGMODE: TRIGSTAT is only cleared by software.
   *   ||||  |  +------ OCFLT0:   No PWM fault condition.
   *   ||||  +--------- ENFLT0:   Fault 0 input disabled.
   *   |+++------------ OCTSEL:   Output compare with Timer 2.
   *   +--------------- OCSIDL:   Output Compare continues in CPU idle mode.
   */
  OC5CON = 0x0000;
#else
  /*
   * OC5CON
   *
   * MSB
   * --0--------00000
   *   |        |||||
   *   |        ||+++-- OCM:    Output compare is disabled.
   *   |        |+----- OCTSEL: Output compare with Timer 2.
   *   |        +------ OCFLT:  No PWM fault condition.
   *   +--------------- OCSIDL: Output Compare continues in CPU idle mode.
   */
  OC5CON = 0x0000;
#endif /* BUSPIRATEV4 */

  /* Stop PWM mode if it is engaged. */
  if (state.mode == AUX_MODE_PWM) {
    /* Detach AUX pin from output source. */
    AUXPIN_RPOUT = 0;
    BPMSG1028;
    state.mode = AUX_MODE_IO;

    if (cmdbuf[((cmdstart + 1) & CMDLENMSK)] == 0x00) {
      /* No arguments, quit. */
      return;
    }
  }

  /* Get parameters. */
  cmdstart = (cmdstart + 1) & CMDLENMSK;
  consumewhitechars();
  pwm_frequency = getint();
  consumewhitechars();
  pwm_period_float = getint();

  valid_arguments = ((pwm_frequency > 0) && (pwm_frequency < 4000) &&
                     ((pwm_period_float > 0) && (pwm_period_float < 100)));

  if (!valid_arguments) {
    mode_configuration.command_error = NO;
    BPMSG1029;
    BPMSG1030;
    pwm_frequency = getnumber(50, 1, 4000, 0);
  }

  pwm_divisor = setup_prescaler_divisor(pwm_frequency);
  pwm_period = (pwm_divisor / pwm_frequency) - 1;

  if (!valid_arguments) {
    BPMSG1033;
    pwm_period_float = getnumber(50, 0, 99, 0);
  }

  pwm_period_float /= 100;
  pwm_duty_cycle = pwm_period * pwm_period_float;

  /* Assign output source to AUX pin. */
  AUXPIN_RPOUT = OC5_IO;

  OC5R = pwm_duty_cycle;
  OC5RS = pwm_duty_cycle;

#if defined(BUSPIRATEV4)
  /*
   * OC5CON1
   *
   * MSB
   * --0000--0--00110
   *   ||||  |  |||||
   *   ||||  |  ||+++-- OCM:      Edge-aligned PWM mode.
   *   ||||  |  |+----- TRIGMODE: TRIGSTAT is only cleared by software.
   *   ||||  |  +------ OCFLT0:   No PWM fault condition.
   *   ||||  +--------- ENFLT0:   Fault 0 input disabled.
   *   |+++------------ OCTSEL:   Output compare with Timer 2.
   *   +--------------- OCSIDL:   Output Compare continues in CPU idle mode.
   */
  OC5CON =
      (0b110 << _OC5CON1_OCM_POSITION) | (OFF << _OC5CON1_TRIGMODE_POSITION) |
      (OFF << _OC5CON1_OCFLT0_POSITION) | (OFF << _OC5CON1_ENFLT0_POSITION) |
      (OFF << _OC5CON1_OCTSEL_POSITION) | (OFF << _OC5CON1_OCSIDL_POSITION);
#else
  /*
   * OC5CON
   *
   * MSB
   * --0--------00110
   *   |        |||||
   *   |        ||+++-- OCM:    Edge-aligned PWM mode.
   *   |        |+----- OCTSEL: Output compare with Timer 2.
   *   |        +------ OCFLT:  No PWM fault condition.
   *   +--------------- OCSIDL:   Output Compare continues in CPU idle mode.
   */
  OC5CON = (0b110 << _OC5CON_OCM_POSITION) | (OFF << _OC5CON_OCTSEL_POSITION) |
           (OFF << _OC5CON_OCFLT_POSITION) | (OFF << _OC5CON_OCSIDL_POSITION);
#endif /* BUSPIRATEV4 */

  PR2 = pwm_period;
  T2CONbits.TON = ON;

  BPMSG1034;
  state.mode = AUX_MODE_PWM;
}

void bp_frequency_counter_setup(void) {
  uint64_t frequency;
  uint64_t period;

  if (state.mode == AUX_MODE_PWM) {
    BPMSG1037;
    return;
  }

  BPMSG1038;
  stop_timers();

  /* Connect Timer2 clock to AUX. */
  AUXPIN_DIR = INPUT;
  RPINR3bits.T2CKR = AUXPIN_RPIN;

  /*
   * T2CON
   *
   * MSB
   * 0-0------111-1-
   * | |      ||| |
   * | |      ||| +--- TCS:   Internal clock.
   * | |      ||+----- T32:   TIMER2 is bound with TIMER3 for 32 bit mode.
   * | |      ++------ TCKPS: 1:256 Prescaler.
   * | +-------------- TSIDL: Continue module operation in idle mode.
   * +---------------- TON:   Timer OFF.
   */
  T2CON = (ON << _T2CON_TCS_POSITION) | (ON << _T2CON_T32_POSITION) |
          (ON << _T2CON_TCKPS0_POSITION) | (ON << _T2CON_TCKPS1_POSITION);

  /* Can measure up to 67MHz (26 bits). */
  frequency = poll_frequency_counter_value();

  if (frequency > 0x3FFF) {
    /* Adjust for prescaler. */
    frequency *= 256;
  } else {
    /* Use a less aggressive prescaler, set to 1:1. */
    BPMSG1245;
    T2CONbits.TCKPS0 = OFF;
    T2CONbits.TCKPS1 = OFF;
    frequency = poll_frequency_counter_value();
  }

  if (frequency > 3999) {
    bp_write_dec_dword_friendly(frequency);
    MSG_PWM_HZ_MARKER;
  } else {
    if (frequency > 0) {
      BPMSG1245;
      period = average_sample_frequency(frequency);
      if (period > 400000) {
        frequency = 16e11 / period;
        bp_write_dec_dword_friendly(frequency / 100000);
        user_serial_transmit_character('.');
        frequency = frequency % 100000;
        if (frequency < 10000) {
          user_serial_transmit_character('0');
        }
        if (frequency < 1000) {
          user_serial_transmit_character('0');
        }
        if (frequency < 100) {
          user_serial_transmit_character('0');
        }
        if (frequency < 10) {
          user_serial_transmit_character('0');
        }
        bp_write_dec_dword(frequency);
        goto write_marker;
      }

      if (period > 126491) {
        frequency = 16e10 / period;
        bp_write_dec_dword_friendly(frequency / 10000);
        user_serial_transmit_character('.');
        frequency = frequency % 10000;
        if (frequency < 1000) {
          user_serial_transmit_character('0');
        }
        if (frequency < 100) {
          user_serial_transmit_character('0');
        }
        if (frequency < 10) {
          user_serial_transmit_character('0');
        }
        bp_write_dec_word(frequency);
        goto write_marker;
      }

      if (period > 40000) {
        frequency = 16e9 / period;
        bp_write_dec_dword_friendly(frequency / 1000);
        user_serial_transmit_character('.');
        frequency = frequency % 1000;
        if (frequency < 100) {
          user_serial_transmit_character('0');
        }
        if (frequency < 10) {
          user_serial_transmit_character('0');
        }
        bp_write_dec_word(frequency);
        goto write_marker;
      }

      if (period > 12649) {
        frequency = 16e8 / period;
        bp_write_dec_dword_friendly(frequency / 100);
        user_serial_transmit_character('.');
        frequency = frequency % 100;
        if (frequency < 10) {
          user_serial_transmit_character('0');
        }
        bp_write_dec_byte(frequency);
        goto write_marker;
      }

      frequency = 16e7 / period;
      bp_write_dec_dword_friendly(frequency / 10);
      user_serial_transmit_character('.');
      bp_write_dec_byte(frequency % 10);

    write_marker:
      MSG_PWM_HZ_MARKER;
    } else {
      MSG_PWM_FREQUENCY_TOO_LOW;
    }
  }

  /* Detach Timer2 Clock signal from the AUX pin. */
  RPINR3bits.T2CKR = 0b11111;
  stop_timers();
}

uint32_t bp_measure_frequency(void) {
  uint32_t frequency;

  stop_timers();
  AUXPIN_DIR = INPUT;

  /* Set timer 2 clock input pin. */
  RPINR3bits.T2CKR = AUXPIN_RPIN;

  /* Finish timer 2 setup. */

  /*
   * T2CON
   *
   * MSB
   * 0-0------111-1-
   * | |      ||| |
   * | |      ||| +--- TCS:   Internal clock.
   * | |      ||+----- T32:   TIMER2 is bound with TIMER3 for 32 bit mode.
   * | |      ++------ TCKPS: 1:256 Prescaler.
   * | +-------------- TSIDL: Continue module operation in idle mode.
   * +---------------- TON:   Timer OFF.
   */
  T2CON = (ON << _T2CON_TCS_POSITION) | (ON << _T2CON_T32_POSITION) |
          (ON << _T2CON_TCKPS0_POSITION) | (ON << _T2CON_TCKPS1_POSITION);

  frequency = poll_frequency_counter_value();
  if (frequency > 0xFF) {
    /* Adjust for prescaler. */
    frequency *= 256;
  } else {
    /* Use a less aggressive prescaler, set to 1:1. */
    T2CONbits.TCKPS0 = OFF;
    T2CONbits.TCKPS1 = OFF;
    frequency = poll_frequency_counter_value();
  }

  /* Remove clock input pin assignment. */
  RPINR3bits.T2CKR = 0b011111;

  stop_timers();

  return frequency;
}

uint32_t poll_frequency_counter_value(void) {
  uint32_t counter_low;
  uint32_t counter_high;

  /* Set 32-bits period register for timer #2 (0xFFFFFFFF). */
  PR3 = 0xFFFF;
  PR2 = 0xFFFF;

  /* Clear timer #2 counter. */
  TMR3HLD = 0;
  TMR2 = 0;

  /* Clear timer #4 counter. */
  TMR5HLD = 0;
  TMR4 = 0;

  /* Set timer #4 as 32 bits. */
  T4CONbits.T32 = YES;

  /* Set 32-bits period register for timer #4 (0x00F42400, one second). */
  PR5 = 0x00F4;
  PR4 = 0x2400;

  /* Clear timer #4 interrupt flag (32 bits mode). */
  IFS1bits.T5IF = OFF;

  /* Start timer #4. */
  T4CONbits.TON = ON;

  /* Start timer #2. */
  T2CONbits.TON = ON;

  /* Wait for timer #4 interrupt to occur. */
  while (IFS1bits.T5IF == 0) {
  }

  /* Stop timers. */
  T2CONbits.TON = OFF;
  T4CONbits.TON = OFF;

  /* Timer #2 now contains the frequency value. */
  counter_low = TMR2;
  counter_high = TMR3HLD;

  return (counter_high << 16) + counter_low;
}

#if defined(BUSPIRATEV4)
#define IC1ICBNE IC1CON1bits.ICBNE
#define IC2ICBNE IC2CON1bits.ICBNE
#else
#define IC1ICBNE IC1CONbits.ICBNE
#define IC2ICBNE IC2CONbits.ICBNE
#endif /* BUSPIRATEV4 */

uint32_t average_sample_frequency(const uint16_t count) {
  uint32_t current_low, counter_low, current_high, counter_high, total_samples;
  uint16_t index;

  /* Clear input capture interrupts. */
  IFS0bits.IC2IF = OFF;
  IFS0bits.IC1IF = OFF;

  /* Assign input capture pin. */
  RPINR7bits.IC2R = AUXPIN_RPIN;
  RPINR7bits.IC1R = AUXPIN_RPIN;

#if defined(BUSPIRATEV4)

  /* Setup timer 4 for interval measurement. */
  TMR5HLD = 0x0000;
  TMR4 = 0x0000;

  /* Start timer 4. */

  /*
   * T4CON
   *
   * MSB
   * 1-0------001-0-
   * | |      ||| |
   * | |      ||| +--- TCS:   External clock from pin.
   * | |      ||+----- T32:   TIMER4 is bound with TIMER5 for 32 bit mode.
   * | |      ++------ TCKPS: 1:1 Prescaler.
   * | +-------------- TSIDL: Continue module operation in idle mode.
   * +---------------- TON:   Timer ON.
   */
  T4CON = (ON << _T4CON_TON_POSITION) | (ON << _T4CON_T32_POSITION);

  /* Setup input capture 2. */

  /*
   * IC2CON1
   *
   * MSB
   * --0011---00--011
   *   ||||   ||  |||
   *   ||||   ||  +++-- ICM:    Simple capture mode, on every rising edge.
   *   ||||   ++------- ICI:    Interrupt on every capture event.
   *   |+++------------ ICTSEL: Use input capture timer 5.
   *   +--------------- ICSIDL: Input capture continues on CPU idle mode.
   */
  IC2CON1 = (0b011 << _IC2CON1_ICM_POSITION) | (0b00 << _IC2CON1_ICI_POSITION) |
            (0b011 << _IC2CON1_ICTSEL_POSITION) |
            (OFF << _IC2CON1_ICSIDL_POSITION);

  /*
   * IC2CON2
   *
   * MSB
   * -------000-10100
   *        ||| |||||
   *        ||| +++++-- SYNCSEL:  Use Input Capture 2 as trigger.
   *        ||+-------- TRIGSTAT: Timer source has not been triggered.
   *        |+--------- ICTRIG:   Synchronize input capture with SYNCSEL source.
   *        +---------- IC32:     Do not cascade input capture units.
   */
  IC2CON2 = (0b10100 << _IC2CON2_SYNCSEL_POSITION) |
            (OFF << _IC2CON2_TRIGSTAT_POSITION) |
            (OFF << _IC2CON2_ICTRIG_POSITION) | (OFF << _IC2CON2_IC32_POSITION);

  /* Setup input capture 1. */

  /*
   * IC1CON1
   *
   * --0010---00--011
   *   ||||   ||  |||
   *   ||||   ||  +++-- ICM:    Simple capture mode, on every rising edge.
   *   ||||   ++------- ICI:    Interrupt on every capture event.
   *   |+++------------ ICTSEL: Use input capture timer 4.
   *   +--------------- ICSIDL: Input capture continues on CPU idle mode.
   */
  IC1CON1 = (0b011 << _IC1CON1_ICM_POSITION) | (0b00 << _IC1CON1_ICI_POSITION) |
            (0b010 << _IC1CON1_ICTSEL_POSITION) |
            (OFF << _IC1CON1_ICSIDL_POSITION);

  /*
   * IC1CON2
   *
   * MSB
   * -------000-10100
   *        ||| |||||
   *        ||| +++++-- SYNCSEL:  Use Input Capture 2 as trigger.
   *        ||+-------- TRIGSTAT: Timer source has not been triggered.
   *        |+--------- ICTRIG:   Synchronize input capture with SYNCSEL source.
   *        +---------- IC32:     Do not cascade input capture units.
   */
  IC1CON2 = (0b10100 << _IC1CON2_SYNCSEL_POSITION) |
            (OFF << _IC1CON2_TRIGSTAT_POSITION) |
            (OFF << _IC1CON2_ICTRIG_POSITION) | (OFF << _IC1CON2_IC32_POSITION);

#else

  /* Setup timer 2 for interval measurement. */
  TMR3HLD = 0x0000;
  TMR2 = 0x0000;

  /* Start timer 2. */

  /*
   * T2CON
   *
   * MSB
   * 1-0------001-0-
   * | |      ||| |
   * | |      ||| +--- TCS:   External clock from pin.
   * | |      ||+----- T32:   TIMER2 is bound with TIMER3 for 32 bit mode.
   * | |      ++------ TCKPS: 1:1 Prescaler.
   * | +-------------- TSIDL: Continue module operation in idle mode.
   * +---------------- TON:   Timer ON.
   */
  T2CON = (ON << _T2CON_TON_POSITION) | (ON << _T2CON_T32_POSITION);

  /* Setup Input Capture 2. */

  /*
   * IC2CON
   *
   * MSB
   * --0-----000--011
   *   |     |||  |||
   *   |     |||  +++-- ICM:    Capture every rising edge.
   *   |     |++------- ICI:    Interrupt on every capture event.
   *   |     +--------- ICTMR:  TMR3 contents are captured on event.
   *   +--------------- ICSIDL: Input capture continues on CPU idle.
   */
  IC2CON = (0b011 << _IC2CON_ICM_POSITION) | (0b00 << _IC2CON_ICI_POSITION) |
           (OFF << _IC2CON_ICTMR_POSITION) | (OFF << _IC2CON_ICSIDL_POSITION);

  /* Setup Input Capture 1. */

  /*
   * IC1CON
   *
   * MSB
   * --0-----100--011
   *   |     |||  |||
   *   |     |||  +++-- ICM:    Capture every rising edge.
   *   |     |++------- ICI:    Interrupt on every capture event.
   *   |     +--------- ICTMR:  TMR2 contents are captured on event.
   *   +--------------- ICSIDL: Input capture continues on CPU idle.
   */
  IC1CON = (0b011 << _IC2CON_ICM_POSITION) | (0b00 << _IC2CON_ICI_POSITION) |
           (ON << _IC2CON_ICTMR_POSITION) | (OFF << _IC2CON_ICSIDL_POSITION);

#endif /* BUSPIRATEV4 */

  /* Flush IC1. */
  while (IC1ICBNE == ON) {
    current_low = IC1BUF;
  }

  /* Flush IC2. */
  while (IC2ICBNE == ON) {
    counter_low = IC2BUF;
  }

  while (IC1ICBNE == OFF) {
  }

  counter_low = IC1BUF;
  counter_high = IC2BUF;
  total_samples = 0;

  for (index = 0; index < count; index++) {
    /* Wait for signal. */
    while (IC1ICBNE == OFF) {
    }

    current_low = IC1BUF;
    current_high = IC2BUF;
    total_samples +=
        ((current_high - counter_high) << 16) + (current_low - counter_low);
    counter_high = current_high;
    counter_low = current_low;
  }

#if defined(BUSPIRATEV4)

  /* Stop input capture units. */

  /*
   * IC1CON1
   *
   * --0000---00--000
   *   ||||   ||  |||
   *   ||||   ||  +++-- ICM:    Input capture module turned off.
   *   ||||   ++------- ICI:    Interrupt on every capture event.
   *   |+++------------ ICTSEL: Use input capture timer 3.
   *   +--------------- ICSIDL: Input capture continues on CPU idle mode.
   */
  IC1CON1 = 0x0000;

  /*
   * IC2CON1
   *
   * --0000---00--000
   *   ||||   ||  |||
   *   ||||   ||  +++-- ICM:    Input capture module turned off.
   *   ||||   ++------- ICI:    Interrupt on every capture event.
   *   |+++------------ ICTSEL: Use input capture timer 3.
   *   +--------------- ICSIDL: Input capture continues on CPU idle mode.
   */
  IC2CON1 = 0x0000;

  /* Stop timer 4. */
  T4CONbits.TON = OFF;

#else

  /* Stop Input Capture 1. */

  /*
   * IC1CON
   *
   * MSB
   * --0-----000--000
   *   |     |||  |||
   *   |     |||  +++-- ICM:    Capture module turned off.
   *   |     |++------- ICI:    Interrupt on every capture event.
   *   |     +--------- ICTMR:  TMR3 contents are captured on event.
   *   +--------------- ICSIDL: Input capture continues on CPU idle.
   */
  IC1CON = 0x0000;

  /* Stop Input Capture 2. */

  /*
   * IC2CON
   *
   * MSB
   * --0-----000--000
   *   |     |||  |||
   *   |     |||  +++-- ICM:    Capture module turned off.
   *   |     |++------- ICI:    Interrupt on every capture event.
   *   |     +--------- ICTMR:  TMR3 contents are captured on event.
   *   +--------------- ICSIDL: Input capture continues on CPU idle.
   */
  IC2CON = 0x0000;

  /* Stop timer 2. */
  T2CONbits.TON = OFF;

#endif /* BUSPIRATEV4 */

  return total_samples / count;
}

void bp_aux_pin_set_high_impedance(void) {
#ifdef BUSPIRATEV3
  if (mode_configuration.alternate_aux == 0) {
    BP_AUX0_DIR = INPUT;
  } else {
    BP_CS_DIR = INPUT;
  }
#else
  switch (mode_configuration.alternate_aux) {
  case 0:
    BP_AUX0_DIR = INPUT;
    break;

  case 1:
    BP_CS_DIR = INPUT;
    break;

  case 2:
    BP_AUX1_DIR = INPUT;
    break;

  case 3:
    BP_AUX2_DIR = INPUT;
    break;

  default:
    break;
  }
#endif /* BUSPIRATEV3 */

  BPMSG1039;
}

void bp_aux_pin_set_high(void) {
#ifdef BUSPIRATEV3
  if (mode_configuration.alternate_aux == 0) {
    BP_AUX0_DIR = OUTPUT;
    BP_AUX0 = HIGH;
  } else {
    BP_CS_DIR = OUTPUT;
    BP_CS = HIGH;
  }
#else
  switch (mode_configuration.alternate_aux) {
  case 0:
    BP_AUX0_DIR = OUTPUT;
    BP_AUX0 = HIGH;
    break;

  case 1:
    BP_CS_DIR = OUTPUT;
    BP_CS = HIGH;
    break;

  case 2:
    BP_AUX1_DIR = OUTPUT;
    BP_AUX1 = HIGH;
    break;

  case 3:
    BP_AUX2_DIR = OUTPUT;
    BP_AUX2 = HIGH;
    break;

  default:
    break;
  }
#endif /* BUSPIRATEV3 */

  BPMSG1040;
}

void bp_aux_pin_set_low(void) {
#ifdef BUSPIRATEV3
  if (mode_configuration.alternate_aux == 0) {
    BP_AUX0_DIR = OUTPUT;
    BP_AUX0 = LOW;
  } else {
    BP_CS_DIR = OUTPUT;
    BP_CS = LOW;
  }
#else
  switch (mode_configuration.alternate_aux) {
  case 0:
    BP_AUX0_DIR = OUTPUT;
    BP_AUX0 = LOW;
    break;

  case 1:
    BP_CS_DIR = OUTPUT;
    BP_CS = LOW;
    break;

  case 2:
    BP_AUX1_DIR = OUTPUT;
    BP_AUX1 = LOW;
    break;

  case 3:
    BP_AUX2_DIR = OUTPUT;
    BP_AUX2 = LOW;
    break;

  default:
    break;
  }
#endif /* BUSPIRATEV3 */

  BPMSG1041;
}

bool bp_aux_pin_read(void) {
#ifdef BUSPIRATEV3
  if (mode_configuration.alternate_aux == 0) {
    BP_AUX0_DIR = INPUT;
    Nop();
    Nop();
    return BP_AUX0;
  }

  BP_CS_DIR = INPUT;
  Nop();
  Nop();
  return BP_CS;
#else
  switch (MASKBOTTOM8(mode_configuration.alternate_aux, 2)) {
  case 0:
    BP_AUX0_DIR = INPUT;
    Nop();
    Nop();
    return BP_AUX0;

  case 1:
    BP_CS_DIR = INPUT;
    Nop();
    Nop();
    return BP_CS;

  case 2:
    BP_AUX1_DIR = INPUT;
    Nop();
    Nop();
    return BP_AUX1;

  case 3:
    BP_AUX2_DIR = INPUT;
    Nop();
    Nop();
    return BP_AUX2;

  default:
    /* Should not happen. */
    return LOW;
  }
#endif /* BUSPIRATEV3 */
}

void bp_aux_pin_set_change_notification(const bool state) {
#ifdef BUSPIRATEV3
  if (mode_configuration.alternate_aux == 0) {
    BP_AUX0_CN = state;
  } else {
    BP_CS_CN = state;
  }
#else
  switch (MASKBOTTOM8(mode_configuration.alternate_aux, 2)) {
  case 0:
    BP_AUX0_CN = state;
    break;

  case 1:
    BP_CS_CN = state;
    break;

  case 2:
    BP_AUX1_CN = state;
    break;

  case 3:
    BP_AUX2_CN = state;
    break;

  default:
    break;
  }
#endif /* BUSPIRATEV3 */
}

void bp_servo_setup(void) {
  uint16_t period;
  uint16_t duty_cycle;
  bool keep_asking;
  float pwm_period;

  stop_timers();

  /* Reset output comparator. */

#if defined(BUSPIRATEV4)
  /*
   * OC5CON1
   *
   * MSB
   * --0000--0--00000
   *   ||||  |  |||||
   *   ||||  |  ||+++-- OCM:      Output compare is disabled.
   *   ||||  |  |+----- TRIGMODE: TRIGSTAT is only cleared by software.
   *   ||||  |  +------ OCFLT0:   No PWM fault condition.
   *   ||||  +--------- ENFLT0:   Fault 0 input disabled.
   *   |+++------------ OCTSEL:   Output compare with Timer 2.
   *   +--------------- OCSIDL:   Output Compare continues in CPU idle mode.
   */
  OC5CON = 0x0000;
#else
  /*
   * OC5CON
   *
   * MSB
   * --0--------00000
   *   |        |||||
   *   |        ||+++-- OCM:    Output compare is disabled.
   *   |        |+----- OCTSEL: Output compare with Timer 2.
   *   |        +------ OCFLT:  No PWM fault condition.
   *   +--------------- OCSIDL: Output Compare continues in CPU idle mode.
   */
  OC5CON = 0x0000;
#endif /* BUSPIRATEV4 */

  if ((state.mode == AUX_MODE_PWM) &&
      (cmdbuf[((cmdstart + 1) & CMDLENMSK)] == 0x00)) {
    AUXPIN_RPOUT = 0;
    BPMSG1028;
    state.mode = AUX_MODE_IO;
    return;
  }

  keep_asking = false;

  cmdstart = (cmdstart + 1) & CMDLENMSK;
  consumewhitechars();
  pwm_period = getint();
  if ((mode_configuration.command_error == YES)|| (pwm_period > 180)) {
    mode_configuration.command_error = NO;
    BPMSG1254;
    pwm_period = getnumber(90, 0, 180, 0);
    keep_asking = true;
  }

  do {
    T2CONbits.TCKPS1 = ON;
    T2CONbits.TCKPS0 = ON;
    period = 1250;
    pwm_period /= 3500;
    duty_cycle = (period * pwm_period) + 62;
    AUXPIN_RPOUT = OC5_IO;
    OC5R = duty_cycle;
    OC5RS = duty_cycle;

#if defined(BUSPIRATEV4)
    /*
     * OC5CON1
     *
     * MSB
     * --0000--0--00110
     *   ||||  |  |||||
     *   ||||  |  ||+++-- OCM:      Edge-aligned PWM mode.
     *   ||||  |  |+----- TRIGMODE: TRIGSTAT is only cleared by software.
     *   ||||  |  +------ OCFLT0:   No PWM fault condition.
     *   ||||  +--------- ENFLT0:   Fault 0 input disabled.
     *   |+++------------ OCTSEL:   Output compare with Timer 2.
     *   +--------------- OCSIDL:   Output Compare continues in CPU idle mode.
     */
    OC5CON =
        (0b110 << _OC5CON1_OCM_POSITION) | (OFF << _OC5CON1_TRIGMODE_POSITION) |
        (OFF << _OC5CON1_OCFLT0_POSITION) | (OFF << _OC5CON1_ENFLT0_POSITION) |
        (OFF << _OC5CON1_OCTSEL_POSITION) | (OFF << _OC5CON1_OCSIDL_POSITION);
#else
    /*
     * OC5CON
     *
     * MSB
     * --0--------00110
     *   |        |||||
     *   |        ||+++-- OCM:    Edge-aligned PWM mode.
     *   |        |+----- OCTSEL: Output compare with Timer 2.
     *   |        +------ OCFLT:  No PWM fault condition.
     *   +--------------- OCSIDL:   Output Compare continues in CPU idle mode.
     */
    OC5CON = (0b110 << _OC5CON_OCM_POSITION) |
             (OFF << _OC5CON_OCTSEL_POSITION) |
             (OFF << _OC5CON_OCFLT_POSITION) | (OFF << _OC5CON_OCSIDL_POSITION);
#endif /* BUSPIRATEV4 */

    PR2 = period;

    /* Start timer 2. */
    T2CONbits.TON = ON;
    BPMSG1255;
    state.mode = AUX_MODE_PWM;

    if (keep_asking) {
      pwm_period = getnumber(-1, 0, 180, 1);
      if (pwm_period < 0) {
        bpBR;
        return;
      }
    }
  } while (keep_asking == true);
}

void stop_timers(void) {

  /*
   * T4CON
   *
   * MSB
   * 0-0------000-0-
   * | |      ||| |
   * | |      ||| +--- TCS:   External clock from pin.
   * | |      ||+----- T32:   TIMER4 is not bound with TIMER5 for 32 bit mode.
   * | |      ++------ TCKPS: 1:1 Prescaler.
   * | +-------------- TSIDL: Continue module operation in idle mode.
   * +---------------- TON:   Timer OFF.
   */
  T4CON = 0x0000;

  /*
   * T2CON
   *
   * MSB
   * 0-0------000-0-
   * | |      ||| |
   * | |      ||| +--- TCS:   External clock from pin.
   * | |      ||+----- T32:   TIMER2 is not bound with TIMER3 for 32 bit mode.
   * | |      ++------ TCKPS: 1:1 Prescaler.
   * | +-------------- TSIDL: Continue module operation in idle mode.
   * +---------------- TON:   Timer OFF.
   */
  T2CON = 0x0000;
}
