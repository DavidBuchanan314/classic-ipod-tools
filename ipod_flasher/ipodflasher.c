#include "plugin.h"

// This code is based on functions ripped from rockbox/apps/plugins/iriver_flash.c
// This file is therefore also licensed under the GPLv2 license.

/*
 _   _   _   ____    _    _   _  ____ _____ ____    _   _   _ 
| | | | | | |  _ \  / \  | \ | |/ ___| ____|  _ \  | | | | | |
| | | | | | | | | |/ _ \ |  \| | |  _|  _| | |_) | | | | | | |
|_| |_| |_| | |_| / ___ \| |\  | |_| | |___|  _ <  |_| |_| |_|
(_) (_) (_) |____/_/   \_\_| \_|\____|_____|_| \_\ (_) (_) (_)

This code can brick an iPod very easily. It is a functional prototype, not
suitable for general use. Only use it if you understand *exactly*
what it's doing.

This is a rockbox plugin - if don't know how to compile it, you probably shouldn't :P

 - Retr0id, 2021-04-17

*/


/*
 * Flash commands may rely on null pointer dereferences to work correctly.
 * Disable this feature of GCC that may interfere with proper code generation.
 */
#pragma GCC optimize "no-delete-null-pointer-checks"


#define WORD_SIZE 2
#define SECTOR_SIZE 4096
#define FLASH_BASE 0x20000000

// rockbox defines this wrong...
#undef FLASH_SIZE
#define FLASH_SIZE 0x80000

static volatile uint32_t *_DAT_70000030 = (uint32_t*)0x70000030L;

#define IPOD_5555 (0xAAAA / WORD_SIZE)
#define IPOD_2AAA (0x5554 / WORD_SIZE)

/* no idea what these registers are for, but they're important! */
void set_bit_30(int bit_30_value)
{
	/* wait for bit 27 to be 1 */
	while ((*_DAT_70000030 & 0x8000000) == 0);

	/* set bit 30 */
	*_DAT_70000030 = (*_DAT_70000030 & 0xbfffffff) | ((bit_30_value != 0) << 30);
	return;
}

#if 0
/* get read-only access to flash at the given offset */
static const void* flash(uint32_t offset)
{
    const uint16_t* FB = (uint16_t*) FLASH_BASE;
    return &FB[offset / WORD_SIZE];
}
#endif

/* wait until the rom signals completion of an operation */
static bool flash_wait_for_rom(uint32_t offset)
{
    const size_t MAX_TIMEOUT = 0xFFFFFF; /* should be sufficient for most targets */
    const size_t RECOVERY_TIME = 64; /* based on 140MHz MCF 5249 */
    volatile uint16_t* FB = (uint16_t*) FLASH_BASE;
    uint16_t old_data = FB[offset / WORD_SIZE] & 0x0040; /* we only want DQ6 */
    volatile size_t i; /* disables certain optimizations */
    bool result;

    /* repeat up to MAX_TIMEOUT times or until DQ6 stops flipping */
    for (i = 0; i < MAX_TIMEOUT; i++)
    {
        uint16_t new_data = FB[offset / WORD_SIZE] & 0x0040; /* we only want DQ6 */
        if (old_data == new_data)
            break;
        old_data = new_data;
    }

    result = i != MAX_TIMEOUT;

    /* delay at least 1us to give the bus time to recover */
    for (i = 0; i < RECOVERY_TIME; i++);

    return result;
}

/* erase the sector at the given offset */
static bool flash_erase_sector(uint32_t offset)
{
    volatile uint16_t* FB = (uint16_t*) FLASH_BASE;

    /* execute the sector erase command */
    FB[IPOD_5555] = 0xAA;
    FB[IPOD_2AAA] = 0x55;
    FB[IPOD_5555] = 0x80;
    FB[IPOD_5555] = 0xAA;
    FB[IPOD_2AAA] = 0x55;
    FB[offset / WORD_SIZE] = 0x30;

    return flash_wait_for_rom(offset);
}

/* program a word at the given offset */
static bool flash_program_word(uint32_t offset, uint16_t word)
{
    volatile uint16_t* FB = (uint16_t*) FLASH_BASE;

    /* execute the word program command */
    FB[IPOD_5555] = 0xAA;
    FB[IPOD_2AAA] = 0x55;
    FB[IPOD_5555] = 0xA0;
    
    // XXX: endian swap - should probably do this inside flash_program_bytes,
    // but I don't understand their word packing logic...
    // Refactoring is hard because a single mistake could brick an iPod.
    uint16_t foo = (word >> 8) | (word << 8);
    
    FB[offset / WORD_SIZE] = foo;

    return flash_wait_for_rom(offset);
}

#if 0
/* bulk erase of adjacent sectors */
static void flash_erase_sectors(uint32_t offset, uint32_t sectors,
                                bool progress)
{
    for (uint32_t i = 0; i < sectors; i++)
    {
        set_bit_30(1);
        flash_erase_sector(offset + i * SECTOR_SIZE);
        set_bit_30(0);

        /* display a progress report if requested */
        if (progress)
        {
            rb->lcd_putsf(0, 3, "Erasing... %u%%", (i + 1) * 100 / sectors);
            rb->lcd_update();
        }
    }
}
#endif

