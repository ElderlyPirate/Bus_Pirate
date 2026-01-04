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

#include "hd44780.h"

#ifdef BP_ENABLE_HD44780_SUPPORT

#include "spi.h"
#include "bitbang.h"
#include "base.h"
#include "proc_menu.h"

//Define how the HCT595 pins connect to the LCD
#define HCT595_LCD_LED  0b00000001   // P0
#define HCT595_LCD_RS   0b00000010   // P1
#define HCT595_LCD_RW   0b00000100   // P2
#define HCT595_LCD_EN   0b00001000   // P3
#define HCT595_LCD_D4   0b00010000   // P4
#define HCT595_LCD_D5   0b00100000   // P5
#define HCT595_LCD_D6   0b01000000   // P6
#define HCT595_LCD_D7   0b10000000   // P7

//Define how the PCF8574 pins connect to the LCD
#define PCF8574_LCD_RS   0b00000001   // P0
#define PCF8574_LCD_RW   0b00000010   // P1
#define PCF8574_LCD_EN   0b00000100   // P2
#define PCF8574_LCD_LED  0b00001000   // P3
#define PCF8574_LCD_D4   0b00010000   // P4
#define PCF8574_LCD_D5   0b00100000   // P5
#define PCF8574_LCD_D6   0b01000000   // P6
#define PCF8574_LCD_D7   0b10000000   // P7

#define PCF8574_DEFAULT_ADDRESS 0x27

//RS (register select) pin states
#define HD44780_COMMAND 0 
#define HD44780_DATA 1 

//
//HD44780 commands and related options
//
#define CMD_CLEARDISPLAY        0b00000001 //82us-1.64ms

#define CMD_RETURNHOME          0b00000010 //40us-1.64ms

#define CMD_ENTRYMODESET        0b00000100 //40us
#define INCREMENT 0b10
#define DECREMENT 0b00
#define DISPLAYSHIFTON 0b1
#define DISPLAYSHIFTOFF 0

#define CMD_DISPLAYCONTROL      0b00001000 //40us
#define DISPLAYON 0b100
#define DISPLAYOFF 0
#define CURSERON 0b10
#define CURSEROFF 0
#define BLINKON 0b1
#define BLINKOFF 0

#define CMD_CURSERDISPLAYSHIFT 0b00010000 //40us
#define DISPLAYSHIFT 0b1000
#define CURSERMOVE 0
#define SHIFTRIGHT 0b100
#define SHIFTLEFT 0

#define CMD_FUNCTIONSET         0b00100000 //40us
#define DATAWIDTH8 0b10000
#define DATAWIDTH4 0
#define DISPLAYLINES2 0b1000
#define DISPLAYLINES1 0
#define FONT5X10 0b100
#define FONT5X7 0
#define MODULE24X4 0b1

#define CMD_SETCGRAMADDR        0b01000000 //40us
//6bit character generator RAM address

#define CMD_SETDDRAMADDR        0b10000000 //40us
//7bit display data RAM address

#define ADAPTER_SPI 0
#define ADAPTER_I2C 1

#define SCL             BP_CLK
#define SCL_TRIS        BP_CLK_DIR
#define SDA             BP_MOSI
#define SDA_TRIS        BP_MOSI_DIR

#define SPIMOSI_TRIS    BP_MOSI_DIR
#define SPICLK_TRIS     BP_CLK_DIR
#define SPIMISO_TRIS    BP_MISO_DIR
#define SPICS_TRIS      BP_CS_DIR
#define SPIMOSI         BP_MOSI
#define SPICLK          BP_CLK
#define SPIMISO         BP_MISO
#define SPICS           BP_CS

extern mode_configuration_t mode_configuration;
extern command_t last_command;

struct _HD44780_interface {
    unsigned char EN:1; //clock, active high
    unsigned char RS:1; //register select, 0=command, 1=text
    unsigned char RW:1; //read write, 0=write, 1=read
    unsigned char LED:1;
    unsigned char adapter_type:1;   //0=SPI (74HC595), 1=I2C (PCF8574)
    unsigned char i2c_address;      //PCF8574 write address (7-bit << 1)
    /* Abstract pinout across adapters */
    uint8_t EN_mask;
    uint8_t RS_mask;
    uint8_t RW_mask;
    uint8_t LED_mask;
} HD44780;

