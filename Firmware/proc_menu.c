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

#include <stdbool.h>
#include <stddef.h>

#include "configuration.h"

#include "base.h"
#include "aux_pin.h"
#include "core.h"
#include "proc_menu.h" //need our public versionInfo() function
#include "selftest.h"
#include "binary_io.h"
#include "sump.h"
#include "basic.h"

extern bus_pirate_configuration_t bus_pirate_configuration;
extern mode_configuration_t mode_configuration;
extern command_t last_command;
extern bus_pirate_protocol_t enabled_protocols[ENABLED_PROTOCOLS_COUNT];

#ifdef BUSPIRATEV4
static bool __attribute__((address(0x47FA),persistent)) skip_pgc_pgd_check;
#else
static bool __attribute__((address(0x27FA),persistent)) skip_pgc_pgd_check;
#endif /* BUSPIRATEV4 */

#ifdef BUSPIRATEV4
extern volatile BYTE cdc_Out_len;
#endif /* BUSPIRATEV4 */

static const uint8_t READ_DISPLAY_BASE[] = { 'x', 'd', 'b', 'w' };

static uint8_t change_read_display(void);

static void setDisplayMode(void); //user terminal number display mode dialog (eg HEX, DEC, BIN, RAW)
static void set_baud_rate(void); //configure user terminal side UART baud rate
static void statusInfo(void); //display properties of the current bus mode (pullups, vreg, lsb, output type, etc)

/**
 * Outputs '0' to the serial port if the given value is zero, '1' otherwise.
 * 
 * @param[in] value the value to test the state for.
 */
static void echo_state(const uint16_t value);

/**
 * Outputs both direction and state to the serial port for all available pins.
 */
static void print_pins_information(void);

/**
 * Outputs the direction (input/output) of the given pin to the serial port.
 * 
 * @warning if the pin bit mask has more than one bit set to one, the result
 * will print the AND result of all directions marked by the pin mask.  This
 * should probably be not used that way, as it does not make much sense in the
 * first place.
 * 
 * @param[in] pin the pin to use for printing signal direction.
 */
static void print_pin_direction(uint16_t pin);

/**
 * Outputs the state (low/high) of the given pin to the serial port.
 * 
 * @warning if the pin bit mask has more than one bit set to one, the result
 * will print the AND result of all states marked by the pin mask.  This
 * should probably be not used that way, as it does not make much sense in the
 * first place.
 * 
 * @param[in] pin the pin to use for printing signal state.
 */
static void print_pin_state(uint16_t pin);

#ifdef BUSPIRATEV4
void setPullupVoltage(void); // onboard Vpu selection
#endif /* BUSPIRATEV4 */

//global vars    move to bpconfig structure?
char cmdbuf[BP_COMMAND_BUFFER_SIZE] = { 0 };
unsigned int cmdend;
unsigned int cmdstart;

/**
 * Flag indicating whether an error condition was detected in the previous
 * command execution.
 */
bool command_error;

static char user_macros[BP_USER_MACROS_COUNT][BP_USER_MACRO_MAX_LENGTH];
static int user_macro;

