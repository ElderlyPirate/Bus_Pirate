/* Author: James Stephenson
 * This program runs under windows and uses the bus pirate to read/write a file in 
 * ascii hex to/from an I2C eeprom.
 * Page write isn't used since chips don't have a standard write buffer size.
 * Released under the WTF license.
 *
 * Defaults to devices such as the 24FC64, note the address used is 2 bytes.
 * Smaller devices such as the 24LC08 with 1 byte addresses require the 'Rs' or 'Ws' option
 *
 * Hex file should contain space separated hex values and no line breaks, ie: 
 * 0xFF 0xAA 0xBB or
 * FF AA BB
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

int main(int argc, char **argv){
	FILE *inoutpfile;
    DCB my_dcb;
	HANDLE hComm;
    int bytes_written, bytes_received, retval, memaddr = 0, failcount, datalen = 0;
    unsigned char receive_buf[10], write_buf[9], curcode[5], curbincode, chipaddr, flags = 0, cmdlen, addrtype = 'D';
    
    if(argc < 4 || argc > 7){
	    printf("i2ceepromwin [com?] [chip addr 0-7] [R/W/Rs/Ws] [file] (read len) (mem addr)\n");
		return 1;		
    }

	if(argv[2][0] > '7' || argv[2][0] < '0'){
		printf("Chip address out of range\n");
		return 2;
	}
	chipaddr = 0xA0 | ((argv[2][0] - '0') << 1);

	if(argc >= 6){
		if(!sscanf(argv[5], "%d", &datalen)){
			printf("Data length invalid\n");
			return 3;
		}
	}

	if(argc == 7){
		if(!sscanf(argv[6], "%d", &memaddr)){
			printf("Memory address invalid\n");
			return 3;
		}
	}

	if(argv[3][0] != 'R' && argv[3][0] != 'W'){
		printf("Must specify read or write mode\n");
		return 4;
	}
	
	if(argv[3][1] == 'S' || argv[3][1] == 's'){
			addrtype = 'S';
		}

	if(argv[3][0] == 'R' && !datalen){
		printf("Must specify read length\n");
		return 4;
	}

	if(argv[3][0] == 'R')
		inoutpfile = fopen(argv[4], "wb");
	else
		inoutpfile = fopen(argv[4], "rb");
	if(!inoutpfile){
		printf("Error opening file: %s\n", argv[4]);
		return 2;
	}
    
    hComm = CreateFile(argv[1], GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(hComm == INVALID_HANDLE_VALUE){
        printf("Error Opening Serial Port\n");
        return 3;
    }
    
	my_dcb.DCBlength = sizeof(my_dcb);
	GetCommState(hComm, &my_dcb);
	my_dcb.BaudRate = CBR_115200;
	my_dcb.ByteSize = 8;
	my_dcb.StopBits = ONESTOPBIT;
	my_dcb.Parity = NOPARITY;

	if(!SetCommState(hComm, &my_dcb)){
		printf("Error setting up serial port\n");
		return 4;
	}
	
	// Enter bitbang mode, then I2C mode.
	// Enable power supplies and pullup resistors
	// Check for correct version string
	WriteFile(hComm, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x02\x4C", 22, &bytes_written, 0);
	ReadFile(hComm, receive_buf, 10, &bytes_received, 0);
	receive_buf[9] = 0;
	if(memcmp(receive_buf, "BBIO1I2C1", 9)){
		printf("Error setting up RAW I2C mode: %s\n", receive_buf);
		return 5;
	}
	sleep(500);

	// Read data
	if(argv[3][0] == 'R'){
		if(addrtype == 'S')
			cmdlen = 5;
		else
			cmdlen = 6;
		
		// Set the write pointer.  First the start bit...
		write_buf[0] = '\x02';
		if(cmdlen == 6){
			// For double address byte devices, three data bytes (chip write address, data address)
			write_buf[1] = '\x12';
			write_buf[2] = chipaddr;
			write_buf[3] = memaddr >> 8;
			write_buf[4] = memaddr & 0xFF;
		}
		else{
			// For single address byte devices, two data bytes (chip write address/page address, data address)
			write_buf[1] = '\x11';
			write_buf[2] = chipaddr | ((memaddr >> 7) & 0xFE);
			write_buf[3] = memaddr & 0xFF;
		}
		// ...and end with the stop bit
		write_buf[cmdlen-1] = '\x03';

		WriteFile(hComm, write_buf, cmdlen, &bytes_written, 0);
		ReadFile(hComm, receive_buf, cmdlen, &bytes_received, 0);
		
		// Start the read
		// Send start bit, one data byte (chip read address)
		memcpy(write_buf, "\x02\x10\xAA", 3);
		if(cmdlen == 6)
			write_buf[2] = chipaddr | 1;
		else
			write_buf[2] = chipaddr | 1 | (memaddr >> 8);
		
		WriteFile(hComm, write_buf, 3, &bytes_written, 0);
		ReadFile(hComm, receive_buf, 3, &bytes_received, 0);
		
		// Read the data
		for(; datalen; datalen--){
			// Send read command, ACK bit
			WriteFile(hComm, "\x04\x06", 2, &bytes_written, 0);
			ReadFile(hComm, receive_buf, 2, &bytes_received, 0);
			printf("%02X ", receive_buf[0]);
			fprintf(inoutpfile, "%02X ", receive_buf[0]);
		}
		
		// Finish the read
		// Send read command, NACK bit, stop bit.
		// Then exit to bitbang mode and reset the buspirate
		WriteFile(hComm, "\x04\x07\x03\x00\x0F", 5, &bytes_written, 0);
	}
	
	// Write data
	else{
		failcount = 0;
		
		if(addrtype == 'S')
			cmdlen = 6;
		else
			cmdlen = 7;
		
		// Write a byte
		// Send start bit...
		write_buf[0] = '\x02';
		
		if(cmdlen == 7){
			// For double address byte devices, four data bytes
			// (chip write address, data address, data)
			write_buf[1] = '\x13';
			write_buf[2] = chipaddr;
		}
		else{
			// For single address byte devices, three data bytes
			// (chip write address/page address, data address, data)
			write_buf[1] = '\x12';
		}

		// ...Stop bit
		write_buf[cmdlen-1] = '\x03';
		
		while(fscanf(inoutpfile, "%4s", curcode) == 1){
			if(!sscanf(curcode, "%X", &curbincode)){
				printf("Error processing code in file: %s", curcode);
				return 6;
			}
			
			// Update the address and data
			if(cmdlen == 7){
				write_buf[3] = memaddr >> 8;
			}
			else{
				write_buf[2] = chipaddr | ((memaddr >> 7) & 0xFE);
			}	
			write_buf[cmdlen-3] = memaddr & 0xFF;
			write_buf[cmdlen-2] = curbincode;
			
			// Send the write command, and check that it was successful
			// On a write failure, repeat
			do{
				failcount++;
		    	WriteFile(hComm, write_buf, cmdlen, &bytes_written, 0);
				ReadFile(hComm, receive_buf, cmdlen, &bytes_received, 0);
			}while(receive_buf[2] || receive_buf[3] || receive_buf[4] || (addrtype == 'S'?0:receive_buf[5]));
			printf("%02X ", curbincode);
			failcount--;
			memaddr++;
			datalen++;
		}
		printf("\n%d bytes written with %d retries\n", datalen, failcount);
		
		// Exit to bitbang mode and reset the buspirate
		WriteFile(hComm, "\x00\x0F", 2, &bytes_written, 0);
	}
	
	fclose(inoutpfile);
    return 0;
}
