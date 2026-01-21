#include "cpu.h"
#include "bus.h"
#include "cJSON.h"
#include "cart.h"
#include "instructions.h"
#include "stack.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern CPU cpu;
extern uint8_t memory[0xFFFF];

Instruction instructions[512];

DataType getOperandType(cJSON *operand, char *mnemonic) {
    char *op = operand->child->valuestring;
    if (strcmp(op, "A") == 0) {
        return DT_A;
    }
    if (strcmp(op, "F") == 0) {
        return DT_F;
    }
    if (strcmp(op, "B") == 0) {
        return DT_B;
    }
    if (strcmp(op, "C") == 0) {
        if (strcmp(mnemonic, "CALL") == 0 || strcmp(mnemonic, "JP") == 0 ||
            strcmp(mnemonic, "JR") == 0 || strcmp(mnemonic, "RET") == 0) {
            return DT_CC_C;
        } else if (!cJSON_IsTrue(operand->child->next)) {
            return DT_A_C;
        }
        return DT_C;
    }
    if (strcmp(op, "D") == 0) {
        return DT_D;
    }
    if (strcmp(op, "E") == 0) {
        return DT_E;
    }
    if (strcmp(op, "H") == 0) {
        return DT_H;
    }
    if (strcmp(op, "L") == 0) {
        return DT_L;
    }
    if (strcmp(op, "AF") == 0) {
        if (cJSON_IsTrue(operand->child->next)) {
            return DT_AF;
        }
        return DT_A_AF;
    }
    if (strcmp(op, "BC") == 0) {
        if (cJSON_IsTrue(operand->child->next)) {
            return DT_BC;
        }
        return DT_A_BC;
    }
    if (strcmp(op, "DE") == 0) {
        if (cJSON_IsTrue(operand->child->next)) {
            return DT_DE;
        }
        return DT_A_DE;
    }
    if (strcmp(op, "HL") == 0) {
        bool immediate;
        if (strcmp(operand->child->next->string, "decrement") == 0) {
            immediate = cJSON_IsTrue(operand->child->next->next);
            if (immediate) {
                return DT_HLD;
            }
            return DT_A_HLD;
        }
        if (strcmp(operand->child->next->string, "increment") == 0) {
            immediate = cJSON_IsTrue(operand->child->next->next);
            if (immediate) {
                return DT_HLI;
            }
            return DT_A_HLI;
        }
        immediate = cJSON_IsTrue(operand->child->next);
        if (immediate) {
            return DT_HL;
        }
        return DT_A_HL;
    }

    if (strcmp(op, "SP") == 0) {
        return DT_SP;
    }
    if (strcmp(op, "PC") == 0) {
        return DT_PC;
    }
    if (strcmp(op, "n8") == 0) {
        return DT_N8;
    }
    if (strcmp(op, "n16") == 0) {
        return DT_N16;
    }
    if (strcmp(op, "e8") == 0) {
        return DT_E8;
    }
    if (strcmp(op, "Z") == 0) {
        return DT_CC_Z;
    }
    if (strcmp(op, "NZ") == 0) {
        return DT_CC_NZ;
    }
    if (strcmp(op, "NC") == 0) {
        return DT_CC_NC;
    }
    if (strcmp(op, "$00") == 0) {
        return DT_RST0;
    }
    if (strcmp(op, "$08") == 0) {
        return DT_RST8;
    }
    if (strcmp(op, "$10") == 0) {
        return DT_RST10;
    }
    if (strcmp(op, "$18") == 0) {
        return DT_RST18;
    }
    if (strcmp(op, "$20") == 0) {
        return DT_RST20;
    }
    if (strcmp(op, "$28") == 0) {
        return DT_RST28;
    }
    if (strcmp(op, "$30") == 0) {
        return DT_RST30;
    }
    if (strcmp(op, "$38") == 0) {
        return DT_RST38;
    }
    if (strcmp(op, "a8") == 0) {
        return DT_A8;
    }
    if (strcmp(op, "a16") == 0) {
        return DT_A16;
    }
    return DT_NONE;
}

