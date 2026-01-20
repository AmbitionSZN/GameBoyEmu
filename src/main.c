#include "bus.h"
#include "cart.h"
#include "cpu.h"
#include <stdint.h>
#include <stdio.h>

CPU cpu;
uint8_t memory[0xFFFF];
Cartridge cart;

int main() {
    cpu.Regs.PC = 0x100;
    cpu.Regs.A = 0x01;
    bool isNextInstr = false;
    cart = LoadCartridge("../roms/Tetris.gb");
    opcodesJsonParser("../Opcodes.json");
    while (1) {
        fetchInstruction();
        fetchData();
        execute();
        if (cpu.EnableIME) {
            if (isNextInstr == true) {
                cpu.IMEFlag = true;
                isNextInstr = true;
            } else {
                isNextInstr = false;
            }
        }
    }
}
