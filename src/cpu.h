#pragma once

#include "cart.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint8_t A;
    uint8_t F;
    uint8_t B;
    uint8_t C;
    uint8_t D;
    uint8_t E;
    uint8_t H;
    uint8_t L;
    uint16_t PC;
    uint16_t SP;
} CPURegisters;

typedef enum {
    AM_IMP,
    AM_R_N16,
    AM_R_R,
    AM_MR_R,
    AM_R,
    AM_R_N8,
    AM_R_MR,
    AM_R_HLI,
    AM_R_HLD,
    AM_HLI_R,
    AM_HLD_R,
    AM_R_A8,
    AM_A8_R,
    AM_HL_SPR,
    AM_N16,
    AM_N8,
    AM_N16_R,
    AM_MR_N8,
    AM_MR,
    AM_A16_R,
    AM_R_A16,
    AM_E8_
} AddressingMode;

typedef enum {
	FLAG_Z = 0b10000000,
	FLAG_N = 0b01000000,
	FLAG_H = 0b00100000,
	FLAG_C = 0b00010000,
} Flag;

typedef enum {
    FLAGINSTR_NONE,
    FLAGINSTR_CLEAR,
    FLAGINSTR_SET,
    FLAGINSTR_Z,
    FLAGINSTR_N,
    FLAGINSTR_H,
    FLAGINSTR_C,
} FlagInstruction;

typedef enum {
    IN_NONE,
    IN_NOP,
    IN_LD,
    IN_INC,
    IN_DEC,
    IN_RLCA,
    IN_ADD,
    IN_RRCA,
    IN_STOP,
    IN_RLA,
    IN_JR,
    IN_RRA,
    IN_DAA,
    IN_CPL,
    IN_SCF,
    IN_CCF,
    IN_HALT,
    IN_ADC,
    IN_SUB,
    IN_SBC,
    IN_AND,
    IN_XOR,
    IN_OR,
    IN_CP,
    IN_POP,
    IN_JP,
    IN_PUSH,
    IN_RET,
    IN_CB,
    IN_CALL,
    IN_RETI,
    IN_LDH,
    IN_JPHL,
    IN_DI,
    IN_EI,
    IN_RST,
    IN_ERR,
    // CB instructions...
    IN_RLC,
    IN_RRC,
    IN_RL,
    IN_RR,
    IN_SLA,
    IN_SRA,
    IN_SWAP,
    IN_SRL,
    IN_BIT,
    IN_RES,
    IN_SET
} Mnemonic;

typedef enum {
    RT_NONE,
    RT_A,
    RT_F,
    RT_B,
    RT_C,
    RT_D,
    RT_E,
    RT_H,
    RT_L,
    RT_AF,
    RT_BC,
    RT_DE,
    RT_HL,
    RT_SP,
    RT_PC
} RegType;

typedef enum {
    DT_NONE,
    DT_A,
    DT_F,
    DT_B,
    DT_C,
    DT_D,
    DT_E,
    DT_H,
    DT_L,
    DT_AF,
    DT_BC,
    DT_DE,
    DT_HL,
    DT_SP,
    DT_PC,
	DT_HLI,
	DT_HLD,
    DT_N8,
    DT_N16,
    DT_E8,
	DT_CC_Z,
	DT_CC_C,
	DT_CC_NZ,
	DT_CC_NC,
	DT_RST0,
	DT_RST8,
	DT_RST10,
	DT_RST18,
	DT_RST20,
	DT_RST28,
	DT_RST30,
	DT_RST38,
	DT_A8,
	DT_A16,
	DT_A_AF,
	DT_A_BC,
	DT_A_DE,
	DT_A_HL,
	DT_A_HLI,
	DT_A_HLD,
} DataType;

typedef enum {
    RST_VEC0 = 0x00,
    RST_VEC8 = 0x08,
    RST_VEC10 = 0x10,
    RST_VEC18 = 0x18,
    RST_VEC20 = 0x20,
    RST_VEC28 = 0x28,
    RST_VEC30 = 0x30,
    RST_VEC38 = 0x38
} RSTVec;

void opcodesJsonParser(char *file);

uint16_t reverseEndian(const uint16_t* n);


typedef struct {
	uint16_t Opcode;
    char *Mnemonic;
    uint8_t Bytes;
    uint8_t Cycles[2];
	DataType Operand1;
	DataType Operand2;
    FlagInstruction Flags[4];
    bool Immediate;
    AddressingMode AddrMode;
} Instruction;

typedef struct {
    CPURegisters Regs;
    Instruction* CurInstr;
	bool IMEFlag;
} CPU;



bool CheckFlag(Flag flag);

void fetchInstruction();

void execute();

uint8_t* getRegisterU8(DataType reg); 
uint16_t* getRegisterU16(DataType reg); 
uint16_t readRegisterU16(DataType reg);
void writeRegisterU16(DataType reg, uint16_t val);
