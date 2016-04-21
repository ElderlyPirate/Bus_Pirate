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
#include "base.h"
#include "bitbang.h"
#include "busPirateCore.h"//need access to bpConfig
#include "binIOhelpers.h"
#include "AUXpin.h"

#include "procMenu.h"		// for the userinteraction subs

//software or hardware I2C mode defines
#define SOFT 0
#define HARD 1
#if defined (BUSPIRATEV4)
#define BP_USE_I2C_HW
unsigned char i2cinternal = 0;
#endif

#define SCL 		BP_CLK
#define SCL_TRIS 	BP_CLK_DIR     //-- The SCL Direction Register Bit
#define SDA 		BP_MOSI        //-- The SDA output pin
#define SDA_TRIS 	BP_MOSI_DIR    //-- The SDA Direction Register Bit

extern struct _bpConfig bpConfig; //holds persistant bus pirate settings (see base.h) need hardware version info
extern struct _modeConfig modeConfig;
extern struct _command bpCommand;

/*
static struct _i2csniff {
        unsigned char bits;
        unsigned char data;
        unsigned char ACK:1;
        unsigned char datastate:1; //are we collecting data yet?
        unsigned char I2CD:2;
        unsigned char I2CC:2; //prevous and current clock pin state
} I2Csniff;
 */
//hardware functions
void hwi2cSetup(void);
void hwi2cstart(void);
void hwi2cstop(void);
void hwi2csendack(unsigned char ack);
unsigned char hwi2cgetack(void);
void hwi2cwrite(unsigned char c);
unsigned char hwi2cread(void);
void binI2CversionString(void);

#ifdef BP_USE_I2C_HW
static unsigned char I2Cspeed[] = {157, 37, 13}; //100,400,1000khz; datasheet pg 145
#endif

//software functions
void I2C_Setup(void);
void I2Csetup_exc(void);
void I2C_SnifferSetup(void);
void I2C_Sniffer(unsigned char termMode);

int i2cmode;
int ackPending;

/*
extern int getnumber(int def, int max); //linker happy? everybody happy :D
// move into a .h or other .c??? 
int getnumber(int def, int max); // everything to make the compiler happy *dubbelzucht*
int getint(void);
int getrepeat(void);
void consumewhitechars(void);
extern int cmderror;
 */

unsigned int I2Cread(void) {
    unsigned char c = 0;
    if (ackPending) {
        if (i2cmode == SOFT) {
            bbI2Cack();
        }
#ifdef BP_USE_I2C_HW
        else {
            hwi2csendack(0); //all other reads get an ACK
        }
#endif
	bpSP;
        //bpWmessage(MSG_ACK);
        BPMSG1060;
        bpSP;
        ackPending = 0;
    }

    if (i2cmode == SOFT) {
        c = bbReadByte();
    }
#ifdef BP_USE_I2C_HW
    else {
        c = hwi2cread();
    }
#endif
    ackPending = 1;
    return c;
}

unsigned int I2Cwrite(unsigned int c) { //unsigned char c;
    if (ackPending) {
        bpSP;
        //bpWmessage(MSG_ACK);
        BPMSG1060;
        bpSP;
        if (i2cmode == SOFT) {
            bbI2Cack();
        }
#ifdef BP_USE_I2C_HW
        else {
            hwi2csendack(0); //all other reads get an ACK
        }
#endif
        ackPending = 0;
    }

    if (i2cmode == SOFT) {
        bbWriteByte(c);
        c = bbReadBit();
    }
#ifdef BP_USE_I2C_HW
    else {
        hwi2cwrite(c);
        c = hwi2cgetack();
    }
#endif
    bpSP;
    if (c == 0) { //bpWmessage(MSG_ACK);
        BPMSG1060;
        return 0x300; // bit 9=ack
    } else { //bpWmessage(MSG_NACK);
        BPMSG1061;
        return 0x100; // bit 9=ack
    }
}

void I2Cstart(void) {
    if (ackPending) { //bpWmessage(MSG_NACK);
        BPMSG1061;
        bpBR; //bpWline(OUMSG_I2C_READ_PEND_NACK);
        if (i2cmode == SOFT) {
            bbI2Cnack();
        }
#ifdef BP_USE_I2C_HW
        else {
            hwi2csendack(1); //the last read before a stop/start condition gets an NACK
        }
#endif
        ackPending = 0;
    }

    if (i2cmode == SOFT) {
        if (bbI2Cstart()) {//bus contention
            BPMSG1019; //warning
            BPMSG1020; //short or no pullups
            bpBR;
        }
    }
#ifdef BP_USE_I2C_HW
    else {
        hwi2cstart();
    }
#endif
    //bpWmessage(MSG_I2C_START);
    BPMSG1062;
}

