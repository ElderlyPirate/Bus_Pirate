/*

 Pirate-Loader for Bootloader v4

 Version  : 1.0.2

 Changelog:
 +2010-06-28 - Made HEX parser case-insensative

  + 2010-02-04 - Changed polling interval to 10ms on Windows select wrapper, suggested by Michal (robots)

  + 2010-02-04 - Added sleep(0) between write instructions, patch submitted by kbulgrien

  + 2010-01-22 - Added loader version number to the console output and source code

  + 2010-01-19 - Fixed BigEndian incompatibility
			   - Added programming simulate switch ( --simulate ) for data verification

  + 2010-01-18 - Initial release


 Building:

  UNIX family systems:

	gcc pirate-loader.c -o pirate-loader

  WINDOWS:

	cl pirate-loader.c /DWIN32=1


 Usage:

	Run ./pirate-loader --help for more information on usage and possible switches

 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <fcntl.h>
#include <errno.h>

#define PIRATE_LOADER_VERSION "1.0.2"

#define STR_EXPAND(tok) #tok
#define OS_NAME(tok) STR_EXPAND(tok)


#ifdef WIN32
#include <windows.h>
#include <time.h>

#define O_NOCTTY 0
#define O_NDELAY 0
#define B115200 921600

#define OS WINDOWS

int write(int fd, const void* buf, int len)
{
    HANDLE hCom = (HANDLE)fd;
    int res = 0;
    unsigned long bwritten = 0;


    res = WriteFile(hCom, buf, len, &bwritten, NULL);

    if( res == FALSE )
    {
        return -1;
    }
    else
    {
        return bwritten;
    }
}

int read(int fd, void* buf, int len)
{
    HANDLE hCom = (HANDLE)fd;
    int res = 0;
    unsigned long bread = 0;

    res = ReadFile(hCom, buf, len, &bread, NULL);

    if( res == FALSE )
    {
        return -1;
    }
    else
    {
        return bread;
    }
}

int close(int fd)
{
    HANDLE hCom = (HANDLE)fd;

    CloseHandle(hCom);
    return 0;
}

int open(const char* path, unsigned long flags)
{
    static char full_path[32] = {0};

    HANDLE hCom = NULL;

    if( path[0] != '\\' )
    {
        _snprintf(full_path, sizeof(full_path) - 1, "\\\\.\\%s", path);
        path = full_path;
    }

    hCom = CreateFileA(path, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if( !hCom || hCom == INVALID_HANDLE_VALUE )
    {
        return -1;
    }
    else
    {
        return (int)hCom;
    }
}

int __stdcall select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfs, const struct timeval* timeout)
{
    time_t maxtc = time(0) + (timeout->tv_sec);
    COMSTAT cs = {0};
// JTR not used so get rig of warning    unsigned long dwErrors = 0;

    if( readfds->fd_count != 1 )
    {
        return -1;
    }

    while( time(0) <= maxtc )
    {
        //only one file supported
        if( ClearCommError( (HANDLE)readfds->fd_array[0], 0, &cs) != TRUE )
        {
            return -1;
        }

        if( cs.cbInQue > 0 )
        {
            return 1;
        }

        Sleep(10);
    }
    return 0;
}

unsigned int sleep(unsigned int sec)
{
    Sleep(sec * 1000);

    return 0;
}

#else
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#endif

/* macro definitions */

/* type definitions */

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32;

#if !defined OS
#define OS UNKNOWN
#endif

#define BOOTLOADER_HELLO_STR "\xC1"
#define BOOTLOADER_OK 0x4B
#define BOOTLOADER_PROT 'P'
#define PIC_WORD_SIZE  (3)
#define PIC_NUM_ROWS_IN_PAGE  8
#define PIC_NUM_WORDS_IN_ROW 64
#define PIC_ROW_SIZE  (PIC_NUM_WORDS_IN_ROW * PIC_WORD_SIZE)
#define PIC_PAGE_SIZE (PIC_NUM_ROWS_IN_PAGE  * PIC_ROW_SIZE)
#define PIC_ROW_ADDR(p,r)		(((p) * PIC_PAGE_SIZE) + ((r) * PIC_ROW_SIZE))
#define PIC_WORD_ADDR(p,r,w)	(PIC_ROW_ADDR(p,r) + ((w) * PIC_WORD_SIZE))
#define PIC_PAGE_ADDR(p)		(PIC_PAGE_SIZE * (p))

