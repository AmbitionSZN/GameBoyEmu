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
void EI() { cpu.EnablingIME = true; }

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
    } else {
        regs->F &= ~FLAG_Z;
    }
    regs->F &= ~FLAG_N;
    regs->F &= ~FLAG_H;
    regs->F &= ~FLAG_C;
}

void XOR() {
    CPURegisters *regs = &cpu.Regs;
    uint16_t op2 = getOperandTwo();
    regs->A ^= op2;
    if (regs->A == 0) {
        regs->F |= FLAG_Z;
    } else {
        regs->F &= ~FLAG_Z;
    }
    regs->F &= ~FLAG_N;
    regs->F &= ~FLAG_H;
    regs->F &= ~FLAG_C;
}

void AND() {
    CPURegisters *regs = &cpu.Regs;
    uint16_t op2 = getOperandTwo();
    if (regs->A == 0) {
        regs->F |= FLAG_Z;
    } else {
        regs->F &= ~FLAG_Z;
    }
    regs->F &= ~FLAG_N;
    regs->F |= FLAG_H;
    regs->F &= ~FLAG_C;
}

void LD() {
    CPURegisters *regs = &cpu.Regs;
    Instruction *instr = cpu.CurInstr;
    uint16_t op2 = getOperandTwo();
    if (instr->Operand3) {
        int8_t op3 = ((int8_t *)&cpu.InstrData)[0];
        if ((op2 & 0xF) + (op3 & 0xF) > 0xF) {
            regs->F |= FLAG_H;
        } else {
            regs->F &= ~FLAG_H;
        }
        if (op2 + op3 > 0xFF) {
            regs->F |= FLAG_C;
        } else {
            regs->F &= ~FLAG_C;
        }
        regs->F &= ~FLAG_Z;
        regs->F &= ~FLAG_N;
        writeRegisterU16(DT_HL, op2 + op3);
        return;
    }

    switch (instr->Operand1) {
    case DT_A ... DT_L:
        *getRegisterU8(instr->Operand1) = op2;

        break;
    case DT_A_AF ... DT_A_HLD:

        busWrite(readRegisterU16(instr->Operand1), op2);
        break;
    case DT_A16: {
        uint16_t addr = ((uint16_t)(cpu.InstrData[0])) |
                        ((uint16_t)(cpu.InstrData[1]) << 8);
        if (instr->Operand2 == DT_SP) {
            busWrite16(addr, regs->SP);
            break;
        }

        busWrite(addr, op2);
        break;
    }
    case DT_AF ... DT_HLD:
        writeRegisterU16(instr->Operand1, op2);
        break;
    default:
        printf("error in LD\n");
        exit(EXIT_FAILURE);
    }
    if (instr->Operand1 == DT_HLI || instr->Operand2 == DT_HLI ||
        instr->Operand1 == DT_A_HLI || instr->Operand2 == DT_A_HLI) {
        uint16_t val = readRegisterU16(DT_HL) + 1;
        writeRegisterU16(DT_HL, val);
    }
    if (instr->Operand1 == DT_HLD || instr->Operand2 == DT_HLD ||
        instr->Operand1 == DT_A_HLD || instr->Operand2 == DT_A_HLD) {
        uint16_t val = readRegisterU16(DT_HL) - 1;
        writeRegisterU16(DT_HL, val);
    }
}

