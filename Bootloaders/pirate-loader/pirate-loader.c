/*

 Pirate-Loader for Bootloader v4

  Version  : 1.0.7

 Changelog:

  + 2026-10-21 - Accept the Sandbox Electric BPv3.b clone: its bootloader answers
                 0x13 instead of 'K' and reports device ID 0x02, so both the
                 handshake and the flash size detection failed on it.

  + 2025-08-15 - Add back support for BPv3, detected from smaller PIC devices

  + 2024-03-31 - Improve the error reporting and do not show the errors after
                 the successful actions like hello or flashing new firmware.
                 Compatible with bootloader 4.12 but works with 4.10 too.

  + 2016-08-22 - Migrated to CMake, minor fixes.

  + 2010-06-28 - Made HEX parser case-insensitive

  + 2010-02-04 - Changed polling interval to 10ms on Windows select wrapper, suggested by Michal (robots)

  + 2010-02-04 - Added sleep(0) between write instructions, patch submitted by kbulgrien

  + 2010-01-22 - Added loader version number to the console output and source code

  + 2010-01-19 - Fixed BigEndian incompatibility
  + 2010-01-19 - Added programming simulate switch ( --simulate ) for data verification

  + 2010-01-18 - Initial release


 Building:

  CMake is required for building this program.  CMake can be downloaded for
  free from www.cmake.org, or installed on your system via apt-get, yum,
  homebrew, ports, etc.

  UNIX family systems:
 
   cd Bootloaders/pirate-loader
  mkdir build
  cd build
  cmake ..
  make
 
  WINDOWS:
 
   cd Bootloaders/pirate-loader
  mkdir build
  cd build
  cmake ..
  start pirate-loader.sln
  (then build from Visual Studio)

 Usage:

	Run ./pirate-loader --help for more information on usage and possible switches

 */

#ifdef _MSC_VER
/* Use the ISO C stdio/string API rather than MSVC-only *_s variants. */
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>

/*
 * Type used to hold a serial port handle.  On POSIX systems this is a file
 * descriptor; on Windows it is a HANDLE, which is pointer-sized and must not
 * be truncated to an int on 64-bit builds.
 */
typedef intptr_t bp_handle_t;

#define PIRATE_LOADER_VERSION "1.0.7"

#define STR_EXPAND(tok) #tok
#define OS_NAME(tok) STR_EXPAND(tok)


#if defined(_WIN32)
#include <windows.h>
#include <time.h>
#include <stdint.h>

#define O_NOCTTY 0
#define O_NDELAY 0
#define B115200 115200

int serial_write(bp_handle_t fd, const void* buf, int len)
{
    HANDLE hCom = (HANDLE)fd;
    unsigned long bwritten = 0;

    if( WriteFile(hCom, buf, len, &bwritten, NULL) == FALSE )
    {
        return -1;
    }

    return (int)bwritten;
}

int serial_read(bp_handle_t fd, void* buf, int len)
{
    HANDLE hCom = (HANDLE)fd;
    unsigned long bread = 0;

    if( ReadFile(hCom, buf, len, &bread, NULL) == FALSE )
    {
        return -1;
    }

    return (int)bread;
}

int serial_close(bp_handle_t fd)
{
    CloseHandle((HANDLE)fd);
    return 0;
}

