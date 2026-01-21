#include "instructions.h"
#include "bus.h"
#include "cart.h"
#include "cpu.h"
#include "stack.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern CPU cpu;

void JP() {
    bool b;
    switch (cpu.CurInstr->Operand1) {
    case DT_HL:
        cpu.Regs.PC = readRegisterU16(DT_HL);
        return;
    case DT_A16:
        b = true;
        break;
    case DT_CC_Z ... DT_CC_NC:
        b = CheckCondition(cpu.CurInstr->Operand1);
        break;
    default:
        printf("error in JP\n");
        printf("%i\n", cpu.CurInstr->Operand1);

        exit(EXIT_FAILURE);
    }
    if (b) {
        uint16_t lo = cpu.InstrData[0];
        uint16_t hi = cpu.InstrData[1];
        cpu.Regs.PC = lo | (hi << 8);
    }
}

void PUSH() {
    switch (cpu.CurInstr->Operand1) {
    case DT_AF:
        stackPush16(readRegisterU16(DT_AF));
        break;
    case DT_BC ... DT_HL: {
        stackPush16(readRegisterU16(cpu.CurInstr->Operand1));
        break;
    }
    default:
        printf("error in push\n");
        exit(EXIT_FAILURE);
    }
}

void POP() {
    switch (cpu.CurInstr->Operand1) {
    case DT_AF:
        writeRegisterU16(DT_AF, stackPop16());
        break;
    case DT_BC ... DT_HL: {
        writeRegisterU16(cpu.CurInstr->Operand1, stackPop16());
        break;
    }
    default:
        printf("error in pop\n");
        exit(EXIT_FAILURE);
    }
}

void CALL() {
    uint16_t lo = cpu.InstrData[0];
    uint16_t hi = cpu.InstrData[1];
    uint16_t *pc = &cpu.Regs.PC;
    switch (cpu.CurInstr->Operand1) {
    case DT_A16:
        stackPush16(*pc);
        *pc = (lo | (hi << 8));
        break;
    case DT_CC_Z ... DT_CC_NC:
        if (CheckCondition(cpu.CurInstr->Operand1)) {
            stackPush16(*pc);
            *pc = (lo | (hi << 8));
        }
        break;
    default:
        printf("error in call\n");
        exit(EXIT_FAILURE);
    }
}

void RET() {
    switch (cpu.CurInstr->Operand1) {
    case DT_NONE: {
        writeRegisterU16(DT_PC, stackPop16());
        break;
    }
    case DT_CC_Z ... DT_CC_NC:
        if (CheckCondition(cpu.CurInstr->Operand1)) {
            writeRegisterU16(DT_PC, stackPop16());
        }
        break;
    default:
        printf("error in ret");
        exit(EXIT_FAILURE);
    }
}

void DI() { cpu.IMEFlag = false; }
void EI() { cpu.EnableIME = true; }

void OR() {
    CPURegisters *regs = &cpu.Regs;
    switch (cpu.CurInstr->Operand2) {
    case DT_N8:
        regs->A |= cpu.InstrData[0];
        break;
    case DT_A:
    case DT_B:
    case DT_C:
    case DT_D:
    case DT_E:
    case DT_H:
    case DT_L:
        regs->A |= *getRegisterU8(cpu.CurInstr->Operand2);
        break;
    case DT_A_HL:
        regs->A |= busRead(readRegisterU16(cpu.CurInstr->Operand2));
        break;
    default:
        printf("error in XOR");
        exit(EXIT_FAILURE);
    }
    if (regs->A == 0) {
        regs->F |= FLAG_Z;
    }
    regs->F &= ~FLAG_N;
    regs->F &= ~FLAG_H;
    regs->F &= ~FLAG_C;
}

void XOR() {
    CPURegisters *regs = &cpu.Regs;
    switch (cpu.CurInstr->Operand2) {
    case DT_N8:
        regs->A ^= cpu.InstrData[0];
        break;
    case DT_A ... DT_L:
        regs->A ^= *getRegisterU8(cpu.CurInstr->Operand2);
        break;
    case DT_A_HL:
        regs->A ^= busRead(readRegisterU16(cpu.CurInstr->Operand2));
        break;
    default:
        printf("error in XOR");
        exit(EXIT_FAILURE);
    }
    if (regs->A == 0) {
        regs->F |= FLAG_Z;
    }
    regs->F &= ~FLAG_N;
    regs->F &= ~FLAG_H;
    regs->F &= ~FLAG_C;
}