void LDH() {
    uint16_t op2 = getOperandTwo();
    switch (cpu.CurInstr->Operand1) {
    case DT_A8:
        busWrite((0xFF00 + (uint16_t)cpu.InstrData[0]), op2);
        break;
    case DT_A_C:
        busWrite((0xFF00 + (uint16_t)cpu.Regs.C), op2);
        break;
    case DT_A:
        cpu.Regs.A = op2;
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
        if ((int)(*reg & 0xF) - 1 < 0) {
            regs->F |= FLAG_H;
        } else {
            regs->F &= ~FLAG_H;
        }
        *reg -= 1;
        if (*reg == 0) {
            regs->F |= FLAG_Z;
        } else {
            regs->F &= ~FLAG_Z;
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
        if ((int)(op1 & 0xF) - 1 < 0) {
            regs->F |= FLAG_H;
        } else {
            regs->F &= ~FLAG_H;
        }
        busWrite(addr, op1 - 1);
        if (op1 - 1 == 0) {
            regs->F |= FLAG_Z;
        } else {
            regs->F &= ~FLAG_Z;
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
        } else {
            regs->F &= ~FLAG_H;
        }
        *reg += 1;
        if (*reg == 0) {
            regs->F |= FLAG_Z;
        } else {
            regs->F &= ~FLAG_Z;
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
        } else {
            regs->F &= ~FLAG_H;
        }
        busWrite(addr, op1 + 1);
        if (op1 + 1 == 0) {
            regs->F |= FLAG_Z;
        } else {
            regs->F &= ~FLAG_Z;
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
    uint16_t op2 = getOperandTwo();

    switch (instr->Operand1) {
    case DT_A: {
        uint8_t *reg = &regs->A;
        if ((*reg & 0xF) + (op2 & 0xF) > 0xF) {
            regs->F |= FLAG_H;
        } else {
            regs->F &= ~FLAG_H;
        }
        if (*reg + op2 > 0xFF) {
            regs->F |= FLAG_C;
        } else {
            regs->F &= ~FLAG_C;
        }
        *reg += op2;
        if (*reg == 0) {
            regs->F |= FLAG_Z;
        } else {
            regs->F &= ~FLAG_Z;
        }
        regs->F &= ~FLAG_N;

        break;
    }
    case DT_HL: {
        uint16_t regVal = readRegisterU16(instr->Operand1);
        writeRegisterU16(instr->Operand1, regVal + op2);
        if ((regVal & 0xFFF) + (op2 & 0xFFF) > 0xFFF) {
            regs->F |= FLAG_H;
        } else {
            regs->F &= ~FLAG_H;
        }
        if (regVal + op2 > 0xFFFF) {
            regs->F |= FLAG_C;
        } else {
            regs->F &= ~FLAG_C;
        }
        regs->F &= ~FLAG_N;
        break;
    }

    case DT_SP: {
        uint16_t regVal = readRegisterU16(instr->Operand1);
        int8_t value = ((int8_t *)cpu.InstrData)[0];
        if ((regVal & 0xF) + (value & 0xF) > 0xF) {
            regs->F |= FLAG_H;
        } else {
            regs->F &= ~FLAG_H;
        }
        if (regVal + value > 0xFF) {
            regs->F |= FLAG_C;
        } else {
            regs->F &= ~FLAG_C;
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
    uint16_t op2 = getOperandTwo();

    uint8_t *reg = &regs->A;
    if ((int)(*reg & 0xF) - (int)(op2 & 0xF) < 0) {
        regs->F |= FLAG_H;
    } else {
        regs->F &= ~FLAG_H;
    }
    if (*reg < op2) {
        regs->F |= FLAG_C;
    } else {
        regs->F &= ~FLAG_C;
    }
    *reg -= op2;
    if (*reg == 0) {
        regs->F |= FLAG_Z;
    } else {
        regs->F &= ~FLAG_Z;
    }
    regs->F |= FLAG_N;
}

void ADC() {
    CPURegisters *regs = &cpu.Regs;
    uint16_t val = getOperandTwo() + checkFlag(FLAG_C);
    if ((regs->A & 0xF) + (val & 0xF) > 0xF) {
        regs->F |= FLAG_H;
    } else {
        regs->F &= ~FLAG_H;
    }
    if (regs->A + val > 0xFF) {
        regs->F |= FLAG_C;
    } else {
        regs->F &= ~FLAG_C;
    }
    if (regs->A == 0) {
        regs->F |= FLAG_Z;
    } else {
        regs->F &= ~FLAG_Z;
    }
    regs->F &= ~FLAG_N;
}

void JR() {
    Instruction *instr = cpu.CurInstr;
    int8_t data = ((int8_t *)&cpu.InstrData)[0];
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
    uint16_t op2 = getOperandTwo();
    if (regs->A - op2 == 0) {
        regs->F |= FLAG_Z;
    } else {
        regs->F &= ~FLAG_Z;
    }
    regs->F |= FLAG_N;
    if ((int)(regs->A & 0xF) - (int)(op2 & 0xF) < 0) {
        regs->F |= FLAG_H;
    } else {
        regs->F &= ~FLAG_H;
    }
    if (op2 > regs->A) {
        regs->F |= FLAG_C;
    } else {
        regs->F &= ~FLAG_C;
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
    } else {
			regs->F &= ~FLAG_C;
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
        } else {
			regs->F &= ~FLAG_Z;
			}
        *getRegisterU8(cpu.CurInstr->Operand1) = op1;
        break;
    case DT_A_HL:
        op1 = busRead(readRegisterU16(cpu.CurInstr->Operand1));
        op1 = (op1 >> 4) | (op1 << 4);
        regs->F = 0;
        if (op1 == 0) {
            regs->F |= FLAG_Z;
        } else {
			regs->F &= ~FLAG_Z;
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
