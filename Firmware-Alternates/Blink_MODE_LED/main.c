#pragma config POSCMOD = NONE
#pragma config I2C1SEL = PRI
#pragma config IOL1WAY = ON
#pragma config OSCIOFNC = ON
#pragma config FCKSM = CSDCMD
#pragma config FNOSC = FRCPLL
#pragma config SOSCSEL = SOSC
#pragma config WUTSEL = LEG
#pragma config IESO = ON
#pragma config WDTPS = PS32768
#pragma config FWPSA = PR128
#pragma config WINDIS = ON
#pragma config FWDTEN = OFF
#pragma config ICS = PGx1
#pragma config GWRP = OFF
#pragma config GCP = OFF
#pragma config JTAGEN = OFF

#include <xc.h>

#pragma code

int main(void) {
  unsigned long i; // a 32 bit variable to use as a timer

  CLKDIVbits.RCDIV0 = 0; // clock divider to 0
  AD1PCFG = 0xFFFF;      // Default all pins to digital
  OSCCONbits.SOSCEN = 0; // Disables the secondary oscilator

  TRISAbits.TRISA1 = 0; // sets the Mode LED pin RA1 as output
  LATAbits.LATA1 = 0;   // turns LED off

  //////////////////////////////////////////////////////////////////
  /// FOREVER LOOP///////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////
  while (1) {
    LATAbits.LATA1 = ~LATAbits.LATA1; // toggles the MODE LED
    i = 0xFFFFF;                      // sets i to 1048575
    while (i--)
      ; // delay function
  }     // decrements i until 0
}
