all: out.raw.lcd5

shellcode.bin: shellcode.asm
	arm-none-eabi-as shellcode.asm -o shellcode.o
	arm-none-eabi-ld -Ttext 0x108899b0 -o shellcode.elf shellcode.o
	arm-none-eabi-objdump -d shellcode.elf
	arm-none-eabi-objcopy -O binary shellcode.elf shellcode.bin

out.raw.lcd5: test_image.png shellcode.bin
	python img_to_lcd5.py test_image.png shellcode.bin

install: out.raw.lcd5
	sudo mount /dev/sdb2 /mnt/tmp/
	sudo cp out.raw.lcd5 /mnt/tmp/iPod_Control/Games_RO/Hacked/Vortex.raw.lcd5
	sudo umount /mnt/tmp
	sudo eject /dev/sdb
