//get a byte from UART
unsigned char UART1RX(void); 

//add byte to buffer, pause if full
//uses PIC 4 byte UART FIFO buffer
void UART1TX(char c); 

void InitializeUART1(void);