#include "ftdi.h"

#include <stdbool.h>
#include <stdint.h>

enum FT_STATUS : uint8_t
{
    FT_STATUS_DATA_READY = 0x01,
    FT_STATUS_WRITE_READY = 0x02,
};

// To do, make static and remove extern from header
volatile uint16_t *const ftdi_data = (uint16_t *)(FT232_BASE + 0);
volatile uint16_t *const ftdi_status = (uint16_t *)(FT232_BASE + 2);

uint8_t FT_status()
{
    return (*ftdi_status & 0xf); // We only care about the lower 4 bits
}

bool FT_dataReady()
{
    return (FT_status() & FT_STATUS_DATA_READY);
}

bool FT_writeReady()
{
    return (FT_status() & FT_STATUS_WRITE_READY);

}

uint8_t FT_readByte()
{
    return (*ftdi_data & 0xff);
}

uint8_t FT_readByteBlocking()
{
    while (!FT_dataReady())
    {
        ; // Wait for data
    }

    return (*ftdi_data & 0xff);
}

void FT_writeByte(uint8_t data)
{
    *ftdi_data = data;
}

void FT_writeByteBlocking(uint8_t data)
{
    while (!FT_writeReady())
    {
        ; // Wait for write
    }

    *ftdi_data = data;
}