Mnemonic getMnemonic(Instruction *instr) {

    if (instr->Opcode == 0) {
        return MNEM_NOP;
    } else if (strcmp(instr->StrMnemonic, "JP") == 0) {
        return MNEM_JP;
    } else if (strcmp(instr->StrMnemonic, "PUSH") == 0) {
        return MNEM_PUSH;
    } else if (strcmp(instr->StrMnemonic, "POP") == 0) {
        return MNEM_POP;
    } else if (strcmp(instr->StrMnemonic, "CALL") == 0) {
        return MNEM_CALL;
    } else if (strcmp(instr->StrMnemonic, "RET") == 0) {
        return MNEM_RET;
    } else if (strcmp(instr->StrMnemonic, "OR") == 0) {
        return MNEM_OR;
    } else if (strcmp(instr->StrMnemonic, "XOR") == 0) {
        return MNEM_XOR;
    } else if (strcmp(instr->StrMnemonic, "AND") == 0) {
        return MNEM_AND;
    } else if (strcmp(instr->StrMnemonic, "DI") == 0) {
        return MNEM_DI;
    } else if (strcmp(instr->StrMnemonic, "LD") == 0) {
        return MNEM_LD;
    } else if (strcmp(instr->StrMnemonic, "LDH") == 0) {
        return MNEM_LDH;
    } else if (strcmp(instr->StrMnemonic, "DEC") == 0) {
        return MNEM_DEC;
    } else if (strcmp(instr->StrMnemonic, "INC") == 0) {
        return MNEM_INC;
    } else if (strcmp(instr->StrMnemonic, "ADD") == 0) {
        return MNEM_ADD;
    } else if (strcmp(instr->StrMnemonic, "SUB") == 0) {
        return MNEM_SUB;
    } else if (strcmp(instr->StrMnemonic, "JR") == 0) {
        return MNEM_JR;
    } else if (strcmp(instr->StrMnemonic, "CP") == 0) {
        return MNEM_CP;
    } else if (strcmp(instr->StrMnemonic, "RRA") == 0) {
        return MNEM_RRA;
    } else if (strcmp(instr->StrMnemonic, "ADC") == 0) {
        return MNEM_ADC;
    } else if (strcmp(instr->StrMnemonic, "RLCA") == 0) {
        return MNEM_RLCA;
    } else if (strcmp(instr->StrMnemonic, "RRCA") == 0) {
        return MNEM_RRCA;
    } else if (strcmp(instr->StrMnemonic, "STOP") == 0) {
        return MNEM_STOP;
    } else if (strcmp(instr->StrMnemonic, "RLA") == 0) {
        return MNEM_RLA;
    } else if (strcmp(instr->StrMnemonic, "DAA") == 0) {
        return MNEM_DAA;
    } else if (strcmp(instr->StrMnemonic, "CPL") == 0) {
        return MNEM_CPL;
    } else if (strcmp(instr->StrMnemonic, "SCF") == 0) {
        return MNEM_SCF;
    } else if (strcmp(instr->StrMnemonic, "CCF") == 0) {
        return MNEM_CCF;
    } else if (strcmp(instr->StrMnemonic, "HALT") == 0) {
        return MNEM_HALT;
    } else if (strcmp(instr->StrMnemonic, "SBC") == 0) {
        return MNEM_SBC;
    } else if (strcmp(instr->StrMnemonic, "RST") == 0) {
        return MNEM_RST;
    } else if (strcmp(instr->StrMnemonic, "PREFIX") == 0) {
        return MNEM_PREFIX;
    } else if (strcmp(instr->StrMnemonic, "ILLEGAL_D3") == 0) {
        return MNEM_ILLEGAL_D3;
    } else if (strcmp(instr->StrMnemonic, "ILLEGAL_DB") == 0) {
        return MNEM_ILLEGAL_DB;
    } else if (strcmp(instr->StrMnemonic, "ILLEGAL_DD") == 0) {
        return MNEM_ILLEGAL_DD;
    } else if (strcmp(instr->StrMnemonic, "ILLEGAL_E3") == 0) {
        return MNEM_ILLEGAL_E3;
    } else if (strcmp(instr->StrMnemonic, "ILLEGAL_E4") == 0) {
        return MNEM_ILLEGAL_E4;
    } else if (strcmp(instr->StrMnemonic, "ILLEGAL_EB") == 0) {
        return MNEM_ILLEGAL_EB;
    } else if (strcmp(instr->StrMnemonic, "ILLEGAL_EC") == 0) {
        return MNEM_ILLEGAL_EC;
    } else if (strcmp(instr->StrMnemonic, "ILLEGAL_ED") == 0) {
        return MNEM_ILLEGAL_ED;
    } else if (strcmp(instr->StrMnemonic, "ILLEGAL_F4") == 0) {
        return MNEM_ILLEGAL_F4;
    } else if (strcmp(instr->StrMnemonic, "ILLEGAL_FC") == 0) {
        return MNEM_ILLEGAL_FC;
    } else if (strcmp(instr->StrMnemonic, "ILLEGAL_FD") == 0) {
        return MNEM_ILLEGAL_FD;
    } else if (strcmp(instr->StrMnemonic, "EI") == 0) {
        return MNEM_EI;
    } else if (strcmp(instr->StrMnemonic, "RETI") == 0) {
        return MNEM_RETI;
    } else if (strcmp(instr->StrMnemonic, "RLC") == 0) {
        return MNEM_RLC;
    } else if (strcmp(instr->StrMnemonic, "RRC") == 0) {
        return MNEM_RRC;
    } else if (strcmp(instr->StrMnemonic, "RL") == 0) {
        return MNEM_RL;
    } else if (strcmp(instr->StrMnemonic, "RR") == 0) {
        return MNEM_RR;
    } else if (strcmp(instr->StrMnemonic, "SLA") == 0) {
        return MNEM_SLA;
    } else if (strcmp(instr->StrMnemonic, "SRA") == 0) {
        return MNEM_SRA;
    } else if (strcmp(instr->StrMnemonic, "SWAP") == 0) {
        return MNEM_SWAP;
    } else if (strcmp(instr->StrMnemonic, "SRL") == 0) {
        return MNEM_SRL;
    } else if (strcmp(instr->StrMnemonic, "BIT") == 0) {
        return MNEM_BIT;
    } else if (strcmp(instr->StrMnemonic, "RES") == 0) {
        return MNEM_RES;
    } else if (strcmp(instr->StrMnemonic, "SET") == 0) {
        return MNEM_SET;
    } else {
        printf("error in GetMnemonic:\n");
        printf("\tOpcode: %2.2X\n", instr->Opcode);
        printf("\tMnemonic: %s\n", instr->StrMnemonic);
        exit(EXIT_FAILURE);
    }
}

