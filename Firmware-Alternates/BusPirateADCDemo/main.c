#pragma config POSCMOD = NONE
#pragma config I2C1SEL = PRI
#pragma config IOL1WAY = ON
#pragma config OSCIOFNC = OFF
#pragma config FCKSM = CSDCMD
#pragma config FNOSC = FRCDIV
#pragma config SOSCSEL = SOSC
#pragma config WUTSEL = LEG
#pragma config IESO = ON
#pragma config WDTPS = PS32768
#pragma config FWPSA = PR128
#pragma config WINDIS = ON
#pragma config FWDTEN = ON
#pragma config ICS = PGx1
#pragma config GWRP = OFF
#pragma config GCP = OFF
#pragma config JTAGEN = ON

#include <xc.h>

#define VREG33_DIR TRISAbits.TRISA0
#define VREG33_EN LATAbits.LATA0
#define MODE_LED_DIR TRISAbits.TRISA1
#define MODE_LED LATAbits.LATA1

#pragma code

int main(void) {

  CLKDIVbits.RCDIV0 = 0; // clock divider to 0
  AD1PCFG = 0xFFFF;      // Default all pins to digital
  OSCCONbits.SOSCEN = 0; // Disables the secondary oscilator

  MODE_LED_DIR = 0; // sets the Mode LED pin RA1 as output
  MODE_LED = 0;     // turns LED off
  VREG33_DIR = 0;   // sets teh VREG pin RA0 as output
  VREG33_EN = 1;    // turns on the voltage regulator

  unsigned long int i, voltage;
  //////////////////////////////////////////////////////////////
  ///////////////ADC config//////////////////////////////////
  AD1PCFGbits.PCFG12 = 0;   // configure RB12 as analog
  AD1CON1bits.SSRC = 0b111; // SSRC<3:0> = 111 implies internal
                            // counter ends sampling and starts
                            // converting.
  AD1CON3 = 0x1F02;         // Sample time = 31Tad,
                            // Tad = 2 Tcy
  AD1CHS = 12;              // ADC channel select 12
  AD1CON1bits.ADON = 1;     // turn ADC on
  /// FOREVER LOOP///////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////
  while (1) {
    // this just gives us a little delay between measurements
    i = 0xFFFFF; // sets i to 1048575
    while (i--)
      ; // delay function

    // start a measurement with the ADC
    AD1CON1bits.DONE = 0; // resets DONE bit
    AD1CON1bits.SAMP = 1; // start sample

    while (AD1CON1bits.DONE == 0)
      ; // wait for conversion to finish

    // get the measurement and use it to control the LED
    voltage = ADC1BUF0; // get the voltage measurement
    if (voltage > 0x1D1)
      MODE_LED = 1; // enable LED if measurement is > 3volts
    else
      MODE_LED = 0; // disable LED if less than 3volts
  }
}
