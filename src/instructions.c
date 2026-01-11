#include "instructions.h"
#include "cpu.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


void JP(CPU *cpu, Cartridge *cart) {
    bool b;
    switch (cpu->CurInstr->Operand1) {
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
