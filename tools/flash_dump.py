# This script is based off rockbox/rbutil/ipodpatcher/ipodpatcher.c
import sys
import struct
from pathlib import Path

def fourcc(string):
	return int.from_bytes(string.encode(), "big")

def unfourcc(n):
	return n.to_bytes(4, "big").decode()

if len(sys.argv) != 3:
	print(f"USAGE: {sys.argv[0]} rom_dump.bin dest_dir/")
	exit()

fw_file = open(sys.argv[1], "rb")
dest_dir = sys.argv[2]

Path(f"{dest_dir}/sections").mkdir(parents=True, exist_ok=True)

fw = fw_file.read()
table = fw[-0x200:]

i = 0
first_section_start = 0xFFFFFFFF
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
    in struct.iter_unpack("IIIIIIIIII", table[:40*(len(table)//40)]):

	if not imgtype:
		break
	
	print()
	print("ityp ftyp id         dev_offset length     addr       entry_off  checksum   version    load_addr")
	print(f"{unfourcc(imgtype)} {unfourcc(filetype)} 0x{id_:08x} 0x{dev_offset:08x} 0x{length:08x} 0x{addr:08x} 0x{entry_offset:08x} 0x{checksum:08x} 0x{version:08x} 0x{load_addr:08x}")
	
	first_section_start = min(first_section_start, dev_offset)
	
	fw_file.seek(dev_offset)
	part_data = fw_file.read(length)
	
	checksum_calc = sum(part_data) & 0xFFFFFFFF
	assert(checksum == checksum_calc)
	
	dest = f"{dest_dir}/sections/{i}_{unfourcc(filetype)}.bin"
	print(f"\nExtracting to {dest}")
	with open(dest, "wb") as f:
		f.write(part_data)
	
	i += 1

idx = first_section_start
while fw[idx-1] == 0xFF:
	idx -= 1

bootloader = fw[:idx]
dest = f"{dest_dir}/bootloader.bin"

print(f"Extracting bootloader to {dest}")
with open(dest, "wb") as f:
	f.write(bootloader)