/* bulk program of bytes */
static void flash_program_bytes(uint32_t offset, const void* ptr,
                                uint32_t len, bool progress)
{
    const uint8_t* data = ptr;

    for (uint32_t i = 0; i < len; i += WORD_SIZE)
    {
        uint32_t j = i + 1;
        uint32_t k = ((j < len) ? j : i) + 1;
        uint16_t word = (data[i] << 8) | (j < len ? data[j] : 0xFF);

        //set_bit_30(1);
        flash_program_word(offset + i, word);
        //set_bit_30(0);

        /* display a progress report if requested */
        if (progress && ((i % SECTOR_SIZE) == 0 || k == len))
        {
            rb->lcd_putsf(0, 4, "Programming... %u%%", k * 100 / len);
            rb->lcd_update();
        }
    }
}

#if 0
/* bulk verify of programmed bytes */
static bool flash_verify_bytes(uint32_t offset, const void* ptr,
                               uint32_t len, bool progress)
{
    const uint8_t* FB = flash(offset);
    const uint8_t* data = ptr;

    /* don't use memcmp so we can provide progress updates */
    for (uint32_t i = 0; i < len; i++)
    {
        uint32_t j = i + 1;

        if (FB[i] != data[i])
            return false;

        /* display a progress report if requested */
        if (progress && ((i % SECTOR_SIZE) == 0 || j == len))
        {
            rb->lcd_putsf(0, 5, "Verifying... %u%%", j * 100 / len);
            rb->lcd_update();
        }
    }

    return true;
}
#endif

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
	int button, quit = 0;
	volatile uint16_t* FB = (uint16_t*) FLASH_BASE;
	uint8_t* buffer;
	size_t buffer_len;
	int fd;
	off_t fd_len;

	(void)parameter;

	//int mode = rb->system_memory_guard(MEMGUARD_NONE);

	rb->splash(HZ*1, "Hello NOR World!");

	fd = rb->open("/rom.bin", O_RDONLY);
	if (fd < 0) {
		rb->lcd_puts(0, 0, "Failed to open file.");
		goto waitloop;
	}

	fd_len = rb->filesize(fd);
	buffer = rb->plugin_get_audio_buffer(&buffer_len);

	if ((size_t) fd_len > buffer_len) {
		rb->lcd_puts(0, 0, "Not enough RAM.");
		goto waitloop;
	}

	if (fd_len != FLASH_SIZE) {
		rb->lcd_puts(0, 0, "Firmware wrong size.");
		goto waitloop;
	}

	if (rb->read(fd, buffer, fd_len) != fd_len) {
		rb->lcd_puts(0, 0, "File read failed.");
		goto waitloop;
	}

	// disable cache for flash ranges?
	*(volatile uint32_t*)0xf000f040 = 0x3a00 | 0x40000000;

	set_bit_30(1);

	/* Enter CFI device ID mode */
	FB[IPOD_5555] = 0xAA;
	FB[IPOD_2AAA] = 0x55;
	FB[IPOD_5555] = 0x90;
	rb->sleep(1);
	
	/* read manufacturer id and device id */
	uint16_t manufacturer_id = FB[0];
	uint16_t device_id = FB[1];

	/* Leave device ID mode */
	FB[0] = 0xf0;
	FB[0] = 0xff;
	rb->sleep(1);
	
	set_bit_30(0);

	rb->lcd_putsf(0, 0, "ID: 0x%04x 0x%04x", manufacturer_id, device_id);
	rb->lcd_update();

	if (manufacturer_id != 0xbf || device_id != 0x272f) {
		rb->lcd_puts(0, 1, "Unrecognised ID!");
		goto waitloop;
	}


	for (size_t sector=0; sector < FLASH_SIZE / SECTOR_SIZE; sector++) {
		size_t sector_offset = sector * SECTOR_SIZE;
		if (memcmp((void*)&FB[sector_offset / WORD_SIZE], &buffer[sector_offset], SECTOR_SIZE) == 0) {
			// sector does not need to be changed
			continue;
		}

#if 1
		set_bit_30(1);
		flash_erase_sector(sector_offset);
		// TODO: check erase success ?
		flash_program_bytes(sector_offset, &buffer[sector_offset], SECTOR_SIZE, 0);
		set_bit_30(0);

		if (memcmp((void*)&FB[sector_offset / WORD_SIZE], &buffer[sector_offset], SECTOR_SIZE) != 0) {
			rb->lcd_puts(0, 1, "panik! verify failed.");
			rb->lcd_puts(0, 2, "see source comment");
			
			/*
			If you're reading this, something very bad has happened, and there's
			a good chance that your iPod is now bricked. But don't panic! If you
			can fix the issue and reflash the ROM correctly before you reboot,
			you might be able to save it (I had to do this once already!).
			
			Use the rockbox debug option to dump the current state of the ROM,
			so that you can inspect it on a PC.
			
			Figure out your bug(s), recompile the plugin, and copy it back onto
			the iPod to run again.
			
			*/
			
			goto waitloop;
		}
#endif

		rb->lcd_putsf(0, 1, "Wrote sector 0x%x", sector);
		rb->lcd_update();
		
		//rb->sleep(HZ*2);
	}

	rb->lcd_puts(0, 2, "Done writing.");

waitloop:

	//rb->system_memory_guard(mode);
	
	if (fd >= 0) {
		rb->close(fd);
	}

	rb->lcd_update();

	/* Wait for MENU to be pressed (give time to read logs) */

	while(!quit) {
		button = rb->button_get(true);
		switch(button) {
			case BUTTON_MENU:
				quit = true;
				break;
			default:
				if (rb->default_event_handler(button) == SYS_USB_CONNECTED) {
					return PLUGIN_USB_CONNECTED;
				}
				break;
		}
	}

	return PLUGIN_OK;
}
