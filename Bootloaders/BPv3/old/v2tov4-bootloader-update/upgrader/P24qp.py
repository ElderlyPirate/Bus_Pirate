#!/usr/bin/python
### Bus Pirate Programmer (should work for other PIC24F devices as well)
### (C) 2009 Joseph Jezak <josejx@gentoo.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

import serial
import sys
import os
import getopt
import math
import ConfigParser

### Prints the usage screen
def Usage():
	print "Bus Pirate Programmer v1.0"
	print "\nP24qp <command>"
	print "\t-a --auto=FILE\t\t- Erase, Program and Quit the Bootloader"
	print "\t-c --config=FILE\t- P24qp compatible configuration file"
	print "\t\t\t\t  (Default: P24qp.ini)"
	print "\t-e --erase\t\t- Erase the flash programming"
	print "\t-f --finalize\t- Finalize the programming and go to user mode"
	print "\t-i --info\t\t- Information about the bootloader"
	print "\t-q --quiet\t\t- Quiet messages about writing progress"
	print "\t-r --read=FILE\t\t- Read programming from flash"
	print "\t-s --serial=DEVICE\t- Override serial device path"
	print "\t-t --reset\t\t- Reset the bootloader"
	print "\t-v \t\t\t- Enable verification of write commands"
	print "\t-w --write=FILE\t- Write programming file to flash"

### Get the other command line options
try:
	optlist, args = getopt.getopt(sys.argv[1:], "fhveitqs:w:r:a:", ["auto=", "config=", "help","write=","read=", "serial=", "erase", "info", "quiet", "reset", "finalize"])
except getopt.GetoptError, err:
	print str(err)
	Usage()
	sys.exit(2)

### Figure out what the user wants to do
Command = None
File = None
Verify = False
SerialPort = None
Quiet = False
### Default config file
Config = "P24qp.ini"
for op, val in optlist:
	if op in ("-h", "--help"):
		Usage()
		sys.exit()
	elif op in ("-w", "--write"):
		Command = "WT_FLASH"
		File = val
	elif op in ("-r", "--read"):
		Command = "RD_FLASH"
		File = val
	elif op in ("-e", "--erase"):
		Command = "ER_FLASH"
	elif op in ("-f", "--finalize"):
		Command = "VERIFY_OK"
	elif op in ("-i", "--info"):
		Command = "RD_VER"
	elif op in ("-a", "--auto"):
		Command = "Auto"	
		File = val
	elif op in ("-t", "--reset"):
		Command = "Reset"
	elif op in ("-v"):
		Verify = True
	elif op in ("-c", "--config"):
		Config = val
	elif op in ("-s", "--serial"):
		SerialPort = val
	elif op in ("-q", "--quiet"):
		Quiet = True

### If there's no command, just print the options
if Command == None:
	Usage()
	sys.exit(-1)
	
