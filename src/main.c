#include "bus.h"
#include "cart.h"
#include "cpu.h"
#include <stdint.h>
#include <stdio.h>

int main() {
	CPU cpu;
	cpu.Regs.PC = 0x101;
	Cartridge cart = LoadCartridge("../roms/Tetris.gb");
	opcodesJsonParser("../Opcodes.json");
	fetchInstruction(&cpu, cart);

}
