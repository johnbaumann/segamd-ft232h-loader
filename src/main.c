#include "md.h"

#include "flash.h"
#include "ftdi.h"
#include "joy.h"
#include <stdbool.h>
#include <stdint.h>

char test_status[36];

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
	uint32_t target_sector = 0;
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
		 sprintf(test_status, "Loading ROM");
		len = FT_read32(); // Check against flash size
		sprintf(test_status, "Loading %lu bytes to %lx with xaddr %lx", len, addr, xaddr);
		vdp_text_clear(VDP_PLAN_A, 1, 19, 36);
		vdp_puts(VDP_PLAN_A, test_status, 1, 19);
		vdp_vsync();
		target_sector = 0;
		while (x < len)
		{
			for (uint32_t i = 0; i < sector_size && x < len; i++)
			{
				buffer[i] = FT_read8();
				x++;
			}
			sprintf(test_status, "Writing sector %li / %li", target_sector + 1, len / sector_size);
			vdp_text_clear(VDP_PLAN_A, 1, 19, 36);
			vdp_puts(VDP_PLAN_A, test_status, 1, 19);
			vdp_vsync();
			FLASH_writeSector(target_sector, buffer, sector_size);

			target_sector++;
		}
		reset_console();
		break;

	case 0x64: // 'd'
		// Dump BIN
		addr = FT_read32();
		len = FT_read32();
		sprintf(test_status, "Dumping %08lx bytes from %08lx", len, addr);
		vdp_text_clear(VDP_PLAN_A, 1, 19, 36);
		vdp_puts(VDP_PLAN_A, test_status, 1, 19);
		vdp_vsync();
		while (x < len)
		{
			FT_write8(*(uint8_t *)(addr + x));
			x++;
		}

		// Read from VRAM
		/*while (x < len)
		{
			vdp_dma_vram(x, 0x0120 << 5, 512 << 4);
			*vdp_ctrl_wide = DMA_ADDR(0x0120 << 5); // VRAM read
			for (int i = 0; i < 0x4000; i++)
			{
				FT_write16(*vdp_data_port);
			}
			x += 0x8000;
		}*/
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

	default:
		sprintf(test_status, "Unknown command %02x", response);
	}
}

int main()
{
	uint16_t input_old = 0;
	uint16_t input_held = 0;
	uint16_t input_pressed = 0;

	char to_display[64];
	// uint16_t counter = 0;
	// char counter_text[32];
	//char ft232_data[32];
	//char ft232_status[32];

	// Calculate available space in RAM, not including the stack, vars, etc
	const long unsigned space_avail = (64 * 1024) - (unsigned long)_sdata;

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
		// vdp_text_clear(VDP_PLAN_A, 1, 8, 32);
		// vdp_puts(VDP_PLAN_A, counter_text, 1, 8);
		// FT232H Data/Status
		// vdp_puts(VDP_PLAN_A, ft232_data, 1, 11);
		// vdp_puts(VDP_PLAN_A, ft232_status, 1, 12);
		// vdp_text_clear(VDP_PLAN_A, 1, 13, 32);

		vdp_text_clear(VDP_PLAN_A, 1, 19, 36);
		vdp_puts(VDP_PLAN_A, test_status, 1, 19);

		// Button mapping
		/*vdp_puts(VDP_PLAN_A, "A: Read Data/Status", 1, 15);
		vdp_puts(VDP_PLAN_A, "B: Write Data", 1, 16);
		vdp_puts(VDP_PLAN_A, "C: Write Status", 1, 17);

		// Button actions
		if (input_pressed & BUTTON_A) // Read Data/Status
		{
			vdp_color(0, 0xf00);
			sprintf(ft232_data, "FT232 DATA: %02x", *ftdi_data);
			sprintf(ft232_status, "FT232 STATUS: %02x", *ftdi_status);
			// reset_console();
		}
		else if (input_pressed & BUTTON_B) // Write Data
		{
			vdp_color(0, 0x0f0);
			*ftdi_data = 0x69;
		}
		else if (input_pressed & BUTTON_C) // Write Status
		{
			vdp_color(0, 0x00f);
			// FLASH_testBypassMode();
			*ftdi_status = 0x42;
		}
		else
		{
			vdp_color(0, 0x570);
		}*/

		vdp_color(0, 0x570);

		vdp_vsync();
	};

	return 0;
}