void serviceuser(void) {
    int cmd, stop;
    int newstart;
    int oldstart;
    unsigned int sendw, received;
    int repeat;
    unsigned char c;
    int temp;
    int temp2;
    int binmodecnt;
    int numbits;
    unsigned int tmpcmdend, histcnt, tmphistcnt;
    unsigned char oldDmode;//temporarily holds the default display mode, while a different display read is performed
    unsigned char newDmode;
    
    // init
    cmd = 0;
    cmdstart = 0;
    cmdend = 0;
    tmpcmdend = cmdend;
    histcnt = 0;
    tmphistcnt = 0;
    bus_pirate_configuration.bus_mode = BP_HIZ;
    temp2 = 0;
    command_error = false;
    binmodecnt = 0;

    stop = 0;
    newstart = 0;
    oldstart = 0;
    
    oldDmode=0;//temporarily holds the default display mode, while a different display read is performed
    newDmode=0;
    
    memset(user_macros, 0, sizeof(user_macros));
    user_macro = 0;

    for (;;) {
        bp_write_string(enabled_protocols[bus_pirate_configuration.bus_mode].name);
#ifdef BP_ENABLE_BASIC_SUPPORT
        if (bus_pirate_configuration.basic) {
            //bpWstring("(BASIC)");
            BPMSG1084;
        }
#endif /* BP_ENABLE_BASIC_SUPPORT */
        bp_write_string(">");
        while (!cmd) {
            if (user_macro) {
                user_macro--;
                temp = 0;
                while (user_macros[user_macro][temp]) {
                    cmdbuf[cmdend] = user_macros[user_macro][temp];
                    user_serial_transmit_character(user_macros[user_macro][temp]);
                    cmdend++;
                    temp++;
                    cmdend &= CMDLENMSK;
                }
                user_macro = 0;
            }

            while (!user_serial_ready_to_read()) // as long as there is no user input poll periodicservice
            {
                if (mode_configuration.periodicService == 1) {
                    if (enabled_protocols[bus_pirate_configuration.bus_mode].periodic_update()) // did we print something?
                    {
                        bp_write_string(enabled_protocols[bus_pirate_configuration.bus_mode].name);
                        bp_write_string(">");
                        if (cmdstart != cmdend) {
                            for (temp = cmdstart; temp != cmdend; temp++) {
                                user_serial_transmit_character(cmdbuf[temp]);
                                temp &= CMDLENMSK;
                            }
                        }
                    }
                }

#ifdef BUSPIRATEV4
                if (!BP_BUTTON) { // button pressed
                } else { // button not pressed
                }
#endif /* BUSPIRATEV4 */
            }

            if (user_serial_check_overflow()) { //check for user terminal buffer overflow error
                user_serial_clear_overflow();
                continue; //resume getting more user input
            } else {
                c = user_serial_read_byte(); //no error, process byte
            }

            switch (c) {
                case 0x08: // backspace(^H)
                    if (tmpcmdend != cmdstart) // not at begining?
                    {
                        if (tmpcmdend == cmdend) // at the end?
                        {
                            cmdend = (cmdend - 1) & CMDLENMSK;
                            cmdbuf[cmdend] = 0x00; // add end marker
                            tmpcmdend = cmdend; // update temp
                            bp_write_string("\x08 \x08"); // destructive backspace ian !! :P
                        } else // not at end, left arrow used
                        {
                            repeat = 0; // use as temp, not valid here anyway
                            tmpcmdend = (tmpcmdend - 1) & CMDLENMSK;
                            bp_write_string("\x1B[D"); // move left
                            for (temp = tmpcmdend; temp != cmdend; temp = (temp + 1) & CMDLENMSK) {
                                cmdbuf[temp] = cmdbuf[temp + 1];
                                if (cmdbuf[temp]) // not NULL
                                    user_serial_transmit_character(cmdbuf[temp]);
                                else
                                    user_serial_transmit_character(0x20);
                                repeat++;
                            }
                            cmdend = (cmdend - 1) & CMDLENMSK; // end pointer moves left one
                            bp_write_string("\x1B["); // move left
                            bp_write_dec_byte(repeat); // to original
                            bp_write_string("D"); // cursor position
                        }
                    } else {
                        user_serial_transmit_character(BELL); // beep, at begining
                    }
                    break;
                case 0x04: // delete (^D)
                case 0x7F: // delete key
                    if (tmpcmdend != cmdend) // not at the end
                    {
                        repeat = 0; // use as temp, not valid here anyway
                        for (temp = tmpcmdend; temp != cmdend; temp = (temp + 1) & CMDLENMSK) {
                            cmdbuf[temp] = cmdbuf[temp + 1];
                            if (cmdbuf[temp]) // not NULL
                                user_serial_transmit_character(cmdbuf[temp]);
                            else
                                user_serial_transmit_character(0x20);
                            repeat++;
                        }
                        cmdend = (cmdend - 1) & CMDLENMSK; // end pointer moves left one
                        bp_write_string("\x1B["); // move left
                        bp_write_dec_byte(repeat); // to original
                        bp_write_string("D"); // cursor position
                    } else {
                        user_serial_transmit_character(BELL); // beep, at end
                    }
                    break;
                case 0x1B: // escape
                    c = user_serial_read_byte(); // get next char
                    if (c == '[') // got CSI
                    {
                        c = user_serial_read_byte(); // get next char
                        switch (c) {
                            case 'D': // left arrow
                                goto left;
                                break;
                            case 'C': // right arrow
                                goto right;
                                break;
                            case 'A': // up arrow
                                goto up;
                                break;
                            case 'B': // down arrow
                                goto down;
                                break;
                            case '1': // VT100+ home key (example use in PuTTY)
                                c = user_serial_read_byte();
                                if (c == '~') goto home;
                                break;
                            case '4': // VT100+ end key (example use in PuTTY)
                                c = user_serial_read_byte();
                                if (c == '~') goto end;
                                break;
                        }
                    }
                    break;
left:
                case 0x02: // ^B (left arrow) or SUMP
#ifdef BP_ENABLE_SUMP_SUPPORT                    
                    if (binmodecnt >= 5) {
                        enter_sump_mode();
                        binmodecnt = 0; // do we get here or not?
                    } else // ^B (left arrow)
#endif /* BP_ENABLE_SUMP_SUPPORT */
                    {
                        if (tmpcmdend != cmdstart) // at the begining?
                        {
                            tmpcmdend = (tmpcmdend - 1) & CMDLENMSK;
                            bp_write_string("\x1B[D"); // move left
                        } else {
                            user_serial_transmit_character(BELL); // beep, at begining
                        }
                    }
                    break;
right:
                case 0x06: // ^F (right arrow)
                    if (tmpcmdend != cmdend) // ^F (right arrow)
                    { // ensure not at end
                        tmpcmdend = (tmpcmdend + 1) & CMDLENMSK;
                        bp_write_string("\x1B[C"); // move right
                    } else {
                        user_serial_transmit_character(BELL); // beep, at end
                    }
                    break;
up:
                case 0x10: // ^P (up arrow)
                    tmphistcnt = 0; // reset counter
                    for (temp = (cmdstart - 1) & CMDLENMSK; temp != cmdend; temp = (temp - 1) & CMDLENMSK) {
                        if (!cmdbuf[temp] && cmdbuf[(temp - 1) & CMDLENMSK]) { // found previous entry, temp is old cmdend
                            tmphistcnt++;
                            if (tmphistcnt > histcnt) {
                                histcnt++;
                                if (cmdstart != cmdend) { // clear partially entered cmd line
                                    while (cmdend != cmdstart) {
                                        cmdbuf[cmdend] = 0x00;
                                        cmdend = (cmdend - 1) & CMDLENMSK;
                                    }
                                    cmdbuf[cmdend] = 0x00;
                                }
                                repeat = (temp - 1) & CMDLENMSK;
                                while (repeat != cmdend) {
                                    if (!cmdbuf[repeat]) {
                                        temp2 = (repeat + 1) & CMDLENMSK;
                                        /* start of old cmd */
                                        break;
                                    }
                                    repeat = (repeat - 1) & CMDLENMSK;
                                }
                                bp_write_string("\x1B[2K\x0D"); // clear line, CR
                                bp_write_string(enabled_protocols[bus_pirate_configuration.bus_mode].name);
#ifdef BP_ENABLE_BASIC_SUPPORT
                                if (bus_pirate_configuration.basic) {
                                    BPMSG1084;
                                }
#endif /* BP_ENABLE_BASIC_SUPPORT */
                                bp_write_string(">");
                                for (repeat = temp2; repeat != temp; repeat = (repeat + 1) & CMDLENMSK) {
                                    user_serial_transmit_character(cmdbuf[repeat]);
                                    cmdbuf[cmdend] = cmdbuf[repeat];
                                    cmdend = (cmdend + 1) & CMDLENMSK;
                                }
                                cmdbuf[cmdend] = 0x00;
                                tmpcmdend = cmdend; // resync
                                break;
                            }
                        }
                    }
                    if (temp == cmdend) user_serial_transmit_character(BELL); // beep, top
                    break;
down:
                case 0x0E: // ^N (down arrow)
                    tmphistcnt = 0; // reset counter
                    for (temp = (cmdstart - 1) & CMDLENMSK; temp != cmdend; temp = (temp - 1) & CMDLENMSK) {
                        if (!cmdbuf[temp] && cmdbuf[(temp - 1) & CMDLENMSK]) { // found previous entry, temp is old cmdend
                            tmphistcnt++;
                            if (tmphistcnt == (histcnt - 1)) {
                                histcnt--;
                                if (cmdstart != cmdend) { // clear partially entered cmd line
                                    while (cmdend != cmdstart) {
                                        cmdbuf[cmdend] = 0x00;
                                        cmdend = (cmdend - 1) & CMDLENMSK;
                                    }
                                    cmdbuf[cmdend] = 0x00;
                                }
                                repeat = (temp - 1) & CMDLENMSK;
                                while (repeat != cmdend) {
                                    if (!cmdbuf[repeat]) {
                                        temp2 = (repeat + 1) & CMDLENMSK;
                                        /* start of old cmd */
                                        break;
                                    }
                                    repeat = (repeat - 1) & CMDLENMSK;
                                }
                                bp_write_string("\x1B[2K\x0D"); // clear line, CR
                                bp_write_string(enabled_protocols[bus_pirate_configuration.bus_mode].name);
#ifdef BP_ENABLE_BASIC_SUPPORT
                                if (bus_pirate_configuration.basic) {
                                    BPMSG1084;
                                }
#endif /* BP_ENABLE_BASIC_SUPPORT */
                                bp_write_string(">");
                                for (repeat = temp2; repeat != temp; repeat = (repeat + 1) & CMDLENMSK) {
                                    user_serial_transmit_character(cmdbuf[repeat]);
                                    cmdbuf[cmdend] = cmdbuf[repeat];
                                    cmdend = (cmdend + 1) & CMDLENMSK;
                                }
                                cmdbuf[cmdend] = 0x00;
                                tmpcmdend = cmdend; // resync
                                break;
                            }
                        }
                    }
                    if (temp == cmdend) {
                        if (histcnt == 1) {
                            bp_write_string("\x1B[2K\x0D"); // clear line, CR
                            bp_write_string(enabled_protocols[bus_pirate_configuration.bus_mode].name);
#ifdef BP_ENABLE_BASIC_SUPPORT
                            if (bus_pirate_configuration.basic) {
                                BPMSG1084;
                            }
#endif /* BP_ENABLE_BASIC_SUPPORT */
                            bp_write_string(">");
                            while (cmdend != cmdstart) {
                                cmdbuf[cmdend] = 0x00;
                                cmdend = (cmdend - 1) & CMDLENMSK;
                            }
                            cmdbuf[cmdend] = 0x00;
                            tmpcmdend = cmdend; // resync
                            histcnt = 0;
                        } else user_serial_transmit_character(BELL); // beep, top
                    }
                    break;
home:
                    case 0x01: // ^A (goto begining of line)
                    if (tmpcmdend != cmdstart) {
                        repeat = (tmpcmdend - cmdstart) & CMDLENMSK;
                        bp_write_string("\x1B["); // move left
                        bp_write_dec_byte(repeat); // to start
                        bp_write_string("D"); // of command line
                        tmpcmdend = cmdstart;
                    } else {
                        user_serial_transmit_character(BELL); // beep, at start
                    }
                    break;
end:
                    case 0x05: // ^E (goto end of line)
                    if (tmpcmdend != cmdend) {
                        repeat = (cmdend - tmpcmdend) & CMDLENMSK;
                        bp_write_string("\x1B["); // move right
                        bp_write_dec_byte(repeat); // to end
                        bp_write_string("C"); // of command line
                        tmpcmdend = cmdend;
                    } else {
                        user_serial_transmit_character(BELL); // beep, at end
                    }
                    break;
                case 0x0A: // Does any terminal only send a CR?
                case 0x0D: // Enter pressed (LF)
                    cmd = 1; // command received
                    histcnt = 0; // reset counter
                    cmdbuf[cmdend] = 0x00; // use to find history
                    cmdend = (cmdend + 1) & CMDLENMSK;
                    tmpcmdend = cmdend; // resync
                    bpBR;
                    break;
                case 0x00:
                    binmodecnt++;
                    if (binmodecnt == 20) {
                        binBB();
#ifdef BUSPIRATEV4
                        binmodecnt = 0; //no reset, cleanup manually 
                        goto bpv4reset; //versionInfo(); //and simulate reset for dependent apps (looking at you AVR dude!)
#endif /* BUSPIRATEV4 */
                    }
                    break;
                    
                default:
                    if ((((cmdend + 1) & CMDLENMSK) != cmdstart) && (c >= 0x20) && (c < 0x7F)) { // no overflow and printable
                        if (cmdend == tmpcmdend) // adding to the end
                        {
                            user_serial_transmit_character(c); // echo back
                            cmdbuf[cmdend] = c; // store char
                            cmdend = (cmdend + 1) & CMDLENMSK;
                            cmdbuf[cmdend] = 0x00; // add end marker
                            tmpcmdend = cmdend; // update temp
                        } else // not at end, left arrow used
                        {
                            repeat = (cmdend - tmpcmdend) & CMDLENMSK;
                            bp_write_string("\x1B["); // move right
                            bp_write_dec_byte(repeat); // to end
                            bp_write_string("C"); // of line
                            temp = cmdend;
                            while (temp != ((tmpcmdend - 1) & CMDLENMSK)) {
                                cmdbuf[temp + 1] = cmdbuf[temp];
                                if (cmdbuf[temp]) // not NULL
                                {
                                    user_serial_transmit_character(cmdbuf[temp]);
                                    bp_write_string("\x1B[2D"); // left 2
                                }
                                temp = (temp - 1) & CMDLENMSK;
                            }
                            user_serial_transmit_character(c); // echo back
                            cmdbuf[tmpcmdend] = c; // store char
                            tmpcmdend = (tmpcmdend + 1) & CMDLENMSK;
                            cmdend = (cmdend + 1) & CMDLENMSK;
                        }
                    } else {
                        user_serial_transmit_character(BELL); // beep, overflow or non printable
                    } //default:
            } //switch(c)
        } //while(!cmd)

        newstart = cmdend;
        oldstart = cmdstart;
        cmd = 0;

        stop = 0;
        command_error = false;

#ifdef BP_ENABLE_BASIC_SUPPORT
        if (bus_pirate_configuration.basic) {
            bp_basic_enter_interactive_interpreter();
            //bpWline("Ready.");
            BPMSG1085;
            stop = 1;
        }
#endif /* BP_ENABLE_BASIC_SUPPORT */

        oldDmode=0;//temporarily holds the default display mode, while a different display read is performed
        newDmode=0;
        while (!stop) {
            c = cmdbuf[cmdstart];

            switch (c) { // generic commands (not bus specific)
                case 'h': //bpWline("-command history");
#ifdef BP_ENABLE_COMMAND_HISTORY
                    if (!cmdhistory()) {
                        oldstart = cmdstart;
                        newstart = cmdend;
                    }
#endif /* BP_ENABLE_COMMAND_HISTORY */
                    break;

                case '?': //bpWline("-HELP");
                    print_help();
                    break;
                case 'i': //bpWline("-Status info");
                    print_version_info(); //display hardware and firmware version string
                    if (bus_pirate_configuration.bus_mode != BP_HIZ) {
                        statusInfo();
                    }
                    break;
                case 'm': //bpWline("-mode change");
                    changemode();
                    break;
                case 'b': //bpWline("-terminal speed set");
                    set_baud_rate();
                    break;
                case 'o': //bpWline("-data output set");
                    setDisplayMode();
                    break;
                case 'v': //bpWline("-check supply voltage");
                    print_pins_information();
                    //measureSupplyVoltages();
                    break;
                case 'f': //bpWline("-frequency count on AUX");
                    bp_frequency_counter_setup();
                    break;
                case 'g':
                    if (bus_pirate_configuration.bus_mode == BP_HIZ) { //bpWmessage(MSG_ERROR_MODE);
                        BPMSG1088;
                    } else {
                        bp_pwm_setup();
                    }
                    break;
                case 'c': //bpWline("-aux pin assigment");
                    mode_configuration.alternate_aux = 0;
                    //bpWmessage(MSG_OPT_AUXPIN_AUX);
                    BPMSG1086;
                    break;
                case 'C': //bpWline("-aux pin assigment");
                    mode_configuration.alternate_aux = 1;
                    //bpWmessage(MSG_OPT_AUXPIN_CS);
                    BPMSG1087;
                    break;
#ifdef BUSPIRATEV4
                case 'k': mode_configuration.alternate_aux = 2;
                    //bpWline("AUX1 selected");
                    BPMSG1263;
                    break;
                case 'K': mode_configuration.alternate_aux = 3;
                    //bpWline("AUX2 selected");
                    BPMSG1264;
                    break;
#endif /* BUSPIRATEV4 */
                case 'L':
                    mode_configuration.little_endian = YES;
                    BPMSG1124;
                    bpBR;
                    break;
                case 'l': 
                    mode_configuration.little_endian = NO;
                    BPMSG1123;
                    bpBR;
                    break;
                case 'p': 
                    //bpWline("-pullup resistors off");
                    //don't allow pullups on some modules. also: V0a limitation of 2 resistors
                    if (bus_pirate_configuration.bus_mode == BP_HIZ) { //bpWmessage(MSG_ERROR_MODE);
                        BPMSG1088;
                    } else {
                        BP_PULLUP_OFF(); //pseudofunction in hardwarevx.h
                        //								modeConfig.pullupEN=0;
                        //bpWmessage(MSG_OPT_PULLUP_OFF);
                        BPMSG1089;
                        bpBR;
                    }
                    break;
                case 'P': //bpWline("-pullup resistors on");
                    //don't allow pullups on some modules. also: V0a limitation of 2 resistors
                    if (bus_pirate_configuration.bus_mode == BP_HIZ) { //bpWmessage(MSG_ERROR_MODE);
                        BPMSG1088;
                    } else {
                        if (mode_configuration.high_impedance == 0) { //bpWmessage(MSG_ERROR_NOTHIZPIN);
                            BPMSG1209;
                        }
                        BP_PULLUP_ON(); //pseudofunction in hardwarevx.h
                        //								modeConfig.pullupEN=1;
                        //bpWmessage(MSG_OPT_PULLUP_ON);
                        BPMSG1091;
                        bpBR;

                        ADCON();
                        if (bp_read_adc(BP_ADC_VPU) < 0x50) {
                            /* No voltage on pullup pin detected. */
                            MSG_NO_VOLTAGE_ON_PULLUP_PIN;
                        }
                        ADCOFF();
                    }
                    break;
#ifdef BUSPIRATEV4
                case 'e': setPullupVoltage();
                    break;
#endif /* BUSPIRATEV4 */
                case '=':
                    cmdstart = (cmdstart + 1) & CMDLENMSK;
                    consumewhitechars();
                    temp = getint();
                    bp_write_hex_byte(temp);
                    MSG_BASE_CONVERTER_EQUAL_SIGN;
                    bp_write_dec_byte(temp);
                    MSG_BASE_CONVERTER_EQUAL_SIGN;
                    bp_write_bin_byte(temp);
                    bpBR;
                    break;
                case '|':
                    cmdstart = (cmdstart + 1) & CMDLENMSK;
                    consumewhitechars();
                    temp = getint();
                    temp = bp_reverse_integer((uint8_t) temp, mode_configuration.numbits);
                    bp_write_hex_byte(temp);
                    MSG_BASE_CONVERTER_EQUAL_SIGN;
                    bp_write_dec_byte(temp);
                    MSG_BASE_CONVERTER_EQUAL_SIGN;
                    bp_write_bin_byte(temp);
                    bpBR;
                    break;
                case '~':
                    if (bus_pirate_configuration.bus_mode == BP_HIZ) {
                        perform_selftest(true, true);
                    } else {
                        //bpWline(OUMSG_PM_SELFTEST_HIZ);
                        BPMSG1092;
                    }
                    break;
                case '#':
#ifdef BUSPIRATEV4
                    MSG_RESET_MESSAGE;
bpv4reset:
                    print_version_info();
#else
                    BPMSG1093;
                    user_serial_wait_transmission_done(); //wait until TX finishes
                    __asm volatile ("RESET");
#endif /* BUSPIRATEV4 */
                    break;
                case '$': 
                    //bpWline("-bootloader jump");
                    if (agree()) { //bpWline("BOOTLOADER");
                        skip_pgc_pgd_check = true;
                        BPMSG1094;
                        bp_delay_ms(100);
                        bp_reset_board_state(); // turn off nasty things, cleanup first needed?
                        user_serial_wait_transmission_done(); //wait until TX finishes
                        __asm volatile ("RESET");
                    }
                    break;
                case 'a':
                    //bpWline("-AUX low");
                    repeat = getrepeat() + 1;
                    while (--repeat) bp_aux_pin_set_low();
                    break;
                case 'A':
                    //bpWline("-AUX hi");
                    repeat = getrepeat() + 1;
                    while (--repeat) bp_aux_pin_set_high();
                    break;
                case '@':
                    //bpWline("-Aux read");
                    repeat = getrepeat() + 1;
                    while (--repeat) { //bpWstring(OUMSG_AUX_INPUT_READ);
                        BPMSG1095;
                        echo_state(bp_aux_pin_read());
                        bpBR;
                    }
                    break;
                case 'W': //bpWline("-PSU on");	//enable any active power supplies
                    if (bus_pirate_configuration.bus_mode == BP_HIZ) { //bpWmessage(MSG_ERROR_MODE);
                        BPMSG1088;
                    } else {
                        BP_VREG_ON();
                        ADCON(); // turn ADC ON
                        bp_delay_ms(2); //wait for VREG to come up

                        if ((bp_read_adc(BP_ADC_3V3) > V33L) && (bp_read_adc(BP_ADC_5V0) > V5L)) { //voltages are correct
                            //bpWmessage(MSG_VREG_ON);
                            BPMSG1096;
                            bpBR;
                            //modeConfig.vregEN=1;
                            
                            //Engaging Clutch
                            //finishes the set up and connects the pins...
                            enabled_protocols[bus_pirate_configuration.bus_mode].setup_execute();
                            MSG_CLUTCH_ENGAGED;
                        } else {
                            BP_VREG_OFF();
                            MSG_VREG_TOO_LOW;
                            BPMSG1097;
                            bpBR;
                        }
                        ADCOFF(); // turn ADC OFF
                    }
                    break;
                case 'w': //bpWline("-PSU off");	//disable the power supplies
                    if (bus_pirate_configuration.bus_mode == BP_HIZ) { //bpWmessage(MSG_ERROR_MODE);
                        BPMSG1088;
                    } else {
                        //disengaging Clutch
                        //cleans up the protocol and HiZs the pins
                        enabled_protocols[bus_pirate_configuration.bus_mode].cleanup();
                        MSG_CLUTCH_DISENGAGED;
                        
                        BP_VREG_OFF();
                        //bpWmessage(MSG_VREG_OFF);
                        BPMSG1097;
                        bpBR;
                        //modeConfig.vregEN=0;
                    }
                    break;
                case 'd':
                    MSG_ADC_VOLTAGE_PROBE_HEADER;
                    bp_adc_probe();
                    MSG_VOLTAGE_UNIT;
                    bpBR;
                    break;
                case 'D': //bpWline("-DVM mode");	//dumb voltmeter mode
                    bp_adc_continuous_probe();
                    break;
                case '&': //bpWline("-delay 1ms");
                    repeat = getrepeat();
                    //bpWstring(OUMSG_PS_DELAY);
                    BPMSG1099;
                    bp_write_dec_word(repeat);
                    //bpWline(OUMSG_PS_DELAY_US);
                    BPMSG1100;
                    bp_delay_us(repeat);
                    break;
                case '%': repeat = getrepeat();
                    BPMSG1099;
                    bp_write_dec_word(repeat);
                    BPMSG1212;
                    bp_delay_ms(repeat);
                    break;
#ifdef BP_ENABLE_BASIC_SUPPORT
                case 's': //bpWline("Listing:");
                    bus_pirate_configuration.basic = 1;
                    break;
#endif /* BP_ENABLE_BASIC_SUPPORT */
                case 'S': //servo control
                    if (bus_pirate_configuration.bus_mode == BP_HIZ) { //bpWmessage(MSG_ERROR_MODE);
                        BPMSG1088;
                    } else {
                        bp_servo_setup();
                    }
                    break;
                case '<': command_error = true;
                    temp = 1;

                    while (cmdbuf[((cmdstart + temp) & CMDLENMSK)] != 0x00) {
                        if (cmdbuf[((cmdstart + temp) & CMDLENMSK)] == '>') command_error = false; // clear error if we found a > before the command ends
                        temp++;
                    }
                    if (temp >= (BP_USER_MACRO_MAX_LENGTH + 3)) command_error = true; // too long (avoid overflows)
                    if (!command_error) {
                        cmdstart = (cmdstart + 1) & CMDLENMSK;
                        temp = getint();
                        if (cmdbuf[((cmdstart) & CMDLENMSK)] == '=') // assignment
                        {
                            if ((temp > 0) && (temp <= BP_USER_MACROS_COUNT)) {
                                cmdstart = (cmdstart + 1) & CMDLENMSK;
                                temp--;
                                for (repeat = 0; repeat < BP_USER_MACRO_MAX_LENGTH; repeat++) {
                                    user_macros[temp][repeat] = 0;
                                }
                                repeat = 0;
                                while (cmdbuf[cmdstart] != '>') {
                                    user_macros[temp][repeat] = cmdbuf[cmdstart];
                                    repeat++;
                                    cmdstart = (cmdstart + 1) & CMDLENMSK;
                                }
                            } else {
                                command_error = true;
                            }
                        } else {
                            if (temp == 0) {
                                for (repeat = 0; repeat < BP_USER_MACROS_COUNT; repeat++) {
                                    bp_write_dec_byte(repeat + 1);
                                    bp_write_string(". <");
                                    bp_write_string(user_macros[repeat]);
                                    bp_write_line(">");
                                }
                            } else if ((temp > 0) && (temp <= BP_USER_MACROS_COUNT)) { //bpWstring("execute : ");
                                //BPMSG1236;
                                //bpWdec(temp-1);
                                bpBR;
                                user_macro = temp;
                            } else {
                                command_error = true;
                            }
                        }
                    }
                    break;
                    // command for subsys (i2c, UART, etc)
                case '(': //bpWline("-macro");
                    cmdstart = (cmdstart + 1) & CMDLENMSK;
                    sendw = getint();
                    consumewhitechars();
                    if (cmdbuf[((cmdstart) & CMDLENMSK)] == ')') { //cmdstart++;				// skip )
                        //cmdstart&=CMDLENMSK;
                        //bpWdec(sendw);
                        enabled_protocols[bus_pirate_configuration.bus_mode].run_macro(sendw);
                        bpBR;
                    } else {
                        command_error = true;
                    }
                    break;
                case 0x22: //bpWline("-send string");
                    command_error = true;
                    temp = 1;

                    while (cmdbuf[((cmdstart + temp) & CMDLENMSK)] != 0x00) {
                        if (cmdbuf[((cmdstart + temp) & CMDLENMSK)] == 0x22) command_error = false; // clear error if we found a " before the command ends
                        temp++;
                    }

                    if (!command_error) {
                        BPMSG1101;
                        user_serial_transmit_character(0x22);
                        while (cmdbuf[((++cmdstart) & CMDLENMSK)] != 0x22) {
                            cmdstart &= CMDLENMSK;
                            user_serial_transmit_character(cmdbuf[cmdstart]);
                            sendw = cmdbuf[cmdstart];
                            if (mode_configuration.little_endian == YES)
                            {
                                sendw = bp_reverse_integer(sendw, mode_configuration.numbits);
                            }
                            enabled_protocols[bus_pirate_configuration.bus_mode].send(sendw);
                        }
                        cmdstart &= CMDLENMSK;
                        user_serial_transmit_character(0x22);
                        bpBR;
                    }
                    break;
                case '[': //bpWline("-Start");
                    enabled_protocols[bus_pirate_configuration.bus_mode].start();
                    break;
                case '{': //bpWline("-StartR");
                    enabled_protocols[bus_pirate_configuration.bus_mode].start_with_read();
                    break;
                case ']': //bpWline("-Stop");
                    enabled_protocols[bus_pirate_configuration.bus_mode].stop();
                    break;
                case '}': //bpWline("-StopR");
                    enabled_protocols[bus_pirate_configuration.bus_mode].stop_from_read();
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9': //bpWline("-Send");
                    //bpWmessage(MSG_WRITE);
                    BPMSG1101;
                    sendw = getint();
                    cmdstart--;
                    cmdstart &= CMDLENMSK;
                    repeat = getrepeat() + 1;
                    numbits = getnumbits();
                    if (numbits) {
                        mode_configuration.numbits = numbits;
                        if (numbits > 8) mode_configuration.int16 = 1;
                        else mode_configuration.int16 = 0;
                    }
                    while (--repeat) {
                        bp_write_formatted_integer(sendw);
                        if (((mode_configuration.int16 == 0) && (mode_configuration.numbits != 8)) || ((mode_configuration.int16 == 1) && (mode_configuration.numbits != 16))) {
                            user_serial_transmit_character(';');
                            bp_write_dec_byte(mode_configuration.numbits);
                        }
                        if (mode_configuration.little_endian == YES) {
                            sendw = bp_reverse_integer(sendw, mode_configuration.numbits);
                        }
                        received = enabled_protocols[bus_pirate_configuration.bus_mode].send(sendw);
                        bpSP;
                        if (mode_configuration.write_with_read) { //bpWmessage(MSG_READ);
                            BPMSG1102;
                            if (mode_configuration.little_endian == YES) {
                                received = bp_reverse_integer(received, mode_configuration.numbits);
                            }
                            bp_write_formatted_integer(received);
                            bpSP;
                        }
                    }
                    bpBR;
                    break;
                case 'r': //bpWline("-Read");
                    //bpWmessage(MSG_READ);
                    BPMSG1102;
				    //newDmode = 0;
				    newDmode = change_read_display();
                    repeat = getrepeat() + 1;
                    numbits = getnumbits();
                    if (numbits) {
                        mode_configuration.numbits = numbits;
                        if (numbits > 8) mode_configuration.int16 = 1;
                        else mode_configuration.int16 = 0;
                    }
						  if(newDmode)
						  {
						  		oldDmode = bus_pirate_configuration.display_mode;
								bus_pirate_configuration.display_mode = newDmode-1;
						  }
                    while (--repeat) {
                        received = enabled_protocols[bus_pirate_configuration.bus_mode].read();
                        if (mode_configuration.little_endian == YES) {
                            received = bp_reverse_integer(received, mode_configuration.numbits);
                        }
                        bp_write_formatted_integer(received);
                        if (((mode_configuration.int16 == 0) && (mode_configuration.numbits != 8)) || ((mode_configuration.int16 == 1) && (mode_configuration.numbits != 16))) {
                            user_serial_transmit_character(';');
                            bp_write_dec_byte(mode_configuration.numbits);
                        }
                        bpSP;
                    }
						  if(newDmode)
						  {
								bus_pirate_configuration.display_mode = oldDmode;
								newDmode=0;
						  }
                    bpBR;
                    break;
                case '/': //bpWline("-CLK hi");
                    //repeat=getrepeat()+1;
                    //while(--repeat)
                    //{	//bpWmessage(MSG_BIT_CLKH);
                    BPMSG1103;
                    enabled_protocols[bus_pirate_configuration.bus_mode].clock_high();
                    //}
                    break;
                case '\\': //bpWline("-CLK lo");
                    //repeat=getrepeat()+1;
                    //while(--repeat)
                    //{	//bpWmessage(MSG_BIT_CLKL);
                    BPMSG1104;
                    enabled_protocols[bus_pirate_configuration.bus_mode].clock_low();
                    //}
                    break;
                case '-': //bpWline("-DAT hi");
                    //repeat=getrepeat()+1;
                    //while(--repeat)
                    //{	//bpWmessage(MSG_BIT_DATH);
                    BPMSG1105;
                    enabled_protocols[bus_pirate_configuration.bus_mode].data_high();
                    //}
                    break;
                case '_': //bpWline("-DAT lo");
                    //repeat=getrepeat()+1;
                    //while(--repeat)
                    //{	//bpWmessage(MSG_BIT_DATL);
                    BPMSG1106;
                    enabled_protocols[bus_pirate_configuration.bus_mode].data_low();
                    //}
                    break;
                case '.': //bpWline("-DAT state read");
                    //repeat=getrepeat()+1;
                    BPMSG1098;
                    echo_state(enabled_protocols[bus_pirate_configuration.bus_mode].data_state());
                    break;
                case '^': //bpWline("-CLK pulse");
                    repeat = getrepeat();
                    BPMSG1108;
                    bp_write_formatted_integer(repeat);
                    repeat++;
                    while (--repeat) { //bpWmessage(MSG_BIT_CLK);
                        enabled_protocols[bus_pirate_configuration.bus_mode].clock_pulse();
                    }
                    bpBR;
                    break;
                case '!': //bpWline("-bit read");
                    repeat = getrepeat() + 1;
                    BPMSG1109;
                    while (--repeat) {
                        echo_state(enabled_protocols[bus_pirate_configuration.bus_mode].read_bit());
                        bpSP;
                    }
                    //bpWmessage(MSG_BIT_NOWINPUT);
                    BPMSG1107;
                    break;
                    // white char/delimeters
                case 0x00:
                case 0x0D: // not necessary but got random error msg at end, just to be sure
                case 0x0A: // same here
                case ' ':
                case ',': break; // no match so it is an error
                default: command_error = true;
            } //switch(c)
            cmdstart = (cmdstart + 1) & CMDLENMSK;

            if (command_error) { //bpWstring("Syntax error at char ");
                BPMSG1110;
                if (cmdstart > oldstart) // find error position :S
                {
                    bp_write_dec_byte(cmdstart - oldstart);
                } else {
                    bp_write_dec_byte((BP_COMMAND_BUFFER_SIZE + cmdstart) - oldstart);
                }
                command_error = false;
                stop = 1;
                bpBR;
            }

            if (cmdstart == cmdend) stop = 1; // reached end of user input??
        } //while(!stop)


        cmdstart = newstart;
        cmdend = newstart; // 'empty' cmdbuffer
        cmd = 0;
    } //while(1)
} //serviceuser(void)

