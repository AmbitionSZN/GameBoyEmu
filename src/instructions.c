#include "instructions.h"
#include "cart.h"
#include "cpu.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern CPU cpu;
extern Cartridge cart;

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
        uint16_t lo = cart.RomData[cpu.Regs.PC];
        uint16_t hi = cart.RomData[cpu.Regs.PC + 1];
        cpu.Regs.PC = lo | (hi << 8);
    }
}

void DI() { cpu.IMEFlag = false; }

void XOR() {
    CPURegisters *regs = &cpu.Regs;
    switch (cpu.CurInstr->Operand2) {
    case DT_N8:
        regs->A ^= cart.RomData[cpu.Regs.PC];
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
        regs->A ^= cart.RomData[readRegisterU16(cpu.CurInstr->Operand2)];
        break;
    default:
        printf("error in XOR");
        exit(EXIT_FAILURE);
    }
    regs->PC += cpu.CurInstr->Bytes - 1;
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
    case DT_A_AF ... DT_A_HLD:
        op2 = cart.RomData[reverseEndian(getRegisterU16(instr->Operand2))];
        break;
    case DT_N8:
        op2 = cart.RomData[regs->PC];
        break;
    case DT_N16:
        op2U16 = cart.RomData[regs->PC] | (cart.RomData[regs->PC + 1] << 8);
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
        op2U16 = readRegisterU16(instr->Operand2) +
                 *((int8_t *)&cart.RomData[regs->PC]);
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
        op1 = &cart.RomData[readRegisterU16(instr->Operand1)];
        *op1 = op2;
        break;
    case DT_A16: {
        uint16_t addr = ((uint16_t)cart.RomData[regs->PC]) |
                        ((uint16_t)cart.RomData[regs->PC + 1] << 8);
        op1 = &cart.RomData[addr];
        if (instr->Operand2 == DT_SP) {
            *((uint16_t *)op1) = regs->SP;
            break;
        }
        *op1 = op2;
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
    regs->PC += instr->Bytes - 1;
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
        if (((int)cart.RomData[addr] & 0xF) - 1 < 0) {
            regs->F |= FLAG_H;
        }
        cart.RomData[addr] -= 1;
        if (cart.RomData[addr] == 0) {
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

void JR() {
    Instruction *instr = cpu.CurInstr;
    int8_t data = *((int8_t *)&cart.RomData[cpu.Regs.PC]);
    cpu.Regs.PC++;
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
