#include "flash.h"

#include <stdbool.h>
#include <stdint.h>
#include "string.h"

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

void FLASH_writeWord(uint32_t address, uint16_t data)
{
    volatile uint16_t *const flash16bit = (uint16_t *)(address & ~1); // Mask off the low bit to word align
    *flash16bit = data;
}

void FLASH_writeByte(uint32_t address, uint8_t data)
{
    // A0 not connected, use it to trick /LWR into always strobing
    volatile uint8_t *const flash8bit = (uint8_t *)(address | 1); // Set the low bit, aligns all writes low
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

void FLASH_eraseSector(uint32_t sector)
{
    const uint32_t sector_size = 8U * 1024U; // To-do: Get this from the flash chip
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

// Not working yet
void FLASH_writeProgramBuffered(uint8_t *data, uint32_t address, uint32_t length)
{
    // Unlock
    FLASH_writeByte(0x555 << 1, 0xaa);
    FLASH_writeByte(0x2aa << 1, 0x55);

    // Write to buffer
    FLASH_writeByte(address, 0x25);

    // Write WC
    FLASH_writeWord(address, length - 1);

    // Write bytes to buffer
    // To-do: Change to word writes
    for (uint32_t i = 0; i < length; i++)
    {
        FLASH_writeByte(address + i, data[i]);
    }

    // Program buffer to flash
    FLASH_writeByte(address, 0x29);
    FLASH_waitForDQ6Blocking();
}

bool FLASH_writeSector(uint32_t sector, const uint8_t *data)
{
    // To-do: Check if sector is valid. Also, use length argument
    const uint32_t sector_size = 8U * 1024U; // To-do: Get this from the flash chip
    const uint32_t sector_address = sector * sector_size;

    // Unlock and erase sector
    FLASH_unlockBypass();
    if (sector < 8 || (sector > 8 && (sector % 8) == 0))
    {
        FLASH_eraseSector(sector);
        FLASH_waitForDQ6Blocking();
        FLASH_waitForDQ3Blocking();
        FLASH_waitForSectorEraseBlocking(sector);
    }

    for (uint32_t i = 0; i < sector_size; i += 2)
    {
        uint16_t towrite = ((data[i] << 8) | data[i + 1]);

        FLASH_writeByte(0x555 << 1, 0xa0); // Unlock bypass program command
        FLASH_writeWord(sector_address + i, towrite);
        FLASH_waitForProgramBlocking(sector_address + i, towrite);
    }

    FLASH_waitForDQ6Blocking();
    FLASH_resetBypass();

    return true;
}

bool FLASH_writeSectorDummy(uint32_t sector)
{
    // To-do: Check if sector is valid. Also, use length argument
    const uint32_t sector_size = 8U * 1024U; // To-do: Get this from the flash chip
    const uint32_t sector_address = sector * sector_size;

    // Unlock and erase sector
    FLASH_unlockBypass();
    if (sector < 8 || (sector > 8 && (sector & 1) == 0))
    {
        FLASH_eraseSector(sector);
        FLASH_waitForDQ6Blocking();
        FLASH_waitForDQ3Blocking();
        FLASH_waitForSectorEraseBlocking(sector);
    }

    for (uint16_t i = 0; i < sector_size; i += 2)
    {
        FLASH_writeByte(0x555 << 1, 0xa0); // Unlock bypass program command
        FLASH_writeWord(sector_address + i, i);
        FLASH_waitForProgramBlocking(sector_address + i, i);
    }

    FLASH_waitForDQ6Blocking();
    FLASH_resetBypass();

    return true;
}

inline uint8_t FLASH_getStatus()
{
    return cart_flash8[1]; // Address doesn't matter, just need to read from the flash on /LWR
}

bool FLASH_waitForDQ3Blocking()
{
    uint32_t delay = 0xFFFF; // To-do: Widdle this down to a reasonable value

    while (--delay > 0) // Hard fail after timeout
    {
        // READ DQ7-DQ0
        uint8_t status = FLASH_getStatus();
        // DQ3 = 1?
        if ((status & (1 << 3)))
        {
            // Yes
            return true; // Program/Erase Operation Complete
        }
    }

    return false; // Timed out
}

bool FLASH_waitForDQ6Blocking()
{
    uint32_t delay = 0xFFFF; // To-do: Widdle this down to a reasonable value

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

bool FLASH_waitForSectorEraseBlocking(uint32_t sector)
{
    const uint32_t sector_size = 8U * 1024U; // To-do: Get this from the flash chip
    const uint32_t sector_start = sector * sector_size;
    const uint32_t sector_end = sector_start + sector_size - 1U;

    uint32_t delay = 0xFFFF; // To-do: Widdle this down to a reasonable value

    while (--delay > 0) // Hard fail after timeout
    {
        if (cart_flash8[sector_end] == 0xFF)
        {
            return true; // Sector erased
        }
    }

    return false; // Timed out
}

bool FLASH_waitForProgramBlocking(uint32_t address, uint16_t data)
{
    volatile uint16_t *const flash16bit = (uint16_t *)(address & ~1);
    uint32_t delay = 0xFFFF; // To-do: Widdle this down to a reasonable value

    while (--delay > 0) // Hard fail after timeout
    {
        if (*flash16bit == data) // Compare the data
        {
            return true; // Programmed
        }
    }

    return false; // Timed out
}
