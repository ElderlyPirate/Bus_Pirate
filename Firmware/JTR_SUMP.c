/*
 *
 *	USB infrared remote control receiver transmitter firmware v1.0
 *	License: creative commons - attribution, share-alike
 *	Copyright Ian Lesnet 2010
 *	http://dangerousprototypes.com
 *
 */
/* *****************************************************************************
 BPv4 JTR-SUMP A folked version by JTR. Please see changelog.txt for details.

 * JTR SUMP Version 0.1 beta
 ********************************************************************************/

#include "base.h"
#include "sump.h"

//commandset
//http://www.sump.org/projects/analyzer/protocol/
#define SUMP_RESET 	0x00
#define SUMP_RUN	0x01
#define SUMP_ID		0x02
#define SUMP_DESC	0x04
#define SUMP_XON	0x11
#define SUMP_XOFF 	0x13
#define SUMP_DIV 	0x80
#define SUMP_CNT	0x81
#define SUMP_FLAGS	0x82
#define SUMP_TRIG	0xc0
#define SUMP_TRIG_VALS  0xc1
#define LA_SAMPLE_SIZE  4096

extern BYTE *InPtr, *OutPtr, cdc_In_len, cdc_timeout_count;
extern BDentry *CDC_Outbdp, *CDC_Inbdp;
//extern CDC_FLUSH_CONTROL cdc_Flush_Control;
static unsigned int sumpSamples;

// REMAPPING OF THE CHANGE NOTIFICATIONS BPv3 Vs BPv4
//BPv3
//       PORTB0, PORTB1, PORTB2, PORTB3, PORTB4, PORTB5, PORTB6, PORTB7, PORTB8, PORTB9, PORTB10
//PIN      4      5        6       7       11     14     15      16       17      18      21
//CN_No    4      5        6       7       1      27     24      23       22      21      16
//CNEN                                                   2:8     2:7      2:6     2:5     2:0
//Func                                                   CS      MISO     CLK    MOSI     AUX1


//BPv4
//       PORTD0, PORTD1, PORTD2, PORTD3, PORTD4, PORTD5, PORTD6, PORTD7, PORTD8
//PIN      46     49      50      51      52      53      54      55      42
//CN_N0    49     50      51      52      13      14      15      16      53
//CNEN    4:1     4:2     4:3     4:4    1:13    1:14    1:15     2:0    4:5
//Func    AUX2   MOSI    CLK     MISO     CS      AUX0                   AUX1