/* Private helpers */
static void HD44780_Reset(void); //reset the LCD to 4 bit mode
static void HD44780_Init(unsigned char displaylines); //initialize LCD to 4bit mode with typical settings and X displaylines
static void HD44780_WriteByte(unsigned char reg, unsigned char dat); //write a byte to LCD to register REG
static void HD44780_WriteNibble(unsigned char reg, unsigned char dat); //write 4 bits to LCD to register REG
static void HD44780_Write(unsigned char datout); //abstracts data output to 74HC595 or PCF8574 backpacks
static void HD44780_Test(unsigned char first_ch, unsigned char last_ch, int count);

/* 
 * Duplicate the minimum amount of SPI functionality if SPI support is disabled.
 */

#ifndef BP_ENABLE_SPI_SUPPORT

//open drain control registers for OUTPUT pins
#define SPIMOSI_ODC             BP_MISO_ODC     
#define SPICLK_ODC              BP_CLK_ODC      
#define SPICS_ODC               BP_CS_ODC       

unsigned char spi_write_byte(unsigned char c) {
    SPI1BUF = c;
    while(!IFS0bits.SPI1IF);
    c=SPI1BUF;
    IFS0bits.SPI1IF = 0;
    return c;
}

void spi_disable_interface(void) {
    SPI1STATbits.SPIEN = 0;
    RPINR20bits.SDI1R=0b11111;  //B7 MISO
    
    //PPS Disable
    BP_MOSI_RPOUT=0;
    BP_CLK_RPOUT=0;
    
    //disable all open drain control register bits
    SPIMOSI_ODC=0;
    SPICLK_ODC=0;
    SPICS_ODC=0;
    //make all input maybe???
}

#endif /* !BP_ENABLE_SPI_SUPPORT */

unsigned int LCDwrite(unsigned int c) {
    HD44780_WriteByte(HD44780.RS, c);
    return 0x100;
}

void LCDstart(void) {
    HD44780.RS = HD44780_COMMAND;
    BPMSG1213; // Command mode
}

void LCDstop(void) {
    HD44780.RS = HD44780_DATA;
    BPMSG1214; // Data mode
}

