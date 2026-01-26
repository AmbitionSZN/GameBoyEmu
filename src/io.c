#include "io.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern uint8_t memory[0x10000];
static char serialData[2];

uint8_t ioRead(uint16_t address) {
    if (address == 0xFF01) {
        return serialData[0];
    }

    if (address == 0xFF02) {
        return serialData[1];
    }

	return memory[address];
}

void ioWrite(uint16_t address, uint8_t val) {
    if (address == 0xFF01) {
        serialData[0] = val;
        return;
    }

    if (address == 0xFF02) {
        serialData[1] = val;
        return;
    }
	
	memory[address] = val;
}