bp_handle_t serial_open(const char* path, unsigned long flags)
{
    static char full_path[32] = {0};

    HANDLE hCom = NULL;

    (void)flags;

    if( path[0] != '\\' )
    {
        if (snprintf(full_path, sizeof(full_path), "\\\\.\\%s", path) >= (int)sizeof(full_path))
        {
            errno = ENAMETOOLONG;
            return (bp_handle_t)-1;
        }
        path = full_path;
    }

    hCom = CreateFileA(path, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if( !hCom || hCom == INVALID_HANDLE_VALUE )
    {
        switch( GetLastError() )
        {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                errno = ENOENT;
                break;

            case ERROR_ACCESS_DENIED:
                errno = EACCES;
                break;

            default:
                errno = EINVAL;
                break;
        }

        return (bp_handle_t)-1;
    }

    return (bp_handle_t)hCom;
}

unsigned int serial_sleep(unsigned int sec)
{
    Sleep(sec * 1000);

    return 0;
}

/*
 * Monotonic millisecond clock.  GetTickCount() wraps after roughly 49 days;
 * the unsigned arithmetic below handles that correctly.
 */
uint32_t serial_millis(void)
{
    return (uint32_t)GetTickCount();
}

/*
 * Waits until at least one byte can be read from the port, or timeout_ms
 * elapses.  Returns 1 when data is available, 0 on timeout and -1 on error.
 */
int serial_wait_readable(bp_handle_t fd, int timeout_ms)
{
    DWORD start = GetTickCount();
    COMSTAT cs = {0};

    for(;;)
    {
        if( ClearCommError( (HANDLE)fd, NULL, &cs ) != TRUE )
        {
            return -1;
        }

        if( cs.cbInQue > 0 )
        {
            return 1;
        }

        if( (DWORD)(GetTickCount() - start) >= (DWORD)timeout_ms )
        {
            return 0;
        }

        Sleep(10);
    }
}

#else
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

#define serial_open  open
#define serial_close close
#define serial_read  read
#define serial_write write
#define serial_sleep sleep

/*
 * Monotonic millisecond clock.
 */
uint32_t serial_millis(void)
{
    struct timespec ts;

    if( clock_gettime(CLOCK_MONOTONIC, &ts) != 0 )
    {
        return 0;
    }

    return (uint32_t)((uint64_t)ts.tv_sec * 1000u +
                      (uint64_t)ts.tv_nsec / 1000000u);
}

/*
 * Waits until at least one byte can be read from the port, or timeout_ms
 * elapses.  Returns 1 when data is available, 0 on timeout and -1 on error.
 */
int serial_wait_readable(bp_handle_t fd, int timeout_ms)
{
    fd_set fds;
    struct timeval tv;

    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    return select(fd + 1, &fds, NULL, NULL, &tv);
}

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
#define BOOTLOADER_OK 'K'
#define BOOTLOADER_PROT 'P'
#define BOOTLOADER_CHECKSUM 'N'
#define BOOTLOADER_PLACEMENT 1

/* The Sandbox Electric BPv3.b clone answers with this status byte instead of
   the original bootloader's 'K'. */
#define BOOTLOADER_OK_CLONE 0x13

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

#define BPV3_FLASHSIZE 0XAC00
//#define flashsize 0x2AC00 //was 0xac00
//#define PIC_NUM_PAGES 512

//unsigned short pic_num_pages;
unsigned long flashsize = 0x2AC00;


/* global settings, command line arguments */

uint8		g_verbose = 0;
uint8		g_hello_only = 0;
uint8		g_simulate = 0;
const char* g_device_path  = NULL;
const char* g_hexfile_path = NULL;

/* functions */

int readWithTimeout(bp_handle_t fd, uint8* out, int length, int timeout)
{
    int res = -1;
    int got = 0;
    uint32_t deadline = serial_millis() + (uint32_t)(timeout * 1000);

    do
    {
        int32_t remaining = (int32_t)(deadline - serial_millis());

        if( remaining <= 0 )
        {
            break;
        }

        res = serial_wait_readable(fd, remaining);

        if( res > 0 )
        {
            res = serial_read(fd, out, length);
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

static int writeAll(bp_handle_t fd, const uint8 *data, int length)
{
    int sent = 0;
    uint32_t start = serial_millis();
    while (sent < length && (uint32_t)(serial_millis() - start) < 5000u)
    {
        int count = serial_write(fd, data + sent, length - sent);
        if (count <= 0) return -1;
        sent += count;
    }
    return sent == length ? sent : -1;
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
    uint8  linebin[260] = {0};
    uint8* data = (linebin + HEX_DATA_OFFSET);
    uint8  hex_crc, hex_type, hex_len;
    uint32 hex_addr;
    uint32 hex_base_addr = 0;
    uint32 hex_words     = 0;

    uint32 f_addr = 0;
    uint32 o_addr = 0;

    uint32 num_words = 0;

    char  line[524] = {0};
    char  *pc;
    char  *pline = line + 1;
    int   res = 0;
    int	  binlen = 0;
    int   line_no = 0;
    int   i = 0;
    uint8 seen_eof = 0;

    FILE* fp = fopen(file, "rb");

    if( !fp )
    {
        return -1;
    }

    while( !feof(fp) && fgets(line, sizeof(line) - 1, fp) )
    {
        line_no++;

        /* Tolerate blank lines... */
        if( line[0] == '\0' || line[0] == '\n' || line[0] == '\r' )
        {
            continue;
        }

        /*
         * ...but anything else that is not a record means the file is
         * malformed or truncated.  Silently stopping here would make the
         * loader flash a partial image, so report it instead.
         */
        if( line[0] != ':' )
        {
            fprintf(stderr, "Malformed HEX file, line %d: no record marker\n", line_no);
            goto invalid;
        }

        res = (int)strlen(pline);
        pc  = pline + res - 1;

        while( pc > pline && *pc <= ' ' )
        {
            *pc-- = 0;
            res--;
        }

        if( res & 0x01 || res > 520 || res < 10)
        {
            fprintf(stderr, "Incorrect number of characters on line %d:%d\n", line_no, res);
            goto invalid;
        }

        hex_crc = 0;

        for( pc = pline, i = 0; i<res; i+=2, pc+=2 )
        {
            if (!isxdigit((unsigned char)pc[0]) || !isxdigit((unsigned char)pc[1]))
            {
                fprintf(stderr, "Invalid HEX digit, line %d\n", line_no);
                goto invalid;
            }
            linebin[i >> 1] = hexdec(pc);
            hex_crc += linebin[i >> 1];
        }

        binlen = res / 2;

        if( hex_crc != 0 )
        {
            fprintf(stderr, "Checksum does not match, line %d\n", line_no);
            goto invalid;
        }

        hex_addr = (linebin[1] << 8) | linebin[2];
        hex_len  = linebin[0];
        hex_type = linebin[3];

        if( binlen - (1 + 2 + 1 + hex_len + 1) != 0 )
        {
            fprintf(stderr, "Incorrect number of bytes, line %d\n", line_no);
            goto invalid;
        }

        if( hex_type == 0x00 )
        {
            f_addr  = (hex_base_addr | (hex_addr)) / 2; //PCU

            if( hex_len % 4 || hex_addr % 4 )
            {
                fprintf(stderr, "Misaligned data, line %d\n", line_no);
                goto invalid;
            }
            else if( f_addr >= flashsize || hex_len / 2UL > flashsize - f_addr ||
                     ((f_addr / 2) * PIC_WORD_SIZE) > max_length ||
                     (hex_len / 4UL) * PIC_WORD_SIZE > max_length - ((f_addr / 2) * PIC_WORD_SIZE))
            {
                fprintf(stderr, "Current record address is higher than maximum allowed, line %d\n", line_no);
                goto invalid;
            }

            hex_words = hex_len  / 4;
            o_addr  = (f_addr / 2) * PIC_WORD_SIZE; //BYTES

            for( i=0; (uint32)i<hex_words; i++)
            {
                bout[o_addr + 0] = data[(i*4) + 2];
                bout[o_addr + 1] = data[(i*4) + 0];
                bout[o_addr + 2] = data[(i*4) + 1];

                pages_used[ (o_addr / PIC_PAGE_SIZE) ] = 1;

                o_addr    += PIC_WORD_SIZE;
                num_words ++;
            }

        }
        else if ( hex_type == 0x04 && hex_len == 2 && hex_addr == 0)
        {
            hex_base_addr = ((uint32)linebin[4] << 24) | ((uint32)linebin[5] << 16);
        }
        else if ( hex_type == 0x01 && hex_len == 0 && hex_addr == 0 )
        {
            seen_eof = 1;
            while (fgets(line, sizeof(line), fp))
            {
                for (pc = line; *pc; ++pc)
                    if (!isspace((unsigned char)*pc)) goto invalid;
            }
            break; //EOF
        }
        else
        {
            fprintf(stderr, "Unsupported record type %02x, line %d\n", hex_type, line_no);
            goto invalid;
        }

    }

    if (ferror(fp)) goto invalid;

    if( !seen_eof )
    {
        fprintf(stderr, "Malformed HEX file: no end-of-file record\n");
        goto invalid;
    }

    fclose(fp);
    return num_words;
invalid:
    fclose(fp);
    return -1;
}

uint8 makeCrc(uint8* buf, uint32 len)
{
    uint8 crc = 0;
    uint32 i;

    for(i=0; i<len; i++)
    {
        crc -= *buf++;
    }

    return crc;
}

/*
 * The bootloader answers BOOTLOADER_OK to a command it accepted; a clone
 * answers BOOTLOADER_OK_CLONE to the same commands, so accept both.
 */
uint8 bootloaderOk(uint8 status)
{
    return status == BOOTLOADER_OK || status == BOOTLOADER_OK_CLONE;
}

int sendCommandAndWaitForResponse(bp_handle_t fd, uint8 *command)
{
    uint8  response[4] = {0};
    int    res = 0;

    res = writeAll(fd, command, HEADER_LENGTH + command[LENGTH_OFFSET]);

    if( res <= 0 )
    {
        puts("ERROR");
        return -1;
    }

    res = readWithTimeout(fd, response, 1, 5);
    if( res != 1 )
    {
        puts("ERROR, timeout");
        return -1;
    }

    switch (response[0])
    {
        case BOOTLOADER_OK:
        case BOOTLOADER_OK_CLONE:
            return 0;
            break;

        case BOOTLOADER_PROT:
            printf("(SKIPPED by bootloader)...");
            return 0;
            break;

        case BOOTLOADER_CHECKSUM:
            printf("cmd checksum error...\n");
            return -1;
            break;

        default:
            printf("ERROR [0x%02x]\n", response[0]);
            return -1;
            break;
    }
}

int sendFirmware(bp_handle_t fd, uint8* data, uint8* pages_used)
{
    uint32 u_addr;

    uint32 page  = 0;
    uint32 done  = 0;
    uint32 row   = 0;
    uint8  command[256] = {0};

    /* Validate the entire image against the detected part before the first erase. */
    for (page = 0; page < PIC_NUM_PAGES; page++)
    {
        if (pages_used[page] && page * 1024UL >= flashsize)
        {
            fprintf(stderr, "Image exceeds detected device flash\n");
            return -1;
        }
    }
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
        command[0] = (uint8)((u_addr & 0x00FF0000) >> 16);
        command[1] = (uint8)((u_addr & 0x0000FF00) >>  8);
        command[2] = (uint8)((u_addr & 0x000000FF) >>  0);
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
            command[0] = (uint8)((u_addr & 0x00FF0000) >> 16);
            command[1] = (uint8)((u_addr & 0x0000FF00) >>  8);
            command[2] = (uint8)((u_addr & 0x000000FF) >>  0);
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

            serial_sleep(0);


            done += PIC_ROW_SIZE;
        }
    }

    return done;
}

void fixJumps(uint8* bin_buff, uint8* pages_used)
{
    uint32 iGotoUserAppAdress = 0;
    uint32 iGotoUserAppAdressB3 = 0, iIter = 0;
    uint32 iBLAddress = 0;
    
    iBLAddress = ( flashsize - (BOOTLOADER_PLACEMENT * PIC_NUM_ROWS_IN_PAGE * PIC_NUM_WORDS_IN_ROW * 2)); //PCU
    iGotoUserAppAdress = iBLAddress  - 4; 
    iGotoUserAppAdressB3 = (iGotoUserAppAdress / 2) * 3;
    
    for ( iIter = 0; iIter < 6; iIter++ ) {
    	bin_buff[ iGotoUserAppAdressB3 + iIter ] = bin_buff[ iIter ];
    }
    
    pages_used[ (iGotoUserAppAdressB3 / PIC_PAGE_SIZE) ] = 1;
    
    bin_buff[0] = 0x04;
    bin_buff[1] = (uint8)(( (iBLAddress & 0x0000FE) ));
    bin_buff[2] = (uint8)(( (iBLAddress & 0x00FF00) >> 8 ));
    bin_buff[3] = 0x00;	
    bin_buff[4] = (uint8)(( (iBLAddress & 0x7F0000) >> 16 ));
    bin_buff[5] = 0x00;
}

/* non-firmware functions */

int configurePort(bp_handle_t fd, unsigned long baudrate)
{
#if defined(_WIN32)
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

    {
        COMMTIMEOUTS timeouts = {MAXDWORD, 0, 0, 0, 2000};
        if (!SetCommTimeouts(hCom, &timeouts)) return -1;
    }

    return 0;
#else
    struct termios g_new_tio;

    memset(&g_new_tio, 0x00 , sizeof(g_new_tio));
    cfmakeraw(&g_new_tio);

    g_new_tio.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
    g_new_tio.c_cflag |=  (CS8 | CLOCAL | CREAD);
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

bp_handle_t openPort(const char* dev, unsigned long flags)
{
    return serial_open(dev, O_RDWR | O_NOCTTY | O_NDELAY | flags);
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
        puts(" ./pirate-loader --dev=/path/to/device --hex=/path/to/hexfile.hex [ --verbose ]");
        puts(" ./pirate-loader --simulate --hex=/path/to/hexfile.hex [ --verbose ] ");
        puts("");

        return 0;
    }

    return 1;
}

/* entry point */

int main (int argc, const char** argv)
{
    bp_handle_t dev_fd = -1;
    int		res = -1;
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

        bin_buff = (uint8*)malloc((flashsize / 2) * PIC_WORD_SIZE);
        if( !bin_buff )
        {
            fprintf(stderr, "Could not allocate 256kB buffer\n");
            goto Error;
        }

        //fill the buffer with 0xFF
        memset(bin_buff, 0xFF, (flashsize / 2) * PIC_WORD_SIZE);

        printf("Parsing HEX file [%s]\n", g_hexfile_path);

        res = readHEX(g_hexfile_path, bin_buff, (flashsize / 2) * PIC_WORD_SIZE, pages_used);
        if( res <= 0 || (unsigned long)res > flashsize )
        {
            fprintf(stderr, "Could not load HEX file, result=%d\n", res);
            goto Error;
        }

        printf("Found %d words (%d bytes)\n", res, res * 3);
    }

    if( g_simulate )
    {
        if (g_hello_only || sendFirmware(dev_fd, bin_buff, pages_used) < 0) goto Error;
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
        /*
         * Report what actually went wrong. These are the errors a serial port
         * really produces, and the everyday one is "in use" - a terminal window
         * or another copy of the loader still holding the port. The old text
         * said "unknown failure" for all of these and then claimed "file read
         * failed", which is meaningless when the hex file has already parsed.
         */
        switch( errno )
        {
            case EACCES:
            case EBUSY:
                printf("in use, or permission denied");
                break;

            case ENOENT:
            case ENODEV:
                printf("no such device");
                break;

            case EIO:
                printf("device did not respond");
                break;

            default:
                printf("%s", strerror(errno));
                break;
        }

        puts("");
        puts("ERROR");
        fprintf(stderr, "Could not open %s.\n", g_device_path);
        fprintf(stderr, "Check the device name, and make sure nothing else has it open -\n");
        fprintf(stderr, "a serial terminal still connected is the usual culprit.\n");
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
    res = writeAll(dev_fd, (const uint8 *)BOOTLOADER_HELLO_STR, 1);
    if (res != 1) goto Error;

    res = readWithTimeout(dev_fd, buffer, 4, 3);

    if( res != 4 || !bootloaderOk(buffer[3]) )
    {
        puts("ERROR");
        if( res == 4 )
        {
            fprintf(stderr, "The bootloader answered with an unexpected status: 0x%02x\n", buffer[3]);
        }
        else
        {
            fprintf(stderr, "No reply from the bootloader, or invalid reply received: %d\n", res);
        }
        fprintf(stderr, "Please make sure that PGD and PGC are connected, reconnect\n");
        fprintf(stderr, "the device and try again\n");
        goto Error;
    }
    puts("OK\n"); //extra LF for spacing

    printf("Bootloader version: %d,%02d\n", buffer[1], buffer[2]);

    printf("Device ID [0x%02x]:",buffer[0]);
    switch(buffer[0])
    {
    case 241:
        printf("PIC24FJ256GB106\n");
        flashsize = 0x2AC00;
        break;
    case 212:
        printf("PIC24FJ64GA002\n");
        flashsize = BPV3_FLASHSIZE;
        break;
    case 213:
    case 0x02:
        printf("PIC24FJ64GA004 (BPv3 clone)\n");
        flashsize = BPV3_FLASHSIZE;
        break;
    default:
        fprintf(stderr, "Unsupported Bus Pirate device ID 0x%02x\n", buffer[0]);
        goto Error;
    }

    if( !g_hello_only )
    {
        if(flashsize <= BPV3_FLASHSIZE) {
            printf("Fixing bootloader/userprogram jumps\n");
            fixJumps(bin_buff, pages_used);
        }
        res = sendFirmware(dev_fd, bin_buff, pages_used);

        if( res > 0 )
        {
            puts("\nFirmware updated successfully! :)");
            //printf("Use screen %s 115200 to verify\n", g_device_path);
        }
        else
        {
            puts("\nError updating firmware :(");
            goto Error;
        }
    }

    if(flashsize > BPV3_FLASHSIZE) {
        uint8  command[6] = {0};

        command[0] = (uint8)(1);	// fake data
        command[1] = (uint8)(2);
        command[2] = (uint8)(3);
        command[COMMAND_OFFSET] = 0xff;
        command[LENGTH_OFFSET ] = 0x01; // 1 byte, CRC
        command[PAYLOAD_OFFSET] = makeCrc(command, 5);

        /*
         * Ask the bootloader to restart. Fire and forget on purpose: some
         * bootloaders acknowledge this, and some - the one on this desk
         * included - simply act on it silently. Waiting for a reply therefore
         * reported "ERROR, timeout" immediately after a successful flash, which
         * reads as if the flash had failed. Say what actually happened instead.
         */
        if( writeAll(dev_fd, command, HEADER_LENGTH + command[LENGTH_OFFSET]) > 0 )
        {
            serial_sleep(1);
            printf("Asked the bootloader to restart.\n");
            printf("If the Bus Pirate does not come back as a serial device, power-cycle it\n");
            printf("(remove any bootloader jumper first).\n");
        }
        else
        {
            fprintf(stderr, "Note: could not ask the bootloader to restart.\n");
            fprintf(stderr, "Power-cycle the Bus Pirate to run the new firmware; the flash itself\n");
            fprintf(stderr, "completed as reported above.\n");
        }
    }

Finished:
    if( bin_buff )
    {
        free( bin_buff );
    }
    if( dev_fd >= 0 )
    {
        serial_close(dev_fd);
    }
    return 0;

Error:
    if( bin_buff )
    {
        free( bin_buff );
    }
    if( dev_fd >= 0 )
    {
        serial_close(dev_fd);
    }
    return -1;
}
