#include "instructions.h"
#include "bus.h"
#include "cart.h"
#include "cpu.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern CPU cpu;
extern uint8_t memory[0xFFFF];

void JP() {
    bool b;
    switch (cpu.CurInstr->Operand1) {
    case DT_HL:
        cpu.Regs.PC = reverseEndian((uint16_t *)&cpu.Regs.H);
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
    uint16_t *sp = &cpu.Regs.SP;
    switch (cpu.CurInstr->Operand1) {
    case DT_AF:
        sp[0]--;
        memory[sp[0]] = cpu.Regs.A;
        sp[0]--;
        memory[sp[0]] = cpu.Regs.F;
        break;
    case DT_BC ... DT_HL: {
        uint16_t val = readRegisterU16(cpu.CurInstr->Operand1);
        sp[0]--;
        memory[sp[0]] = (val >> 8);
        sp[0]--;
        memory[sp[0]] = (val & 0xFF);
        break;
    }
    default:
        printf("error in push\n");
        exit(EXIT_FAILURE);
    }
}

void POP() {
    uint16_t *sp = &cpu.Regs.SP;
    switch (cpu.CurInstr->Operand1) {
    case DT_AF:
        cpu.Regs.F = memory[sp[0]];
        sp[0]++;
        cpu.Regs.A = memory[sp[0]];
        sp[0]++;
        break;
    case DT_BC ... DT_HL: {
        uint16_t lo;
        uint16_t hi;
        lo = memory[sp[0]];
        sp[0]++;
        hi = memory[sp[0]];
        writeRegisterU16(cpu.CurInstr->Operand1, (lo | (hi << 8)));
        sp[0]++;
        break;
    }
    default:
        printf("error in push\n");
        exit(EXIT_FAILURE);
    }
}

void CALL() {
    uint16_t lo = cpu.InstrData[0];
    uint16_t hi = cpu.InstrData[1];
    uint16_t *sp = &cpu.Regs.SP;
    switch (cpu.CurInstr->Operand1) {

    case DT_A16: {
        uint16_t pc = cpu.Regs.PC;
        sp[0]--;
        memory[sp[0]] = (pc >> 8);
        sp[0]--;
        memory[sp[0]] = (pc & 0xFF);
        cpu.Regs.PC = (lo | (hi << 8));
        break;
    }
    case DT_CC_Z ... DT_CC_NC:
        if (CheckCondition(cpu.CurInstr->Operand1)) {
            uint16_t pc = cpu.Regs.PC;
            sp[0]--;
            memory[sp[0]] = (pc >> 8);
            sp[0]--;
            memory[sp[0]] = (pc & 0xFF);
            cpu.Regs.PC = (lo | (hi << 8));
        }
        break;
    default:
        printf("error in call\n");
        exit(EXIT_FAILURE);
    }
}

void RET() {
    uint16_t *sp = &cpu.Regs.SP;
    switch (cpu.CurInstr->Operand1) {
    case DT_NONE: {
        uint16_t lo;
        uint16_t hi;
        lo = memory[sp[0]];
        sp[0]++;
        hi = memory[sp[0]];
        writeRegisterU16(DT_PC, (lo | (hi << 8)));
        sp[0]++;
        break;
    }
    case DT_CC_Z ... DT_CC_NC:
        if (CheckCondition(cpu.CurInstr->Operand1)) {
            uint16_t lo;
            uint16_t hi;
            lo = memory[sp[0]];
            sp[0]++;
            hi = memory[sp[0]];
            writeRegisterU16(DT_PC, (lo | (hi << 8)));
            sp[0]++;
        }
        break;
    default:
        printf("error in ret");
        exit(EXIT_FAILURE);
    }
}

void DI() { cpu.IMEFlag = false; }

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
    case DT_A:
    case DT_B:
    case DT_C:
    case DT_D:
    case DT_E:
    case DT_H:
    case DT_L:
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
        op2 = memory[cpu.InstrData[0] | (cpu.InstrData[1] << 8)];
		break;
    case DT_A_AF ... DT_A_HLD:
        op2 = busRead(reverseEndian(getRegisterU16(instr->Operand2)));
        break;
    case DT_N8:
        op2 = cpu.InstrData[0];
        break;
    case DT_N16:
        op2U16 = cpu.InstrData[0] | (cpu.InstrData[1] << 8);
        break;
    case DT_AF ... DT_HL:
    case DT_PC:
        op2U16 = readRegisterU16(instr->Operand2);
        break;
    case DT_SP:
        if (instr->Operand1 == DT_HL) {
            op2U16 = readRegisterU16(instr->Operand2);
            break;
        }
        op2U16 =
            readRegisterU16(instr->Operand2) + ((int8_t *)cpu.InstrData)[0];
        break;
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
        op1 = &memory[readRegisterU16(instr->Operand1)];
        *op1 = op2;
        break;
    case DT_A16: {
        uint16_t addr =
            ((uint16_t)cpu.InstrData[0]) | ((uint16_t)cpu.InstrData[1] << 8);
        op1 = &memory[addr];
        if (instr->Operand2 == DT_SP) {
            op1[0] = regs->SP & 0xFF;
            op1[1] = regs->SP >> 8;
            break;
        }
        op1[0] = op2;
        break;
    }
    case DT_AF ... DT_HLD:
        op1U16 = getRegisterU16(instr->Operand1);
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
        data = memory[0xFF00 + data];
        break;
    case DT_A_C:
        data = memory[0xFF00 + cpu.Regs.C];
        break;
    case DT_A:
        data = memory[cpu.Regs.A];
        break;
    default:
        printf("error in LDH");
        exit(EXIT_FAILURE);
    }
    switch (cpu.CurInstr->Operand1) {
    case DT_A8:
        memory[0xFF00 + cpu.InstrData[0]] = data;
        break;
    case DT_A_C:
        memory[0xFF00 + cpu.Regs.C] = data;
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
        uint16_t *reg = getRegisterU16(instr->Operand1);
        writeRegisterU16(instr->Operand1, reverseEndian(reg) - 1);
        break;
    }
    case DT_A_HL: {
        uint16_t addr = readRegisterU16(instr->Operand1);
        if (((int)memory[addr] & 0xF) - 1 < 0) {
            regs->F |= FLAG_H;
        }
        memory[addr] -= 1;
        if (memory[addr] == 0) {
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
        uint16_t *reg = getRegisterU16(instr->Operand1);
        writeRegisterU16(instr->Operand1, reverseEndian(reg) + 1);
        break;
    }
    case DT_A_HL: {
        uint16_t addr = readRegisterU16(instr->Operand1);
        if ((memory[addr] & 0xF) + 1 > 0xF) {
            regs->F |= FLAG_H;
        }
        memory[addr] += 1;
        if (memory[addr] == 0) {
            regs->F |= FLAG_Z;
        }
        regs->F &= ~FLAG_N;
        break;
    }

    default:
        printf("error in DEC\n");
        exit(EXIT_FAILURE);
    }
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
        printf("error in XOR");
        exit(EXIT_FAILURE);
    }
    if (regs->A - val == 0) {
        regs->F |= FLAG_Z;
    }
    regs->F |= FLAG_N;
    if ((regs->A & 0xF) + (val & 0xF) > 0xF) {
        regs->F |= FLAG_H;
    }
    if (val > regs->A) {
        regs->F |= FLAG_C;
    }
}
