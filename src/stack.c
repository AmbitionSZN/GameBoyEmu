#include "stack.h"
#include "bus.h"
#include "cpu.h"

extern CPU cpu;

void stackPush(uint8_t data) {
    cpu.Regs.SP--;
    busWrite(cpu.Regs.SP, data);
}

void stackPush16(uint16_t data) {
    stackPush((data >> 8) & 0xFF);
    stackPush(data & 0xFF);
}

uint8_t stackPop() {
    return busRead(cpu.Regs.SP++);
}

uint16_t stackPop16() {
    uint16_t lo = stackPop();
    uint16_t hi = stackPop();

    return (hi << 8) | lo;
}
