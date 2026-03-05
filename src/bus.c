#include "bus.h"
#include "cart.h"
#include "cpu.h"
#include "io.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern uint8_t memory[0x10000];
extern CPU cpu;
extern DMA dma;

uint8_t busRead(uint16_t address) {
    if (address < 0x8000) {
        // ROM Data
        return cartRead(address);
    } else if (address < 0xA000) {
        // Char/Map Data
		return memory[address];
    } else if (address < 0xC000) {
        // Cartridge RAM
		return cartRead(address);
    } else if (address < 0xE000) {
        // WRAM (Working RAM)
        return memory[address];
    } else if (address < 0xFE00) {
        // reserved echo ram...
        return 0;
    } else if (address < 0xFEA0) {
        // OAM
		if (dma.Active) {
			return 0xFF;
		}
        return memory[address];
    } else if (address < 0xFF00) {
        // reserved unusable...
        return 0;
    } else if (address < 0xFF80) {
        // io
        return ioRead(address);
    } else if (address < 0xFFFF) {
        // hram
        return memory[address];
    } else if (address == 0xFFFF) {
        // CPU ENABLE REGISTER...
        return memory[address];
    }
    printf("error in busRead");
    exit(EXIT_FAILURE);
}

uint16_t busRead16(uint16_t address) {
    uint16_t lo = busRead(address);
    uint16_t hi = busRead(address + 1);
    return (lo | hi << 8);
}

void busWrite(uint16_t address, uint8_t data) {
    if (address < 0x8000) {
        // ROM Data
		cartWrite(address, data);
        return;
    } else if (address < 0xA000) {
        // Char/Map Data
		memory[address] = data;
		return;
    } else if (address < 0xC000) {
        // EXT-RAM
        cartWrite(address, data);
        return;
    } else if (address < 0xE000) {
        // WRAM
        memory[address] = data;
        return;
    } else if (address < 0xFE00) {
        // reserved echo ram
    } else if (address < 0xFEA0) {
        // OAM
		if (!dma.Active) {
			memory[address] = data;
		}
		return;
    } else if (address < 0xFF00) {
        // unusable reserved
    } else if (address < 0xFF80) {
        // IO Registers...
        ioWrite(address, data);
        return;
    } else if (address < 0xFFFF) {
        // HRam
        memory[address] = data;
        return;
    } else if (address == 0xFFFF) {
        // CPU enable register
        memory[address] = data;
        return;
    } else {
        printf("error in busWrite\n");
        exit(EXIT_FAILURE);
    }
}

void busWrite16(uint16_t address, uint16_t data) {
    busWrite(address, (data & 0xFF));
    busWrite(address + 1, (data >> 8) & 0xFF);
}