#define PAYLOAD_OFFSET 5
#define HEADER_LENGTH PAYLOAD_OFFSET
#define LENGTH_OFFSET 4
#define COMMAND_OFFSET 3
#define IS_24FJ 1
#define PIC_NUM_PAGES 512

//#define flashsize 0x2AC00 //was 0xac00
//#define PIC_NUM_PAGES 512

//unsigned short pic_num_pages;
unsigned short family = IS_24FJ;
unsigned long flashsize = 0x2AC00;
unsigned short eesizeb  = 0;
unsigned long blstartaddr = 0x400L; // not currently used
unsigned long blendaddr = 0x23FFL; // not currently used


/* global settings, command line arguments */

uint8		g_verbose = 0;
uint8		g_hello_only = 0;
uint8		g_simulate = 0;
const char* g_device_path  = NULL;
const char* g_hexfile_path = NULL;

/* functions */

int readWithTimeout(int fd, uint8* out, int length, int timeout)
{
    fd_set fds;
    struct timeval tv = {timeout, 0};
    int res = -1;
    int got = 0;

    do
    {

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        res = select(fd + 1, &fds, NULL, NULL, &tv);

        if( res > 0 )
        {
            res = read(fd, out, length);
            if( res > 0 )
            {
                length -= res;
                got    += res;
                out    += res;
            }
            else
            {
                break;
            }
        }
        else
        {
            return res;
        }
    }
    while( length > 0);

    return got;
}

unsigned char hexdec(const char* pc)
{
    unsigned char temp;

    if(pc[0]>='a')
    {
        temp=pc[0]-'a'+10;
    }
    else if(pc[0] >= 'A')
    {
        temp=pc[0]-'A'+10;
    }
    else
    {
        temp=pc[0] - '0';
    }
    temp=temp<<4;

    if(pc[1]>='a')
    {
        temp|=pc[1]-'a'+10;
    }
    else if(pc[1] >= 'A')
    {
        temp|=pc[1]-'A'+10;
    }
    else
    {
        temp|=pc[1] - '0';
    }

    return(temp & 0x0FF);


    //return (((pc[0] >= 'A') ? ( pc[0] - 'A' + 10 ) : ( pc[0] - '0' ) ) << 4 |
    //		((pc[1] >= 'A') ? ( pc[1] - 'A' + 10 ) : ( pc[1] - '0' ) )) & 0x0FF;

}

void dumpHex(uint8* buf, uint32 len)
{
    uint32 i=0;

    for(i=0; i<len; i++)
    {
        printf("%02X ", buf[i]);
    }
    putchar('\n');
}