void AND() {
    CPURegisters *regs = &cpu.Regs;
    switch (cpu.CurInstr->Operand2) {
    case DT_N8:
        regs->A &= cpu.InstrData[0];
        break;
    case DT_A ... DT_L:
        regs->A &= *getRegisterU8(cpu.CurInstr->Operand2);
        break;
    case DT_A_HL:
        regs->A &= busRead(readRegisterU16(cpu.CurInstr->Operand2));
        break;
    default:
        printf("error in AND");
        exit(EXIT_FAILURE);
    }
    if (regs->A == 0) {
        regs->F |= FLAG_Z;
    }
    regs->F &= ~FLAG_N;
    regs->F |= FLAG_H;
    regs->F &= ~FLAG_C;
}



void LD() {
    CPURegisters *regs = &cpu.Regs;
    Instruction *instr = cpu.CurInstr;
    uint8_t *op1 = NULL;
    uint8_t op2 = 0;
    uint16_t *op1U16 = NULL;
    uint16_t op2U16 = 0;

    switch (instr->Operand2) {
    case DT_A ... DT_L:
        op2 = *getRegisterU8(instr->Operand2);
        break;
    case DT_A16:
        op2 = busRead((uint16_t)cpu.InstrData[0] |
                      ((uint16_t)cpu.InstrData[1] << 8));
        break;
    case DT_A_AF ... DT_A_HLD:
        op2 = busRead(readRegisterU16(instr->Operand2));
        break;
    case DT_N8:
        op2 = cpu.InstrData[0];
        break;
    case DT_N16:
        op2U16 = (uint16_t)cpu.InstrData[0] | (uint16_t)(cpu.InstrData[1] << 8);
        break;
    case DT_AF ... DT_HL:
        op2U16 = readRegisterU16(instr->Operand2);
        break;
    case DT_PC:
        op2U16 = readRegisterU16(instr->Operand2);
        break;
    case DT_SP: {
        if (instr->Operand1 == DT_HL) {
            op2U16 = readRegisterU16(instr->Operand2);
            break;
        }
        uint16_t regVal = readRegisterU16(DT_SP);
        int8_t offset = ((int8_t *)cpu.InstrData)[0];
        if ((regVal & 0xF) + (offset & 0xF) > 0xF) {
            regs->F |= FLAG_H;
        }
        if (regVal + offset > 0xFF) {
            regs->F |= FLAG_C;
        }

        op2U16 = regVal + offset;
        break;
    }
    default:
        printf("error in LD\n");
        exit(EXIT_FAILURE);
    }

    switch (instr->Operand1) {
    case DT_A ... DT_L:
        op1 = getRegisterU8(instr->Operand1);
        *op1 = op2;
        break;
    case DT_A_AF ... DT_A_HLD:
        busWrite(readRegisterU16(instr->Operand1), op2);
        break;
    case DT_A16: {
        uint16_t addr =
            ((uint16_t)cpu.InstrData[0]) | ((uint16_t)cpu.InstrData[1] << 8);
        if (instr->Operand2 == DT_SP) {
            uint16_t lo = regs->SP & 0xFF;
            uint16_t hi = regs->SP >> 8;
            busWrite16(addr, lo | (hi << 8));
            break;
        }
        busWrite(addr, op2);
        break;
    }
    case DT_AF ... DT_HLD:
        writeRegisterU16(instr->Operand1, op2U16);
        break;
    default:
        printf("error in LD\n");
        exit(EXIT_FAILURE);
    }
    if (instr->Operand1 == DT_HLI || instr->Operand2 == DT_HLI ||
        instr->Operand1 == DT_A_HLI || instr->Operand2 == DT_A_HLI) {
        writeRegisterU16(DT_HL, readRegisterU16(DT_HL) + 1);
    } else if (instr->Operand1 == DT_HLD || instr->Operand2 == DT_HLD ||
               instr->Operand1 == DT_A_HLD || instr->Operand2 == DT_A_HLD) {
        writeRegisterU16(DT_HL, readRegisterU16(DT_HL) - 1);
    }
}