int getint(void) // get int from user (accept decimal, hex (0x) or binairy (0b)
{
    int i;
    int number;

    i = 0;
    number = 0;

    if ((cmdbuf[((cmdstart + i) & CMDLENMSK)] >= 0x31) && (cmdbuf[((cmdstart + i) & CMDLENMSK)] <= 0x39)) // 1-9 is for sure decimal
    {
        number = cmdbuf[(cmdstart + i) & CMDLENMSK] - 0x30;
        i++;
        while ((cmdbuf[((cmdstart + i) & CMDLENMSK)] >= 0x30) && (cmdbuf[((cmdstart + i) & CMDLENMSK)] <= 0x39)) // consume all digits
        {
            number *= 10;
            number += cmdbuf[((cmdstart + i) & CMDLENMSK)] - 0x30;
            i++;
        }
    } else if (cmdbuf[((cmdstart + i) & CMDLENMSK)] == 0x30) // 0 could be anything
    {
        i++;
        if ((cmdbuf[((cmdstart + i) & CMDLENMSK)] == 'b') || (cmdbuf[((cmdstart + i) & CMDLENMSK)] == 'B')) {
            i++;
            while ((cmdbuf[((cmdstart + i) & CMDLENMSK)] == '1') || (cmdbuf[((cmdstart + i) & CMDLENMSK)] == '0')) {
                number <<= 1;
                number += cmdbuf[((cmdstart + i) & CMDLENMSK)] - 0x30;
                i++;
            }
        } else if ((cmdbuf[((cmdstart + i) & CMDLENMSK)] == 'x') || (cmdbuf[((cmdstart + i) & CMDLENMSK)] == 'X')) {
            i++;
            while (((cmdbuf[((cmdstart + i) & CMDLENMSK)] >= 0x30) && (cmdbuf[((cmdstart + i) & CMDLENMSK)] <= 0x39)) ||    \
				((cmdbuf[((cmdstart + i) & CMDLENMSK)] >= 'A') && (cmdbuf[((cmdstart + i) & CMDLENMSK)] <= 'F')) ||    \
				((cmdbuf[((cmdstart + i) & CMDLENMSK)] >= 'a') && (cmdbuf[((cmdstart + i) & CMDLENMSK)] <= 'f'))) {
                number <<= 4;
                if ((cmdbuf[(cmdstart + i) & CMDLENMSK] >= 0x30) && (cmdbuf[((cmdstart + i) & CMDLENMSK)] <= 0x39)) {
                    number += cmdbuf[((cmdstart + i) & CMDLENMSK)] - 0x30;
                } else {
                    cmdbuf[((cmdstart + i) & CMDLENMSK)] |= 0x20; // make it lowercase
                    number += cmdbuf[((cmdstart + i) & CMDLENMSK)] - 0x57; // 'a'-0x61+0x0a
                }
                i++;
            }
        } else if ((cmdbuf[((cmdstart + i) & CMDLENMSK)] >= 0x30) && (cmdbuf[((cmdstart + i) & CMDLENMSK)] <= 0x39)) {
            number = cmdbuf[((cmdstart + i) & CMDLENMSK)] - 0x30;
            while ((cmdbuf[((cmdstart + i) & CMDLENMSK)] >= 0x30) && (cmdbuf[((cmdstart + i) & CMDLENMSK)] <= 0x39)) // consume all digits
            {
                number *= 10;
                number += cmdbuf[((cmdstart + i) & CMDLENMSK)] - 0x30;
                i++;
            }
        }
    } else // how did we come here??
    {
        command_error = true;
        return 0;
    }

    cmdstart += i; // we used i chars
    cmdstart &= CMDLENMSK;
    return number;
} //getint(void)

