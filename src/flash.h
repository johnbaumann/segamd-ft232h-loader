#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CART_BASE 0x000000

extern volatile uint16_t *const cart_flash;

void delay(int length);
void FLASH_eraseChip();
void FLASH_eraseSector(uint16_t sector);
bool FLASH_testBypassMode();
bool FLASH_testManufacturerIDMode();

void FLASH_resetBypass();
void FLASH_unlockBypass();

bool FLASH_waitForDQ6Blocking();