### Note: Only supports Intel 32 Hex format
class HEX_File:
	def __init__(self):
		self.data = {}
		self.encoded = None
	
	### Clear the stored data in the file
	def clear(self):
		self.data = {}

	### Read the encoded file
	def read(self, filename):
		FILE = open(filename, 'r')
		self.encoded = FILE.readlines()
		FILE.close()
		self.decode()

	### Write the encoded file
	def write(self, filename):
		self.encode()
		FILE = open(filename, 'w')
		for line in self.encoded:
			FILE.write(line)
		FILE.close()

	### Decode the encoded file into data
	def decode(self):
		### Bail out if there isn't an encoded file
		if self.encoded == None:
			print "No encoded file yet"
			return
		line = 0
		### Current high address
		addrh = 0
		### Current low address (for concatenation)
		addrl = -1
		### Decode each line
		for r in self.encoded:
			r = r.rstrip('\x0D\x0A')
			### Every line starts with ":"
			if r[0] != ":":
				continue
			### Get the byte count, address and record type
			bc = int(r[1:3], 16)
			### Get the address
			addr = int(r[3:7], 16)
			### Get the record type
			rt = int(r[7:9], 16)
			### Decide if it's time to move to the next key
			if rt == 0:
				if addrl == -1 or addr != addrl + len(self.data[(addrh << 16) | addrl]):
					addrl = addr
					self.data[(addrh << 16) | addrl] = []
			### Get the checksum
			ck = int(r[-2:], 16)
			### Compute the checksum, make sure it matches
			chksum = 0
			for c in range(0, len(r[1:-2])/2):
				chksum += int(r[c * 2 + 1: c * 2 + 3], 16)
			chksum = (~(chksum & 0xFF) + 1) & 0xFF
			if ck != chksum:
				print "Checksum doesn't match on line " + str(line) +  "!" 
				break

			### Figure out the record type
			### Data Record
			if rt == 0:
				for d in range(0, bc):
					t = int(r[d * 2 + 9: d * 2 + 11], 16)
					self.data[(addrh<< 16) | addrl].append(t)
			### EOF Record
			elif rt == 1:
				break

			### Extended Segment Address Record
			elif rt == 2:
				print "Record type 2 Not supported!"
				break
			
			### Start Segment Address Record
			elif rt == 3:
				print "Record type 3 Not supported!"
				break
			 
			### Extended Linear Address Record
			elif rt == 4:
				addrh = int(r[9:13], 16)
				addrl = -1
			### Start Linear Address Record
			elif rt == 5:
				print "Record type 5 Not supported!"
				break

			line += 1

	### Encode the data
	def encode(self):
		if len(self.data.keys()) == 0:
			print "No data to encode yet"
			return
			
		self.encoded = []
		line = 0
		### Encode each line
		data_k = self.data.keys()
		data_k.sort()
		last_addru = -1
		### Loop over the address segments
		for base_addr in data_k:
			### Add the data lines (16 per line)
			for line in range(0, len(self.data[base_addr]) / 16):
				### Find the upper address
				addru = (((base_addr + (line * 16)) >> 16) & 0xFF)
				### Print the address record if needed
				if addru != last_addru:
					enc = ":02000004%04X" % addru
					### Make the checksum
					chksum = 0
					for c in range(0, len(enc[1:])/2):
						chksum += int(enc[c * 2 + 1: c * 2 + 3], 16)
					chksum = (~(chksum & 0xFF) + 1) & 0xFF
					enc += "%02X" % chksum
					self.encoded.append(enc+"\x0D\x0A")
					last_addru = addru
				### Every line starts with ":", followed by record size (always 16(?)), address, then record type
				enc = ":%02X%04X%02X" % (16, (base_addr + (line * 16)) & 0xFFFF, 0)
				### Add the data
				for d in self.data[base_addr][line * 16:(line+1) * 16]:
					enc += "%02X" % ord(d)
				### Compute the checksum
				chksum = 0
				for c in range(0, len(enc[1:])/2):
					chksum += int(enc[c * 2 + 1: c * 2 + 3], 16)
				chksum = (~(chksum & 0xFF) + 1) & 0xFF
				enc = enc + "%02X" % chksum
				self.encoded.append(enc+"\x0D\x0A")

		### Add the end record
		enc = ":00000001FF\x0D\x0A"
		self.encoded.append(enc)
	
	