int getrepeat(void) {
    int temp;

    if (cmdbuf[(cmdstart + 1) & CMDLENMSK] == ':') {
        cmdstart += 2;
        cmdstart &= CMDLENMSK;
        temp = getint();
        cmdstart--; // to allow [6:10] (start send 6 10 times stop)
        cmdstart &= CMDLENMSK;
        return temp;
    }
    return 1; // no repeat count=1
} //

int getnumbits(void) {
    int temp;

    if (cmdbuf[(cmdstart + 1) & CMDLENMSK] == ';') {
        cmdstart = (cmdstart + 2) & CMDLENMSK;
        temp = getint();
        cmdstart = (cmdstart - 1) & CMDLENMSK; // to allow [6:10] (start send 6 10 times stop)
        return temp;
    }
    return 0; // no numbits=0;
} //

uint8_t change_read_display(void) {
    size_t index;

    for (index = 0; index < sizeof(READ_DISPLAY_BASE); index++) {
        if (cmdbuf[(cmdstart + 1) & CMDLENMSK] == READ_DISPLAY_BASE[index]) {
            return index + 1;
        }
    }

    return 0;
}

void consumewhitechars(void) {
    while (cmdbuf[cmdstart] == 0x20) {
        cmdstart = (cmdstart + 1) & CMDLENMSK; // remove spaces
    }
}

