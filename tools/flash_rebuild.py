# This script is based off rockbox/rbutil/ipodpatcher/ipodpatcher.c
import sys
import struct
import os

def fourcc(string):
	return int.from_bytes(string.encode(), "big")

def unfourcc(n):
	return n.to_bytes(4, "big").decode()

if len(sys.argv) != 3:
	print(f"USAGE: {sys.argv[0]} source_dir/ dest_rom.bin")
	exit()

def arraycpy(dest_bytearray, dest_offset, source_bytearray):
	dest_bytearray[dest_offset:dest_offset+len(source_bytearray)] = source_bytearray

src_dir = sys.argv[1]
dest_path = sys.argv[2]

rom = bytearray([0xFF] * 0x80000)

bootloader = open(f"{src_dir}/bootloader.bin", "rb").read()
arraycpy(rom, 0, bootloader)

footer = b""
freespace_end = len(rom) - 0x200
for section_filename in sorted(os.listdir(f"{src_dir}/sections")):
	path = f"{src_dir}/sections/{section_filename}"
	fcc = path.split(".")[-2][-4:]
	body = open(path, "rb").read()
	bodylen = len(body)
	
	freespace_end -= bodylen
	arraycpy(rom, freespace_end, body)
	
	print(f"Section {fcc} at {hex(freespace_end)}")
	
	footer += struct.pack("<IIIIIIIIII",
		fourcc("flsh"),
		fourcc(fcc),
		0,
		freespace_end,
		bodylen,
		0x10000000,
		0,
		sum(body) & 0xFFFFFFFF,
		0xc009,
		0x00670c70)

while len(footer) < 40*5:
	footer += bytes(([0]*4) + ([0xFF]*36))

arraycpy(rom, len(rom) - 0x200, footer)

bytes_remaining = freespace_end - len(bootloader)
assert(bytes_remaining >= 0)

print(f"{hex(bytes_remaining)} bytes remaning")

with open(dest_path, "wb") as f:
	f.write(rom)