class PIC24F_Prog:
	def __init__(self, Config, SerialPort):
		self.device = {}
		self.serial = {}
		
		### Assume that the size of the read block is 4 until we know better
		self.device['readblock'] = 4
		### Assume that there are 2 bytes per address until we know better
		self.device['bytesperaddr'] = 2

		### Load the configuration
		if os.path.isfile(Config):
			cfg = ConfigParser.SafeConfigParser()		
			cfg.read(Config)
			if not cfg.has_section("PIC24FBOOT"):
				print "Invalid configuration, no PIC24FBOOT section!"
				sys.exit(-1)

			### Get the serial settings
			rates = [ 0, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200 ]
			rate_index = cfg.getint('PIC24FBOOT', 'bitrateindex')
			self.serial['rate'] = rates[rate_index]
			self.serial['timeout'] = cfg.getint('PIC24FBOOT', 'commtimeout')
			self.serial['retries'] = cfg.getint('PIC24FBOOT', 'maxretry')
			self.serial['maxpacket'] = cfg.getint('PIC24FBOOT', 'maxpacketsize') 
			### If the serial port isn't forced, use the config
			if SerialPort == None:
				self.serial['port'] = cfg.getint('PIC24FBOOT', 'portindex')
			else:
				self.serial['port'] = SerialPort

			### Get the location of the bootloader
			self.device['bootaddrlo'] = int(cfg.get('PIC24FBOOT', 'bootaddrlo')[1:-1], 16)
			self.device['bootaddrhi'] = int(cfg.get('PIC24FBOOT', 'bootaddrhi')[1:-1], 16)

		### Otherwise, error out
		else:
			print "Please provide a configuration file!"
			Usage()
			sys.exit(-1)

		### State variables
		self.last_cmd = None
		self.verify = False
		self.hexfile = HEX_File()
		self.quiet = False
		### Configuration words
		self.config_words = None
		
		### Set up the serial connection
		print "Using Serial Port", self.serial['port'], "@", self.serial['rate']
		try:
			self.ser = serial.Serial(self.serial['port'], self.serial['rate'], xonxoff=0, rtscts=0, timeout=(self.serial['timeout']/1000))
		except:
			print "Can't open serial port", self.serial['port'], "!"
			sys.exit(-1)

		### Get the device type to set the device settings from the config
		device = self.GetDevice()	

		if not cfg.has_section('DEVICELIST'):
			print "Invalid configuration, no DEVICELIST section!"
			sys.exit(-1)

		if not cfg.has_option('DEVICELIST', str(device)):
			print "Device with ID:", device, "not listed in the config file!"
			sys.exit(-1)

		device_name = cfg.get('DEVICELIST', str(device))[1:-1]
		
		if not cfg.has_section(device_name):
			print device_name, "not specified in the config file!"
			sys.exit(-1)

		print "Found", device_name

		### Get the locations of the boot vectors
		self.device['userresetvector'] = int(cfg.get(device_name, 'userresetvector')[1:-1], 16)
		self.device['bootdelay'] = int(cfg.get(device_name, 'bootdelay')[1:-1], 16)

		### Fill in the device specific settings
		self.device['writeblock'] = cfg.getint(device_name, 'writeblock')
		self.device['readblock'] = cfg.getint(device_name, 'readblock')
		self.device['eraseblock'] = cfg.getint(device_name, 'eraseblock')

		### Some devices specify larger packet sizes
		self.device['maxpacket'] = cfg.getint(device_name, 'maxpacketsize')
		self.device['bytesperaddr'] = cfg.getint(device_name, 'bytesperaddr')

		### Program memory area addresses
		self.device['pmrangelow'] = int(cfg.get(device_name, 'pmrangelow')[1:-1], 16)
		self.device['pmrangehigh'] = int(cfg.get(device_name, 'pmrangehigh')[1:-1], 16)
		
		### Vector addresses
		self.device['userresetvector'] = int(cfg.get(device_name, 'userresetvector')[1:-1], 16)
		self.device['bootdelay'] = int(cfg.get(device_name, 'bootdelay')[1:-1], 16)
		
	### Get the device type
	def GetDevice(self):
		device_data = self.ReadFlash(0xFF0000, 4)
		if len(device_data) < 2:
			print "Can't connect to device, sorry!"
			sys.exit(-1)
		device = ord(device_data[0]) | ord(device_data[1]) << 8
		return device

	def TX_PKT(self, data): 
		### PKT Wire format
		STX = "\x55"
		ETX = "\x04"
		DLE = "\x05"

		### Sanity check the data
		if len(data) > self.serial['maxpacket']:
			print "Warning: truncated packet"
			data = data[0:self.serial['maxpacket']]
	
		### Generate the checksum
		chksum = 0
		for d in data:
			chksum += ord(d)
		chksum = chr((~(chksum) + 1) & 0xFF)

		### Preamble
		self.ser.write(STX)
		self.ser.write(STX)
		for d in data:
			### Add escape character for special characters
			if d == STX or d == ETX or d == DLE:
				self.ser.write(DLE)
			self.ser.write(d)
		if chksum == STX or chksum == ETX or chksum == DLE:
			self.ser.write(DLE)
		self.ser.write(chksum)
		### Postfix
		self.ser.write(ETX)

	### Read a byte (with retries) from the serial device
	def ReadSer(self):
		db = ""
		for i in range(0, self.serial['retries']):
			db = self.ser.read()
			if len(db) != 0:
				break
		return db
	
	def RX_PKT(self):
		### PKT Wire format
		ETX = "\x04"
		DLE = "\x05"

		data = ""
		data_len = 0
		### Get the data packet
		while True:
			db = self.ReadSer()

			### Throw an error if we didn't get data
			if len(db) == 0:
				print "RX Error: ", len(data), " bytes received"
				if len(data) > 0:
					for d in data:
						print ord(d),
					print "\n"
				return data

			### Always add the escaped character
			if db == DLE:
				db = self.ReadSer()
				
			### End condition
			elif db == ETX:
				break

			### Normal data
			data += db
			data_len += 1
			
		### Get the checksum byte
		ck = ord(data[-1])
		### Remove the framing characters
		data = data[2:-1]
		### Check the checksum of the data segment
		chksum = 0
		for d in data:
			chksum += ord(d)
		chksum = (~(chksum & 0xFF) + 1) & 0xFF

		### Print a warning if the checksum fails
		if chksum != ck:
			print "\nWARNING: Checksum mismatch: %02X / %02X!" % (chksum, ck)
			for d in data:
				print ord(d),	
			print "\n"
		return data

	### Generate the packet to send over the wire and send it
	def BootloaderCMD(self, cmd, args = {}):
		repeat = False
		replicate = False
		data = []

		### Repeat and replicate use the last command
		if cmd == "REPEAT":
			if self.last_cmd != None:
				cmd = self.last_cmd				
				repeat = True
			else:
				print "Can't repeat, no command in buffer"
		if cmd == "REPLICATE":
			if self.last_cmd != None and self.last_cmd[0:2] == "WT":
				cmd = self.last_cmd				
				replicate = True
			else:
				print "Can't replicate, no compatible command in buffer"
		
		### Process the rest of the commands
		if cmd == "RESET":
			data = ["\x00", "\x00"]
			self.TX_PKT(data)
			### No response...

		### Read Version Information
		elif cmd == "RD_VER":
			print "Reading Bootloader Version Information: "
			### Send the data packet
			data = ["\x00", "\x02"]
			self.TX_PKT(data)
			### Get the result
			data = self.RX_PKT()
			if len(data) == 0:
				print "\tUnable to determine version!"
				return
			print "\tBootloader Version: " + str(ord(data[3])) + "." + str(ord(data[2]))

		### Read the programming out of the flash
		elif cmd == "RD_FLASH":
			#print "Reading Program Memory:"
			### Send the data packet
			if repeat:
				#print "\tReading from last location"
				data = ["\x01"]
			else:
				#print "\tReading 0x%02X words from 0x%08X" % (args['LEN'], (args['ADDRU'] << 16 | args['ADDRH'] << 8 | args['ADDRL']))
				data = ["\x01", chr(args['LEN']), chr(args['ADDRL']), chr(args['ADDRH']), chr(args['ADDRU'])]
			self.TX_PKT(data)
			### Get the result
			data = self.RX_PKT()
			### If there was an error, just store junk
			if len(data) == 0:
				print "\tError reading from device!"
				data = ""
				for i in range(0, args['LEN']):
					data += "\xFF\xFF\xFF\x00"
				return data	
			### Return only the data read from flash, not the address preamble
			return data[5:]

		### Write a new program into the flash
		elif cmd == "WT_FLASH":
			#print "Write Program Memory:"
			### Generate the data packet
			if repeat:
				#print "\tWriting to last location"
				data = ["\x02"]
			else:
				#print "\tWriting row to 0x%08X" % ((args['ADDRU'] << 16 | args['ADDRH'] << 8 | args['ADDRL']))
				data = ["\x02", chr(args['LEN']), chr(args['ADDRL']), chr(args['ADDRH']), chr(args['ADDRU'])]
				### On a replication, we don't provide the data
				if not replicate:
					### Generate the data string
					for d in args['DATA']:
						data.append(chr(d))
			### Send the data packet
			self.TX_PKT(data)
			### Get the response
			data = self.RX_PKT()
			if len(data) == 0 or data[0] != "\x02":
				print "\tError writing to device!"

		### Erase Flash
		elif cmd == "ER_FLASH":
			print "Erase Flash:"
			### Send the data packet
			if repeat:
				print "\tErasing previous location"
				data = ["\x03"]
			else:
				print "\tErasing %i pages, starting at 0x%08X" % (args['LEN'], (args['ADDRU'] << 16 | args['ADDRH'] << 8 | args['ADDRL']))
				data = ["\x03", chr(args['LEN']), chr(args['ADDRL']), chr(args['ADDRH']), chr(args['ADDRU'])]
			self.TX_PKT(data)
			### Get the response
			data = self.RX_PKT()
			if len(data) == 0 or data[0] != "\x03":
				print "\tError erasing device!"
			else:
				print "\tErase complete"

		### These commands are unimplemented at this time since we don't
		### use them with the BP. If needed, implement the same as the
		### flash read/write with the correct addressing and instructions
		elif cmd == "RD_EEDATA":
			print "Unsupported at this time!"
			pass
		elif cmd == "WT_EEDATA":
			print "Unsupported at this time!"
			pass
		elif cmd == "RD_CONFIG":
			print "Unsupported at this time!"
			pass
		elif cmd == "WT_CONFIG":
			print "Unsupported at this time!"
			pass

		### Send the packet that tells the device to start the watchdog
		### to restart into userspace mode
		elif cmd == "VERIFY_OK":
			print "Verified Okay."
			### Send the data packet
			data = ["\x08", "\x01", "\x00", "\x00", "\x00"]
			self.TX_PKT(data)
			### Get the response
			data = self.RX_PKT()
			if data[0] != "\x08":
				print "\tError verifying device!"
			else:
				print "\tVerification complete, switching to user mode"
				### Send a reset packet
				data = ["\x00", "\x00"]
				self.TX_PKT(data)

		### Back up the last command for repeating
		self.last_cmd = cmd
		return

	### Convienence functions for saving and loading files
	def ReadFile(self, filename):
		self.hexfile.read(filename)	
	def WriteFile(self, filename):
		self.hexfile.write(filename)	
	def DataToFile(self, addr, data):
		self.hexfile.data[addr] = data

	### Write to the flash
	def WriteFlash(self, start_address, data):
		### Get the word count
		bc = len(data)
		dots = ""

		for p in range(0, int(bc / self.device['writeblock'])):
			### Build the argument tree
			args = {}
			### Length is always 1 since we can write only one block at a time
			args['LEN'] = 1 
			### Now write in the data
			args['DATA'] = data[p * self.device['writeblock']:(p + 1) * self.device['writeblock']]
			### Finally, generate the address
			addr_p = start_address + p * (self.device['writeblock'] / self.device['bytesperaddr'])
			if addr_p >= self.device['pmrangehigh']:
				return
			args['ADDRL'] = addr_p & 0xFF
			args['ADDRH'] = (addr_p >> 8) & 0xFF
			args['ADDRU'] = (addr_p >> 16) & 0xFF
			if not self.quiet:
				print "Writing %i bytes to address 0x%08X" % (self.device['writeblock'], addr_p)

			self.BootloaderCMD("WT_FLASH", args)

			### If we're verifying as we write, read the row we just wrote and compare
			if self.verify:
				if addr_p in range(self.device['bootaddrlo'], self.device['bootaddrhi']):
					print "Skipping verification of bootloader area."
				else:
					### Read the flash we just wrote
					verify_data = self.ReadFlash(addr_p, self.device['writeblock'])
	
					for i in range(0, len(verify_data)):
						if ord(verify_data[i]) != args['DATA'][i]:
							print "Verification failed at 0x%08X: %i != %i" % (int(addr_p + i), ord(verify_data[i]), args['DATA'][i])
	### Read from the flash
	def ReadFlash(self, addr, length):
		data = ""
		### Count of reads to do
		max_read_len = (self.serial['maxpacket'] - 5) / self.device['readblock']
		length /= self.device['readblock']
		dots = ""
		### Loop over each segment in a read block
		while length > 0:
			### Build the argument tree
			args = {}
			if length > max_read_len:
				args['LEN'] = max_read_len
			else:
				args['LEN'] = length
			length -= args['LEN']
			### Generate the address
			args['ADDRL'] = addr & 0xFF
			args['ADDRH'] = (addr >> 8) & 0xFF
			args['ADDRU'] = (addr >> 16) & 0xFF
			if not self.quiet:
				print "Reading %i bytes from address 0x%08X" % (args['LEN'] * self.device['readblock'], addr)
			data += self.BootloaderCMD("RD_FLASH", args)
			addr += (args['LEN'] * self.device['readblock']) / self.device['bytesperaddr']
		return data
			
	### Erase the flash device
	### Note: Not sure if the cfg word backup is really needed...
	def EraseFlash(self):
		### Back up the configuration words
		cfg_words = self.ReadCFGWords()

		### Loop to catch all address (if the # of blocks is greater than 255)
		nBlocks = (self.device['pmrangehigh'] - self.device['pmrangelow'] + 1) / (self.device['eraseblock'] / self.device['bytesperaddr'])
		start_addr = self.device['pmrangelow']
		while nBlocks > 0:
			### Build the argument tree
			args = {}

			if nBlocks > 255:
				args['LEN'] = 255
			else:
				args['LEN'] = nBlocks
			nBlocks -= args['LEN']
			### Generate the address
			addr = start_addr
			args['ADDRL'] = addr & 0xFF
			args['ADDRH'] = (addr >> 8) & 0xFF
			args['ADDRU'] = (addr >> 16) & 0xFF
			
			self.BootloaderCMD("ER_FLASH", args)
			start_addr += (self.device['eraseblock'] / self.device['bytesperaddr']) * nBlocks

		### Write the configuration words
		self.WriteCFGWords(cfg_words)

	### Read the config words
	def ReadCFGWords(self):
		addr = (self.device['pmrangehigh'] + 1) - 8
		return self.ReadFlash(addr, 2)

	### Write the config words into memory (shouldn't mess up other data if present?)
	def WriteCFGWords(self, cfg_words):
		### Write the config words
		args = {}
		args['LEN'] = 1
		addr = (self.device['pmrangehigh'] + 1) - (self.device['eraseblock'] / self.device['bytesperaddr'])
		args['ADDRL'] = addr & 0xFF
		args['ADDRH'] = (addr >> 8) & 0xFF
		args['ADDRU'] = (addr >> 16) & 0xFF
		### Build the data packet
		args['DATA'] = [0xFF, 0xFF, 0xFF, 0x00] * ((self.device['writeblock'] / 4) - 2)
		for i in cfg_words:
			args['DATA'].append(ord(i))
		self.BootloaderCMD("WT_FLASH", args)

