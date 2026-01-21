#pragma once

#include "cart.h"
#include <stdint.h>
uint8_t busRead(uint16_t address);
uint16_t busRead16(uint16_t address);

void busWrite(uint16_t address, uint8_t val);
void busWrite16(uint16_t address, uint16_t val);
