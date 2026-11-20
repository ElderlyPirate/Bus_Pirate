//these are the functions we need to provide for XAPP58
#ifndef xsvf_ports_h
#define xsvf_ports_h

#define TCK (short) 0
#define TMS (short) 1
#define TDI (short) 2

//setup the read buffer before starting
void xsvf_setup(void);

//setup the specified output pin p with val
extern void setPort(short p, short val);

//read TDO pin
extern unsigned char readTDOBit();

//read byte of xsvf
extern void readByte(unsigned char *data);

//pause one microsecond
extern void waitTime(long microsec);

#endif
