import serial
import os
import time
from PIL import Image
import sys

def sendcmd(ser, mode=4, cmd=0, param=[]):
	body = [mode, cmd>>8, cmd&0xFF] + list(param)
	cksum = -sum(body)-len(body) & 0xFF
	pkt = bytes([0xff, 0x55, len(body)] + body + [cksum])
	print(">", pkt.hex())
	ser.write(pkt)

def read_resp(ser):
	header = ser.read(2)
	assert(header == b"\xff\x55")
	length = ser.read(1)
	body = ser.read(length[0])
	csum = ser.read(1)
	print("<", (header+length+body+csum).hex())
	assert(csum[0] == (-sum(body)-len(body) & 0xFF))
	mode = body[0]
	cmd = body[1]<<8 | body[2]
	return mode, cmd, body[3:]

ser = serial.Serial('/dev/ttyUSB0', 57600, timeout=1)

# enter mode 4
sendcmd(ser, mode=0, cmd=0x104)

# play
sendcmd(ser, cmd=0x29, param=[1])
read_resp(ser)

sendcmd(ser, cmd=0x14)
ipod_name = read_resp(ser)
print(ipod_name)

sendcmd(ser, cmd=0x20, param=(0).to_bytes(4, "big"))
song_name = read_resp(ser)
print(song_name)

im = Image.open(sys.argv[1]).convert('L')

width = im.width # safe max: 0x130
height = im.height # safe max: 0xa8
# XXX: width must be multiple of 4 (or maybe 8?)
img_data = [0x01,   width>>8, width&0xFF,   0x00, height,   0x00, 0x00, 0x00, width//4]

pixels = list(im.getdata())
for i in range((width*height)//4):
	pix = 0
	for j in range(4):
		pix <<= 2
		pix |= 3 - (pixels[i*4+j]>>6)
	img_data.append(pix)

#img_data += os.urandom((width*height*2)//8)

i=0
while img_data:
	chunk, img_data = img_data[:0xF0], img_data[0xF0:]
	sendcmd(ser, cmd=0x32, param=[i>>8, i&0xFF]+list(chunk))
	read_resp(ser)
	i += 1

#read_resp(ser)