void LCDsetup(void) {
    int choice;
    mode_configuration.high_impedance = ON;
    mode_configuration.command_error = NO;

    HD44780.RS = HD44780_DATA;
    HD44780.adapter_type = ADAPTER_I2C;          // default
    HD44780.i2c_address = PCF8574_DEFAULT_ADDRESS << 1;

    // BPMSG1225: "Adapter type:\r\n 1. SPI (74HC595)\r\n 2. I2C (PCF8574)"
    BPMSG1225;
    choice = getnumber(2, 1, 2, 0); //Default to I2C adapter
    HD44780.adapter_type = (choice == 2) ? ADAPTER_I2C : ADAPTER_SPI;
    
    if (HD44780.adapter_type == ADAPTER_I2C) {
        BPMSG1215; // "I2C address (default 0x27):"
        HD44780.i2c_address = getnumber(PCF8574_DEFAULT_ADDRESS, 0, 255, 0) << 1;
        
        HD44780.EN_mask = PCF8574_LCD_EN;
        HD44780.RS_mask = PCF8574_LCD_RS;
        HD44780.RW_mask = PCF8574_LCD_RW;
        HD44780.LED_mask = PCF8574_LCD_LED;

        SCL_TRIS = INPUT;               //SCL Direction Register Bit
        SDA_TRIS = INPUT;               //SDA Direction Register Bit

        SCL = LOW;                      //B8 SCL
        SDA = LOW;                      //B9 SDA

        bitbang_setup(2, BITBANG_SPEED_100KHZ); //2wire mode, 100kHz (PCF8574 max)
    } else {
        HD44780.EN_mask = HCT595_LCD_EN;
        HD44780.RS_mask = HCT595_LCD_RS;
        HD44780.RW_mask = HCT595_LCD_RW;
        HD44780.LED_mask = HCT595_LCD_LED;

		//PPS Setup
		// Inputs
		RPINR20bits.SDI1R = BP_MISO_RPIN; //MISO
		// Outputs
		BP_MOSI_RPOUT = SDO1_IO;          //B9 MOSI
		BP_CLK_RPOUT = SCK1OUT_IO;        //B8 CLK

        SPICS = LOW;                      //B6 CS low
        SPICS_TRIS = OUTPUT;              //B6 CS output

        //pps configures pins and this doesn't really matter....
        SPICLK_TRIS = OUTPUT;             //B8 SCK output
        SPIMISO_TRIS = INPUT;             //B7 SDI input
        SPIMOSI_TRIS = OUTPUT;            //B9 SDO output

        /* CKE=1, CKP=0, SMP=0 */
        SPI1CON1 = 0b0100111101; //(SPIspeed[modeConfig.speed]); // CKE (output edge) active to idle, CKP idle low, SMP data sampled middle of output time.
        //SPI1CON1=0b11101;
        //SPI1CON1bits.MSTEN=1;
        //SPI1CON1bits.CKP=0;
        //SPI1CON1bits.CKE=1;           
        //SPI1CON1bits.SMP=0;

/*
 * Duplicated spi_write_byte works with enhanced mode off while spi_write_byte from spi.c works with enhanced mode on
 */
#ifndef BP_ENABLE_SPI_SUPPORT 
        SPI1CON2 = 0x0000; // Enhanced mode off
#else
        SPI1CON2 = 0x0001; // Enhanced mode on
#endif /* !BP_ENABLE_SPI_SUPPORT */

        SPI1STAT = 0x0000;    // clear SPI
        SPI1STATbits.SPIEN = ON;
    }
    BPMSG1216; // Adapter ready message
}

void LCDsetup_exc(void) {
}

void LCDmacro(unsigned int c) {
    int input;

    consumewhitechars();
    if (cmdbuf[cmdstart] == ')') {
        cmdstart = (cmdstart + 1) & CMDLENMSK;
    }

    consumewhitechars();
    input = getint();

    mode_configuration.command_error = NO;
    
    switch (c) {
        case 0:
            BPMSG1219; // macro menu
            break;
        case 1: // reset only
            BPMSG1093;
            HD44780_Reset();
            break;
        case 2:
            BPMSG1093;
            HD44780_Reset();

            if (!((input >= 1) && (input <= 2))) {
                BPMSG1220;
                input = getnumber(2, 1, 2, 0);
            }
            HD44780_Init((input == 1) ? DISPLAYLINES1 : DISPLAYLINES2);
            BPMSG1221;
            break;
        case 3:
            HD44780_WriteByte(HD44780_COMMAND, CMD_CLEARDISPLAY);
            bp_delay_ms(15);
            BPMSG1222;
            break;
        case 4:
            HD44780_WriteByte(HD44780_COMMAND, CMD_SETDDRAMADDR | (unsigned char)input);
            BPMSG1223;
            break;
        case 6: // numbers test
            HD44780_Test(0x30, 0x39, input); //0 to 9
            break;
        case 7: // characters test
            HD44780_Test(0x21, 0x7E, input); //! to ~
            break;
        default:
            MSG_UNKNOWN_MACRO_ERROR;
    }
}

void LCDpins(void) {
    if (HD44780.adapter_type == ADAPTER_I2C) {
        MSG_I2C_PINS_STATE;
    } else {
        // BPMSG1226: "-\tCLK\t-\tMISO\t-\tMOSI\t-\tCS"
        BPMSG1226;
    }
}

