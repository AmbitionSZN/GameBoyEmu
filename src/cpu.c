#include "cpu.h"
#include "bus.h"
#include "cart.h"
#include <stdint.h>
#include <stdio.h>

void jump(CPU* cpu, Cartridge cart) {
	uint16_t pc;
	uint16_t lo = cart.RomData[cpu->Regs.PC + 1];
	uint16_t hi = cart.RomData[cpu->Regs.PC + 2];
	pc = lo | (hi << 8);
	printf("pc: %2.2X", pc);
}

void fetchInstruction(CPU *cpu, Cartridge cart) {
	uint8_t opcode = busRead(cpu->Regs.PC, cart);
	jump(cpu, cart);
}
