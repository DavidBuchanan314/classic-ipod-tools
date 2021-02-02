# This script is based off rockbox/rbutil/ipodpatcher/ipodpatcher.c
import sys
import struct

# This sign can't stop me because I can't read
fw_header = rb"""
{{~~  /-----\   
{{~~ /       \  
{{~~|         | 
{{~~| S T O P | 
{{~~|         | 
{{~~ \       /  
{{~~  \-----/   
Copyright(C) 200
1 Apple Computer
, Inc.----------
----------------
----------------
----------------
----------------
----------------
---------------
""".replace(b"\n", b"")+b"\0"

# The maximum number of images in a firmware partition - a guess...
# ( from rbutil/ipodpatcher/ipodio.h )
MAX_IMAGES = 10

def fourcc(string):
	return int.from_bytes(string.encode(), "big")

def unfourcc(n):
	return n.to_bytes(4, "big").decode()




fw_file = open(sys.argv[1], "rb")

if fw_file.read(0x100) != fw_header:
	exit("Could not find Apple copyright header")

fw_directory_header = fw_file.read(0x100)

dirmagic, diroffset, _, version = struct.unpack_from("<IiHH", fw_directory_header)

if dirmagic != fourcc("[hi]"):
	exit("Bad firmware directory")

if version not in {2, 3}:
	exit("Unknown firmware format version")

diraddr = 0x200 + diroffset
print("Reading image directory @", hex(diraddr))
fw_file.seek(diraddr)

for imgtype, \
    filetype, \
    id_, \
    dev_offset, \
    length, \
    addr, \
    entry_offset, \
    checksum, \
    version, \
    load_addr \
    in struct.iter_unpack("IIIIIIIIII", fw_file.read(40*MAX_IMAGES)):

	if not imgtype:
		break
	
	print()
	print("ityp ftyp id         dev_offset length     addr       entry_off  checksum   version    load_addr")
	print(f"{unfourcc(imgtype)} {unfourcc(filetype)} 0x{id_:08x} 0x{dev_offset:08x} 0x{length:08x} 0x{addr:08x} 0x{entry_offset:08x} 0x{checksum:08x} 0x{version:08x} 0x{load_addr:08x}")
	
	fw_file.seek(0x200 + dev_offset)
	part_data = fw_file.read(length)
	
	checksum_calc = sum(part_data) & 0xFFFFFFFF
	print(checksum_calc, checksum, checksum_calc-checksum)
	
	f = open(f"dump_{unfourcc(filetype)}.bin", "wb")
	f.write(part_data)
	f.close()
