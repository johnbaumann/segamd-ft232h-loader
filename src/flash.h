#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CART_BASE 0x000000

extern volatile uint16_t *const cart_flash;
extern volatile uint8_t *const cart_flash8;

void delay(int length);
void FLASH_eraseChip();
void FLASH_eraseSector(uint32_t sector);
bool FLASH_testBypassMode();
bool FLASH_testManufacturerIDMode();

void FLASH_resetBypass();
void FLASH_unlockBypass();

bool FLASH_waitForDQ3Blocking();
bool FLASH_waitForDQ6Blocking();
bool FLASH_waitForProgramBlocking(uint32_t addr, uint16_t data);
bool FLASH_waitForSectorEraseBlocking(uint32_t sector);

bool FLASH_writeSector(uint32_t sector, const uint8_t *data);