void opcodesJsonParser(char *file) {
    FILE *fptr;
    char *str;
    size_t strSize;
    fptr = fopen(file, "rb");
    if (fptr == NULL) {
        printf("failed to open json file: %s", file);
        exit(EXIT_FAILURE);
    }
    fseek(fptr, 0, SEEK_END);
    strSize = ftell(fptr);
    rewind(fptr);
    str = malloc(strSize);
    fread(str, strSize, 1, fptr);
    fclose(fptr);
    cJSON *json = cJSON_Parse(str);
    cJSON *unprefixed = json->child;
    cJSON *prefixed = json->child->next;
    cJSON *opcode = NULL;
    size_t instrSetSize =
        cJSON_GetArraySize(unprefixed) + cJSON_GetArraySize(prefixed);
    for (size_t i = 0; i < instrSetSize; i++) {
        Instruction instruction;

        if (i == 256) {
            opcode = prefixed->child;
            instruction.Opcode = i - 256;
            instruction.Prefixed = true;
        } else if (i > 256) {
            opcode = opcode->next;
            instruction.Opcode = i - 256;
            instruction.Prefixed = true;
        } else if (!opcode) {
            opcode = unprefixed->child;
            instruction.Opcode = i;
            instruction.Prefixed = false;
        } else {
            opcode = opcode->next;
            instruction.Opcode = i;
            instruction.Prefixed = false;
        }

        size_t mnemonicLen =
            strlen(cJSON_GetObjectItem(opcode, "mnemonic")->valuestring);
        instruction.StrMnemonic = malloc(mnemonicLen + 1);
        strcpy(instruction.StrMnemonic,
               cJSON_GetObjectItem(opcode, "mnemonic")->valuestring);
        instruction.Mnem = getMnemonic(&instruction);

        instruction.Bytes = cJSON_GetObjectItem(opcode, "bytes")->valueint;
        cJSON *jsonCycles = cJSON_GetObjectItem(opcode, "cycles");
        if (cJSON_GetArraySize(jsonCycles) == 2) {
            instruction.Cycles[0] = jsonCycles->child->valueint / 4;
            instruction.Cycles[1] = jsonCycles->child->next->valueint / 4;
        } else {
            instruction.Cycles[0] = jsonCycles->child->valueint / 4;
            instruction.Cycles[1] = 0;
        }
        cJSON *jsonOperands = cJSON_GetObjectItem(opcode, "operands");
        size_t operSize = cJSON_GetArraySize(jsonOperands);
        if (operSize == 0) {
            instruction.Operand1 = DT_NONE;
            instruction.Operand2 = DT_NONE;
        }
        if (operSize == 1) {
            cJSON *op1 = jsonOperands->child;
            instruction.Operand1 = getOperandType(op1, instruction.StrMnemonic);
            instruction.Operand2 = DT_NONE;
        }
        if (operSize >= 2) {
            cJSON *op1 = jsonOperands->child;
            cJSON *op2 = jsonOperands->child->next;
            instruction.Operand1 = getOperandType(op1, instruction.StrMnemonic);
            instruction.Operand2 = getOperandType(op2, instruction.StrMnemonic);
        }
        cJSON *jsonFlags = cJSON_GetObjectItem(opcode, "flags");
        cJSON *jsonFlag = NULL;
        for (size_t j = 0; j < 4; j++) {
            if (!jsonFlag) {
                jsonFlag = jsonFlags->child;
            } else {
                jsonFlag = jsonFlag->next;
            }
            if (cJSON_IsString(jsonFlag) == false && jsonFlag->valueint == 0) {
                instruction.Flags[j] = FLAGINSTR_CLEAR;
                continue;
            }
            if (cJSON_IsString(jsonFlag) == false && jsonFlag->valueint == 1) {
                instruction.Flags[j] = FLAGINSTR_SET;
                continue;
            }
            if (jsonFlag->valuestring[0] == '0') {
                instruction.Flags[j] = FLAGINSTR_CLEAR;
                continue;
            }
            if (jsonFlag->valuestring[0] == '1') {
                instruction.Flags[j] = FLAGINSTR_SET;
                continue;
            }
            if (jsonFlag->valuestring[0] == '-') {
                instruction.Flags[j] = FLAGINSTR_NONE;
                continue;
            }
            if (jsonFlag->valuestring[0] == 'Z') {
                instruction.Flags[j] = FLAGINSTR_Z;
                continue;
            }
            if (jsonFlag->valuestring[0] == 'N') {
                instruction.Flags[j] = FLAGINSTR_N;
                continue;
            }
            if (jsonFlag->valuestring[0] == 'H') {
                instruction.Flags[j] = FLAGINSTR_H;
                continue;
            }
            if (jsonFlag->valuestring[0] == 'C') {
                instruction.Flags[j] = FLAGINSTR_C;
                continue;
            }
        }
        instructions[i] = instruction;
    }
    cJSON_Delete(json);
};

