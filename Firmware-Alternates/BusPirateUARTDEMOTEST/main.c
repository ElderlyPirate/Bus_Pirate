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

#pragma code

int main(void) {

  CLKDIVbits.RCDIV0 = 0; // clock divider to 0
  AD1PCFG = 0xFFFF;      // Default all pins to digital
  OSCCONbits.SOSCEN = 0;

  // Initialize UART1
  RPINR18bits.U1RXR = 5; // UART1 RX assign to RP5
  RPOR2bits.RP4R = 3;    // UART1 TX assign to RP4

  U1BRG = 34;
  U1MODE = 0;
  U1MODEbits.BRGH = 1;
  U1STA = 0;
  U1MODEbits.UARTEN = 1;
  U1STAbits.UTXEN = 1;
  IFS0bits.U1RXIF = 0;

  char c;
  // Main Loop
  while (1) {
    // wait for byte from USB to serial converter
    while (U1STAbits.URXDA == 0)
      ;
    c = U1RXREG; // get byte

    // wait for empty spot in transmit buffer
    while (U1STAbits.UTXBF == 1)
      ;
    // echo received character back to USB to serial converter
    U1TXREG = c;
  }
}