void LDH() {
    uint8_t data;
    switch (cpu.CurInstr->Operand2) {
    case DT_A8:
        data = busRead(0xFF00 + (uint16_t)data);
        break;
    case DT_A_C:
        data = busRead(0xFF00 + (uint16_t)cpu.Regs.C);
        break;
    case DT_A:
        data = cpu.Regs.A;
        break;
    default:
        printf("error in LDH");
        exit(EXIT_FAILURE);
    }
    switch (cpu.CurInstr->Operand1) {
    case DT_A8:
        busWrite((0xFF00 + (uint16_t)cpu.InstrData[0]), data);
        break;
    case DT_A_C:
        busWrite((0xFF00 + (uint16_t)cpu.Regs.C), data);
        break;
    case DT_A:
        cpu.Regs.A = data;
        break;
    default:
        printf("error in LDH");
        exit(EXIT_FAILURE);
    }
}

void DEC() {
    Instruction *instr = cpu.CurInstr;
    CPURegisters *regs = &cpu.Regs;
    switch (instr->Operand1) {
    case DT_A ... DT_L: {
        uint8_t *reg = getRegisterU8(instr->Operand1);
        if (((int)*reg & 0xF) - 1 < 0) {
            regs->F |= FLAG_H;
        }
        *reg -= 1;
        if (*reg == 0) {
            regs->F |= FLAG_Z;
        }
        regs->F |= FLAG_N;
        break;
    }
    case DT_BC ... DT_PC: {
        writeRegisterU16(instr->Operand1, readRegisterU16(instr->Operand1) - 1);
        break;
    }
    case DT_A_HL: {
        uint16_t addr = readRegisterU16(instr->Operand1);
        uint8_t op1 = busRead(addr);
        if (((int)op1 & 0xF) - 1 < 0) {
            regs->F |= FLAG_H;
        }
        busWrite(addr, op1 - 1);
        if (op1 - 1 == 0) {
            regs->F |= FLAG_Z;
        }
        regs->F |= FLAG_N;
        break;
    }

    default:
        printf("error in DEC\n");
        exit(EXIT_FAILURE);
    }
};

void INC() {
    Instruction *instr = cpu.CurInstr;
    CPURegisters *regs = &cpu.Regs;
    switch (instr->Operand1) {
    case DT_A ... DT_L: {
        uint8_t *reg = getRegisterU8(instr->Operand1);
        if ((*reg & 0xF) + 1 > 0xF) {
            regs->F |= FLAG_H;
        }
        *reg += 1;
        if (*reg == 0) {
            regs->F |= FLAG_Z;
        }
        regs->F &= ~FLAG_N;
        break;
    }
    case DT_BC ... DT_PC: {
        writeRegisterU16(instr->Operand1, readRegisterU16(instr->Operand1) + 1);
        break;
    }
    case DT_A_HL: {
        uint16_t addr = readRegisterU16(instr->Operand1);
        uint8_t op1 = busRead(addr);
        if ((op1 & 0xF) + 1 > 0xF) {
            regs->F |= FLAG_H;
        }
        busWrite(addr, op1 + 1);
        if (op1 + 1 == 0) {
            regs->F |= FLAG_Z;
        }
        regs->F &= ~FLAG_N;
        break;
    }

    default:
        printf("error in INC\n");
        exit(EXIT_FAILURE);
    }
}

void ADD() {
    Instruction *instr = cpu.CurInstr;
    CPURegisters *regs = &cpu.Regs;
    uint16_t op2;

    switch (instr->Operand2) {
    case DT_A ... DT_L:
        op2 = *getRegisterU8(instr->Operand2);
        break;
    case DT_N8:
        op2 = cpu.InstrData[0];
        break;
    case DT_A_HL:
        op2 = busRead(readRegisterU16(instr->Operand2));
        break;
    case DT_AF ... DT_HL:
        op2 = readRegisterU16(DT_HL);
    case DT_E8:
        break;
    case DT_N16:
        op2 = ((uint16_t)cpu.InstrData[0] | ((uint16_t)cpu.InstrData[1] << 8));
        break;
    case DT_SP:
        op2 = regs->SP;
        break;
    default:
        printf("error in ADD\n");
        exit(EXIT_FAILURE);
    }

    switch (instr->Operand1) {
    case DT_A: {
        uint8_t *reg = &regs->A;
        if ((*reg & 0xF) + (op2 & 0xF) > 0xF) {
            regs->F |= FLAG_H;
        }
        if (*reg + op2 > 0xFF) {
            regs->F |= FLAG_C;
        }
        *reg += op2;
        if (*reg == 0) {
            regs->F |= FLAG_Z;
        }
        regs->F &= ~FLAG_N;

        break;
    }
    case DT_HL: {
        uint16_t regVal = readRegisterU16(instr->Operand1);
        writeRegisterU16(instr->Operand1, regVal + op2);
        if ((regVal & 0xFFF) + (op2 & 0xFFF) > 0xFFF) {
            regs->F |= FLAG_H;
        }
        if (regVal + op2 > 0xFFFF) {
            regs->F |= FLAG_C;
        }
        regs->F &= ~FLAG_N;
        break;
    }

    case DT_SP: {
        uint16_t regVal = readRegisterU16(instr->Operand1);
        int8_t value = ((int8_t *)cpu.InstrData)[0];
        if ((regVal & 0xF) + (value & 0xF) > 0xF) {
            regs->F |= FLAG_H;
        }
        if (regVal + value > 0xFF) {
            regs->F |= FLAG_C;
        }
        regs->F &= ~FLAG_Z;
        regs->F &= ~FLAG_N;
        regs->SP += value;
        break;
    }
    default:
        printf("error in ADD\n");
        exit(EXIT_FAILURE);
    }
}

