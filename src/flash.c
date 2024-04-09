#include "flash.h"

#include <stdbool.h>
#include <stdint.h>
#include "string.h"

extern char test_status[32];
extern const uint8_t FlashData[16384];

volatile uint16_t *const cart_flash = (uint16_t *)(0x000000);
volatile uint8_t *const cart_flash8 = (uint8_t *)(0x000000);

void delay(int length)
{
    int delay = 0;
    while (++delay < length)
    {
        __asm__ volatile("");
    }
    delay = 0;
}

void FLASH_writeWord(uint32_t addr, uint16_t data)
{
    volatile uint16_t *const flash16bit = (uint16_t *)(addr & ~1); // Mask off the low bit to word align
    *flash16bit = data;
}

void FLASH_writeByte(uint32_t addr, uint8_t data)
{
    // A0 not connected, use it to trick /LWR into always strobing
    volatile uint8_t *const flash8bit = (uint8_t *)(addr | 1); // Set the low bit, aligns all writes low
    *flash8bit = data;
}

void FLASH_eraseChip()
{
    // Takes a long time, maybe 20+ seconds?
    FLASH_writeByte(0x555 << 1, 0xaa);
    FLASH_writeByte(0x2aa << 1, 0x55);
    FLASH_writeByte(0x555 << 1, 0x80);
    FLASH_writeByte(0x555 << 1, 0xaa);
    FLASH_writeByte(0x2aa << 1, 0x55);
    FLASH_writeByte(0x555 << 1, 0x10);
}

void FLASH_eraseSector(uint16_t sector)
{
    const uint32_t sector_size = 8 * 1024; // To-do: Get this from the flash chip
    const uint32_t sector_start = sector * sector_size;

    FLASH_writeByte(0x555 << 1, 0xaa);
    FLASH_writeByte(0x2aa << 1, 0x55);
    FLASH_writeByte(0x555 << 1, 0x80);
    FLASH_writeByte(0x555 << 1, 0xaa);
    FLASH_writeByte(0x2aa << 1, 0x55);
    FLASH_writeByte(sector_start, 0x30);
}

void FLASH_unlockBypass()
{
    FLASH_writeByte(0x555 << 1, 0xaa);
    FLASH_writeByte(0x2aa << 1, 0x55);
    FLASH_writeByte(0x555 << 1, 0x20);
}

void FLASH_reset()
{
    FLASH_writeByte(0x100, 0xf0);
}

void FLASH_resetBypass()
{
    FLASH_reset();
    FLASH_writeByte(0x100, 0x90);
    FLASH_writeByte(0x100, 0x00);
}

bool FLASH_writeSector(uint16_t sector, const uint8_t *data, uint16_t length)
{
    // To-do: Check if sector is valid. Also, use length argument
    uint32_t sector_size = 8 * 1024; // To-do: Get this from the flash chip
    const uint32_t sector_address = sector * sector_size;

    if (length > sector_size)
    {
        return false;
    }

    // Unlock and erase sector
    FLASH_unlockBypass();
    FLASH_eraseSector(sector);
    FLASH_waitForDQ6Blocking();

    // Enter program mode - not sure if needed in bypass mode
    FLASH_writeByte(0x555 << 1, 0xaa);
    FLASH_writeByte(0x2aa << 1, 0x55);

    for (uint32_t i = 0; i < sector_size; i+= 2)
    {
        uint16_t towrite = (data[i] << 8 | data[i+1]);
        FLASH_writeByte(0x555 << 1, 0xa0); // Unlock bypass program command
        FLASH_writeWord(sector_address + i, towrite);
        delay(100);
    }

    FLASH_waitForDQ6Blocking();
    FLASH_resetBypass();

    return true;
}

bool FLASH_testBypassMode()
{
    delay(5000); // Easy to spot on LA
    
    for(uint32_t i = 0; i < 2; i++)
    {
        FLASH_writeSector(i, &FlashData[i * 0x2000], 0x1fff);
    }

    delay(5000); // Easy to spot on LA

    return true;
}

inline uint8_t FLASH_getStatus()
{
    return cart_flash8[1]; // Address doesn't matter, just need to read from the flash on /LWR
}

bool FLASH_waitForDQ6Blocking()
{
    uint32_t delay = 0xFFFF; // Widdle this down to a reasonable value

    // START
    // READ DQ7-DQ0
    uint8_t status = FLASH_getStatus();
    uint8_t old_status = status;

    while (--delay > 0) // Hard fail after timeout
    {
        // READ DQ7-DQ0
        status = FLASH_getStatus();
        if ((status & 0x40) == (old_status & 0x40)) // Toggle Bit = Toggle?
        {
            // No
            return true; // Program/Erase Operation Complete
        }
        else
        {
            // Yes
            // DQ5 = 1?
            if ((status & 0x20))
            {
                // Yes
                // Read DQ7-DQ0 Twice
                status = FLASH_getStatus();
                old_status = status;
                status = FLASH_getStatus();
                // Toggle Bit = Toggle?
                if ((status & 0x40) == (old_status & 0x40))
                {
                    // No
                    return true; // Program/Erase Operation Complete
                }
                else
                {
                    // Yes
                    return false; // Program/Erase Operation Not Complete, Write Reset Command
                }
            } // DQ5 != 1, repeat loop
        }
        old_status = status;
    }
    return false;
}