int readHEX(const char* file, uint8* bout, unsigned long max_length, uint8* pages_used)
{
    static const uint32 HEX_DATA_OFFSET = 4;
    uint8  linebin[256] = {0};
    uint8* data = (linebin + HEX_DATA_OFFSET);
    uint8  hex_crc, hex_type, hex_len;
    uint32 hex_addr;
    uint32 hex_base_addr = 0;
    uint32 hex_words     = 0;

    uint32 f_addr = 0;
    uint32 o_addr = 0;

    uint32 num_words = 0;

    char  line[512] = {0};
    char  *pc;
    char  *pline = line + 1;
    int   res = 0;
    int	  binlen = 0;
    int   line_no = 0;
    int   i = 0;

    FILE* fp = fopen(file, "rb");

    if( !fp )
    {
        return -1;
    }

    while( !feof(fp) && fgets(line, sizeof(line) - 1, fp) )
    {
        line_no++;

        if( line[0] != ':' )
        {
            break;
        }

        res = strlen(pline);
        pc  = pline + res - 1;

        while( pc > pline && *pc <= ' ' )
        {
            *pc-- = 0;
            res--;
        }

        if( res & 0x01 || res > 512 || res < 10)
        {
            fprintf(stderr, "Incorrect number of characters on line %d:%d\n", line_no, res);
            return -1;
        }

        hex_crc = 0;

        for( pc = pline, i = 0; i<res; i+=2, pc+=2 )
        {
            linebin[i >> 1] = hexdec(pc);
            hex_crc += linebin[i >> 1];
        }

        binlen = res / 2;

        if( hex_crc != 0 )
        {
            fprintf(stderr, "Checksum does not match, line %d\n", line_no);
            return -1;
        }

        hex_addr = (linebin[1] << 8) | linebin[2];
        hex_len  = linebin[0];
        hex_type = linebin[3];

        if( binlen - (1 + 2 + 1 + hex_len + 1) != 0 )
        {
            fprintf(stderr, "Incorrect number of bytes, line %d\n", line_no);
            return -1;
        }

        if( hex_type == 0x00 )
        {
            f_addr  = (hex_base_addr | (hex_addr)) / 2; //PCU

            if( hex_len % 4 )
            {
                fprintf(stderr, "Misaligned data, line %d\n", line_no);
                return -1;
            }
            else if( f_addr >= flashsize )
            {
                fprintf(stderr, "Current record address is higher than maximum allowed, line %d\n", line_no);
                return -1;
            }

            hex_words = hex_len  / 4;
            o_addr  = (f_addr / 2) * PIC_WORD_SIZE; //BYTES

            for( i=0; i<hex_words; i++)
            {
                bout[o_addr + 0] = data[(i*4) + 2];
                bout[o_addr + 1] = data[(i*4) + 0];
                bout[o_addr + 2] = data[(i*4) + 1];

                pages_used[ (o_addr / PIC_PAGE_SIZE) ] = 1;

                o_addr    += PIC_WORD_SIZE;
                num_words ++;
            }

        }
        else if ( hex_type == 0x04 && hex_len == 2)
        {
            hex_base_addr = (linebin[4] << 24) | (linebin[5] << 16);
        }
        else if ( hex_type == 0x01 )
        {
            break; //EOF
        }
        else
        {
            fprintf(stderr, "Unsupported record type %02x, line %d\n", hex_type, line_no);
            return -1;
        }

    }

    fclose(fp);
    return num_words;
}

uint8 makeCrc(uint8* buf, uint32 len)
{
    uint8 crc = 0, i = 0;

    for(i=0; i<len; i++)
    {
        crc -= *buf++;
    }

    return crc;
}

int sendCommandAndWaitForResponse(int fd, uint8 *command)
{
    uint8  response[4] = {0};
    int    res = 0;

    res = write(fd, command, HEADER_LENGTH + command[LENGTH_OFFSET]);

    if( res <= 0 )
    {
        puts("ERROR");
        return -1;
    }

    res = readWithTimeout(fd, response, 1, 5);
    if( res != 1 )
    {
        puts("ERROR");
        return -1;
    }
    else if (response[0]== BOOTLOADER_PROT)
    {
        printf("(SKIPPED by bootloader)...");
        return 0;

    }
    else if ( response[0] != BOOTLOADER_OK )
    {
        printf("ERROR [%02x]\n", response[0]);
        return -1;
    }
    else
    {
        return 0;
    }
}


