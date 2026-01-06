#include "bus.h"
#include "cart.h"
#include "cpu.h"
#include <stdint.h>
#include <stdio.h>

int main() {
	CPU cpu;
	cpu.Regs.PC = 0x101;
	Cartridge cart = LoadCartridge("../roms/Tetris.gb");
	
	uint8_t opcode = busRead(cpu.Regs.PC, cart);
//	printf("opcode: %2.2X\n", opcode);
	fetchInstruction(&cpu, cart);


}
