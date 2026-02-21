#pragma once

#include <stdint.h>

extern volatile uint8_t *const ftdi_data;
extern volatile uint8_t *const ftdi_status;

uint8_t FT_status();

uint8_t FT_dataReady();
uint8_t FT_writeReady();

void FT_sendString(const char *inChar);

uint8_t FT_read8();
uint16_t FT_read16();
uint32_t FT_read32();

void FT_write8(uint8_t data);
void FT_write16(uint16_t data);
void FT_write32(uint32_t data);

uint16_t FT_read16real();
uint32_t FT_read32real();
void FT_write16real(uint16_t data);