
#include "globals.h"
#include "bootloader.h"

extern struct _irtoyusbbuffer usbbuf;
void usbbufservice(void);
void bootloader(void);
void usb_handler(void);
void WritePage(void);
void __builtin_write_NVM(void);
void __builtin_tblwtl(unsigned int offset, unsigned int data);
void __builtin_tblwth(unsigned int offset, unsigned int data);
unsigned int __builtin_tblrdl(unsigned int offset);
unsigned int __builtin_tblrdh(unsigned int offset);


BYTE inbyte; // JTR3 Global
BYTE errflag;
long fulladdress;

BYTE bldone = 0;
extern BYTE cdc_In_buffer[64];
extern BYTE cdc_Out_buffer[64];
#define VER_H 0x04
#define VER_L 0x06

unsigned int userversion  __attribute__((space(prog),address(BLENDADDR-9))) = ((VER_H<<8)|VER_L); 

int foo __attribute__((section("mysection"),address(0x2000)));

struct _bootstruct {
    BYTE enableerase;
    BYTE blreturn;
    BYTE addrU;
    BYTE addrH;
    BYTE addrL;
    BYTE cmd;
    BYTE datasize;
    BYTE checksum;
    BYTE data[64 * 3];

} bootstruct;

#define  HAS_PAGES
#define  PROT_BL
#define  PROT_GOTO
#define  PROT_CONFIG


void bootloader(void) {
	volatile unsigned int i,j;
	BYTE crc;

    bootstruct.enableerase = 0;

    do {
        do {
            usb_handler();
            usbbufservice();
        } while (usbbufgetbyte(&inbyte) == 0);

    } while (inbyte != 0xc1); //wait for bootloader hello

	//send device and protocol info
    WaitInReady();
    cdc_In_buffer[0] = DEVICEID; //answer OK
    cdc_In_buffer[1] = VER_H;
    cdc_In_buffer[2] = VER_L;
    putUnsignedCharArrayUsbUsart(cdc_In_buffer, 3);

	//if no errors below we return K for OK
    bootstruct.blreturn = 'K';

    // MAIN BOOTLOADER LOOP HERE
    do {
error:
        	usb_handler();
        	WaitInReady();
            cdc_In_buffer[0] = bootstruct.blreturn; //answer OK
            putUnsignedCharArrayUsbUsart(cdc_In_buffer, 1);
			
			crc=0;

			//get bootloader command
            usbbufgetbyte(&bootstruct.addrU);
			crc+=bootstruct.addrU;
            usbbufgetbyte(&bootstruct.addrH);
			crc+=bootstruct.addrH;
            usbbufgetbyte(&bootstruct.addrL);
			crc+=bootstruct.addrL;
            usbbufgetbyte(&bootstruct.cmd);
			crc+=bootstruct.cmd;
            usbbufgetbyte(&bootstruct.datasize);
			crc+=bootstruct.datasize;

            TBLPAG = bootstruct.addrU;

			//get data, if any
            if (bootstruct.datasize > 1) {
                for (i = 0; i < bootstruct.datasize - 1; i++) {
                    usbbufgetbyte(&bootstruct.data[i]);
					crc+=bootstruct.data[i];
                }
            }
	
			//get checksum
            usbbufgetbyte(&bootstruct.checksum);
			crc+=bootstruct.checksum;
            // TODO add checksum computation and check
			if(crc!=0){
				bootstruct.blreturn='N';//return checksum error
				goto error;

			}

			//calculate flash address
        	//fulladdress = ( ((bootstruct.addrU) << 16) + ((bootstruct.addrH) << 8) + bootstruct.addrL);
        	fulladdress = (bootstruct.addrU); 
			fulladdress=fulladdress<<8;
			fulladdress+=bootstruct.addrH;
			fulladdress=fulladdress<<8;
			fulladdress+=bootstruct.addrL;

			//do command
	        switch (bootstruct.cmd) {
	            case 1: // enable erase, actual erase is before the next write
	                bootstruct.enableerase = 1;        
	                break;
	            case 2: //protect the bootloader and write the row
	                WritePage();
	                break;
				case 0xff:
					 U1CONbits.USBEN=0; //USB off
					//delay a while so computer sees us turn off USB
					j=0xFFFF;
					while(j--){
						i=0xFFFF;
						while(i--);
					}
					asm("RESET");//reset								
	            default: //unknown command
	                bootstruct.blreturn = 'U';
	                break;
	        }//switch(c)
    } while (!bldone);
}

