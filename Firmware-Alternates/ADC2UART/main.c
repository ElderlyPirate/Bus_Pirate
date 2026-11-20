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

#define VREG_DIR TRISAbits.TRISA0 // sets VREG33_DIR to TRISBIT A0
#define VREG_EN LATAbits.LATA0    // sets VREG_EN to LAT bit A0 (output latch)
#define MODE_LED_DIR TRISAbits.TRISA1 // sets the MODE_LED_DIR to TRISBIT A1
#define MODE_LED LATAbits.LATA1       // sets the MODE_LED to LAT bit A1 (output
                                // latch)

#pragma code

void initADC();
unsigned int getADC();
unsigned char UART1RX(void);
void UART1TX(char c);
void InitializeUART1(void);

// this loop services user input and passes it to be processed on <enter>
int main(void) {

  CLKDIVbits.RCDIV0 = 0; // clock divider to 0
  AD1PCFG = 0xFFFF;      // Default all pins to digital
  OSCCONbits.SOSCEN = 0; // Oscilator setup, registar-OSCON bits-SOSCEN
  MODE_LED_DIR = 0;      // Mode LED set as output
  VREG_DIR = 0;          // VREG is set as output
  VREG_EN = 1;  // Sets the VREG pin to 1, turns on the Voltage regulators.
  MODE_LED = 1; // Turns the MOD LED 0N

  InitializeUART1(); // Initialize UART1
  initADC();         // Initialise ADC

  unsigned int voltage;

  /////FOREVER///LOOP//////////
  while (1) {
    if (UART1RX() == 'a') // Checks if the recived byte is 'a' if so it sends
                          // the two RAW bytes form ADC
    {
      voltage = getADC();    // reads the ADC pin
      UART1TX(voltage >> 8); // seds the top byte of ADC to UART
      UART1TX(voltage);      // sends the bottom byte of ADC to UART
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////
/////Analog to digital converter intitilisationn
///function///////////////////////////
/////Setch the ADC to read the AN12 chanel, whic is connected ot the BPv3 pin
///ADC///
////////////////////////////////////////////////////////////////////////////////////
void initADC() {
  //////////////////////////////////////////////////////////////
  ///////////////ADC config//////////////////////////////////
  AD1PCFGbits.PCFG12 = 0; // B12/AN12/ADC1/EXT

  // configure the ADC
  AD1CON1bits.SSRC = 0b111; // SSRC<3:0> = 111 implies internal
                            // counter ends sampling and starts
                            // converting.
  AD1CSSL = 0;
  AD1CON3 = 0x1F02; // Sample time = 31Tad,
                    // Tad = 2 Tcy
  AD1CON2 = 0;      // CSCNA =0 -does not scan inputs,
  // BUFS =0 -A/D is currently filling buffer 00-07,
  // SMPI=0 -Interrupts at the completion of conversion for each sample/convert
  // sequence,
  // BUFM =0 -Buffer configured as one 16-word buffer,
  // ALTS = 0 -Always uses MUX A input multiplexer settings

  AD1CHS = 12; // chanell selected 12, this is the AN12 pin which is conected to
               // the voltage devider and the ADC pin on the BPv3
  /////////////////////////////////////////////////////////////////
}

unsigned int getADC() {
  AD1CON1bits.ADON = 1; // turns the ADC module on
  AD1CON1bits.SAMP = 1; // start sample
  AD1CON1bits.DONE = 0; // starts conversion
  while (AD1CON1bits.DONE == 0)
    ;                   // wait for conversion to finish
  AD1CON1bits.ADON = 0; // turns the ADC module off
  return ADC1BUF0;
}

// get a byte from UART
unsigned char UART1RX(void) {

  while (U1STAbits.URXDA == 0)
    ;
  return U1RXREG;
}

// add byte to buffer, pause if full
// uses PIC 4 byte UART FIFO buffer
void UART1TX(char c) {
  while (U1STAbits.UTXBF == 1)
    ; // if buffer is full, wait
  U1TXREG = c;
}

// Initialize the terminal UART for the speed currently set in
// bpConfig.termSpeed
void InitializeUART1(void) {

  TRISBbits.TRISB4 = 1; // UART 1 RX set as input
  // set pin configuration using peripheral pin select
  RPINR18bits.U1RXR = 5; // UART1 RX assign to RP5
  RPOR2bits.RP4R = 3;    // UART1 TX assign to RP4

  U1BRG = 34; // 13332=300, 1666=2400, 416=9600, 34@32mhz=115200....
  U1MODE = 0;
  U1MODEbits.BRGH = 1;
  U1STA = 0;
  U1MODEbits.UARTEN = 1;
  U1STAbits.UTXEN = 1;
  IFS0bits.U1RXIF = 0;
}
