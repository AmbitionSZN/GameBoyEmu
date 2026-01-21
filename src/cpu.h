#pragma once

#include "cart.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "instructions.h"

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
	FLAG_Z = 0b10000000,
	FLAG_N = 0b01000000,
	FLAG_H = 0b00100000,
	FLAG_C = 0b00010000,
} Flag;

typedef enum {
    MNEM_NONE,
    MNEM_NOP,
    MNEM_LD,
    MNEM_INC,
    MNEM_DEC,
    MNEM_RLCA,
    MNEM_ADD,
    MNEM_RRCA,
    MNEM_STOP,
    MNEM_RLA,
    MNEM_JR,
    MNEM_RRA,
    MNEM_DAA,
    MNEM_CPL,
    MNEM_SCF,
    MNEM_CCF,
    MNEM_HALT,
    MNEM_ADC,
    MNEM_SUB,
    MNEM_SBC,
    MNEM_AND,
    MNEM_XOR,
    MNEM_OR,
    MNEM_CP,
    MNEM_POP,
    MNEM_JP,
    MNEM_PUSH,
    MNEM_RET,
    MNEM_CB,
    MNEM_CALL,
    MNEM_RETI,
    MNEM_LDH,
    MNEM_JPHL,
    MNEM_DI,
    MNEM_EI,
    MNEM_RST,
    MNEM_ERR,
	MNEM_PREFIX,
	MNEM_ILLEGAL_D3,
	MNEM_ILLEGAL_DB,
	MNEM_ILLEGAL_DD,
	MNEM_ILLEGAL_E3,
	MNEM_ILLEGAL_E4,
	MNEM_ILLEGAL_EB,
	MNEM_ILLEGAL_EC,
	MNEM_ILLEGAL_ED,
	MNEM_ILLEGAL_F4,
	MNEM_ILLEGAL_FC,
	MNEM_ILLEGAL_FD,
    // CB instructions...
    MNEM_RLC,
    MNEM_RRC,
    MNEM_RL,
    MNEM_RR,
    MNEM_SLA,
    MNEM_SRA,
    MNEM_SWAP,
    MNEM_SRL,
    MNEM_BIT,
    MNEM_RES,
    MNEM_SET,
} Mnemonic;

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
	DT_A_C,
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

typedef enum {
	INT_VBLANK = 0,
	INT_LCD = 1 << 1,
	INT_TIMER = 1 << 2,
	INT_SERIAL = 1 << 3,
	INT_JOYPAD = 1 << 4,

} Interrupt;

void opcodesJsonParser(char *file);

uint16_t reverseEndian(const uint16_t* n);


typedef struct {
	uint16_t Opcode;
    char *StrMnemonic;
	Mnemonic Mnem;
    uint8_t Bytes;
    uint8_t Cycles[2];
	DataType Operand1;
	DataType Operand2;
    FlagInstruction Flags[4];
    bool Immediate;
	bool Prefixed;
} Instruction;

typedef struct {
    CPURegisters Regs;
    Instruction* CurInstr;
	bool Halted;
	uint8_t* InstrData;
	bool IMEFlag;
	bool EnableIME;
	bool EnablingIME;
} CPU;



bool checkFlag(Flag flag);
bool CheckCondition(DataType condition);

void fetchInstruction();
void fetchData();
void execute();
void cpuStep();

uint8_t* getRegisterU8(DataType reg); 
uint16_t* getRegisterU16(DataType reg); 
uint16_t readRegisterU16(DataType reg);
void writeRegisterU16(DataType reg, uint16_t val);

void handleInterrupts();
