#include "md.h"

#include "joy.h"

#define FT232_BASE 0xA13000

static volatile uint8_t *const FT232_DATA = (uint8_t *)FT232_BASE + 0;
static volatile uint8_t *const FT232_STATUS = (uint8_t *)FT232_BASE + 2;

/*void reset_console()
{
	__asm__("move   #0x2700,%sr\n\t"
			"move.l (0),%a7\n\t"
			"jmp 0x000200");
}*/

extern const unsigned long _sdata[];

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

	vdp_init();
	enable_ints;

	joy_init();

	sprintf(ft232_data, "FT232 DATA: %02x", *FT232_DATA);
	sprintf(ft232_status, "FT232 STATUS: %02x", *FT232_STATUS);

	while (1)
	{
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

		// Button actions
		if (input_pressed & BUTTON_A) // Read Data/Status
		{
			vdp_color(0, 0xf00);
			sprintf(ft232_data, "FT232 DATA: %02x", *FT232_DATA);
			sprintf(ft232_status, "FT232 STATUS: %02x", *FT232_STATUS);
		}
		else if (input_pressed & BUTTON_B) // Write Data
		{
			vdp_color(0, 0x0f0);
			*FT232_DATA = 0x69;
		}
		else if (input_pressed & BUTTON_C) // Write Status
		{
			vdp_color(0, 0x00f);
			*FT232_STATUS = 0x42;
		}
		else
		{
			vdp_color(0, 0x570);
		}

		vdp_vsync();
	};

	return 0;
}