void SUB() {
    Instruction *instr = cpu.CurInstr;
    CPURegisters *regs = &cpu.Regs;
    uint8_t val;
    switch (instr->Operand2) {
    case DT_A ... DT_L:
        val = *getRegisterU8(instr->Operand2);
        break;
    case DT_N8:
        val = cpu.InstrData[0];
        break;
    case DT_A_HL:
        val = busRead(readRegisterU16(instr->Operand2));
        break;
    default:
        printf("error in SUB\n");
        exit(EXIT_FAILURE);
    }

    uint8_t *reg = &regs->A;
    if ((int)(*reg & 0xF) - (int)(val & 0xF) < 0) {
        regs->F |= FLAG_H;
    }
    if (*reg < val) {
        regs->F |= FLAG_C;
    }
    *reg -= val;
    if (*reg == 0) {
        regs->F |= FLAG_Z;
    }
    regs->F |= FLAG_N;
}

void ADC() {
    CPURegisters *regs = &cpu.Regs;
    uint8_t val;
    switch (cpu.CurInstr->Operand2) {
    case DT_A ... DT_L:
        val = *getRegisterU8(cpu.CurInstr->Operand2);
        break;
    case DT_N8:
        val = cpu.InstrData[0];
        break;
    case DT_A_HL:
        val = busRead(readRegisterU16(DT_HL));
        break;
    default:
        printf("error in adc\n");
        exit(EXIT_FAILURE);
    }
    val += checkFlag(FLAG_C);
    if ((regs->A & 0xF) + (val & 0xF) > 0xF) {
        regs->F |= FLAG_H;
    }
    if (regs->A + val > 0xFF) {
        regs->F |= FLAG_C;
    }
    if (regs->A == 0) {
        regs->F |= FLAG_Z;
    }
    regs->F &= ~FLAG_N;
}

void JR() {
    Instruction *instr = cpu.CurInstr;
    int8_t data = ((int8_t *)cpu.InstrData)[0];
    switch (instr->Operand1) {
    case DT_E8:
        cpu.Regs.PC += data;
        break;
    case DT_CC_Z ... DT_CC_NC:
        if (CheckCondition(instr->Operand1)) {
            cpu.Regs.PC += data;
        }
        break;
    default:
        printf("error in JR\n");
        exit(EXIT_FAILURE);
    }
}

void CP() {
    CPURegisters *regs = &cpu.Regs;
    uint8_t val;
    switch (cpu.CurInstr->Operand2) {
    case DT_N8:
        val = cpu.InstrData[0];
        break;
    case DT_A ... DT_L:
        val = *getRegisterU8(cpu.CurInstr->Operand2);
        break;
    case DT_A_HL:
        val = busRead(readRegisterU16(cpu.CurInstr->Operand2));
        break;
    default:
        printf("error in CP");
        exit(EXIT_FAILURE);
    }
    if (regs->A - val == 0) {
        regs->F |= FLAG_Z;
    }
    regs->F |= FLAG_N;
    if (((int)regs->A & 0xF) - ((int)val & 0xF) < 0) {
        regs->F |= FLAG_H;
    }
    if (val > regs->A) {
        regs->F |= FLAG_C;
    }
    regs->F |= FLAG_N;
}

void CPL() {
    cpu.Regs.A = ~cpu.Regs.A;
    cpu.Regs.F |= FLAG_N;
    cpu.Regs.F |= FLAG_H;
}

