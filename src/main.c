#include "bus.h"
#include "cart.h"
#include "cpu.h"
#include <stdint.h>
#include <stdio.h>

CPU cpu;
Cartridge cart;

int main() {
    cpu.Regs.PC = 0x100;
    cpu.Regs.A = 0x01;
    cart = LoadCartridge("../roms/Tetris.gb");
    opcodesJsonParser("../Opcodes.json");
	uint8_t num = 255;
	
    uint16_t max = ((uint16_t)num) | ((uint16_t)num << 8);
	printf("max = %d", max);
	return 1;
    while (1) {
        fetchInstruction();
        fetchData();
        execute();
    }
}
