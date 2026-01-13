#include "instructions.h"
#include "cpu.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void JP(CPU *cpu, Cartridge *cart) {
    bool b;
    switch (cpu->CurInstr->Operand1) {
    case DT_HL:
        cpu->Regs.PC = reverseEndian((uint16_t *)&cpu->Regs.H);
        return;
    case DT_A16:
        b = true;
        break;
    case DT_CC_Z:
        b = CheckFlag(cpu, FLAG_Z);
        break;
    case DT_CC_C:
        b = CheckFlag(cpu, FLAG_C);
        break;
    case DT_CC_NZ:
        b = !CheckFlag(cpu, FLAG_Z);
        break;
    case DT_CC_NC:
        b = !CheckFlag(cpu, FLAG_C);
        break;
    default:
        printf("error in JP\n");
        printf("%i\n", cpu->CurInstr->Operand1);

        exit(EXIT_FAILURE);
    }
    if (b) {
        uint16_t lo = cart->RomData[cpu->Regs.PC];
        uint16_t hi = cart->RomData[cpu->Regs.PC + 1];
        cpu->Regs.PC = lo | (hi << 8);
    }
}

void DI(CPU *cpu) { cpu->IMEFlag = false; }

void XOR(CPU *cpu, Cartridge *cart) {
    CPURegisters *regs = &cpu->Regs;
    switch (cpu->CurInstr->Operand2) {
    case DT_N8:
        regs->A ^= cart->RomData[cpu->Regs.PC];
        break;
    case DT_A:
    case DT_B:
    case DT_C:
    case DT_D:
    case DT_E:
    case DT_H:
    case DT_L:
        regs->A ^= *getRegisterU8(cpu, cpu->CurInstr->Operand2);
        break;
    case DT_A_HL:
        regs->A ^= cart->RomData[reverseEndian((uint16_t *)&regs->H)];
        break;
    default:
        printf("error in XOR");
        exit(EXIT_FAILURE);
    }
    regs->PC += cpu->CurInstr->Bytes - 1;
}

void LD(CPU *cpu, Cartridge *cart) {
    CPURegisters *regs = &cpu->Regs;
    Instruction *instr = cpu->CurInstr;
    uint8_t *op1 = NULL;
    uint8_t op2 = 0;
    uint16_t *op1U16 = NULL;
    uint16_t op2U16 = 0;

    switch (instr->Operand2) {
    case DT_A ... DT_L:
        op2 = *getRegisterU8(cpu, instr->Operand2);
        break;
    case DT_A_AF ... DT_A_HLD:
        op2 =
            cart->RomData[reverseEndian(getRegisterU16(cpu, instr->Operand2))];
        break;
    case DT_N8:
        op2 = cart->RomData[regs->PC];
        break;
    case DT_N16:
        op2 = cart->RomData[regs->PC] | (cart->RomData[regs->PC + 1] << 8);
		break;
    case DT_AF ... DT_PC:
        op2U16 = readRegisterU16(cpu, instr->Operand2);
		break;
    default:
        printf("error in LD\n");
        exit(EXIT_FAILURE);
    }

    switch (instr->Operand1) {
    case DT_A ... DT_L:
        op1 = getRegisterU8(cpu, instr->Operand1);
        *op1 = op2;
        break;
    case DT_A_AF ... DT_A_HLD:
        op1 =
            &cart->RomData[reverseEndian(getRegisterU16(cpu, instr->Operand1))];
        *op1 = op2;
        break;
    case DT_A16: {
        uint16_t addr = ((uint16_t)cart->RomData[regs->PC]) |
                        ((uint16_t)cart->RomData[regs->PC + 1] >> 8);
		op1 = &cart->RomData[addr];
        if (instr->Operand2 == DT_SP) {
			*((uint16_t*)op1) = regs->SP;
            break;
        }
        *op1 = op2;
        break;
    }
    case DT_AF ... DT_HLD:
        op1U16 = getRegisterU16(cpu, instr->Operand1);
		writeRegisterU16(cpu, instr->Operand1, op2U16);
        break;
    default:
        printf("error in LD\n");
        exit(EXIT_FAILURE);
    }
    regs->PC += instr->Bytes - 1;
}
