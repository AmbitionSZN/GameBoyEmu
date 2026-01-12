#include "bus.h"
#include "cart.h"
#include "cpu.h"
#include <stdint.h>
#include <stdio.h>

int main() {
    CPU cpu;
    cpu.Regs.PC = 0x100;
	cpu.Regs.A = 0x01;
    Cartridge cart = LoadCartridge("../roms/Tetris.gb");
    opcodesJsonParser("../Opcodes.json");
	while (1) {
    fetchInstruction(&cpu, &cart);
	execute(&cpu, &cart);
	}
}
