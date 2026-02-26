#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CART_BASE 0x000000

extern volatile uint16_t *const cart_flash;
extern volatile uint8_t *const cart_flash8;

void delay(int length);
void FLASH_eraseChip();
bool FLASH_testBypassMode();
bool FLASH_testManufacturerIDMode();

void FLASH_resetBypass();
void FLASH_unlockBypass();

unsigned int FLASH_waitForDQ3Blocking();
unsigned int FLASH_waitForDQ6Blocking();
bool FLASH_waitForProgramBlocking(uint32_t address, uint16_t data);
void FLASH_writeProgramBuffered(uint32_t sector, const uint8_t *data);
bool FLASH_waitForSectorEraseBlocking(uint32_t sector);

bool FLASH_writeChunk(uint32_t chunk, const uint16_t *data);
bool FLASH_writeSectorDummy(uint32_t sector);

void FLASH_sectorErase(uint32_t sa);

unsigned int FLASH_testEraseSector(uint32_t sectorStart, uint32_t sectorEnd);
unsigned int FLASH_testWriteSector(uint32_t sector);