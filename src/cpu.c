#include "cpu.h"
#include "instructions.h"
#include "bus.h"
#include "cJSON.h"
#include "cart.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Instruction instructions[512];

DataType GetOperandType(cJSON *operand, char *mnemonic) {
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
        if (strcmp(operand->child->next->string, "increment") == 0) {
            if (cJSON_IsTrue(operand->child->next)) {
                return DT_HLI;
            }
            return DT_HLD;
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
    cJSON *opcode = NULL;
    for (size_t i = 0; i < cJSON_GetArraySize(unprefixed); i++) {
        Instruction instruction;
        instruction.Opcode = i;
        if (!opcode) {
            opcode = unprefixed->child;
        } else {
            opcode = opcode->next;
        }
        size_t mnemonicLen =
            strlen(cJSON_GetObjectItem(opcode, "mnemonic")->valuestring);
        instruction.Mnemonic = malloc(mnemonicLen + 1);
        strcpy(instruction.Mnemonic,
               cJSON_GetObjectItem(opcode, "mnemonic")->valuestring);

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
            instruction.Operand1 = GetOperandType(op1, instruction.Mnemonic);
            instruction.Operand2 = DT_NONE;
        }
        if (operSize == 2) {
            cJSON *op1 = jsonOperands->child;
            cJSON *op2 = jsonOperands->child->next;
            instruction.Operand1 = GetOperandType(op1, instruction.Mnemonic);
            instruction.Operand2 = GetOperandType(op2, instruction.Mnemonic);
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



void fetchInstruction(CPU *cpu, Cartridge *cart) {
    uint16_t opcode = busRead(cpu->Regs.PC, cart);
	if (opcode > 255) {
		printf("Instruction not implemented: %2.2X", opcode);
		exit(EXIT_FAILURE);
	}
    cpu->CurInstr = &instructions[opcode];
	cpu->Regs.PC++;
    printf("Opcode: %2.2X\n", cpu->CurInstr->Opcode);
    printf("Mnemonic: %s\n", cpu->CurInstr->Mnemonic);
    printf("PC: %X\n", cpu->Regs.PC);
}

bool CheckFlag(CPU *cpu, Flag flag) {
    return (cpu->Regs.F & (1 << flag)) != 0;
}

void execute(CPU *cpu, Cartridge *cart) {
	Instruction* instr = cpu->CurInstr;
	if (instr->Opcode == 0) {
		return;
	}
	if (strcmp(instr->Mnemonic, "JP") == 0) {
		JP(cpu, cart);
		return;
	}
}
