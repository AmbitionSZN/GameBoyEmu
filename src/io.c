#include "io.h"
#include "bus.h"
#include "cpu.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

extern uint8_t memory[0x10000];

static uint16_t div = 0xAC00;
static uint8_t *tima = &memory[0xFF05];
static uint8_t *tma = &memory[0xFF06];
static uint8_t *tac = &memory[0xFF07];

uint8_t ioRead(uint16_t address) {
	switch(address) {
		case 0xFF04:
			return (div >> 8);
		case 0xFF44:
			return 0x90;
		default:
			return memory[address];
	}
}



void ioWrite(uint16_t address, uint8_t data) {
	switch (address) {
		case 0xFF04:
			div = 0;
			break;
		case 0xFF46:
			DMATransfer(address, data);
		default:
			memory[address] = data;
	}
}

void timerTick() {
    uint16_t prevDiv = div;
    div += 1;
    bool timerUpdate = false;

    switch (*tac & 0b11) {
    case 0b00:
        timerUpdate = prevDiv & (1 << 9) && (!(div & (1 << 9)));
        break;
    case 0b01:
        timerUpdate = prevDiv & (1 << 3) && (!(div & (1 << 3)));
        break;
    case 0b10:
        timerUpdate = prevDiv & (1 << 5) && (!(div & (1 << 5)));
        break;
    case 0b11:
        timerUpdate = prevDiv & (1 << 7) && (!(div & (1 << 7)));
        break;
    }
	
	if (timerUpdate && *tac & (1 << 2)) {
		*tima += 1;

		if (*tima == 0xFF) {
			*tima = *tma;
			requestInterrupt(INT_TIMER);	
		}
	}

}

void DMATransfer(uint16_t dest, uint16_t src) {
	src *= 0x100;
	uint8_t data = busRead(src);
	busWrite(dest, data);

}
