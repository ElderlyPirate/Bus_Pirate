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
#include "base.h"
#include "proc_menu.h"

//direction registers
#define SPIMOSI_TRIS    BP_MOSI_DIR     
#define SPICLK_TRIS     BP_CLK_DIR      
#define SPIMISO_TRIS    BP_MISO_DIR     
#define SPICS_TRIS              BP_CS_DIR       

//pin control registers
#define SPIMOSI                 BP_MOSI
#define SPICLK                  BP_CLK  
#define SPIMISO                 BP_MISO 
#define SPICS                   BP_CS   

//Define how the PCF8574 pins connect to the LCD
#define HCT595_LCD_LED 0b00000001
#define HCT595_LCD_RS 0b00000010
#define HCT595_LCD_RW 0b00000100
#define HCT595_LCD_EN 0b00001000
#define HCT595_LCD_D4 0b00010000
#define HCT595_LCD_D5 0b00100000
#define HCT595_LCD_D6 0b01000000
#define HCT595_LCD_D7 0b10000000


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

//configuration structure
extern mode_configuration_t mode_configuration;
extern command_t last_command;

struct _HD44780_interface {
        unsigned char EN:1; //clock, active high
        unsigned char RS:1; //register select, 0=command, 1=text
        unsigned char RW:1; //read write, 0=write, 1=read
        unsigned char LED:1; //LED on IO board, 0=on, 1=off
        //unsigned char dat; //8 data bits
} HD44780;

static void HD44780_Reset(void); //reset the LCD to 4 bit mode
static void HD44780_Init(unsigned char displaylines); //initialize LCD to 4bit mode with typical settings and X displaylines
static void HD44780_WriteByte(unsigned char reg, unsigned char dat); //write a byte to LCD to register REG
static void HD44780_WriteNibble(unsigned char reg, unsigned char dat);//write 4 bits to LCD to register REG
static void HD44780_SPIwrite(unsigned char datout); //abstracts data output to PCF8574 IO expander over I2C bus

/* 
 * Duplicate the minimum amount of SPI functionality if SPI support
 * is disabled.
 */

#ifndef BP_ENABLE_SPI_SUPPORT

//open drain control registers for OUTPUT pins
#define SPIMOSI_ODC             BP_MISO_ODC     
#define SPICLK_ODC              BP_CLK_ODC      
#define SPICS_ODC               BP_CS_ODC       

unsigned char spi_write_byte(unsigned char c){
        SPI1BUF = c;
        while(!IFS0bits.SPI1IF);
        c=SPI1BUF;
        IFS0bits.SPI1IF = 0;
        return c;
}

