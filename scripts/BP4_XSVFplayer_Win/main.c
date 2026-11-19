#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef WIN32
#include <conio.h>
 #include <windef.h>

#else
//#include <curses.h>
#endif

#include "serial.h"
#include "buspirate.h"


#define  JTAG_RESET        0x01
#define  JTAG_CHAIN_SCAN   0x02
#define  XSVF_PLAYER       0x03
#define  CHAIN_SCAN_MAX_TRIES 5


#define XSVF_ERROR_NONE            0x00
#define XSVF_ERROR_UNKNOWN         0x01
#define XSVF_ERROR_TDOMISMATCH     0x02
#define XSVF_ERROR_MAXRETRIES      0x03
#define XSVF_ERROR_ILLEGALCMD      0x04
#define XSVF_ERROR_ILLEGALSTATE    0x05
#define XSVF_ERROR_DATAOVERFLOW    0x06
#define XSVF_ERROR_LAST            0x07
#define XSVF_READY_FOR_DATA        0xFF

//#ifndef WIN32
#define usleep(x) Sleep(x);
//#define Sleep(x) usleep(x);
#define TRUE 1
#define FALSE 0
//#endif

char *dumpfile;
#ifdef WIN32
HANDLE dumphandle;
#endif

int modem =FALSE;
int cnt=0;
uint8_t *bin_buf;
uint32_t bin_buf_size;
#define FREE(x) if(x) free(x);
#define MAX_BUFFER 4096
//#define MAX_BUFFER 255  //255 bytes

//http://www.whereisian.com/files/j-xsvf_002.swf

int print_usage(char * appname)
{
		//print usage
		printf("\n");
		printf("\n");
	    printf(" Help Menu\n");
        printf(" Usage:              \n");
		printf("   %s  -p device -f filename.xsvf -s speed [-x] [-r] \n ",appname);
		printf("\n");
		printf("   Example Usage:   %s -p COM1 -s 115200 -f example.xsvf  \n",appname);
		printf("\n");
		printf("           Where: -p device is port e.g.  COM1  \n");
		printf("                  -s Speed is port Speed  default is 115200 \n");
		printf("                  -f Filename of XSVF file \n");
		printf("                  -x Perform a JTAG Chain Scan by sending 0x02 command. -f is optional. \n");
		printf("                  -r Perform a JTAG Reset  Scan by sending 0x01 command. -f is optional. \n");
		printf(" Exit status: 0 = success; 1 = player result error; 2 = host/transport error.\n");
		printf("\n");

        printf("-----------------------------------------------------------------------------\n");


		return 0;
}

/* The scan count is a byte count, not a device count. Consume its entire
 * payload before 0x03 so IDCODE bytes cannot become player result codes. */
static int read_serial_exact(int fd, uint8_t *buffer, int length)
{
    int received = 0, tries = 0;
    while (received < length) {
        int res = serial_read(fd, (char *)buffer + received, length - received);
        if (res <= 0) {
            if (++tries >= CHAIN_SCAN_MAX_TRIES)
                return 0;
        } else {
            if (res > length - received)
                return 0;
            received += res;
            tries = 0;
        }
    }
    return 1;
}

static int do_chain_scan(int fd)
{
    uint8_t count, buffer[255];
    char command = JTAG_CHAIN_SCAN;
    int c;
    printf(" Performing Chain Scan..\n");
    if (serial_write(fd, &command, 1) != 1) {
        printf(" Chain scan: failed to send the command\n");
        return 2;
    }
    if (!read_serial_exact(fd, &count, 1) ||
        !read_serial_exact(fd, buffer, count)) {
        printf(" Chain scan: incomplete reply (bounded timeout)\n");
        return 2;
    }
    printf(" Chain Scan Result: %02X", count);
    for (c = 0; c < count; ++c)
        printf(" %02X", buffer[c]);
    printf("\n\n");
    return 0;
}

