#include "flash.h"

#include <stdbool.h>
#include <stdint.h>
#include "string.h"

extern char test_status[32];

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

void FLASH_doWrite(uint32_t addr, uint8_t data)
{
    volatile uint16_t *const flash16bit = (uint16_t *)(addr);

    *flash16bit = ((uint16_t)(data << 8) & 0xff00);
}

void FLASH_writeWord(uint32_t addr, uint16_t data)
{
    //volatile uint16_t *const flash16bit = (uint16_t *)((addr == 0) ? (1) : addr);
    volatile uint16_t *const flash16bit = (uint16_t *)(addr & ~1); // Mask off the low bit
    *flash16bit = data;
}

void FLASH_writeByte(uint32_t addr, uint8_t data)
{
    // A0 not connected, use it to trick /LWR into always strobing
    //volatile uint8_t *const flash8bit = (uint8_t *)(((addr % 2) == 0) ? (addr + 1) : addr);
    volatile uint8_t *const flash8bit = (uint8_t *)(addr | 1); // Set the low bit
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
    FLASH_writeByte(0x555 << 1, 0xaa);
    FLASH_writeByte(0x2aa << 1, 0x55);
    FLASH_writeByte(0x555 << 1, 0x80);
    FLASH_writeByte(0x555 << 1, 0xaa);
    FLASH_writeByte(0x2aa << 1, 0x55);
    FLASH_writeByte(sector << 1, 0x30);
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

bool FLASH_testBypassMode()
{
    delay(5000);
    FLASH_unlockBypass();
    FLASH_eraseSector(0);
    FLASH_waitForDQ6Blocking();

    // Enter program mode
    FLASH_writeByte(0x555 << 1, 0xaa);
    FLASH_writeByte(0x2aa << 1, 0x55);


    for (int i = 0; i <= 0x1fff; i+= 2)
    {
        FLASH_writeByte(0x555 << 1, 0xa0);
        FLASH_writeWord(i, 0x1234);
        delay(50);
        /*int timeout = 0x1234;
        while(cart_flash[i] != 0x1234)
        {
            if(--timeout == 0)
            {
                // Timeout
                sprintf(test_status, "Bypass mode test failed timeout byte index %04x", i);
                return false;
            }
        }*/
    }

    FLASH_waitForDQ6Blocking();
    delay(5000);
    FLASH_resetBypass();
    delay(5000);

    uint8_t cartresult = cart_flash8[0x100];

    if (cartresult != 'S')
    {
        sprintf(test_status, "Bypass mode test passed %02x", cartresult);
        return false;
    }
    else
    {
        sprintf(test_status, "Bypass mode test failed %02x", cartresult);
        return true;
    }
}

inline uint8_t FLASH_getStatus()
{
    return cart_flash8[1]; // Address doesn't matter, just need to read from the flash
}

bool FLASH_waitForDQ6Blocking()
{
    uint32_t delay = 0xFFFFFFFF; // Widdle this down to a reasonable value

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