uint16_t reverseEndian(const uint16_t *n) {
    uint16_t foo = ((n[0]) >> 8) | ((n[0]) << 8);
    return foo;
}

bool checkFlag(Flag flag) { return (cpu.Regs.F & flag) != 0; }

bool CheckCondition(DataType condition) {
    switch (condition) {
    case DT_CC_Z:
        return checkFlag(FLAG_Z);
    case DT_CC_C:
        return checkFlag(FLAG_C);
    case DT_CC_NZ:
        return !checkFlag(FLAG_Z);
    case DT_CC_NC:
        return !checkFlag(FLAG_C);
    default:
        printf("error in CheckCondition\n");
        exit(EXIT_FAILURE);
    }
}

void fetchInstruction() {
    uint16_t opcode = busRead(cpu.Regs.PC);
    if (opcode == 0xCB) {
        cpu.Regs.PC++;
        opcode = (busRead(cpu.Regs.PC) + 0xFF);
        cpu.CurInstr = &instructions[opcode];
    } else {
        cpu.CurInstr = &instructions[opcode];
    }

    	if (cpu.CurInstr->Mnem != MNEM_NOP) {
    printf("=====\nFetched instruction:\n");
    printf("\tOpcode: %2.2X\n", cpu.CurInstr->Opcode);
    printf("\tMnemonic: %s\n", cpu.CurInstr->StrMnemonic);
    printf("\tPC: %X\n=====\n\n", cpu.Regs.PC);
    	}
}