//initialize LCD to 4bits with standard features
//displaylines=0 for single line displays, displaylines=1 for multiline displays
void HD44780_Init(unsigned char displaylines) {
    //Function set
    HD44780_WriteByte(HD44780_COMMAND, (CMD_FUNCTIONSET + DATAWIDTH4 + FONT5X7 + displaylines));//0x28, 0b101000
    bp_delay_ms(15);//delay 15ms
    
    //Turn display off
    HD44780_WriteByte(HD44780_COMMAND, CMD_DISPLAYCONTROL + DISPLAYOFF + CURSEROFF + BLINKOFF);//0x08, 0b1000
    bp_delay_ms(15);//delay 15ms
    
    //Clear LCD and return home
    HD44780_WriteByte(HD44780_COMMAND, CMD_CLEARDISPLAY);
    bp_delay_ms(15);//delay 15ms
    
    //Turn on display, turn off cursor and blink
    HD44780_WriteByte(HD44780_COMMAND, CMD_DISPLAYCONTROL + DISPLAYON + CURSERON + BLINKOFF);// 0x0f, 0b1111
    bp_delay_ms(15);//delay 15ms
}

//reset LCD to 4bit mode
void HD44780_Reset(void) {
    HD44780_Write(0);//clear IO pins to HD44780

    bp_delay_ms(15);
    //# Write 0x03 to LCD and wait 5 msecs for the instruction to complete
    HD44780_WriteNibble(HD44780_COMMAND, 0x03);
    bp_delay_ms(5);

    //# Write 0x03 to LCD and wait 160 usecs for instruction to complete
    HD44780_WriteNibble(HD44780_COMMAND, 0x03);
    bp_delay_us(160);

    //# Write 0x03 AGAIN to LCD and wait 160 usecs (or poll the Busy Flag)
    HD44780_WriteNibble(HD44780_COMMAND, 0x03);
    bp_delay_us(160);

    //Set the Operating Characteristics of the LCD
    //* Write 0x02 to the LCD to Enable Four Bit Mode
    HD44780_WriteNibble(HD44780_COMMAND, 0x02);
    bp_delay_us(160);
}

void HD44780_WriteByte(unsigned char reg, unsigned char dat) {
    HD44780_WriteNibble(reg, (dat>>4));
    HD44780_WriteNibble(reg, (dat & 0x0F));
}

void HD44780_WriteNibble(unsigned char reg, unsigned char dat) {
    //EN pin should already be low
    //RW bit should be 0 (already 0 in dat)
    //LED bit should be 0 (already 0 in dat)
    
    dat = dat << 4; //Nibble to upper bits to match adapter pinout

    if (reg == HD44780_DATA) { dat |= HD44780.RS_mask; }
    dat |= HD44780.LED_mask; //keep LED on

    HD44780_Write(dat);  // Setup: EN low, data/RS ready

    dat |= HD44780.EN_mask;
    HD44780_Write(dat);  // EN high (execute)
    
    dat &= ~HD44780.EN_mask;
    HD44780_Write(dat);  // EN low (end)
}

/* Low-level transport abstraction */
static void HD44780_Write(unsigned char datout) {
    if (HD44780.adapter_type == ADAPTER_I2C) {
        bitbang_i2c_start(BITBANG_I2C_START_ONE_SHOT);
        bitbang_write_value(HD44780.i2c_address);
        if (bitbang_read_bit() == 1) { MSG_NACK; return; }
        bitbang_write_value(datout);
        if (bitbang_read_bit() == 1) { MSG_NACK; return; }
        bitbang_i2c_stop();
    } else {
        spi_write_byte(datout);
        SPICS = HIGH;   //B6 CS high
        bp_delay_us(255); //because was getting garbled screen with test macros
        SPICS = LOW;    //B6 CS low
    }
}

static void HD44780_Test(unsigned char first_ch, unsigned char last_ch, int count) {
    unsigned char ch;
    int i;

    HD44780_WriteByte(HD44780_COMMAND, CMD_CLEARDISPLAY);
    bp_delay_ms(15);

    if (count == 0) count = 80;  // Default

    ch = first_ch;
    for (i = 0; i < count; i++) {
        if (ch > last_ch) ch = first_ch;
        HD44780_WriteByte(HD44780_DATA, ch);
        user_serial_transmit_character(ch);
        ch++;
    }
}

#endif /* BP_ENABLE_HD44780_SUPPORT */