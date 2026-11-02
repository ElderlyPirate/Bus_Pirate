#ifndef BUSPIRATE_h_
#define BUSPIRATE_h_

#include <stdint.h>



#endif


#define ERR  -1
#define BBIO 0x00
#define SPI 0x01
#define I2C 0x02
#define ART  0x03
#define OWD  0x04
#define RAW  0X05
#define JTAG 0x18


uint32_t BP_WriteToPirate(int , char *);
int BP_EnableBinary(int);
int BP_EnableMode(int , char );
uint32_t BP_WriteToPirateNoCheck(int fd, char * val);