void spi_disable_interface(void){
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

unsigned int LCDwrite(unsigned int c)
{       
        HD44780_WriteByte(HD44780.RS, c);
        return 0x100;
}

void LCDstart(void)
{       HD44780.RS=HD44780_COMMAND;
        //bpWline(OUMSG_LCD_COMMAND_MODE);
        BPMSG1213;
}

void LCDstop(void)
{       HD44780.RS=HD44780_DATA;
        //bpWline(OUMSG_LCD_DATA_MODE);
        BPMSG1214;
}

void LCDsetup(void)
{       
        mode_configuration.high_impedance=1;//yes, always HiZ
}

void LCDsetup_exc(void)
{
        HD44780.RS=HD44780_DATA;

		//PPS Setup
		// Inputs
		RPINR20bits.SDI1R = BP_MISO_RPIN; //MISO
		// Outputs
		BP_MOSI_RPOUT = SDO1_IO; //B9 MOSI
		BP_CLK_RPOUT = SCK1OUT_IO; //B8 CLK

        SPICS=0;                                //B6 cs low
        SPICS_TRIS=0;                   //B6 cs output

        //pps configures pins and this doesn't really matter....
        SPICLK_TRIS=0;                  //B8 sck output
        SPIMISO_TRIS=1;                 //B7 SDI input
        SPIMOSI_TRIS=0;                 //B9 SDO output

        /* CKE=1, CKP=0, SMP=0 */
        SPI1CON1 = 0b0100111101;//(SPIspeed[modeConfig.speed]); // CKE (output edge) active to idle, CKP idle low, SMP data sampled middle of output time.
        //SPI1CON1=0b11101;
        //SPI1CON1bits.MSTEN=1;
        //SPI1CON1bits.CKP=0;
        //SPI1CON1bits.CKE=1;           
        //SPI1CON1bits.SMP=0;
	    SPI1CON2 = 0;
	    SPI1STAT = 0;    // clear SPI
	    SPI1STATbits.SPIEN = 1;

        //bpWmessage(MSG_ADAPTER);
        BPMSG1216;
}

void LCDmacro(unsigned int c)
{       int input,i;

        cmdstart=(cmdstart+1)&CMDLENMSK;
        //cmdstart&=CMDLENMSK;
        consumewhitechars();
        input=getint();
        mode_configuration.command_error=false;

        switch(c)
        {       case 0://menu
                        //bpWline(OUMSG_LCD_MACRO_MENU);
                        BPMSG1219;
                        break;
                case 1:
                case 2:
                        //bpWline(OUMSG_LCD_MACRO_RESET);
                        BPMSG1093;
                        HD44780_Reset();
                        if(c==1) break;
        
                        if(!((input>=1)&&(input<=2)))
                        {       //bpWline(OUMSG_LCD_MACRO_INIT_DISPLAYLINES);
                                BPMSG1220;
                                //c=bpUserNumberPrompt(1, 2, 2);
                                input=getnumber(2,1,2, 0);
                                if(input==1) HD44780_Init(DISPLAYLINES1); else HD44780_Init(DISPLAYLINES2);
                                //bpWline(OUMSG_LCD_MACRO_INIT);
                        }
                        BPMSG1221;
                        break;          
                case 3: //Clear LCD and return home
                        HD44780_WriteByte(HD44780_COMMAND, CMD_CLEARDISPLAY);
                        bp_delay_ms(15);//delay 15ms
                        //bpWline(OUMSG_LCD_MACRO_CLEAR);
                        BPMSG1222;
                        break;  
                case 4: 
                        HD44780_WriteByte(HD44780_COMMAND, CMD_SETDDRAMADDR | (unsigned char)input);
                        //bpWline(OUMSG_LCD_MACRO_CURSOR);
                        BPMSG1223;
                        break;
                case 6: //write numbers 
                        HD44780_WriteByte(HD44780_COMMAND, CMD_CLEARDISPLAY);//Clear LCD and return home
                        bp_delay_ms(15);//delay 15ms
                        c=0x30;
                        if(input==0) input=80;
                        for(i=0; i<input; i++){
                                if(c>0x39) c=0x30;
                                HD44780_WriteByte(HD44780_DATA, c);
                                user_serial_transmit_character(c);
                                c++;
                        }
                        break;  
                case 7://write characters                               
                        HD44780_WriteByte(HD44780_COMMAND, CMD_CLEARDISPLAY); //Clear LCD and return home
                        bp_delay_ms(15);//delay 15ms
                        c=0x21; //start character (!)
                        if(input==0) input=80;
                        for(i=0; i<input; i++){
                                if(c>127)c=0x21;
                                HD44780_WriteByte(HD44780_DATA, c);
                                user_serial_transmit_character(c);
                                c++;
                        }
                        break;
/*              case 8://terminal mode/pass through   //superseeded by send string command
                                bpWline(OUMSG_LCD_MACRO_TEXT);
                                HD44780_Term();
                        break; */
                default:
                        MSG_UNKNOWN_MACRO_ERROR;
                        break;
        }

}

void LCDpins(void) {
    MSG_I2C_PINS_STATE;
}


//initialize LCD to 4bits with standard features
//displaylines=0 for single line displays, displaylines=1 for multiline displays
void HD44780_Init(unsigned char displaylines){
        //Function set
        HD44780_WriteByte(HD44780_COMMAND, (CMD_FUNCTIONSET + DATAWIDTH4 + FONT5X7 + displaylines)); //0x28, 0b101000
        bp_delay_ms(15);//delay 15ms
        
        //Turn display off
        HD44780_WriteByte(HD44780_COMMAND, CMD_DISPLAYCONTROL + DISPLAYOFF + CURSEROFF + BLINKOFF);//0x08, 0b1000
        bp_delay_ms(15);//delay 15ms
        
        //Clear LCD and return home
        HD44780_WriteByte(HD44780_COMMAND, CMD_CLEARDISPLAY);
        bp_delay_ms(15);//delay 15ms
        
        //Turn on display, turn off cursor and blink
        HD44780_WriteByte(HD44780_COMMAND, CMD_DISPLAYCONTROL + DISPLAYON + CURSERON + BLINKOFF);   // 0x0f, 0b1111
        bp_delay_ms(15);//delay 15ms
}

//reset LCD to 4bit mode
void HD44780_Reset(void){

        HD44780_SPIwrite(0); //clear IO pins to HD44780
        //# Wait more than 15 msecs after power is applied.
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

//write byte dat to register reg
void HD44780_WriteByte(unsigned char reg, unsigned char dat){
        HD44780_WriteNibble(reg, (dat>>4) );
        HD44780_WriteNibble(reg, (dat & 0x0F));
}

//write 4 bits dat to register reg
void HD44780_WriteNibble(unsigned char reg, unsigned char dat){
        //EN pin should already be low
        //RW bit should be 0 (already 0 in dat)
        //LED bit should be 0 (already 0 in dat)

        //databits are aligned with the upper four bits of 595
        //push them from the lower four bits of dat ot the upper four bits
        dat=dat<<4;

        dat|=HCT595_LCD_LED; //keep LED on

        if(reg==HD44780_DATA){  //does RS need to be set?
                dat |= HCT595_LCD_RS; //set register select flag for text
        }//leave as 0 for a command

        HD44780_SPIwrite(dat); //load values

        dat |= HCT595_LCD_EN; //raise the EN line to clock in the values
        HD44780_SPIwrite(dat);

        //optional setup delay

        dat &= (~HCT595_LCD_EN); //lower EN line
        HD44780_SPIwrite(dat);

}


//write value to 595 IO expander, message and return on ACK error
void HD44780_SPIwrite(unsigned char datout){
        spi_write_byte(datout);
        SPICS=1;
        //bpDelayUS(255);
        SPICS=0;
}

#endif /* BP_ENABLE_HD44780_SUPPORT */