void WritePage() {
    BYTE i;
    int dataword;
    unsigned int offset;
    BYTE errflag = 0;

	//check that we don't overwrite any important bootloader stuff


    #ifdef PROT_GOTO
        if (fulladdress == 0x0000) {//protect jump vector
            bootstruct.data[0] = 0x04; //replace with bootloader start address
            bootstruct.data[1] = (unsigned char) (BLSTARTADDR);
            bootstruct.data[2] = (unsigned char) (BLSTARTADDR >> 8);
            bootstruct.data[3] = 0x00;
            bootstruct.data[4] = (unsigned char) ((BLSTARTADDR >> 16) && 0xFF);
            bootstruct.data[5] = 0x00;
            } // EDIT to add this
    #endif
           
    #ifdef PROT_BL       
            if ((fulladdress <= BLENDADDR) && (fulladdress >= BLSTARTADDR)) {//protect config words
                bootstruct.blreturn = 'P'; 
                errflag = 1;
            }
    #endif       

    #ifdef PROT_CONFIG  // best guess not tested yet
            if (fulladdress >= (FLASHSIZE - (2 * (PAGESIZER * ROWSIZEW)))) {//protect config page
                bootstruct.blreturn = 'P';
                errflag = 1;
            }
    #endif
/*

    if (fulladdress == 0x0000) {//protect jump vector
#ifdef PROT_CONFIG
		bootstruct.data[0]=0x04; //replace with bootloader start address
		bootstruct.data[1]=(unsigned char)(BLSTARTADDR);
		bootstruct.data[2]=(unsigned char)(BLSTARTADDR>>8);
		bootstruct.data[3]=0x00;
		bootstruct.data[4]=(unsigned char) ((BLSTARTADDR >> 16) && 0xFF);
		bootstruct.data[5]=0x00;
#endif
    }else if ((fulladdress<=BLENDADDR) && (fulladdress>=BLSTARTADDR)) {//protect bootloader
#ifdef PROT_BL
        bootstruct.blreturn = 'P'; 
        errflag = 1;
#endif
	}else if (fulladdress >= (FLASHSIZE - (2 * (PAGESIZER * ROWSIZEW)))) {//protect config page
#ifdef PROT_CONFIG
        bootstruct.blreturn = 'P';
        errflag = 1;
#endif
    } 
*/
    if (!errflag) { //if no protection error, write row

		//erase page if enabled
        if (bootstruct.enableerase) {
            NVMCON = 0x4042; //Erase page on next WR

            __builtin_tblwtl((unsigned int) fulladdress, 0xFFFF);
            __builtin_write_NVM();
            while (NVMCONbits.WR == 1);
            bootstruct.enableerase = 0;
        }

        offset = (unsigned int) fulladdress;

        NVMCON = 0x4001; //setup row writes
		//write data to buffer
        for (i = 0; i < (bootstruct.datasize-1);)
        {
            dataword = bootstruct.data[i];
            __builtin_tblwth(offset, dataword);
            i++;
			dataword = bootstruct.data[i];
			i++;
			dataword |= (bootstruct.data[i] << 8);
			i++;
			__builtin_tblwtl(offset, dataword);
			offset +=2;
        }
		//write buffer to flash
        __builtin_write_NVM();
        while (NVMCONbits.WR == 1);
        NVMCONbits.WREN = 0;

        offset = (unsigned int) fulladdress;
        bootstruct.blreturn = 'K';
		//read data back and compare
        for (i = 0; i < (bootstruct.datasize-1);)
        {
            dataword = __builtin_tblrdh((unsigned int) (offset));
            dataword &= 0xFF;
            if (dataword != bootstruct.data[i])
                bootstruct.blreturn = 'V';
            i++;
            dataword = __builtin_tblrdl((unsigned int) offset);
            if ((BYTE) dataword != bootstruct.data[i])
                bootstruct.blreturn = 'V';
            i++;
            if ((BYTE) (dataword >> 8) != bootstruct.data[i])
                bootstruct.blreturn = 'V';
            i++;
            offset +=2;
        }

    }
}