void fetchData() {
    cpu.Regs.PC++;
    cpu.InstrData = NULL;
    if (cpu.CurInstr->Bytes == 1 || cpu.CurInstr->Prefixed) {
        return;
    }
    cpu.InstrData = &memory[cpu.Regs.PC];
    cpu.Regs.PC += cpu.CurInstr->Bytes - 1;
}

void execute() {
    Instruction *instr = cpu.CurInstr;
    switch (instr->Mnem) {
    case MNEM_NOP:
        break;
    case MNEM_JP:
        JP();
        break;
    case MNEM_PUSH:
        PUSH();
        break;
    case MNEM_POP:
        POP();
        break;
    case MNEM_CALL:
        CALL();
        break;
    case MNEM_RET:
        RET();
        break;
    case MNEM_OR:
        OR();
        break;
    case MNEM_XOR:
        XOR();
        break;
    case MNEM_AND:
        AND();
        break;
    case MNEM_DI:
        DI();
        break;
    case MNEM_EI:
        EI();
        break;
    case MNEM_LD:
        LD();
        break;
    case MNEM_LDH:
        LDH();
        break;
    case MNEM_DEC:
        DEC();
        break;
    case MNEM_INC:
        INC();
        break;
    case MNEM_SUB:
        SUB();
        break;
    case MNEM_ADD:
        ADD();
        break;
    case MNEM_JR:
        JR();
        break;
    case MNEM_CP:
        CP();
        break;
    case MNEM_CPL:
        CPL();
        break;
    case MNEM_RRA:
        RRA();
        break;
    case MNEM_RR:
        RR();
        break;
    case MNEM_ADC:
        ADC();
        break;
    case MNEM_SRL:
        SRL();
        break;
    case MNEM_CCF:
        CCF();
        break;
    case MNEM_SWAP:
        SWAP();
        break;
    case MNEM_RST:
        RST();
        break;
    default:
        printf("Instruction not implemented:\n");
        printf("\tOpcode: %2.2X\n", cpu.CurInstr->Opcode);
        printf("\tMnemonic: %s\n", cpu.CurInstr->StrMnemonic);
        printf("\tPC: %X\n", cpu.Regs.PC);
        exit(EXIT_FAILURE);
    }
}

void interruptHandle(uint16_t address) {
    stackPush16(cpu.Regs.PC);
    cpu.Regs.PC = address;
}

bool interruptCheck(uint16_t address, Interrupt it) {
    static const uint16_t IF = 0xFF0F;
    static const uint16_t IE = 0xFFFF;
    if (busRead(IF) & it && busRead(IE) & it) {
        interruptHandle(address);
        busWrite16(IF, busRead16(IF) & ~it);
        cpu.Halted = false;
        cpu.IMEFlag = false;

        return true;
    }

    return false;
}

