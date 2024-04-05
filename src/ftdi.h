#pragma once

#include <stdbool.h>
#include <stdint.h>

#define FT232_BASE 0xA13000

extern volatile uint16_t *const ftdi_data;
extern volatile uint16_t *const ftdi_status;

bool FT_dataReady();
uint8_t FT_status();

uint8_t FT_readByte();
uint8_t FT_readByteBlocking();
void FT_writeByte(uint8_t data);