void RRA() {

    CPURegisters *regs = &cpu.Regs;
    bool oldCarry = checkFlag(FLAG_C);
    regs->F = 0;
    if ((regs->A & 1) != 0) {
        regs->F = FLAG_C;
    }
    regs->A >>= 1;
    if (oldCarry) {
        regs->A |= 0b10000000;
    }
}

void RR() {
    CPURegisters *regs = &cpu.Regs;
    uint8_t op1;
    bool oldCarry = checkFlag(FLAG_C);
    switch (cpu.CurInstr->Operand1) {
    case DT_A ... DT_L:
        op1 = *getRegisterU8(cpu.CurInstr->Operand1);
        regs->F = 0;
        if ((op1 & 1) != 0) {
            regs->F = FLAG_C;
        }
        op1 >>= 1;
        if (oldCarry) {
            op1 |= 0b10000000;
        }
        if (op1 == 0) {
            regs->F |= FLAG_Z;
        }
        *getRegisterU8(cpu.CurInstr->Operand1) = op1;
        break;
    case DT_A_HL:
        op1 = busRead(readRegisterU16(DT_HL));
        regs->F = 0;
        if ((op1 & 1) != 0) {
            regs->F = FLAG_C;
        }
        op1 >>= 1;
        if (oldCarry) {
            op1 |= 0b10000000;
        }
        if (op1 == 0) {
            regs->F |= FLAG_Z;
        }
        busWrite(readRegisterU16(DT_HL), op1);
        break;
    default:
        printf("error in RR\n");
        exit(EXIT_FAILURE);
    }
}

void SRL() {
    CPURegisters *regs = &cpu.Regs;
    regs->F = 0;
    uint8_t op1;
    switch (cpu.CurInstr->Operand1) {
    case DT_A ... DT_L:
        op1 = *getRegisterU8(cpu.CurInstr->Operand1);
        if ((op1 & 1) != 0) {
            regs->F |= FLAG_C;
        }
        op1 >>= 1;
        op1 &= ~(1 << 7);
        if (op1 == 0) {
            regs->F |= FLAG_Z;
        }
        *getRegisterU8(cpu.CurInstr->Operand1) = op1;
        break;
    case DT_A_HL:
        op1 = busRead(readRegisterU16(DT_HL));
        if ((op1 & 1) != 0) {
            regs->F |= FLAG_C;
        }
        op1 >>= 1;
        op1 &= ~(1 << 7);
        if (op1 == 0) {
            regs->F |= FLAG_Z;
        }
        busWrite(busRead(readRegisterU16(DT_HL)), op1);
        break;
    default:
        printf("error in SRL\n");
        exit(EXIT_FAILURE);
    }
}

void CCF() {
    cpu.Regs.F &= ~FLAG_N;
    cpu.Regs.F &= ~FLAG_H;
    cpu.Regs.F ^= FLAG_C;
}

void SWAP() {
    CPURegisters *regs = &cpu.Regs;
    uint8_t op1;
    switch (cpu.CurInstr->Operand1) {
    case DT_A ... DT_L:
        op1 = *getRegisterU8(cpu.CurInstr->Operand1);
        op1 = (op1 >> 4) | (op1 << 4);
        regs->F = 0;
        if (op1 == 0) {
            regs->F |= FLAG_Z;
        }
        *getRegisterU8(cpu.CurInstr->Operand1) = op1;
        break;
    case DT_A_HL:
        op1 = busRead(readRegisterU16(cpu.CurInstr->Operand1));
        op1 = (op1 >> 4) | (op1 << 4);
        regs->F = 0;
        if (op1 == 0) {
            regs->F |= FLAG_Z;
        }
        busWrite(readRegisterU16(cpu.CurInstr->Operand1), op1);
        break;
    default:
        printf("error in SWAP\n");
        exit(EXIT_FAILURE);
    }
}

void RST() {
    CPURegisters *regs = &cpu.Regs;
    uint16_t addr;
    switch (cpu.CurInstr->Operand1) {
    case DT_RST0:
        addr = 0;
        break;
    case DT_RST8:
        addr = 0x8;
        break;
    case DT_RST10:
        addr = 0x10;
        break;
    case DT_RST18:
        addr = 0x18;
        break;
    case DT_RST20:
        addr = 0x20;
        break;
    case DT_RST28:
        addr = 0x28;
        break;
    case DT_RST30:
        addr = 0x30;
        break;
    case DT_RST38:
        addr = 0x38;
        break;
    default:
        printf("error in RST\n");
        exit(EXIT_FAILURE);
    }
    stackPush16(regs->PC);
    regs->PC = addr;
}
