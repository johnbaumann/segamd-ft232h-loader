#pragma once

#include <stdbool.h>
#include <stdint.h>

extern volatile uint16_t *const ftdi_data;
extern volatile uint16_t *const ftdi_status;

bool FT_dataReady();
bool FT_writeReady();
uint8_t FT_status();

void FT_sendString(const char *inChar);

uint8_t FT_read8();
uint16_t FT_read16();

void FT_write8(uint8_t data);
void FT_write16(uint16_t data);