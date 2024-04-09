#include "ftdi.h"

#include <stdbool.h>
#include <stdint.h>

#define FT_STATUS_DATA_AVAILABLE 0x01  // RXF
#define FT_STATUS_SPACE_AVAILABLE 0x02 // TXE
#define FT_STATUS_SUSPEND 0x04         // SUSP
#define FT_STATUS_CONFIGURED 0x08      // CONFIG

#define FT232_BASE 0xa13000

// To do, make static and remove extern from header
volatile uint16_t *const ftdi_data = (uint16_t *)(FT232_BASE + 0);
volatile uint16_t *const ftdi_status = (uint16_t *)(FT232_BASE + 2);

uint8_t FT_status()
{
    return (*ftdi_status);
}

bool FT_dataReady()
{
    return (FT_status() & FT_STATUS_DATA_AVAILABLE);
}

bool FT_writeReady()
{
    return (FT_status() & FT_STATUS_SPACE_AVAILABLE);
}

void FT_sendString(const char *inChar)
{
    while (*inChar)
    {
        FT_write8(*inChar++);
    }
}

uint8_t FT_read8()
{
    while (!FT_dataReady())
    {
        ; // Wait for data
    }

    return (*ftdi_data & 0xff);
}

uint16_t FT_read16()
{
    uint32_t value = 0;

    value |= FT_read8();
    value |= FT_read8() << 8;

    return value;
}

void FT_write8(uint8_t data)
{
    while (!FT_writeReady())
    {
        ; // Wait for write
    }

    *ftdi_data = data;
}

void FT_write16(uint16_t data)
{
    FT_write8(data & 0xff);
    FT_write8(data >> 8);
}