void SUMP(void) {
    BYTE RECVedByte, sump_mode;
    static unsigned int i;
    static unsigned long l;
    unsigned int sump_port;

    struct _sumpRX {
        BYTE command[5];
        BYTE parameters;
        BYTE parCnt;
    } sumpRX;

#ifndef BUSPIRATEV4
    IODIR |= (AUX + MOSI + CLK + MISO + CS); //AUX, MOSI, CLK, MISO, CS pins to input
#else
    IODIR |= (AUX0 + AUX1 + AUX2 + MOSI + CLK + MISO + CS + SUMP_SPARE6 + SUMP_SPARE7); // BPv4 PORTD SUMP inputs x9
#endif

    SUMPreset();
    sump_mode = 1; // Remains in SUMP mode until sump_mode == 0
    bpWstring("1ALS");

    do { // Remains in this loop until a SUMP RESET (0x00) is received.
#ifndef USB_INTERRUPTS
        usb_handler();
#endif
        if (poll_getc_cdc(&RECVedByte) == 1) {//process any command bytes, otherwise it does nothing but loop

            switch (RECVedByte) {//switch on the current byte
                case SUMP_RESET://reset
                    SUMPreset();
                    sump_mode = 0; // Will exit at a single exit point below at the end of te do ... while loop
                    break;
                case SUMP_ID://SLA0 or 1 backwards: 1ALS
                    bpWstring("1ALS");
                    break;

                case SUMP_RUN://arm the triger, when completed exits SUMP mode.

                    BP_LEDMODE = 1; //ARMED, turn on LED

                    //setup timer
                    T4CON = 0; //make sure the counters are off
                    //timer 4 internal, measures interval
                    TMR5HLD = 0x00;
                    TMR4 = 0x00;
                    T4CON = 0b1000; //.T32=1, bit 3

                    //setup of change notice interrupt done in meta handler below

                    IFS1bits.CNIF = 0; //clear interrupt flag
                    IEC1bits.CNIE = 0; //interrupts DISABLED
                    IPC4bits.CNIP = 1; //priority to 0, not actual interupt

                    WaitInReady();
                    cdc_In_len = 0;
                    CDC_Flush_In_Now(); // Start with a clean slate. Reset InPtr and count etc.
                    CDC_Flush_In_Now();

#ifndef BUSPIRATEV4
                    do {

                    } while ((0 == IFS1bits.CNIF) && CNEN2);
#else
                    do {
#ifndef USB_INTERRUPTS
                        usb_handler();
#endif
                    } while (0 == IFS1bits.CNIF);

#endif
                    //OK! Ready to sample 
                    // Add LED control here.
                    // BP_VREGEN = 0; // JTR this is for debug ATM

                    T4CONbits.TON = 1; //start timer4
                    IFS1bits.T5IF = 0; //clear interrupt flag//setup timer and wait

                    // Critical timed loop, inversely proportional to our acheivable bandwidth
                    for (i = 0; i < sumpSamples; i++) { //take SAMPLE_SIZE samples
#ifndef BUSPIRATEV4
                        *InPtr++ = (PORTB >> 6); //change to pointer for faster use...
#else
#ifdef BPv4_SUMP_SOFT_WIRE // See hardwarev4a.h
                        sump_port = (PORTD & 0x13f);
                        if (sump_port & 0x100)
                            sump_port |= 0x80;

                        // Debugging. Chose one
                        * InPtr++ = sump_port;
                        // * InPtr++ = i;
#else
                        * InPtr++ = (BYTE)(PORTD & 0xff);  //sump_port; //change to pointer for faster use...
#endif

#endif                        
                        cdc_In_len += 1;
                        cdc_timeout_count = 0; // Must do this to prevent CDC_FlushOnTimeOut firing.

                        if (cdc_In_len == CDC_BUFFER_SIZE) {
                            putda_cdc(CDC_BUFFER_SIZE);
                            cdc_In_len = 0;
                            usb_handler();
                        }
                        while (IFS1bits.T5IF == 0); //wait for timer4 (timer 5 interrupt)
                        IFS1bits.T5IF = 0; //clear interrupt flag
                    }
#ifdef BUSPIRATEV4
                    CNEN1 &= 0b1001111111111111; //turn off used pins change notice
                    CNEN4 &= 0b1111111111000001;
#else
                    CNEN2 = 0; //change notice off
#endif
                    T4CON = 0; //stop count
#ifndef USB_INTERRUPTS
                    usb_handler();
#endif
                    WaitInReady();

                    // JTR debug only
                    //                    putc_cdc('q');
                    //                    putc_cdc('w');
                    //                    putc_cdc('e');
                    //                    BP_VREGEN = 1;
                    // End JTR debug.
                    SendZLP();
                    SUMPreset();
                    sump_mode = 0;
                    break;

                case SUMP_DESC:
                    // device name string
                    UART1TX(0x01);
#ifdef BUSPIRATEV4
                    bpWstring("BPv4");
#else                    
                    bpWstring("BPv3");
#endif
                    UART1TX(0x00);
                    //sample memory (4096) meaningless in JTR_SUMP mode
                    UART1TX(0x21);
                    UART1TX(0x00);
                    UART1TX(0x00);
                    UART1TX(0x10);
                    UART1TX(0x00);
                    //sample rate (1MHz)
                    UART1TX(0x23);
                    UART1TX(0x00);
                    UART1TX(0x0F);
                    UART1TX(0x42);
                    UART1TX(0x40);
                    //number of probes (5 or 8 on BPv4)
                    UART1TX(0x40);
                    UART1TX(NUM_OF_SUMP_CHANNELS);
                    //protocol version (2)
                    UART1TX(0x41);
                    UART1TX(0x02);
                    UART1TX(0x00);
                    break;
                case SUMP_XON://resume send data
                    //	xflow=1;
                    //break;
                case SUMP_XOFF://pause send data
                    //	xflow=0;
                    break;

                default://long command
                    sumpRX.command[0] = RECVedByte; //store first command byte
                    sumpRX.command[1] = getc_cdc(); //store first command byte
                    sumpRX.command[2] = getc_cdc(); //store first command byte
                    sumpRX.command[3] = getc_cdc(); //store first command byte
                    sumpRX.command[4] = getc_cdc(); //store first command byte
                    switch (sumpRX.command[0]) {

                        case SUMP_TRIG: //set CN on these pins
#ifdef BUSPIRATEV4                    
                            if (sumpRX.command[1] & 0b1000000) CNEN4 |= 0b10; //AUX2
#ifdef BPv4_SUMP_SOFT_WIRE // See hardwarev4a.h                            
                            if (sumpRX.command[1] & 0b100000) CNEN4 |= 0b100000; //AUX1 on PORTD:8 will be moved in firmware to B7 of SUMP byte
#else // possible GREEN WIRE mode PORTD8 to PORTD7
                            if (sumpRX.command[1] & 0b100000) CNEN2 |= 0b1; //AUX1 on PORTD:7
#endif                            
                            if (sumpRX.command[1] & 0b10000) CNEN1 |= 0b100000000000000; //AUX0
                            if (sumpRX.command[1] & 0b1000) CNEN4 |= 0b100; // MOSI
                            if (sumpRX.command[1] & 0b100) CNEN4 |= 0b1000; // CLK
                            if (sumpRX.command[1] & 0b10) CNEN4 |= 0b10000; // MISO
                            if (sumpRX.command[1] & 0b1) CNEN1 |= 0b10000000000000; // CS
#else                    
                            if (sumpRX.command[1] & 0b10000) CNEN2 |= 0b1; //AUX
                            if (sumpRX.command[1] & 0b1000) CNEN2 |= 0b100000; // MOSI
                            if (sumpRX.command[1] & 0b100) CNEN2 |= 0b1000000; // CLK
                            if (sumpRX.command[1] & 0b10) CNEN2 |= 0b10000000; // MISO
                            if (sumpRX.command[1] & 0b1) CNEN2 |= 0b100000000; // CS
#endif                     

                        case SUMP_CNT:
                            sumpSamples = sumpRX.command[2];
                            sumpSamples <<= 8;
                            sumpSamples |= sumpRX.command[1];
                            sumpSamples = (sumpSamples + 1)*4;
#ifndef BUSPIRATEV4
                            //prevent buffer overruns
                            if (sumpSamples > LA_SAMPLE_SIZE) sumpSamples = LA_SAMPLE_SIZE;
#endif
                            break;
                        case SUMP_DIV:
                            l = sumpRX.command[3];
                            l <<= 8;
                            l |= sumpRX.command[2];
                            l <<= 8;
                            l |= sumpRX.command[1];

                            //convert from SUMP 100MHz clock to our 16MIPs
                            l = ((l + 1)*4) / 25;

                            //adjust downwards a bit
                            if (l > 0x10)
                                l -= 0x10;
                            else //fast as possible
                                l = 1;

                            //setup PR register
                            PR5 = (l >> 16); //most significant word
                            PR4 = l; //least significant word
                            break;
                    } // sumpRX.command
            } // switch outbyte
        } // if byte received
    } while (1 == sump_mode);
}

void SUMPreset(void) {
    BP_LEDMODE = 0; //LED
#ifdef BUSPIRATEV4 
    CNPU1 &= 0b1001111111111111; //turn off pullups on used pins
    CNPU4 &= 0b1111111111000001;
    CNEN1 &= 0b1001111111111111; //turn off used pins change notice
    CNEN4 &= 0b1111111111000001;

#else    
    CNPU1 = 0; //pullups off
    CNPU2 = 0;
    CNEN1 = 0; //all change notice off
    CNEN2 = 0;
#endif   

    T4CON = 0; //stop count
    IPC4bits.CNIP = 0;

    //default speed and samples
    //setup PR register
    PR5 = 0; //most significant word
    PR4 = 0x640; //least significant word
    sumpSamples = LA_SAMPLE_SIZE;
}


