#include "bus.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern uint8_t memory[0xFFFF];

uint8_t busRead(uint16_t address) {

	return memory[address];
}

void busWrite(uint16_t address, uint8_t val) {
	memory[address] = val; 
}