int sendFirmware(int fd, uint8* data, uint8* pages_used)
{
    uint32 u_addr;


    uint32 page  = 0;
    uint32 done  = 0;
    uint32 row   = 0;
    uint8  command[256] = {0};


    for( page=0; page<PIC_NUM_PAGES; page++)
    {

        u_addr = page * ( PIC_NUM_WORDS_IN_ROW * 2 * PIC_NUM_ROWS_IN_PAGE );

        if( pages_used[page] != 1 )
        {
            if( g_verbose && u_addr < flashsize)
            {
                fprintf(stdout, "Skipping page %ld [ %06lx ], not used\n", page, u_addr);
            }
            continue;
        }

        if( u_addr >= flashsize )
        {
            fprintf(stderr, "Address out of flash\n");
            return -1;
        }

        //erase page
        command[0] = (u_addr & 0x00FF0000) >> 16;
        command[1] = (u_addr & 0x0000FF00) >>  8;
        command[2] = (u_addr & 0x000000FF) >>  0;
        command[COMMAND_OFFSET] = 0x01; //erase command
        command[LENGTH_OFFSET ] = 0x01; //1 byte, CRC
        command[PAYLOAD_OFFSET] = makeCrc(command, 5);

        if( g_verbose )
        {
            dumpHex(command, HEADER_LENGTH + command[LENGTH_OFFSET]);
        }

        printf("Erasing page %ld, %04lx...", page, u_addr);

        if( g_simulate == 0 && sendCommandAndWaitForResponse(fd, command) < 0 )
        {
            return -1;
        }

        puts("OK");

        //write 8 rows
        for( row = 0; row < PIC_NUM_ROWS_IN_PAGE; row ++, u_addr += (PIC_NUM_WORDS_IN_ROW * 2))
        {
            command[0] = (u_addr & 0x00FF0000) >> 16;
            command[1] = (u_addr & 0x0000FF00) >>  8;
            command[2] = (u_addr & 0x000000FF) >>  0;
            command[COMMAND_OFFSET] = 0x02; //write command
            command[LENGTH_OFFSET ] = PIC_ROW_SIZE + 0x01; //DATA_LENGTH + CRC

            memcpy(&command[PAYLOAD_OFFSET], &data[PIC_ROW_ADDR(page, row)], PIC_ROW_SIZE);

            command[PAYLOAD_OFFSET + PIC_ROW_SIZE] = makeCrc(command, HEADER_LENGTH + PIC_ROW_SIZE);

            printf("Writing page %ld row %ld, %04lx...", page, row + page*PIC_NUM_ROWS_IN_PAGE, u_addr);

            if( g_verbose )
            {
                dumpHex(command, HEADER_LENGTH + command[LENGTH_OFFSET]);
            }

            if( g_simulate == 0 && sendCommandAndWaitForResponse(fd, command) < 0 )
            {
                return -1;
            }

            puts("OK");

            sleep(0);


            done += PIC_ROW_SIZE;
        }
    }

    return done;
}


/* non-firmware functions */

int configurePort(int fd, unsigned long baudrate)
{
#ifdef WIN32
    DCB dcb = {0};
    HANDLE hCom = (HANDLE)fd;

    dcb.DCBlength = sizeof(dcb);

    dcb.BaudRate = baudrate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    if( !SetCommState(hCom, &dcb) )
    {
        return -1;
    }

    return (int)hCom;
#else
    struct termios g_new_tio;

    memset(&g_new_tio, 0x00 , sizeof(g_new_tio));
    cfmakeraw(&g_new_tio);

    g_new_tio.c_cflag |=  (CS8 | CLOCAL | CREAD);
    g_new_tio.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
    g_new_tio.c_oflag = 0;
    g_new_tio.c_lflag = 0;


    g_new_tio.c_cc[VTIME] = 0;
    g_new_tio.c_cc[VMIN] = 1;

    cfsetispeed (&g_new_tio, baudrate);
    cfsetospeed (&g_new_tio, baudrate);

    tcflush(fd, TCIOFLUSH);

    return tcsetattr(fd, TCSANOW, &g_new_tio);
#endif
}

int openPort(const char* dev, unsigned long flags)
{
    return open(dev, O_RDWR | O_NOCTTY | O_NDELAY | flags);
}

int parseCommandLine(int argc, const char** argv)
{
    int i = 0;

    for(i=1; i<argc; i++)
    {

        if( !strncmp(argv[i], "--hex=", 6) )
        {
            g_hexfile_path = argv[i] + 6;
        }
        else if ( !strncmp(argv[i], "--dev=", 6) )
        {
            g_device_path = argv[i] + 6;
        }
        else if ( !strcmp(argv[i], "--verbose") )
        {
            g_verbose = 1;
        }
        else if ( !strcmp(argv[i], "--hello") )
        {
            g_hello_only = 1;
        }
        else if ( !strcmp(argv[i], "--simulate") )
        {
            g_simulate = 1;
        }
        else if ( !strcmp(argv[i], "--help") )
        {
            argc = 1; //that's not pretty, but it works :)
            break;
        }
        else
        {
            fprintf(stderr, "Unknown parameter %s, please use pirate-loader --help for usage\n", argv[i]);
            return -1;
        }
    }

    if( argc == 1 )
    {
        //print usage
        puts("pirate-loader usage:\n");
        puts(" ./pirate-loader --dev=/path/to/device --hello");
        puts(" ./pirate-loader --dev=/path/to/device --hex=/path/to/hexfile.hex [ --verbose");
        puts(" ./pirate-loader --simulate --hex=/path/to/hexfile.hex [ --verbose");
        puts("");

        return 0;
    }

    return 1;
}

