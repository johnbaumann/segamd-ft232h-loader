#include "flash.h"

#include <stdbool.h>
#include <stdint.h>

volatile uint16_t *const cart_flash = (uint16_t *)(0x000000);
volatile uint8_t *const cart_flash8 = (uint8_t *)(0x000000);

void delay(int length) {
    int delay = 0;
    while (++delay < length) {
        __asm__ volatile("");
    }
    delay = 0;
}

uint32_t getSectorAddress(uint32_t sector) {
    if (sector <= 7) {
        return sector * 8u * 1024;
    } else {
        return ((sector - 8) * 64u * 1024) + (8u * 8u * 1024u);
    }
}

uint32_t getSectorSize(uint32_t sector) {
    if (sector <= 7) {
        return (8U * 1024U);
    } else {
        return (64U * 1024U);
    }
}

void FLASH_writeWord(uint32_t address, uint16_t data) {
    volatile uint16_t *const flash16bit = (uint16_t *)(address & ~1);  // Mask off the low bit to word align
    *flash16bit = data;
}

void FLASH_writeByte(uint32_t address, uint8_t data) {
    // A0 not connected, use it to trick /LWR into always strobing
    volatile uint8_t *const flash8bit = (uint8_t *)(address | 1);  // Set the low bit, aligns all writes low
    *flash8bit = data;
}

// Reset - 1 cycle
void FLASH_reset() { FLASH_writeByte(0x100, 0xf0); }

// Autoselect commands
// -Manufacturer ID - 4 cycles
// -Device ID - 6 cycles
// -Device ID - 4 cycles
// -Secured Silicon Sector Factor Protect - 4 cycles

// Enter Secured Silicon Sector Region - 3 cycles
// Exit Secured Silicon Sector Region - 4 cycles

// Program - 4 cycles
void FLASH_program(uint32_t pa, uint16_t pd) {
    FLASH_writeByte(0x555 << 1, 0xaa);
    FLASH_writeByte(0x2aa << 1, 0x55);
    FLASH_writeByte(0x555 << 1, 0xa0);
    cart_flash[pa] = pd;
}

// Write to Buffer - 3 cycles
// Program Buffer to Flash - 1 cycle
// Write to Buffer Abort Reset - 3 cycles

// Unlock Bypass - 3 cycles
void FLASH_unlockBypass() {
    FLASH_writeByte(0x555 << 1, 0xaa);
    FLASH_writeByte(0x2aa << 1, 0x55);
    FLASH_writeByte(0x555 << 1, 0x20);
}

// Unlock Bypass Program - 2 cycles
void FLASH_unlockBypassProgram(uint32_t pa, uint16_t pd) {
    FLASH_writeByte(0x100 << 1, 0xa0);
    cart_flash[pa] = pd;
}

// Unlock Bypass Reset - 2 cycles
void FLASH_unlockResetBypass() {
    FLASH_reset();
    FLASH_writeByte(0x100 << 1, 0x90);
    FLASH_writeByte(0x100 << 1, 0x00);
}

// Chip Erase - 6 cycles
void FLASH_chipErase() {
    FLASH_writeByte(0x555 << 1, 0xaa);
    FLASH_writeByte(0x2aa << 1, 0x55);
    FLASH_writeByte(0x555 << 1, 0x80);
    FLASH_writeByte(0x555 << 1, 0xaa);
    FLASH_writeByte(0x2aa << 1, 0x55);
    FLASH_writeByte(0x555 << 1, 0x10);
}

// Sector Erase - 6 cycles
void FLASH_sectorErase(uint32_t sa) {
    FLASH_writeByte(0x555 << 1, 0xaa);
    FLASH_writeByte(0x2aa << 1, 0x55);
    FLASH_writeByte(0x555 << 1, 0x80);
    FLASH_writeByte(0x555 << 1, 0xaa);
    FLASH_writeByte(0x2aa << 1, 0x55);
    FLASH_writeByte(sa, 0x30);
}

// Program/Erase Suspend - 1 cycle
// Program/Erase Resume - 1 cylce
// CFI Query - 1 cycle

uint8_t FLASH_getStatus() {
    return cart_flash8[1];  // Address doesn't matter, just need to read from the flash on /LWR
}

unsigned int FLASH_waitForDQ3Blocking() {
    uint32_t delay = 0xFFFF;  // To-do: Widdle this down to a reasonable value

    while (--delay > 0)  // Hard fail after timeout
    {
        // READ DQ7-DQ0
        uint8_t status = FLASH_getStatus();
        // DQ3 = 1?
        if ((status & (1 << 3))) {
            // Yes
            return delay;  // Program/Erase Operation Started
        }
    }

    return delay;  // Timed out
}