void changemode(void) {
    int i, busmode;

    busmode = 0;
    cmdstart = (cmdstart + 1) & CMDLENMSK;
    consumewhitechars();
    busmode = getint();

    if (!busmode) // no argument entered
    {
        for (i = 0; i < ENABLED_PROTOCOLS_COUNT; i++) {
            bp_write_dec_byte(i + 1);
            bp_write_string(". ");
            bp_write_line(enabled_protocols[i].name);
        }
        //bpWline("x. exit(without change)");
        BPMSG1111;
        command_error = false; // error is set because no number found, but it is no error here:S eeeh confusing right?
        busmode = getnumber(1, 1, ENABLED_PROTOCOLS_COUNT, 1) - 1;
        if ((busmode == -2) || (busmode == -1)) {
            //bpWline("no mode change");
            BPMSG1112;
        } else {
            enabled_protocols[bus_pirate_configuration.bus_mode].cleanup();
            bp_reset_board_state();
            bus_pirate_configuration.bus_mode = busmode;
            enabled_protocols[bus_pirate_configuration.bus_mode].setup_prepare();
            MSG_CLUTCH_DISENGAGED;
            if (busmode) {
               BP_LEDMODE = 1; // mode led is on when proto >0
               MSG_FINISH_SETUP_PROMPT;
            }
            //bpWmessage(MSG_READY);
            BPMSG1085;
        }
    } else // number entered
    {
        busmode--; // save a couple of programwords to do it here :D
        if (busmode < ENABLED_PROTOCOLS_COUNT) {
            enabled_protocols[bus_pirate_configuration.bus_mode].cleanup();
            bp_reset_board_state();
            bus_pirate_configuration.bus_mode = busmode;
            enabled_protocols[bus_pirate_configuration.bus_mode].setup_prepare();
            if (busmode) BP_LEDMODE = 1; // mode led is on when proto >0
            //bpWmessage(MSG_READY);
            BPMSG1085;
        } else { //bpWline("Nonexistent protocol!!");
            BPMSG1114;
        }
    }
    cmdstart = (cmdend - 1) & CMDLENMSK;
}

