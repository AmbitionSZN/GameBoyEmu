#pragma once

#include "cart.h"
#include <stdint.h>
uint8_t busRead(uint16_t address);

void busWrite(uint16_t address, uint8_t val);
