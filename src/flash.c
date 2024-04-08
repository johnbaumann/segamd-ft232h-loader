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
    volatile uint16_t *const flash16bit = (uint16_t *)((addr == 0) ? (1) : addr);
    *flash16bit = data;
}

void FLASH_writeByte(uint32_t addr, uint8_t data)
{
    volatile uint8_t *const flash8bit = (uint8_t *)(((addr % 2) == 0) ? (addr + 1) : addr);
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

void FLASH_eraseChip2()
{
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
    FLASH_writeByte(0, 0xf0);
    FLASH_writeByte(0, 0x90);
    FLASH_writeByte(0, 0x00);
}

bool FLASH_testBypassMode()
{
    uint32_t delay = 0xFFFF;

    //FLASH_eraseChip();
    FLASH_eraseSector(0);

    /*for (int i = (0x100/2); i <= ((0x100/2) + (4096/2)); i++)
    {
        cart_flash[1] = 0xA0;
        cart_flash[i] = 0x1234;
    }
    while (++delay < 5000)
    {
        __asm__ volatile("");
    }
    delay = 0;
    FLASH_resetBypass();
    while (++delay < 5000)
    {
        __asm__ volatile("");
    }
    delay = 0;*/

    int dq7 = cart_flash8[1];

    while (--delay > 0)
    {
        if (dq7 == 0xff)
        {
            FLASH_reset();
            sprintf(test_status, "Flash data changed, delay %lu", delay);
            return true;
        }
        __asm__ volatile("");
        dq7 = cart_flash8[1];
    }

    sprintf(test_status, "Flash data unchanged: %04x %04x", cart_flash[0x80], cart_flash[0x81]);
    FLASH_reset();

    return false;
}