void handleInterrupts() {
    if (interruptCheck(0x40, INT_VBLANK)) {
    } else if (interruptCheck(0x48, INT_LCD)) {

    } else if (interruptCheck(0x50, INT_TIMER)) {

    } else if (interruptCheck(0x58, INT_SERIAL)) {

    } else if (interruptCheck(0x60, INT_JOYPAD)) {
    }
}

void cpuStep() {
    if (!cpu.Halted) {

        fetchInstruction();
        fetchData();
        execute();
        if (cpu.IMEFlag) {
            handleInterrupts();
            cpu.IMEFlag = false;
        }

        if (cpu.EnablingIME) {
            cpu.IMEFlag = true;
        }
    } else {
		
	}
}

uint8_t *getRegisterU8(DataType reg) {
    CPURegisters *regs = &cpu.Regs;
    switch (reg) {
    case DT_A:
        return &regs->A;
    case DT_F:
        return &regs->F;
    case DT_B:
        return &regs->B;
    case DT_C:
        return &regs->C;
    case DT_D:
        return &regs->D;
    case DT_E:
        return &regs->E;
    case DT_H:
        return &regs->H;
    case DT_L:
        return &regs->L;
    default:
        printf("error in getRegisterU8\n");
        printf("%d\n", reg);
        exit(EXIT_FAILURE);
    }
    return NULL;
}

uint16_t *getRegisterU16(DataType reg) {
    CPURegisters *regs = &cpu.Regs;
    switch (reg) {
    case DT_A_AF:
    case DT_AF:
        return (uint16_t *)&regs->A;
    case DT_A_BC:
    case DT_BC:
        return (uint16_t *)&regs->B;
    case DT_A_DE:
    case DT_DE:
        return (uint16_t *)&regs->D;
    case DT_A_HL:
    case DT_A_HLI:
    case DT_A_HLD:
    case DT_HLI:
    case DT_HLD:
    case DT_HL:
        return (uint16_t *)&regs->H;
    case DT_SP:
        return &regs->SP;
    case DT_PC:
        return &regs->PC;
    default:
        printf("error in getRegisterU16");
        exit(EXIT_FAILURE);
    }
    printf("error in getRegisterU16");
    exit(EXIT_FAILURE);
}

uint16_t readRegisterU16(DataType reg) {
    CPURegisters *regs = &cpu.Regs;
    switch (reg) {
    case DT_A_AF:
    case DT_AF:
        return reverseEndian((uint16_t *)&regs->A);
    case DT_A_BC:
    case DT_BC:
        return reverseEndian((uint16_t *)&regs->B);
    case DT_A_DE:
    case DT_DE:
        return reverseEndian((uint16_t *)&regs->D);
    case DT_A_HL:
    case DT_A_HLI:
    case DT_A_HLD:
    case DT_HLI:
    case DT_HLD:
    case DT_HL:
        return reverseEndian((uint16_t *)&regs->H);
    case DT_SP:
        return regs->SP;
    case DT_PC:
        return regs->PC;
    default:
        printf("error in readRegisterU16\n");
        printf("enum: %i", reg);
        exit(EXIT_FAILURE);
    }
}

void writeRegisterU16(DataType reg, uint16_t val) {
    CPURegisters *regs = &cpu.Regs;
    switch (reg) {
    case DT_AF:
        *((uint16_t *)&regs->A) = reverseEndian((uint16_t *)&val);
        break;
    case DT_BC:
        *((uint16_t *)&regs->B) = reverseEndian((uint16_t *)&val);
        break;
    case DT_DE:
        *((uint16_t *)&regs->D) = reverseEndian((uint16_t *)&val);
        break;
    case DT_HL:
        *((uint16_t *)&regs->H) = reverseEndian((uint16_t *)&val);
        break;
    case DT_SP:
        regs->SP = val;
        break;
    case DT_PC:
        regs->PC = val;
        break;
    default:
        printf("error in writeRegisterU16");
        exit(EXIT_FAILURE);
    }
}