int main(int argc, char** argv)
{
	int opt;
	uint8_t buffer[MAX_BUFFER]={0};
	uint8_t temp[2]={0};  // command buffer
//	struct stat stbuf;
	int fd,timeout_counter;
	int res,c, nparam_bytechunks, bytePointer, readSize;
	long fileSize;
	FILE *XSVF;
//	int  xsvf;
    int timer_out=0;
	char *param_port = NULL;
	char *param_speed = NULL;
	char *param_XSVF=NULL;
	char *param_bytechunks=NULL;
	int  jtag_reset=FALSE;
    int  chainscan=FALSE;
    int status=0;

    const char *XSVF_ERROR[]={  "XSVF_ERROR_NONE",
                                "XSVF_ERROR_UNKNOWN",
                                "XSVF_ERROR_TDOMISMATCH",
                                "XSVF_ERROR_MAXRETRIES",
                                "XSVF_ERROR_ILLEGALCMD",
                                "XSVF_ERROR_ILLEGALSTATE",
                                "XSVF_ERROR_DATAOVERFLOW",
                                "XSVF_ERROR_LAST",
                                "XSVF_READY_FOR_DATA",
                                 0 };

	printf("-----------------------------------------------------------------------------\n");
	printf("\n");
	printf(" BusPirate XSVF Player V.02\n");
	printf(" http://www.dangerousprototypes.com\n");
	printf("\n");
	printf("-----------------------------------------------------------------------------\n");



	if (argc <= 1)  {
		print_usage(argv[0]);
		return 2;
	}


	while ((opt = getopt(argc, argv, "s:p:f:rx")) != -1) {

		switch (opt) {
			case 'p':  // device   eg. com1 com12 etc
				if ( param_port != NULL){
					printf("Device/PORT error!\n");
					return 2;
				}
				param_port = strdup(optarg);
                break;
            case 'r':
                jtag_reset=TRUE;
                break;
            case 'x':
                chainscan=TRUE;
            	break;
			case 'f':
				if (param_XSVF != NULL) {
					printf(" No XSVF file \n");
					return 2;
				}
				param_XSVF = strdup(optarg);

				break;

			case 's':
				if (param_speed != NULL) {
					printf(" Speed should be set: eg  115200 \n");
					return 2;
				}
				param_speed = strdup(optarg);

				break;

			default:
				printf(" Invalid argument %c", opt);
				print_usage(argv[0]);
				return 2;
				break;
		}
	}

	if (param_port==NULL){
		printf(" No serial port specified\n");
		print_usage(argv[0]);
		return 2;
	}

    nparam_bytechunks=MAX_BUFFER;

	if (param_speed==NULL) {
		param_speed=strdup("115200");  //default is 115200kbps
	}


	fd = serial_open(param_port);
	if (fd < 0) {
		fprintf(stderr, " Error opening serial port\n");
		return 2;
	}

	//setup port and speed
	if (serial_setup(fd, (speed_t)atoi(param_speed)) != 0) {
        fprintf(stderr, " Error configuring serial port\n");
        serial_close(fd);
        return 2;
    }
	
//--- Added JM
    res = BP_EnableBinary(fd); // Activate Binary mode (Sends 20 OxOO)
    if (res == ERR){
       printf (" Unable to activate BP4 Binary mode \n");
       return 2;
       };
    res = BP_EnableMode(fd,0x18); // Activate XSVF Player Sub-Mode
    if (res == ERR){
       printf (" Unable to activate XSVF player mode \n");
       return 2;
       };
    
//--- End added JM

	if (jtag_reset==TRUE){
        printf(" Performing Reset..\n");
        temp[0]=0x01;
        if (serial_write(fd, (char *)temp, 1) != 1) {
            printf(" Reset: failed to send the command\n");
            serial_close(fd);
            return 2;
        }
        Sleep(1);
        printf(" Done \n\n");
	}

	// perform Chain scan
	 // Send 0x02 to perform a chain scan
     // Wait for 1 byte, the number of bytes that will be returned
     // Get that many bytes
	if (chainscan==TRUE) {
        status = do_chain_scan(fd);
        if (status != 0)
            return status;
    }
//---	}

   if (param_XSVF !=NULL) {
		//open the XSVF file
            XSVF = fopen(param_XSVF, "rb");
            if (XSVF == NULL) {
                printf(" Error opening file\n");
                serial_close(fd);
                return 2;
            }
            if (fseek(XSVF, 0, SEEK_END) != 0 ||
                (fileSize = ftell(XSVF)) <= 0 ||
                fseek(XSVF, 0, SEEK_SET) != 0) {
                printf(" Error sizing nonempty input file\n");
                fclose(XSVF);
                serial_close(fd);
                return 2;
            }
            printf(" File is %ld bytes, ",fileSize);

            bin_buf = (uint8_t*)malloc(fileSize);
            if (bin_buf == NULL) {
               printf(" Error allocating %ld bytes of memory\n", (long)fileSize);
               fclose(XSVF);
               serial_close(fd);
               return 2;
            }
            memset(bin_buf, 0, fileSize);

            if (fread(bin_buf, sizeof(uint8_t), fileSize, XSVF) != (size_t)fileSize) {
                printf(" Error reading file %s \n", param_XSVF);
                fclose(XSVF);
                free(bin_buf);
                serial_close(fd);
                return 2;
            }
            printf(" Read %ld bytes\n",fileSize);

            fclose(XSVF);


	} else {
        /* Scan/reset are standalone operations, as advertised by -x/-r. */
        serial_close(fd);
        if (!chainscan && !jtag_reset) {
            printf(" No file specified. Need an input xsvf file \n");
            return 2;
        }
        return status;
	}
	printf(" Opening Bus Pirate on %s at %sbps, using XSVF file %s \n", param_port, param_speed,param_XSVF);

	// Enter XSVF Player Mode
	//Open the port and send 0x03 to enter XSVF player mode
	printf(" Entering XSVF Player Mode\n");
	temp[0]=0x03;
	if (serial_write(fd, (char *)temp, 1) != 1) {
        printf(" Failed to send player start\n");
        status=2;
        goto cleanup;
    }

	// Wait for 0xFF, if <0xFF then it is finished or error codes (see below)
    bytePointer=0; //where we are in the byte buffer array
    readSize=MAX_BUFFER; //hack this to be a full chunk
    cnt=0;
    printf(" Waiting for first data request...");
	while(1) {

            //wait for reply before sending the next chunks
			timeout_counter=0;
			timer_out=0;   // 0 if ok, else -1 if exit
			while(1) {
				res= serial_read(fd, (char *)buffer, 1);
				if(res>0){
                    printf("ok\n");
				  // wait for 0xFF and send data, or error
					if ((buffer[0]!=XSVF_READY_FOR_DATA) || (fileSize==0)) {
					    c=buffer[0];
					    if (c==0xFF)
					         c=8;
                        else if (c > XSVF_ERROR_LAST)
                            c=XSVF_ERROR_UNKNOWN;
                        if (buffer[0] != XSVF_ERROR_NONE &&
                            buffer[0] != XSVF_READY_FOR_DATA)
                            status=1;
                        printf(" End of operation reply: %s (0x%02X) \n",XSVF_ERROR[c],buffer[0]);
                        switch (buffer[0]) {
                            case  XSVF_ERROR_NONE :
                                printf(" Success!\n");
                                break;
                            case XSVF_ERROR_UNKNOWN:
                             printf(" Unknown error: XSVF_ERROR_UNKNOWN \n");
                                break;
                            case XSVF_ERROR_TDOMISMATCH:
                             printf(" Device did not respond as expected: XSVF_ERROR_TDOMISMATCH \n");
                                break;
                            case XSVF_ERROR_MAXRETRIES:
                             printf(" Device did not respond: XSVF_ERROR_MAXRETRIES \n");
                                break;
                            case XSVF_ERROR_ILLEGALCMD :
                             printf(" Unknown XSVF command: XSVF_ERROR_ILLEGALCMD \n");
                                break;
                            case XSVF_ERROR_ILLEGALSTATE:
                             printf(" Unknown JTAG state: XSVF_ERROR_ILLEGALSTATE \n");
                                break;
                            case XSVF_ERROR_DATAOVERFLOW :
                             printf(" Error, data overflow: XSVF_ERROR_DATAOVERFLOW \n");
                                break;
                            case XSVF_ERROR_LAST:
                             printf(" Some other error I don't remember, probably isn't active: XSVF_ERROR_LAST \n");
                                break;
                             case XSVF_READY_FOR_DATA:
                                 if (fileSize==0) {
                                     printf(" End of file reached. \n");
                                     status=2; /* More input requested, not completion. */
                                } else {
									printf(" Programmer says more data: XSVF_READY_FOR_DATA \n");
                                }
                                break;
                            default:
                             printf(" Unknown error\n ");

                         }

                     }
                      if (buffer[0] != XSVF_READY_FOR_DATA)
                          timer_out=-1; /* A result terminates the run before any write. */
                      break;
				}else{
					printf("\n Waiting for reply...");
				//	Sleep(1);
					timeout_counter++;
					if(timeout_counter > 4){
						printf("\n No reply.... Quitting.\n ");
						timeout_counter=0;
						timer_out=-1;
						status=2;
						break;
					}
				}
			}
            if (fileSize==0) {
			       break;
			}

            if (timer_out==-1)
                break;
            //send data
            if(fileSize<MAX_BUFFER){
                readSize=fileSize;
            }
			//send to bp
			temp[0]=(readSize>>8);
			temp[1]=readSize;
			cnt=cnt+readSize;

			printf(" Sending %i Bytes (%04X)...",readSize, cnt);
			if (serial_write(fd, (char *)temp, 2) != 2 ||
                serial_write(fd, (char *)&bin_buf[bytePointer], readSize) != readSize) {
                printf(" Failed to send XSVF data\n");
                status=2;
                break;
            }
			bytePointer=bytePointer+readSize;//start 1 chunk in next itme
			fileSize=fileSize-readSize; //deincrement the remaining byte count

	}

cleanup:
    printf(" Thank you for playing! :-)\n\n");
#ifdef WIN32
	FREE(param_port);
 	FREE(param_speed);
    FREE(param_bytechunks);
    FREE(param_XSVF);
    FREE( bin_buf);
#endif
    serial_close(fd);
    return status;
 }  //end main()