void I2Cstop(void) {
    if (ackPending) { //bpWmessage(MSG_NACK);
        BPMSG1061;
        bpBR; //bpWline(OUMSG_I2C_READ_PEND_NACK);
        if (i2cmode == SOFT) {
            bbI2Cnack();
        }
#ifdef BP_USE_I2C_HW
        else {
            hwi2csendack(1); //the last read before a stop/start condition gets an NACK
        }
#endif
        ackPending = 0;
    }

    if (i2cmode == SOFT) {
        bbI2Cstop();
    }
#ifdef BP_USE_I2C_HW
    else {
        hwi2cstop();
    }
#endif
    //bpWmessage(MSG_I2C_STOP);
    BPMSG1063;
}

void I2Csettings(void) { //bpWstring("I2C (mod spd)=( ");
    BPMSG1068;
#ifdef BP_USE_I2C_HW
    bpWdec(i2cmode);
    bpSP;
#else
    bpWdec(0);
    bpSP; // softmode
#endif 
    bpWdec(modeConfig.speed);
    bpSP;
    bpWline(")");
}

void I2Csetup(void) {
    int HW, speed;

    HW = 0; // keep compiler happy if BP_USE_HW is not defined

#ifdef BP_USE_I2C_HW
    consumewhitechars();
    HW = getint();
#else
    i2cmode = SOFT;
#endif

    consumewhitechars();
    speed = getint();

#ifdef BP_USE_I2C_HW
    if ((HW > 0) && (HW <= 2)) {
        i2cmode = HW - 1;
    } else {
        speed = 0;
    }
#endif

    if ((speed > 0) && (speed <= 4)) {
        modeConfig.speed = speed - 1;
    } else {
        speed = 0;
    }

    if (speed == 0) {
        cmderror = 0;

#ifdef BP_USE_I2C_HW
        //bpWline(OUMSG_I2C_CON);
        BPMSG1064;
        i2cmode = (getnumber(1, 1, 2, 0) - 1);
#else
        i2cmode = SOFT;
#endif

        if (i2cmode == SOFT) {
            //bpWmessage(MSG_OPT_BB_SPEED);
            BPMSG1065;
            modeConfig.speed = (getnumber(1, 1, 4, 0) - 1);
        } else {
#if defined (BUSPIRATEV1A)||defined (BUSPIRATEV2)
            // There is a hardware incompatibility with <B4
            // See http://forum.microchip.com/tm.aspx?m=271183&mpage=1
            if (bpConfig.dev_rev <= PIC_REV_A3) BPMSG1066; //bpWline(OUMSG_I2C_REV3_WARN);
#endif
            //bpWline(OUMSG_I2C_HWSPEED);
            BPMSG1067;
            modeConfig.speed = (getnumber(1, 1, 3, 0) - 1);
        }
    } else {
#if defined (BUSPIRATEV1A)||defined (BUSPIRATEV2)
        // There is a hardware incompatibility with <B4
        // See http://forum.microchip.com/tm.aspx?m=271183&mpage=1
        if (bpConfig.dev_rev <= PIC_REV_A3) BPMSG1066; //bpWline(OUMSG_I2C_REV3_WARN);
#endif
        I2Csettings();

        ackPending = 0;
        //		I2Cstop();			// this needs to be done after a mode change??
    }

    //set the options avaiable here....
    modeConfig.HiZ = 1; //yes, always hiz
}

void I2Csetup_exc(void) //Executes the setup.. controlled with 'P' command
{
   if (i2cmode == SOFT) {
       SDA_TRIS = 1;
       SCL_TRIS = 1;
       SCL = 0; //B8 scl
       SDA = 0; //B9 sda
       bbSetup(2, modeConfig.speed); //configure the bitbang library for 2-wire, set the speed
   }
#ifdef BP_USE_I2C_HW
   else {
       hwi2cSetup();
   }
#endif   
}

   
void I2Ccleanup(void) {
    ackPending = 0; //clear any pending ACK from previous use
    if (i2cmode == HARD) {
        I2C1CONbits.I2CEN = 0; //disable I2C module
#ifdef BUSPIRATEV4
        I2C3CONbits.I2CEN = 0; //disable I2C module
#endif
    }
}

