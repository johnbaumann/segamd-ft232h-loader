#!/usr/bin/env python3
# psx232h.py - simple exe uploader for ft232h
# Runs on Python 3.7 - requires the pyserial libaries to be installed
# Tested on Windows & Linux 
# by @danhans42 (instag/twitter/psxdev/@gmail.com)

import sys
import serial
import os
import zlib
import time
from time import sleep

args = int(len(sys.argv))

def usage():
    sys.stdout.write("-h                                  help (this screen)\n")
    sys.stdout.write("-exe    <port> <psx exe>            upload & execute PSX-EXE\n")    
    sys.stdout.write("-bin    <port> <file> <addr>        upload file to address\n")    
    sys.stdout.write("-goto   <port> <addr>               goto to <addr>\n")
    sys.stdout.write("-dump   <port> <file> <addr> <len>  download data from PSX\n")
    sys.stdout.write("-reboot <port>                      reboot attached PSX\n\n")
    sys.exit()

def resetpsx():
	serialport = sys.argv[2]
	sys.stdout.write("Port       : ")
	sys.stdout.write(serialport)
	sys.stdout.write("\n")
	ser = serial.Serial(serialport,1000000,write_timeout=10,dsrdtr=False)
	ser.write(b'\x00\x72')
	sys.stdout.write("Command    : Reset PSX\n\n")
	sys.stdout.write("Operation Complete\n\n")

def download():
	serialport = sys.argv[2]
	file = sys.argv[3]
	addr = (int(sys.argv[4],16)).to_bytes(4, byteorder='little',signed=False)
	len = (int(sys.argv[5],16)).to_bytes(4, byteorder='little',signed=False)
	sys.stdout.write("Port       : ")
	sys.stdout.write(serialport)
	sys.stdout.write("\nFilename   : {}\n".format(file))
	sys.stdout.write("Address    : ")
	sys.stdout.write(hex(int(sys.argv[4],16)))
	sys.stdout.write("\n")
	sys.stdout.write("Length     : ")
	sys.stdout.write(str(int(sys.argv[5],16)))
	sys.stdout.write(" bytes\n")
	sys.stdout.write("Command    : Download data\n")
	ser = serial.Serial(serialport,1000000,write_timeout=10,dsrdtr=False)
	dump = open(file,'wb')
	buffer = bytearray()
	ser.write(b'\x00\x64')
	addrLo = addr[0:2]
	addrHi = addr[2:4]
	ser.write((int.from_bytes(addrLo)).to_bytes(2, byteorder='little',signed=False))
	ser.write((int.from_bytes(addrHi)).to_bytes(2, byteorder='little',signed=False))
	lenLo = len[0:2]
	lenHi = len[2:4]
	ser.write((int.from_bytes(lenLo)).to_bytes(2, byteorder='little',signed=False))
	ser.write((int.from_bytes(lenHi)).to_bytes(2, byteorder='little',signed=False))	
	sys.stdout.write("Reading Data...")
	sys.stdout.flush()
	start_time = time.perf_counter()
	buffer=ser.read(int(sys.argv[5],16))
	end_time = time.perf_counter()
	elapsed_time = end_time - start_time
	sys.stdout.write(" Done!\n")
	sys.stdout.write("Operation Complete\n\n")
	print(f"Elapsed time: {elapsed_time:.4f} seconds")
	print(f"Speed: {(int(sys.argv[5],16) / 1000) / elapsed_time:.1f} KB/s\n")
	print("CRC32: %x" % zlib.crc32(buffer))
	dump.write(buffer)
	dump.close()

def gotoaddr():
	serialport = sys.argv[2]
	addr = (int(sys.argv[3],16)).to_bytes(4, byteorder='little',signed=False)
	sys.stdout.write("Port       : ")
	sys.stdout.write(serialport)
	sys.stdout.write("\n")
	ser = serial.Serial(serialport,1000000,write_timeout=10,dsrdtr=False)
	ser.write(b'\x00\x65')
	sys.stdout.write("Command    : Goto address ")
	sys.stdout.write(hex(int(sys.argv[3],16)))
	ser.write(addr)
	sys.stdout.write("\n\nOperation Complete\n\n")

