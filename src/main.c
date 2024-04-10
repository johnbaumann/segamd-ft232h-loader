#include "md.h"

#include "flash.h"
#include "ftdi.h"
#include "joy.h"
#include <stdbool.h>
#include <stdint.h>

char test_status[32];

void reset_console()
{
	__asm__("move   #0x2700,%sr\n\t"
			"move.l (0),%a7\n\t"
			"jmp 0x000200");
}

extern const unsigned long _sdata[];

void checkCommand()
{
	const uint32_t sector_size = 8 * 1024; // To-do: Get this from the flash chip

	// To-do: Optimize for Sega, 16-bit, etc
	uint8_t response = 0;
	uint32_t xaddr, addr, len, x = 0;
	uint8_t buffer[sector_size];
	response = FT_read8();
	vdp_color(0, 0x00f);

	switch (response)
	{
	case 0x62: // 'b'
		// Load BIN
		sprintf(test_status, "Loading BIN");
		addr = FT_read32(); // Make sure it's within RAM? Might overwrite the loader, etc
		len = FT_read32();
		vdp_vsync();
		while (x < len)
		{
			*(uint8_t *)(addr + x) = FT_read8();
			x++;
		}
		break;

	case 0x63: // 'c'
		// Load ROM
		// sprintf(test_status, "Loading ROM");
		xaddr = FT_read32(); // Don't need, will always be 0x200
		addr = FT_read32();	 // Don't need, will always be 0
		len = FT_read32();	 // Check against flash size
		sprintf(test_status, "Loading %lu bytes to %lu with xaddr %lu", len, addr, xaddr);
		vdp_vsync();
		while (x < len)
		{
			//*(uint8_t *)(addr + x) = FT_read8();

			// To-do:
			// Stuff data into a buffer of sector size
			// Write buffer to flash
			// Repeat until all data is written
			buffer[x % sector_size] = FT_read8();
			if (((x % sector_size) == 0 && x > 0) || x == len - 1)
			{
				FLASH_writeSector(x / sector_size, buffer, sector_size);
			}
		}
		reset_console();
		break;

	case 0x64: // 'd'
		// Dump BIN
		addr = FT_read32();
		len = FT_read32();
		sprintf(test_status, "Dumping %lu bytes from %lu", len, addr);
		vdp_vsync();
		while (x < len)
		{
			FT_write8(*(uint8_t *)(addr + x));
			x++;
		}
		break;

	case 0x65: // 'e'
		// Call address
		sprintf(test_status, "Calling address");
		vdp_vsync();
		break;

	case 0x72: // 'r'
		// Reset
		sprintf(test_status, "Resetting console");
		reset_console();
		break;
	}
}

int main()
{
	uint16_t input_old = 0;
	uint16_t input_held = 0;
	uint16_t input_pressed = 0;

	char to_display[64];
	uint16_t counter = 0;
	char ft232_data[32];
	char ft232_status[32];

	// Calculate available space in RAM, not including the stack, vars, etc
	const long unsigned space_avail = (64 * 1024) - (unsigned long)_sdata;

	delay(5000);
	FLASH_resetBypass();
	delay(5000);

	vdp_init();
	// enable_ints;
	joy_init();

	while (1)
	{
		if (FT_status() != 0xff)
		{
			if (FT_dataReady())
			{
				checkCommand();
			}
		}

		// Update input
		input_old = input_held;
		input_held = 0;
		joy_update();
		input_held = joy_get_state(JOY_1);
		input_pressed = input_held & ~input_old;

		// Banner
		vdp_puts(VDP_PLAN_A, "**** FT232H LOADER TEST - RAM ****", 1, 4);
		sprintf(to_display, "64K RAM SYSTEM %lu BYTES FREE", space_avail);
		vdp_puts(VDP_PLAN_A, to_display, 1, 6);

		// Counter
		sprintf(to_display, "%u", counter++);
		vdp_text_clear(VDP_PLAN_A, 1, 8, 32);
		vdp_puts(VDP_PLAN_A, to_display, 1, 8);

		// FT232H Data/Status
		vdp_puts(VDP_PLAN_A, ft232_data, 1, 11);
		vdp_puts(VDP_PLAN_A, ft232_status, 1, 12);
		vdp_text_clear(VDP_PLAN_A, 1, 13, 32);

		// Joypad
		sprintf(to_display, "JOY1: %04x", joy_get_state(JOY_1));
		vdp_puts(VDP_PLAN_A, to_display, 1, 13);

		// Button mapping
		vdp_puts(VDP_PLAN_A, "A: Read Data/Status", 1, 15);
		vdp_puts(VDP_PLAN_A, "B: Write Data", 1, 16);
		vdp_puts(VDP_PLAN_A, "C: Write Status", 1, 17);

		vdp_text_clear(VDP_PLAN_A, 1, 19, 32);
		vdp_puts(VDP_PLAN_A, test_status, 1, 19);

		// Button actions
		if (input_pressed & BUTTON_A) // Read Data/Status
		{
			vdp_color(0, 0xf00);
			/*sprintf(ft232_data, "FT232 DATA: %02x", *ftdi_data & 0xff);
			sprintf(ft232_status, "FT232 STATUS: %02x", *ftdi_status & 0xff);*/
			// reset_console();
		}
		else if (input_pressed & BUTTON_B) // Write Data
		{
			vdp_color(0, 0x0f0);
			//*ftdi_data = 0x69;
		}
		else if (input_pressed & BUTTON_C) // Write Status
		{
			vdp_color(0, 0x00f);
			// FLASH_testBypassMode();
			//*ftdi_status = 0x42;
		}
		else
		{
			vdp_color(0, 0x570);
		}

		vdp_vsync();
	};

	return 0;
}