#ifdef BP_ENABLE_COMMAND_HISTORY

int cmdhistory(void) {
    int i, j, k;

    int historypos[BP_COMMAND_HISTORY_LENGTH];

    i = 1;
    j = (cmdstart - 1) & CMDLENMSK;

    while (j != cmdstart) // scroll through the whole buffer
    {
        if ((cmdbuf[j] == 0x00) && (cmdbuf[(j + 1) & CMDLENMSK] != 0x00)) // did we find an end? is it not empty?
        {
            bp_write_dec_byte(i);
            bp_write_string(". ");
            k = 1;
            while (cmdbuf[((j + k) & CMDLENMSK)]) {
                user_serial_transmit_character(cmdbuf[((j + k) & CMDLENMSK)]); // print it
                k++;
            }
            historypos[i] = (j + 1) & CMDLENMSK;
            i++;
            if (i == BP_COMMAND_HISTORY_LENGTH) break;
            bpBR;
        }
        j = (j - 1) & CMDLENMSK;
    }

    bpBR;
    BPMSG1115;

    j = getnumber(0, 1, i, 1);

    if (j == -1 || !j) // x is -1, default is 0
    {
        bpBR;
        return 1;
    }
    
        i = 0;
        while (cmdbuf[(historypos[j] + i) & CMDLENMSK]) // copy it to the end of the ringbuffer
        {
            cmdbuf[(cmdend + i) & CMDLENMSK] = cmdbuf[(historypos[j] + i) & CMDLENMSK];
            i++;
        }
        cmdstart = (cmdend - 1) & CMDLENMSK; // start will be increased before parsing in main loop
        cmdend = (cmdstart + i + 2) & CMDLENMSK;
        cmdbuf[(cmdend - 1) & CMDLENMSK] = 0x00;

    return 0;
}

#endif /* BP_ENABLE_COMMAND_HISTORY */

// gets number from input
// -1 = abort (x)
// -2 = input to much
// 0-max return
// x=1 exit is enabled (we don't want that in the mode changes ;)

int getnumber(int def, int min, int max, int x) //default, minimum, maximum, show exit option
{
    char c;
    char buf[6]; // max 4 digits;
    int i, j, stop, temp, neg;

again: // need to do it proper with whiles and ifs..

    i = 0;
    stop = 0;
    temp = 0;
    neg = 0;

    bp_write_string("\r\n(");
    if (def < 0) {
        bp_write_string("x");
    } else {
        bp_write_dec_byte(def);
    }
    bp_write_string(")>");

    while (!stop) {
        c = user_serial_read_byte();
        switch (c) {
            case 0x08: if (i) {
                    i--;
                    bp_write_string("\x08 \x08");
                } else {
                    if (neg) {
                        neg = 0;
                        bp_write_string("\x08 \x08");
                    } else {
                        user_serial_transmit_character(BELL);
                    }
                }
                break;
            case 0x0A:
            case 0x0D: stop = 1;
                break;
            case '-': if (!i) // enable negative numbers
                {
                    user_serial_transmit_character('-');
                    neg = 1;
                } else {
                    user_serial_transmit_character(BELL);
                }
                break;
            case 'x': if (x) return -1; // user wants to quit :( only if we enable it :D
            default: if ((c >= 0x30) && (c <= 0x39)) // we got a digit
                {
                    if (i > 3) // 0-9999 should be enough??
                    {
                        user_serial_transmit_character(BELL);
                        i = 4;
                    } else {
                        user_serial_transmit_character(c);
                        buf[i] = c; // store user input
                        i++;
                    }
                } else // ignore input :)
                {
                    user_serial_transmit_character(BELL);
                }

        }
    }
    bpBR;

    if (i == 0) {
        return def; // no user input so return default option
    } else {
        temp = 0;
        i--;
        j = i;

        for (; i >= 0; i--) {
            temp *= 10;
            temp += (buf[j - i] - 0x30);
        }

        if ((temp >= min) && (temp <= max)) {
            if (neg) {
                return -temp;
            } else {
                return temp;
            }
        } else { //bpWline("\r\nInvalid choice, try again\r\n");
            BPMSG1211;
            goto again;
        }
    }
    return temp; // we dont get here, but keep compiler happy
}

#ifdef BUSPIRATEV4
// gets number from input
// -1 = abort (x)
// -2 = input to much
// 0-max return
// x=1 exit is enabled (we don't want that in the mode changes ;)