def uploadexe():
	serialport = sys.argv[2]
	filename = sys.argv[3]
	filesize = os.path.getsize(filename)
	inputfile = open(filename, "rb")
	bin = inputfile.read()
	inputfile.close()
	sys.stdout.write("Port       : ")
	sys.stdout.write(serialport)
	sys.stdout.write("\nEXE Name   : {}\n".format(filename))
	sys.stdout.write("File Size  : {} bytes\n".format(filesize))
	ser = serial.Serial(serialport,1000000,write_timeout=10,dsrdtr=False)
	ser.write(b'\x00\x63')
	sys.stdout.write("Command    : Upload & execute PS-X EXE\n\n")
	len = filesize.to_bytes(4, byteorder='little',signed=False)
	lenLo = len[0:2]
	lenHi = len[2:4]
	ser.write((int.from_bytes(lenLo)).to_bytes(2, byteorder='little',signed=False))
	ser.write((int.from_bytes(lenHi)).to_bytes(2, byteorder='little',signed=False))	
	sys.stdout.write("Sending Filesize\n")
	sys.stdout.write("Sending Data... \n")
	sys.stdout.flush()
	start_time = time.perf_counter()
	for i, x in enumerate(bin):
		sys.stdout.write("Sending byte {} of {}\r".format(i, filesize))
		sys.stdout.flush()
		ser.write(x.to_bytes(1, byteorder='little',signed=False))
	end_time = time.perf_counter()
	elapsed_time = end_time - start_time
	sys.stdout.write("\nDone!\n")
	sys.stdout.write("Executing\n")
	sys.stdout.write("Operation Complete\n\n")
	print(f"Elapsed time: {elapsed_time:.4f} seconds")
	print(f"Speed: {((filesize / 1000) / elapsed_time):.1f} KB/s\n", )
	sleep(0.1)

def upload():
	serialport = sys.argv[2]
	filename = sys.argv[3]
	addr = (int(sys.argv[4],16)).to_bytes(4, byteorder='little',signed=False)
	filesize = os.path.getsize(filename)
	inputfile = open(filename, 'rb')
	inputfile.seek(0, 0)
	sys.stdout.write("Port       : ")
	sys.stdout.write(serialport)
	sys.stdout.write("\nFilename   : {}\n".format(filename))
	sys.stdout.write("File Size  : {} bytes\n".format(os.path.getsize(filename)))
	sys.stdout.write("Address    : ")
	sys.stdout.write(hex(int(sys.argv[4],16)))
	sys.stdout.write("\n")
	bin = inputfile.read(os.path.getsize(filename))
	ser = serial.Serial(serialport,1000000,write_timeout=10,dsrdtr=False)
	ser.write(b'\x00\x62')
	sys.stdout.write("Command    : Upload data\n\n")
	addrLo = addr[0:2]
	addrHi = addr[2:4]
	ser.write((int.from_bytes(addrLo)).to_bytes(2, byteorder='little',signed=False))
	ser.write((int.from_bytes(addrHi)).to_bytes(2, byteorder='little',signed=False))
	#ser.write(addr)
	sys.stdout.write("Sending Load Address\n")
	lenLo = filesize.to_bytes(4, byteorder='little',signed=False)[0:2]
	lenHi = filesize.to_bytes(4, byteorder='little',signed=False)[2:4]
	ser.write((int.from_bytes(lenLo)).to_bytes(2, byteorder='little',signed=False))
	ser.write((int.from_bytes(lenHi)).to_bytes(2, byteorder='little',signed=False))	
	#ser.write(filesize.to_bytes(4, byteorder='little',signed=False))
	sys.stdout.write("Sending Filesize\n")
	sys.stdout.write("Sending Data...")
	sys.stdout.flush()
	for i, x in enumerate(bin):
		sys.stdout.write("Sending byte {} of {}\r".format(i, filesize))
		sys.stdout.flush()
		ser.write(x.to_bytes(1, byteorder='little',signed=False))
	sys.stdout.write(" Done!\n")
	sys.stdout.write("Operation Complete\n\n")
	sleep(0.1)


#main

sys.stdout.write("psx232h.py - by @danhans42\n\n")

if args < 2:
    sys.stdout.write("insufficient parameters - try psx232h.py -h\n\n")
    sys.exit()

command = sys.argv[1]

if  command == "-h":
    usage()

elif  command == "-exe":
    if args < 4:
        sys.stdout.write("insufficient parameters - try psx232h.py -h\n\n")
        sys.exit()
    serialport = sys.argv[2]
    file = sys.argv[3]
    stat = "Uploading"
    uploadexe()

elif  command == "-bin":
    if args < 5:
        sys.stdout.write("insufficient parameters - try psx232h.py -h\n\n")
        sys.exit()
    serialport = sys.argv[2]
    file = sys.argv[3]
    stat = "Uploading"
    upload()

elif  command == "-reboot":
    if args < 3:
        sys.stdout.write("insufficient parameters - try psx232h.py -h\n\n")
        sys.exit()
    serialport = sys.argv[2]
    resetpsx()

elif  command == "-goto":
    if args < 3:
        sys.stdout.write("insufficient parameters - try psx232h.py -h\n\n")
        sys.exit()
    serialport = sys.argv[2]
    gotoaddr()

elif  command == "-dump":
	if args < 6:
		sys.stdout.write("insufficient parameters - try psx232h.py -h\n\n")
		sys.exit()
	stat = "Downloading"
	download()

else:
    sys.stdout.write("error: invalid command\n\n")
    usage()
	
	
	