void I2Cmacro(unsigned int c) {
    int i;

    switch (c) {
        case 0://menu
            //bpWline(OUMSG_I2C_MACRO_MENU);// 2. I2C bus sniffer\x0D\x0A");
            BPMSG1069;
            break;
        case 1:
            //setup both lines high first
            bbH(MOSI + CLK, 0);
            //bpWline(OUMSG_I2C_MACRO_SEARCH);
            BPMSG1070;
#ifdef BUSPIRATEV4
            if (((i2cinternal == 0) && (BP_CLK == 0 || BP_MOSI == 0)) || ((i2cinternal == 1) && (BP_EE_SDA == 0 && BP_EE_SCL == 0))) {
#else
            if (BP_CLK == 0 || BP_MOSI == 0) {
#endif
                BPMSG1019; //warning
                BPMSG1020; //short or no pullups
                bpBR;
                return;
            }
            for (i = 0; i < 0x100; i++) {

                if (i2cmode == SOFT) {
                    bbI2Cstart(); //send start
                    bbWriteByte(i); //send address
                    c = bbReadBit(); //look for ack
                }
#ifdef BP_USE_I2C_HW
                else {
                    hwi2cstart();
                    hwi2cwrite(i);
                    c = hwi2cgetack();
                }
#endif		
                if (c == 0) {//0 is ACK

                    bpWbyte(i);
                    bpWchar('('); //bpWstring("(");
                    bpWbyte((i >> 1));
                    if ((i & 0b1) == 0) {//if the first bit is set it's a read address, send a byte plus nack to clean up
                        bpWstring(" W");
                    } else {
                        if (i2cmode == SOFT) {
                            bbReadByte();
                            bbI2Cnack(); //bbWriteBit(1);//high bit is NACK
                        }
#ifdef BP_USE_I2C_HW
                        else {
                            hwi2cread();
                            hwi2csendack(1); //high bit is NACK
                        }
#endif
                        bpWstring(" R");
                    }
                    bpWstring(")");
                    bpSP;
                }
                if (i2cmode == SOFT) bbI2Cstop();
#ifdef BP_USE_I2C_HW
                else hwi2cstop();
#endif
            }
            bpWBR;

            break;
        case 2:
            if (i2cmode == HARD)I2C1CONbits.I2CEN = 0; //disable I2C module

            //bpWline(OUMSG_I2C_MACRO_SNIFFER);
            BPMSG1071;
            BPMSG1250;
            I2C_Sniffer(1); //set for terminal output

#ifdef BP_USE_I2C_HW
            if (i2cmode == HARD) hwi2cSetup(); //setup hardware I2C again
#endif
            break;
#if defined (BUSPIRATEV4)
        case 3: //in hardware mode (or software, I guess) we can edit the on-board EEPROM -software mode unimplemented...
            bpWline("Now using on-board EEPROM I2C interface");
            i2cinternal = 1;
            I2C1CONbits.A10M = 0;
            I2C1CONbits.SCLREL = 0;

            I2C1ADD = 0;
            I2C1MSK = 0;

            // Enable SMBus
            I2C1CONbits.SMEN = 0;

            // Baud rate setting
            I2C1BRG = I2Cspeed[modeConfig.speed];

            // Enable I2C module
            I2C1CONbits.I2CEN = 1;

            // disable other I2C module
            I2C3CONbits.I2CEN = 0;
            break;
        case 4:
            if (i2cinternal == 1) {
                bpWline("On-board EEPROM write protect disabled");
                BP_EE_WP = 0;
            }
            break;
#endif
        default:
            //bpWmessage(MSG_ERROR_MACRO);
            BPMSG1016;
    }
}

void I2Cpins(void) {
        #if defined(BUSPIRATEV4)
        BPMSG1261; //bpWline("-\t-\tSCL\tSDA");
        #else
       	BPMSG1231; //bpWline("SCL\tSDA\t-\t-");
        #endif
}
//
//
//	HARDWARE I2C BASE FUNCTIONS
//
//
#if defined BP_USE_I2C_HW 

void hwi2cstart(void) {

#if defined (BUSPIRATEV4)
    if (i2cinternal == 0) {
        // Enable a start condition
        I2C3CONbits.SEN = 1;
        while (I2C3CONbits.SEN == 1); //wait
        return;
    }
#endif
    // Enable a start condition
    I2C1CONbits.SEN = 1;
    while (I2C1CONbits.SEN == 1); //wait
}

void hwi2cstop(void) {

#if defined (BUSPIRATEV4)
    if (i2cinternal == 0) {
        I2C3CONbits.PEN = 1;
        while (I2C3CONbits.PEN == 1); //wait
        return;
    }
#endif
    I2C1CONbits.PEN = 1;
    while (I2C1CONbits.PEN == 1); //wait
}

unsigned char hwi2cgetack(void) {

#if defined (BUSPIRATEV4)
    if (i2cinternal == 0) {
        return I2C3STATbits.ACKSTAT;
    }
#endif
    return I2C1STATbits.ACKSTAT;
}

void hwi2csendack(unsigned char ack) {
#if defined (BUSPIRATEV4)
    if (i2cinternal == 0) {
        I2C3CONbits.ACKDT = ack; //send ACK (0) or NACK(1)?
        I2C3CONbits.ACKEN = 1;
        while (I2C3CONbits.ACKEN == 1);
        return;
    }
#endif
    I2C1CONbits.ACKDT = ack; //send ACK (0) or NACK(1)?
    I2C1CONbits.ACKEN = 1;
    while (I2C1CONbits.ACKEN == 1);
}

void hwi2cwrite(unsigned char c) {
#if defined (BUSPIRATEV4)
    if (i2cinternal == 0) {
        I2C3TRN = c;
        while (I2C3STATbits.TRSTAT == 1);
        return;
    }
#endif
    I2C1TRN = c;
    while (I2C1STATbits.TRSTAT == 1);
}

unsigned char hwi2cread(void) {
    unsigned char c;
#if defined (BUSPIRATEV4)
    if (i2cinternal == 0) {
        I2C3CONbits.RCEN = 1;
        while (I2C3CONbits.RCEN == 1);
        c = I2C3RCV;
        return c;
    }
#endif
    I2C1CONbits.RCEN = 1;
    while (I2C1CONbits.RCEN == 1);
    c = I2C1RCV;
    return c;
}

void hwi2cSetup(void) {
    I2C3CONbits.A10M = 0;
    I2C3CONbits.SCLREL = 0;

    I2C3ADD = 0;
    I2C3MSK = 0;

    // Enable SMBus
    I2C3CONbits.SMEN = 0;


    // Baud rate setting
    I2C3BRG = I2Cspeed[modeConfig.speed];

    // Enable I2C module
    I2C3CONbits.I2CEN = 1;

    //
    // This work around didn't work for me...
    //
    /*
            //NICE TRY, BUT NO CIGAR
            //for revision 3, the SDA has to be hit once manually before
            //it will work, we use the connected pullup resistor to jump start
            //the broken hardware module.
            bpDelayUS(200);
            LATBbits.LATB11=0;//hold to ground
            TRISBbits.TRISB11=0;//SDA Pullup
            bpDelayUS(250);
            TRISBbits.TRISB11=1;//SDA Pullup
            LATBbits.LATB11=1;//release
     */

}

#endif
//*******************/
//
//
//	sofware I2C sniffer (very alpha)
//
//
//*******************/
/*
void I2C_SnifferSetup(void){
//we dont actually use interrupts, we poll the interrupt flag

//1. Ensure that the CN pin is configured as a digital input by setting the associated bit in the
//TRISx register.
//2. Enable interrupts for the selected CN pins by setting the appropriate bits in the CNENx
//registers.
//3. Turn on the weak pull-up devices (if desired) for the selected CN pins by setting the
//appropriate bits in the CNPUx registers.
//4. Clear the CNxIF interrupt flag.

        //-- Ensure pins are in high impedance mode --
        SDA_TRIS=1;
        SCL_TRIS=1;
        //writes to the PORTs write to the LATCH
        SCL=0;			//B8 scl
        SDA=0;			//B9 sda
	
        //enable change notice on SCL and SDA
        CNEN2bits.CN21IE=1;//MOSI
        CNEN2bits.CN22IE=1;

        //clear the CNIF interrupt flag
        IFS1bits.CNIF=0;

        I2Csniff.datastate=0;
        I2Csniff.bits=0;
        I2Csniff.I2CD|=SDA; //save current pin state in var
        I2Csniff.I2CC|=SCL; //use to see which pin changes on interrupt

}
 */


// \ - escape character
//[ - start
//0xXX - data
//+ - ACK +
//- - NACK -
//] - stop
#define ESCAPE_CHAR '\\'

/*
void I2C_Sniffer(unsigned char termMode){
        unsigned char c;

        //setup ring buffer pointers
        UARTbufSetup();
        I2C_SnifferSetup();

        while(1){

                //user IO service
                UARTbufService();
                if(UART1RXRdy == 1){//any key pressed, exit
                        c=UART1RX(); // JTR usb port
                        bpBR;
                        break;
                }

                //check for change in pin state, if none, return
                if(IFS1bits.CNIF==0) continue;

                IFS1bits.CNIF=0;
                I2Csniff.I2CD|=SDA; //save current pin state in var
                I2Csniff.I2CC|=SCL; //use to see which pin changes on interrupt
	
                if (I2Csniff.datastate==1 && I2Csniff.I2CC==0b01){//sample when clock goes low to high
	
                        if(I2Csniff.bits<8){
                                //the next 8 bits are data
                                I2Csniff.data <<=1; //move over one bit
                                I2Csniff.data |= (I2Csniff.I2CD & (~0b10)); //set bit, clear previous bit
                                I2Csniff.bits++;
                        }else{
                                I2Csniff.ACK=SDA; //check for ACK/NACK
	
                                if(termMode){//output for the terminal
                                        bpWhexBuf(I2Csniff.data);
                                }else{ //output for binary mode
                                        UARTbuf(ESCAPE_CHAR); //escape character
                                        UARTbuf(I2Csniff.data); //write byte value
                                }
	
                                if(I2Csniff.ACK)
                                        UARTbuf('-');
                                else
                                        UARTbuf('+'); //write ACK status
				
                                I2Csniff.bits=0;
                        }
	
                }else if(I2Csniff.I2CC==0b11){//clock high, must be data transition
                //if data changed while clock is high, start condition (HL) or stop condition (LH)
	
                        if(I2Csniff.I2CD==0b10){//start condition
                                I2Csniff.datastate=1;//start condition, allow data byte collection
                                I2Csniff.bits=0;
                                UARTbuf('[');//say start, use bus pirate syntax to display data
                        }else if(I2Csniff.I2CD==0b01){//stop condition
                                I2Csniff.datastate=0; //stop condition, don't allow byte collection
                                I2Csniff.bits=0;
                                UARTbuf(']');//say stop
                        }
	
                }

                //move current pin state to previous pin state
                I2Csniff.I2CD<<=1; //pSDA=I2Cpin.cSDA;
                I2Csniff.I2CC<<=1; //pin.pSCL=I2Cpin.cSCL;

        }
}
 */

void I2C_Sniffer(unsigned char termMode) {
    unsigned char SDANew, SDAOld;
    unsigned char SCLNew, SCLOld;

    unsigned char DataState = 0;
    unsigned char DataBits = 0;
    unsigned char dat = 0;

    //unsigned char *BitBuffer=bpConfig.terminalInput;
    //unsigned short BufferPos=0;
    //unsigned short AckPos=0;
    //unsigned short DataPos=0;

    //setup ring buffer pointers
    UARTbufSetup();

    SDA_TRIS = 1; // -- Ensure pins are in high impedance mode --
    SCL_TRIS = 1;

    SCL = 0; // writes to the PORTs write to the LATCH
    SDA = 0;

    BP_MOSI_CN = 1; // enable change notice on SCL and SDA
    BP_CLK_CN = 1;

    IFS1bits.CNIF = 0; // clear the change interrupt flag

    SDAOld = SDANew = SDA;
    SCLOld = SDANew = SCL;

    //while(!UART1RXRdy&&(BufferPos<32768)) // BitBuffer Space = 4096*8 bits
    while (1) {
        if (!IFS1bits.CNIF) {//check change notice interrupt
            //user IO service
                UARTbufService();
            if (UART1RXRdy()) {
                dat = UART1RX();
                break;
            }
            continue;

        }

        IFS1bits.CNIF = 0; //clear interrupt flag

        SDANew = SDA; //store current state right away
        SCLNew = SCL;

        if (DataState && !SCLOld && SCLNew) // Sample When SCL Goes Low To High
        {
            if (DataBits < 8) //we're still collecting data bits
            {
                dat = dat << 1;
                if (SDANew) {
                    dat |= 1;
                }

                DataBits++;
            } else {
                //put the data byte in the terminal or binary output
                if (termMode) {//output for the terminal
                    bpWhexBuf(dat);
                } else { //output for binary mode
                    UARTbuf(ESCAPE_CHAR); //escape character
                    UARTbuf(dat); //write byte value
                }

                if (SDANew) // SDA High Means NACK
                {
                    UARTbuf('-');
                } else // SDA Low Means ACK
                {
                    UARTbuf('+'); //write ACK status
                }

                DataBits = 0; // Ready For Next Data Byte

            }
        } else if (SCLOld && SCLNew) // SCL High, Must Be Data Transition
        {
            if (SDAOld && !SDANew) // Start Condition (High To Low)
            {
                DataState = 1; // Allow Data Collection
                DataBits = 0;

                UARTbuf('['); //say start, use bus pirate syntax to display data

            } else if (!SDAOld && SDANew) // Stop Condition (Low To High)
            {
                DataState = 0; // Don't Allow Data Collection
                DataBits = 0;

                UARTbuf(']'); //say stop

            }
        }

        SDAOld = SDANew; // Save Last States
        SCLOld = SCLNew;
    }

    BP_MOSI_CN = 0; // clear change notice
    BP_CLK_CN = 0;

    if (termMode) {
        bpBR;
    }
}

/*
rawI2C mode:
# 00000000//reset to BBIO
# 00000001 � mode version string (I2C1)
# 00000010 � I2C start bit
# 00000011 � I2C stop bit
# 00000100 - I2C read byte
# 00000110 - ACK bit
# 00000111 - NACK bit
# 0001xxxx � Bulk transfer, send 1-16 bytes (0=1byte!)
# (0110)000x - Set I2C speed, 3 = 400khz 2=100khz 1=50khz 0=5khz
# (0111)000x - Read speed, (planned)
# (0100)wxyz � Configure peripherals w=power, x=pullups, y=AUX, z=CS (was 0110)
# (0101)wxyz � read peripherals (planned, not implemented)
 */
void binI2CversionString(void) {
    bpWstring("I2C1");
}

void binI2C(void) {
    static unsigned char inByte, rawCommand, i;
    unsigned int j, fw, fr;
    //I2C setup
    SDA_TRIS = 1;
    SCL_TRIS = 1;
    SCL = 0; //B8 scl
    SDA = 0; //B9 sda
	

    //set CS pin direction to output on setup
    BP_CS_DIR = 0; //B6 cs output

    modeConfig.HiZ = 1; //yes, always hiz (bbio uses this setting, should be changed to a setup variable because stringing the modeconfig struct everyhwere is getting ugly!)
    modeConfig.lsbEN = 0; //just in case!
    bbSetup(2, 0xff); //configure the bitbang library for 2-wire, set the speed to default/high
    binI2CversionString(); //reply string

    while (1) {

        //JTR Not requiredwhile (UART1RXRdy == 0); //wait for a byte
        inByte = UART1RX();
        rawCommand = (inByte >> 4); //get command bits in seperate variable

        switch (rawCommand) {
            case 0://reset/setup/config commands
                switch (inByte) {
                    case 0://0, reset exit
                        //cleanup!!!!!!!!!!
                        return; //exit
                        break;
                    case 1://1 - id reply string
                        binI2CversionString(); //reply string
                        break;
                    case 2://I2C start bit
                        bbI2Cstart();
                        UART1TX(1);
                        break;
                    case 3://I2C stop bit
                        bbI2Cstop();
                        UART1TX(1);
                        break;
                    case 4://I2C read byte
                        UART1TX(bbReadByte());
                        break;
                    case 6://I2C send ACK
                        bbI2Cack();
                        UART1TX(1);
                        break;
                    case 7://I2C send NACK
                        bbI2Cnack();
                        UART1TX(1);
                        break;
                    case 8: //write-then-read
                        //get the number of commands that will follow
                        //JTR Not required while (!UART1RXRdy()); //wait for a byte
                        fw = UART1RX();
                        /* JTR usb port; */; //get byte
                        fw = fw << 8;
                        //JTR Not required while (!UART1RXRdy()); //wait for a byte
                        fw |= UART1RX();
                        /* JTR usb port; */; //get byte

                        //get the number of reads to do
                        //JTR Not required while (!UART1RXRdy()); //wait for a byte
                        fr = UART1RX();
                        /* JTR usb port; */; //get byte
                        fr = fr << 8;
                        //JTR Not required while (!UART1RXRdy()); //wait for a byte
                        fr |= UART1RX();
                        /* JTR usb port; */; //get byte


                        //check length and report error
                        if (fw > TERMINAL_BUFFER || fr > TERMINAL_BUFFER) {
I2C_write_read_error: //use this for the read error too
                            UART1TX(0);
                            break;
                        }

                        //get bytes
                        for (j = 0; j < fw; j++) {
                            //JTR Not required while (!UART1RXRdy()); //wait for a byte
                            bpConfig.terminalInput[j] = UART1RX();
                            /* JTR usb port; */;
                        }

                        //start
                        bbI2Cstart();

                        for (j = 0; j < fw; j++) {
                            //get ACK
                            //if no ack, goto error
                            bbWriteByte(bpConfig.terminalInput[j]); //send byte
                            if (bbReadBit() == 1) goto I2C_write_read_error;
                        }

                        fw = fr - 1; //reuse fw
                        for (j = 0; j < fr; j++) { //read bulk bytes from SPI
                            //send ack
                            //i flast byte, send NACK
                            bpConfig.terminalInput[j] = bbReadByte();

                            if (j < fw) {
                                bbI2Cack();
                            } else {
                                bbI2Cnack();
                            }
                        }
                        //I2C stop
                        bbI2Cstop();

                        UART1TX(1); //send 1/OK

                        for (j = 0; j < fr; j++) { //send the read buffer contents over serial
                            UART1TX(bpConfig.terminalInput[j]);
                        }

                        break;//00001001 xxxxxxxx
					case 9: //extended AUX command
					      UART1TX(1); //confirm that the command is known
					      //inByte - used as extended commmand
					      //fr - used as result
					      while(U1STAbits.URXDA == 0);//wait for subcommand byte
					      inByte=U1RXREG; //get byte
					      //0x00 - AUX/CS low
					      //0x01 - AUX/CS high
					      //0x02 - AUX/CS HiZ
					      //0x03 - AUX read
					      //0x10 - use AUX
					      //0x20 - use CS
					      fr=1;
					      switch( inByte ) {
					         case 0x00:
					            bpAuxLow();
					            break;
					         case 0x01:
					            bpAuxHigh();
					            break;
					         case 0x02:
					            bpAuxHiZ();
					            break;
					         case 0x03:
					            fr = bpAuxRead();
					            break;
					         case 0x10:
					            modeConfig.altAUX = 0;
					            break;
					         case 0x20:
					            modeConfig.altAUX = 1;
					            break;
					         default:
					            fw = 0;
					            break;
					      }
					      UART1TX(fr);//result
					      break;
                    case 0b1111:
                        I2C_Sniffer(0); //set for raw output
                        UART1TX(1);
                        break;
                    default:
                        UART1TX(0);
                        break;
                }
                break;

            case 0b0001://get x+1 bytes
                inByte &= (~0b11110000); //clear command portion
                inByte++; //increment by 1, 0=1byte
                UART1TX(1); //send 1/OK

                for (i = 0; i < inByte; i++) {
                    //JTR Not required while (UART1RXRdy() == 0); //wait for a byte
                    bbWriteByte(UART1RX()); // JTR usb port //send byte
                    UART1TX(bbReadBit()); //return ACK0 or NACK1
                }

                break;

            case 0b0110://set speed
                inByte &= (~0b11111100); //clear command portion
                bbSetup(2, inByte); //set I2C speed
                UART1TX(1);
                break;

            case 0b0100: //configure peripherals w=power, x=pullups, y=AUX, z=CS
                binIOperipheralset(inByte);
                UART1TX(1); //send 1/OK
                break;
#ifdef BUSPIRATEV4
            case 0b0101:
                UART1TX(binBBpullVoltage(inByte));
                break;
#endif
            default:
                UART1TX(0x00); //send 0/Error
                break;
        }//command switch
    }//while loop

}