unsigned int FLASH_waitForDQ6Blocking() {
    uint32_t delay = 0xFFFF;

    // START
    // READ DQ7-DQ0
    uint8_t status = FLASH_getStatus();
    uint8_t old_status = status;

    while (--delay > 0)  // Hard fail after timeout
    {
        // READ DQ7-DQ0
        status = FLASH_getStatus();
        if ((status & 0x40) == (old_status & 0x40))  // Toggle Bit = Toggle?
        {
            // No
            return delay;  // Program/Erase Operation Complete
        } else {
            // Yes
            // DQ5 = 1?
            if ((status & 0x20)) {
                // Yes
                // Read DQ7-DQ0 Twice
                status = FLASH_getStatus();
                old_status = status;
                status = FLASH_getStatus();
                // Toggle Bit = Toggle?
                if ((status & 0x40) == (old_status & 0x40)) {
                    // No
                    return delay;  // Program/Erase Operation Complete
                } else {
                    // Yes
                    return delay;  // Program/Erase Operation Not Complete, Write Reset Command
                }
            }  // DQ5 != 1, repeat loop
        }
        old_status = status;
    }
    return delay;
}

unsigned int FLASH_testEraseSector(uint32_t sectorStart, uint32_t sectorEnd) {
    // Reset the unlock bypass to make sure chip is ready for erase command
    FLASH_unlockResetBypass();

    for (unsigned int i = sectorStart; i <= sectorEnd; i++) {
        const uint32_t sectorAddress = getSectorAddress(i);
        FLASH_sectorErase(sectorAddress);
    }

    // Wait for Sector Erase timer to expire
    FLASH_waitForDQ3Blocking();

    return FLASH_waitForDQ6Blocking();
}

unsigned int FLASH_testWriteSector(uint32_t sector) {
    // To-do: Check if sector is valid. Also, use length argument
    const uint32_t sector_address = getSectorAddress(sector);
    const uint32_t sectorSize = getSectorSize(sector);

    // Unlock and erase sector
    FLASH_unlockResetBypass();
    FLASH_sectorErase(sector_address);
    FLASH_waitForDQ3Blocking();
    FLASH_waitForDQ6Blocking();

    FLASH_unlockBypass();
    for (uint16_t i = 0; i < sectorSize; i += 2) {
        FLASH_writeByte(0x555 << 1, 0xa0);  // Unlock bypass program command
        FLASH_writeWord(sector_address + i, i);
        FLASH_waitForProgramBlocking(sector_address, i);
    }

    FLASH_waitForDQ6Blocking();
    FLASH_unlockResetBypass();

    return true;
}

bool FLASH_writeChunk(uint32_t chunk, const uint16_t *data) {
    const uint32_t chunkSize = 8U * 1024U;  // To-do: Get this from the flash chip
    const uint32_t writeAddress = chunk * chunkSize;

    FLASH_unlockResetBypass();

    // Erase sector
    if (chunk < 8 || (chunk >= 8 && (chunk % 8) == 0)) {
        FLASH_sectorErase(writeAddress);
        FLASH_waitForDQ3Blocking();
        FLASH_waitForDQ6Blocking();
    }

    FLASH_unlockBypass();

    for (uint32_t i = 0; i < chunkSize / 2; i++) {
        FLASH_writeByte(0x555 << 1, 0xa0);  // Unlock bypass program command
        FLASH_writeWord(writeAddress + (i * 2), data[i]);
        FLASH_waitForProgramBlocking(writeAddress + (i * 2), data[i]);
        //delay(100);
    }
    FLASH_waitForDQ6Blocking();

    // Reset the unlock bypass to make sure chip is ready for erase command
    FLASH_unlockResetBypass();

    return true;
}

bool FLASH_waitForSectorEraseBlocking(uint32_t sector) {
    const uint32_t sector_start = getSectorAddress(sector);
    const uint32_t sector_end = sector_start + getSectorSize(sector) - 1U;

    uint32_t delay = 0xFFFF;  // To-do: Widdle this down to a reasonable value

    while (--delay > 0)  // Hard fail after timeout
    {
        if (cart_flash8[sector_end] == 0xFF) {
            return true;  // Sector erased
        }
    }

    return false;  // Timed out
}

bool FLASH_waitForProgramBlocking(uint32_t address, uint16_t data) {
    volatile uint16_t *const flash16bit = (uint16_t *)(address & ~1);
    uint32_t delay = 0xFF;  // To-do: Widdle this down to a reasonable value

    while (--delay > 0)  // Hard fail after timeout
    {
        if (*flash16bit == data)  // Compare the data
        {
            return true;  // Programmed
        }
    }

    return false;  // Timed out
}