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
			"move.l (4),%a6\n\t"
			"jmp (%a6)");
}

extern const unsigned long _sdata[];

void checkCommand()
{
	const uint32_t sector_size = 8U * 1024U; // To-do: Get this from the flash chip

	// To-do: Optimize for Sega, 16-bit, etc
	uint8_t response = 0;
	uint32_t addr, len, x = 0;
	uint8_t buffer[sector_size];
	uint32_t target_sector = 0;
	response = FT_read8();
	vdp_color(0, 0x00f);

	switch (response)
	{
	case 0x62: // 'b'
		// Load BIN
		sprintf(test_status, "Loading BIN");
		addr = FT_read32();
		len = FT_read32();

		/*if (addr < 0x400000)
		{
			// Cartridge ROM/RAM
			// To-do: Flash the chip
		}
		else
		{
			// RAM, probably
			while (x < len)
			{
				*(uint8_t *)(addr + x) = FT_read8();
				x++;
			}
		}*/
		while (x < len)
		{
			uint8_t lo, hi;
			hi = FT_read8();
			lo = FT_read8();
			*(uint16_t *)(addr + x) = (hi << 8 | lo);
			x += 2;
		}
		break;

	case 0x63: // 'c'
		// Load ROM
		sprintf(test_status, "Loading ROM");
		len = FT_read32(); // To-do: Check against flash size
		target_sector = 0;
		while (x < len)
		{
			for (uint32_t i = 0; i < sector_size && x < len; i++)
			{
				buffer[i] = FT_read8();
				x++;
			}
			sprintf(test_status, "Writing sector %li / %li", target_sector + 1, len / sector_size);
			vdp_text_clear(VDP_PLAN_A, 5, 8, 36);
			vdp_puts(VDP_PLAN_A, test_status, 5, 8);
			vdp_vsync();
			FLASH_writeSector(target_sector, buffer);

			target_sector++;
		}
		sprintf(test_status, "Resetting...");
		vdp_text_clear(VDP_PLAN_A, 0, 8, 36);
		vdp_puts(VDP_PLAN_A, test_status, 0, 8);
		reset_console();
		break;

	case 0x64: // 'd'
		// Dump BIN
		addr = FT_read32();
		len = FT_read32();
		sprintf(test_status, "Dumping %08lx bytes from %08lx", len, addr);
		vdp_text_clear(VDP_PLAN_A, 5, 8, 36);
		vdp_puts(VDP_PLAN_A, test_status, 5, 8);
		vdp_vsync();
		while (x < len)
		{
			FT_write8(*(uint8_t *)(addr + x));
			x++;
		}
		break;

	case 0x65: // 'e'
		addr = FT_read32();
		goto *(void *)(addr);
		break;

	case 0x72: // 'r'
		reset_console();
		break;

	default:
		sprintf(test_status, "Unknown command %02x", response);
	}
}

const uint16_t black_box[] = {0xeeee, 0xeeee, 0xeeee, 0xeeee, 0xeeee, 0xeeee, 0xeeee, 0xeeee, 0xeeee, 0xeeee, 0xeeee, 0xeeee, 0xeeee, 0xeeee, 0xeeee, 0xeeee};
const uint16_t cursor_box[] = {0xdddd, 0xdddd, 0xdddd, 0xdddd, 0xdddd, 0xdddd, 0xdddd, 0xdddd, 0xdddd, 0xdddd, 0xdddd, 0xdddd, 0xdddd, 0xdddd, 0xdddd, 0xdddd};

int main()
{
	uint16_t input_old = 0;
	uint16_t input_held = 0;
	uint16_t input_pressed = 0;

	char to_display[64];

	// Calculate available space in RAM, not including the stack, vars, etc
	const long unsigned space_avail = (64 * 1024) - (unsigned long)_sdata;

	vdp_init();
	// enable_ints;
	joy_init();

	// Make a black rect we can use to clear the screen
	vdp_tiles_load((uint32_t *)black_box, TILE_FONTINDEX - 1, 1);
	// Cursor rect
	vdp_tiles_load((uint32_t *)cursor_box, TILE_FONTINDEX - 2, 1);

	const uint16_t cursor_x = 5;
	const uint16_t cursor_y = 10;

	uint32_t vsync_counter = 0;

	while (1)
	{
		if (FT_status() != 0xff)
		{
			if (FT_dataReady())
			{
				checkCommand();
			}
			else
			{
				// sprintf(test_status, "Ready");
			}
		}

		// Update input
		input_old = input_held;
		input_held = 0;
		joy_update();
		input_held = joy_get_state(JOY_1);
		input_pressed = input_held & ~input_old;

		// Banner
		vdp_text_clear(VDP_PLAN_A, 5, 4, 28);
		vdp_puts(VDP_PLAN_A, "**** FT232H TEST - RAM ****", 5, 4);
		sprintf(to_display, "64K RAM SYSTEM %lu BYTES FREE", space_avail);
		vdp_puts(VDP_PLAN_A, to_display, 5, 6);

		vdp_text_clear(VDP_PLAN_A, 5, 8, 36);
		vdp_puts(VDP_PLAN_A, test_status, 5, 8);

		// Set BG Color default
		vdp_color(0, 0x570);

		// Black bg box
		vdp_map_fill_rect(VDP_PLAN_B, TILE_FONTINDEX - 1, 3, 3, 34, 22, 0);

		// Blink cursor every 120 frames
		if (vsync_counter % 120 < 60)
		{
			vdp_map_fill_rect(VDP_PLAN_B, TILE_FONTINDEX - 2, cursor_x, cursor_y, 1, 1, 0);
		}

		vdp_vsync();
		vsync_counter++;
	};

	return 0;
}