long getlong(long def, int min, long max, int x) //default, minimum, maximum, show exit option
{
    char c;
    char buf[12]; // max long = 2147483647 so 10
    int i, j, stop, neg;
    long temp;

again: // need to do it proper with whiles and ifs..

    i = 0;
    stop = 0;
    temp = 0;
    neg = 0;

    bp_write_string("\r\n(");
    if (def < 0) {
        bp_write_string("x");
    } else {
        bp_write_dec_dword(def);
    }
    bp_write_string(")>");

    while (!stop) {
        c = user_serial_read_byte();
        switch (c) {
            case 0x08: if (i) {
                    i--;
                    bp_write_string("\x08 \x08");
                } else {
                    if (neg) {
                        neg = 0;
                        bp_write_string("\x08 \x08");
                    } else {
                        user_serial_transmit_character(BELL);
                    }
                }
                break;
            case 0x0A:
            case 0x0D: stop = 1;
                break;
            case '-': if (!i) // enable negative numbers
                {
                    user_serial_transmit_character('-');
                    neg = 1;
                } else {
                    user_serial_transmit_character(BELL);
                }
                break;
            case 'x': if (x) return -1; // user wants to quit :( only if we enable it :D
            default: if ((c >= 0x30) && (c <= 0x39)) // we got a digit
                {
                    if (i > 9) // 0-9999 should be enough??
                    {
                        user_serial_transmit_character(BELL);
                        i = 10;
                    } else {
                        user_serial_transmit_character(c);
                        buf[i] = c; // store user input
                        i++;
                    }
                } else // ignore input :)
                {
                    user_serial_transmit_character(BELL);
                }

        }
    }
    bpBR;

    if (i == 0) {
        return def; // no user input so return default option
    } else {
        temp = 0;
        i--;
        j = i;

        for (; i >= 0; i--) {
            temp *= 10;
            temp += (buf[j - i] - 0x30);
        }

        if ((temp >= min) && (temp <= max)) {
            if (neg) {
                return -temp;
            } else {
                return temp;
            }
        } else { //bpWline("\r\nInvalid choice, try again\r\n");
            BPMSG1211;
            goto again;
        }
    }
    return temp; // we dont get here, but keep compiler happy
}

#endif /* BUSPIRATEV4 */

//print version info (used in menu and at startup in main.c)

void print_version_info(void) {
    unsigned int i;

#ifdef BUSPIRATEV3 //we can tell if it's v3a or v3b, show it here
    bp_write_string(BP_VERSION_STRING);
    user_serial_transmit_character('.');
    user_serial_transmit_character(bus_pirate_configuration.hardware_version);
    if (bus_pirate_configuration.device_type == 0x44F) {
        //sandbox electronics clone with 44pin PIC24FJ64GA004
        MSG_CHIP_IDENTIFIER_CLONE;
    }
    bpBR;
#else
    bp_write_line(BP_VERSION_STRING);
#endif /* BUSPIRATEV3 */

    bp_write_string(BP_FIRMWARE_STRING);

    user_serial_transmit_character('[');
    for (i = 0; i < ENABLED_PROTOCOLS_COUNT; i++) {
        if (i) bpSP;
        bp_write_string(enabled_protocols[i].name);
    }
    user_serial_transmit_character(']');

#ifndef BUSPIRATEV4
    //bpWstring(" Bootloader v");
    BPMSG1126;
    i = bp_read_from_flash(0x0000, BL_ADDR_VER);
    bp_write_dec_byte(i >> 8);
    user_serial_transmit_character('.');
    bp_write_dec_byte(i);
#endif /* !BUSPIRATEV4 */
    bpBR;

    //bpWstring("DEVID:");
    BPMSG1117;
    bp_write_hex_word(bus_pirate_configuration.device_type);

    //bpWstring(" REVID:");
    BPMSG1210;
    bp_write_hex_word(bus_pirate_configuration.device_revision);
#ifdef BUSPIRATEV4
    MSG_CHIP_REVISION_ID_BEGIN;
    switch (bus_pirate_configuration.device_revision) {
        case PIC_REV_A3:
            MSG_CHIP_REVISION_A3;
            break;
        case PIC_REV_A5:
            MSG_CHIP_REVISION_A5;
            break;
        default:
            MSG_CHIP_REVISION_UNKNOWN;
            break;
    }
#else
    MSG_CHIP_REVISION_ID_BEGIN;
    if (bus_pirate_configuration.device_type == 0x44F) {
        //sandbox electronics clone with 44pin PIC24FJ64GA004
        MSG_CHIP_REVISION_ID_END_4;
    } else {
        MSG_CHIP_REVISION_ID_END_2;
    }

    switch (bus_pirate_configuration.device_revision) {
        case PIC_REV_A3:
            MSG_CHIP_REVISION_A3;
            break;
        case PIC_REV_B4:
            MSG_CHIP_REVISION_B4;
            break;
        case PIC_REV_B5:
            MSG_CHIP_REVISION_B5;
            break;
        case PIC_REV_B8:
            MSG_CHIP_REVISION_B8;
            break;
        default:
            MSG_CHIP_REVISION_UNKNOWN;
            break;
    }
#endif /* BUSPIRATEV4 */

    bp_write_line(")");
    //bpWline("http://dangerousprototypes.com");
    BPMSG1118;
    i = 0;
} //versionInfo(void)

//display properties of the current bus mode (pullups, vreg, lsb, output type, etc)

void statusInfo(void) {
#ifdef BUSPIRATEV4
    MSG_CFG0_FIELD;
    bp_write_hex_word(bp_read_from_flash(CFG_ADDR_UPPER, CFG_ADDR_0));
    bpSP;
#endif /* BUSPIRATEV4 */

    BPMSG1136;
    bp_write_hex_word(bp_read_from_flash(CFG_ADDR_UPPER, CFG_ADDR_1));
    BPMSG1137;
    bp_write_hex_word(bp_read_from_flash(CFG_ADDR_UPPER, CFG_ADDR_2));
    bpBR;

    BPMSG1119;

    print_pins_information();

    //vreg status (was modeConfig.vregEN)
    if (BP_VREGEN == 1) BPMSG1096;
    else BPMSG1097; //bpWmessage(MSG_VREG_ON); else bpWmessage(MSG_VREG_OFF);
    user_serial_transmit_character(',');
    bpSP;

    //pullups available, enabled?
    //was modeConfig.pullupEN
    if (BP_PULLUP == 1) BPMSG1091;
    else BPMSG1089; //bpWmessage(MSG_OPT_PULLUP_ON); else bpWmessage(MSG_OPT_PULLUP_OFF);
    user_serial_transmit_character(',');
    bpSP;

#ifdef BUSPIRATEV4
    if (BP_PUVSEL50_DIR == 0) {
        MSG_VPU_5V_MARKER;
    }
    if (BP_PUVSEL33_DIR == 0) {
        MSG_VPU_3V3_MARKER;
    }
#endif /* BUSPIRATEV4 */

    //open collector outputs?
    if (mode_configuration.high_impedance == 1) BPMSG1120;
    else BPMSG1121; // bpWmessage(MSG_STATUS_OUTPUT_HIZ); else bpWmessage(MSG_STATUS_OUTPUT_NORMAL);

    //bitorder toggle available, enabled
    if (mode_configuration.little_endian == NO) BPMSG1123;
    else BPMSG1124; //bpWmessage(MSG_OPT_BITORDER_LSB); else bpWmessage(MSG_OPT_BITORDER_MSB);
    user_serial_transmit_character(',');
    bpSP;

    // show partial writes
    //bpWline("Number of bits read/write: ");
    BPMSG1252;
    bp_write_dec_byte(mode_configuration.numbits);
    bpBR;

    //AUX pin setting
#ifndef BUSPIRATEV4
    if (mode_configuration.alternate_aux == 1) BPMSG1087;
    else BPMSG1086; //bpWmessage(MSG_OPT_AUXPIN_CS); else bpWmessage(MSG_OPT_AUXPIN_AUX);
#endif /* !BUSPIRATEV4 */
    
#ifdef BUSPIRATEV4
    switch (mode_configuration.alternate_aux) {
        case 0: BPMSG1087;
            break;
        case 1: BPMSG1086;
            break;
        case 2: BPMSG1263;
            break;
        case 3: BPMSG1264;
            break;
    }
#endif /* BUSPIRATEV4 */

    enabled_protocols[bus_pirate_configuration.bus_mode].print_settings();

    //bpWline("*----------*");
    BPMSG1119;
} //statusInfo(void)