/* entry point */

int main (int argc, const char** argv)
{
    int		dev_fd = -1, res = -1;
    uint8	buffer[256] = {0};
    uint8	pages_used[PIC_NUM_PAGES] = {0};
    uint8*	bin_buff = NULL;


    puts("+++++++++++++++++++++++++++++++++++++++++++");
    puts("  Pirate-Loader for BP with Bootloader v4+  ");
    puts("  Loader version: " PIRATE_LOADER_VERSION "  OS: " OS_NAME(OS));
    puts("+++++++++++++++++++++++++++++++++++++++++++\n");

    if( (res = parseCommandLine(argc, argv)) < 0 )
    {
        return -1;
    }
    else if( res == 0 )
    {
        return 0;
    }

    if( !g_hello_only )
    {

        if( !g_hexfile_path )
        {
            fprintf(stderr, "Please specify hexfile path --hex=/path/to/hexfile.hex\n");
            return -1;
        }

        bin_buff = (uint8*)malloc(0xFFFFFF * sizeof(uint8)); //256kB
        if( !bin_buff )
        {
            fprintf(stderr, "Could not allocate 256kB buffer\n");
            goto Error;
        }

        //fill the buffer with 0xFF
        memset(bin_buff, 0xFFFFFFFF, (0xFFFFFF * sizeof(uint8)));

        printf("Parsing HEX file [%s]\n", g_hexfile_path);

        res = readHEX(g_hexfile_path, bin_buff, (0xFFFFFF * sizeof(uint8)), pages_used);
        if( res <= 0 || res > flashsize )
        {
            fprintf(stderr, "Could not load HEX file, result=%d\n", res);
            goto Error;
        }

        printf("Found %d words (%d bytes)\n", res, res * 3);

        //printf("Fixing bootloader/userprogram jumps\n");
        //fixJumps(bin_buff, pages_used);
    }

    if( g_simulate )
    {
        sendFirmware(dev_fd, bin_buff, pages_used);
        goto Finished;
    }

    if( !g_device_path )
    {
        fprintf(stderr, "Please specify serial device path --dev=/dev/...\n");
        return -1;
    }

    printf("Opening serial device %s...", g_device_path);

    dev_fd = openPort(g_device_path, 0);

    if( dev_fd < 0 )
    {
        puts("ERROR");
        fprintf(stderr, "Could not open %s\n", g_device_path);
        goto Error;
    }
    puts("OK");

    printf("Configuring serial port settings...");

    if( configurePort(dev_fd, B115200) < 0 )
    {
        puts("ERROR");
        fprintf(stderr, "Could not configure device, errno=%d\n", errno);
        goto Error;
    }
    puts("OK");

    printf("Sending Hello to the Bootloader...");

    //send HELLO
    res = write(dev_fd, BOOTLOADER_HELLO_STR, 1);

    res = readWithTimeout(dev_fd, buffer, 4, 3);

    if( res != 4 || buffer[3] != BOOTLOADER_OK )
    {
        puts("ERROR");
        fprintf(stderr, "No reply from the bootloader, or invalid reply received: %d\n", res);
        fprintf(stderr, "Please make sure that PGND and PGC are connected, replug the devide and try again\n");
        goto Error;
    }
    puts("OK\n"); //extra LF for spacing

    printf("Bootloader version: %d,%02d\n", buffer[1], buffer[2]);

    printf("Device ID [%02x]:",buffer[0]);
    switch(buffer[0])
    {
//    case 0xd4:
//        printf("PIC24FJ64GA002\n");
//        break;
    case  9:
        printf("PIC24FJ128GB206\n");
        //    __PIC24FJ128GB206__
        family = IS_24FJ;
        flashsize = 0x15800;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 17:
        printf("PIC24FJ128GB210\n");
//    __PIC24FJ128GB210__
        family = IS_24FJ;
        flashsize = 0x15800;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;

        break;
    case 18:
        printf("PIC24FJ256GB206\n");
        //    __PIC24FJ256GB206__
        family = IS_24FJ;
        flashsize = 0x2AC00;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 19:
        printf("PIC24FJ256GB210\n");
        //    __PIC24FJ256GB210__
        family = IS_24FJ;
        flashsize = 0x2AC00;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 191:
        printf("PIC24FJ256DA206\n");
        //    __PIC24FJ256DA206__
        family = IS_24FJ;
        flashsize = 0x2AC00;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 192:
        printf("PIC24FJ256DA210\n");
        //    __PIC24FJ256DA210__
        family = IS_24FJ;
        flashsize = 0x2AC00;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 217:
        printf("PIC24FJ64GB106\n");
        //    __PIC24FJ64GB106__
        family = IS_24FJ;
        flashsize = 0xAC00;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 218:
        printf("PIC24FJ64GB108\n");
        //    __PIC24FJ64GB108__
        family = IS_24FJ;
        flashsize = 0xAC00;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 219:
        printf("PIC24FJ64GB110\n");
        //    __PIC24FJ64GB110__
        family = IS_24FJ;
        flashsize = 0xAC00;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 229:
        printf("PIC24FJ128GB106\n");
        //    __PIC24FJ128GB106__
        family = IS_24FJ;
        flashsize = 0x15800;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 230:
        printf("PIC24FJ128GB108\n");
        //    __PIC24FJ128GB108__
        family = IS_24FJ;
        flashsize = 0x15800;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 231:
        printf("PIC24FJ128GB110\n");
        //    __PIC24FJ128GB110__
        family = IS_24FJ;
        flashsize = 0x15800;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 235:
        printf("PIC24FJ192GB106\n");
        //    __PIC24FJ192GB106__
        family = IS_24FJ;
        flashsize = 0x20C00;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 236:
        printf("PIC24FJ192GB108\n");
        //    __PIC24FJ192GB108__
        family = IS_24FJ;
        flashsize = 0x20C00;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 237:
        printf("PIC24FJ192GB110\n");
        //    __PIC24FJ192GB110__
        family = IS_24FJ;
        flashsize = 0x20C00;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 241:
        printf("PIC24FJ256GB106\n");
        //    __PIC24FJ256GB106__
#define IS_24FJ                 1
        flashsize = 0x2AC00;
        eesizeb  =         0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 242:
        printf("PIC24FJ256GB108\n");
        //    __PIC24FJ256GB108__
        family = IS_24FJ;
        flashsize = 0x2AC00;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 243:
        printf("PIC24FJ256GB110\n");
        //    __PIC24FJ256GB110__
        family = IS_24FJ;
        flashsize = 0x2AC00;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 244:
        printf("PIC24FJ32GB002\n");
        //    __PIC24FJ32GB002__
        family = IS_24FJ;
        flashsize = 0x5800;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 245:
        printf("PIC24FJ32GB004\n");
        //    __PIC24FJ32GB004__
        family = IS_24FJ;
        flashsize = 0x5800;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 246:
        printf("PIC24FJ64GB002\n");
        //    __PIC24FJ64GB002__
        family = IS_24FJ;
        flashsize = 0xAC00;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 247:
        printf("PIC24FJ64GB004\n");
        //    __PIC24FJ64GB004__
        family = IS_24FJ;
        flashsize = 0xAC00;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 250:
        printf("PIC24FJ128DA106\n");//    __PIC24FJ128DA106__
        family = IS_24FJ;
        flashsize = 0x15800;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 251:
        printf("PIC24FJ128DA110\n");
        //    __PIC24FJ128DA110__
        family = IS_24FJ;
        flashsize = 0x15800;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 252:
        printf("PIC24FJ128DA206\n");
        //    __PIC24FJ128DA206__
        family = IS_24FJ;
        flashsize = 0x15800;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 253:
        printf("PIC24FJ128DA210\n");
        //    __PIC24FJ128DA210__
        family = IS_24FJ;
        flashsize = 0x15800;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 254:
        printf("PIC24FJ256DA106\n");
        //    __PIC24FJ256DA106__
        family = IS_24FJ;
        flashsize = 0x2AC00;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case 255:
        printf("PIC24FJ256DA110\n");
        //    __PIC24FJ256DA110__
        family = IS_24FJ;
        flashsize = 0x2AC00;
        eesizeb  = 0;
        blstartaddr = 0x400L;
        blendaddr = 0x23FFL;
        break;
    case	206:
        printf("PIC24FJ16GA002\n");
        flashsize = 	0x2C00;

        break;

    case	207:
        printf("PIC24FJ16GA004\n");
        flashsize = 	0x2C00;

        break;


    case	208:
        printf("PIC24FJ32GA002\n");
        flashsize = 	0x5800;

        break;

    case	209:
        printf("PIC24FJ32GA004\n");
        flashsize = 	0x5800;

        break;

    case	210:
        printf("PIC24FJ48GA002\n");
        flashsize = 	0x8400;

        break;

    case	211:
        printf("PIC24FJ48GA004\n");
        flashsize = 	0x8400;

        break;


    case	212:
        printf("PIC24FJ64GA002\n");
        flashsize = 	0xAC00;

        break;

    case	213:
        printf("PIC24FJ64GA004\n");
        flashsize = 	0xAC00;

        break;

    case	214:
        printf("PIC24FJ64GA006\n");
        flashsize = 	0xAC00;

        break;

        printf("PIC24FJ64GA008\n");
        flashsize = 	0xAC00;
    case	215:
        break;

    case	216:
        printf("PIC24FJ64GA010\n");
        flashsize = 	0xAC00;

        break;

    case	220:
            printf("PIC24FJ96GA006\n");
        flashsize = 	0x10000;

        break;

    case	221:
        printf("PIC24FJ96GA008\n");
        flashsize = 	0x10000;

        break;

    case	222:
        printf("PIC24FJ96GA010\n");
        flashsize = 	0x10000;

        break;


    case	223:
        printf("PIC24FJ128GA006\n");
        flashsize = 	0x15800;

        break;

    case	224:
        printf("PIC24FJ128GA008\n");
        flashsize = 	0x15800;

        break;

    case	225:
            printf("PIC24FJ128GA010\n");
        flashsize = 	0x15800;

        break;

    case	226:
        printf("PIC24FJ128GA106\n");
        flashsize = 	0x15800;

        break;

    case	227:
        printf("PIC24FJ128GA108\n");
        flashsize = 	0x15800;
        break;

    case	228:
        printf("PIC24FJ128GA110\n");
        flashsize = 	0x15800;
        break;

    case	232:
        printf("PIC24FJ192GA106\n");
        flashsize = 	0x20C00;
        break;

    case	233:
        printf("PIC24FJ192GA108\n");
        flashsize = 	0x20C00;
        break;

    case	234:
        printf("PIC24FJ192GA110\n");
        flashsize = 	0x20C00;
        break;

    case	238:
        printf("PIC24FJ256GA106\n");
        flashsize = 	0x2AC00;

        break;

    case	239:
        printf("PIC24FJ256GA108\n");
        flashsize = 	0x2AC00;
        break;

    case	240:
        printf("PIC24FJ256GA110\n");
        flashsize = 	0x2AC00;
        break;


    default:
        printf("UNKNOWN");
        fprintf(stderr, "Unsupported device (%02x:UNKNOWN)\n", buffer[0]);
        goto Error;
        break;
    }

    if( !g_hello_only )
    {

        res = sendFirmware(dev_fd, bin_buff, pages_used);

        if( res > 0 )
        {
            puts("\nFirmware updated successfully :)!");
            //printf("Use screen %s 115200 to verify\n", g_device_path);
        }
        else
        {
            puts("\nError updating firmware :(");
            goto Error;
        }

    }

Finished:
    if( bin_buff )
    {
        free( bin_buff );
    }
    if( dev_fd >= 0 )
    {
        close(dev_fd);
    }
    return 0;

Error:
    if( bin_buff )
    {
        free( bin_buff );
    }
    if( dev_fd >= 0 )
    {
        close(dev_fd);
    }
    return -1;
}
