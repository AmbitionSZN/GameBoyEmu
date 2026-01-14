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
    while (1) {
        fetchInstruction();
        execute();

    }
}
