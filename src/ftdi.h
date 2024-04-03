#include <stdint.h>

#define FT232_BASE 0xA13000

static volatile uint16_t *const ftdi_data = (uint16_t *)FT232_BASE + 0;
static volatile uint16_t *const ftdi_status = (uint16_t *)FT232_BASE + 2;