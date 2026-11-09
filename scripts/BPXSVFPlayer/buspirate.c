#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "serial.h"
#include "buspirate.h"

#ifndef WIN32
#define TRUE 1
#define FALSE 0
#define Sleep(x) usleep(x);
#endif

const char *modes[]={
    "BBIO",
    "SPI",
    "I2C",
    "ART",
    "1W0",
    "RAW",
    "0x06",
    "0x07",
    "0x08",
    "0x09",
    "0x0a",
    "0x0b",
    "0x0c",
    "0x0d",
    "0x0e",
    "0x0f",
    "0x10",
    "0x11",
    "0x12",
    "0x13",
    "0x14",
    "0x15",
    "0x16",
    "0x17",
    "XSVF",
};

//static struct BP_t pBP;
//static uint8_t BP_reversebyte(uint8_t c);
//static char bpbuf[4096];
//static int bpbufcnt;
extern int disable_comport;
extern int dumphandle;
extern int verbose;
extern int modem;
//extern char *modes[];
//low lever send command, get reply function

uint32_t BP_WriteToPirate(int fd, char * val) {
	int res = -1;
	char ret = 0;


    serial_write(fd, val, 1);
    Sleep(1);
    res = serial_read(fd, &ret, 1);

	if( ret != '\x01') {
	       if (modem==TRUE){
	             printf(" Modem responded with %i byte and with a value of 0X%X\n",res,ret);
                 return 0;
	           }
	            else {
		          printf(" ERROR: BusPirate replied with 0x%x instead of 0x01 \n",ret);
		          return -1;
	          }
	        }
    //printf(" BusPirate said: OK\n");
	return 0;
}

uint32_t BP_WriteToPirateNoCheck(int fd, char * val) {
	int res = -1;
	char ret = 0;


    serial_write(fd, val, 1);
    Sleep(1);
    res = serial_read(fd, &ret, 1);

	return 0;
}

int BP_EnableBinary(int fd)   // should return BBIO if ok, ERR if not
{
	int ret;
	char tmp[100] = { [0 ... 20] = 0x00 };
	int done = 0;
	//int cmd_sent = 0;
	int tries=0;

	printf(" Entering binary mode...\n");

	if (fd==-1)   //added because the fd has already returned null
	{
	    printf("Port does not exist!");
	    ret=ERR;

	}
   else
   {


	while (!done) {
		tmp[0]=0x00;
		//printf("Sending 0X%X to port\n",tmp[0]);
		serial_write(fd, tmp, 1);
		tries++;
	//	printf("tries: %i Ret %i\n",tries,ret);
		usleep(1);
		ret = serial_read(fd, tmp, 5);
		if (modem==TRUE)
		{
            printf("\n Modem Responded = %i\n",ret);
            done=1;
		}
		else {
		     if (ret != 5 && tries>20) {
			         fprintf(stderr, " Buspirate did not respond correctly :( %i \n", ret );
			         ret=ERR;
		      }
		      else if (strncmp(tmp, "BBIO1", 5) == 0) {
			       done=1;
		            ret=BBIO;
		           }
		}

		if (tries>25){
		printf(" Buspirate:Too many tries in serial read! -exiting \n - chip not detected, or not readable/writable\n");
		break;
		ret=ERR;
		}
	}
   }
   return ret;
}




int BP_EnableMode(int fd, char bbmode)
{
	int ret,response,current_mode,c;
	char tmp[100] = { [0 ... 20] = 0x00 };

	//int cmd_sent = 0;
	int tries=0;
    current_mode=(int)bbmode;
	printf(" Switching to %s mode\n",modes[current_mode]);


	tmp[0] = bbmode;
	//printf("Sending 0X%X to port\n",tmp[0]);
	serial_write(fd, tmp, 1);
	tries++;
	 usleep(1);
	ret = serial_read(fd, tmp, 4);
	if (modem==TRUE)
		{
            printf(" Modem Responded = %i with value of ",ret);
             if(ret>0){
            for(c=0; c<ret; c++){
                printf("%02X ", (uint8_t)tmp[c]);
            }
            printf("\n");
        }
            ret=4;
            strcpy(tmp,modes[current_mode]);  //just to pass test without a real hardware
            tmp[3]='1';
            tmp[4]='\0';

		}


	        switch(bbmode){
            case SPI:
                if ( (ret==4) && (strncmp(tmp, "SPI1", 4) == 0))
                      response=bbmode;
                else
                      response=ERR;
                break;
            case I2C:
                if ( (ret==4) && (strncmp(tmp, "I2C1", 4) == 0))
                      response=bbmode;
                else
                      response=ERR;  ;
                break;
            case ART:
                if ( (ret==4) && (strncmp(tmp, "ART1", 4) == 0))
                      response=bbmode;
                else
                      response=ERR;  ;
                break;
            case OWD:
                 if ( (ret==4) && (strncmp(tmp, "1W01", 4) == 0))
                       response=bbmode;
                 else
                       response=ERR;
                 break;
            case RAW:
               if ( (ret==4) && (strncmp(tmp, "RAW1", 4) == 0))
                     response=bbmode;
               else
                     response=ERR;
               break;
            case JTAG:
               if ( (ret==4) && (strncmp(tmp, "XSV1", 4) == 0))
                     response=bbmode;
               else
                     response=ERR;
               break;

            default:
                 fprintf(stderr, " Buspirate did not respond correctly :( %i \n", ret );
                 response=ERR;
             }


	     return response;    //response back whatever in bbmode means ok, else NULL if something is wrong

}