###############################################################################
### Run the commands the user requests
###############################################################################

### Initialize
p = PIC24F_Prog(Config, SerialPort)
p.verify = Verify
p.quiet = Quiet

### Prepare the command and run
if Command == "Auto":
	### Erase the flash
	p.EraseFlash()	
	### Read in the flash file
	if File == None:
		Usage()
		sys.exit(2)
	p.ReadFile(File)
	### Now write to the flash
	for k in p.hexfile.data.keys():
		### Addresses from the hexfile are in bytes
		### convert to device's addressing scheme first
		p.WriteFlash(k / p.device['bytesperaddr'], p.hexfile.data[k])
	print "Write operation complete."
	### We're done, finish up
	p.BootloaderCMD("VERIFY_OK")

### These commands are just run with no other arguments
elif Command in ("VERIFY_OK", "RD_VER"):
	p.BootloaderCMD(Command)

### Read the flash into a file
elif Command == "RD_FLASH":
	print "Starting read operation..."
	### Make sure the filename was given
	if File == None:
		Usage()
		sys.exit(2)
	### Read the entire flash out
	data = p.ReadFlash(p.device['pmrangelow'], ((p.device['pmrangehigh'] + 1) - p.device['pmrangelow']) * p.device['bytesperaddr'])
	### Add the data to the file
	p.DataToFile(p.device['pmrangelow'], data)	
	### Write out the file
	p.WriteFile(File)
	print "Read operation complete."

### Erase the flash
elif Command == "ER_FLASH":
	p.EraseFlash()

### Write the flash from a file
elif Command == "WT_FLASH":
	print "Starting write operation..."
	### Open the file
	if File == None:
		Usage()
		sys.exit(2)
	p.ReadFile(File)
	### Now write to the flash
	for k in p.hexfile.data.keys():
		### Addresses from the hexfile are in bytes
		### convert to device's addressing scheme first
		p.WriteFlash(k / p.device['bytesperaddr'], p.hexfile.data[k])
	print "Write operation complete."