void print_pins_information(void) { 
    //bpWline("Pinstates:");
    BPMSG1226;
#ifdef BUSPIRATEV4
    BPMSG1256; //bpWstring("12.(RD)\t11.(BR)\t10.(BLK)\t9.(WT)\t8.(GR)\t7.(PU)\t6.(BL)\t5.(GN)\t4.(YW)\t3.(OR)\t2.(RD)\1.(BR)");
    BPMSG1257; //bpWstring("GND\t5.0V\t3.3V\tVPU\tADC\tAUX2\tAUX1\tAUX\t");
#else
    BPMSG1233; //bpWstring("1.(BR)\t2.(RD)\t3.(OR)\t4.(YW)\t5.(GN)\t6.(BL)\t7.(PU)\t8.(GR)\t9.(WT)\t0.(BLK)");
    BPMSG1227; //bpWstring("GND\t3.3V\t5.0V\tADC\tVPU\tAUX\t");
#endif /* BUSPIRATEV4 */

    enabled_protocols[bus_pirate_configuration.bus_mode].print_pins_state();
    BPMSG1228; //bpWstring("P\tP\tP\tI\tI\t");
#ifdef BUSPIRATEV4
    print_pin_direction(AUX2);
    print_pin_direction(AUX1);
    print_pin_direction(AUX);
    print_pin_direction(CS);
    print_pin_direction(MISO);
    print_pin_direction(CLK);
    print_pin_direction(MOSI);
#else    
    print_pin_direction(AUX);
    print_pin_direction(CLK);
    print_pin_direction(MOSI);
    print_pin_direction(CS);
    print_pin_direction(MISO);
#endif /* BUSPIRATEV4 */
    bpBR;
    BPMSG1234; //bpWstring("GND\t");
    ADCON();

#ifdef BUSPIRATEV4
    bp_write_voltage(bp_read_adc(BP_ADC_5V0));
#else
#if defined(BP_VERSION2_SUPPORT) && (BP_VERSION2_SUPPORT == 1)
    bp_write_voltage(bp_read_adc(BP_ADC_PROBE));
#else
    bp_write_voltage(bp_read_adc(BP_ADC_3V3));
#endif /* BP_VERSION2_SUPPORT && (BP_VERSION2_SUPPORT == 1) */
#endif /* BUSPIRATEV4 */
    MSG_VOLTAGE_UNIT;
    user_serial_transmit_character('\t');

#ifdef BUSPIRATEV4
    bp_write_voltage(bp_read_adc(BP_ADC_3V3));
#else
    bp_write_voltage(bp_read_adc(BP_ADC_5V0));
#endif /* BUSPIRATEV4 */
    MSG_VOLTAGE_UNIT;
    user_serial_transmit_character('\t');

#ifdef BUSPIRATEV4
    bp_write_voltage(bp_read_adc(BP_ADC_VPU));
#else
#if defined(BP_VERSION2_SUPPORT) && (BP_VERSION2_SUPPORT == 1)
    bp_write_voltage(bp_read_adc(BP_ADC_3V3));
#else
    bp_write_voltage(bp_read_adc(BP_ADC_PROBE));
#endif /* BP_VERSION2_SUPPORT && (BP_VERSION2_SUPPORT == 1) */
#endif /* BUSPIRATEV4 */
    MSG_VOLTAGE_UNIT;
    user_serial_transmit_character('\t');

#ifdef BUSPIRATEV4
    bp_write_voltage(bp_read_adc(BP_ADC_PROBE));
#else
    bp_write_voltage(bp_read_adc(BP_ADC_VPU));
#endif /* BUSPIRATEV4 */
    MSG_VOLTAGE_UNIT;
    user_serial_transmit_character('\t');
    
    ADCOFF();
    
#ifdef BUSPIRATEV4
    print_pin_state(AUX2);
    print_pin_state(AUX1);
    print_pin_state(AUX);
    print_pin_state(CS);
    print_pin_state(MISO);
    print_pin_state(CLK);
    print_pin_state(MOSI);
#else
    print_pin_state(AUX);
    print_pin_state(CLK);
    print_pin_state(MOSI);
    print_pin_state(CS);
    print_pin_state(MISO);
#endif /* BUSPIRATEV4 */
    bpBR;
}

void print_pin_direction(uint16_t pin) {
    bp_write_string(IODIR & pin ? "I\t" : "O\t");
}

void print_pin_state(uint16_t pin) {
    bp_write_string(IOPOR & pin ? "H\t" : "L\t");
}

//user terminal number display mode dialog (eg HEX, DEC, BIN, RAW)

void setDisplayMode(void) {
    int mode;

    cmdstart = (cmdstart + 1) & CMDLENMSK;

    consumewhitechars();
    mode = getint();

    if ((mode > 0) && (mode <= 4)) {
        bus_pirate_configuration.display_mode = mode - 1;
    } else {
        command_error = false;
        //bpWmessage(MSG_OPT_DISPLAYMODE); //show the display mode options message
        BPMSG1127;
        //	bpConfig.displayMode=(bpUserNumberPrompt(1, 4, 1)-1); //get, store user reply
        bus_pirate_configuration.display_mode = getnumber(1, 1, 4, 0) - 1; //get, store user reply
    }
    //bpWmessage(MSG_OPT_DISPLAYMODESET);//show display mode update text
    BPMSG1128;
} //

//configure user terminal side UART baud rate

void set_baud_rate(void) {
    unsigned char speed;
    unsigned char brg = 0;

    cmdstart = (cmdstart + 1) & CMDLENMSK;

    consumewhitechars();
    speed = getint();

    if ((speed > 0) && (speed <= 10)) {
        bus_pirate_configuration.terminal_speed = speed - 1;
    } else {
        command_error = false;
        //bpWmessage(MSG_OPT_UART_BAUD); //show stored dialog
        BPMSG1133;
        //	bpConfig.termSpeed=(bpUserNumberPrompt(1, 9, 9)-1);
        bus_pirate_configuration.terminal_speed = getnumber(9, 1, 10, 0) - 1;
    }

    if (bus_pirate_configuration.terminal_speed == 9) {
        consumewhitechars();
        brg = getint();

        if (brg == 0) {
            command_error = false;
            MSG_RAW_BRG_VALUE_INPUT;
            brg = getnumber(34, 0, 32767, 0);
        }
    }

    //bpWmessage(MSG_OPT_TERMBAUD_ADJUST); //show 'adjust and press space dialog'
    BPMSG1134;
    BPMSG1251;
    user_serial_wait_transmission_done(); //wait for TX to finish or reinit flushes part of prompt string from buffer

    if (bus_pirate_configuration.terminal_speed == 9) {
        user_serial_set_baud_rate(brg);
    }
    user_serial_initialise();
    
    //wait for space to prove valid baud rate switch
    for (;;) {
        if (user_serial_read_byte() == ' ') {
            break;
        }
    }
}

void echo_state(const uint16_t value) {
    user_serial_transmit_character(value ? '1' : '0');
}

#ifdef BUSPIRATEV4

void setPullupVoltage(void) {
    int temp;

    //don't allow pullups on some modules. also: V0a limitation of 2 resistors
    if (bus_pirate_configuration.bus_mode == BP_HIZ) { //bpWmessage(MSG_ERROR_MODE);
        BPMSG1088;
        command_error = true; // raise error
        return;
    }
    if (mode_configuration.high_impedance == 0) { //bpWmessage(MSG_ERROR_NOTHIZPIN);
        BPMSG1209;
        command_error = true; // raise error
        return;
    }

    BP_3V3PU_OFF(); //disable any existing pullup
    bp_delay_ms(2);
    ADCON();
    if (bp_read_adc(BP_ADC_VPU) > 0x100) { //is there already an external voltage?
        /* Shouldn't this be an error? */
        MSG_VOLTAGE_VPULLUP_ALREADY_PRESENT;
    }
    ADCOFF();

    cmdstart = (cmdstart + 1) & CMDLENMSK;
    consumewhitechars();

    temp = getint();
    if (command_error) // I think the user wants a menu
    {
        command_error = false;

        BPMSG1271;

        temp = getnumber(1, 1, 3, 0);
    }
    switch (temp) {
        case 1: BP_3V3PU_OFF();

            BPMSG1272; //;0;" on-board pullup voltage "
            BPMSG1274; //1;"disabled"

            //bpWline("on-board pullup voltage disabled");
            break;
        case 2: BP_3V3PU_ON();
            BPMSG1173; //3.3v
            BPMSG1272; //;0;" on-board pullup voltage "
            BPMSG1273; //1;"enabled"
            //bpWline("3V3 on-board pullup voltage enabled");
            break;
        case 3: BP_5VPU_ON();
            BPMSG1171; //5v
            BPMSG1272; //;0;" on-board pullup voltage "
            BPMSG1273; //1;"enabled"
            //bpWline("5V on-board pullup voltage enabled");
            break;
        default:BP_3V3PU_OFF();
            BPMSG1272; //;0;" on-board pullup voltage "
            BPMSG1274; //1;"disabled"
            //bpWline("on-board pullup voltage disabled");
    }
}

#endif /* BUSPIRATEV4 */
