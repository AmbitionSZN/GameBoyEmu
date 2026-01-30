#pragma once
#include <stdint.h>

uint8_t ioRead(uint16_t address);
void ioWrite(uint16_t address, uint8_t value);
void timerTick();
